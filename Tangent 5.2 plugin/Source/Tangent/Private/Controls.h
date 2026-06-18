// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 388 $

#pragma once

// named modes we may use for selected actor types
namespace ModeId
{
	enum
	{
		// special case no mode change value
		NoChange = 0,

		Editor = 0x00010001,
		Light = 0x00010002,
		Camera = 0x00010003,
		PostProcess = 0x00010004,
		Sequencer = 0x00010005,
		ColorCorrect = 0x00010006,

		// preselected default
		Default = Editor
	};
}

// defined panel types
namespace PanelType
{
	enum
	{
		// special case value, unsigned byte maximum as this value is bit packed in control ids
		Undefined = UINT8_MAX,

		// supported panel types with their specific value assignments
		CP100 = 0x00,
		CP200B = 0x01,
		CP200T = 0x02,
		CP200BK = 0x03,
		CP200K = 0x04,
		CP200TS = 0x05,
		Reserved1 = 0x06,
		Reserved2 = 0x07,
		Reserved3 = 0x08,
		CP200S = 0x09,
		Wave = 0x0A,
		vWaveLite = 0x0B,
		ElementTk = 0x0C,
		ElementMf = 0x0D,
		ElementKb = 0x0E,
		ElementBt = 0x0F,
		ElementCb = 0x10,
		Ripple = 0x11,
		ArcFunction = 0x12,
		ArcGrading = 0x13,
		ArcNavigation = 0x14,

		// special case top value marker, used for range processing
		Count
	};
}

// defined control types
namespace ControlType
{
	enum
	{
		// special case value, unsigned byte maximum as this value is bit packed in control ids
		Undefined = UINT8_MAX,

		// control types
		Encoder = 0,
		Button,

		// special case top value marker, used for range processing
		Count
	};
}

// macros used to pack data into panel control ids
#define CONTROL_ID_PANEL_TYPE_SHIFT         24
#define CONTROL_ID_CONTROL_TYPE_SHIFT       16
#define CONTROL_ID_FIELD_MASK               0xFF

// returns the individual panel type and control type from the given panel control id
#define UNPACK_CONTROL_ID_PANEL_TYPE(_cid_)     ((_cid_ >> CONTROL_ID_PANEL_TYPE_SHIFT) & CONTROL_ID_FIELD_MASK)
#define UNPACK_CONTROL_ID_CONTROL_TYPE(_cid_)   ((_cid_ >> CONTROL_ID_CONTROL_TYPE_SHIFT) & CONTROL_ID_FIELD_MASK)

// support forF control ids
namespace ControlId
{
	// returns the panel type of the specified id
	inline uint32 panelTypeOf(uint32 controlId) { return UNPACK_CONTROL_ID_PANEL_TYPE(controlId); }

	// returns the control type of the specified id
	inline uint32 controlTypeOf(uint32 controlId) { return UNPACK_CONTROL_ID_CONTROL_TYPE(controlId); }

	enum
	{
		// special case value
		Undefined = UINT32_MAX,

		// reserved bit space for tangent defined ids
		ReservedBit = (1 << 31)
	};
}

// defines the specific type of a data object used for training controls with mappings
namespace MappingDataType
{
	enum
	{
		Action = 4,
		Parameter = 5,
		Menu = 6
	};
}

// definitions for plugin mappable commands
namespace TargetId
{
	enum
	{
		// actions
		SelectActor1 = 0x00020001,
		SelectActor2 = 0x00020002,
		SelectActor3 = 0x00020003,
		SelectActor4 = 0x00020004,
		SelectActor5 = 0x00020005,
		SelectActor6 = 0x00020006,
		SelectActor7 = 0x0002001B,
		SelectActor8 = 0x0002001C,
		SelectActor9 = 0x0002001D,
		SelectActor10 = 0x0002001E,
		SelectActor11 = 0x0002001F,
		SelectActor12 = 0x00020020,
		SelectActor13 = 0x00020021,
		SelectActor14 = 0x00020022,
		SelectActor15 = 0x00020023,
		SelectActor16 = 0x00020024,
		SelectActor17 = 0x00020025,
		SelectActor18 = 0x00020026,
		SelectActor19 = 0x00020027,
		SelectActor20 = 0x00020028,
		SelectActor21 = 0x00020029,
		SelectActor22 = 0x0002002A,
		SelectActor23 = 0x0002002B,
		SelectActor24 = 0x0002002C,
		ClearSelectActors = 0x00020019,
		ClearSelectModes = 0x00020035,
		DeselectAll = 0x0002000B,

		SequencerPlayForward = 0x0002000C,
		SequencerPlayReverse = 0x0002002F,
		SequencerStop = 0x0002000D,
		SequencerStepForward = 0x0002000E,
		SequencerStepBackward = 0x0002000F,
		SequencerJumpToStart = 0x00020010,
		SequencerJumpToEnd = 0x00020011,
		SequencerShuttleForward = 0x00020012,
		SequencerShuttleBackward = 0x00020013,
		SequencerSetStartPlayback = 0x00020014,
		SequencerSetEndPlayback = 0x00020015,
		SequencerZoomIn = 0x00020016,
		SequencerZoomOut = 0x00020017,
		SequencerZoomToFit = 0x00020018,

		StartPIESession = 0x00020007,
		StartSIESession = 0x00020008,
		ToggleSession = 0x00020009,
		StopSession = 0x0002000A,

		AddDirectionalLight = 0x00020030,
		AddPointLight = 0x00020031,
		AddSpotLight = 0x0002001A,
		AddRectLight = 0x00020032,
		AddSkyLight = 0x00020033,

		Undo = 0x0002002D,
		Redo = 0x0002002E,

		ClearResetCache = 0x00020034,

		// menus
		ActorMobility = 0x00040001,
		TransformReference = 0x00040002,

		UseLightTemperature = 0x00040003,
		AffectsWorld = 0x00040004,
		CastShadows = 0x00040005,

