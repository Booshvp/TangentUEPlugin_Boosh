// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 382 $

#include "PropertyData.h"
#include "ResetCache.h"

// helper with property based parameter value changes,  must only be used in the game thread by the caller, if the delta is zero 
// the value is reset, the associated actor instance is required to trigger editor ui updates, returns true if the value was modified
bool PropertyData::AdjustParameterValue(PropertyChangeHelper* helper, AActor* actor, uint32 targetId, float delta, float &newValue)
{
	// prepare a text description for the transaction that will be used if a new one is required, using the user friendly property name
	FString description = "Edit " + Prop->GetDisplayNameText().ToString();

	// try to convert the property to a float type
	if (FFloatProperty* floatProp = CastField<FFloatProperty>(Prop))
	{
		double value;

		// a zero delta triggers a reset to default
		if (delta == 0)
		{
			// the reset action was registered when it first came in, check now if this is a single or double tap, a single tap may reset
			// to a cached value from before any changes were made if that is available - if no cached value is available we do nothing, a double
			// tap will always reset to the default taken from the property itself
			if (ResetCache::IsSingleTap(targetId))
			{
				// so this is a single tap reset, check for a cached value to apply for this target and actor, if we get one we will apply it as the new value
				if (!ResetCache::GetValue(targetId, actor, &value))
				{
					// there is no pre-change cached value to use so simply return as there is nothing more to do 
					return false;
				}
			}
			else
			{ 
				// fetch the default value to be applied and passed back as the reset
				value = floatProp->GetFloatingPointPropertyValue(DefaultDataPtr);
			}
		}
		else
		{
			// fetch the current value
			value = floatProp->GetFloatingPointPropertyValue(DataPtr);

			// we store the first pre-change value as a potential single tap reset in a cache, pass it to the cache singleton which will only
			// store it if we haven't already cached a value for this target and actor combo
			ResetCache::CacheInitialValue(targetId, actor, value);

			// now we can add the given delta to get the new value to apply
			value += delta;

			// optionally apply clipping to the value here
			if (Clipped)
			{
				value = (value < Minimum)? Minimum: value;
				value = (value > Maximum)? Maximum: value;
			}
		}

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// now we can set the new property value
		floatProp->SetFloatingPointPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// we supply the caller with the new value, we get it rather than use our calculated one in case of rounding or clipping in the editor
		newValue = (float) floatProp->GetFloatingPointPropertyValue(DataPtr);

		// return with success
		return true;
	}
	// we support double properties
	else if (FDoubleProperty* doubleProp = CastField<FDoubleProperty>(Prop))
	{
		double value;

		// a zero delta triggers a reset to default
		if (delta == 0)
		{
			// the reset action was registered when it first came in, check now if this is a single or double tap, a single tap may reset
			// to a cached value from before any changes were made if that is available - if no cached value is available we do nothing, a double
			// tap will always reset to the default taken from the property itself
			if (ResetCache::IsSingleTap(targetId))
			{
				// so this is a single tap reset, check for a cached value to apply for this target and actor, if we get one we will apply it as the new value
				if (!ResetCache::GetValue(targetId, actor, &value))
				{
					// there is no pre-change cached value to use so simply return as there is nothing more to do 
					return false;
				}
			}
			else
			{ 
				// fetch the default value to be applied and passed back as the reset
				value = doubleProp->GetFloatingPointPropertyValue(DefaultDataPtr);
			}
		}
		else
		{
			// fetch the current value
			value = doubleProp->GetFloatingPointPropertyValue(DataPtr);

			// we store the first pre-change value as a potential single tap reset in a cache, pass it to the cache singleton which will only
			// store it if we haven't already cached a value for this target and actor combo
			ResetCache::CacheInitialValue(targetId, actor, value);

			// now we can add the given delta to get the new value to apply
			value += delta;

			// optionally apply clipping to the value here
			if (Clipped)
			{
				value = (value < Minimum)? Minimum: value;
				value = (value > Maximum)? Maximum: value;
			}
		}

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// now we can set the new property value
		doubleProp->SetFloatingPointPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// we supply the caller with the new value, we get it rather than use our calculated one in case of rounding or clipping in the editor
		newValue = (float) doubleProp->GetFloatingPointPropertyValue(DataPtr);

		// return with success
		return true;
	}
	// we support unsigned byte properties
	else if (FByteProperty* byteProp = CastField<FByteProperty>(Prop))
	{
		uint64 value;

		// a zero delta triggers a reset to default
		if (delta == 0)
		{
			// the reset action was registered when it first came in, check now if this is a single or double tap, a single tap may reset
			// to a cached value from before any changes were made if that is available - if no cached value is available we do nothing, a double
			// tap will always reset to the default taken from the property itself
			if (ResetCache::IsSingleTap(targetId))
			{
				// so this is a single tap reset, check for a cached value to apply for this target and actor, if we get one we will apply it as the new value
				if (!ResetCache::GetValue(targetId, actor, &value))
				{
					// there is no pre-change cached value to use so simply return as there is nothing more to do 
					return false;
				}
			}
			else
			{ 
				// fetch the default value to be applied and passed back as the reset
				value = byteProp->GetUnsignedIntPropertyValue(DefaultDataPtr);
			}
		}
		else
		{
			// fetch the current value
			value = byteProp->GetUnsignedIntPropertyValue(DataPtr);

			// we store the first pre-change value as a potential single tap reset in a cache, pass it to the cache singleton which will only
			// store it if we haven't already cached a value for this target and actor combo
			ResetCache::CacheInitialValue(targetId, actor, value);

			// to guard against going 'negative' we have an intermin signed value as byte property values are bound to the range of 0 to 255
			int64 sValue = (int64) value;

			// now we can add the given delta to get the new value to apply
			sValue += delta;

			// apply a fixed range to the unsigned byte value here to prevent wraparound
			value = (sValue < 0) ? 0 : sValue;
			value = (value > 255)? 255: value;

			// now optionally apply clipping to the value here
			if (Clipped)
			{
				value = (value < (uint64) Minimum)? (uint64) Minimum: value;
				value = (value > (uint64) Maximum)? (uint64) Maximum: value;
			}
		}

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// now we can set the new property value
		byteProp->SetIntPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// we supply the caller with the new value, we get it rather than use our calculated one in case of rounding or clipping in the editor
		newValue = (float) byteProp->GetUnsignedIntPropertyValue(DataPtr);

		// return with success
		return true;
	}
	// we support signed 32-bit int properties
	else if (FIntProperty* intProp = CastField<FIntProperty>(Prop))
	{
		int64 value;

		// a zero delta triggers a reset to default
		if (delta == 0)
		{
			// the reset action was registered when it first came in, check now if this is a single or double tap, a single tap may reset
			// to a cached value from before any changes were made if that is available - if no cached value is available we do nothing, a double
			// tap will always reset to the default taken from the property itself
			if (ResetCache::IsSingleTap(targetId))
			{
				// so this is a single tap reset, check for a cached value to apply for this target and actor, if we get one we will apply it as the new value
				if (!ResetCache::GetValue(targetId, actor, &value))
				{
					// there is no pre-change cached value to use so simply return as there is nothing more to do 
					return false;
				}
			}
			else
			{ 
				// fetch the default value to be applied and passed back as the reset
				value = intProp->GetSignedIntPropertyValue(DefaultDataPtr);
			}
		}
		else
		{
			// fetch the current value
			value = intProp->GetSignedIntPropertyValue(DataPtr);

			// we store the first pre-change value as a potential single tap reset in a cache, pass it to the cache singleton which will only
			// store it if we haven't already cached a value for this target and actor combo
			ResetCache::CacheInitialValue(targetId, actor, value);

			// now we can add the given delta to get the new value to apply
			value += delta;

			// optionally apply clipping to the value here
			if (Clipped)
			{
				value = (value < (int64) Minimum)? (int64) Minimum: value;
				value = (value > (int64) Maximum)? (int64) Maximum: value;
			}
		}

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// now we can set the new property value
		intProp->SetIntPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// we supply the caller with the new value, we get it rather than use our calculated one in case of rounding or clipping in the editor
		newValue = (float) intProp->GetSignedIntPropertyValue(DataPtr);

		// return with success
		return true;
	}

	// getting here means the property is not a float value, return to say we did not change the value
	return false;
}

