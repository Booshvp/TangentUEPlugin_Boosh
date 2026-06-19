// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 382 $

#pragma once

#include "CoreMinimal.h"
#include "PropertyChangeHelper.h"
#include "PropertyData.h"
#include "Controls.h"
#include "HSVColorCache.h"
#include "ISequencer.h"												// Sequencer module
#include "Runtime/Engine/Classes/Engine/PostProcessVolume.h"		// Engine module
#include "Sockets.h"
#include "ColorCorrectRegion.h"										// ColorCorrectRegions plugin
#include "Editor.h"
#include "UObject/ObjectSaveContext.h"
#include "EngineHelper.generated.h"

/**
 * 
 */
UCLASS()
class TANGENT_API UEngineHelper : public UObject, public PropertyChangeHelper
{
	GENERATED_BODY()

public:
	// note - the cycled values are used in a lut for text so must be sequential and changed with care
	enum TransformReference
	{
		FIRST = 0,			// first cycled value

		Local = FIRST,		// use or override with local transform
		World,				// use or override with world transform
		Camera,				// use or override with camera relative transform
		Viewport,			// use or override with viewport relative transform

		LAST = Viewport,	// last cycled value

		Current = -1,		// use current setting

		Default = Local		// default setting
	};

	// used to encapsulate the type of the currently selected actor or component
	enum class ObjectType
	{
		None = 0,
		Other,
		Light,
		Camera,
		PostProcess,
		Sequencer,
		ColorCorrect
	};

	// bitwise flags for things that refer to x/y/z axes
	enum AxisFlag
	{
		AxisX = 1,
		AxisY = 2,
		AxisZ = 4
	};

public:
	DECLARE_MULTICAST_DELEGATE(FSelectionChangedEvent);
	static FSelectionChangedEvent SelectionChangedEvent;

	DECLARE_MULTICAST_DELEGATE(FActorPropertyChangedEvent);
	static FActorPropertyChangedEvent ActorPropertyChangedEvent;

	DECLARE_MULTICAST_DELEGATE_SixParams(FTrainResponseEvent, int32, FName, FName, float, float, float);
	static FTrainResponseEvent TrainResponseEvent;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FMenuString, uint32, FString&);
	static FMenuString MenuStringEvent;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FParameterValue, uint32, float);
	static FParameterValue ParameterValueEvent;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FCustomParameterValue, FName, float);
	static FCustomParameterValue CustomParameterValueEvent;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FCustomMenuString, FName, FString&);
	static FCustomMenuString CustomMenuStringEvent;

public:
	UEngineHelper();
	~UEngineHelper();

	void OnPreChange(AActor *actor, FProperty *property, const FString& description = "Edit Property");
	void OnPostChange(AActor* actor, FProperty *property);
	void OnPostChangeCompletion();

	void PlaySessionControl(uint32 actionId);

	FName GetSelectedClassName();
	void GetSelectedName(FString& name, bool fullName = true);
	FString GetSelectedName();
	ObjectType GetSelectedObjectType();

	void SelectObjectByName(FString &name);
	void DeselectAll();

	UEngineHelper::ObjectType GetSupportedTypeOfActor(AActor* actor);

	void SetActionTimer(FTimerDelegate &gate, float delay);
	void ClearActionTimer();

	void SequencerAction(uint32 actionId);
	void UpdateSequencerParameter(uint32 targetId, float delta);
	void ResetSequencerParameter(uint32 targetId);
	void RequestSequencerParameterValue(uint32 targetId);

	void TransactionHistoryAction(uint32 actionId);

	void UpdateCustomParameter(uint32 targetId, const FName controlString, float delta);
	void ResetCustomParameter(uint32 targetId, const FName controlString);
	void RequestCustomParameterValue(const FName controlString);

	void StepCustomMenu(const FName controlString, int32 step);
	void ResetCustomMenu(const FName controlString);
	void RequestCustomMenuString(const FName controlString);

	void OnCustomAction(const FName controlString, bool state);

	void MoveSelected(const FVector& deltas);
	void ResetMoveSelected(uint32 axisFlags);
	void RequestSelectedLocation();

	void RotateSelected(const FRotator& deltas);
	void ResetRotateSelected(uint32 axisFlags);
	void RequestSelectedRotation();

	void ScaleSelected(const FVector& deltas);
	void ResetScaleSelected(uint32 axisFlags);
	void RequestSelectedScale();

	void StepTransformReference(int32 step);
	void ResetTransformReference();
	void RequestTransformReference();

	void StepSelectionChangesMode(int32 step);
	void ResetSelectionChangesMode();
	void RequestSelectionChangesMode();

	void StepUseAllColorGrading(uint32 targetId, int32 step);
	void ResetUseAllColorGrading(uint32 targetId);
	void RequestUseAllColorGrading(uint32 targetId);

	void RegisterTargetIdProperty(uint32 targetId, const FName propertyPath);

	void UpdateParameterProperty(uint32 targetId, float delta);
	void UpdateParameterProperty(uint32 targetId, float minimum, float maximum, float delta);
	void ResetParameterProperty(uint32 targetId);
	void RequestParameterPropertyValue(uint32 targetId);

	void SetMenuProperty(uint32 targetId, bool value);
	void StepMenuProperty(uint32 targetId, int32 step);
	void ResetMenuProperty(uint32 targetId);
	void RequestMenuPropertyString(uint32 targetId);

	void UpdateCameraPan(float delta);
	void UpdateCameraTilt(float delta);
	void UpdateCameraRoll(float delta);
	void UpdateCameraDolly(float delta);
	void UpdateCameraTruck(float delta);
	void UpdateCameraPedestal(float delta);

	void StepCineCameraLensSettingsPreset(int32 step);
	void ResetCineCameraLensSettingsPreset();
	void RequestCineCameraLensSettingsPreset();

	void UpdateLightXYZ(uint32 targetId, float delta);
	void ResetLightXYZ(uint32 targetId);

	void UpdateLightHSV(uint32 targetId, float delta);
	void ResetLightHSV(uint32 targetId);
	void RequestLightHSV(uint32 targetId);

	FVector4* SelectPostProcessSettingsVector(uint32 targetId, APostProcessVolume* volume);
	FVector4* SelectColorCorrectSettingsVector(uint32 targetId, AColorCorrectRegion* region);
	FVector4* SelectCameraPostProcessSettingsVector(uint32 targetId, ACameraActor* camera);

	template<typename T1, typename T2 = T1>
	bool HandleResetOrUpdate(T1& value, T2 defaultValue, float delta, uint32 targetId, AActor* actor);

	void UpdateGradingHSV(uint32 targetId, float delta);
	void ResetGradingHSV(uint32 targetId);
	void RequestGradingHSV(uint32 targetId);

	void UpdateGradingXYZ(uint32 targetId, float delta);
	void ResetGradingXYZ(uint32 targetId);

	void StartTrainingForPanelControl(uint32 controlId);

	bool GetSelectionChangesMode();
	void SetSelectionChangesMode(bool value);

	void SpawnActor(UClass *typeUClass);

	bool IsSaveWorldActive();

