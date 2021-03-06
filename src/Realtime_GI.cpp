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

#include "ProbeManager.h"
#include "Light.h"
#include "ShadowMapSettings.h"
#include "LoadScenes.h"

using namespace SampleFramework11;
using std::wstring;

const uint32 WindowWidth = 1280;
const uint32 WindowHeight = 720;
const float WindowWidthF = static_cast<float>(WindowWidth);
const float WindowHeightF = static_cast<float>(WindowHeight);

static const float NearClip = 0.01f;
static const float FarClip = 300.0f;


Realtime_GI::Realtime_GI() :  App(L"Realtime GI (CSCI 580)", MAKEINTRESOURCEW(IDI_DEFAULT)),
                            _camera(WindowWidthF / WindowHeightF, Pi_4 * 0.75f, NearClip, FarClip), 
							_prevForward(0.0f), _prevStrafe(0.0f), _prevAscend(0.0f), _numScenes(0)
{
    _deviceManager.SetBackBufferWidth(WindowWidth);
    _deviceManager.SetBackBufferHeight(WindowHeight);
    _deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);

    _window.SetClientArea(WindowWidth, WindowHeight);
}

void Realtime_GI::BeforeReset()
{
    App::BeforeReset();
}

void Realtime_GI::AfterReset()
{
	App::AfterReset();

	float aspect = static_cast<float>(_deviceManager.BackBufferWidth()) / _deviceManager.BackBufferHeight();
	_camera.SetAspectRatio(aspect);

	CreateRenderTargets();

	_meshRenderer.CreateReductionTargets(_deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());

	_postProcessor.AfterReset(_deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());
}

void Realtime_GI::AddScene(SceneScript *sceneScript)
{
	_scenes[_numScenes] = Scene();
	Scene *scene = &_scenes[_numScenes];
	scene->Initialize(_deviceManager.Device(), _deviceManager.ImmediateContext(), sceneScript, &_camera);

	_numScenes++;
}
//
//void Realtime_GI::LoadScenes(ID3D11DevicePtr device)
//{
//	// TODO: put the following in json
//	static const wstring ModelPaths[uint64(Scenes::NumValues)] =
//	{
//		// L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza.obj",
//		//L"C:\\Users\\wqxho_000\\Downloads\\Cerberus_by_Andrew_Maximov\\Cerberus_by_Andrew_Maximov\\testfbxascii.fbx",
//		L"..\\Content\\Models\\CornellBox\\CornellBox_fbx.FBX",
//		// L"..\\Content\\Models\\CornellBox\\TestPlane.FBX",
//		// L"..\\Content\\Models\\CornellBox\\CornellBox_Max.obj",
//		//L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaNon_PBR\\Converted\\sponza.obj",
//		// L"..\\Content\\Models\\Powerplant\\Powerplant.sdkmesh",
//		// L"..\\Content\\Models\\RoboHand\\RoboHand.meshdata",
//
//		L"..\\Content\\Models\\CornellBox\\UVUnwrapped\\cbox_unwrapped.FBX",
//		// L"",
//	};
//
//	Scene *scene = nullptr;
//
//
//	/// Scene 1 /////////////////////////////////////////////////////////////
//	_scenes[_numScenes] = Scene();
//	scene = &_scenes[_numScenes];
//	scene->Initialize(device, _deviceManager.ImmediateContext());
//
//	Model *m = scene->addModel(ModelPaths[0]);
//	scene->addStaticOpaqueObject(m, 0.1f, Float3(0, 0, 0), Quaternion());
//
//	scene->setProxySceneObject(ModelPaths[1], 0.1f, Float3(0, 0, 0), Quaternion());
//	// scene->fillPointLightsUniformGrid(50.0f, 100.0f);
//	// scene->fillPointLightsUniformGrid(1.3f, 3.0f, Float3(0, 0, 0));
//
//	_numScenes++;
//	/// Scene 2 /////////////////////////////////////////////////////////////
//	_scenes[_numScenes] = Scene();
//	scene = &_scenes[_numScenes];
//	scene->Initialize(device, _deviceManager.ImmediateContext());
//
//	scene->addDynamicOpaqueBoxObject(1.0f, Float3(-1.0f, 0.0f, 0.0f), Quaternion(0.0f, 1.0f, 0.0f, 0.3f));
//	scene->addDynamicOpaqueBoxObject(1.5f, Float3(0.0f, 0.0f, 0.0f), Quaternion(-0.7f, 1.0f, 0.0f, 0.3f));
//	scene->addDynamicOpaqueBoxObject(2.0f, Float3(1.0f, 0.0f, 0.0f), Quaternion(0.0f, 1.0f, 0.7f, 0.3f));
//	_numScenes++;
//
//	/// Scene 3 /////////////////////////////////////////////////////////////
//	_scenes[_numScenes] = Scene();
//	scene = &_scenes[_numScenes];
//	scene->Initialize(device, _deviceManager.ImmediateContext());
//
//	scene->addDynamicOpaquePlaneObject(10.0f, Float3(0.0f, 0.0f, 0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
//	_numScenes++;
//}