		UseGlobalSaturation = 0x00040006,
		UseGlobalContrast = 0x00040007,
		UseGlobalGamma = 0x00040008,
		UseGlobalGain = 0x00040009,
		UseGlobalOffset = 0x0004000A,

		UseShadowsSaturation = 0x0004000B,
		UseShadowsContrast = 0x0004000C,
		UseShadowsGamma = 0x0004000D,
		UseShadowsGain = 0x0004000E,
		UseShadowsOffset = 0x0004000F,

		UseMidtonesSaturation = 0x00040010,
		UseMidtonesContrast = 0x00040011,
		UseMidtonesGamma = 0x00040012,
		UseMidtonesGain = 0x00040013,
		UseMidtonesOffset = 0x00040014,

		UseHighlightsSaturation = 0x00040015,
		UseHighlightsContrast = 0x00040016,
		UseHighlightsGamma = 0x00040017,
		UseHighlightsGain = 0x00040018,
		UseHighlightsOffset = 0x00040019,

		SelectionChangesMode = 0x0004001A,

		CineCameraLensSettings = 0x0004001B,
		CineCameraFocusPlane = 0x0004001D,

		UseAllColorGrading = 0x0004001C,

		UseTemperatureType = 0x0004001E,
		UseTemp = 0x0004001F,
		UseTint = 0x00040020,

		TempType = 0x00040021,

		CCRType = 0x0040022,
		CCRInvert = 0x00040023,
		CCRTemperatureType = 0x00040024,
		CCREnabled = 0x00040025,
		CCRExcludeStencil = 0x00040026,

		Visible = 0x0040027,

		CPPUseTemperatureType = 0x00040028,
		CPPUseTemp = 0x00040029,
		CPPUseTint = 0x0004002A,

		CPPTempType = 0x0004002B,

		CPPUseGlobalSaturation = 0x0004002C,
		CPPUseGlobalContrast = 0x0004002D,
		CPPUseGlobalGamma = 0x0004002E,
		CPPUseGlobalGain = 0x0004002F,
		CPPUseGlobalOffset = 0x00040030,

		CPPUseShadowsSaturation = 0x00040031,
		CPPUseShadowsContrast = 0x00040032,
		CPPUseShadowsGamma = 0x00040033,
		CPPUseShadowsGain = 0x00040034,
		CPPUseShadowsOffset = 0x00040035,

		CPPUseMidtonesSaturation = 0x00040036,
		CPPUseMidtonesContrast = 0x00040037,
		CPPUseMidtonesGamma = 0x00040038,
		CPPUseMidtonesGain = 0x00040039,
		CPPUseMidtonesOffset = 0x0004003A,

		CPPUseHighlightsSaturation = 0x0004003B,
		CPPUseHighlightsContrast = 0x0004003C,
		CPPUseHighlightsGamma = 0x0004003D,
		CPPUseHighlightsGain = 0x0004003E,
		CPPUseHighlightsOffset = 0x0004003F,

		CPPUseAllColorGrading = 0x00040040,

		// parameters
		ActorLocationX = 0x00030001,
		ActorLocationY = 0x00030002,
		ActorLocationZ = 0x00030003,
		ActorRotationX = 0x00030004,
		ActorRotationY = 0x00030005,
		ActorRotationZ = 0x00030006,
		ActorScaleX = 0x00030007,
		ActorScaleY = 0x00030008,
		ActorScaleZ = 0x00030009,
		ActorScaleAll = 0x0003000A,

  		ActorOrientationRoll = 0x00030180,
		ActorOrientationPitch = 0x00030181,
		ActorOrientationYaw = 0x00030182,

		CameraDolly = 0x0003000B,
		CameraTruck = 0x0003000C,
		CameraPedestal = 0x0003000D,
		CameraPan = 0x0003000E,
		CameraTilt = 0x0003000F,
		CameraRoll = 0x00030010,

		CineCameraFocalLength = 0x00030011,
		CineCameraAperature = 0x00030012,
		CineCameraFocusMethod = 0x00030013,
		CineCameraManualFocusDistance = 0x00030014,
		CineCameraFocusOffset = 0x00030015,

		SequencerZoom = 0x00030016,
		SequencerPlaybackSpeed = 0x000300EE,

		LightColorRed = 0x00030017,
		LightColorGreen = 0x00030018,
		LightColorBlue = 0x00030019,
		LightColorX = 0x0003001A,
		LightColorY = 0x0003001B,
		LightColorZ = 0x0003001C,
		LightColorHue = 0x0003001D,
		LightColorSaturation = 0x0003001E,
		LightColorValue = 0x0003001F,
		LightIntensity = 0x0003002A,
		LightTemperature = 0x0003002B,
		IndirectLightingIntensity = 0x0003002C,
		VolumetricScatteringintensity = 0x0003002D,
		LightSourceAngle = 0x0003002E,
		LightSourceSoftAngle = 0x0003002F,
		AttenuationRadius = 0x00030030,
		SourceRadius = 0x00030031,
		SoftSourceRadius = 0x00030032,
		SourceLength = 0x00030033,
		InnerConeAngle = 0x00030034,
		OuterConeAngle = 0x00030035,
		SourceWidth = 0x00030036,
		SourceHeight = 0x00030037,
		BarnDoorAngle = 0x00030038,
		BarnDoorLength = 0x00030039,

		CCRPriority = 0x00030096,
		CCRIntensity = 0x00030097,
		CCRInner = 0x00030098,
		CCROuter = 0x00030099,
		CCRFalloff = 0x0003009A,
		CCRTemperature = 0x0003009B,

		CCRGlobalSaturationR = 0x0003009C,
		CCRGlobalSaturationG = 0x0003009D,
		CCRGlobalSaturationB = 0x0003009E,
		CCRGlobalSaturationY = 0x0003009F,

		CCRGlobalContrastR = 0x000300A0,
		CCRGlobalContrastG = 0x000300A1,
		CCRGlobalContrastB = 0x000300A2,
		CCRGlobalContrastY = 0x000300A3,