// helper with property based parameter values, returns true if the value was available and passed back
bool PropertyData::GetParameterValue(float &value)
{
	// try to convert the property to a float type
	if (FFloatProperty* floatProp = CastField<FFloatProperty>(Prop))
	{
		// fetch the current value and pass it back with success
		value = (float) floatProp->GetFloatingPointPropertyValue(DataPtr);
		return true;
	}
	// we support double properties
	else if (FDoubleProperty* doubleProp = CastField<FDoubleProperty>(Prop))
	{
		// fetch the current value and pass it back with success
		value = (float) doubleProp->GetFloatingPointPropertyValue(DataPtr);
		return true;
	}
	// we support unsigned byte properties
	else if (FByteProperty* byteProp = CastField<FByteProperty>(Prop))
	{
		// fetch the current value as a unsigned int as that is what byte properties support
		value = (float) byteProp->GetUnsignedIntPropertyValue(DataPtr);
		return true;
	}
	// we support signed 32-bit int properties
	else if (FIntProperty* intProp = CastField<FIntProperty>(Prop))
	{
		// fetch the current value as a signed int and pass it back with success
		value = (float) intProp->GetSignedIntPropertyValue(DataPtr);
		return true;
	}

	// getting here means the value is not available
	return false;
}

// special case helper to set a specific boolean property value, must only be used in the game thread by the caller, if the value
// is changed the function returns true and the new string version is passed back
bool PropertyData::SetMenuValue(PropertyChangeHelper* helper, AActor* actor, bool value, FString& newStrValue)
{
	// prepare a text description for the transaction that will be used if a new one is required, using the user friendly property name
	FString description = "Edit " + Prop->GetDisplayNameText().ToString();

	// this special case function requires a boolean property
	if (FBoolProperty* boolProp = CastField<FBoolProperty>(Prop))
	{
		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// apply the new state
		boolProp->SetPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// return a value string based on the boolean state
		newStrValue = value? FString("On"): FString("Off");
		return true;
	}

	// getting here means the value is not available
	return false;
}

