// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 381 $

#pragma once

#include "CoreMinimal.h"
#include "Common.h"

// class used to handle receiving messages from the hub with some useful functions for parsing them
class RxContext
{
public:

	RxContext();

	FVector		MoveDeltaCache;
	FRotator	RotateDeltaCache;
	FVector		ScaleDeltaCache;

	uint32		MoveResetCache;
	uint32		RotateResetCache;
	uint32		ScaleResetCache;

	static const uint32 LocationRequest;
	static const uint32 RotationRequest;
	static const uint32 ScaleRequest;
	void FlagValueRequest(uint32 request);
	bool IsValueRequested(uint32 request);

	static const uint32 AxisX;
	static const uint32 AxisY;
	static const uint32 AxisZ;
	void FlagMoveReset(uint32 axis);
	void FlagRotateReset(uint32 axis);
	void FlagScaleReset(uint32 axis);

	bool WaitingForData();
	bool ProcessingHeader();
	void GetReceiveBuffer(uint8* &buffer, int32 &bytesExpected);
	void AdjustForDataReceived(uint32 bytesReceived);

	bool HasCommandToRead();
	uint32 ReadUInt32();
	uint32 ReadInt32();
	float ReadFloat32();
	FName ReadString();
	void Skip(uint32 bytesToSkip);

	void ResetForHeader();
	void ResetForMessage(uint32 messsageLen);
	void ResetCaches();

private:
	uint32		_valueRequestCache;					// bit-wise cache for value requests for location, rotation and scale
	bool		_rxHeader;							// true when waiting to receive a message header
	int32		_readIndex;							// keeps track of where in the buffer we are to next read out a value
	int32		_bufferLen,							// number of bytes expected for the buffer contents
				_bytesRemaining;					// number of bytes still outstanding
	uint8		_buffer[IPCComms::MaxMessageLen];	// a received message is either a header or message payload, the message is the larger
};