void Realtime_GI::LoadShaders(ID3D11DevicePtr device)
{
	// Load shaders
	// TODO: add load shader caption
	for (uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
	{
		CompileOptions opts;
		opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
		_resolvePS[msaaMode] = CompilePSFromFile(device, L"Resolve.hlsl", "ResolvePS", "ps_5_0", opts);
	}

	_resolveVS = CompileVSFromFile(device, L"Resolve.hlsl", "ResolveVS");

	_backgroundVelocityVS = CompileVSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityVS");
	_backgroundVelocityPS = CompilePSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityPS");

	_clusteredDeferredVS = CompileVSFromFile(device, L"ClusteredDeferred.hlsl", "ClusteredDeferredVS");
	_clusteredDeferredPS = CompilePSFromFile(device, L"ClusteredDeferred.hlsl", "ClusteredDeferredPS");

	_samplerStates.Initialize(device);
}

void Realtime_GI::Initialize()
{
    App::Initialize();

    ID3D11DevicePtr device = _deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = _deviceManager.ImmediateContext();

    // Create a font + SpriteRenderer
    _font.Initialize(L"Arial", 18, SpriteFont::Regular, true, device);
    _spriteRenderer.Initialize(device);

    // Camera setup
    _camera.SetPosition(Float3(0.0f, 2.5f, -10.0f));

	LoadScenes();

	_debugRenderer.Initialize(&_deviceManager, _deviceManager.Device(), _deviceManager.ImmediateContext(), &_camera);

	_prevScene = &_scenes[AppSettings::CurrentScene];

    _meshRenderer.Initialize(device, _deviceManager.ImmediateContext(), &_debugRenderer);
    _meshRenderer.SetScene(&_scenes[AppSettings::CurrentScene]);

    _skybox.Initialize(device);

    _envMap = LoadTexture(device, L"..\\Content\\EnvMaps\\Ennis.dds");

    FileReadSerializer serializer(L"..\\Content\\EnvMaps\\Ennis.shdata");
    SerializeItem(serializer, _envMapSH);

	LoadShaders(device);
   
    _resolveConstants.Initialize(device);
    _backgroundVelocityConstants.Initialize(device);
	_deferredPassConstants.Initialize(device);

    // Init the post processor
    _postProcessor.Initialize(device);

	CreateLightBuffers();
	CreateQuadBuffers();

	_lightClusters.Initialize(_deviceManager.Device(), _deviceManager.ImmediateContext(), &_irradianceVolume);
	_lightClusters.SetScene(&_scenes[AppSettings::CurrentScene]);

	_irradianceVolume.Initialize(_deviceManager.Device(), _deviceManager.ImmediateContext(), 
		&_meshRenderer, &_camera, &_pointLightBuffer, &_lightClusters, &_debugRenderer, &_shProbeLightBuffer);

	_irradianceVolume.SetScene(&_scenes[AppSettings::CurrentScene]);

	_ssr.Initialize(_deviceManager.Device(), _deviceManager.ImmediateContext(), &_camera, &_colorTarget, &_rt1Target, &_rt2Target
		,&_depthBuffer, &_ssrTarget, _quadVB, _quadIB);


	UpdateSpecularProbeUIInfo();
}

void Realtime_GI::UpdateSpecularProbeUIInfo()
{
	if (_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums() != 0)
	{
		CreateCubemap *cubeMap;
		for (uint32 probeIndex = 0; probeIndex < _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums(); ++probeIndex)
		{
			_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbe(&cubeMap, probeIndex);

			if (probeIndex == AppSettings::ProbeIndex.Value())
			{
				Float3 pos = cubeMap->GetPosition();
				Float3 boxSize = cubeMap->GetBoxSize();

				AppSettings::ProbeX.SetValue(pos.x);
				AppSettings::ProbeY.SetValue(pos.y);
				AppSettings::ProbeZ.SetValue(pos.z);

				AppSettings::BoxSizeX.SetValue(boxSize.x);
				AppSettings::BoxSizeY.SetValue(boxSize.y);
				AppSettings::BoxSizeZ.SetValue(boxSize.z);
			}
		}
	}
}

void Realtime_GI::UpdateSpecularProbeProperties()
{
	if (_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums() != 0)
	{
		CreateCubemap *cubeMap;
		for (uint32 probeIndex = 0; probeIndex < _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums(); ++probeIndex)
		{
			_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbe(&cubeMap, probeIndex);

			if (probeIndex == AppSettings::ProbeIndex.Value())
			{
				Float3 pos = cubeMap->GetPosition();
				Float3 boxSize = cubeMap->GetBoxSize();

				if (AppSettings::ProbeIndex.Changed()){
					if (AppSettings::ProbeX != pos.x || AppSettings::ProbeY != pos.y || AppSettings::ProbeZ != pos.z
						|| AppSettings::BoxSizeX != boxSize.x || AppSettings::BoxSizeY != boxSize.y || AppSettings::BoxSizeZ != boxSize.z)
					{
						AppSettings::ProbeX.SetValue(pos.x);
						AppSettings::ProbeY.SetValue(pos.y);
						AppSettings::ProbeZ.SetValue(pos.z);

						AppSettings::BoxSizeX.SetValue(boxSize.x);
						AppSettings::BoxSizeY.SetValue(boxSize.y);
						AppSettings::BoxSizeZ.SetValue(boxSize.z);
					}
				}
				else
				{
					if (AppSettings::EnableRealtimeCubemap.Changed())
					{
						AppSettings::ProbeX.SetValue(pos.x);
						AppSettings::ProbeY.SetValue(pos.y);
						AppSettings::ProbeZ.SetValue(pos.z);

						AppSettings::BoxSizeX.SetValue(boxSize.x);
						AppSettings::BoxSizeY.SetValue(boxSize.y);
						AppSettings::BoxSizeZ.SetValue(boxSize.z);
					}
					cubeMap->SetPosition(Float3(AppSettings::ProbeX, AppSettings::ProbeY, AppSettings::ProbeZ));
					_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->_probes.at(probeIndex).pos = 
						Float3(AppSettings::ProbeX, AppSettings::ProbeY, AppSettings::ProbeZ);
					cubeMap->SetBoxSize(Float3(AppSettings::BoxSizeX, AppSettings::BoxSizeY, AppSettings::BoxSizeZ));
					_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->_probes.at(probeIndex).BoxSize =
						Float3(AppSettings::BoxSizeX, AppSettings::BoxSizeY, AppSettings::BoxSizeZ);
				}
			}
		}
	}
}

void Realtime_GI::RenderSpecularProbeCubemaps()
{
	_meshRenderer.SetInitializeProbes(true);
	if (_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums() != 0)
	{
		CreateCubemap *cubeMap;
		RenderTarget2D renderTarget;

		for (uint32 probeIndex = 0; probeIndex < _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums(); ++probeIndex)
		{
			_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbe(&cubeMap, probeIndex);

			cubeMap->Create(&_meshRenderer, _globalTransform, _envMap, _envMapSH, _jitterOffset, &_skybox);
			cubeMap->GetTargetViews(renderTarget);
			_deviceManager.ImmediateContext()->GenerateMips(renderTarget.SRView);
			cubeMap->RenderPrefilterCubebox(_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeArray(), 
				_globalTransform, probeIndex);
			//cubeMap->GetPreFilterRT(renderTarget);
			renderTarget = _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeArray();
			_deviceManager.ImmediateContext()->GenerateMips(renderTarget.SRView);
		}

		_meshRenderer.SetInitializeProbes(false);
		cubeMap = nullptr;
		return;
	}

	_meshRenderer.SetInitializeProbes(false);

}

// Creates all required render targets
void Realtime_GI::CreateRenderTargets()
{
    ID3D11Device* device = _deviceManager.Device();
    uint32 width = _deviceManager.BackBufferWidth();
    uint32 height = _deviceManager.BackBufferHeight();

	const uint32 NumSamples = AppSettings::NumMSAASamples();
	const uint32 Quality = NumSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;

	if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		_colorTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples, Quality);
		_depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D32_FLOAT, true, NumSamples, Quality);
		_velocityTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16_FLOAT, true, NumSamples, Quality);

		if (_velocityResolveTarget.RTView == nullptr)
		{
			_velocityResolveTarget.Initialize(device, width, height, _velocityTarget.Format);
		}
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		// TODO: make sure re-initialize won't alloc more memory
		// consider supporting MSAA for deferred shading later
		_rt0Target.Initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
		_rt1Target.Initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
		_rt2Target.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
		_depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D32_FLOAT, true);
		_colorTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
		// SSR
		_ssrTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	//if (_firstFrame)
	{
		//_cubemapGenerator.Initialize(device);
		
	}

    if(_resolveTarget.Width != width || _resolveTarget.Height != height)
    {
        _resolveTarget.Initialize(device, width, height, _colorTarget.Format);
		if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
		{
			_velocityResolveTarget.Initialize(device, width, height, _velocityTarget.Format);
		}
        _prevFrameTarget.Initialize(device, width, height, _colorTarget.Format);
    }
}

