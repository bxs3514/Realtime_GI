//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <PCH.h>

#include "Realtime_GI.h"
#include "SharedConstants.h"

#include "resource.h"
#include <InterfacePointers.h>
#include <Window.h>
#include <Graphics\\DeviceManager.h>
#include <Input.h>
#include <Graphics\\SpriteRenderer.h>
#include <Graphics\\Model.h>
#include <Utility.h>
#include <Graphics\\Camera.h>
#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\Profiler.h>
#include <Graphics\\Textures.h>
#include <Graphics\\Sampling.h>

using namespace SampleFramework11;
using std::wstring;

const uint32 WindowWidth = 1280;
const uint32 WindowHeight = 720;
const float WindowWidthF = static_cast<float>(WindowWidth);
const float WindowHeightF = static_cast<float>(WindowHeight);

static const float NearClip = 0.01f;
static const float FarClip = 100.0f;

static const float ModelScales[uint64(Scenes::NumValues)] = { 0.1f, 1.0f };
static const Float3 ModelPositions[uint64(Scenes::NumValues)] = { Float3(-1.0f, 2.0f, 0.0f), Float3(0.0f, 0.0f, 0.0f) };

// Model filenames
static const wstring ModelPaths[uint64(Scenes::NumValues)] =
{
    L"..\\Content\\Models\\RoboHand\\RoboHand.meshdata",
    L"",
};

Realtime_GI::Realtime_GI() :  App(L"Realtime GI (CSCI 580)", MAKEINTRESOURCEW(IDI_DEFAULT)),
                            camera(WindowWidthF / WindowHeightF, Pi_4 * 0.75f, NearClip, FarClip)
{
    deviceManager.SetBackBufferWidth(WindowWidth);
    deviceManager.SetBackBufferHeight(WindowHeight);
    deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);

    window.SetClientArea(WindowWidth, WindowHeight);
}

void Realtime_GI::BeforeReset()
{
    App::BeforeReset();
}

void Realtime_GI::AfterReset()
{
    App::AfterReset();

    float aspect = static_cast<float>(deviceManager.BackBufferWidth()) / deviceManager.BackBufferHeight();
    camera.SetAspectRatio(aspect);

    CreateRenderTargets();

    meshRenderer.CreateReductionTargets(deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());

    postProcessor.AfterReset(deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}

void Realtime_GI::Initialize()
{
    App::Initialize();

    ID3D11DevicePtr device = deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = deviceManager.ImmediateContext();

    // Create a font + SpriteRenderer
    font.Initialize(L"Arial", 18, SpriteFont::Regular, true, device);
    spriteRenderer.Initialize(device);

    // Camera setup
    camera.SetPosition(Float3(0.0f, 2.5f, -10.0f));

    // Load the scenes
    for(uint64 i = 0; i < uint64(Scenes::NumValues); ++i)
    {
        if(i == uint64(Scenes::Plane))
            models[i].GeneratePlaneScene(device, Float2(10.0f, 10.0f), Float3(), Quaternion(),
                                         L"", L"Bricks_NML.dds");
        else
            models[i].CreateFromMeshData(device, ModelPaths[i].c_str());
    }

    modelOrientations[uint64(Scenes::RoboHand)] = Quaternion(0.41f, -0.55f, -0.29f, 0.67f);
    AppSettings::ModelOrientation.SetValue(modelOrientations[AppSettings::CurrentScene]);

    meshRenderer.Initialize(device, deviceManager.ImmediateContext());
    meshRenderer.SetModel(&models[AppSettings::CurrentScene]);
    skybox.Initialize(device);

    envMap = LoadTexture(device, L"..\\Content\\EnvMaps\\Ennis.dds");

    FileReadSerializer serializer(L"..\\Content\\EnvMaps\\Ennis.shdata");
    SerializeItem(serializer, envMapSH);

    // Load shaders
    for(uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
    {
        CompileOptions opts;
        opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
        resolvePS[msaaMode] = CompilePSFromFile(device, L"Resolve.hlsl", "ResolvePS", "ps_5_0", opts);
    }

    resolveVS = CompileVSFromFile(device, L"Resolve.hlsl", "ResolveVS");

    backgroundVelocityVS = CompileVSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityVS");
    backgroundVelocityPS = CompilePSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityPS");

    resolveConstants.Initialize(device);
    backgroundVelocityConstants.Initialize(device);

    // Init the post processor
    postProcessor.Initialize(device);
}

// Creates all required render targets
void Realtime_GI::CreateRenderTargets()
{
    ID3D11Device* device = deviceManager.Device();
    uint32 width = deviceManager.BackBufferWidth();
    uint32 height = deviceManager.BackBufferHeight();

    const uint32 NumSamples = AppSettings::NumMSAASamples();
    const uint32 Quality = NumSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    colorTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples, Quality);
    depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D32_FLOAT, true, NumSamples, Quality);
    velocityTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16_FLOAT, true, NumSamples, Quality);

    if(resolveTarget.Width != width || resolveTarget.Height != height)
    {
        resolveTarget.Initialize(device, width, height, colorTarget.Format);
        velocityResolveTarget.Initialize(device, width, height, velocityTarget.Format);
        prevFrameTarget.Initialize(device, width, height, colorTarget.Format);
    }
}

