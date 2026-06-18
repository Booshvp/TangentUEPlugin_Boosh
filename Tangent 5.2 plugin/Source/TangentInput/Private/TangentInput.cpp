// Tangent Panels Plugin for Unreal Editor
// Copyright 2023 Tangent Wave Ltd.
// SVN: $Revision: 390 $

#include "TangentInput.h"
#include "IInputDeviceModule.h"
#include "IInputDevice.h"

#define LOCTEXT_NAMESPACE "TangentInputPlugin"

// hardcoded maximum number of encoders and button control keys and supporting data
#define TANGENT_MAX_ENCODERS	30
#define TANGENT_MAX_BUTTONS		30

//  names of inputs
struct FTangentInputKeyNames
{
	// encoders
	static const FName TangentInputEncoder01;
	static const FName TangentInputEncoder02;
	static const FName TangentInputEncoder03;
	static const FName TangentInputEncoder04;
	static const FName TangentInputEncoder05;
	static const FName TangentInputEncoder06;
	static const FName TangentInputEncoder07;
	static const FName TangentInputEncoder08;
	static const FName TangentInputEncoder09;
	static const FName TangentInputEncoder10;
	static const FName TangentInputEncoder11;
	static const FName TangentInputEncoder12;
	static const FName TangentInputEncoder13;
	static const FName TangentInputEncoder14;
	static const FName TangentInputEncoder15;
	static const FName TangentInputEncoder16;
	static const FName TangentInputEncoder17;
	static const FName TangentInputEncoder18;
	static const FName TangentInputEncoder19;
	static const FName TangentInputEncoder20;
	static const FName TangentInputEncoder21;
	static const FName TangentInputEncoder22;
	static const FName TangentInputEncoder23;
	static const FName TangentInputEncoder24;
	static const FName TangentInputEncoder25;
	static const FName TangentInputEncoder26;
	static const FName TangentInputEncoder27;
	static const FName TangentInputEncoder28;
	static const FName TangentInputEncoder29;
	static const FName TangentInputEncoder30;

	// buttons
	static const FName TangentInputButton01;
	static const FName TangentInputButton02;
	static const FName TangentInputButton03;
	static const FName TangentInputButton04;
	static const FName TangentInputButton05;
	static const FName TangentInputButton06;
	static const FName TangentInputButton07;
	static const FName TangentInputButton08;
	static const FName TangentInputButton09;
	static const FName TangentInputButton10;
	static const FName TangentInputButton11;
	static const FName TangentInputButton12;
	static const FName TangentInputButton13;
	static const FName TangentInputButton14;
	static const FName TangentInputButton15;
	static const FName TangentInputButton16;
	static const FName TangentInputButton17;
	static const FName TangentInputButton18;
	static const FName TangentInputButton19;
	static const FName TangentInputButton20;
	static const FName TangentInputButton21;
	static const FName TangentInputButton22;
	static const FName TangentInputButton23;
	static const FName TangentInputButton24;
	static const FName TangentInputButton25;
	static const FName TangentInputButton26;
	static const FName TangentInputButton27;
	static const FName TangentInputButton28;
	static const FName TangentInputButton29;
	static const FName TangentInputButton30;
};

// key instances for inputs
struct FTangentInputKeys
{
	// encoders
	static const FKey TangentInputEncoder01;
	static const FKey TangentInputEncoder02;
	static const FKey TangentInputEncoder03;
	static const FKey TangentInputEncoder04;
	static const FKey TangentInputEncoder05;
	static const FKey TangentInputEncoder06;
	static const FKey TangentInputEncoder07;
	static const FKey TangentInputEncoder08;
	static const FKey TangentInputEncoder09;
	static const FKey TangentInputEncoder10;
	static const FKey TangentInputEncoder11;
	static const FKey TangentInputEncoder12;
	static const FKey TangentInputEncoder13;
	static const FKey TangentInputEncoder14;
	static const FKey TangentInputEncoder15;
	static const FKey TangentInputEncoder16;
	static const FKey TangentInputEncoder17;
	static const FKey TangentInputEncoder18;
	static const FKey TangentInputEncoder19;
	static const FKey TangentInputEncoder20;
	static const FKey TangentInputEncoder21;
	static const FKey TangentInputEncoder22;
	static const FKey TangentInputEncoder23;
	static const FKey TangentInputEncoder24;
	static const FKey TangentInputEncoder25;
	static const FKey TangentInputEncoder26;
	static const FKey TangentInputEncoder27;
	static const FKey TangentInputEncoder28;
	static const FKey TangentInputEncoder29;
	static const FKey TangentInputEncoder30;

