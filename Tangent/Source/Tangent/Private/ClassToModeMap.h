// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 371 $

#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"

// convenience map class for fname instances that can save/load as user friendly text to a file
class ClassToModeMap : public TMap<FName, FName>
{
public:
	ClassToModeMap();

	void SaveToFile();
	void ResetToDefault();

private:
	FString SavedFileName();
	void InsertDefaultMapData();
	void LoadFromFile();
};
