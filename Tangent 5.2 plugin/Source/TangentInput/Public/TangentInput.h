// Tangent Panels Plugin for Unreal Editor
// Copyright 2023 Tangent Wave Ltd.
// SVN: $Revision: 309 $

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"
#include "IInputDevice.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"

// stores incoming data changes before sending them into UE for encoders
struct FEncoderData
{
	FEncoderData() : deltaAccumulator(0.0f), active(false) { }

	FKey	key;
	float	deltaAccumulator;
	bool	active;
};
 
// stores incoming data changes before sending them into UE for buttons
struct FButtonData
{
	FButtonData() : state(false), previousState(false), active(false) { }

	FKey	key;
	bool	state,
			previousState;
	bool	active;
};

// implements an input device to allow raw tangent panel control input
class TANGENTINPUT_API FTangentInput : public IInputDevice
{
public:
	FTangentInput(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);
	virtual ~FTangentInput();

	virtual void Tick(float DeltaTime) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override;
    virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override;

	void OnButtonChange(int buttonNumber, bool state);
	void OnEncoderChange(int encoderNumber, float delta);

private:
	TSharedRef<FGenericApplicationMessageHandler>	_messageHandler;
	TArray<FEncoderData>							_encoderData;
	TArray<FButtonData>								_buttonData;
	int												_activeEncoderDataCount,
													_activeButtonDataCount;
	FCriticalSection								_dataLock;
};

// plugin interface to support the input device implementation
class FTangentInputPlugin : public IInputDeviceModule
{
public:
	// helper accessor for instance
	TSharedPtr<class FTangentInput>& GetTangentInputDevice() { return _tangentInputDevice; }

	// boilerplate function to support the plugin module
	virtual TSharedPtr<class IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;

	// boilerplate function to support the plugin module
	static inline FName GetModularFeatureName()
	{
		static FName FeatureName = FName(TEXT("TangentInput"));
		return FeatureName;
	}

	// boilerplate function to support the plugin module
	static inline FTangentInputPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<FTangentInputPlugin>(GetModularFeatureName());
	}

	// boilerplate function to support the plugin module
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded(GetModularFeatureName());
	}

	// boilerplate function to support the plugin module
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<class FTangentInput> _tangentInputDevice;
};