private:
	UWorld* GetEditorWorld();
	UWorld* GetPlayWorld();
	UWorld* GetActiveWorld();
	ULevel* GetPersistentLevel();
	void ExclusivelySelectNamedObject(const FString &targetName);
	void UpdateSelectionCaches(AActor* newSelection);
	void OnSelectionChanged(UObject* obj);
	void OnMapChanged(uint32 flags);
	void OnPreObjectPropertyChanged(UObject* obj, const class FEditPropertyChain& propChain);
	void OnObjectPropertyChanged(UObject* obj, struct FPropertyChangedEvent& event);
	void OnSequencerCreated(TSharedRef<ISequencer> sequencerRef);
	void PostPIEStarted(bool isSimulating);
	void OnEndPIE(bool isSimulating);
	void OnPreSaveWorldWithContext(UWorld* world, FObjectPreSaveContext context);
	void OnPostSaveWorldWithContext(UWorld* world, FObjectPostSaveContext context);
	bool ResolvePropertyRecursive(TArray<FString>& nameStrings, int32 nameIndex, FProperty*& currentProp, void*& currentDataPtr, void*& currentDefaultDataPtr);
	bool ResolveProperty(UObject* rootObject, TArray<FString>& nameStrings, FProperty*& outProperty, void*& outDataPtr, void*& outDefaultDataPtr);
	FProperty* FetchProperty(UObject* obj, const FName propertyPath);
	PropertyData* FetchPropertyData(UObject* obj, const FString& propertyPath);
	PropertyData* FetchPropertyData(UObject* obj, const FName propertyPath);
	PropertyData* FetchPropertyData(UObject* obj, uint32 targetId);
	void ConvertHSToXY(const FLinearColor& color, FVector2D& coords);
	void ConvertXYToHS(const FVector2D& coords, FLinearColor& color);
	void RefreshDetailsView();
	bool TrainingArbitrator(UObject* obj, FString& propertyPath);
	void ApplyMoveSelected(const FVector& value, TransformReference transformReference, bool reset);
	void ApplyRotateSelected(const FRotator& value, TransformReference transformReference, bool reset);
	void ApplyScaleSelected(const FVector& value, TransformReference transformReference, bool reset);
	void RegisterOnChangeActorProperty(AActor* actor, FProperty* prop);
	void CompleteOnChangeActorProperties();

	template<class T>
	FORCEINLINE bool SelectedActorIsA() const;

private:
	FTimerHandle						_actionTimer;
	TransformReference					_transformReference = TransformReference::Default;
	TArray<FName>						_objectPropertyChangedExceptions;
	TMultiMap<uint32, PropertyData*>	_propertyDataCache;
	TMap<uint32, FName>					_targetIdPropertyMap;
	uint32								_trainingPanelControl = ControlId::Undefined;
	FDelegateHandle						_onSequencerCreatedDelegate;
	TWeakPtr<ISequencer>				_sequencerWeakPtr = nullptr;
	HSVColorCache						_hsvColorCache;
	bool								_selectionChangesMode = true;
	bool								_ppvUseAllColorGrading = false,
										_cppUseAllColorGrading = false;
	FTimerHandle						_postChangeTimer;
	TArray<AActor*>						_onChangeActors;		// these pointer arrays are very short lived so we live life on the edge and
	TArray<FProperty*>					_onChangeProperties;	// don't bother with shared or weak pointer etc.
	FThreadSafeBool						_saveWorldActive;

	UPROPERTY()
	AActor*								_selectedActor = nullptr;

	UPROPERTY()
	UActorComponent*					_selectedComponent = nullptr;

	UPROPERTY()
	ACameraActor*						_lastSelectedCamera = nullptr;
};
