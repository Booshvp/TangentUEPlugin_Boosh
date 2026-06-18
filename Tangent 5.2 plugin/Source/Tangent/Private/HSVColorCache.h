// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 264 $

#pragma once

#include "CoreMinimal.h"
#include "Engine/Light.h"

/**
 * 
 */
class HSVColorCache
{
public:
	HSVColorCache();
	void getHSV(ALight *light, FLinearColor &color);
	void setHSV(FLinearColor &color);

private:
	FDateTime		_timestamp;
	FLinearColor	_colorCache;
};
