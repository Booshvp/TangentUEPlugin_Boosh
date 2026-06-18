// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 377 $

#pragma once

#include "CoreMinimal.h"

// ipc protocol settings
class IPCComms
{
public:
	static constexpr uint32 MaxMessageLen = 1024;		// maximum size of a message payload, this does not include the header prefix
	static constexpr uint32 HeaderLen = 4;				// size of the header used to precede a message payload to define the payload length
	static constexpr int32 Port = 64246;				// preset port used to connect to the tangent hub
};