void Realtime_GI::CreateQuadBuffers()
{
	ID3D11Device *device = _deviceManager.Device();
	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	DXCall(device->CreateInputLayout(layout, 1, _clusteredDeferredVS->ByteCode->GetBufferPointer(),
		_clusteredDeferredVS->ByteCode->GetBufferSize(), &_quadInputLayout));

	// Create and initialize the vertex and index buffers
	XMFLOAT4 verts[4] =
	{
		XMFLOAT4(1, 1, 1, 1),
		XMFLOAT4(1, -1, 1, 1),
		XMFLOAT4(-1, -1, 1, 1),
		XMFLOAT4(-1, 1, 1, 1),
	};

	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = sizeof(verts);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = verts;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	DXCall(device->CreateBuffer(&desc, &initData, &_quadVB));

	unsigned short indices[6] = { 0, 1, 2, 2, 3, 0 };

	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = sizeof(indices);
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	initData.pSysMem = indices;
	DXCall(device->CreateBuffer(&desc, &initData, &_quadIB));
}

void Realtime_GI::CreateLightBuffers()
{
	_pointLightBuffer.Initialize(_deviceManager.Device(), sizeof(PointLight), Scene::MAX_SCENE_LIGHTS, true);
	_shProbeLightBuffer.Initialize(_deviceManager.Device(), sizeof(SHProbeLight), IrradianceVolume::MAX_PROBE_NUM, true);
	_probeStructBuffer.Initialize(_deviceManager.Device(), sizeof(Probe), 12, true);
}

void Realtime_GI::ApplyMomentum(float &prevVal, float &val, float deltaTime)
{
	float blendedValue;
	if (fabs(val) > fabs(prevVal))
		blendedValue = Lerp(val, prevVal, pow(0.6f, deltaTime * 60.0f));
	else
		blendedValue = Lerp(val, prevVal, pow(0.8f, deltaTime * 60.0f));
	prevVal = blendedValue;
	val = blendedValue;
}

