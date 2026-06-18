// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 382 $

#pragma once

#include "CoreMinimal.h"
#include "PropertyChangeHelper.h"

// property data store that we use to have all useful information for a ue property to hand for adjusting property path based values
class PropertyData
{
public:
	PropertyData(const FName propertyPath, FProperty* prop, void* dataPtr, void* defaultDataPtr) : PropertyPath(propertyPath), Prop(prop), DataPtr(dataPtr), DefaultDataPtr(defaultDataPtr), Clipped(false), Minimum(0.0), Maximum(0.0) {};

	bool AdjustParameterValue(PropertyChangeHelper* helper, AActor* actor, uint32 targetId, float delta, float &newValue);
	bool GetParameterValue(float& value);

	bool SetMenuValue(PropertyChangeHelper* helper, AActor* actor, bool value, FString& newStrValue);
	bool AdjustMenuValue(PropertyChangeHelper* helper, AActor* actor, int32 step, FString& newStrValue);
	bool GetMenuValueAsString(FString& strValue);

public:
	FName		PropertyPath;
	FProperty*	Prop;
	void*		DataPtr;
	void*		DefaultDataPtr;
	bool		Clipped;
	double		Minimum,
				Maximum;
};