	// buttons
	static const FKey TangentInputButton01;
	static const FKey TangentInputButton02;
	static const FKey TangentInputButton03;
	static const FKey TangentInputButton04;
	static const FKey TangentInputButton05;
	static const FKey TangentInputButton06;
	static const FKey TangentInputButton07;
	static const FKey TangentInputButton08;
	static const FKey TangentInputButton09;
	static const FKey TangentInputButton10;
	static const FKey TangentInputButton11;
	static const FKey TangentInputButton12;
	static const FKey TangentInputButton13;
	static const FKey TangentInputButton14;
	static const FKey TangentInputButton15;
	static const FKey TangentInputButton16;
	static const FKey TangentInputButton17;
	static const FKey TangentInputButton18;
	static const FKey TangentInputButton19;
	static const FKey TangentInputButton20;
	static const FKey TangentInputButton21;
	static const FKey TangentInputButton22;
	static const FKey TangentInputButton23;
	static const FKey TangentInputButton24;
	static const FKey TangentInputButton25;
	static const FKey TangentInputButton26;
	static const FKey TangentInputButton27;
	static const FKey TangentInputButton28;
	static const FKey TangentInputButton29;
	static const FKey TangentInputButton30;
};

// define the names of the inputs, encoders...
const FName FTangentInputKeyNames::TangentInputEncoder01("TangentInput_Encoder01");
const FName FTangentInputKeyNames::TangentInputEncoder02("TangentInput_Encoder02");
const FName FTangentInputKeyNames::TangentInputEncoder03("TangentInput_Encoder03");
const FName FTangentInputKeyNames::TangentInputEncoder04("TangentInput_Encoder04");
const FName FTangentInputKeyNames::TangentInputEncoder05("TangentInput_Encoder05");
const FName FTangentInputKeyNames::TangentInputEncoder06("TangentInput_Encoder06");
const FName FTangentInputKeyNames::TangentInputEncoder07("TangentInput_Encoder07");
const FName FTangentInputKeyNames::TangentInputEncoder08("TangentInput_Encoder08");
const FName FTangentInputKeyNames::TangentInputEncoder09("TangentInput_Encoder09");
const FName FTangentInputKeyNames::TangentInputEncoder10("TangentInput_Encoder10");
const FName FTangentInputKeyNames::TangentInputEncoder11("TangentInput_Encoder11");
const FName FTangentInputKeyNames::TangentInputEncoder12("TangentInput_Encoder12");
const FName FTangentInputKeyNames::TangentInputEncoder13("TangentInput_Encoder13");
const FName FTangentInputKeyNames::TangentInputEncoder14("TangentInput_Encoder14");
const FName FTangentInputKeyNames::TangentInputEncoder15("TangentInput_Encoder15");
const FName FTangentInputKeyNames::TangentInputEncoder16("TangentInput_Encoder16");
const FName FTangentInputKeyNames::TangentInputEncoder17("TangentInput_Encoder17");
const FName FTangentInputKeyNames::TangentInputEncoder18("TangentInput_Encoder18");
const FName FTangentInputKeyNames::TangentInputEncoder19("TangentInput_Encoder19");
const FName FTangentInputKeyNames::TangentInputEncoder20("TangentInput_Encoder20");
const FName FTangentInputKeyNames::TangentInputEncoder21("TangentInput_Encoder21");
const FName FTangentInputKeyNames::TangentInputEncoder22("TangentInput_Encoder22");
const FName FTangentInputKeyNames::TangentInputEncoder23("TangentInput_Encoder23");
const FName FTangentInputKeyNames::TangentInputEncoder24("TangentInput_Encoder24");
const FName FTangentInputKeyNames::TangentInputEncoder25("TangentInput_Encoder25");
const FName FTangentInputKeyNames::TangentInputEncoder26("TangentInput_Encoder26");
const FName FTangentInputKeyNames::TangentInputEncoder27("TangentInput_Encoder27");
const FName FTangentInputKeyNames::TangentInputEncoder28("TangentInput_Encoder28");
const FName FTangentInputKeyNames::TangentInputEncoder29("TangentInput_Encoder29");
const FName FTangentInputKeyNames::TangentInputEncoder30("TangentInput_Encoder30");

