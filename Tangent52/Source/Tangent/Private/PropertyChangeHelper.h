// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 382 $

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

// interface class that is used for property value changes that need to be wrapped in pre and post change operations to trigger updates
class PropertyChangeHelper
{
public:
	virtual ~PropertyChangeHelper() {}
	
	virtual void OnPreChange(AActor *actor, FProperty *property, const FString& description = "Edit Property") = 0;
	virtual void OnPostChange(AActor* actor, FProperty *property) = 0;
};