		CCRGlobalGammaR = 0x000300A4,
		CCRGlobalGammaG = 0x000300A5,
		CCRGlobalGammaB = 0x000300A6,
		CCRGlobalGammaY = 0x000300A7,

		CCRGlobalGainR = 0x000300A8,
		CCRGlobalGainG = 0x000300A9,
		CCRGlobalGainB = 0x000300AA,
		CCRGlobalGainY = 0x000300AB,

		CCRGlobalOffsetR = 0x000300AC,
		CCRGlobalOffsetG = 0x000300AD,
		CCRGlobalOffsetB = 0x000300AE,
		CCRGlobalOffsetY = 0x000300AF,

		CCRShadowsSaturationR = 0x000300B0,
		CCRShadowsSaturationG = 0x000300B1,
		CCRShadowsSaturationB = 0x000300B2,
		CCRShadowsSaturationY = 0x000300B3,

		CCRShadowsContrastR = 0x000300B4,
		CCRShadowsContrastG = 0x000300B5,
		CCRShadowsContrastB = 0x000300B6,
		CCRShadowsContrastY = 0x000300B7,

		CCRShadowsGammaR = 0x000300B8,
		CCRShadowsGammaG = 0x000300B9,
		CCRShadowsGammaB = 0x000300BA,
		CCRShadowsGammaY = 0x000300BB,

		CCRShadowsGainR = 0x000300BC,
		CCRShadowsGainG = 0x000300BD,
		CCRShadowsGainB = 0x000300BE,
		CCRShadowsGainY = 0x000300BF,

		CCRShadowsOffsetR = 0x000300C0,
		CCRShadowsOffsetG = 0x000300C1,
		CCRShadowsOffsetB = 0x000300C2,
		CCRShadowsOffsetY = 0x000300C3,

		CCRMidtonesSaturationR = 0x000300C4,
		CCRMidtonesSaturationG = 0x000300C5,
		CCRMidtonesSaturationB = 0x000300C6,
		CCRMidtonesSaturationY = 0x000300C7,

		CCRMidtonesContrastR = 0x000300C8,
		CCRMidtonesContrastG = 0x000300C9,
		CCRMidtonesContrastB = 0x000300CA,
		CCRMidtonesContrastY = 0x000300CB,

		CCRMidtonesGammaR = 0x000300CC,
		CCRMidtonesGammaG = 0x000300CD,
		CCRMidtonesGammaB = 0x000300CE,
		CCRMidtonesGammaY = 0x000300CF,

		CCRMidtonesGainR = 0x000300D0,
		CCRMidtonesGainG = 0x000300D1,
		CCRMidtonesGainB = 0x000300D2,
		CCRMidtonesGainY = 0x000300D3,

		CCRMidtonesOffsetR = 0x000300D4,
		CCRMidtonesOffsetG = 0x000300D5,
		CCRMidtonesOffsetB = 0x000300D6,
		CCRMidtonesOffsetY = 0x000300D7,

		CCRHighlightsSaturationR = 0x000300D8,
		CCRHighlightsSaturationG = 0x000300D9,
		CCRHighlightsSaturationB = 0x000300DA,
		CCRHighlightsSaturationY = 0x000300DB,

		CCRHighlightsContrastR = 0x000300DC,
		CCRHighlightsContrastG = 0x000300DD,
		CCRHighlightsContrastB = 0x000300DE,
		CCRHighlightsContrastY = 0x000300DF,

		CCRHighlightsGammaR = 0x000300E0,
		CCRHighlightsGammaG = 0x000300E1,
		CCRHighlightsGammaB = 0x000300E2,
		CCRHighlightsGammaY = 0x000300E3,

		CCRHighlightsGainR = 0x000300E4,
		CCRHighlightsGainG = 0x000300E5,
		CCRHighlightsGainB = 0x000300E6,
		CCRHighlightsGainY = 0x000300E7,

		CCRHighlightsOffsetR = 0x000300E8,
		CCRHighlightsOffsetG = 0x000300E9,
		CCRHighlightsOffsetB = 0x000300EA,
		CCRHighlightsOffsetY = 0x000300EB,

		CCRShadowsMax = 0x000300EC,
		CCRHighlightsMin = 0x000300ED,

		LightCardOpacity = 0x00030141,
		LightCardFeathering = 0x00030142,

		// multiple parameters for post process volumes, color correct regions and camera grading support are encoded to simplify 
		// the code, we have a master detection value for both hsv and xyz based processing in all types, the sub levels then
		// share common values
		GRADING_PP_EncodedHSV = 0x0000F000,			// post process volumes
		GRADING_PP_EncodedXYZ = 0x0000E000,
		GRADING_CC_EncodedHSV = 0x0000D000,			// colour correct regions
		GRADING_CC_EncodedXYZ = 0x0000C000,
		GRADING_CPP_EncodedHSV = 0x0000B000,		// camera
		GRADING_CPP_EncodedXYZ = 0x0000A000,

		// there are top level sections...
		GRADING_Global = 0x100,
		GRADING_Shadows = 0x200,
		GRADING_Midtones = 0x300,
		GRADING_Highlights = 0x400,
		// ... and each section has multiple sub-sections...
		GRADING_Saturation = 0x010,
		GRADING_Contrast = 0x020,
		GRADING_Gamma = 0x030,
		GRADING_Gain = 0x040,
		GRADING_Offset = 0x050,
		// ... and each sub-section has multiple colour components of hsv...
		GRADING_H = 0x001,
		GRADING_S = 0x002,
		GRADING_V = 0x003,
		// ... or xyz
		GRADING_X = 0x001,
		GRADING_Y = 0x002,
		GRADING_Z = 0x003,

		// shortcut definitions for section selections as defined above
		GRADING_GlobalSaturation = (GRADING_Global | GRADING_Saturation),
		GRADING_GlobalContrast = (GRADING_Global | GRADING_Contrast),
		GRADING_GlobalGamma = (GRADING_Global | GRADING_Gamma),
		GRADING_GlobalGain = (GRADING_Global | GRADING_Gain),
		GRADING_GlobalOffset = (GRADING_Global | GRADING_Offset),

