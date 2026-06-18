// Tangent Panels Plugin for Unreal Editor
// Copyright 2023 Tangent Wave Ltd.
// SVN: $Revision: 339 $

#include "ResetCache.h"

// clears all data currently stored in the reset cache singleton instance
void ResetCache::Clear()
{
	// release all data in the luts to give us a blank canvas
	ResetCache::GetInstance()._cacheTargetLUT.Reset();
	ResetCache::GetInstance()._cacheTapLUT.Reset();
}

// called to register a reset action for the given control target, marking it as a potential single or double tap, the two may
// generate different behaviours in terms of what reset values are to be applied
void ResetCache::TapNow(uint32 targetId)
{
	// we use a seconds counter to mark time periods between taps
	double now = FPlatformTime::Seconds();

	// get a tap struct for this target id, if there is no existing entry then one is created for us
	ResetCacheTap& tap = ResetCache::GetInstance()._cacheTapLUT.FindOrAdd(targetId);

	// populate this struct, updating or setting the single tap boolean flag (true -> single tap, false -> double or multiple tap) and also
	// saving the timestamp when this tap was registered, any taps for the same target id more than a predefined time apart is taken as a single
	tap.singleTap = ((now - tap.lastTap) > 0.5);
	tap.lastTap = now;
}

// returns true if the last interaction for the specified target id is considered as a single reset tap
bool ResetCache::IsSingleTap(uint32 targetId)
{
	// look for an entry in the tap table under this target id, if there is no entry we will get back a null pointer as we don't want to create a
	// new entry here when looking up a state
	ResetCacheTap* tap = ResetCache::GetInstance()._cacheTapLUT.Find(targetId);
	if (tap)
	{
		// we have a valid entry, return the tap state flag that was updated on the last tap for the target id
		return tap->singleTap;
	}

	// getting here means no entry is available, this shouldn't happen as this means we are testing for a single/double tap when no taps have been 
	// registered for the target id but to avoid problems we return as if a single tap is present
	return true;
}

// internal helper function used when trying to store a value in the cache, only returns a valid pointer if there is no existing value for 
// this target id and object pairing so the new value can be stored
ResetCacheValue* ResetCache::CacheValueHelper(uint32 targetId, UObject* obj)
{
	// first use the target id to get the second level map of objects and values, if there is no entry a new one will be created for us
	TMap<uint32, ResetCacheValue>& cacheObjectLUT = _cacheTargetLUT.FindOrAdd(targetId);

	// now use this map to look up values for this specific object, again a new entry will be created if not already available
	ResetCacheValue& cacheValue = cacheObjectLUT.FindOrAdd(obj->GetUniqueID());

	// we will always have a value struct here which may or may not have a value
	if (!cacheValue._hasValue)
	{
		// no value is stored for this context so we are free to return the pointer to it for the caller to cache a new value
		return &cacheValue;
	}

	// getting here means a value is already cached for this target and object and we only take the first, return a null pointer
	return nullptr;
}

// internal helper function used when trying to fetch a value from the cache, only returns a valid pointer if an entry is found for this target
// id and object pairing with a stored value
ResetCacheValue* ResetCache::GetValueHelper(uint32 targetId, UObject* obj)
{
	// first use the target id to get the second level map of objects and values, if there is no entry we will get back a null pointer as we don't want
	// to create a new entry here for value look up
	TMap<uint32, ResetCacheValue>* cacheObjectLUT = _cacheTargetLUT.Find(targetId);
	if (cacheObjectLUT)
	{
		// we have data for the target id, see if there is any value struct for this specific object, again we don't want to create a new entry if there is not one in place
		ResetCacheValue* cacheValue = cacheObjectLUT->Find(obj->GetUniqueID());
		if (cacheValue && cacheValue->_hasValue)
		{
			// we have a value struct for the given context and it has a stored value, return access to the struct for access to the appropriate value type
			return cacheValue;
		}
	}

	// getting here means there is no cached value for this target/object combo, return a null pointer
	return nullptr;
}

// called to potentially store a value of the specified type in the cache for the given control target id and object, we only cache the first value
void ResetCache::CacheInitialValue(uint32 targetId, UObject* obj, double value)
{
	// check for a valid cache value struct where we might store the value, it will return null if we already have a value
	ResetCacheValue* cacheValue = ResetCache::GetInstance().CacheValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid value struct which means this is the first value that has come in for this target/object, store via the appropriate union struct member
		// remembering to set the flag to say this struct now has valid data and cannot be written to again
		cacheValue->_value.doubleValue = value;
		cacheValue->_hasValue = true;
	}
}