void Realtime_GI::QueueDebugCommands()
{
	// Debug renders
	if (AppSettings::RenderSceneObjectBBox)
	{
		for (int i = 0; i < _scenes[AppSettings::CurrentScene].getNumStaticOpaqueObjects(); i++)
		{
			BBox &b = _scenes[AppSettings::CurrentScene].getStaticObjectBBoxPtr()[i];
			_debugRenderer.QueueBBoxTranslucent(b, Float4(0.7f, 0.3f, 0.3f, 0.1f));
		}

		for (int i = 0; i < _scenes[AppSettings::CurrentScene].getNumDynamicOpaueObjects(); i++)
		{
			BBox &b = _scenes[AppSettings::CurrentScene].getDynamicObjectBBoxPtr()[i];
			_debugRenderer.QueueBBoxTranslucent(b, Float4(0.3f, 0.7f, 0.3f, 0.1f));
		}

		_debugRenderer.QueueBBoxWire(_scenes[AppSettings::CurrentScene].getSceneBoundingBox(), Float4(0.1f, 0.3f, 0.9f, 1.0f));
	}

	if (AppSettings::RenderIrradianceVolumeProbes)
	{
		const std::vector<Float3> posList = _irradianceVolume.getPositionList();
		for (size_t i = 0; i < posList.size(); i++)
		{
			const Float3 &pos = posList[i];
			_debugRenderer.QueueLightSphere(pos, Float4(1.0f, 1.0f, 1.0f, 0.3f), 0.2f);
		}
	}

	for (int i = 0; i < _scenes[AppSettings::CurrentScene].getNumPointLights(); i++)
	{
		Float3 &pos = _scenes[AppSettings::CurrentScene].getPointLightPtr()[i].cPos;
		_debugRenderer.QueueLightSphere(pos, Float4(1.0f, 1.0f, 1.0f, 0.2f), 0.2f);
	}

	if (AppSettings::RenderProbeBBox)
	{
		for (uint32 i = 0; i < _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums(); ++i)
		{
			BBox b;
			CreateCubemap *cubemap;
			_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbe(&cubemap, i);

			b.Max.x = cubemap->GetPosition().x + cubemap->GetBoxSize().x;
			b.Max.y = cubemap->GetPosition().y + cubemap->GetBoxSize().y;
			b.Max.z = cubemap->GetPosition().z + cubemap->GetBoxSize().z;

			b.Min.x = cubemap->GetPosition().x - cubemap->GetBoxSize().x;
			b.Min.y = cubemap->GetPosition().y - cubemap->GetBoxSize().y;
			b.Min.z = cubemap->GetPosition().z - cubemap->GetBoxSize().z;

			if (i == AppSettings::ProbeIndex)
				_debugRenderer.QueueBBoxTranslucent(b, Float4(0.3f, 0.3f, 0.7f, 0.1f));
			else
				_debugRenderer.QueueBBoxTranslucent(b, Float4(0.7f, 0.3f, 0.3f, 0.1f));
		}
	}
	
}

void Realtime_GI::Update(const Timer& timer)
{
    AppSettings::UpdateUI();

	if (!AppSettings::PauseSceneScript)
	{
		_scenes[AppSettings::CurrentScene].Update(timer);
	}

    MouseState mouseState = MouseState::GetMouseState(_window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(_window);

    if(kbState.IsKeyDown(KeyboardState::Escape))
        _window.Destroy();

	float deltaSec = timer.DeltaSecondsF();;
	float CamMoveSpeed = 5.0f * deltaSec;
	const float CamRotSpeed = 0.180f * deltaSec;
	const float MeshRotSpeed = 0.180f * deltaSec;

    // Move the camera with keyboard input
    if(kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

	float forward = CamMoveSpeed *
		((kbState.IsKeyDown(KeyboardState::W) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::S) ? -1 : 0.0f));

	float strafe = CamMoveSpeed *
		((kbState.IsKeyDown(KeyboardState::A) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::D) ? -1 : 0.0f));

	float ascend = CamMoveSpeed * 
		((kbState.IsKeyDown(KeyboardState::Q) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::E) ? -1 : 0.0f));

	/*if (kbState.IsKeyDown(KeyboardState::Up))
	{
		Float3 trans = _scenes[0].getStaticOpaqueObjectsPtr()->base->Translation();
		_scenes[0].getStaticOpaqueObjectsPtr()->base->SetTranslation(Float3(0, trans[1] + 1, 0));
	}*/


	Float3 camPos = _camera.Position();

	ApplyMomentum(_prevForward, forward, deltaSec);
	ApplyMomentum(_prevStrafe, strafe, deltaSec);
	ApplyMomentum(_prevAscend, ascend, deltaSec);

	if (AppSettings::ProbeIndex.Changed() && 
		AppSettings::ProbeIndex < _scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeNums())
	{
		CreateCubemap *cubeMap;
		_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbe(&cubeMap, (uint32)AppSettings::ProbeIndex);
		Float3 pos = cubeMap->GetPosition();
		Float3 boxSize = cubeMap->GetBoxSize();

		_camera.SetLookAt(pos + boxSize * 2, pos, Float3(0, 1, 0));
		_camera.SetPosition(pos + boxSize * 2);
		_camera.SetXRotation(45 * Pi / 180);
		_camera.SetYRotation(-135 * Pi / 180);

		cubeMap = nullptr;
	}
	else
	{
		camPos += _camera.Forward() * forward;
		camPos += _camera.Left() * strafe;
		camPos += _camera.Up() * ascend;

		_camera.SetPosition(camPos);
	}

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = _camera.XRotation();
        float yRot = _camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        _camera.SetXRotation(xRot);
        _camera.SetYRotation(yRot);
    }

    // Reset the camera projection
    _camera.SetAspectRatio(_camera.AspectRatio());
    Float2 jitter = 0.0f;
    if (AppSettings::EnableTemporalAA 
	 && AppSettings::EnableJitter() 
	 && AppSettings::UseStandardResolve == false)
    {
        const float jitterScale = 0.5f;

        if(AppSettings::JitterMode == JitterModes::Uniform2x)
        {
            jitter = _frameCount % 2 == 0 ? -0.5f : 0.5f;
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersly16)
        {
            uint64 idx = _frameCount % 16;
            jitter = Hammersley2D(idx, 16) - Float2(0.5f);
        }

        jitter *= jitterScale;

        const float offsetX = jitter.x * (1.0f / _colorTarget.Width);
        const float offsetY = jitter.y * (1.0f / _colorTarget.Height);
        Float4x4 offsetMatrix = Float4x4::TranslationMatrix(Float3(offsetX, -offsetY, 0.0f));
        _camera.SetProjection(_camera.ProjectionMatrix() * offsetMatrix);
    }

    _jitterOffset = (jitter - _prevJitter) * 0.5f;
    _prevJitter = jitter;

	// Set centroid sampling mode - change shader bindings
	if (AppSettings::CentroidSampling.Changed())
	{
		_meshRenderer.ReMapMeshShaders();
	}

	if (AppSettings::DiffuseGIBounces.Changed())
	{
		_irradianceVolume.SetNumOfBounces(AppSettings::DiffuseGIBounces);
	}

    // Toggle VSYNC
    if(kbState.RisingEdge(KeyboardState::V))
        _deviceManager.SetVSYNCEnabled(!_deviceManager.VSYNCEnabled());

    _deviceManager.SetNumVSYNCIntervals(AppSettings::DoubleSyncInterval ? 2 : 1);

    if(AppSettings::CurrentScene.Changed())
    {
		_prevScene->OnSceneChange();

		Scene *currScene = &_scenes[AppSettings::CurrentScene];
		_camera = *currScene->getSceneCameraSavedPtr();

		_meshRenderer.SetScene(currScene);
        AppSettings::SceneOrientation.SetValue(currScene->getSceneOrientation());
		_lightClusters.SetScene(currScene);
		_irradianceVolume.SetScene(currScene);

		_prevScene = &_scenes[AppSettings::CurrentScene];

		UpdateSpecularProbeUIInfo();
    }

	Quaternion orientation = AppSettings::SceneOrientation;
	orientation = orientation * Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), 
		AppSettings::ModelRotationSpeed * timer.DeltaSecondsF());

	AppSettings::SceneOrientation.SetValue(orientation);
	_globalTransform = orientation.ToFloat4x4() *
		Float4x4::ScaleMatrix(_scenes[AppSettings::CurrentScene].getSceneScale()) *
		Float4x4::TranslationMatrix(_scenes[AppSettings::CurrentScene].getSceneTranslation());

	_irradianceVolume.Update();
	UpdateSpecularProbeProperties();

	QueueDebugCommands();
}