// ... and buttons
const FName FTangentInputKeyNames::TangentInputButton01("TangentInput_Button01");
const FName FTangentInputKeyNames::TangentInputButton02("TangentInput_Button02");
const FName FTangentInputKeyNames::TangentInputButton03("TangentInput_Button03");
const FName FTangentInputKeyNames::TangentInputButton04("TangentInput_Button04");
const FName FTangentInputKeyNames::TangentInputButton05("TangentInput_Button05");
const FName FTangentInputKeyNames::TangentInputButton06("TangentInput_Button06");
const FName FTangentInputKeyNames::TangentInputButton07("TangentInput_Button07");
const FName FTangentInputKeyNames::TangentInputButton08("TangentInput_Button08");
const FName FTangentInputKeyNames::TangentInputButton09("TangentInput_Button09");
const FName FTangentInputKeyNames::TangentInputButton10("TangentInput_Button10");
const FName FTangentInputKeyNames::TangentInputButton11("TangentInput_Button11");
const FName FTangentInputKeyNames::TangentInputButton12("TangentInput_Button12");
const FName FTangentInputKeyNames::TangentInputButton13("TangentInput_Button13");
const FName FTangentInputKeyNames::TangentInputButton14("TangentInput_Button14");
const FName FTangentInputKeyNames::TangentInputButton15("TangentInput_Button15");
const FName FTangentInputKeyNames::TangentInputButton16("TangentInput_Button16");
const FName FTangentInputKeyNames::TangentInputButton17("TangentInput_Button17");
const FName FTangentInputKeyNames::TangentInputButton18("TangentInput_Button18");
const FName FTangentInputKeyNames::TangentInputButton19("TangentInput_Button19");
const FName FTangentInputKeyNames::TangentInputButton20("TangentInput_Button20");
const FName FTangentInputKeyNames::TangentInputButton21("TangentInput_Button21");
const FName FTangentInputKeyNames::TangentInputButton22("TangentInput_Button22");
const FName FTangentInputKeyNames::TangentInputButton23("TangentInput_Button23");
const FName FTangentInputKeyNames::TangentInputButton24("TangentInput_Button24");
const FName FTangentInputKeyNames::TangentInputButton25("TangentInput_Button25");
const FName FTangentInputKeyNames::TangentInputButton26("TangentInput_Button26");
const FName FTangentInputKeyNames::TangentInputButton27("TangentInput_Button27");
const FName FTangentInputKeyNames::TangentInputButton28("TangentInput_Button28");
const FName FTangentInputKeyNames::TangentInputButton29("TangentInput_Button29");
const FName FTangentInputKeyNames::TangentInputButton30("TangentInput_Button30");

// init the key instances of the inputs using the appropriate names, encoders...
const FKey FTangentInputKeys::TangentInputEncoder01(FTangentInputKeyNames::TangentInputEncoder01);
const FKey FTangentInputKeys::TangentInputEncoder02(FTangentInputKeyNames::TangentInputEncoder02);
const FKey FTangentInputKeys::TangentInputEncoder03(FTangentInputKeyNames::TangentInputEncoder03);
const FKey FTangentInputKeys::TangentInputEncoder04(FTangentInputKeyNames::TangentInputEncoder04);
const FKey FTangentInputKeys::TangentInputEncoder05(FTangentInputKeyNames::TangentInputEncoder05);
const FKey FTangentInputKeys::TangentInputEncoder06(FTangentInputKeyNames::TangentInputEncoder06);
const FKey FTangentInputKeys::TangentInputEncoder07(FTangentInputKeyNames::TangentInputEncoder07);
const FKey FTangentInputKeys::TangentInputEncoder08(FTangentInputKeyNames::TangentInputEncoder08);
const FKey FTangentInputKeys::TangentInputEncoder09(FTangentInputKeyNames::TangentInputEncoder09);
const FKey FTangentInputKeys::TangentInputEncoder10(FTangentInputKeyNames::TangentInputEncoder10);
const FKey FTangentInputKeys::TangentInputEncoder11(FTangentInputKeyNames::TangentInputEncoder11);
const FKey FTangentInputKeys::TangentInputEncoder12(FTangentInputKeyNames::TangentInputEncoder12);
const FKey FTangentInputKeys::TangentInputEncoder13(FTangentInputKeyNames::TangentInputEncoder13);
const FKey FTangentInputKeys::TangentInputEncoder14(FTangentInputKeyNames::TangentInputEncoder14);
const FKey FTangentInputKeys::TangentInputEncoder15(FTangentInputKeyNames::TangentInputEncoder15);
const FKey FTangentInputKeys::TangentInputEncoder16(FTangentInputKeyNames::TangentInputEncoder16);
const FKey FTangentInputKeys::TangentInputEncoder17(FTangentInputKeyNames::TangentInputEncoder17);
const FKey FTangentInputKeys::TangentInputEncoder18(FTangentInputKeyNames::TangentInputEncoder18);
const FKey FTangentInputKeys::TangentInputEncoder19(FTangentInputKeyNames::TangentInputEncoder19);
const FKey FTangentInputKeys::TangentInputEncoder20(FTangentInputKeyNames::TangentInputEncoder20);
const FKey FTangentInputKeys::TangentInputEncoder21(FTangentInputKeyNames::TangentInputEncoder21);
const FKey FTangentInputKeys::TangentInputEncoder22(FTangentInputKeyNames::TangentInputEncoder22);
const FKey FTangentInputKeys::TangentInputEncoder23(FTangentInputKeyNames::TangentInputEncoder23);
const FKey FTangentInputKeys::TangentInputEncoder24(FTangentInputKeyNames::TangentInputEncoder24);
const FKey FTangentInputKeys::TangentInputEncoder25(FTangentInputKeyNames::TangentInputEncoder25);
const FKey FTangentInputKeys::TangentInputEncoder26(FTangentInputKeyNames::TangentInputEncoder26);
const FKey FTangentInputKeys::TangentInputEncoder27(FTangentInputKeyNames::TangentInputEncoder27);
const FKey FTangentInputKeys::TangentInputEncoder28(FTangentInputKeyNames::TangentInputEncoder28);
const FKey FTangentInputKeys::TangentInputEncoder29(FTangentInputKeyNames::TangentInputEncoder29);
const FKey FTangentInputKeys::TangentInputEncoder30(FTangentInputKeyNames::TangentInputEncoder30);

