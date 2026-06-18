// Tangent Panels Plugin for Unreal Editor
// Copyright 2023 Tangent Wave Ltd.
// SVN: $Revision: 339 $

#pragma once

#include "CoreMinimal.h"
#include "Controls.h"

// storage used to cache various value types for reset purposes
struct ResetCacheValue
{
public:
	ResetCacheValue() : _hasValue(false) { }

public:
	bool		_hasValue;
	union
	{
		double	doubleValue;
		float	floatValue;
		int64	intValue;
		uint64	uintValue;
	}			_value;
};

// storage for a single target id that keeps track of the time of the last tap and the single/double tap status
struct ResetCacheTap
{
public:
	ResetCacheTap() : singleTap(true), lastTap(0) {}

public:
	bool		singleTap;
	double		lastTap;
};

// implements a simple singleton caching object for reset behaviour, the functions are presented as static that wrap
// around a singleton instance for ease of use
class ResetCache
{
public:
	static inline ResetCache& GetInstance() { static ResetCache _instance; return _instance; }

	static void Clear();

	static void TapNow(uint32 targetId);
	static bool IsSingleTap(uint32 targetId);

	static void CacheInitialValue(uint32 targetId, UObject* obj, double value);
	static bool GetValue(uint32 targetId, UObject* obj, double* pValue);

	static void CacheInitialValue(uint32 targetId, UObject* obj, float value);
	static bool GetValue(uint32 targetId, UObject* obj, float* pValue);

	static void CacheInitialValue(uint32 targetId, UObject* obj, int64 value);
	static bool GetValue(uint32 targetId, UObject* obj, int64* pValue);

	static void CacheInitialValue(uint32 targetId, UObject* obj, uint64 value);
	static bool GetValue(uint32 targetId, UObject* obj, uint64* pValue);

private:
	ResetCache() { }

	ResetCacheValue* CacheValueHelper(uint32 targetId, UObject* obj);
	ResetCacheValue* GetValueHelper(uint32 targetId, UObject* obj);

private:
	TMap<uint32, TMap<uint32, ResetCacheValue>>	_cacheTargetLUT;
	TMap<uint32, ResetCacheTap>					_cacheTapLUT;
};
