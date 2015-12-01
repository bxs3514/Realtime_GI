#pragma once

#include <PCH.h>
#include <Settings.h>
#include <Graphics\GraphicsTypes.h>

using namespace SampleFramework11;

enum class MSAAModes
{
    MSAANone = 0,
    MSAA2x = 1,
    MSAA4x = 2,
    MSAA8x = 3,

    NumValues
};

typedef EnumSettingT<MSAAModes> MSAAModesSetting;

enum class FilterTypes
{
    Box = 0,
    Triangle = 1,
    Gaussian = 2,
    BlackmanHarris = 3,
    Smoothstep = 4,
    BSpline = 5,
    CatmullRom = 6,
    Mitchell = 7,
    GeneralizedCubic = 8,
    Sinc = 9,

    NumValues
};

typedef EnumSettingT<FilterTypes> FilterTypesSetting;

enum class JitterModes
{
    None = 0,
    Uniform2x = 1,
    Hammersly16 = 2,

    NumValues
};

typedef EnumSettingT<JitterModes> JitterModesSetting;

enum class Scenes
{
    CornellBox = 0,
    DropBoxes = 1,
    Sponza = 2,

    NumValues
};

typedef EnumSettingT<Scenes> ScenesSetting;

enum class ShadingTech
{
    Forward = 0,
    Clustered_Deferred = 1,

    NumValues
};

typedef EnumSettingT<ShadingTech> ShadingTechSetting;

namespace AppSettings
{
    static const bool EnableAutoExposure = false;
    static const float KeyValue = 0.1150f;
    static const float AdaptationRate = 0.5000f;

    extern MSAAModesSetting MSAAMode;
    extern FilterTypesSetting FilterType;
    extern FloatSetting FilterSize;
    extern FloatSetting GaussianSigma;
    extern FloatSetting CubicB;
    extern FloatSetting CubicC;
    extern BoolSetting UseStandardResolve;
    extern BoolSetting InverseLuminanceFiltering;
    extern BoolSetting UseExposureFiltering;
    extern FloatSetting ExposureFilterOffset;
    extern BoolSetting UseGradientMipLevel;
    extern BoolSetting CentroidSampling;
    extern BoolSetting EnableTemporalAA;
    extern FloatSetting TemporalAABlendFactor;
    extern BoolSetting UseTemporalColorWeighting;
    extern BoolSetting ClampPrevColor;
    extern JitterModesSetting JitterMode;
    extern FloatSetting LowFreqWeight;
    extern FloatSetting HiFreqWeight;
    extern FloatSetting SharpeningAmount;
    extern ScenesSetting CurrentScene;
    extern ShadingTechSetting CurrentShadingTech;
    extern DirectionSetting LightDirection;
    extern ColorSetting LightColor;
    extern BoolSetting EnableSSR;
    extern BoolSetting PauseSceneScript;
    extern BoolSetting EnableDirectLighting;
    extern BoolSetting EnableIndirectDiffuseLighting;
    extern BoolSetting EnableIndirectSpecularLighting;
    extern BoolSetting RenderBackground;
    extern BoolSetting RenderSceneObjectBBox;
    extern BoolSetting RenderProbeBBox;
    extern BoolSetting RenderIrradianceVolumeProbes;
    extern BoolSetting EnableShadows;
    extern BoolSetting EnableNormalMaps;
    extern BoolSetting EnableRealtimeCubemap;
    extern IntSetting DiffuseGIBounces;
    extern FloatSetting DiffuseGI_Intensity;
    extern FloatSetting NormalMapIntensity;
    extern FloatSetting DiffuseIntensity;
    extern FloatSetting Roughness;
    extern FloatSetting SpecularIntensity;
    extern FloatSetting EmissiveIntensity;
    extern FloatSetting ProbeIndex;
    extern FloatSetting ProbeX;
    extern FloatSetting ProbeY;
    extern FloatSetting ProbeZ;
    extern FloatSetting BoxSizeX;
    extern FloatSetting BoxSizeY;
    extern FloatSetting BoxSizeZ;
    extern OrientationSetting SceneOrientation;
    extern FloatSetting ModelRotationSpeed;
    extern BoolSetting DoubleSyncInterval;
    extern FloatSetting ExposureScale;
    extern FloatSetting BloomExposure;
    extern FloatSetting BloomMagnitude;
    extern FloatSetting BloomBlurSigma;
    extern FloatSetting ManualExposure;

    struct AppSettingsCBuffer
    {
        int32 MSAAMode;
        int32 FilterType;
        float FilterSize;
        float GaussianSigma;
        float CubicB;
        float CubicC;
        bool32 UseStandardResolve;
        bool32 InverseLuminanceFiltering;
        bool32 UseExposureFiltering;
        float ExposureFilterOffset;
        bool32 UseGradientMipLevel;
        bool32 EnableTemporalAA;
        float TemporalAABlendFactor;
        bool32 UseTemporalColorWeighting;
        bool32 ClampPrevColor;
        float LowFreqWeight;
        float HiFreqWeight;
        float SharpeningAmount;
        int32 CurrentScene;
        int32 CurrentShadingTech;
        Float3 LightDirection;
        Float4Align Float3 LightColor;
        bool32 EnableSSR;
        bool32 PauseSceneScript;
        bool32 EnableDirectLighting;
        bool32 EnableIndirectDiffuseLighting;
        bool32 EnableIndirectSpecularLighting;
        bool32 RenderBackground;
        bool32 RenderSceneObjectBBox;
        bool32 RenderProbeBBox;
        bool32 RenderIrradianceVolumeProbes;
        bool32 EnableShadows;
        bool32 EnableNormalMaps;
        bool32 EnableRealtimeCubemap;
        int32 DiffuseGIBounces;
        float DiffuseGI_Intensity;
        float NormalMapIntensity;
        float DiffuseIntensity;
        float Roughness;
        float SpecularIntensity;
        float EmissiveIntensity;
        float ProbeIndex;
        float ProbeX;
        float ProbeY;
        float ProbeZ;
        float BoxSizeX;
        float BoxSizeY;
        float BoxSizeZ;
        Float4Align Quaternion SceneOrientation;
        float ModelRotationSpeed;
        bool32 DoubleSyncInterval;
        float ExposureScale;
        float BloomExposure;
        float BloomMagnitude;
        float BloomBlurSigma;
        float ManualExposure;
    };

    extern ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device);
    void Update();
    void UpdateCBuffer(ID3D11DeviceContext* context);
};

// ================================================================================================

namespace AppSettings
{
    inline uint32 NumMSAASamples(MSAAModes mode)
    {
        static const uint32 NumSamples[uint32(MSAAModes::NumValues)] = { 1, 2, 4, 8 };
        return NumSamples[uint32(mode)];
    }

    inline uint32 NumMSAASamples()
    {
        return NumMSAASamples(MSAAMode);
    }

    inline bool EnableJitter()
    {
        return JitterMode != JitterModes::None;
    }

    void UpdateUI();
}