// changes the menu property value by the required step value +/-1, if the step is 0 the value is reset, must only be used in the game thread
// by the caller, if the value is changed the function returns true and the new string version is passed back
bool PropertyData::AdjustMenuValue(PropertyChangeHelper* helper, AActor* actor, int32 step, FString& newStrValue)
{
	FNumericProperty* numProp = nullptr;
	UEnum* enumData = nullptr;

	// prepare a text description for the transaction that will be used if a new one is required, using the user friendly property name
	FString description = "Edit " + Prop->GetDisplayNameText().ToString();

	// we support boolean properties as a toggle, also enumerated values via the enum property type and also the byte property
	// type, they share a lot of the same processing but with some slight variation in set up
	if (FBoolProperty* boolProp = CastField<FBoolProperty>(Prop))
	{
		// fetch the bool value and toggle it
		bool value = !boolProp->GetPropertyValue(DataPtr);

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// apply the new state
		boolProp->SetPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// return a value string based on the boolean state
		newStrValue = value? FString("On"): FString("Off");
		return true;
	}
	else if (FEnumProperty* enumProp = CastField<FEnumProperty>(Prop))
	{
		// fetch the enum data to process
		enumData = enumProp->GetEnum();

		// enum properties are linked to another property that has the actual value
		numProp = enumProp->GetUnderlyingProperty();
	}
	else if (FByteProperty* byteProp = CastField<FByteProperty>(Prop))
	{
		// fetch the enum data to process
		enumData = byteProp->Enum;

		// the byte prop itself is the target property in this context
		numProp = byteProp;
	}

	// if the casting was valid and we have the required data we can continue with the adjustment of the value for enumerated types
	if (numProp && enumData)
	{
		int64 value;

		// a zero step triggers a reset, otherwise the enumerated value is adjusted
		if (step == 0)
		{
			// this is a reset call, prepare the default value
			value = numProp->GetSignedIntPropertyValue(DefaultDataPtr);
		}
		else
		{
			// this is a step adjusted call, start with the current value
			value = numProp->GetSignedIntPropertyValue(DataPtr);

			// convert the value to the enumeration index and fetch the number of enumeration defined values
			int32	index = enumData->GetIndexByValue(value),
					count = enumData->NumEnums();

			// in unreal it is standard to automatically append a special case MAX value to the defined data, if this is the case then
			// we need to discount it from the index range we use for wrapping at the boundary steps
			if (enumData->ContainsExistingMax())
			{
				// this enum has a max definition so decrement the count for index range handling
				count--;
			}

			// modify the index of the value by the correct step direction
			if (step > 0)
			{
				// step up one index allowing for reaching the end of the definitions in which case we wrap round to the first
				index = (index == (count - 1)) ? 0 : index + 1;
			}
			else
			{
				// step down ine index allowing for being at the first defined value in which case we wrap around to the last
				index = (index == 0) ? count - 1 : index - 1;
			}

			// convert the new index into its associated value as we need this to apply to the property
			value = enumData->GetValueByIndex(index);
		}

		// we need to mark the target actor as about to be modified
		helper->OnPreChange(actor, Prop, description);

		// apply the new property value
		numProp->SetIntPropertyValue(DataPtr, value);

		// complete the transaction by saying the edit is now done
		helper->OnPostChange(actor, Prop);

		// we supply the caller with the new string value, we get it rather than use our calculated one in case of clipping in the editor
		value = numProp->GetSignedIntPropertyValue(DataPtr);
		newStrValue = enumData->GetNameStringByValue(value);

		// return with success
		return true;
	}

	// getting here means the value is not available
	return false;
}