		GRADING_ShadowsSaturation = (GRADING_Shadows | GRADING_Saturation),
		GRADING_ShadowsContrast = (GRADING_Shadows | GRADING_Contrast),
		GRADING_ShadowsGamma = (GRADING_Shadows | GRADING_Gamma),
		GRADING_ShadowsGain = (GRADING_Shadows | GRADING_Gain),
		GRADING_ShadowsOffset = (GRADING_Shadows | GRADING_Offset),

		GRADING_MidtonesSaturation = (GRADING_Midtones | GRADING_Saturation),
		GRADING_MidtonesContrast = (GRADING_Midtones | GRADING_Contrast),
		GRADING_MidtonesGamma = (GRADING_Midtones | GRADING_Gamma),
		GRADING_MidtonesGain = (GRADING_Midtones | GRADING_Gain),
		GRADING_MidtonesOffset = (GRADING_Midtones | GRADING_Offset),

		GRADING_HighlightsSaturation = (GRADING_Highlights | GRADING_Saturation),
		GRADING_HighlightsContrast = (GRADING_Highlights | GRADING_Contrast),
		GRADING_HighlightsGamma = (GRADING_Highlights | GRADING_Gamma),
		GRADING_HighlightsGain = (GRADING_Highlights | GRADING_Gain),
		GRADING_HighlightsOffset = (GRADING_Highlights | GRADING_Offset),

		// encoded ids for hsv based params
		CCRGlobalSaturationH = 0x0003D111,
		CCRGlobalSaturationS = 0x0003D112,
		CCRGlobalSaturationV = 0x0003D113,

		CCRGlobalContrastH = 0x0003D121,
		CCRGlobalContrastS = 0x0003D122,
		CCRGlobalContrastV = 0x0003D123,

		CCRGlobalGammaH = 0x0003D131,
		CCRGlobalGammaS = 0x0003D132,
		CCRGlobalGammaV = 0x0003D133,

		CCRGlobalGainH = 0x0003D141,
		CCRGlobalGainS = 0x0003D142,
		CCRGlobalGainV = 0x0003D143,

		CCRShadowsSaturationH = 0x0003D211,
		CCRShadowsSaturationS = 0x0003D212,
		CCRShadowsSaturationV = 0x0003D213,

		CCRShadowsContrastH = 0x0003D221,
		CCRShadowsContrastS = 0x0003D222,
		CCRShadowsContrastV = 0x0003D223,

		CCRShadowsGammaH = 0x0003D231,
		CCRShadowsGammaS = 0x0003D232,
		CCRShadowsGammaV = 0x0003D233,

		CCRShadowsGainH = 0x0003D241,
		CCRShadowsGainS = 0x0003D242,
		CCRShadowsGainV = 0x0003D243,

		CCRMidtonesSaturationH = 0x0003D311,
		CCRMidtonesSaturationS = 0x0003D312,
		CCRMidtonesSaturationV = 0x0003D313,

		CCRMidtonesContrastH = 0x0003D321,
		CCRMidtonesContrastS = 0x0003D322,
		CCRMidtonesContrastV = 0x0003D323,

		CCRMidtonesGammaH = 0x0003D331,
		CCRMidtonesGammaS = 0x0003D332,
		CCRMidtonesGammaV = 0x0003D333,

		CCRMidtonesGainH = 0x0003D341,
		CCRMidtonesGainS = 0x0003D342,
		CCRMidtonesGainV = 0x0003D343,

		CCRHighlightsSaturationH = 0x0003D411,
		CCRHighlightsSaturationS = 0x0003D412,
		CCRHighlightsSaturationV = 0x0003D413,

		CCRHighlightsContrastH = 0x0003D421,
		CCRHighlightsContrastS = 0x0003D422,
		CCRHighlightsContrastV = 0x0003D423,

		CCRHighlightsGammaH = 0x0003D431,
		CCRHighlightsGammaS = 0x0003D432,
		CCRHighlightsGammaV = 0x0003D433,

		CCRHighlightsGainH = 0x0003D441,
		CCRHighlightsGainS = 0x0003D442,
		CCRHighlightsGainV = 0x0003D443,

		// encoded ids for xyz based params for trackerballs, note the extra 'T' to avoid clash with the luminance definition
		CCRGlobalSaturationTX = 0x0003C111,
		CCRGlobalSaturationTY = 0x0003C112,
		CCRGlobalSaturationTZ = 0x0003C113,

		CCRGlobalContrastTX = 0x0003C121,
		CCRGlobalContrastTY = 0x0003C122,
		CCRGlobalContrastTZ = 0x0003C123,

		CCRGlobalGammaTX = 0x0003C131,
		CCRGlobalGammaTY = 0x0003C132,
		CCRGlobalGammaTZ = 0x0003C133,

		CCRGlobalGainTX = 0x0003C141,
		CCRGlobalGainTY = 0x0003C142,
		CCRGlobalGainTZ = 0x0003C143,

		CCRGlobalOffsetTX = 0x0003C151,
		CCRGlobalOffsetTY = 0x0003C152,
		CCRGlobalOffsetTZ = 0x0003C153,

		CCRShadowsSaturationTX = 0x0003C211,
		CCRShadowsSaturationTY = 0x0003C212,
		CCRShadowsSaturationTZ = 0x0003C213,

		CCRShadowsContrastTX = 0x0003C221,
		CCRShadowsContrastTY = 0x0003C222,
		CCRShadowsContrastTZ = 0x0003C223,

		CCRShadowsGammaTX = 0x0003C231,
		CCRShadowsGammaTY = 0x0003C232,
		CCRShadowsGammaTZ = 0x0003C233,

		CCRShadowsGainTX = 0x0003C241,
		CCRShadowsGainTY = 0x0003C242,
		CCRShadowsGainTZ = 0x0003C243,

		CCRShadowsOffsetTX = 0x0003C251,
		CCRShadowsOffsetTY = 0x0003C252,
		CCRShadowsOffsetTZ = 0x0003C253,