// ... and buttons
const FKey FTangentInputKeys::TangentInputButton01(FTangentInputKeyNames::TangentInputButton01);
const FKey FTangentInputKeys::TangentInputButton02(FTangentInputKeyNames::TangentInputButton02);
const FKey FTangentInputKeys::TangentInputButton03(FTangentInputKeyNames::TangentInputButton03);
const FKey FTangentInputKeys::TangentInputButton04(FTangentInputKeyNames::TangentInputButton04);
const FKey FTangentInputKeys::TangentInputButton05(FTangentInputKeyNames::TangentInputButton05);
const FKey FTangentInputKeys::TangentInputButton06(FTangentInputKeyNames::TangentInputButton06);
const FKey FTangentInputKeys::TangentInputButton07(FTangentInputKeyNames::TangentInputButton07);
const FKey FTangentInputKeys::TangentInputButton08(FTangentInputKeyNames::TangentInputButton08);
const FKey FTangentInputKeys::TangentInputButton09(FTangentInputKeyNames::TangentInputButton09);
const FKey FTangentInputKeys::TangentInputButton10(FTangentInputKeyNames::TangentInputButton10);
const FKey FTangentInputKeys::TangentInputButton11(FTangentInputKeyNames::TangentInputButton11);
const FKey FTangentInputKeys::TangentInputButton12(FTangentInputKeyNames::TangentInputButton12);
const FKey FTangentInputKeys::TangentInputButton13(FTangentInputKeyNames::TangentInputButton13);
const FKey FTangentInputKeys::TangentInputButton14(FTangentInputKeyNames::TangentInputButton14);
const FKey FTangentInputKeys::TangentInputButton15(FTangentInputKeyNames::TangentInputButton15);
const FKey FTangentInputKeys::TangentInputButton16(FTangentInputKeyNames::TangentInputButton16);
const FKey FTangentInputKeys::TangentInputButton17(FTangentInputKeyNames::TangentInputButton17);
const FKey FTangentInputKeys::TangentInputButton18(FTangentInputKeyNames::TangentInputButton18);
const FKey FTangentInputKeys::TangentInputButton19(FTangentInputKeyNames::TangentInputButton19);
const FKey FTangentInputKeys::TangentInputButton20(FTangentInputKeyNames::TangentInputButton20);
const FKey FTangentInputKeys::TangentInputButton21(FTangentInputKeyNames::TangentInputButton21);
const FKey FTangentInputKeys::TangentInputButton22(FTangentInputKeyNames::TangentInputButton22);
const FKey FTangentInputKeys::TangentInputButton23(FTangentInputKeyNames::TangentInputButton23);
const FKey FTangentInputKeys::TangentInputButton24(FTangentInputKeyNames::TangentInputButton24);
const FKey FTangentInputKeys::TangentInputButton25(FTangentInputKeyNames::TangentInputButton25);
const FKey FTangentInputKeys::TangentInputButton26(FTangentInputKeyNames::TangentInputButton26);
const FKey FTangentInputKeys::TangentInputButton27(FTangentInputKeyNames::TangentInputButton27);
const FKey FTangentInputKeys::TangentInputButton28(FTangentInputKeyNames::TangentInputButton28);
const FKey FTangentInputKeys::TangentInputButton29(FTangentInputKeyNames::TangentInputButton29);
const FKey FTangentInputKeys::TangentInputButton30(FTangentInputKeyNames::TangentInputButton30);

