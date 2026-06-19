// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 388 $

#include "TangentEngine.h"
#include "ResetCache.h"
#include "Common.h"
#include "MessageBuilder.h"
#include "RxContext.h"
#include "Common/TcpSocketBuilder.h"	// Networking module
#include "Sockets.h"					// Sockets module
#include "Interfaces/IPluginManager.h"	// Projects module
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"

// use this definition to switch to in-house development settings
//#define TANGENT_DEVELOPMENT

// standard macros to place a compile time value in quotes to make it a string
#define QUOTEME_(x)		#x
#define QUOTEME(x)		QUOTEME_(x)

// definitions for commands from the hub to the plugin app
namespace HubCommand
{
	enum
	{
		InitiateComms = 0x01,
		ParameterChange = 0x02,
		ParameterReset = 0x03,
		ParameterValueRequest = 0x04,
		MenuChange = 0x05,
		MenuReset = 0x06,
		MenuStringRequest = 0x07,
		ActionOn = 0x08,
		ModeChange = 0x09,
		Transport = 0x0A,
		ActionOff = 0x0B,
		CustomParameterChange = 0x36,
		CustomParameterReset = 0x37,
		CustomParameterValueRequest = 0x38,
		CustomMenuChange = 0x39,
		CustomMenuReset = 0x3A,
		CustomMenuStringRequest = 0x3B,
		CustomActionOn = 0x3C,
		CustomActionOff = 0x3D,
		TrainControl = 0x3E,
		RawInput = 0x3F,
		ModeList = 0x40
	};
}

// definitions for commands from the plugin app to the hub
namespace AppCommand
{
	enum
	{
		ApplicationDefinition = 0x81,
		ParameterValue = 0x82,
		MenuString = 0x83,
		AllChange = 0x84,
		ModeValue = 0x85,
		RenameControl = 0xA2,
		CustomParameterValue = 0xA6,
		CustomMenuString = 0xA7,
		TrainResponse = 0xB0,
		EnableRequestsAllModes = 0xB1
	};
}