		CCRMidtonesSaturationTX = 0x0003C311,
		CCRMidtonesSaturationTY = 0x0003C312,
		CCRMidtonesSaturationTZ = 0x0003C313,

		CCRMidtonesContrastTX = 0x0003C321,
		CCRMidtonesContrastTY = 0x0003C322,
		CCRMidtonesContrastTZ = 0x0003C323,

		CCRMidtonesGammaTX = 0x0003C331,
		CCRMidtonesGammaTY = 0x0003C332,
		CCRMidtonesGammaTZ = 0x0003C333,

		CCRMidtonesGainTX = 0x0003C341,
		CCRMidtonesGainTY = 0x0003C342,
		CCRMidtonesGainTZ = 0x0003C343,

		CCRMidtonesOffsetTX = 0x0003C351,
		CCRMidtonesOffsetTY = 0x0003C352,
		CCRMidtonesOffsetTZ = 0x0003C353,

		CCRHighlightsSaturationTX = 0x0003C411,
		CCRHighlightsSaturationTY = 0x0003C412,
		CCRHighlightsSaturationTZ = 0x0003C413,

		CCRHighlightsContrastTX = 0x0003C421,
		CCRHighlightsContrastTY = 0x0003C422,
		CCRHighlightsContrastTZ = 0x0003C423,

		CCRHighlightsGammaTX = 0x0003C431,
		CCRHighlightsGammaTY = 0x0003C432,
		CCRHighlightsGammaTZ = 0x0003C433,

		CCRHighlightsGainTX = 0x0003C441,
		CCRHighlightsGainTY = 0x0003C442,
		CCRHighlightsGainTZ = 0x0003C443,

		CCRHighlightsOffsetTX = 0x0003C451,
		CCRHighlightsOffsetTY = 0x0003C452,
		CCRHighlightsOffsetTZ = 0x0003C453,

		GlobalSaturationR = 0x0003003E,
		GlobalSaturationG = 0x0003003F,
		GlobalSaturationB = 0x0003004A,
		GlobalSaturationY = 0x0003004B,

		GlobalContrastR = 0x00030054,
		GlobalContrastG = 0x00030055,
		GlobalContrastB = 0x00030056,
		GlobalContrastY = 0x00030057,

		GlobalGammaR = 0x00030058,
		GlobalGammaG = 0x00030059,
		GlobalGammaB = 0x0003005A,
		GlobalGammaY = 0x0003005B,

		GlobalGainR = 0x0003005C,
		GlobalGainG = 0x0003005D,
		GlobalGainB = 0x0003005E,
		GlobalGainY = 0x0003005F,

		GlobalOffsetR = 0x00030060,
		GlobalOffsetG = 0x00030061,
		GlobalOffsetB = 0x00030062,
		GlobalOffsetY = 0x00030063,

		ShadowsSaturationR = 0x0003003A,
		ShadowsSaturationG = 0x0003003B,
		ShadowsSaturationB = 0x0003003C,
		ShadowsSaturationY = 0x0003003D,

		ShadowsContrastR = 0x00030064,
		ShadowsContrastG = 0x00030065,
		ShadowsContrastB = 0x00030066,
		ShadowsContrastY = 0x00030067,

		ShadowsGammaR = 0x00030068,
		ShadowsGammaG = 0x00030069,
		ShadowsGammaB = 0x0003006A,
		ShadowsGammaY = 0x0003006B,

		ShadowsGainR = 0x0003006C,
		ShadowsGainG = 0x0003006D,
		ShadowsGainB = 0x0003006E,
		ShadowsGainY = 0x0003006F,

		ShadowsOffsetR = 0x00030070,
		ShadowsOffsetG = 0x00030071,
		ShadowsOffsetB = 0x00030072,
		ShadowsOffsetY = 0x00030073,

		MidtonesSaturationR = 0x0003004C,
		MidtonesSaturationG = 0x0003004D,
		MidtonesSaturationB = 0x0003004E,
		MidtonesSaturationY = 0x0003004F,

		MidtonesContrastR = 0x00030074,
		MidtonesContrastG = 0x00030075,
		MidtonesContrastB = 0x00030076,
		MidtonesContrastY = 0x00030077,

		MidtonesGammaR = 0x00030078,
		MidtonesGammaG = 0x00030079,
		MidtonesGammaB = 0x0003007A,
		MidtonesGammaY = 0x0003007B,

		MidtonesGainR = 0x0003007C,
		MidtonesGainG = 0x0003007D,
		MidtonesGainB = 0x0003007E,
		MidtonesGainY = 0x0003007F,

		MidtonesOffsetR = 0x00030080,
		MidtonesOffsetG = 0x00030081,
		MidtonesOffsetB = 0x00030082,
		MidtonesOffsetY = 0x00030083,

		HighlightsSaturationR = 0x00030050,
		HighlightsSaturationG = 0x00030051,
		HighlightsSaturationB = 0x00030052,
		HighlightsSaturationY = 0x00030053,

		HighlightsContrastR = 0x00030084,
		HighlightsContrastG = 0x00030085,
		HighlightsContrastB = 0x00030086,
		HighlightsContrastY = 0x00030087,

		HighlightsGammaR = 0x00030088,
		HighlightsGammaG = 0x00030089,
		HighlightsGammaB = 0x0003008A,
		HighlightsGammaY = 0x0003008B,

		HighlightsGainR = 0x0003008C,
		HighlightsGainG = 0x0003008D,
		HighlightsGainB = 0x0003008E,
		HighlightsGainY = 0x0003008F,

		HighlightsOffsetR = 0x00030090,
		HighlightsOffsetG = 0x00030091,
		HighlightsOffsetB = 0x00030092,
		HighlightsOffsetY = 0x00030093,

		// encoded ids for hsv based params
		GlobalSaturationH = 0x0003F111,
		GlobalSaturationS = 0x0003F112,
		GlobalSaturationV = 0x0003F113,

		GlobalContrastH = 0x0003F121,
		GlobalContrastS = 0x0003F122,
		GlobalContrastV = 0x0003F123,