// standard device constructor
FTangentInput::FTangentInput(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) : _messageHandler(InMessageHandler)
{
//	UE_LOG(LogTemp, Log, TEXT("TangentInput constructor"));

	// prep the control data arrays by reserving enough space and inserting the key instances, for both encoders...
	_encoderData.AddDefaulted(TANGENT_MAX_ENCODERS);
	_encoderData[0].key = FTangentInputKeys::TangentInputEncoder01;
	_encoderData[1].key = FTangentInputKeys::TangentInputEncoder02;
	_encoderData[2].key = FTangentInputKeys::TangentInputEncoder03;
	_encoderData[3].key = FTangentInputKeys::TangentInputEncoder04;
	_encoderData[4].key = FTangentInputKeys::TangentInputEncoder05;
	_encoderData[5].key = FTangentInputKeys::TangentInputEncoder06;
	_encoderData[6].key = FTangentInputKeys::TangentInputEncoder07;
	_encoderData[7].key = FTangentInputKeys::TangentInputEncoder08;
	_encoderData[8].key = FTangentInputKeys::TangentInputEncoder09;
	_encoderData[9].key = FTangentInputKeys::TangentInputEncoder10;
	_encoderData[10].key = FTangentInputKeys::TangentInputEncoder11;
	_encoderData[11].key = FTangentInputKeys::TangentInputEncoder12;
	_encoderData[12].key = FTangentInputKeys::TangentInputEncoder13;
	_encoderData[13].key = FTangentInputKeys::TangentInputEncoder14;
	_encoderData[14].key = FTangentInputKeys::TangentInputEncoder15;
	_encoderData[15].key = FTangentInputKeys::TangentInputEncoder16;
	_encoderData[16].key = FTangentInputKeys::TangentInputEncoder17;
	_encoderData[17].key = FTangentInputKeys::TangentInputEncoder18;
	_encoderData[18].key = FTangentInputKeys::TangentInputEncoder19;
	_encoderData[19].key = FTangentInputKeys::TangentInputEncoder20;
	_encoderData[20].key = FTangentInputKeys::TangentInputEncoder21;
	_encoderData[21].key = FTangentInputKeys::TangentInputEncoder22;
	_encoderData[22].key = FTangentInputKeys::TangentInputEncoder23;
	_encoderData[23].key = FTangentInputKeys::TangentInputEncoder24;
	_encoderData[24].key = FTangentInputKeys::TangentInputEncoder25;
	_encoderData[25].key = FTangentInputKeys::TangentInputEncoder26;
	_encoderData[26].key = FTangentInputKeys::TangentInputEncoder27;
	_encoderData[27].key = FTangentInputKeys::TangentInputEncoder28;
	_encoderData[28].key = FTangentInputKeys::TangentInputEncoder29;
	_encoderData[29].key = FTangentInputKeys::TangentInputEncoder30;

	// ... and buttons
	_buttonData.AddDefaulted(TANGENT_MAX_BUTTONS);
	_buttonData[0].key = FTangentInputKeys::TangentInputButton01;
	_buttonData[1].key = FTangentInputKeys::TangentInputButton02;
	_buttonData[2].key = FTangentInputKeys::TangentInputButton03;
	_buttonData[3].key = FTangentInputKeys::TangentInputButton04;
	_buttonData[4].key = FTangentInputKeys::TangentInputButton05;
	_buttonData[5].key = FTangentInputKeys::TangentInputButton06;
	_buttonData[6].key = FTangentInputKeys::TangentInputButton07;
	_buttonData[7].key = FTangentInputKeys::TangentInputButton08;
	_buttonData[8].key = FTangentInputKeys::TangentInputButton09;
	_buttonData[9].key = FTangentInputKeys::TangentInputButton10;
	_buttonData[10].key = FTangentInputKeys::TangentInputButton11;
	_buttonData[11].key = FTangentInputKeys::TangentInputButton12;
	_buttonData[12].key = FTangentInputKeys::TangentInputButton13;
	_buttonData[13].key = FTangentInputKeys::TangentInputButton14;
	_buttonData[14].key = FTangentInputKeys::TangentInputButton15;
	_buttonData[15].key = FTangentInputKeys::TangentInputButton16;
	_buttonData[16].key = FTangentInputKeys::TangentInputButton17;
	_buttonData[17].key = FTangentInputKeys::TangentInputButton18;
	_buttonData[18].key = FTangentInputKeys::TangentInputButton19;
	_buttonData[19].key = FTangentInputKeys::TangentInputButton20;
	_buttonData[20].key = FTangentInputKeys::TangentInputButton21;
	_buttonData[21].key = FTangentInputKeys::TangentInputButton22;
	_buttonData[22].key = FTangentInputKeys::TangentInputButton23;
	_buttonData[23].key = FTangentInputKeys::TangentInputButton24;
	_buttonData[24].key = FTangentInputKeys::TangentInputButton25;
	_buttonData[25].key = FTangentInputKeys::TangentInputButton26;
	_buttonData[26].key = FTangentInputKeys::TangentInputButton27;
	_buttonData[27].key = FTangentInputKeys::TangentInputButton28;
	_buttonData[28].key = FTangentInputKeys::TangentInputButton29;
	_buttonData[29].key = FTangentInputKeys::TangentInputButton30;

	// init to zero incoming data activity
	_activeEncoderDataCount = 0;
	_activeButtonDataCount = 0;
}

// standard device destructor
FTangentInput::~FTangentInput()
{
//	UE_LOG(LogTemp, Log, TEXT("TangentInput destructor"));
}

// tick handler, not used in this context
void FTangentInput::Tick(float DeltaTime)
{
	// n/a
}

