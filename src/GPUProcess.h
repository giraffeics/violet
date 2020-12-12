#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>

class GPUProcess
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