// inner class function to help with menu based values, returns true if the string value was available and passed back
bool PropertyData::GetMenuValueAsString(FString &strValue)
{
	FNumericProperty* numProp = nullptr;
	UEnum* enumData = nullptr;

	// we support boolean properties, also enumerated values via the enum property type and also the byte property
	// type, they share a lot of the same processing but with some slight variation in set up
	if (FBoolProperty* boolProp = CastField<FBoolProperty>(Prop))
	{
		// fetch the bool value
		bool value = boolProp->GetPropertyValue(DataPtr);

		// return a value string based on the boolean state
		strValue = value ? FString("On") : FString("Off");
		return true;
	}
	else if (FEnumProperty* enumProp = CastField<FEnumProperty>(Prop))
	{
		// fetch the enum data to process
		enumData = enumProp->GetEnum();

		// enum properties are linked to another property that has the actual value
		numProp = enumProp->GetUnderlyingProperty();
	}
	else if (FByteProperty* byteProp = CastField<FByteProperty>(Prop))
	{
		// fetch the enum data to process
		enumData = byteProp->Enum;

		// the byte prop itself is the target property in this context
		numProp = byteProp;
	}

	// if the casting was valid and we have the required data we can continue
	if (numProp && enumData)
	{
		// fetch the current integer value
		int64 value = numProp->GetSignedIntPropertyValue(DataPtr);

		// get the string (without the scope prefix) and return with success
		strValue = enumData->GetNameStringByValue(value);
		return true;
	}

	// getting here means the value is not available
	return false;
}