void Realtime_GI::RenderAA()
{
    PIXEvent pixEvent(L"MSAA Resolve + Temporal AA");

    ID3D11DeviceContext* context = _deviceManager.ImmediateContext();

	ID3D11ShaderResourceView* velocitySRV = nullptr;
	if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		velocitySRV = _velocityTarget.SRView;
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		velocitySRV = _rt2Target.SRView;
	}

    if(AppSettings::NumMSAASamples() > 1)
    {
        context->ResolveSubresource(_velocityResolveTarget.Texture, 0, _velocityTarget.Texture, 0, _velocityTarget.Format);
        velocitySRV = _velocityResolveTarget.SRView;
    }


    ID3D11RenderTargetView* rtvs[1] = { _resolveTarget.RTView };

    context->OMSetRenderTargets(1, rtvs, nullptr);

	

	RenderTarget2D  raaResource ;//= _ssrTarget
	raaResource = AppSettings::EnableSSR ? _ssrTarget : _colorTarget;

    if(AppSettings::UseStandardResolve)
    {
        if(AppSettings::MSAAMode == 0)
			context->CopyResource(_resolveTarget.Texture, raaResource.Texture); //_colorTarget
        else
			context->ResolveSubresource(_resolveTarget.Texture, 0, raaResource.Texture, 0, raaResource.Format);//
        return;
    }

    const uint32 SampleRadius = static_cast<uint32>((AppSettings::FilterSize / 2.0f) + 0.499f);
    ID3D11PixelShader* pixelShader = _resolvePS[AppSettings::MSAAMode];
    context->PSSetShader(pixelShader, nullptr, 0);
    context->VSSetShader(_resolveVS, nullptr, 0);

	_resolveConstants.Data.TextureSize = Float2(static_cast<float>(raaResource.Width),//
		static_cast<float>(raaResource.Height));//
    _resolveConstants.Data.SampleRadius = SampleRadius;;
    _resolveConstants.ApplyChanges(context);
    _resolveConstants.SetPS(context, 0);

	ID3D11ShaderResourceView* srvs[3] = { raaResource.SRView, _prevFrameTarget.SRView, velocitySRV };//
    context->PSSetShaderResources(0, 3, srvs);

    ID3D11SamplerState* samplers[1] = { _samplerStates.LinearClamp() };
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

    context->CopyResource(_prevFrameTarget.Texture, _resolveTarget.Texture);
}

void Realtime_GI::RenderSceneCubemaps(ID3D11DeviceContext *context)
{
	_meshRenderer.SetCubemapCapture(true);
	if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		_meshRenderer.SetDrawGBuffer(false);
	}

	RenderSpecularProbeCubemaps();

	_meshRenderer.SetCubemapCapture(false);

	if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		_meshRenderer.SetDrawGBuffer(true);

		if (_scenes[AppSettings::CurrentScene].hasProxySceneObject())
		{
			_meshRenderer.SetCubemapCapture(true);
			_irradianceVolume.RenderSceneAtlasGBuffer();
			_irradianceVolume.RenderSceneAtlasProxyMeshTexcoord();
			_meshRenderer.SetCubemapCapture(false);
		}
	}
	Scene *curScene = &_scenes[AppSettings::CurrentScene];
	if (curScene->getProbeManagerPtr()->GetProbeNums() == 0) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	_deviceManager.ImmediateContext()->Map(_probeStructBuffer.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	Probe *probePtr = static_cast<Probe*>(mappedResource.pData);

	
	memcpy(probePtr, &(curScene->getProbeManagerPtr()->_probes[0]), 
		sizeof(Probe) * curScene->getProbeManagerPtr()->GetProbeNums());
	_deviceManager.ImmediateContext()->Unmap(_probeStructBuffer.Buffer, 0);
}

