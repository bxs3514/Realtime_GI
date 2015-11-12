//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

public class Settings
{
    enum MSAAModes
    {
        [EnumLabel("None")]
        MSAANone = 0,

        [EnumLabel("2x")]
        MSAA2x,

        [EnumLabel("4x")]
        MSAA4x,

        [EnumLabel("8x")]
        MSAA8x,
    }

    enum FilterTypes
    {
        Box = 0,
        Triangle,
        Gaussian,
        BlackmanHarris,
        Smoothstep,
        BSpline,
        CatmullRom,
        Mitchell,
        GeneralizedCubic,
        Sinc,
    }

    enum Scenes
    {
        RoboHand,
        Plane
    }

    enum JitterModes
    {
        None,
        Uniform2x,
        Hammersly16,
    }

    public class AntiAliasing
    {
        MSAAModes MSAAMode = MSAAModes.MSAA4x;

        FilterTypes FilterType = FilterTypes.Smoothstep;

        [MinValue(0.0f)]
        [MaxValue(6.0f)]
        [StepSize(0.01f)]
        float FilterSize = 2.0f;

        [MinValue(0.01f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float GaussianSigma = 0.5f;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float CubicB = 0.33f;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        float CubicC = 0.33f;

        bool UseStandardResolve = false;

        bool InverseLuminanceFiltering = true;

        bool UseExposureFiltering = true;

        [MinValue(-16.0f)]
        [MaxValue(16.0f)]
        float ExposureFilterOffset = 2.0f;

        bool UseGradientMipLevel = false;

        [UseAsShaderConstant(false)]
        bool CentroidSampling = true;

        bool EnableTemporalAA = true;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        float TemporalAABlendFactor = 0.5f;

        bool UseTemporalColorWeighting = true;

        bool ClampPrevColor = true;

        [UseAsShaderConstant(false)]
        JitterModes JitterMode = JitterModes.Uniform2x;

        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        float LowFreqWeight = 0.25f;

        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        float HiFreqWeight = 0.85f;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        float SharpeningAmount = 0.0f;
    }

    public class SceneControls
    {
        Scenes CurrentScene = Scenes.RoboHand;

        [DisplayName("Light Direction")]
        [HelpText("The direction of the light")]
        Direction LightDirection = new Direction(-0.75f, 0.977f, -0.4f);

        [DisplayName("Light Color")]
        [HelpText("The color of the light")]
        [MinValue(0.0f)]
        [MaxValue(20.0f)]
        [StepSize(0.1f)]
        [HDR(true)]
        Color LightColor = new Color(20.0f, 16.0f, 10.0f);

        [HelpText("Enables direct lighting")]
        bool EnableDirectLighting = true;

        [HelpText("Enables ambient lighting from the environment")]
        bool EnableAmbientLighting = true;

        bool RenderBackground = true;

        bool EnableShadows = true;

        bool EnableNormalMaps = true;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        float NormalMapIntensity = 1.0f;

        [DisplayName("Diffuse Intensity")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Diffuse albedo intensity parameter for the material")]
        float DiffuseIntensity = 0.5f;

        [MinValue(0.001f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Specular roughness parameter for the material")]
        [ConversionMode(ConversionMode.Square)]
        float Roughness = 0.1f;

        [DisplayName("Specular Intensity")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Specular intensity parameter for the material")]
        float SpecularIntensity = 0.05f;

        Orientation ModelOrientation = new Orientation(0.41f, -0.55f, -0.29f, 0.67f);

        [MinValue(0.0f)]
        [MaxValue(10.0f)]
        [StepSize(0.01f)]
        float ModelRotationSpeed = 0.0f;

        bool DoubleSyncInterval = false;

        [MinValue(-16.0f)]
        [MaxValue(16.0f)]
        float ExposureScale = 0.0f;
    }

    public class PostProcessing
    {
        [DisplayName("Bloom Exposure Offset")]
        [MinValue(-10.0f)]
        [MaxValue(0.0f)]
        [StepSize(0.01f)]
        [HelpText("Exposure offset applied to generate the input of the bloom pass")]
        float BloomExposure = -4.0f;

        [DisplayName("Bloom Magnitude")]
        [MinValue(0.0f)]
        [MaxValue(2.0f)]
        [StepSize(0.01f)]
        [HelpText("Scale factor applied to the bloom results when combined with tone-mapped result")]
        float BloomMagnitude = 1.0f;

        [DisplayName("Bloom Blur Sigma")]
        [MinValue(0.5f)]
        [MaxValue(5.0f)]
        [StepSize(0.01f)]
        [HelpText("Sigma parameter of the Gaussian filter used in the bloom pass")]
        float BloomBlurSigma = 5.0f;

        [MinValue(-10.0f)]
        [MaxValue(10.0f)]
        [StepSize(0.01f)]
        [HelpText("Manual exposure value when auto-exposure is disabled")]
        float ManualExposure = -2.5f;
    }

    // No auto-exposure for this sample
    const bool EnableAutoExposure = false;
    const float KeyValue = 0.115f;
    const float AdaptationRate = 0.5f;
}