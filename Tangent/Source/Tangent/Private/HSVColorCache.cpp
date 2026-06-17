// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 264 $

#include "HSVColorCache.h"

// implements a time limited cache for an HSV based colour associated to a light based actor
HSVColorCache::HSVColorCache()
{
	// init the timestamp to the creation date
	_timestamp = FDateTime::Now();
}

// returns the current colour of the referenced light instance converted to hsv, it may be sourced from the cache if available
// otherwise it is read from the light instance, the cache is invalidated after a short period 
void HSVColorCache::getHSV(ALight* light, FLinearColor& color)
{
	FDateTime	now = FDateTime::Now();
	FTimespan	span = now - _timestamp;
	double		msecs = span.GetTotalMilliseconds();

	// the cached values are only held for up to a small preset period to avoid drift or use of an incorrect value
	if (msecs >= 500)
	{
		// the cache in invalid, read the rgb colour from the light instance and convert to hsv
		FLinearColor	rgbColor = light->GetLightColor();
		FLinearColor	hsvColor = rgbColor.LinearRGBToHSV();

		// cache the new value and update the timestamp
		_colorCache = hsvColor;
		_timestamp = now;
	}

	// pass back the value as set, either the existing cached value or the newly cached update
	color = _colorCache;
}

// stores the supplied hsv value in the cache with a timestamp
void HSVColorCache::setHSV(FLinearColor& color)
{
	// store the data
	_colorCache = color;
	_timestamp = FDateTime::Now();
}