// called to check the cache for a value for the given control target id and object, if one is found the function returns true with the value passed back
bool ResetCache::GetValue(uint32 targetId, UObject* obj, double* pValue)
{
	// check for a valid cache value struct, it will return null if no data is stored
	ResetCacheValue* cacheValue = ResetCache::GetInstance().GetValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid cache entry for this target/object with a stored value, pass it back using the appropriate union struct member access and return success
		*pValue = cacheValue->_value.doubleValue;
		return true;
	}

	// getting here means there is no cached value for this target/object 
	return false;
}

// called to potentially store a value of the specified type in the cache for the given control target id and object, we only cache the first value
void ResetCache::CacheInitialValue(uint32 targetId, UObject* obj, float value)
{
	// check for a valid cache value struct where we might store the value, it will return null if we already have a value
	ResetCacheValue* cacheValue = ResetCache::GetInstance().CacheValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid value struct which means this is the first value that has come in for this target/object, store via the appropriate union struct member
		// remembering to set the flag to say this struct now has valid data and cannot be written to again
		cacheValue->_value.floatValue = value;
		cacheValue->_hasValue = true;
	}
}

// called to check the cache for a value for the given control target id and object, if one is found the function returns true with the value passed back
bool ResetCache::GetValue(uint32 targetId, UObject* obj, float* pValue)
{
	// check for a valid cache value struct, it will return null if no data is stored
	ResetCacheValue* cacheValue = ResetCache::GetInstance().GetValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid cache entry for this target/object with a stored value, pass it back using the appropriate union struct member access and return success
		*pValue = cacheValue->_value.floatValue;
		return true;
	}

	// getting here means there is no cached value for this target/object 
	return false;
}

// called to potentially store a value of the specified type in the cache for the given control target id and object, we only cache the first value
void ResetCache::CacheInitialValue(uint32 targetId, UObject* obj, int64 value)
{
	// check for a valid cache value struct where we might store the value, it will return null if we already have a value
	ResetCacheValue* cacheValue = ResetCache::GetInstance().CacheValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid value struct which means this is the first value that has come in for this target/object, store via the appropriate union struct member
		// remembering to set the flag to say this struct now has valid data and cannot be written to again
		cacheValue->_value.intValue = value;
		cacheValue->_hasValue = true;
	}
}

// called to check the cache for a value for the given control target id and object, if one is found the function returns true with the value passed back
bool ResetCache::GetValue(uint32 targetId, UObject* obj, int64* pValue)
{
	// check for a valid cache value struct, it will return null if no data is stored
	ResetCacheValue* cacheValue = ResetCache::GetInstance().GetValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid cache entry for this target/object with a stored value, pass it back using the appropriate union struct member access and return success
		*pValue = cacheValue->_value.intValue;
		return true;
	}

	// getting here means there is no cached value for this target/object 
	return false;
}

// called to potentially store a value of the specified type in the cache for the given control target id and object, we only cache the first value
void ResetCache::CacheInitialValue(uint32 targetId, UObject* obj, uint64 value)
{
	// check for a valid cache value struct where we might store the value, it will return null if we already have a value
	ResetCacheValue* cacheValue = ResetCache::GetInstance().CacheValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid value struct which means this is the first value that has come in for this target/object, store via the appropriate union struct member
		// remembering to set the flag to say this struct now has valid data and cannot be written to again
		cacheValue->_value.uintValue = value;
		cacheValue->_hasValue = true;
	}
}

// called to check the cache for a value for the given control target id and object, if one is found the function returns true with the value passed back
bool ResetCache::GetValue(uint32 targetId, UObject* obj, uint64* pValue)
{
	// check for a valid cache value struct, it will return null if no data is stored
	ResetCacheValue* cacheValue = ResetCache::GetInstance().GetValueHelper(targetId, obj);
	if (cacheValue)
	{
		// we have a valid cache entry for this target/object with a stored value, pass it back using the appropriate union struct member access and return success
		*pValue = cacheValue->_value.uintValue;
		return true;
	}

	// getting here means there is no cached value for this target/object 
	return false;
}