// called by the main plugin when the hub reports a raw button state change
void FTangentInput::OnButtonChange(int buttonNumber, bool state)
{
	// guard against invalid data access
	if (buttonNumber < TANGENT_MAX_BUTTONS)
	{
		// lock the data before proceeding to get the data associated with this button
		_dataLock.Lock();
		FButtonData& bData = _buttonData[buttonNumber];

		// check for a state change
		if (bData.state != state)
		{
			// store the new state, we don't touch the previous state when writing here
			bData.state = state;
			
//			UE_LOG(LogTemp, Log, TEXT("TangentInput: OnButtonChange - button %d state is now %d"), buttonNumber, bData.state);

			// we check if this button is already marked as active as it is possible for several calls to come in here from the plugin/hub before it is
			// read out by the device when it is called to send in controller events, we count the number of actual buttons that have changed to optimise
			// the reading so only flag and count this call if appropriate
			if (bData.active == false)
			{
				// a new state change, flag and count it as an active button
				bData.active = true;
				_activeButtonDataCount++;
			}
		}

		// unlock
		_dataLock.Unlock();
	}
}

// called by the main plugin when the hub reports a raw encoder delta change
void FTangentInput::OnEncoderChange(int encoderNumber, float delta)
{
	// guard against invalid data access
	if (encoderNumber < TANGENT_MAX_ENCODERS)
	{
		// lock the data before proceeding to get the data associated with this encoder
		_dataLock.Lock();
		FEncoderData& eData = _encoderData[encoderNumber];

		// add the incoming data to the accumulator, this allows for multiple writes before it can be read out
		eData.deltaAccumulator += delta;

//		UE_LOG(LogTemp, Log, TEXT("TangentInput: OnEncoderChange - encoder %d delta is now %f"), encoderNumber, eData.deltaAccumulator);

		// we check if this encoder is already marked as active as it is possible for several calls to come in here from the plugin/hub before it is
		// read out by the device when it is called to send in controller events, we count the number of actual encoder changes to optimise the reading
		// so only flag and count this call if appropriate
		if (eData.active == false)
		{
			// a new encoder change, flag and count it
			eData.active = true;
			_activeEncoderDataCount++;
		}

		// unlock
		_dataLock.Unlock();
	}
}

// periodically called to poll for input events from this device and to dispatch them into UE
void FTangentInput::SendControllerEvents()
{
	// lock the data before proceeding
	_dataLock.Lock();

	// we have a master counter for active buttons, if this is zero we have nothing more to do, a button is considered active on any state change
	// and remains in the active set while pressed to be able to be processed as a potential repeat
	if (_activeButtonDataCount > 0)
	{
		int index = 0,
			processedDataCount = 0;

//		UE_LOG(LogTemp, Warning, TEXT("----"));

		// we use the default values for user and device ids until we know better
		FPlatformUserId userId = IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser();
		FInputDeviceId deviceId = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();

		// loop through all the buttons up to the maximum, also tracking the outstanding counter for activity, this allows us
		// to exit the button loop early if we know we have processed all the registered activity
		while ((processedDataCount <_activeButtonDataCount) && (index < TANGENT_MAX_BUTTONS))
		{
//			UE_LOG(LogTemp, Warning, TEXT("IN : index is %d"), index);
//			UE_LOG(LogTemp, Warning, TEXT("IN : >>>> %d button(s) active"), _activeButtonDataCount);
//			UE_LOG(LogTemp, Warning, TEXT("IN : >>>> %d button(s) processed"), processedDataCount);
			// check if this button index data is one that has activity
			FButtonData& bData = _buttonData[index];
			if (bData.active)
			{
				// this button is marked as active, check the current state against the previously reported store
				if (bData.state != bData.previousState)
				{
					// there is a state change, it may be for a press or release
					if (bData.state)
					{
						// a move to the active state so this is a button press
//						UE_LOG(LogTemp, Warning, TEXT("TangentInput: ++++ PRESS"));
						_messageHandler->OnControllerButtonPressed(bData.key.GetFName(), userId, deviceId, false);
						
						// presses are counted as a processed item to test for early loop exit, the button remains in the active pool
						processedDataCount++;

						// TODO - save time for repeat processing?
					}
					else
					{
						// a move to the inactive state so this is a button release
//						UE_LOG(LogTemp, Warning, TEXT("TangentInput: ---- RELEASE"));
						_messageHandler->OnControllerButtonReleased(bData.key.GetFName(), userId, deviceId, false);	

						// this button is now off so remove its active state and count
						// this button is now released so remove it from the active pool, note that this is not counted as a processed item
						// as we are adjusting the active pool count rather than the processed count
						// TODO - clear repeat flag
						bData.active = false;
						_activeButtonDataCount--;
					}

					// copy the current state to the previous state store for next time
					bData.previousState = bData.state;
				}
				// there is no state change but still marked as active, this may be due to multiple inputs from the plugin/hub before this function was
				// called to handle it, if the current state is pressed then this will be treated as a repeating press, if not then it is done with
				else if (bData.state)
				{
					// not a state change but still in the pressed state so this is a repeating button press

					// TODO - check time since first press, if pinged then set as repeat, update time
//					UE_LOG(LogTemp, Warning, TEXT("TangentInput: ++++ REPEAT"));
					_messageHandler->OnControllerButtonPressed(bData.key.GetFName(), userId, deviceId, true);	

					// repeats are counted as a processed item to test for early loop exit, the button remains in the active pool
					processedDataCount++;
				}
				else
				{
					// not a state change but it is already released so remove it from the active pool, note that this is not counted as a processed item
					// as we are adjusting the active pool count rather than the processed count
					// TODO - clear repeat
//					UE_LOG(LogTemp, Warning, TEXT("TangentInput: ---- INACTIVE"));
					bData.active = false;
					_activeButtonDataCount--;
				}
			}

			// try the next button 
			index++;

//			UE_LOG(LogTemp, Warning, TEXT("OUT: >>>> %d button(s) active"), _activeButtonDataCount);
//			UE_LOG(LogTemp, Warning, TEXT("OUT: >>>> %d button(s) processed"), processedDataCount);
		}
	}

	// we have a master counter for each individual encoder data change, if this is zero we have nothing more to do, an encoder is considered
	// active if there is an outstanding delta to be applied, once a delta is processed it can be removed from the active pool
	if (_activeEncoderDataCount > 0)
	{
		int index = 0;

		// we use the default values for user and device ids until we know better
		FPlatformUserId userId = IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser();
		FInputDeviceId deviceId = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();

		// loop through all the encoders up to the maximum, also tracking the outstanding counter for activity, this allows us
		// to exit the encoder loop early if we know we have processed all the registered changes
		while ((_activeEncoderDataCount > 0) && (index < TANGENT_MAX_ENCODERS))
		{
			// check if this encoder index data is one that is active
			FEncoderData& eData = _encoderData[index];
			if (eData.active)
			{
				// this encoder is marked as active, send in the accumulated value
//				UE_LOG(LogTemp, Warning, TEXT("TangentInput: ++++ DELTA %f"), eData.deltaAccumulator);
				_messageHandler->OnControllerAnalog(eData.key.GetFName(), userId, deviceId, eData.deltaAccumulator);

				// now.. if we've just sent in non-zero device delta value, it seems that unreal interprets this as an ongoing device controller
				// thing and so will carry on adjussting by this value until a new delta is sent in, spinning away... to allow for this for now
				// we will follow up with a zero delta, keeping the encoder active until we send in the stopping zero value
				if (eData.deltaAccumulator != 0.0f)
				{
					// queue up a still-active but now zero input
					eData.deltaAccumulator = 0.0f;
				}
				else
				{
					// we have consumed this activity, reset the accumulator, clear the flag and count it out	
					eData.deltaAccumulator = 0.0f;
					eData.active = false;
					_activeEncoderDataCount--;
				}
			}

			// try the next encoder
			index++;
		}
	}

	// unlock
	_dataLock.Unlock();
}