void Realtime_GI::Update(const Timer& timer)
{
    AppSettings::UpdateUI();

    MouseState mouseState = MouseState::GetMouseState(window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(window);

    if(kbState.IsKeyDown(KeyboardState::Escape))
        window.Destroy();

    float CamMoveSpeed = 5.0f * timer.DeltaSecondsF();
    const float CamRotSpeed = 0.180f * timer.DeltaSecondsF();
    const float MeshRotSpeed = 0.180f * timer.DeltaSecondsF();

    // Move the camera with keyboard input
    if(kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

    Float3 camPos = camera.Position();
    if(kbState.IsKeyDown(KeyboardState::W))
        camPos += camera.Forward() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::S))
        camPos += camera.Back() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::A))
        camPos += camera.Left() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::D))
        camPos += camera.Right() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::Q))
        camPos += camera.Up() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::E))
        camPos += camera.Down() * CamMoveSpeed;
    camera.SetPosition(camPos);

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = camera.XRotation();
        float yRot = camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        camera.SetXRotation(xRot);
        camera.SetYRotation(yRot);
    }

    // Reset the camera projection
    camera.SetAspectRatio(camera.AspectRatio());
    Float2 jitter = 0.0f;
    if(AppSettings::EnableTemporalAA && AppSettings::EnableJitter() && AppSettings::UseStandardResolve == false)
    {
        const float jitterScale = 0.5f;

        if(AppSettings::JitterMode == JitterModes::Uniform2x)
        {
            jitter = frameCount % 2 == 0 ? -0.5f : 0.5f;
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersly16)
        {
            uint64 idx = frameCount % 16;
            jitter = Hammersley2D(idx, 16) - Float2(0.5f);
        }

        jitter *= jitterScale;

        const float offsetX = jitter.x * (1.0f / colorTarget.Width);
        const float offsetY = jitter.y * (1.0f / colorTarget.Height);
        Float4x4 offsetMatrix = Float4x4::TranslationMatrix(Float3(offsetX, -offsetY, 0.0f));
        camera.SetProjection(camera.ProjectionMatrix() * offsetMatrix);
    }

    jitterOffset = (jitter - prevJitter) * 0.5f;
    prevJitter = jitter;

    // Toggle VSYNC
    if(kbState.RisingEdge(KeyboardState::V))
        deviceManager.SetVSYNCEnabled(!deviceManager.VSYNCEnabled());

    deviceManager.SetNumVSYNCIntervals(AppSettings::DoubleSyncInterval ? 2 : 1);

    if(AppSettings::CurrentScene.Changed())
    {
        meshRenderer.SetModel(&models[AppSettings::CurrentScene]);
        AppSettings::ModelOrientation.SetValue(modelOrientations[AppSettings::CurrentScene]);
    }

    Quaternion orientation = AppSettings::ModelOrientation;
    orientation = orientation * Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), AppSettings::ModelRotationSpeed * timer.DeltaSecondsF());
    AppSettings::ModelOrientation.SetValue(orientation);

    modelTransform = orientation.ToFloat4x4() * Float4x4::ScaleMatrix(ModelScales[AppSettings::CurrentScene]);
    modelTransform.SetTranslation(ModelPositions[AppSettings::CurrentScene]);
}

void Realtime_GI::RenderAA()
{
    PIXEvent pixEvent(L"MSAA Resolve + Temporal AA");

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    ID3D11ShaderResourceView* velocitySRV = velocityTarget.SRView;
    if(AppSettings::NumMSAASamples() > 1)
    {
        context->ResolveSubresource(velocityResolveTarget.Texture, 0, velocityTarget.Texture, 0, velocityTarget.Format);
        velocitySRV = velocityResolveTarget.SRView;
    }


    ID3D11RenderTargetView* rtvs[1] = { resolveTarget.RTView };

    context->OMSetRenderTargets(1, rtvs, nullptr);

    if(AppSettings::UseStandardResolve)
    {
        if(AppSettings::MSAAMode == 0)
            context->CopyResource(resolveTarget.Texture, colorTarget.Texture);
        else
            context->ResolveSubresource(resolveTarget.Texture, 0, colorTarget.Texture, 0, colorTarget.Format);
        return;
    }

    const uint32 SampleRadius = static_cast<uint32>((AppSettings::FilterSize / 2.0f) + 0.499f);
    ID3D11PixelShader* pixelShader = resolvePS[AppSettings::MSAAMode];
    context->PSSetShader(pixelShader, nullptr, 0);
    context->VSSetShader(resolveVS, nullptr, 0);

    resolveConstants.Data.TextureSize = Float2(static_cast<float>(colorTarget.Width), static_cast<float>(colorTarget.Height));
    resolveConstants.Data.SampleRadius = SampleRadius;;
    resolveConstants.ApplyChanges(context);
    resolveConstants.SetPS(context, 0);

    ID3D11ShaderResourceView* srvs[3] = { colorTarget.SRView, prevFrameTarget.SRView, velocitySRV };
    context->PSSetShaderResources(0, 3, srvs);

    ID3D11SamplerState* samplers[1] = { samplerStates.LinearClamp() };
    context->PSSetSamplers(0, 1, samplers);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);

    srvs[0] = srvs[1] = srvs[2] = nullptr;
    context->PSSetShaderResources(0, 3, srvs);

    context->CopyResource(prevFrameTarget.Texture, resolveTarget.Texture);
}