		GlobalGammaH = 0x0003F131,
		GlobalGammaS = 0x0003F132,
		GlobalGammaV = 0x0003F133,

		GlobalGainH = 0x0003F141,
		GlobalGainS = 0x0003F142,
		GlobalGainV = 0x0003F143,

		ShadowsSaturationH = 0x0003F211,
		ShadowsSaturationS = 0x0003F212,
		ShadowsSaturationV = 0x0003F213,

		ShadowsContrastH = 0x0003F221,
		ShadowsContrastS = 0x0003F222,
		ShadowsContrastV = 0x0003F223,

		ShadowsGammaH = 0x0003F231,
		ShadowsGammaS = 0x0003F232,
		ShadowsGammaV = 0x0003F233,

		ShadowsGainH = 0x0003F241,
		ShadowsGainS = 0x0003F242,
		ShadowsGainV = 0x0003F243,

		MidtonesSaturationH = 0x0003F311,
		MidtonesSaturationS = 0x0003F312,
		MidtonesSaturationV = 0x0003F313,

		MidtonesContrastH = 0x0003F321,
		MidtonesContrastS = 0x0003F322,
		MidtonesContrastV = 0x0003F323,

		MidtonesGammaH = 0x0003F331,
		MidtonesGammaS = 0x0003F332,
		MidtonesGammaV = 0x0003F333,

		MidtonesGainH = 0x0003F341,
		MidtonesGainS = 0x0003F342,
		MidtonesGainV = 0x0003F343,

		HighlightsSaturationH = 0x0003F411,
		HighlightsSaturationS = 0x0003F412,
		HighlightsSaturationV = 0x0003F413,

		HighlightsContrastH = 0x0003F421,
		HighlightsContrastS = 0x0003F422,
		HighlightsContrastV = 0x0003F423,

		HighlightsGammaH = 0x0003F431,
		HighlightsGammaS = 0x0003F432,
		HighlightsGammaV = 0x0003F433,

		HighlightsGainH = 0x0003F441,
		HighlightsGainS = 0x0003F442,
		HighlightsGainV = 0x0003F443,

		// encoded ids for xyz based params for trackerballs, note the extra 'T' to avoid clash with the luminance definition
		GlobalSaturationTX = 0x0003E111,
		GlobalSaturationTY = 0x0003E112,
		GlobalSaturationTZ = 0x0003E113,

		GlobalContrastTX = 0x0003E121,
		GlobalContrastTY = 0x0003E122,
		GlobalContrastTZ = 0x0003E123,

		GlobalGammaTX = 0x0003E131,
		GlobalGammaTY = 0x0003E132,
		GlobalGammaTZ = 0x0003E133,

		GlobalGainTX = 0x0003E141,
		GlobalGainTY = 0x0003E142,
		GlobalGainTZ = 0x0003E143,

		GlobalOffsetTX = 0x0003E151,
		GlobalOffsetTY = 0x0003E152,
		GlobalOffsetTZ = 0x0003E153,

		ShadowsSaturationTX = 0x0003E211,
		ShadowsSaturationTY = 0x0003E212,
		ShadowsSaturationTZ = 0x0003E213,

		ShadowsContrastTX = 0x0003E221,
		ShadowsContrastTY = 0x0003E222,
		ShadowsContrastTZ = 0x0003E223,

		ShadowsGammaTX = 0x0003E231,
		ShadowsGammaTY = 0x0003E232,
		ShadowsGammaTZ = 0x0003E233,

		ShadowsGainTX = 0x0003E241,
		ShadowsGainTY = 0x0003E242,
		ShadowsGainTZ = 0x0003E243,

		ShadowsOffsetTX = 0x0003E251,
		ShadowsOffsetTY = 0x0003E252,
		ShadowsOffsetTZ = 0x0003E253,

		MidtonesSaturationTX = 0x0003E311,
		MidtonesSaturationTY = 0x0003E312,
		MidtonesSaturationTZ = 0x0003E313,

		MidtonesContrastTX = 0x0003E321,
		MidtonesContrastTY = 0x0003E322,
		MidtonesContrastTZ = 0x0003E323,

		MidtonesGammaTX = 0x0003E331,
		MidtonesGammaTY = 0x0003E332,
		MidtonesGammaTZ = 0x0003E333,

		MidtonesGainTX = 0x0003E341,
		MidtonesGainTY = 0x0003E342,
		MidtonesGainTZ = 0x0003E343,

		MidtonesOffsetTX = 0x0003E351,
		MidtonesOffsetTY = 0x0003E352,
		MidtonesOffsetTZ = 0x0003E353,

		HighlightsSaturationTX = 0x0003E411,
		HighlightsSaturationTY = 0x0003E412,
		HighlightsSaturationTZ = 0x0003E413,

		HighlightsContrastTX = 0x0003E421,
		HighlightsContrastTY = 0x0003E422,
		HighlightsContrastTZ = 0x0003E423,

		HighlightsGammaTX = 0x0003E431,
		HighlightsGammaTY = 0x0003E432,
		HighlightsGammaTZ = 0x0003E433,

		HighlightsGainTX = 0x0003E441,
		HighlightsGainTY = 0x0003E442,
		HighlightsGainTZ = 0x0003E443,

		HighlightsOffsetTX = 0x0003E451,
		HighlightsOffsetTY = 0x0003E452,
		HighlightsOffsetTZ = 0x0003E453,

		Temp = 0x00030094,
		Tint = 0x00030095,

		CPPTemp = 0x000300EF,
		CPPTint = 0x000300F0,

		CPPGlobalSaturationR = 0x000300F1,
		CPPGlobalSaturationG = 0x000300F2,
		CPPGlobalSaturationB = 0x000300F3,
		CPPGlobalSaturationY = 0x000300F4,

		CPPGlobalContrastR = 0x000300F5,
		CPPGlobalContrastG = 0x000300F6,
		CPPGlobalContrastB = 0x000300F7,
		CPPGlobalContrastY = 0x000300F8,