// message handler accessor
void FTangentInput::SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
{
	// store the new instance
	_messageHandler = InMessageHandler;
}

// called to run command within this instance, not used in this context
bool FTangentInput::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// n/a so always return false
	return false;
}

// force feedback rountine, not used
void FTangentInput::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
	// n/a
}

// force feedback rountine, not used
void FTangentInput::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values)
{
	// n/a
}

// plugin level boilerplate function to create a new input device instance
TSharedPtr<class IInputDevice> FTangentInputPlugin::CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	// we keep a local handle to the new instance for easy access
	_tangentInputDevice = MakeShareable(new FTangentInput(InMessageHandler)); 
//	UE_LOG(LogTemp, Log, TEXT("TangentInput CreateInputDevice()"));
	return _tangentInputDevice;
}

// plugin level boilerplate function to kick off the plugin session
void FTangentInputPlugin::StartupModule()
{
//	UE_LOG(LogTemp, Log, TEXT("TangentInput plugin Startup..."));

	// this is the category name that the keys are stored under in the binding menu in project settings engine inputs
	const FName menuCategoryName(TEXT("TangentRawInput"));

	// create our menu category passing in a user friendly display name and icon
	EKeys::AddMenuCategoryDisplayInfo(menuCategoryName, LOCTEXT("TangentRawInputSubCategory", "Tangent Raw Input"), TEXT("GraphEditor.KeyEvent_16x"));

	// create and insert the binding key contents for both encoders...
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder01, LOCTEXT("TangentInputEncoder01", "Tangent Encoder 1"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder02, LOCTEXT("TangentInputEncoder02", "Tangent Encoder 2"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder03, LOCTEXT("TangentInputEncoder03", "Tangent Encoder 3"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder04, LOCTEXT("TangentInputEncoder04", "Tangent Encoder 4"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder05, LOCTEXT("TangentInputEncoder05", "Tangent Encoder 5"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder06, LOCTEXT("TangentInputEncoder06", "Tangent Encoder 6"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder07, LOCTEXT("TangentInputEncoder07", "Tangent Encoder 7"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder08, LOCTEXT("TangentInputEncoder08", "Tangent Encoder 8"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder09, LOCTEXT("TangentInputEncoder09", "Tangent Encoder 9"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder10, LOCTEXT("TangentInputEncoder10", "Tangent Encoder 10"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder11, LOCTEXT("TangentInputEncoder11", "Tangent Encoder 11"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder12, LOCTEXT("TangentInputEncoder12", "Tangent Encoder 12"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder13, LOCTEXT("TangentInputEncoder13", "Tangent Encoder 13"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder14, LOCTEXT("TangentInputEncoder14", "Tangent Encoder 14"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder15, LOCTEXT("TangentInputEncoder15", "Tangent Encoder 15"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder16, LOCTEXT("TangentInputEncoder16", "Tangent Encoder 16"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder17, LOCTEXT("TangentInputEncoder17", "Tangent Encoder 17"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder18, LOCTEXT("TangentInputEncoder18", "Tangent Encoder 18"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder19, LOCTEXT("TangentInputEncoder19", "Tangent Encoder 19"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder20, LOCTEXT("TangentInputEncoder20", "Tangent Encoder 20"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder21, LOCTEXT("TangentInputEncoder21", "Tangent Encoder 21"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder22, LOCTEXT("TangentInputEncoder22", "Tangent Encoder 22"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder23, LOCTEXT("TangentInputEncoder23", "Tangent Encoder 23"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder24, LOCTEXT("TangentInputEncoder24", "Tangent Encoder 24"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder25, LOCTEXT("TangentInputEncoder25", "Tangent Encoder 25"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder26, LOCTEXT("TangentInputEncoder26", "Tangent Encoder 26"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder27, LOCTEXT("TangentInputEncoder27", "Tangent Encoder 27"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder28, LOCTEXT("TangentInputEncoder28", "Tangent Encoder 28"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder29, LOCTEXT("TangentInputEncoder29", "Tangent Encoder 29"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputEncoder30, LOCTEXT("TangentInputEncoder30", "Tangent Encoder 30"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D, menuCategoryName));

	// ... and buttons
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton01, LOCTEXT("TangentInputButton01", "Tangent Button 1"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton02, LOCTEXT("TangentInputButton02", "Tangent Button 2"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton03, LOCTEXT("TangentInputButton03", "Tangent Button 3"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton04, LOCTEXT("TangentInputButton04", "Tangent Button 4"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton05, LOCTEXT("TangentInputButton05", "Tangent Button 5"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton06, LOCTEXT("TangentInputButton06", "Tangent Button 6"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton07, LOCTEXT("TangentInputButton07", "Tangent Button 7"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton08, LOCTEXT("TangentInputButton08", "Tangent Button 8"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton09, LOCTEXT("TangentInputButton09", "Tangent Button 9"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton10, LOCTEXT("TangentInputButton10", "Tangent Button 10"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton11, LOCTEXT("TangentInputButton11", "Tangent Button 11"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton12, LOCTEXT("TangentInputButton12", "Tangent Button 12"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton13, LOCTEXT("TangentInputButton13", "Tangent Button 13"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton14, LOCTEXT("TangentInputButton14", "Tangent Button 14"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton15, LOCTEXT("TangentInputButton15", "Tangent Button 15"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton16, LOCTEXT("TangentInputButton16", "Tangent Button 16"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton17, LOCTEXT("TangentInputButton17", "Tangent Button 17"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton18, LOCTEXT("TangentInputButton18", "Tangent Button 18"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton19, LOCTEXT("TangentInputButton19", "Tangent Button 19"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton20, LOCTEXT("TangentInputButton20", "Tangent Button 20"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton21, LOCTEXT("TangentInputButton21", "Tangent Button 21"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton22, LOCTEXT("TangentInputButton22", "Tangent Button 22"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton23, LOCTEXT("TangentInputButton23", "Tangent Button 23"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton24, LOCTEXT("TangentInputButton24", "Tangent Button 24"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton25, LOCTEXT("TangentInputButton25", "Tangent Button 25"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton26, LOCTEXT("TangentInputButton26", "Tangent Button 26"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton27, LOCTEXT("TangentInputButton27", "Tangent Button 27"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton28, LOCTEXT("TangentInputButton28", "Tangent Button 28"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton29, LOCTEXT("TangentInputButton29", "Tangent Button 29"), FKeyDetails::GamepadKey, menuCategoryName));
	EKeys::AddKey(FKeyDetails(FTangentInputKeys::TangentInputButton30, LOCTEXT("TangentInputButton30", "Tangent Button 30"), FKeyDetails::GamepadKey, menuCategoryName));

	// register our input device module with the engine, this allows an instance of the device to be created and the various functions to be called when required
	IModularFeatures::Get().RegisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);
}

// plugin level boilerplate function to end a session
void FTangentInputPlugin::ShutdownModule()
{
//	UE_LOG(LogTemp, Log, TEXT("TangentInput plugin Shutdown..."));

	// to tidy up we need to unregister the input device
	IModularFeatures::Get().UnregisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);
}

IMPLEMENT_MODULE(FTangentInputPlugin, TangentInput)

#undef LOCTEXT_NAMESPACE