void Realtime_GI::Render(const Timer& timer)
{
	if (AppSettings::MSAAMode.Changed() || AppSettings::CurrentShadingTech.Changed())
	{
		if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
		{
			AppSettings::MSAAMode.SetValue(MSAAModes::MSAANone);
		}

        CreateRenderTargets();
	}

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();

    AppSettings::UpdateCBuffer(context);

	if (_firstFrame || AppSettings::CurrentScene.Changed() || AppSettings::EnableRealtimeCubemap)
	{
		// render cubemap every time scene has changed
		RenderSceneCubemaps(context);
	}

	if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		RenderSceneGBuffer();

		UploadLights();
		AssignLightAndUploadClusters();

		_irradianceVolume.MainRender();
		RenderLightsDeferred();
		
		//SSR
		_ssr.MainRender();
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		RenderSceneForward();
	}

    RenderBackgroundVelocity();

    RenderAA();

    {
        // Kick off post-processing
        PIXEvent pixEvent(L"Post Processing");
        _postProcessor.Render(context, _resolveTarget.SRView, _deviceManager.BackBuffer(), timer.DeltaSecondsF());
    }

    ID3D11RenderTargetView* renderTargets[1] = { _deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    SetViewport(context, _deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());

	_debugRenderer.FlushDrawQueued();
    RenderHUD();

    //if(++_frameCount == 2)
	_frameCount++;
		_firstFrame = false;
}