		CPPGlobalGammaR = 0x000300F9,
		CPPGlobalGammaG = 0x000300FA,
		CPPGlobalGammaB = 0x000300FB,
		CPPGlobalGammaY = 0x000300FC,

		CPPGlobalGainR = 0x000300FD,
		CPPGlobalGainG = 0x000300FE,
		CPPGlobalGainB = 0x000300FF,
		CPPGlobalGainY = 0x00030100,

		CPPGlobalOffsetR = 0x00030101,
		CPPGlobalOffsetG = 0x00030102,
		CPPGlobalOffsetB = 0x00030103,
		CPPGlobalOffsetY = 0x00030104,

		CPPShadowsSaturationR = 0x00030105,
		CPPShadowsSaturationG = 0x00030106,
		CPPShadowsSaturationB = 0x00030107,
		CPPShadowsSaturationY = 0x00030108,

		CPPShadowsContrastR = 0x00030109,
		CPPShadowsContrastG = 0x0003010A,
		CPPShadowsContrastB = 0x0003010B,
		CPPShadowsContrastY = 0x0003010C,

		CPPShadowsGammaR = 0x0003010D,
		CPPShadowsGammaG = 0x0003010E,
		CPPShadowsGammaB = 0x0003010F,
		CPPShadowsGammaY = 0x00030110,

		CPPShadowsGainR = 0x00030111,
		CPPShadowsGainG = 0x00030112,
		CPPShadowsGainB = 0x00030113,
		CPPShadowsGainY = 0x00030114,

		CPPShadowsOffsetR = 0x00030115,
		CPPShadowsOffsetG = 0x00030116,
		CPPShadowsOffsetB = 0x00030117,
		CPPShadowsOffsetY = 0x00030118,

		CPPMidtonesSaturationR = 0x00030119,
		CPPMidtonesSaturationG = 0x0003011A,
		CPPMidtonesSaturationB = 0x0003011B,
		CPPMidtonesSaturationY = 0x0003011C,

		CPPMidtonesContrastR = 0x0003011D,
		CPPMidtonesContrastG = 0x0003011E,
		CPPMidtonesContrastB = 0x0003011F,
		CPPMidtonesContrastY = 0x00030120,

		CPPMidtonesGammaR = 0x00030121,
		CPPMidtonesGammaG = 0x00030122,
		CPPMidtonesGammaB = 0x00030123,
		CPPMidtonesGammaY = 0x00030124,

		CPPMidtonesGainR = 0x00030125,
		CPPMidtonesGainG = 0x00030126,
		CPPMidtonesGainB = 0x00030127,
		CPPMidtonesGainY = 0x00030128,

		CPPMidtonesOffsetR = 0x00030129,
		CPPMidtonesOffsetG = 0x0003012A,
		CPPMidtonesOffsetB = 0x0003012B,
		CPPMidtonesOffsetY = 0x0003012C,

		CPPHighlightsSaturationR = 0x0003012D,
		CPPHighlightsSaturationG = 0x0003012E,
		CPPHighlightsSaturationB = 0x0003012F,
		CPPHighlightsSaturationY = 0x00030130,

		CPPHighlightsContrastR = 0x00030131,
		CPPHighlightsContrastG = 0x00030132,
		CPPHighlightsContrastB = 0x00030133,
		CPPHighlightsContrastY = 0x00030134,

		CPPHighlightsGammaR = 0x00030135,
		CPPHighlightsGammaG = 0x00030136,
		CPPHighlightsGammaB = 0x00030137,
		CPPHighlightsGammaY = 0x00030138,

		CPPHighlightsGainR = 0x00030139,
		CPPHighlightsGainG = 0x0003013A,
		CPPHighlightsGainB = 0x0003013B,
		CPPHighlightsGainY = 0x0003013C,

		CPPHighlightsOffsetR = 0x0003013D,
		CPPHighlightsOffsetG = 0x0003013E,
		CPPHighlightsOffsetB = 0x0003013F,
		CPPHighlightsOffsetY = 0x00030140,

		// encoded ids for hsv based params
		CPPGlobalSaturationH = 0x0003B111,
		CPPGlobalSaturationS = 0x0003B112,
		CPPGlobalSaturationV = 0x0003B113,

		CPPGlobalContrastH = 0x0003B121,
		CPPGlobalContrastS = 0x0003B122,
		CPPGlobalContrastV = 0x0003B123,

		CPPGlobalGammaH = 0x0003B131,
		CPPGlobalGammaS = 0x0003B132,
		CPPGlobalGammaV = 0x0003B133,

		CPPGlobalGainH = 0x0003B141,
		CPPGlobalGainS = 0x0003B142,
		CPPGlobalGainV = 0x0003B143,

		CPPShadowsSaturationH = 0x0003B211,
		CPPShadowsSaturationS = 0x0003B212,
		CPPShadowsSaturationV = 0x0003B213,

		CPPShadowsContrastH = 0x0003B221,
		CPPShadowsContrastS = 0x0003B222,
		CPPShadowsContrastV = 0x0003B223,

		CPPShadowsGammaH = 0x0003B231,
		CPPShadowsGammaS = 0x0003B232,
		CPPShadowsGammaV = 0x0003B233,

		CPPShadowsGainH = 0x0003B241,
		CPPShadowsGainS = 0x0003B242,
		CPPShadowsGainV = 0x0003B243,

		CPPMidtonesSaturationH = 0x0003B311,
		CPPMidtonesSaturationS = 0x0003B312,
		CPPMidtonesSaturationV = 0x0003B313,

		CPPMidtonesContrastH = 0x0003B321,
		CPPMidtonesContrastS = 0x0003B322,
		CPPMidtonesContrastV = 0x0003B323,

		CPPMidtonesGammaH = 0x0003B331,
		CPPMidtonesGammaS = 0x0003B332,
		CPPMidtonesGammaV = 0x0003B333,

		CPPMidtonesGainH = 0x0003B341,
		CPPMidtonesGainS = 0x0003B342,
		CPPMidtonesGainV = 0x0003B343,

