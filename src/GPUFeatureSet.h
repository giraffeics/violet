#ifndef GPU_FEATURE_SET_H
#define GPU_FEATURE_SET_H

#include <cstdint>

class GPUFeatureSet
{
public:
	virtual const char** getRequiredInstanceExtensions(uint32_t* count)
	{
		*count = 0;
		return nullptr;
	}
	virtual const char** getRequiredDeviceExtensions(uint32_t* count)
	{
		*count = 0;
		return nullptr;
	}
};

#endif