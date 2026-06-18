// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 384 $

#pragma once

#include "CoreMinimal.h"
#include "EngineHelper.h"				// TangentEditor module
#include "TangentInput.h"
#include "ClassToModeMap.h"
#include "RxContext.h"
#include "MessageBuilder.h"

/**
 * 
 */

class TangentEngine : public FRunnable
{
public:
	TangentEngine();
	~TangentEngine();

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit();

private:
	FString ClipString(FString& original, int32 requiredLen = 9);
	void RegisterTargetIds();
	void ConnectHubComms();
	void ResetHubComms();
	FString GetPluginXmlFolder();
	bool ProcessRead(RxContext &rxContext);
	void ParseReceived(RxContext &rxContext);
	void ParseInitCommsCmd(RxContext& rxContext);
	void ParseParamChangeCmd(RxContext& rxContext);
	void ParseParamResetCmd(RxContext& rxContext);
	void ParseParamValueRequestCmd(RxContext& rxContext);
	void ParseCustomParamChangeCmd(RxContext& rxContext);
	void ParseCustomParamResetCmd(RxContext& rxContext);
	void ParseCustomParamValueRequestCmd(RxContext& rxContext);
	void ParseCustomMenuChangeCmd(RxContext& rxContext);
	void ParseCustomMenuResetCmd(RxContext& rxContext);
	void ParseCustomMenuStringRequestCmd(RxContext& rxContext);
	void ParseCustomActionOnCmd(RxContext& rxContext);
	void ParseCustomActionOffCmd(RxContext& rxContext);
	void ParseActionOnCmd(RxContext& rxContext);
	void ParseActionOffCmd(RxContext& rxContext);
	void ParseMenuChangeCmd(RxContext& rxContext);
	void ParseMenuResetCmd(RxContext& rxContext);
	void ParseMenuStringRequestCmd(RxContext& rxContext);
	void ParseModeChangeCmd(RxContext& rxContext);
	void ParseTransportCmd(RxContext& rxContext);
	void ParseRawInputCmd(RxContext& rxContext);
	void ParseTrainControlCmd(RxContext& rxContext);
	void ParseModeListCmd(RxContext& rxContext);
	void SendToHub(MessageBuilder& msg);
	void SendAllChange();
	void SendEnableRequestsAllModes();
	void SendRenameControl(uint32 controlId, const FString& name);
	FString SelectActionMapFile();
	void ResetSelectActionMap(bool deleteFile = false);
	void LoadSelectActionMap();
	void SaveSelectActionMap();
	void SaveClassToModeMap();
	void ResetClassToModeMap();
	FString SelectModeMapFile();
	void RemoveLegacySelectModeMap();
	void SetCurrentMode(uint32 modeId);
	void AssociateSelectedActorAndMode(uint32 actionId);
	void SelectAssociatedActor(uint32 actionId);
	void OnActionTimer(uint32 actionId);
	uint32 ChooseModeForObjectType(UEngineHelper::ObjectType objectType);
	uint32 ChooseModeForObjectClass(const FName& className);
	void OnSelectionChanged();
	void OnActorPropertyChanged();
	void OnMenuString(uint32 targetId, FString &string);
	void OnParameterValue(uint32 targetId, float value);
	void OnCustomParameterValue(FName controlString, float value);
	void OnCustomMenuString(FName controlString, FString& string);
	void OnTrainResponse(int32 targetType, FName name, FName controlString, float min, float max, float stepSize);
	FString SettingsFile();
	void LoadSettings();
	void SaveSettings();
	void UpdateActionFilterOn();
	bool CheckActionFilterOff();
	void ResetActionFilter();
	bool IsTangentInputDeviceAvailable();
	uint32 ConvertCustomControlToTargetId(const FName& controlString);
	uint32 ConvertMode(const FName& modeName);
	FName ConvertMode(uint32 modeId);

private:
	FSocket*						_socket = nullptr;
	FThreadSafeBool					_threadToExit;
	FTimerDelegate					_actionTimerDelegate;
	TMap<uint32, FString>			_selectActionMap;
	ClassToModeMap					_classToModeMap;
	TMap<FName, uint32>				_customControlTargetIdMap;
	TMap<FName, uint32>				_modeList;
	uint32							_currentMode = ModeId::Default;
	int								_actionFilterCounter = 0;
	TSharedPtr<class FTangentInput> _tangentInputDevice;

	UPROPERTY()
	UEngineHelper*					_engineHelper = nullptr;
};