		CPPHighlightsSaturationH = 0x0003B411,
		CPPHighlightsSaturationS = 0x0003B412,
		CPPHighlightsSaturationV = 0x0003B413,

		CPPHighlightsContrastH = 0x0003B421,
		CPPHighlightsContrastS = 0x0003B422,
		CPPHighlightsContrastV = 0x0003B423,

		CPPHighlightsGammaH = 0x0003B431,
		CPPHighlightsGammaS = 0x0003B432,
		CPPHighlightsGammaV = 0x0003B433,

		CPPHighlightsGainH = 0x0003B441,
		CPPHighlightsGainS = 0x0003B442,
		CPPHighlightsGainV = 0x0003B443,

		// encoded ids for xyz based params for trackerballs, note the extra 'T' to avoid clash with the luminance definition
		CPPGlobalSaturationTX = 0x0003A111,
		CPPGlobalSaturationTY = 0x0003A112,
		CPPGlobalSaturationTZ = 0x0003A113,

		CPPGlobalContrastTX = 0x0003A121,
		CPPGlobalContrastTY = 0x0003A122,
		CPPGlobalContrastTZ = 0x0003A123,

		CPPGlobalGammaTX = 0x0003A131,
		CPPGlobalGammaTY = 0x0003A132,
		CPPGlobalGammaTZ = 0x0003A133,

		CPPGlobalGainTX = 0x0003A141,
		CPPGlobalGainTY = 0x0003A142,
		CPPGlobalGainTZ = 0x0003A143,

		CPPGlobalOffsetTX = 0x0003A151,
		CPPGlobalOffsetTY = 0x0003A152,
		CPPGlobalOffsetTZ = 0x0003A153,

		CPPShadowsSaturationTX = 0x0003A211,
		CPPShadowsSaturationTY = 0x0003A212,
		CPPShadowsSaturationTZ = 0x0003A213,

		CPPShadowsContrastTX = 0x0003A221,
		CPPShadowsContrastTY = 0x0003A222,
		CPPShadowsContrastTZ = 0x0003A223,

		CPPShadowsGammaTX = 0x0003A231,
		CPPShadowsGammaTY = 0x0003A232,
		CPPShadowsGammaTZ = 0x0003A233,

		CPPShadowsGainTX = 0x0003A241,
		CPPShadowsGainTY = 0x0003A242,
		CPPShadowsGainTZ = 0x0003A243,

		CPPShadowsOffsetTX = 0x0003A251,
		CPPShadowsOffsetTY = 0x0003A252,
		CPPShadowsOffsetTZ = 0x0003A253,

		CPPMidtonesSaturationTX = 0x0003A311,
		CPPMidtonesSaturationTY = 0x0003A312,
		CPPMidtonesSaturationTZ = 0x0003A313,

		CPPMidtonesContrastTX = 0x0003A321,
		CPPMidtonesContrastTY = 0x0003A322,
		CPPMidtonesContrastTZ = 0x0003A323,

		CPPMidtonesGammaTX = 0x0003A331,
		CPPMidtonesGammaTY = 0x0003A332,
		CPPMidtonesGammaTZ = 0x0003A333,

		CPPMidtonesGainTX = 0x0003A341,
		CPPMidtonesGainTY = 0x0003A342,
		CPPMidtonesGainTZ = 0x0003A343,

		CPPMidtonesOffsetTX = 0x0003A351,
		CPPMidtonesOffsetTY = 0x0003A352,
		CPPMidtonesOffsetTZ = 0x0003A353,

		CPPHighlightsSaturationTX = 0x0003A411,
		CPPHighlightsSaturationTY = 0x0003A412,
		CPPHighlightsSaturationTZ = 0x0003A413,

		CPPHighlightsContrastTX = 0x0003A421,
		CPPHighlightsContrastTY = 0x0003A422,
		CPPHighlightsContrastTZ = 0x0003A423,

		CPPHighlightsGammaTX = 0x0003A431,
		CPPHighlightsGammaTY = 0x0003A432,
		CPPHighlightsGammaTZ = 0x0003A433,

		CPPHighlightsGainTX = 0x0003A441,
		CPPHighlightsGainTY = 0x0003A442,
		CPPHighlightsGainTZ = 0x0003A443,

		CPPHighlightsOffsetTX = 0x0003A451,
		CPPHighlightsOffsetTY = 0x0003A452,
		CPPHighlightsOffsetTZ = 0x0003A453
	};

	// helper functions for post process target ids to extract meta data about settings sections and colour components
	inline bool IsGradingHSV(uint32 targetId) { uint32 val = targetId & 0x0000F000; return ((val == GRADING_PP_EncodedHSV) || (val == GRADING_CC_EncodedHSV) || (val == GRADING_CPP_EncodedHSV)); }
	inline bool IsGradingXYZ(uint32 targetId) { uint32 val = targetId & 0x0000F000; return ((val == GRADING_PP_EncodedXYZ) || (val == GRADING_CC_EncodedXYZ) || (val == GRADING_CPP_EncodedXYZ)); }
	inline bool IsPostProcessGrading(uint32 targetId) { uint32 val = targetId & 0x0000F000; return ((val == GRADING_PP_EncodedHSV) || (val == GRADING_PP_EncodedXYZ)); }
	inline bool IsColorCorrectGrading(uint32 targetId) { uint32 val = targetId & 0x0000F000; return ((val == GRADING_CC_EncodedHSV) || (val == GRADING_CC_EncodedXYZ)); }
	inline bool IsCameraPostProcessGrading(uint32 targetId) { uint32 val = targetId & 0x0000F000; return ((val == GRADING_CPP_EncodedHSV) || (val == GRADING_CPP_EncodedXYZ)); }
	inline bool IsGradingOffset(uint32 targetId) { return ((targetId & 0x000000F0) == GRADING_Offset); }
	inline uint32 GradingSettingsSection(uint32 targetId) { return (targetId & 0x00000FF0); }
	inline uint32 GradingComponent(uint32 targetId) { return (targetId & 0x0000000F); }
}
