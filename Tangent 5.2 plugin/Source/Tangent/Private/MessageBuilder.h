// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 377 $

#pragma once

#include "CoreMinimal.h"
#include "Common.h"

// class to help build up messages to be sent to the hub
class MessageBuilder
{
public:
	MessageBuilder();

	void Reset();
	void WriteUInt32(uint32 value);
	void WriteInt32(int32 value);
	void WriteFloat32(float value);
	void WriteString(const char* string = nullptr);
	void WriteString(const FString& string);
	void WriteString(const FName string);
	void Build();
	bool GetMessage(uint8* &msg, int32 &msgLen);

private:
	bool	_built;															// flag that is set only when a final call to build the message has been made and is ready to send
	uint8	_message[IPCComms::HeaderLen + IPCComms::MaxMessageLen];		// an outgoing message will contain both the header and payload for a message
	int32	_messageLen;													// total length of message to be sent when built (note this does not include the header bytes per design)
	int32	_writeIndex;													// tracks the buffer index where we will write the next value
};