void Realtime_GI::RenderSceneGBuffer()
{
	PIXEvent event(L"Render Scene Deferred");

	_meshRenderer.SetDrawGBuffer(true);

	ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();
	ProbeManager probeManager;
	//RenderTarget2D cubemapRenderTarget;

	SceneObject *staticObj = _scenes[AppSettings::CurrentScene].getStaticOpaqueObjectsPtr();
	SceneObject *dynamicObj = _scenes[AppSettings::CurrentScene].getDynamicOpaqueObjectsPtr();
	int staticNum = _scenes[AppSettings::CurrentScene].getNumStaticOpaqueObjects();
	int dynamicNum = _scenes[AppSettings::CurrentScene].getNumDynamicOpaueObjects();

	SetViewport(context, _colorTarget.Width, _colorTarget.Height);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(_rt0Target.RTView, clearColor);
	context->ClearRenderTargetView(_rt1Target.RTView, clearColor);
	context->ClearRenderTargetView(_rt2Target.RTView, clearColor);

	context->ClearDepthStencilView(_depthBuffer.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* renderTargets[3] = { nullptr, nullptr, nullptr };
	context->OMSetRenderTargets(1, renderTargets, _depthBuffer.DSView);

	renderTargets[0] = _rt0Target.RTView;
	renderTargets[1] = _rt1Target.RTView;
	renderTargets[2] = _rt2Target.RTView;
	context->OMSetRenderTargets(3, renderTargets, _depthBuffer.DSView);

	// _meshRenderer.Render(context, _camera, _globalTransform, _envMap, _envMapSH, _jitterOffset);
	// TODO: note that render is controlled by AppSettings::CurrentShadingTech, in deferred settings, it writes to depth buffer

	_meshRenderer.Render(context, _camera, _globalTransform, _envMap, _envMapSH, _jitterOffset);
	
	// sample the depth after gbuffer and depth is filled
	_meshRenderer.ReduceDepth(context, _depthBuffer, _camera);
	_meshRenderer.RenderShadowMap(context, _camera, _globalTransform);

	renderTargets[0] = renderTargets[1] = renderTargets[2] = nullptr;
	context->OMSetRenderTargets(3, renderTargets, nullptr);
}

Float4 CreateInvDeviceZToWorldZTransform(Float4x4 const & ProjMatrix)
{
	// The depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	// 
	// Note a reversed Z projection matrix will have A=0. 
	//
	// Done in shader as:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	float DepthMul = ProjMatrix.m[2][2];
	float DepthAdd = ProjMatrix.m[3][2];

	if (DepthAdd == 0.f)
	{
		// Avoid dividing by 0 in this case
		DepthAdd = 0.00000001f;
	}

	float SubtractValue = DepthMul / DepthAdd;

	// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
	// This fixes fog not being applied to the black background in the editor.
	SubtractValue -= 0.00000001f;

	return Float4(
		0.0f,			// Unused
		0.0f,			// Unused
		1.f / DepthAdd,
		SubtractValue
		);
}

void Realtime_GI::RenderLightsDeferred()
{
	PIXEvent event(L"Render Lights Deferred");

	ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();

	SetViewport(context, _colorTarget.Width, _colorTarget.Height);
	context->IASetInputLayout(_quadInputLayout);

	// Set the vertex buffer
	uint32 stride = sizeof(XMFLOAT4);
	uint32 offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { _quadVB.GetInterfacePtr() };
	context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	context->IASetIndexBuffer(_quadIB, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);
	context->RSSetState(_rasterizerStates.NoCull());

	ID3D11RenderTargetView* rtvs[1] = { _colorTarget.RTView };
	context->OMSetRenderTargets(1, rtvs, 0);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(_colorTarget.RTView, clearColor);

	context->VSSetShader(_clusteredDeferredVS, nullptr, 0);
	context->PSSetShader(_clusteredDeferredPS, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);

	// Update constant buffer
	// TODO : this is a huge hack... refactor later
	ConstantBuffer<MeshRenderer::MeshPSConstants> *meshPSConstant = _meshRenderer.getMeshPSConstantsPtr();
	memcpy(&_deferredPassConstants.Data, ((char *)&meshPSConstant->Data) + sizeof(Float4), offsetof(DeferredPassConstants, OffsetScale));
	
	_deferredPassConstants.Data.CameraPosWS = _camera.Position();
	_deferredPassConstants.Data.OffsetScale = OffsetScale;
	_deferredPassConstants.Data.PositiveExponent = PositiveExponent;
	_deferredPassConstants.Data.NegativeExponent = NegativeExponent;
	_deferredPassConstants.Data.LightBleedingReduction = LightBleedingReduction;
	_deferredPassConstants.Data.ProjectionToWorld = Float4x4::Transpose(Float4x4::Invert(_camera.ViewProjectionMatrix()));
	_deferredPassConstants.Data.ViewToWorld = Float4x4::Transpose(_camera.WorldMatrix());
	_deferredPassConstants.Data.EnvironmentSH = _envMapSH;

	_deferredPassConstants.Data.NearPlane = NearClip;
	_deferredPassConstants.Data.CameraZAxisWS = _camera.WorldMatrix().Forward();
	_deferredPassConstants.Data.FarPlane = FarClip;
	_deferredPassConstants.Data.ProjTermA = FarClip / (FarClip - NearClip);
	_deferredPassConstants.Data.ProjTermB = (-FarClip * NearClip) / (FarClip - NearClip);
	_deferredPassConstants.Data.ClusterScale = _lightClusters.getClusterScale();
	_deferredPassConstants.Data.ClusterBias = _lightClusters.getClusterBias();
	_deferredPassConstants.Data.WorldToView = Float4x4::Transpose(_camera.ViewMatrix());
	_deferredPassConstants.Data.invNDCToWorldZ = CreateInvDeviceZToWorldZTransform(_camera.ProjectionMatrix());
	_deferredPassConstants.ApplyChanges(context);
	_deferredPassConstants.SetVS(context, 0);
	_deferredPassConstants.SetPS(context, 0);

	// Bind shader resources
	ID3D11ShaderResourceView* clusteredDeferredResources[] =
	{
		_rt0Target.SRView,
		_rt1Target.SRView,
		_rt2Target.SRView,
		_depthBuffer.SRView,
		_meshRenderer.GetVSMRenderTargetPtr()->SRView,
		_pointLightBuffer.SRView,
		_envMap,
		_meshRenderer.GetSpecularLookupTexturePtr(),
		_lightClusters.getLightIndicesListSRV(),
		_lightClusters.getClusterTexSRV(),
		_shProbeLightBuffer.SRView,
		_irradianceVolume.getRelightSHStructuredBufferPtr()->SRView,
		_scenes[AppSettings::CurrentScene].getProbeManagerPtr()->GetProbeArray().SRView,
		_probeStructBuffer.SRView
	};

	ID3D11SamplerState* sampStates[2] = {
		_meshRenderer.GetEVSMSamplerStatePtr(),
		_samplerStates.LinearClamp(),
	};

	context->PSSetSamplers(0, 2, sampStates);

	context->PSSetShaderResources(0, _countof(clusteredDeferredResources), clusteredDeferredResources);
	context->DrawIndexed(6, 0, 0);

	sampStates[0] = sampStates[1] = nullptr;
	context->PSSetSamplers(0, 2, sampStates);

	memset(clusteredDeferredResources, 0, sizeof(clusteredDeferredResources));
	context->PSSetShaderResources(0, _countof(clusteredDeferredResources), clusteredDeferredResources);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	// Skybox
	context->OMSetRenderTargets(1, rtvs, _depthBuffer.DSView);
	if (AppSettings::RenderBackground)
		_skybox.RenderEnvironmentMap(context, _envMap, _camera.ViewMatrix(), _camera.ProjectionMatrix(),
		Float3(std::exp2(AppSettings::ExposureScale)));

	rtvs[0] = nullptr;
	context->OMSetRenderTargets(1, rtvs, nullptr);
}

void Realtime_GI::RenderSceneForward()
{
    PIXEvent event(L"Render Scene Forward");

	_meshRenderer.SetDrawGBuffer(false);

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();
	ProbeManager probeManager;
	//RenderTarget2D cubemapRenderTarget;

    SetViewport(context, _colorTarget.Width, _colorTarget.Height);

	//_cubemapGenerator.GetTargetViews(cubemapRenderTarget)
	//_cubemapGenerator.GetPreFilterTargetViews(cubemapRenderTarget);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(_colorTarget.RTView, clearColor);
    context->ClearRenderTargetView(_velocityTarget.RTView, clearColor);
    context->ClearDepthStencilView(_depthBuffer.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ID3D11RenderTargetView* renderTargets[2] = { nullptr, nullptr };
    context->OMSetRenderTargets(1, renderTargets, _depthBuffer.DSView);
	
	// TODO: reduce api calls
    _meshRenderer.RenderDepth(context, _camera, _globalTransform, false);

    _meshRenderer.ReduceDepth(context, _depthBuffer, _camera);
    _meshRenderer.RenderShadowMap(context, _camera, _globalTransform);

    renderTargets[0] = _colorTarget.RTView;
    renderTargets[1] = _velocityTarget.RTView;
    context->OMSetRenderTargets(2, renderTargets, _depthBuffer.DSView);

	_meshRenderer.Render(context, _camera, _globalTransform, _envMap, _envMapSH, _jitterOffset);

    renderTargets[0] = _colorTarget.RTView;
    renderTargets[1] = nullptr;
	context->OMSetRenderTargets(2, renderTargets, _depthBuffer.DSView);

    if(AppSettings::RenderBackground)
        _skybox.RenderEnvironmentMap(context, _envMap, _camera.ViewMatrix(), _camera.ProjectionMatrix(),
		Float3(std::exp2(AppSettings::ExposureScale)));

    renderTargets[0] = renderTargets[1] = nullptr;
    context->OMSetRenderTargets(2, renderTargets, nullptr);
}

void Realtime_GI::RenderBackgroundVelocity()
{
    PIXEvent pixEvent(L"Render Background Velocity");

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();
	int targetWidth = 0;
	int targetHeight = 0;

	if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		targetWidth = _velocityTarget.Width;
		targetHeight = _velocityTarget.Height;
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		targetWidth = _rt2Target.Width;
		targetHeight = _rt2Target.Height;
	}

	SetViewport(context, targetWidth, targetHeight);

    // Don't use camera translation for background velocity
    FirstPersonCamera tempCamera = _camera;

    _backgroundVelocityConstants.Data.InvViewProjection = Float4x4::Transpose(Float4x4::Invert(tempCamera.ViewProjectionMatrix()));
    _backgroundVelocityConstants.Data.PrevViewProjection = Float4x4::Transpose(_prevViewProjection);
	_backgroundVelocityConstants.Data.RTSize.x = float(targetWidth);
	_backgroundVelocityConstants.Data.RTSize.y = float(targetHeight);
    _backgroundVelocityConstants.Data.JitterOffset = _jitterOffset;
    _backgroundVelocityConstants.ApplyChanges(context);
    _backgroundVelocityConstants.SetPS(context, 0);

    _prevViewProjection = tempCamera.ViewProjectionMatrix();

    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    ID3D11RenderTargetView* rtvs[1] = { nullptr };
	if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		rtvs[0] = _velocityTarget.RTView;
		context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		rtvs[0] = _rt2Target.RTView;

		// Color writable to only r, g channel (where the velocity is)
		D3D11_BLEND_DESC blendStateDesc = _blendStates.BlendDisabledDesc();
		blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN;
		ID3D11BlendStatePtr blendStatePtr;
		DXCall(_deviceManager.Device()->CreateBlendState(&blendStateDesc, &blendStatePtr));
		context->OMSetBlendState(blendStatePtr, blendFactor, 0xFFFFFFFF);
	}

	context->OMSetDepthStencilState(_depthStencilStates.DepthEnabled(), 0);
	context->RSSetState(_rasterizerStates.NoCull());
	
    context->OMSetRenderTargets(1, rtvs, _depthBuffer.DSView);

    context->VSSetShader(_backgroundVelocityVS, nullptr, 0);
    context->PSSetShader(_backgroundVelocityPS, nullptr, 0);
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

	// reset blend state
	if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		rtvs[0] = _velocityTarget.RTView;
		context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
	}
}