// sets up the engine helper instance with all target id properties that are supported using the generic parameter and menu implementation
void TangentEngine::RegisterTargetIds()
{
	// any target id that uses the property approach needs to be registered with the helper and associated with the correct property path
	// for the values that they modify, during runtime the id is converted to the path and that is used to retrieve the property data, if
	// a property is shared by multiple actor types we don't specify the object or component prefix

	// general actor
	_engineHelper->RegisterTargetIdProperty(TargetId::ActorMobility, FName("Mobility"));

	// rendering
	_engineHelper->RegisterTargetIdProperty(TargetId::Visible, FName("bVisible"));

	// orientation
	_engineHelper->RegisterTargetIdProperty(TargetId::ActorOrientationRoll, FName("RelativeRotation.Roll"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ActorOrientationPitch, FName("RelativeRotation.Pitch"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ActorOrientationYaw, FName("RelativeRotation.Yaw"));
	 
	// camera specifics
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraFocalLength, FName("CameraComponent.CurrentFocalLength"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraAperature, FName("CameraComponent.CurrentAperture"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraFocusMethod, FName("CameraComponent.FocusSettings.FocusMethod"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraManualFocusDistance, FName("CameraComponent.FocusSettings.ManualFocusDistance"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraFocusOffset, FName("CameraComponent.FocusSettings.FocusOffset"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CineCameraFocusPlane, FName("CameraComponent.FocusSettings.bDrawDebugFocusPlane"));

	// lightcard
	_engineHelper->RegisterTargetIdProperty(TargetId::LightCardOpacity, FName("Opacity"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightCardFeathering, FName("Feathering"));

	// shared lights
	_engineHelper->RegisterTargetIdProperty(TargetId::LightColorRed, FName("LightColor.R"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightColorGreen, FName("LightColor.G"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightColorBlue, FName("LightColor.B"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightIntensity, FName("Intensity"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightTemperature, FName("Temperature"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseLightTemperature, FName("bUseTemperature"));
	_engineHelper->RegisterTargetIdProperty(TargetId::AffectsWorld, FName("bAffectsWorld"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CastShadows, FName("CastShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::IndirectLightingIntensity, FName("IndirectLightingIntensity"));
	_engineHelper->RegisterTargetIdProperty(TargetId::VolumetricScatteringintensity, FName("VolumetricScatteringintensity"));
	_engineHelper->RegisterTargetIdProperty(TargetId::AttenuationRadius, FName("AttenuationRadius"));

	// directional lights
	_engineHelper->RegisterTargetIdProperty(TargetId::LightSourceAngle, FName("DirectionalLightComponent.LightSourceAngle"));
	_engineHelper->RegisterTargetIdProperty(TargetId::LightSourceSoftAngle, FName("DirectionalLightComponent.LightSourceSoftAngle"));

	// point lights
	_engineHelper->RegisterTargetIdProperty(TargetId::SourceRadius, FName("PointLightComponent.SourceRadius"));
	_engineHelper->RegisterTargetIdProperty(TargetId::SoftSourceRadius, FName("PointLightComponent.SoftSourceRadius"));
	_engineHelper->RegisterTargetIdProperty(TargetId::SourceLength, FName("PointLightComponent.SourceLength"));

	// spot lights
	_engineHelper->RegisterTargetIdProperty(TargetId::InnerConeAngle, FName("SpotLightComponent.InnerConeAngle"));
	_engineHelper->RegisterTargetIdProperty(TargetId::OuterConeAngle, FName("SpotLightComponent.OuterConeAngle"));

	// rect lights
	_engineHelper->RegisterTargetIdProperty(TargetId::SourceWidth, FName("RectLightComponent.SourceWidth"));
	_engineHelper->RegisterTargetIdProperty(TargetId::SourceHeight, FName("RectLightComponent.SourceHeight"));
	_engineHelper->RegisterTargetIdProperty(TargetId::BarnDoorAngle, FName("RectLightComponent.BarnDoorAngle"));
	_engineHelper->RegisterTargetIdProperty(TargetId::BarnDoorLength, FName("RectLightComponent.BarnDoorLength"));

	// color correct regions
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRType, FName("Type"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRInvert, FName("Invert"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRTemperatureType, FName("TemperatureType"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCREnabled, FName("Enabled"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRExcludeStencil, FName("ExcludeStencil"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRPriority, FName("Priority"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRIntensity, FName("Intensity"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRInner, FName("Inner"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCROuter, FName("Outer"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRFalloff, FName("Falloff"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRTemperature, FName("Temperature"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalSaturationR, FName("ColorGradingSettings.Global.Saturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalSaturationG, FName("ColorGradingSettings.Global.Saturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalSaturationB, FName("ColorGradingSettings.Global.Saturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalSaturationY, FName("ColorGradingSettings.Global.Saturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalContrastR, FName("ColorGradingSettings.Global.Contrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalContrastG, FName("ColorGradingSettings.Global.Contrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalContrastB, FName("ColorGradingSettings.Global.Contrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalContrastY, FName("ColorGradingSettings.Global.Contrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGammaR, FName("ColorGradingSettings.Global.Gamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGammaG, FName("ColorGradingSettings.Global.Gamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGammaB, FName("ColorGradingSettings.Global.Gamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGammaY, FName("ColorGradingSettings.Global.Gamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGainR, FName("ColorGradingSettings.Global.Gain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGainG, FName("ColorGradingSettings.Global.Gain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGainB, FName("ColorGradingSettings.Global.Gain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalGainY, FName("ColorGradingSettings.Global.Gain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalOffsetR, FName("ColorGradingSettings.Global.Offset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalOffsetG, FName("ColorGradingSettings.Global.Offset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalOffsetB, FName("ColorGradingSettings.Global.Offset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRGlobalOffsetY, FName("ColorGradingSettings.Global.Offset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsSaturationR, FName("ColorGradingSettings.Shadows.Saturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsSaturationG, FName("ColorGradingSettings.Shadows.Saturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsSaturationB, FName("ColorGradingSettings.Shadows.Saturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsSaturationY, FName("ColorGradingSettings.Shadows.Saturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsContrastR, FName("ColorGradingSettings.Shadows.Contrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsContrastG, FName("ColorGradingSettings.Shadows.Contrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsContrastB, FName("ColorGradingSettings.Shadows.Contrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsContrastY, FName("ColorGradingSettings.Shadows.Contrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGammaR, FName("ColorGradingSettings.Shadows.Gamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGammaG, FName("ColorGradingSettings.Shadows.Gamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGammaB, FName("ColorGradingSettings.Shadows.Gamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGammaY, FName("ColorGradingSettings.Shadows.Gamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGainR, FName("ColorGradingSettings.Shadows.Gain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGainG, FName("ColorGradingSettings.Shadows.Gain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGainB, FName("ColorGradingSettings.Shadows.Gain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsGainY, FName("ColorGradingSettings.Shadows.Gain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsOffsetR, FName("ColorGradingSettings.Shadows.Offset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsOffsetG, FName("ColorGradingSettings.Shadows.Offset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsOffsetB, FName("ColorGradingSettings.Shadows.Offset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsOffsetY, FName("ColorGradingSettings.Shadows.Offset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesSaturationR, FName("ColorGradingSettings.Midtones.Saturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesSaturationG, FName("ColorGradingSettings.Midtones.Saturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesSaturationB, FName("ColorGradingSettings.Midtones.Saturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesSaturationY, FName("ColorGradingSettings.Midtones.Saturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesContrastR, FName("ColorGradingSettings.Midtones.Contrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesContrastG, FName("ColorGradingSettings.Midtones.Contrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesContrastB, FName("ColorGradingSettings.Midtones.Contrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesContrastY, FName("ColorGradingSettings.Midtones.Contrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGammaR, FName("ColorGradingSettings.Midtones.Gamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGammaG, FName("ColorGradingSettings.Midtones.Gamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGammaB, FName("ColorGradingSettings.Midtones.Gamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGammaY, FName("ColorGradingSettings.Midtones.Gamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGainR, FName("ColorGradingSettings.Midtones.Gain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGainG, FName("ColorGradingSettings.Midtones.Gain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGainB, FName("ColorGradingSettings.Midtones.Gain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesGainY, FName("ColorGradingSettings.Midtones.Gain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesOffsetR, FName("ColorGradingSettings.Midtones.Offset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesOffsetG, FName("ColorGradingSettings.Midtones.Offset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesOffsetB, FName("ColorGradingSettings.Midtones.Offset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRMidtonesOffsetY, FName("ColorGradingSettings.Midtones.Offset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsSaturationR, FName("ColorGradingSettings.Highlights.Saturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsSaturationG, FName("ColorGradingSettings.Highlights.Saturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsSaturationB, FName("ColorGradingSettings.Highlights.Saturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsSaturationY, FName("ColorGradingSettings.Highlights.Saturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsContrastR, FName("ColorGradingSettings.Highlights.Contrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsContrastG, FName("ColorGradingSettings.Highlights.Contrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsContrastB, FName("ColorGradingSettings.Highlights.Contrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsContrastY, FName("ColorGradingSettings.Highlights.Contrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGammaR, FName("ColorGradingSettings.Highlights.Gamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGammaG, FName("ColorGradingSettings.Highlights.Gamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGammaB, FName("ColorGradingSettings.Highlights.Gamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGammaY, FName("ColorGradingSettings.Highlights.Gamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGainR, FName("ColorGradingSettings.Highlights.Gain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGainG, FName("ColorGradingSettings.Highlights.Gain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGainB, FName("ColorGradingSettings.Highlights.Gain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsGainY, FName("ColorGradingSettings.Highlights.Gain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsOffsetR, FName("ColorGradingSettings.Highlights.Offset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsOffsetG, FName("ColorGradingSettings.Highlights.Offset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsOffsetB, FName("ColorGradingSettings.Highlights.Offset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsOffsetY, FName("ColorGradingSettings.Highlights.Offset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CCRShadowsMax, FName("ColorGradingSettings.ShadowsMax"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CCRHighlightsMin, FName("ColorGradingSettings.HighlightsMin"));

	// camera color grading
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseTemperatureType, FName("CameraComponent.PostProcessSettings.bOverride_TemperatureType"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseTemp, FName("CameraComponent.PostProcessSettings.bOverride_WhiteTemp"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseTint, FName("CameraComponent.PostProcessSettings.bOverride_WhiteTint"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPTempType, FName("CameraComponent.PostProcessSettings.TemperatureType"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPTemp, FName("CameraComponent.PostProcessSettings.WhiteTemp"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPTint, FName("CameraComponent.PostProcessSettings.WhiteTint"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseGlobalSaturation, FName("CameraComponent.PostProcessSettings.bOverride_ColorSaturation"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseGlobalContrast, FName("CameraComponent.PostProcessSettings.bOverride_ColorContrast"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseGlobalGamma, FName("CameraComponent.PostProcessSettings.bOverride_ColorGamma"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseGlobalGain, FName("CameraComponent.PostProcessSettings.bOverride_ColorGain"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseGlobalOffset, FName("CameraComponent.PostProcessSettings.bOverride_ColorOffset"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseShadowsSaturation, FName("CameraComponent.PostProcessSettings.bOverride_ColorSaturationShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseShadowsContrast, FName("CameraComponent.PostProcessSettings.bOverride_ColorContrastShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseShadowsGamma, FName("CameraComponent.PostProcessSettings.bOverride_ColorGammaShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseShadowsGain, FName("CameraComponent.PostProcessSettings.bOverride_ColorGainShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseShadowsOffset, FName("CameraComponent.PostProcessSettings.bOverride_ColorOffsetShadows"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseMidtonesSaturation, FName("CameraComponent.PostProcessSettings.bOverride_ColorSaturationMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseMidtonesContrast, FName("CameraComponent.PostProcessSettings.bOverride_ColorContrastMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseMidtonesGamma, FName("CameraComponent.PostProcessSettings.bOverride_ColorGammaMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseMidtonesGain, FName("CameraComponent.PostProcessSettings.bOverride_ColorGainMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseMidtonesOffset, FName("CameraComponent.PostProcessSettings.bOverride_ColorOffsetMidtones"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseHighlightsSaturation, FName("CameraComponent.PostProcessSettings.bOverride_ColorSaturationHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseHighlightsContrast, FName("CameraComponent.PostProcessSettings.bOverride_ColorContrastHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseHighlightsGamma, FName("CameraComponent.PostProcessSettings.bOverride_ColorGammaHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseHighlightsGain, FName("CameraComponent.PostProcessSettings.bOverride_ColorGainHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPUseHighlightsOffset, FName("CameraComponent.PostProcessSettings.bOverride_ColorOffsetHighlights"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalSaturationR, FName("CameraComponent.PostProcessSettings.ColorSaturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalSaturationG, FName("CameraComponent.PostProcessSettings.ColorSaturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalSaturationB, FName("CameraComponent.PostProcessSettings.ColorSaturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalSaturationY, FName("CameraComponent.PostProcessSettings.ColorSaturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalContrastR, FName("CameraComponent.PostProcessSettings.ColorContrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalContrastG, FName("CameraComponent.PostProcessSettings.ColorContrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalContrastB, FName("CameraComponent.PostProcessSettings.ColorContrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalContrastY, FName("CameraComponent.PostProcessSettings.ColorContrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGammaR, FName("CameraComponent.PostProcessSettings.ColorGamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGammaG, FName("CameraComponent.PostProcessSettings.ColorGamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGammaB, FName("CameraComponent.PostProcessSettings.ColorGamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGammaY, FName("CameraComponent.PostProcessSettings.ColorGamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGainR, FName("CameraComponent.PostProcessSettings.ColorGain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGainG, FName("CameraComponent.PostProcessSettings.ColorGain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGainB, FName("CameraComponent.PostProcessSettings.ColorGain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalGainY, FName("CameraComponent.PostProcessSettings.ColorGain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalOffsetR, FName("CameraComponent.PostProcessSettings.ColorOffset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalOffsetG, FName("CameraComponent.PostProcessSettings.ColorOffset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalOffsetB, FName("CameraComponent.PostProcessSettings.ColorOffset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPGlobalOffsetY, FName("CameraComponent.PostProcessSettings.ColorOffset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsSaturationR, FName("CameraComponent.PostProcessSettings.ColorSaturationShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsSaturationG, FName("CameraComponent.PostProcessSettings.ColorSaturationShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsSaturationB, FName("CameraComponent.PostProcessSettings.ColorSaturationShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsSaturationY, FName("CameraComponent.PostProcessSettings.ColorSaturationShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsContrastR, FName("CameraComponent.PostProcessSettings.ColorContrastShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsContrastG, FName("CameraComponent.PostProcessSettings.ColorContrastShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsContrastB, FName("CameraComponent.PostProcessSettings.ColorContrastShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsContrastY, FName("CameraComponent.PostProcessSettings.ColorContrastShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGammaR, FName("CameraComponent.PostProcessSettings.ColorGammaShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGammaG, FName("CameraComponent.PostProcessSettings.ColorGammaShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGammaB, FName("CameraComponent.PostProcessSettings.ColorGammaShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGammaY, FName("CameraComponent.PostProcessSettings.ColorGammaShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGainR, FName("CameraComponent.PostProcessSettings.ColorGainShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGainG, FName("CameraComponent.PostProcessSettings.ColorGainShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGainB, FName("CameraComponent.PostProcessSettings.ColorGainShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsGainY, FName("CameraComponent.PostProcessSettings.ColorGainShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsOffsetR, FName("CameraComponent.PostProcessSettings.ColorOffsetShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsOffsetG, FName("CameraComponent.PostProcessSettings.ColorOffsetShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsOffsetB, FName("CameraComponent.PostProcessSettings.ColorOffsetShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPShadowsOffsetY, FName("CameraComponent.PostProcessSettings.ColorOffsetShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesSaturationR, FName("CameraComponent.PostProcessSettings.ColorSaturationMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesSaturationG, FName("CameraComponent.PostProcessSettings.ColorSaturationMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesSaturationB, FName("CameraComponent.PostProcessSettings.ColorSaturationMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesSaturationY, FName("CameraComponent.PostProcessSettings.ColorSaturationMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesContrastR, FName("CameraComponent.PostProcessSettings.ColorContrastMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesContrastG, FName("CameraComponent.PostProcessSettings.ColorContrastMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesContrastB, FName("CameraComponent.PostProcessSettings.ColorContrastMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesContrastY, FName("CameraComponent.PostProcessSettings.ColorContrastMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGammaR, FName("CameraComponent.PostProcessSettings.ColorGammaMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGammaG, FName("CameraComponent.PostProcessSettings.ColorGammaMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGammaB, FName("CameraComponent.PostProcessSettings.ColorGammaMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGammaY, FName("CameraComponent.PostProcessSettings.ColorGammaMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGainR, FName("CameraComponent.PostProcessSettings.ColorGainMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGainG, FName("CameraComponent.PostProcessSettings.ColorGainMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGainB, FName("CameraComponent.PostProcessSettings.ColorGainMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesGainY, FName("CameraComponent.PostProcessSettings.ColorGainMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesOffsetR, FName("CameraComponent.PostProcessSettings.ColorOffsetMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesOffsetG, FName("CameraComponent.PostProcessSettings.ColorOffsetMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesOffsetB, FName("CameraComponent.PostProcessSettings.ColorOffsetMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPMidtonesOffsetY, FName("CameraComponent.PostProcessSettings.ColorOffsetMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsSaturationR, FName("CameraComponent.PostProcessSettings.ColorSaturationHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsSaturationG, FName("CameraComponent.PostProcessSettings.ColorSaturationHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsSaturationB, FName("CameraComponent.PostProcessSettings.ColorSaturationHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsSaturationY, FName("CameraComponent.PostProcessSettings.ColorSaturationHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsContrastR, FName("CameraComponent.PostProcessSettings.ColorContrastHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsContrastG, FName("CameraComponent.PostProcessSettings.ColorContrastHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsContrastB, FName("CameraComponent.PostProcessSettings.ColorContrastHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsContrastY, FName("CameraComponent.PostProcessSettings.ColorContrastHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGammaR, FName("CameraComponent.PostProcessSettings.ColorGammaHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGammaG, FName("CameraComponent.PostProcessSettings.ColorGammaHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGammaB, FName("CameraComponent.PostProcessSettings.ColorGammaHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGammaY, FName("CameraComponent.PostProcessSettings.ColorGammaHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGainR, FName("CameraComponent.PostProcessSettings.ColorGainHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGainG, FName("CameraComponent.PostProcessSettings.ColorGainHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGainB, FName("CameraComponent.PostProcessSettings.ColorGainHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsGainY, FName("CameraComponent.PostProcessSettings.ColorGainHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsOffsetR, FName("CameraComponent.PostProcessSettings.ColorOffsetHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsOffsetG, FName("CameraComponent.PostProcessSettings.ColorOffsetHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsOffsetB, FName("CameraComponent.PostProcessSettings.ColorOffsetHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::CPPHighlightsOffsetY, FName("CameraComponent.PostProcessSettings.ColorOffsetHighlights.W"));

	// post processing
	_engineHelper->RegisterTargetIdProperty(TargetId::UseTemperatureType, FName("Settings.bOverride_TemperatureType"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseTemp, FName("Settings.bOverride_WhiteTemp"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseTint, FName("Settings.bOverride_WhiteTint"));

	_engineHelper->RegisterTargetIdProperty(TargetId::TempType, FName("Settings.TemperatureType"));
	_engineHelper->RegisterTargetIdProperty(TargetId::Temp, FName("Settings.WhiteTemp"));
	_engineHelper->RegisterTargetIdProperty(TargetId::Tint, FName("Settings.WhiteTint"));

	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalSaturationR, FName("Settings.ColorSaturation.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalSaturationG, FName("Settings.ColorSaturation.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalSaturationB, FName("Settings.ColorSaturation.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalSaturationY, FName("Settings.ColorSaturation.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalContrastR, FName("Settings.ColorContrast.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalContrastG, FName("Settings.ColorContrast.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalContrastB, FName("Settings.ColorContrast.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalContrastY, FName("Settings.ColorContrast.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGammaR, FName("Settings.ColorGamma.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGammaG, FName("Settings.ColorGamma.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGammaB, FName("Settings.ColorGamma.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGammaY, FName("Settings.ColorGamma.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGainR, FName("Settings.ColorGain.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGainG, FName("Settings.ColorGain.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGainB, FName("Settings.ColorGain.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalGainY, FName("Settings.ColorGain.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalOffsetR, FName("Settings.ColorOffset.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalOffsetG, FName("Settings.ColorOffset.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalOffsetB, FName("Settings.ColorOffset.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::GlobalOffsetY, FName("Settings.ColorOffset.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsSaturationR, FName("Settings.ColorSaturationShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsSaturationG, FName("Settings.ColorSaturationShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsSaturationB, FName("Settings.ColorSaturationShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsSaturationY, FName("Settings.ColorSaturationShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsContrastR, FName("Settings.ColorContrastShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsContrastG, FName("Settings.ColorContrastShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsContrastB, FName("Settings.ColorContrastShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsContrastY, FName("Settings.ColorContrastShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGammaR, FName("Settings.ColorGammaShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGammaG, FName("Settings.ColorGammaShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGammaB, FName("Settings.ColorGammaShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGammaY, FName("Settings.ColorGammaShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGainR, FName("Settings.ColorGainShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGainG, FName("Settings.ColorGainShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGainB, FName("Settings.ColorGainShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsGainY, FName("Settings.ColorGainShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsOffsetR, FName("Settings.ColorOffsetShadows.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsOffsetG, FName("Settings.ColorOffsetShadows.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsOffsetB, FName("Settings.ColorOffsetShadows.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::ShadowsOffsetY, FName("Settings.ColorOffsetShadows.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesSaturationR, FName("Settings.ColorSaturationMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesSaturationG, FName("Settings.ColorSaturationMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesSaturationB, FName("Settings.ColorSaturationMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesSaturationY, FName("Settings.ColorSaturationMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesContrastR, FName("Settings.ColorContrastMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesContrastG, FName("Settings.ColorContrastMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesContrastB, FName("Settings.ColorContrastMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesContrastY, FName("Settings.ColorContrastMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGammaR, FName("Settings.ColorGammaMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGammaG, FName("Settings.ColorGammaMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGammaB, FName("Settings.ColorGammaMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGammaY, FName("Settings.ColorGammaMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGainR, FName("Settings.ColorGainMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGainG, FName("Settings.ColorGainMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGainB, FName("Settings.ColorGainMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesGainY, FName("Settings.ColorGainMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesOffsetR, FName("Settings.ColorOffsetMidtones.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesOffsetG, FName("Settings.ColorOffsetMidtones.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesOffsetB, FName("Settings.ColorOffsetMidtones.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::MidtonesOffsetY, FName("Settings.ColorOffsetMidtones.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsSaturationR, FName("Settings.ColorSaturationHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsSaturationG, FName("Settings.ColorSaturationHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsSaturationB, FName("Settings.ColorSaturationHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsSaturationY, FName("Settings.ColorSaturationHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsContrastR, FName("Settings.ColorContrastHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsContrastG, FName("Settings.ColorContrastHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsContrastB, FName("Settings.ColorContrastHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsContrastY, FName("Settings.ColorContrastHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGammaR, FName("Settings.ColorGammaHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGammaG, FName("Settings.ColorGammaHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGammaB, FName("Settings.ColorGammaHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGammaY, FName("Settings.ColorGammaHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGainR, FName("Settings.ColorGainHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGainG, FName("Settings.ColorGainHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGainB, FName("Settings.ColorGainHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsGainY, FName("Settings.ColorGainHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsOffsetR, FName("Settings.ColorOffsetHighlights.X"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsOffsetG, FName("Settings.ColorOffsetHighlights.Y"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsOffsetB, FName("Settings.ColorOffsetHighlights.Z"));
	_engineHelper->RegisterTargetIdProperty(TargetId::HighlightsOffsetY, FName("Settings.ColorOffsetHighlights.W"));

	_engineHelper->RegisterTargetIdProperty(TargetId::UseGlobalSaturation, FName("Settings.bOverride_ColorSaturation"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseGlobalContrast, FName("Settings.bOverride_ColorContrast"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseGlobalGamma, FName("Settings.bOverride_ColorGamma"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseGlobalGain, FName("Settings.bOverride_ColorGain"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseGlobalOffset, FName("Settings.bOverride_ColorOffset"));

	_engineHelper->RegisterTargetIdProperty(TargetId::UseShadowsSaturation, FName("Settings.bOverride_ColorSaturationShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseShadowsContrast, FName("Settings.bOverride_ColorContrastShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseShadowsGamma, FName("Settings.bOverride_ColorGammaShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseShadowsGain, FName("Settings.bOverride_ColorGainShadows"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseShadowsOffset, FName("Settings.bOverride_ColorOffsetShadows"));

	_engineHelper->RegisterTargetIdProperty(TargetId::UseMidtonesSaturation, FName("Settings.bOverride_ColorSaturationMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseMidtonesContrast, FName("Settings.bOverride_ColorContrastMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseMidtonesGamma, FName("Settings.bOverride_ColorGammaMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseMidtonesGain, FName("Settings.bOverride_ColorGainMidtones"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseMidtonesOffset, FName("Settings.bOverride_ColorOffsetMidtones"));

	_engineHelper->RegisterTargetIdProperty(TargetId::UseHighlightsSaturation, FName("Settings.bOverride_ColorSaturationHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseHighlightsContrast, FName("Settings.bOverride_ColorContrastHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseHighlightsGamma, FName("Settings.bOverride_ColorGammaHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseHighlightsGain, FName("Settings.bOverride_ColorGainHighlights"));
	_engineHelper->RegisterTargetIdProperty(TargetId::UseHighlightsOffset, FName("Settings.bOverride_ColorOffsetHighlights"));
}

// order of calls is Init() -> Run() -> Exit(), Stop() is called if the thread is reqired to stop early
TangentEngine::TangentEngine()
{
	// we use an editor-capable module to deal with changes in unreal, it is implemented as a uobject so we
	// use the system based constructor and prevent garbage collection by adding to the root set
	_engineHelper = NewObject<UEngineHelper>();
	_engineHelper->AddToRoot();

	// we support some mapping targets using their ids associated with named property paths, these need to be registered with 
	// the engine helper in advance
	RegisterTargetIds();

	// enable notifications for when the selected object in the editor changes and we may want to change mode to match
	UEngineHelper::SelectionChangedEvent.AddRaw(this, &TangentEngine::OnSelectionChanged);

	// listen for actor property changes made in the editor ui
	UEngineHelper::ActorPropertyChangedEvent.AddRaw(this, &TangentEngine::OnActorPropertyChanged);

	// listen for control training responses generated in the editor ui
	UEngineHelper::TrainResponseEvent.AddRaw(this, &TangentEngine::OnTrainResponse);

	// listen for incoming menu string changes
	UEngineHelper::MenuStringEvent.AddRaw(this, &TangentEngine::OnMenuString);

	// listen for incoming parameter value changes
	UEngineHelper::ParameterValueEvent.AddRaw(this, &TangentEngine::OnParameterValue);

	// listen for incoming custom parameter value changes
	UEngineHelper::CustomParameterValueEvent.AddRaw(this, &TangentEngine::OnCustomParameterValue);

	// listen for incoming custom menu string changes
	UEngineHelper::CustomMenuStringEvent.AddRaw(this, &TangentEngine::OnCustomMenuString);

	// finally load any persistent settings from an optional predefined file
	LoadSettings();
}

// destructor
TangentEngine::~TangentEngine()
{
	// tidy up event handling
	UEngineHelper::SelectionChangedEvent.RemoveAll(this);
	UEngineHelper::ActorPropertyChangedEvent.RemoveAll(this);
	UEngineHelper::TrainResponseEvent.RemoveAll(this);
	UEngineHelper::MenuStringEvent.RemoveAll(this);
	UEngineHelper::ParameterValueEvent.RemoveAll(this);
	UEngineHelper::CustomParameterValueEvent.RemoveAll(this);
	UEngineHelper::CustomMenuStringEvent.RemoveAll(this);

	// remove the helper instance, it is a uobject so do not delete!
// note - this call has been removed for now as it causes an engine assertion in ue 5.5
//	_engineHelper->RemoveFromRoot();
	_engineHelper = nullptr;
}

// chooses a mode associated with the given class of actor, this is taken from the local class to mode map data and converted to a mode id, if there is
// no match we return the special case no change value
uint32 TangentEngine::ChooseModeForObjectClass(const FName& className)
{
	// start by looking up any mode associated with the suppied class
	FName* modeName = _classToModeMap.Find(className);
	if (modeName)
	{
		// there is a mode linked to this class, try to convert the mode name to the actual mode id, it will return with no change if there is no match
		return ConvertMode(*modeName);
	}

	// getting here means no class based mode to use, return the special case no change value
	return ModeId::NoChange;
}

// chooses a mode to match the given type of actor, we support specific preset modes, if there is no direct support for an object type we return
// the preset default mode id value
uint32 TangentEngine::ChooseModeForObjectType(UEngineHelper::ObjectType objectType)
{
	// we want to use one of the presets, convert the given type to a preset mode or the special case no change value
	switch (objectType)
	{
		case UEngineHelper::ObjectType::Light:
			return ModeId::Light;
		case UEngineHelper::ObjectType::Camera:
			return ModeId::Camera;
		case UEngineHelper::ObjectType::PostProcess:
			return ModeId::PostProcess;
		case UEngineHelper::ObjectType::Sequencer:
			return ModeId::Sequencer;
		case UEngineHelper::ObjectType::ColorCorrect:
			return ModeId::ColorCorrect;
		default:
			return ModeId::Default;
	}
}

// helper function to try and convert a mode name to its associated id using the local mode list, it returns the special case no change
// value if there is no match
uint32 TangentEngine::ConvertMode(const FName& modeName)
{
	// look up the id value in the list by name key, returns null if there is no match
	uint32 *modeIdPtr = _modeList.Find(modeName);
	if (modeIdPtr)
	{
		// found a match, return the mode id
		return *modeIdPtr;
	}

	// no match, return the special case no change mode id value
	UE_LOG(LogTemp, Warning, TEXT("TangentEngine: ConvertMode - no match for mode name '%s'"), *modeName.ToString());
	return ModeId::NoChange;
}

// helper function to try and convert a mode id to its associated name using the local mode list, it returns an empty name if there is no match
FName TangentEngine::ConvertMode(uint32 modeId)
{
	// look up the name key in the list using the id as the value to search for
	const FName *modeNamePtr = _modeList.FindKey(modeId);
	if (modeNamePtr)
	{
		// found a match, return the name
		return *modeNamePtr;
	}

	// no match, return a default empty name
	UE_LOG(LogTemp, Warning, TEXT("TangentEngine: ConvertMode - no match for mode id 0x%08X"), modeId);
	return FName();
}

// called by the editor helper when the selected object has been changed
void TangentEngine::OnSelectionChanged()
{
	// default to no mode change required
	uint32 modeId = ModeId::NoChange;

	// the selection has changed and we may want to change mode to match, this behaviour is driven by a setting maintained by the helper class
	if (_engineHelper->GetSelectionChangesMode())
	{
		// start by looking up any mode associated with the class of the selected object, it will return with no change if there is no match
		modeId = ChooseModeForObjectClass(_engineHelper->GetSelectedClassName());

		// we give the class selection priority but if it fails to find a new mode we fallback to choosing a preset
		if (modeId == ModeId::NoChange)
		{
			// there is no valid mode linked to the class, try and get a preset for the abstracted object type, it will return with the preset default if 
			// there is no preset directly associated with the type
			modeId = ChooseModeForObjectType(_engineHelper->GetSelectedObjectType());
		}
	}

	// we may be supplied with a defined mode to change to, if so we change to that mode, if no mode change is specified or we are
	// already in that mode we trigger an all change to refresh the panels 
	if ((modeId != ModeId::NoChange) && (_currentMode != modeId))
	{
		// we have a new mode to select, this triggers a mode change command to the hub
		SetCurrentMode(modeId);
	}
	else
	{
		// either no mode change required or already in the specified mode, send an all change to the hub to refresh everything
		// in case of a new selected object of the same type
		SendAllChange();
	}
}

// called by the editor helper when an actor has had a property value changed in the editor ui, the helper filters events
void TangentEngine::OnActorPropertyChanged()
{
	// send all change to update panel displays that may have the changed value mapped to controls
	SendAllChange();
}

// called to try and connect to the hub, it creates a socket if one is not already in place, so it may be called for both initial
// set up and to retry to connect after a previous attempt failed
void TangentEngine::ConnectHubComms()
{
	// it is valid to call this function with an existing socket instance, we only create a new one if required
	if (!_socket)
	{
		// socket needed, use the handy builder class to create a blocking tcp socket with our buffer requirements, note that the
		// receive buffer is a maximum of a full message size as we receive the header in a separate interaction, whereas the send 
		// buffer will always have the header and message contents combined
		_socket = FTcpSocketBuilder(TEXT("TangentHubSocket")).AsBlocking().WithReceiveBufferSize(IPCComms::MaxMessageLen).WithSendBufferSize(IPCComms::HeaderLen + IPCComms::MaxMessageLen).Build();
		if (!_socket)
		{
			// just in case...
			UE_LOG(LogTemp, Fatal, TEXT("TangentEngine: failed to create socket"));
			return;
		}

		// set up the socket so sends just go
		_socket->SetNoDelay(true);
	}

	// use the weird and wonderful way to create an address that can take a port setting
	TSharedRef<FInternetAddr> hubAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	// configure for the preset ipc port number the target ip address
	// TODO: configure the ip address from a plugin setting?
	bool addrIsValid = false;
	hubAddr.Get().SetPort(IPCComms::Port);
#ifdef TANGENT_DEVELOPMENT
	// separate development machine
	hubAddr.Get().SetIp(TEXT("192.168.0.168"), addrIsValid);
#else
	// localhost
	hubAddr.Get().SetIp(TEXT("127.0.0.1"), addrIsValid);
#endif // TANGENT_DEVELOPMENT

	// sanity check
	if (addrIsValid)
	{
		// all good to go, try to connect
		if (_socket->Connect(*hubAddr))
		{
			// all good again
			UE_LOG(LogTemp, Log, TEXT("TangentEngine: connected to the hub"));
		}
		else
		{
			// log a warning
			UE_LOG(LogTemp, Warning, TEXT("TangentEngine: failed to connect to the hub"));
		}
	}
	else
	{
		// log a warning that the address is wrong
		UE_LOG(LogTemp, Warning, TEXT("TangentEngine: hub address %s is invalid"), *hubAddr.Get().ToString(true));
	}
}

// called to shutdown any ongoing socket based hub comms and release all resources
void TangentEngine::ResetHubComms()
{
	// it is valid for this to be called with no existing socket, so protect against this 
	if (_socket)
	{
		// close and delete the socket
		_socket->Close();
		delete _socket;
		_socket = nullptr;
	}
}

// sends the specified pre-built message to the hub
void TangentEngine::SendToHub(MessageBuilder &msg)
{
	// sanity check
	if (_socket)
	{
		uint8*	message = nullptr;
		int32	messageLen = 0,
				bytesSent = 0;

		// the message must have been built to be valid to send, getting the data returns true if this is the case
		if (msg.GetMessage(message, messageLen))
		{
			// try to send, getting the resulting byte count that went out the door
			_socket->Send(message, messageLen, bytesSent);

			// warn about short sends, this should never happen with buffering and blocking, but just in case
			if (bytesSent != messageLen)
			{
				// just warn, nothing more to be done
				UE_LOG(LogTemp, Warning, TEXT("TangentEngine: short send of %d byte(s) out of a total of %d"), bytesSent, messageLen);
			}
		}
		else
		{
			// warn of trying to send a message that has not had the build step completed
			UE_LOG(LogTemp, Warning, TEXT("TangentEngine: ignoring call to send a message that has not been built"));
		}
	}
}

// helper function to send a rename control command to the hub, an empty string resets the name to the default
void TangentEngine::SendRenameControl(uint32 controlId, const FString& name)
{
	// format is the control id to rename and the text, if the text is an empty string the hub will reset to the default 
	// name for that control
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::RenameControl);
	msg.WriteUInt32(controlId);
	msg.WriteString(name);
	msg.Build();
	SendToHub(msg);
}

// helper function to send an enable requests all modes command to the hub, this enables extended functionality so the hub will
// report all mode changes, for both user and app defined modes, via this app plugin, this allows us to track current mode at all times
void TangentEngine::SendEnableRequestsAllModes()
{
	// format is just the command id
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::EnableRequestsAllModes);
	msg.Build();
	SendToHub(msg);
}

// helper function to send an all change command to the hub
void TangentEngine::SendAllChange()
{
	// format is just the command id
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::AllChange);
	msg.Build();
	SendToHub(msg);
}

// returns the full path to the predefined file that may be used to save the selection action map data, the file is optional
FString TangentEngine::SelectActionMapFile()
{
	// we place the file in the config sub-folder inside the project folder
	// (note - moved to config from saved in v1.1.2 (14))
	FString filePath = FPaths::ProjectConfigDir() + TEXT("Tangent/SelectActionMap.ini");
	return FPaths::ConvertRelativePathToFull(filePath);
}

// resets the current select action map, removing any existing settings and optionally deleting the persistent store file
void TangentEngine::ResetSelectActionMap(bool deleteFile)
{
	// check if there is any loaded data to reset
	if (_selectActionMap.Num() > 0)
	{
		// to reset any existing map data we must first tell the hub of the change by resetting any renamed controls
		// back to the default by setting an empty string for each control id
		FString emptyString = FString();
		for (const TPair<uint32, FString>& pair : _selectActionMap)
		{
			// reset for each mapped control id 
			SendRenameControl(pair.Key, emptyString);
		}
		
		// it is now safe to clear the map itself of any data
		_selectActionMap.Empty();
	}

	// the map data is also stored in a file, check if requested to delete this, we may want to reset just the data in memory
	if (deleteFile)
	{
		// simply delete the file if it exists, no need for a warning if not and even if read only
		FString filePath = SelectActionMapFile();
		IFileManager::Get().Delete(*filePath, false, true, true);
	}
}

// clears any existing select action map data (including sending reset commands to the hub) and then loads the data from the predefined
// file if it exists, updating the hub as required
void TangentEngine::LoadSelectActionMap()
{
	// as part of the load process we automatically remove any existing loaded data, this call only clears data in memory and leaves 
	// any map file in place as we will be loading from it next
	ResetSelectActionMap();

	// the data is stored in a preset location in the project folder structure 
	FString filePath = SelectActionMapFile();

	// the file is optional so check if there is one to load to avoid logging a warning if the optional file is not present
	if (IFileManager::Get().FileExists(*filePath))
	{
		// first step is to load the file as a byte array
		TArray<uint8> byteArray;
		if (FFileHelper::LoadFileToArray(byteArray, *filePath))
		{
			// to de-archive we use a memory reader pointing to the loaded array
			FMemoryReader dataReader = FMemoryReader(byteArray, true);
			
			// we will read the data into the same objects as the original select actor list, a map of pairs of 
			// uint control ids and string names
			TMap<uint32, FString>	newMap;

			// read all the data to create a new map
			dataReader.Seek(0);
			dataReader << newMap;

			// release all resources and tidy up the reader
			dataReader.FlushCache();
			dataReader.Close();

			// also now done with the loaded array
			byteArray.Empty();

			// assign the loaded data to the local store
			_selectActionMap = newMap;

			// to complete the load we must tell the hub of the new names for the control ids
			for (const TPair<uint32, FString>& pair : _selectActionMap)
			{
				// the pair value is the original name string
				FString name = pair.Value;
				int32	index;

				// the name string may have a full path of an actor name followed by a component name, in these contexts we want to relabel the control with the
				// more spcific component name, look for the separating character to see if we need to split, if not we simply use the original name
				if (name.FindLastChar(TCHAR('.'), index))
				{
					// we found a full name, chop at the character following the separator to get the component name
					name = name.RightChop(index + 1);
				}

				// as these are displayed on the panels we clip to a length that is most useful for most panels
				name = ClipString(name);

				// the pair key is the control id, rename to whatever we got from the above
				SendRenameControl(pair.Key, name);
			}
		}
	}
}

// saves the map table of object class to mode data to the preset file, this map is slightly different as it is a custom map that saves to text
void TangentEngine::SaveClassToModeMap()
{
	// ask the custom map to save to the preset file
	_classToModeMap.SaveToFile();
}

// helper function to reset the class to mode map data
void TangentEngine::ResetClassToModeMap()
{
	// ask the custom map to reset and create a default file
	_classToModeMap.ResetToDefault();
}

// saves the map table of select actor control ids to actor names to a named file so the data can persist between runs
void TangentEngine::SaveSelectActionMap()
{
	// the data is stored in a preset location in the project folder structure 
	FString filePath = SelectActionMapFile();

	// if we have data, we save to the file, replacing anything previously saved, if there is no data we simply delete any
	// existing file to remove any legacy saves
	if (_selectActionMap.Num() > 0)
	{
		// serialise the data table to the archive
		FBufferArchive dataArchive;
		dataArchive << _selectActionMap;

		// save this data to the file
		FFileHelper::SaveArrayToFile(dataArchive, *filePath);

		// release all the resources used to serialise the data
		dataArchive.FlushCache();
		dataArchive.Empty();
	}
	else
	{
		// no data, quietly delete the file if it exists, no need for a warning if not and even if read only
		IFileManager::Get().Delete(*filePath, false, true, true);
	}
}

// returns the full path to the predefined file that may be used to save the select mode map data, the file is optional
FString TangentEngine::SelectModeMapFile()
{
	// we place the file in the config sub-folder inside the project folder
	// (note - moved to config from saved in v1.1.2 (14))
	FString filePath = FPaths::ProjectConfigDir() + TEXT("Tangent/SelectModeMap.ini");
	return FPaths::ConvertRelativePathToFull(filePath);
}

// deletes any legacy select mode map file from the preset location
void TangentEngine::RemoveLegacySelectModeMap()
{
	// simply delete the file if it exists, no need for a warning if not and even if read only
	FString filePath = SelectModeMapFile();
	IFileManager::Get().Delete(*filePath, false, true, true);
}

// returns the full path to the predefined file that may be used to save various settings, the file is optional
FString TangentEngine::SettingsFile()
{
	// we place the file in the config sub-folder inside the project folder
	// (note - moved to config from saved in v1.1.2 (14))
	FString filePath = FPaths::ProjectConfigDir() + TEXT("Tangent/Settings.ini");
	return FPaths::ConvertRelativePathToFull(filePath);
}

// loads any setting values from the optional persistent store and sets them as required
void TangentEngine::LoadSettings()
{
	// the data is stored in a preset location in the project folder structure 
	FString filePath = SettingsFile();

	// the file is optional so check if there is one to load to avoid logging a warning if the optional file is not present
	if (IFileManager::Get().FileExists(*filePath))
	{
		// first step is to load the file as a byte array
		TArray<uint8> byteArray;
		if (FFileHelper::LoadFileToArray(byteArray, *filePath))
		{
			// to de-archive we use a memory reader pointing to the loaded array
			FMemoryReader	dataReader = FMemoryReader(byteArray, true);
			bool			bValue;

			// read all the data and store appropriately
			dataReader.Seek(0);
			dataReader << bValue;
			_engineHelper->SetSelectionChangesMode(bValue);

			// release all resources and tidy up the reader
			dataReader.FlushCache();
			dataReader.Close();

			// also now done with the loaded array
			byteArray.Empty();
		}
	}
}

// saves any relevant setting values to the persistent predefined file as required
void TangentEngine::SaveSettings()
{
	// the data is stored in a preset location in the project folder structure 
	FString			filePath = SettingsFile();
	FBufferArchive	dataArchive;
	bool			bValue;

	// serialise the various settings to the archive, in order
	bValue = _engineHelper->GetSelectionChangesMode();
	dataArchive << bValue;

	// save this data to the file
	FFileHelper::SaveArrayToFile(dataArchive, *filePath);

	// release all the resources used to serialise the data
	dataArchive.FlushCache();
	dataArchive.Empty();
}

// called on an action on command to register that an action is being processed
void TangentEngine::UpdateActionFilterOn()
{
	// we count actions coming in so we can choose to ignore the corresponding action off events if a mode change has 
	// come in between, this may happen if the action on event does something that causes the mode to change which
	// may change the mapping for the matching action off causing potentially incorrect behaviour
	_actionFilterCounter++;
}

// called on action off events returning true if the event is to be processed as normal, if a mode change has come in since the
// action on event and cleared any pending action off processing the function will return false
bool TangentEngine::CheckActionFilterOff()
{
	// if the action filter counter is zero then we have no valid pending action off events to process, if the counter is active
	// then we decrement it to continue as usual
	if (_actionFilterCounter > 0)
	{
		// we have one or more pending action off events so count this one and return true to show the caller it may continue
		_actionFilterCounter--;
		return true;
	}

	// getting here means we have had any pending action off events reset (due to a mode change) so return false to tell the caller
	// that it should not process this event
	return false;
}

// called to effectively negate any pending action off events, this is called in response to a mode change that would affect the
// control mappings if a mode change happens between an action on and action off
void TangentEngine::ResetActionFilter()
{
	// reset the action counter to prevent pending action off events from being processed 
	_actionFilterCounter = 0;
}

// selects the specified mode id as the currently active mode and sends a confirmation mode value update to the hub
void TangentEngine::SetCurrentMode(uint32 modeId)
{
	// a mode change must prevent any pending action(s) from triggering on action off, reset this now
	ResetActionFilter();

	// update the local store
	_currentMode = modeId;

	// send the mode value to the hub
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::ModeValue);
	msg.WriteUInt32(modeId);
	msg.Build();
	SendToHub(msg);
}

// returns the fully qualified path to the xml system folder in the plugin's resources
FString TangentEngine::GetPluginXmlFolder()
{
	// first get the plugin manager and locate where this plugin actually is based, this allows for development/project/engine variations
	IPluginManager& man = IPluginManager::Get();
	TSharedPtr<IPlugin> plug = man.FindPlugin(TEXT("Tangent"));

	// sanity check
	if (plug.IsValid())
	{
		// get the base dir of this plugin and convert to a fully qualified path
		FString baseDir = plug->GetBaseDir();
		FString fullPath = FPaths::ConvertRelativePathToFull(baseDir);

		// append the resource and preset path to the xml folder that contains our app system files
		FString xmlFolder = FPaths::Combine(fullPath, TEXT("Resources"), TEXT("XML"));

		// return the result
		return xmlFolder;
	}

	// getting here means something went wrong, fallback to a safe default empty string
	UE_LOG(LogTemp, Warning, TEXT("TangentEngine: failed to get XML resources path for the plugin"));
	return FString();
}

// handles an initiate comms command, generating an application definition response to the hub
void TangentEngine::ParseInitCommsCmd(RxContext &rxContext)
{
	uint32			panelCount,
					protocolRevision;
	MessageBuilder	msg;

	// the format is ipc protocol revision, configured panel count and then panel definition data (that we skip)
	protocolRevision = rxContext.ReadUInt32();
	panelCount = rxContext.ReadUInt32();
	rxContext.Skip(panelCount * 8);
	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx InitiateComms command [ IPC %d, %d panel(s) ]"), protocolRevision, panelCount);

#ifdef TANGENT_DEVELOPMENT
	// path on separate development machine
	FString sysFolder = FString(TEXT("/Library/Application Support/Tangent/Hub/KeypressApps/Unreal Editor"));
#else
	// local path relative to this plugin
	FString sysFolder = GetPluginXmlFolder();
#endif // TANGENT_DEVELOPMENT

	// build an app definition message for this app, format is the app name, system folder path and user folder path
	msg.WriteUInt32(AppCommand::ApplicationDefinition);
	msg.WriteString("Unreal Editor");
	msg.WriteString(sysFolder);
	msg.WriteString("");

	// for ipc of 7 or more we may optionally append the process name to be associated with the control maps available for the app
	if (protocolRevision >= 7)
	{
		// switch by platform definition and insert the compile time major engine version - mac/win
#if PLATFORM_WINDOWS
		// windows executable setting
		msg.WriteString("UE" QUOTEME(ENGINE_MAJOR_VERSION) "Editor.exe");
#elif PLATFORM_MAC
		// mac executable setting
		msg.WriteString("UE" QUOTEME(ENGINE_MAJOR_VERSION) "Editor");
#endif
	}

	// build and send the message
	msg.Build();
	SendToHub(msg);

	// now that we have introduced ourselves, we want to enable the extended functionality to have the hub report all mode changes to us for
	// both user and app defined modes, this allows us to track the current mode at all times which we want for our own use
	SendEnableRequestsAllModes();

	// finally load any saved select actor data from the predefined but optional file, we can only do this here as we may need to update the
	// hub with rename control commands that we can only send once the plugin is connected and defined
	LoadSelectActionMap();
}

// handles a parameter change command and applies the change to unreal, some parameters may be cached and applied by the caller, this
// allows us to combine individual control updates for properties in unreal that are set by the same api call such as location, rotation etc.
void TangentEngine::ParseParamChangeCmd(RxContext& rxContext)
{
	uint32		paramId;
	float		delta;

	// the format is the target control id and value delta, the delta will never be zero
	paramId = rxContext.ReadUInt32();
	delta = rxContext.ReadFloat32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ParameterChange command [ id 0x%08X, delta %f ]"), paramId, delta);

	// there are a large number of post process volume and color correct region based params which we don't want to have to support individually 
	// so these target ids are bit encoded with a master detection flag, test for these as exceptions first, if not encoded then carry on as usual
	if (TargetId::IsGradingHSV(paramId))
	{
		// grading based param change for hsv, handle here then we are all done so return
		_engineHelper->UpdateGradingHSV(paramId, delta);
		return;
	}
	else if (TargetId::IsGradingXYZ(paramId))
	{
		// grading based param change for xyz, handle here then we are all done so return
		_engineHelper->UpdateGradingXYZ(paramId, delta);
		return;
	}

	// handle by target id, using the helper class to make the changes in the editor or caching as required
	switch (paramId)
	{
		case TargetId::CameraPan:
			// camera pan is yaw around the local z axis
			_engineHelper->UpdateCameraPan(delta);
			break;
		case TargetId::CameraTilt:
			// camera tilt is pitch around the local y axis
			_engineHelper->UpdateCameraTilt(delta);
			break;
		case TargetId::CameraRoll:
			// camera roll is roll around the local x axis
			_engineHelper->UpdateCameraRoll(delta);
			break;
		case TargetId::CameraDolly:
			// camera dolly is forward/back movement on the local x axis
			_engineHelper->UpdateCameraDolly(delta);
			break;
		case TargetId::CameraTruck:
			// camera truck is left/right movement on the local y axis
			_engineHelper->UpdateCameraTruck(delta);
			break;
		case TargetId::CameraPedestal:
			// camera pedestal is up/down movement on the local z axis
			_engineHelper->UpdateCameraPedestal(delta);
			break;
		case TargetId::ActorLocationX:
			// x position, cached
			rxContext.MoveDeltaCache.X += delta;
			break;
		case TargetId::ActorLocationY:
			// y position, cached
			rxContext.MoveDeltaCache.Y += delta;
			break;
		case TargetId::ActorLocationZ:
			// z position, cached
			rxContext.MoveDeltaCache.Z += delta;
			break;
		case TargetId::ActorRotationX:
			// x rotation (roll), cached
			rxContext.RotateDeltaCache.Roll += delta;
			break;
		case TargetId::ActorRotationY:
			// y rotation (pitch), cached
			rxContext.RotateDeltaCache.Pitch += delta;
			break;
		case TargetId::ActorRotationZ:
			// z rotation (yaw), cached
			rxContext.RotateDeltaCache.Yaw += delta;
			break;
		case TargetId::ActorScaleX:
			// x scale, cached
			rxContext.ScaleDeltaCache.X += delta;
			break;
		case TargetId::ActorScaleY:
			// y scale, cached
			rxContext.ScaleDeltaCache.Y += delta;
			break;
		case TargetId::ActorScaleZ:
			// z scale, cached
			rxContext.ScaleDeltaCache.Z += delta;
			break;
		case TargetId::ActorScaleAll:
			// scale all axes, cached
			rxContext.ScaleDeltaCache = rxContext.ScaleDeltaCache + delta;
			break;
		case TargetId::SequencerZoom:
			// implement using the zoom actions, ignoring the step value
			if (delta > 0)
			{
				// zoom in
				_engineHelper->SequencerAction(TargetId::SequencerZoomIn);
			}
			else
			{
				// zoom out
				_engineHelper->SequencerAction(TargetId::SequencerZoomOut);
			}
			break;
		case TargetId::LightColorX:
		case TargetId::LightColorY:
		case TargetId::LightColorZ:
			// colour adjustment from a colour wheel control
			_engineHelper->UpdateLightXYZ(paramId, delta);
			break;
		case TargetId::LightColorHue:
		case TargetId::LightColorSaturation:
		case TargetId::LightColorValue:
			// hsv based colour adjustment
			_engineHelper->UpdateLightHSV(paramId, delta);
			break;
		case TargetId::SequencerPlaybackSpeed:
			// sequencer based param, handle via the helper
			_engineHelper->UpdateSequencerParameter(paramId, delta);
			break;
		case TargetId::LightCardOpacity:
		case TargetId::LightCardFeathering:
			// property based but with target specific range clipping
			_engineHelper->UpdateParameterProperty(paramId, 0.0f, 1.0f, delta);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->UpdateParameterProperty(paramId, delta);
			break;
	}
}

// called to process a parameter reset command in the rx context, in the current implementation some resets may trigger reset of other 
// associated values where they are applied by the same api call, this may need to be changed, some resets may be cached and applied by the caller, this
// allows us to combine individual control updates for properties in unreal that are set by the same api call such as location, rotation etc.
void TangentEngine::ParseParamResetCmd(RxContext& rxContext)
{
	uint32		paramId;

	// format is just the target control id
	paramId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ParameterReset command [ id 0x%08X ]"), paramId);

	// register a reset action has occurred, the following processing for resets may refer to the reset cache to detect single or double taps and 
	// choose different reset values accordingly, it is up to the various reset functions to implement the required functionality
	// note - the scale all param is a special case as it really changes all three individual axes so we register all three instead, all other params
	// are just themselves
	if (paramId == TargetId::ActorScaleAll)
	{
		// convert the special case scale all to scale x, y and z so they can detect double taps correctly
		ResetCache::TapNow(TargetId::ActorScaleX);
		ResetCache::TapNow(TargetId::ActorScaleY);
		ResetCache::TapNow(TargetId::ActorScaleZ);
	}
	else
	{
		// all other params register as themselves
		ResetCache::TapNow(paramId);
	}

	// there are a large number of post process volume and color correct region based params which we don't want to have to support individually 
	// so these target ids are bit encoded with a master detection flag, test for these as exceptions first, if not encoded then carry on as usual
	if (TargetId::IsGradingHSV(paramId))
	{
		// grading based param reset for hsv, handle here then we are all done so return
		_engineHelper->ResetGradingHSV(paramId);
		return;
	}
	else if (TargetId::IsGradingXYZ(paramId))
	{
		// grading based param reset for xyz, handle here then we are all done so return
		_engineHelper->ResetGradingXYZ(paramId);
		return;
	}

	// handle by target id, individual ids may be handled with bespoke functions, all others are passed to the property based api
	switch (paramId)
	{
		case TargetId::LightColorX:
		case TargetId::LightColorY:
		case TargetId::LightColorZ:
			// xyz based colour reset
			_engineHelper->ResetLightXYZ(paramId);
			break;
		case TargetId::LightColorHue:
		case TargetId::LightColorSaturation:
		case TargetId::LightColorValue:
			// hsv based colour reset
			_engineHelper->ResetLightHSV(paramId);
			break;
		case TargetId::CameraPan:
		case TargetId::CameraTilt:
		case TargetId::CameraRoll:
			// these params don't really have a reset so we simply filter them out
			break;
		case TargetId::CameraDolly:
		case TargetId::CameraTruck:
		case TargetId::CameraPedestal:
			// these params don't really have a reset so we simply filter them out
			break;
		case TargetId::SequencerZoom:
			// implement using the zoom action to zoom to fit on reset
			_engineHelper->SequencerAction(TargetId::SequencerZoomToFit);
			break;
		case TargetId::ActorLocationX:
			// x position reset, cached
			rxContext.FlagMoveReset(RxContext::AxisX);
			break;
		case TargetId::ActorLocationY:
			// y position reset, cached
			rxContext.FlagMoveReset(RxContext::AxisY);
			break;
		case TargetId::ActorLocationZ:
			// z position reset, cached
			rxContext.FlagMoveReset(RxContext::AxisZ);
			break;
		case TargetId::ActorRotationX:
			// x rotation reset, cached
			rxContext.FlagRotateReset(RxContext::AxisX);
			break;
		case TargetId::ActorRotationY:
			// y rotation reset, cached
			rxContext.FlagRotateReset(RxContext::AxisY);
			break;
		case TargetId::ActorRotationZ:
			// z rotation reset, cached
			rxContext.FlagRotateReset(RxContext::AxisZ);
			break;
		case TargetId::ActorScaleX:
			// x scale reset, cached
			rxContext.FlagScaleReset(RxContext::AxisX);
			break;
		case TargetId::ActorScaleY:
			// y scale reset, cached
			rxContext.FlagScaleReset(RxContext::AxisY);
			break;
		case TargetId::ActorScaleZ:
			// z scale reset, cached
			rxContext.FlagScaleReset(RxContext::AxisZ);
			break;
		case TargetId::ActorScaleAll:
			// reset all scale values for the selected object, cached
			rxContext.FlagScaleReset(RxContext::AxisX | RxContext::AxisY | RxContext::AxisZ);
			break;
		case TargetId::SequencerPlaybackSpeed:
			// sequencer based param, handle via the helper
			_engineHelper->ResetSequencerParameter(paramId);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->ResetParameterProperty(paramId);
			break;
	}
}

// called to process a parameter value request command in the rx context, in the current implementation some requests may trigger value
// reports of other associated values where they are fetched by the same api call
void TangentEngine::ParseParamValueRequestCmd(RxContext& rxContext)
{
	uint32		paramId;

	// format is just the target control id
	paramId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ParameterValueRequest command [ id 0x%08X ]"), paramId);
	
	// there are a large number of post process volume and color correct region based params which we don't want to have to support individually 
	// so these target ids are bit encoded with a master detection flag, test for these as exceptions first, if not encoded then carry on as usual
	if (TargetId::IsGradingHSV(paramId))
	{
		// grading based param value request for hsv, handle here then we are all done so return
		_engineHelper->RequestGradingHSV(paramId);
		return;
	}
	else if (TargetId::IsGradingXYZ(paramId))
	{
		// grading based param value request for xyz, these params don't really have an associated value so we simply filter them out
		return;
	}

	// handle by target id, individual ids may be handled with bespoke functions, all others are passed to the property based api
	switch (paramId)
	{
		case TargetId::LightColorX:
		case TargetId::LightColorY:
		case TargetId::LightColorZ:
		case TargetId::CameraPan:
		case TargetId::CameraTilt:
		case TargetId::CameraRoll:
		case TargetId::CameraDolly:
		case TargetId::CameraTruck:
		case TargetId::CameraPedestal:
		case TargetId::SequencerZoom:
			// these params don't really have an associated value so we simply filter them out
			break;
		case TargetId::LightColorHue:
		case TargetId::LightColorSaturation:
		case TargetId::LightColorValue:
			// hsv based colour request
			_engineHelper->RequestLightHSV(paramId);
			break;
		case TargetId::ActorLocationX:
		case TargetId::ActorLocationY:
		case TargetId::ActorLocationZ:
			// request all location values for the selected object, cached
			rxContext.FlagValueRequest(RxContext::LocationRequest);
			break;
		case TargetId::ActorRotationX:
		case TargetId::ActorRotationY:
		case TargetId::ActorRotationZ:
			// request all rotation values for the selected object, cached
			rxContext.FlagValueRequest(RxContext::RotationRequest);
			break;
		case TargetId::ActorScaleX:
		case TargetId::ActorScaleY:
		case TargetId::ActorScaleZ:
		case TargetId::ActorScaleAll:
			// request all scale values for the selected object, cached
			rxContext.FlagValueRequest(RxContext::ScaleRequest);
			break;
		case TargetId::SequencerPlaybackSpeed:
			// sequencer based param, handle via the helper
			_engineHelper->RequestSequencerParameterValue(paramId);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->RequestParameterPropertyValue(paramId);
			break;
	}
}

// handles the fact that all custom controls are initially defined under a generic control target id, as we need to differentiate them
// for processing this function takes the control string and converts it into a unique integer target id for this session, all future
// calls with the same control string will return the same value
uint32 TangentEngine::ConvertCustomControlToTargetId(const FName& controlString)
{
	// we suggest values seeded within the reserved (i.e. Tangent defined) space where we control value usage
	static uint32 candidate = ControlId::ReservedBit | 0x0F000000;
	uint32* resultId;

	// first we look for an existing allocation in the lut
	resultId = _customControlTargetIdMap.Find(controlString);
	if (resultId)
	{
		// we have already assigned a target id, return that for use
		return *resultId;
	}

	// go with the next candidate value up, we don't need to test for existence as we control the value set, so store it and then return for use
	candidate++;
	_customControlTargetIdMap.Add(controlString, candidate);
	return candidate;
}

// handles a custom parameter change command and applies the change to unreal
void TangentEngine::ParseCustomParamChangeCmd(RxContext& rxContext)
{
	// the format is the target control id string and value delta, extract the target string as a more efficient fname instance, the delta will never be zero
	FName controlString = rxContext.ReadString();
	float delta = rxContext.ReadFloat32();

	//UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomParameterChange command [ string '%s', delta %f ]"), *controlString.ToString(), delta);

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// this is a custom param control mapping that needs us to convert the generic mapped control into something that is unique
		// for the control string attached to the mapping
		uint32	targetId = ConvertCustomControlToTargetId(controlString);

		// ask the helper to apply the change and respond with the new value
		_engineHelper->UpdateCustomParameter(targetId, controlString, delta);
	}
}

// called to process a custom parameter reset command in the rx context
void TangentEngine::ParseCustomParamResetCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();

	//UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomParameterReset command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// this is a custom param control mapping that needs us to convert the generic mapped control into something that is unique
		// for the control string attached to the mapping
		uint32	targetId = ConvertCustomControlToTargetId(controlString);

		// register a reset action has occurred, the following processing for resets may refer to the reset cache to detect single or double taps and 
		// choose different reset values accordingly, it is up to the various reset functions to implement the required functionality
		ResetCache::TapNow(targetId);

		// ask the helper to reset the param and respond with the new value
		_engineHelper->ResetCustomParameter(targetId, controlString);
	}
}

// called to process a custom parameter value request command in the rx context
void TangentEngine::ParseCustomParamValueRequestCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();
	
	//UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomParameterValueRequest command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to send back the value
		_engineHelper->RequestCustomParameterValue(controlString);
	}
}

// handles a custom menu change command and applies the change to unreal
void TangentEngine::ParseCustomMenuChangeCmd(RxContext& rxContext)
{
	// the format is the target control id string and step value, extract the target string as a more efficient fname instance, the step will always be +/-1
	FName controlString = rxContext.ReadString();
	int32 step = rxContext.ReadInt32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomMenuChange command [ string '%s', step %d ]"), *controlString.ToString(), step);

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to apply the change and respond with the new value
		_engineHelper->StepCustomMenu(controlString, step);
	}
}

// handles a custom menu reset command and applies the change to unreal
void TangentEngine::ParseCustomMenuResetCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomMenuReset command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to reset the menu and respond with the new value
		_engineHelper->ResetCustomMenu(controlString);
	}
}

// called to process a custom menu string request command in the rx context
void TangentEngine::ParseCustomMenuStringRequestCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ParseCustomMenuStringRequestCmd command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to send back the value
		_engineHelper->RequestCustomMenuString(controlString);
	}
}

// handles a custom action on command and sends the change to unreal
void TangentEngine::ParseCustomActionOnCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomActionOn command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to process the action, specifying a press event
		_engineHelper->OnCustomAction(controlString, true);
	}
}

// handles a custom action off command and sends the change to unreal
void TangentEngine::ParseCustomActionOffCmd(RxContext& rxContext)
{
	// the format is just the target control id string, extract the target string as a more efficient fname instance
	FName controlString = rxContext.ReadString();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx CustomActionOff command [ string '%s' ]"), *controlString.ToString());

	// guard against empty control strings, they are no use to us as the string defines the target value
	if (!controlString.IsNone())
	{
		// ask the helper to process the action, specifying a release event
		_engineHelper->OnCustomAction(controlString, false);
	}
}

// called by the editor helper when a parameter value is available to pass on to the hub
void TangentEngine::OnParameterValue(uint32 targetId, float value)
{
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: parameter id 0x%08X has new value %f"), targetId, value);

	// pass the notification on to the hub with a parameter value command, the format is the target id, the value itself and an
	// at-default flag that we don't currently use in this implementation
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::ParameterValue);
	msg.WriteUInt32(targetId);
	msg.WriteFloat32(value);
	msg.WriteUInt32(0);
	msg.Build();
	SendToHub(msg);
}

// called by the editor helper when a custom parameter value is available to pass on to the hub
void TangentEngine::OnCustomParameterValue(FName controlString, float value)
{
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: custom parameter '%s' has new value %f"), *controlString.ToString(), value);

	// pass the notification on to the hub with a custom parameter value command, the format is the target control string, the value itself and an
	// at-default flag that we don't currently use in this implementation
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::CustomParameterValue);
	msg.WriteString(controlString);
	msg.WriteFloat32(value);
	msg.WriteUInt32(0);
	msg.Build();
	SendToHub(msg);
}

// called by the editor helper when a custom menu string is available to pass on to the hub
void TangentEngine::OnCustomMenuString(FName controlString, FString& string)
{
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: custom menu '%s' has new string '%s'"), *controlString.ToString(), *string);

	// pass the notification on to the hub with a custom menu string command, the format is the target control string, the string itself and an
	// at-default flag that we don't currently use in this implementation
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::CustomMenuString);
	msg.WriteString(controlString);
	msg.WriteString(string);
	msg.WriteUInt32(0);
	msg.Build();
	SendToHub(msg);
}

// called by the editor helper when a control has been flagged as training and a property in the unreal editor has been changed, this call
// ends the training session and will associate the control with a custom control mapping for the target property as defined by the configuration
void TangentEngine::OnTrainResponse(int32 targetType, FName name, FName controlString, float min, float max, float stepSize)
{
	// the format is the command and target mapping type, followed by the displayed label name and custom control string, then 
	// the min/max/step values which are only applicable to parameter data types but must be sent in all commands
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::TrainResponse);
	msg.WriteInt32(targetType);
	msg.WriteString(name);
	msg.WriteString(controlString);
	msg.WriteFloat32(min);
	msg.WriteFloat32(max);
	msg.WriteFloat32(stepSize);
	msg.Build();
	SendToHub(msg);
}

// helper function that clips the supplied string to the given required maximum length, making a string that shows as much of the text from
// the start whilst including the last few characters, this allows for the fact that unreal tends to use common names for objects with a numerical
// tag at the end, this will hopefully make the panel displays less confusing when using multiple select actor actions for similarly named objects, if
// the supplied text is shorter than the required lenth the original is returned unchanged
#define CLIP_TAIL_LEN	3
FString TangentEngine::ClipString(FString& original, int32 requiredLen)
{
	// if the original string is short enough we don't need to do anything
	if (original.Len() <= requiredLen)
	{
		// nothing to do, return the original
		return original;
	}

	// we format the clipping by taking as much of the header of the original text as possible and adding a tail of the original with a character
	// preceding  it to indicate it has been clipped
	FString clipped = original.Left(requiredLen - CLIP_TAIL_LEN);
	clipped += "~" + original.Right(CLIP_TAIL_LEN - 1);
	
	// return the clipped result
	return clipped;	
}

// associates any currently selected actor/component in unreal with the specified select action id and also that the current mode should always be active whenever
// that object's class is selected again in the future, setting the panel display with the actor name, if there is no selection we remove any existing association
void TangentEngine::AssociateSelectedActorAndMode(uint32 actionId)
{
	// first get the currently selected actor by full name (actor and component if available), if there is no selection the name remains empty
	FString selectedName = _engineHelper->GetSelectedName();

	// if the name is empty then we remove any stored info in the look-up map for this action
	if (selectedName.IsEmpty())
	{
		// remove any existing associated actor name from the action map, note in the current implementation we leave the class to mode link in place
		_selectActionMap.Remove(actionId);
	}
	else
	{
		// we have a valid name, store the name of the object under this action for subsequent selection when the action is fired
		_selectActionMap.Add(actionId, selectedName);

		// update the class of this object to associate to the name of the current mode
		_classToModeMap.Add(_engineHelper->GetSelectedClassName(), ConvertMode(_currentMode));
	}

	// to complete the process we send a rename control command to the hub with the actor or component clipped name to be displayed on the panels
	// with buttons mapped to this action, sending an empty name resets the display back to the usual control label
	_engineHelper->GetSelectedName(selectedName, false);

	// as these are displayed on the panels we clip to a length that is most useful for most panels
	selectedName = ClipString(selectedName);
	SendRenameControl(actionId, selectedName);

	// finally we make sure the data persists by saving immediately, we can't depend on the shutdown status to do this as users can kill ue
	SaveSelectActionMap();
	SaveClassToModeMap();
}

// selects any actor that is currently associated with the specified select action id, the call has no effect if there is no data
void TangentEngine::SelectAssociatedActor(uint32 actionId)
{
	// fetch any actor name currently associated with the action
	FString* name = _selectActionMap.Find(actionId);

	// if we have a name we select it
	if (name)
	{
		// have name, will select
		_engineHelper->SelectObjectByName(*name);
	}
}

// called for actions that support a long-press functionality, this function is called after a delay when an action
// control is pressed and held down, a release within this time period will prevent the function being called, note it
// is likely to be called from the game thread so take account of this
void TangentEngine::OnActionTimer(uint32 actionId)
{
	// simply put, if the gate flag says a save activity is running we block processing including through this timer delegate, this guards against trying
	// to change things while the editor is (auto) saving the objects in the world, if we try to access the data during a save we will crash
	if (_engineHelper->IsSaveWorldActive())
	{
		// ignore this call as a save is being processed
		return;
	}

	// handle by target id
	switch (actionId)
	{
		case TargetId::SelectActor1:
		case TargetId::SelectActor2:
		case TargetId::SelectActor3:
		case TargetId::SelectActor4:
		case TargetId::SelectActor5:
		case TargetId::SelectActor6:
		case TargetId::SelectActor7:
		case TargetId::SelectActor8:
		case TargetId::SelectActor9:
		case TargetId::SelectActor10:
		case TargetId::SelectActor11:
		case TargetId::SelectActor12:
		case TargetId::SelectActor13:
		case TargetId::SelectActor14:
		case TargetId::SelectActor15:
		case TargetId::SelectActor16:
		case TargetId::SelectActor17:
		case TargetId::SelectActor18:
		case TargetId::SelectActor19:
		case TargetId::SelectActor20:
		case TargetId::SelectActor21:
		case TargetId::SelectActor22:
		case TargetId::SelectActor23:
		case TargetId::SelectActor24:
			// for the select actions, a timer based long hold means we associate this action id with any currently selected actor and the current mode
			AssociateSelectedActorAndMode(actionId);
			break;
		default:
			// log a warning
			UE_LOG(LogTemp, Warning, TEXT("TangentEngine: unhandled action timer"));
			break;
	}
}

// helper function to try and get a valid shared pointer to the tangent input device defined by a sibling module, on the first call it will try and get the
// module if it is available and store a pointer to the device instance, if successful subsequent calls will use this instance, the function returns true
// if the device can be used
bool TangentEngine::IsTangentInputDeviceAvailable()
{
	// first check if we have already prepared the shared pointer, i.e. it is pointing to something
	if (!_tangentInputDevice.IsValid())
	{
		// we haven't got a valid shared pointer yet, this may be the first call or the module hasn't been available yet, try now
		if (FTangentInputPlugin::IsAvailable())
		{
			// get access to the (now) available input device module
			FTangentInputPlugin& mod = FTangentInputPlugin::Get();
			// use the module function to fetch the input device instance itself as a sahred pointer
			_tangentInputDevice = mod.GetTangentInputDevice();
		}
	}

	// check the current validity of the shared pointer, returning its result as our own
	return _tangentInputDevice.IsValid();
}

// called to process an action on command in the rx context, some actions will fire on this press event, some on the subsequent
// action off event, a timer may be used to perform the action after a preset delay
void TangentEngine::ParseActionOnCmd(RxContext& rxContext)
{
	uint32		actionId;

	// format is just the target control id
	actionId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ActionOn command [ id 0x%08X ]"), actionId);

	// register this action on for filtering purposes
	UpdateActionFilterOn();

	// handle by target id
	switch (actionId)
	{
		case TargetId::AddDirectionalLight:
			// adding an actor may trigger a mode change
			_engineHelper->SpawnActor(ADirectionalLight::StaticClass());
			break;
		case TargetId::AddPointLight:
			// add point light
			_engineHelper->SpawnActor(APointLight::StaticClass());
			break;
		case TargetId::AddSpotLight:
			// add spot light
			_engineHelper->SpawnActor(ASpotLight::StaticClass());
			break;
		case TargetId::AddRectLight:
			// add rect light
			_engineHelper->SpawnActor(ARectLight::StaticClass());
			break;
		case TargetId::AddSkyLight:
			// add sky light
			_engineHelper->SpawnActor(ASkyLight::StaticClass());
			break;
		case TargetId::SelectActor1:
		case TargetId::SelectActor2:
		case TargetId::SelectActor3:
		case TargetId::SelectActor4:
		case TargetId::SelectActor5:
		case TargetId::SelectActor6:
		case TargetId::SelectActor7:
		case TargetId::SelectActor8:
		case TargetId::SelectActor9:
		case TargetId::SelectActor10:
		case TargetId::SelectActor11:
		case TargetId::SelectActor12:
		case TargetId::SelectActor13:
		case TargetId::SelectActor14:
		case TargetId::SelectActor15:
		case TargetId::SelectActor16:
		case TargetId::SelectActor17:
		case TargetId::SelectActor18:
		case TargetId::SelectActor19:
		case TargetId::SelectActor20:
		case TargetId::SelectActor21:
		case TargetId::SelectActor22:
		case TargetId::SelectActor23:
		case TargetId::SelectActor24:
			// for the select actor actions we kick off a single timer to detect if we want to store the current selection for this 
			// action id, the delegate is configured with the action id
			_actionTimerDelegate.BindRaw(this, &TangentEngine::OnActionTimer, actionId);
			_engineHelper->SetActionTimer(_actionTimerDelegate, 1);
			break;
		case TargetId::ClearSelectActors:
			// resets all the select actor actions to empty via the reset function and requesting the storage file is also removed, any legacy select mode data is also removed (no longer used)
			ResetSelectActionMap(true);
			RemoveLegacySelectModeMap();
			break;
		case TargetId::ClearSelectModes:
			// resets the class to mode map data 
			ResetClassToModeMap();
			break;
		case TargetId::DeselectAll:
			// ask the editor to remove any existing selection
			_engineHelper->DeselectAll();
			break;
		case TargetId::SequencerPlayForward:
		case TargetId::SequencerPlayReverse:
		case TargetId::SequencerStop:
		case TargetId::SequencerStepForward:
		case TargetId::SequencerStepBackward:
		case TargetId::SequencerJumpToStart:
		case TargetId::SequencerJumpToEnd:
		case TargetId::SequencerShuttleForward:
		case TargetId::SequencerShuttleBackward:
		case TargetId::SequencerSetStartPlayback:
		case TargetId::SequencerSetEndPlayback:
		case TargetId::SequencerZoomIn:
		case TargetId::SequencerZoomOut:
		case TargetId::SequencerZoomToFit:
			// perform action in the current sequencer, if available
			_engineHelper->SequencerAction(actionId);
			break;
		case TargetId::StartPIESession:
		case TargetId::StartSIESession:
		case TargetId::ToggleSession:
		case TargetId::StopSession:
			// perform action for play sessions
			_engineHelper->PlaySessionControl(actionId);
			break;
		case TargetId::Undo:
		case TargetId::Redo:
			// pass on to the helper to process the transaction history action
			_engineHelper->TransactionHistoryAction(actionId);
			break;
		case TargetId::ClearResetCache:
			// trigger the release of all the reet data in the singleton cache instance
			ResetCache::Clear();
			break;
		default:
			// log a warning
			UE_LOG(LogTemp, Warning, TEXT("TangentEngine: unhandled action"));
			break;
	}
}

// called to process an action off command in the rx context, if any action timer is involved for the target action it will be removed
// not all actions requires an off event so not all control ids are referred to in this function
void TangentEngine::ParseActionOffCmd(RxContext& rxContext)
{
	uint32		actionId;

	// format is just the target control id
	actionId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ActionOff command [ id 0x%08X ]"), actionId);

	// if a mode change has come in between action on and action off events we ignore the off as the mapping is likely to  have changed
	// on the button control, check for this now, if the call returns false then we just return now
	if (!CheckActionFilterOff())
	{ 
		// this action off can be ignored
		return;
	}

	// handle by target id
	switch (actionId)
	{
		case TargetId::SelectActor1:
		case TargetId::SelectActor2:
		case TargetId::SelectActor3:
		case TargetId::SelectActor4:
		case TargetId::SelectActor5:
		case TargetId::SelectActor6:
		case TargetId::SelectActor7:
		case TargetId::SelectActor8:
		case TargetId::SelectActor9:
		case TargetId::SelectActor10:
		case TargetId::SelectActor11:
		case TargetId::SelectActor12:
		case TargetId::SelectActor13:
		case TargetId::SelectActor14:
		case TargetId::SelectActor15:
		case TargetId::SelectActor16:
		case TargetId::SelectActor17:
		case TargetId::SelectActor18:
		case TargetId::SelectActor19:
		case TargetId::SelectActor20:
		case TargetId::SelectActor21:
		case TargetId::SelectActor22:
		case TargetId::SelectActor23:
		case TargetId::SelectActor24:
			// if a select action is released before the long press timer fires we clear it and do the select action process instead
			_engineHelper->ClearActionTimer();
			SelectAssociatedActor(actionId);
			break;
		default:
			// n/a
			break;
	}
}

// called to process a menu change command in the rx context
void TangentEngine::ParseMenuChangeCmd(RxContext& rxContext)
{
	uint32		menuId;
	int32		step;

	// format is the target control id followed by a step count, in the current implementation the step is always +/-1, multiple
	// steps are done by consecutive single step commands
	menuId = rxContext.ReadUInt32();
	step = rxContext.ReadInt32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx MenuChange command [ id 0x%08X, step %d ]"), menuId, step);

	// handle by target id, individual ids may be handled with bespoke functions, all others are passed to the property based api
	switch (menuId)
	{
		case TargetId::TransformReference:
			// change the setting in the helper
			_engineHelper->StepTransformReference(step);
			break;
		case TargetId::SelectionChangesMode:
			// change the setting in the helper
			_engineHelper->StepSelectionChangesMode(step);
			// if we have just turned the setting on we update now to catch any possible mode change for the current selection
			if (_engineHelper->GetSelectionChangesMode())
			{
				// setting has been enabled, make a call to the function that is triggered when a selection change is notified to
				// go through the mode change processing logic to check for a possible mode change
// TODO - this masks the report of the updated setting value, leave out for now but revisit.
				//OnSelectionChanged();
			}
			// this setting is saved in a persistent file, save now to track changes
			SaveSettings();
			break;
		case TargetId::CineCameraLensSettings:
			// change the setting in the helper
			_engineHelper->StepCineCameraLensSettingsPreset(step);
			break;
		case TargetId::UseAllColorGrading:
		case TargetId::CPPUseAllColorGrading:
			// change the setting in the helper
			_engineHelper->StepUseAllColorGrading(menuId, step);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->StepMenuProperty(menuId, step);
			break;
	}
}

// called to process a menu reset command in the rx context
void TangentEngine::ParseMenuResetCmd(RxContext& rxContext)
{
	uint32		menuId;

	// format is just the target control id
	menuId = rxContext.ReadUInt32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx MenuReset command [ id 0x%08X ]"), menuId);

	// handle by target id, individual ids may be handled with bespoke functions, all others are passed to the property based api
	switch (menuId)
	{
		case TargetId::TransformReference:
			// reset the setting in the helper
			_engineHelper->ResetTransformReference();
			break;
		case TargetId::SelectionChangesMode:
			// reset the setting in the helper
			_engineHelper->ResetSelectionChangesMode();
			// this setting is saved in a persistent file, save now to track changes
			SaveSettings();
			break;
		case TargetId::CineCameraLensSettings:
			// reset the setting in the helper
			_engineHelper->ResetCineCameraLensSettingsPreset();
			break;
		case TargetId::UseAllColorGrading:
		case TargetId::CPPUseAllColorGrading:
			// reset the setting in the helper
			_engineHelper->ResetUseAllColorGrading(menuId);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->ResetMenuProperty(menuId);
			break;
	}
}

// called to process a menu string request command in the rx context
void TangentEngine::ParseMenuStringRequestCmd(RxContext& rxContext)
{
	uint32		menuId;

	// format is just the target control id
	menuId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx MenuStringRequest command [ id 0x%08X ]"), menuId);

	// handle by target id, individual ids may be handled with bespoke functions, all others are passed to the property based api
	switch (menuId)
	{
		case TargetId::TransformReference:
			// request the setting from the helper
			_engineHelper->RequestTransformReference();
			break;
		case TargetId::SelectionChangesMode:
			// request the setting from the helper
			_engineHelper->RequestSelectionChangesMode();
			break;
		case TargetId::CineCameraLensSettings:
			// request the setting from the helper
			_engineHelper->RequestCineCameraLensSettingsPreset();
			break;
		case TargetId::UseAllColorGrading:
		case TargetId::CPPUseAllColorGrading:
			// request the setting from the helper
			_engineHelper->RequestUseAllColorGrading(menuId);
			break;
		default:
			// generic property based handling, all these ids must be registered with the engine helper class
			_engineHelper->RequestMenuPropertyString(menuId);
			break;
	}
}

// called to process a mode change command in the rx context
void TangentEngine::ParseModeChangeCmd(RxContext& rxContext)
{
	uint32		modeId;

	// format is just the target mode id
	modeId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx ModeChange command [ id 0x%08X ]"), modeId);

	// in this implementation we always honour the request to change mode, setting the mode updates the local store
	// and responds with a confirmation message of the new mode id to the hub
	SetCurrentMode(modeId);
}

// called to process a train control command in the rx context that sets a custom mapping using data extracted from the ui
void TangentEngine::ParseTrainControlCmd(RxContext& rxContext)
{
	uint32		controlId;

	// format is just the target control id
	controlId = rxContext.ReadUInt32();
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx TrainControl command [ id 0x%08X ]"), controlId);

	// tell the helper to watch for a property change to use to train with this control, it will respond with the data when available
	_engineHelper->StartTrainingForPanelControl(controlId);
}

// called to process a transport command in the rx context, in this implementation transport only applies to the active sequencer
void TangentEngine::ParseTransportCmd(RxContext& rxContext)
{
	int32	jog,
			shuttle;

	// format is jog value...
	jog = rxContext.ReadInt32();

	// ...then shuttle value, only one of which should be non-zero
	shuttle = rxContext.ReadInt32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx Transport command [ jog %d, shuttle %d ]"), jog, shuttle);

	// go through the various options to find the transport type and direction, we ignore the actual step count in this implementation
	if (jog > 0)
	{
		// jog forward with a step
		_engineHelper->SequencerAction(TargetId::SequencerStepForward);
	}
	else if (jog < 0)
	{
		// jog back with a step
		_engineHelper->SequencerAction(TargetId::SequencerStepBackward);
	}
	else if (shuttle > 0)
	{
		// shuttle forward a click
		_engineHelper->SequencerAction(TargetId::SequencerShuttleForward);
	}
	else if (shuttle < 0)
	{
		// shuttle back a click
		_engineHelper->SequencerAction(TargetId::SequencerShuttleBackward);
	}
}

// called to process a mode list command in the rx context, stores a current list of defined mode names and their ids
void TangentEngine::ParseModeListCmd(RxContext& rxContext)
{
	uint32		modeCount,
				modeId;
	FName		modeName;

	// first is the number of modes defined in the command
	modeCount = rxContext.ReadUInt32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx Mode List command [ %d mode(s) ]"), modeCount);

	// these are stored locally, clear existing data
	_modeList.Empty();

	// loop through the data
	for (uint32 i = 0; i < modeCount; i++)
	{
		// each mode definition provides an id and name
		modeId = rxContext.ReadUInt32();
		modeName = rxContext.ReadString();

		// store indexed by name
		_modeList.Add(modeName, modeId);
	}
}

// called to process a raw input command in the rx context, drives UE axis and action bindings from panel control
void TangentEngine::ParseRawInputCmd(RxContext& rxContext)
{
	uint32		controlType,
				controlNumber;
	int32		intValue;
	float		floatValue;

	// read the control type, control number, int value and float value - only one of the values should be used
	controlType = rxContext.ReadUInt32();
	controlNumber = rxContext.ReadUInt32();
	intValue = rxContext.ReadInt32();
	floatValue = rxContext.ReadFloat32();

//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: rx Raw Input command [ type %d, number %d, intVal %d, floatVale %f ]"), controlType, controlNumber, intValue, floatValue);

	// processing this command depends on the availability of the input device instance from the associated sibling module, check if it
	// is in place, if the function returns true then we continue
	if (IsTangentInputDeviceAvailable())
	{
		// the control type tells us which value to use and how to process it
		switch (controlType)
		{
			case ControlType::Encoder:
				// an encoder delta to pass on to the input device
				_tangentInputDevice->OnEncoderChange(controlNumber, floatValue);			
				break;
			case ControlType::Button:
				// a button state change to pass on to the input device
				_tangentInputDevice->OnButtonChange(controlNumber, (bool) intValue);
				break;
			default:
				break;
		}
	}
}

// called by the editor helper when a menu string is available to pass on to the hub
void TangentEngine::OnMenuString(uint32 targetId, FString &string)
{
//	UE_LOG(LogTemp, Log, TEXT("TangentEngine: menu id 0x%08X has new string '%s'"), targetId, *string);

	// pass the notification on to the hub with a menu string command, the format is the target id, the string itself and an
	// at-default flag that we don't currently use in this implementation
	MessageBuilder msg;
	msg.WriteUInt32(AppCommand::MenuString);
	msg.WriteUInt32(targetId);
	msg.WriteString(string);
	msg.WriteUInt32(0);
	msg.Build();
	SendToHub(msg);
}

// called with a buffer with a message payload that has been received from the hub to parse and action 
void TangentEngine::ParseReceived(RxContext &rxContext)
{
	uint32		cmd;

	// simply put, if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
	// (auto) saving the objects in the world, if we try to access the data during a save we will crash
	if (_engineHelper->IsSaveWorldActive())
	{
		// ignore this received packet as a save is being processed
		return;
	}

	// some parameters are applied in unreal using a common api call, combine all such instances in this parse call
	// by caching the deltas at this level then applying them at the same time in a single call after we are done, this
	// catches controls such as trackerball axes where multiple changes may be received at the same time, make sure
	// each received data set starts with empty caches
	rxContext.ResetCaches();

	// a single message may contain multiple commands
	while (rxContext.HasCommandToRead())
	{
		// first we need the command
		cmd = rxContext.ReadUInt32();

		// handle each command as appropriate
		switch (cmd)
		{
			case HubCommand::InitiateComms:
				// handshake command sent by the hub, triggers an app definition response
				ParseInitCommsCmd(rxContext);
				break;
			case HubCommand::ParameterChange:
				// parameter change by an encoder
				ParseParamChangeCmd(rxContext);
				break;
			case HubCommand::ParameterReset:
				// parameter reset
				ParseParamResetCmd(rxContext);
				break;
			case HubCommand::ParameterValueRequest:
				// parameter value request
				ParseParamValueRequestCmd(rxContext);
				break;
			case HubCommand::ActionOn:
				// action on button press
				ParseActionOnCmd(rxContext);
				break;
			case HubCommand::ActionOff:
				// action off button release
				ParseActionOffCmd(rxContext);
				break;
			case HubCommand::MenuChange:
				// menu changed by a panel control
				ParseMenuChangeCmd(rxContext);
				break;
			case HubCommand::MenuReset:
				// menu reset
				ParseMenuResetCmd(rxContext);
				break;
			case HubCommand::MenuStringRequest:
				// menu string request
				ParseMenuStringRequestCmd(rxContext);
				break;
			case HubCommand::CustomParameterChange:
				// custom parameter change by an encoder
				ParseCustomParamChangeCmd(rxContext);
				break;
			case HubCommand::CustomParameterReset:
				// custom parameter reset
				ParseCustomParamResetCmd(rxContext);
				break;
			case HubCommand::CustomParameterValueRequest:
				// custom parameter value request
				ParseCustomParamValueRequestCmd(rxContext);
				break;
			case HubCommand::CustomMenuChange:
				// custom menu change
				ParseCustomMenuChangeCmd(rxContext);
				break;
			case HubCommand::CustomMenuReset:
				// custom menu reset
				ParseCustomMenuResetCmd(rxContext);
				break;
			case HubCommand::CustomMenuStringRequest:
				// custom menu string request
				ParseCustomMenuStringRequestCmd(rxContext);
				break;
			case HubCommand::CustomActionOn:
				// custom action on button press
				ParseCustomActionOnCmd(rxContext);
				break;
			case HubCommand::CustomActionOff:
				// custom action off button release
				ParseCustomActionOffCmd(rxContext);
				break;
			case HubCommand::ModeChange:
				// mode change request
				ParseModeChangeCmd(rxContext);
				break;
			case HubCommand::TrainControl:
				// request to train the specified control from the ui
				ParseTrainControlCmd(rxContext);
				break;
			case HubCommand::Transport:
				// transport change
				ParseTransportCmd(rxContext);
				break;
			case HubCommand::RawInput:
				// raw control input
				ParseRawInputCmd(rxContext);
				break;
			case HubCommand::ModeList:
				// mode definitions
				ParseModeListCmd(rxContext);
				break;
			default:
				UE_LOG(LogTemp, Warning, TEXT("TangentEngine: unhandled command %d, comms will be out of sync"), cmd);
				// we need to support all expected commands so this should not happen
				break;
		}
	}

	// apply any accumulated cached deltas here, actor position first
	if (!rxContext.MoveDeltaCache.IsZero())
	{
		// at least one parameter change command has moved the selected object, apply now
		_engineHelper->MoveSelected(rxContext.MoveDeltaCache);
	}

	// a similar thing is done for actor position resets
	if (rxContext.MoveResetCache != 0)
	{
		// at least one position axis has asked to reset, apply now passing in the axis flags
		_engineHelper->ResetMoveSelected(rxContext.MoveResetCache);
	}

	// deal with any pending location value requests
	if (rxContext.IsValueRequested(RxContext::LocationRequest))
	{
		// the hub has requested one or more of the location values to be sent back
		_engineHelper->RequestSelectedLocation();
	}

	// actor rotation
	if (!rxContext.RotateDeltaCache.IsZero())
	{
		// at least one parameter change command has rotated the selected object, apply now
		_engineHelper->RotateSelected(rxContext.RotateDeltaCache);
	}

	// actor rotation resets
	if (rxContext.RotateResetCache != 0)
	{
		// at least one rotation axis has asked to reset, apply now passing in the axis flags
		_engineHelper->ResetRotateSelected(rxContext.RotateResetCache);
	}

	// rotation value requests
	if (rxContext.IsValueRequested(RxContext::RotationRequest))
	{
		// the hub has requested one or more of the rotation values to be sent back
		_engineHelper->RequestSelectedRotation();
	}

	// actor scale
	if (!rxContext.ScaleDeltaCache.IsZero())
	{
		// at least one parameter change command has scaled the selected object, apply now
		_engineHelper->ScaleSelected(rxContext.ScaleDeltaCache);
	}

	// actor scale resets
	if (rxContext.ScaleResetCache != 0)
	{
		// at least one scale axis has asked to reset, apply now passing in the axis flags
		_engineHelper->ResetScaleSelected(rxContext.ScaleResetCache);
	}

	// scale value requests
	if (rxContext.IsValueRequested(RxContext::ScaleRequest))
	{
		// the hub has requested one or more of the scale values to be sent back
		_engineHelper->RequestSelectedScale();
	}
}

// called when data is available to be read from the hub comms
bool TangentEngine::ProcessRead(RxContext &rxContext)
{
	bool result = false;

	// we should always be expecting data when this is called, anything else is a non-recoverable error
	if (rxContext.WaitingForData())
	{
		uint8	*buffer;
		int32	bytesExpected,
				bytesRead = 0;

		// fetch the context's buffer info for where we are to receive data and how many bytes are remaining to complete the process
		rxContext.GetReceiveBuffer(buffer, bytesExpected);

		// read as much data as we are expecting, inserting it to the appropriate position in the rx buffer, this allows any amount of data to
		// be read at any time until a full message is received
		result = _socket->Recv(buffer, bytesExpected, bytesRead, ESocketReceiveFlags::None);
		if (result)
		{
			// the receive call success does not guarantee data actually arrived so we need to check this explicitly
			if (bytesRead > 0)
			{
				// update the context by the amount we just got
				rxContext.AdjustForDataReceived(bytesRead);

				// if we are no longer waiting for more data now then we have completed either a header or a payload
				if (!rxContext.WaitingForData())
				{
					// if the context is in the header state then we are receiving the byte count of the actual payload, if not then we are receiving the payload itself
					if (rxContext.ProcessingHeader())
					{
						uint32 messageLen;

						// this is the header that tells us how many bytes are to follow, extract the value
						messageLen = rxContext.ReadUInt32();

						// sanity check the payload size
						if (messageLen == 0)
						{
							// a zero length message is technically valid, accept this, reset for another header as no more data is expected
							rxContext.ResetForHeader();
						}
						else if (messageLen > IPCComms::MaxMessageLen)
						{
							UE_LOG(LogTemp, Warning, TEXT("TangentEngine: message length of %d bytes is longer than the maximum of %d"), messageLen, IPCComms::MaxMessageLen);
							// the payload is larger than the maximum allowed, we cannot accept it, we must reset the connection by returning false
							result = false;
						}
						else
						{
							// reset the comms ready to receive the message payload of the specified size
							rxContext.ResetForMessage(messageLen);
						}
					}
					else
					{
						// this is a completed message, parse as appropriate
						ParseReceived(rxContext);

						// all done with this message so reset for the next header
						rxContext.ResetForHeader();
					}
				}
			}
		}
	}

	// return our processing result, false means the connection will be closed and a new connection is required
	return result;
}

// called to set up the instance
bool TangentEngine::Init()
{
	UE_LOG(LogTemp, Log, TEXT("TangentEngine: Init()"));
	// nothing to set up, returning true means the runnable will continue to the run phase
	return true;
}

// main worker function
uint32 TangentEngine::Run()
{
	RxContext rxContext;
	FTimespan timeout(0, 0, 1);

	UE_LOG(LogTemp, Log, TEXT("TangentEngine: Run()"));

	// the outer loop maintains a connection with the hub
	while (!_threadToExit)
	{
		// kick off the inital attempt to connect to the hub
		ConnectHubComms();

		// if we were successful in the connection we will enter the second loop
		if (_socket && (_socket->GetConnectionState() == SCS_Connected))
		{
			// comms are up and running, we initially require a message header to come in 
			rxContext.ResetForHeader();

			// the inner loop continues processing comms on an open connection until we are flagged to exit by the plugin
			// or there is a comms error that requires a connection reset
			while (!_threadToExit)
			{
				// wait for data to be received with a timeout to allow testing of our local exit flag
				if (_socket->Wait(ESocketWaitConditions::WaitForRead, timeout))
				{
					// data is available, process the read, if this function returns false there is an error and we
					// must reset the comms to reconnect
					if (!ProcessRead(rxContext))
					{
						UE_LOG(LogTemp, Warning, TEXT("TangentEngine: socket closed or error"));
						// socket closed or unrecoverable error, we release the connection resources and then pause before a retry
						ResetHubComms();
						FPlatformProcess::Sleep(1);
						// the break takes us out of the inner loop
						break;
					}
				}
			}
		}
		else
		{
			// the connection attempt had failed, pause before trying to connect again, the sleep prevents the thread from
			// spinning out of control
			FPlatformProcess::Sleep(1);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("TangentEngine: thread loop finished"));
	// all done, exit the thread to allow it to shutdown
	return 0;
}

// called to terminate the thread early
void TangentEngine::Stop()
{
	UE_LOG(LogTemp, Log, TEXT("TangentEngine: Stop()"));
	// to stop the thread we safely set the flag, the caller should then wait for the thread to complete
	_threadToExit.AtomicSet(true);
}

// called after the run worker function has finished to clean up
void TangentEngine::Exit()
{
	UE_LOG(LogTemp, Log, TEXT("TangentEngine: Exit()"));
	// on exit we make sure to clean up the comms resources
	ResetHubComms();
}