void Realtime_GI::Render(const Timer& timer)
{
    if(AppSettings::MSAAMode.Changed())
        CreateRenderTargets();

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    AppSettings::UpdateCBuffer(context);

    RenderScene();

    RenderBackgroundVelocity();

    RenderAA();

    {
        // Kick off post-processing
        PIXEvent pixEvent(L"Post Processing");
        postProcessor.Render(context, resolveTarget.SRView, deviceManager.BackBuffer(), timer.DeltaSecondsF());
    }

    ID3D11RenderTargetView* renderTargets[1] = { deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());

    RenderHUD();

    ++frameCount;
}

void Realtime_GI::RenderScene()
{
    PIXEvent event(L"Render Scene");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    SetViewport(context, colorTarget.Width, colorTarget.Height);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(colorTarget.RTView, clearColor);
    context->ClearRenderTargetView(velocityTarget.RTView, clearColor);
    context->ClearDepthStencilView(depthBuffer.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ID3D11RenderTargetView* renderTargets[2] = { nullptr, nullptr };
    context->OMSetRenderTargets(1, renderTargets, depthBuffer.DSView);
    meshRenderer.RenderDepth(context, camera, modelTransform, false);

    meshRenderer.ReduceDepth(context, depthBuffer, camera);
    meshRenderer.RenderShadowMap(context, camera, modelTransform);

    renderTargets[0] = colorTarget.RTView;
    renderTargets[1] = velocityTarget.RTView;
    context->OMSetRenderTargets(2, renderTargets, depthBuffer.DSView);

    meshRenderer.Render(context, camera, modelTransform, envMap, envMapSH, jitterOffset);

    renderTargets[0] = colorTarget.RTView;
    renderTargets[1] = nullptr;
    context->OMSetRenderTargets(2, renderTargets, depthBuffer.DSView);

    if(AppSettings::RenderBackground)
        skybox.RenderEnvironmentMap(context, envMap, camera.ViewMatrix(), camera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));

    renderTargets[0] = renderTargets[1] = nullptr;
    context->OMSetRenderTargets(2, renderTargets, nullptr);
}

void Realtime_GI::RenderBackgroundVelocity()
{
    PIXEvent pixEvent(L"Render Background Velocity");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    SetViewport(context, velocityTarget.Width, velocityTarget.Height);

    // Don't use camera translation for background velocity
    FirstPersonCamera tempCamera = camera;
    tempCamera.SetPosition(Float3(0.0f, 0.0f, 0.0f));

    backgroundVelocityConstants.Data.InvViewProjection = Float4x4::Transpose(Float4x4::Invert(tempCamera.ViewProjectionMatrix()));
    backgroundVelocityConstants.Data.PrevViewProjection = Float4x4::Transpose(prevViewProjection);
    backgroundVelocityConstants.Data.RTSize.x = float(velocityTarget.Width);
    backgroundVelocityConstants.Data.RTSize.y = float(velocityTarget.Height);
    backgroundVelocityConstants.Data.JitterOffset = jitterOffset;
    backgroundVelocityConstants.ApplyChanges(context);
    backgroundVelocityConstants.SetPS(context, 0);

    prevViewProjection = tempCamera.ViewProjectionMatrix();

    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthEnabled(), 0);
    context->RSSetState(rasterizerStates.NoCull());

    ID3D11RenderTargetView* rtvs[1] = { velocityTarget.RTView };
    context->OMSetRenderTargets(1, rtvs, depthBuffer.DSView);

    context->VSSetShader(backgroundVelocityVS, nullptr, 0);
    context->PSSetShader(backgroundVelocityPS, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);
}

void Realtime_GI::RenderHUD()
{
    PIXEvent pixEvent(L"HUD Pass");

    spriteRenderer.Begin(deviceManager.ImmediateContext(), SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText(L"FPS: ");
    fpsText += ToString(fps) + L" (" + ToString(1000.0f / fps) + L"ms)";
    spriteRenderer.RenderText(font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    transform._42 += 25.0f;
    wstring vsyncText(L"VSYNC (V): ");
    vsyncText += deviceManager.VSYNCEnabled() ? L"Enabled" : L"Disabled";
    spriteRenderer.RenderText(font, vsyncText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    Profiler::GlobalProfiler.EndFrame(spriteRenderer, font);

    spriteRenderer.End();
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Realtime_GI app;
    app.Run();
}