void Realtime_GI::RenderHUD()
{
    PIXEvent pixEvent(L"HUD Pass");

    _spriteRenderer.Begin(_deviceManager.ImmediateContext(), SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText(L"FPS: ");
    fpsText += ToString(_fps) + L" (" + ToString(1000.0f / _fps) + L"ms)";
    _spriteRenderer.RenderText(_font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    transform._42 += 25.0f;
    wstring vsyncText(L"VSYNC (V): ");
    vsyncText += _deviceManager.VSYNCEnabled() ? L"Enabled" : L"Disabled";
    _spriteRenderer.RenderText(_font, vsyncText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

	transform._42 += 25.0f;
	wstring posText(L"Camera Position: ");
	posText += ToString(_camera.Position()[0]) + L", " + ToString(_camera.Position()[1]) + L", "
		+ ToString(_camera.Position()[2]) + L", ";
	_spriteRenderer.RenderText(_font, posText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

	/*Float3 trans = _scenes[0].getStaticOpaqueObjectsPtr()->base->Translation();
	transform._42 += 25.0f;
	wstring objText(L"Object Position: ");
	probeText += ToString(trans[0]) + L", " + ToString(trans[1]) + L", "
		+ ToString(trans[2]) + L", ";
	_spriteRenderer.RenderText(_font, probeText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));*/

	BBox sceneBoundingBox = _scenes[AppSettings::CurrentScene].getSceneBoundingBox();
	/*std::wstring sceneBoundDebugText =
		L"Scene Bound Max: "
		+ std::to_wstring(sceneBoundingBox.Max.x) + L"  "
		+ std::to_wstring(sceneBoundingBox.Max.y) + L"  "
		+ std::to_wstring(sceneBoundingBox.Max.z) + L"  "
		+ L"\nScene Bound Min: "
		+ std::to_wstring(sceneBoundingBox.Min.x) + L"  "
		+ std::to_wstring(sceneBoundingBox.Min.y) + L"  "
		+ std::to_wstring(sceneBoundingBox.Min.z);

		transform._42 += 25.0f;
		_spriteRenderer.RenderText(_font, sceneBoundDebugText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));*/

    Profiler::GlobalProfiler.EndFrame(_spriteRenderer, _font);

    _spriteRenderer.End();
}

void Realtime_GI::UploadLights()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	Scene *curScene = &_scenes[AppSettings::CurrentScene];
	// pointlights
	if (curScene->getNumPointLights() > 0)
	{
		_deviceManager.ImmediateContext()->Map(_pointLightBuffer.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		PointLight *pointLightGPUBufferPtr = static_cast<PointLight *>(mappedResource.pData);

		PointLight *pointLightPtr = curScene->getPointLightPtr();
		memcpy(pointLightGPUBufferPtr, pointLightPtr, sizeof(PointLight) * curScene->getNumPointLights());
		_deviceManager.ImmediateContext()->Unmap(_pointLightBuffer.Buffer, 0);//!
	}

	// sh probe lights
	if (_irradianceVolume.getSHProbeLights().size() > 0)
	{
		_deviceManager.ImmediateContext()->Map(_shProbeLightBuffer.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		SHProbeLight *shProbeLightGPUBufferPtr = static_cast<SHProbeLight *>(mappedResource.pData);
		const std::vector<SHProbeLight> &shProbeLights = _irradianceVolume.getSHProbeLights();
		memcpy(shProbeLightGPUBufferPtr, &shProbeLights[0], sizeof(SHProbeLight) * shProbeLights.size());
		_deviceManager.ImmediateContext()->Unmap(_shProbeLightBuffer.Buffer, 0);
	}
}

void Realtime_GI::AssignLightAndUploadClusters()
{
	_lightClusters.AssignLightToClusters();
	_lightClusters.UploadClustersData();
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Realtime_GI app;
    app.Run();
}