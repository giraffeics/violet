#ifndef GPUMESH_H
#define GPUMESH_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class GPUEngine;

class GPUMesh
{
public:
	enum AttributeType
	{
		MESH_ATTRIBUTE_POSITION,
		MESH_ATTRIBUTE_ENUM_LENGTH
	};

	static bool getAttributeProperties(uint32_t& stride, VkFormat& format, AttributeType type);

	// constructors & destructor
	GPUMesh(std::string name, GPUEngine* engine);
	GPUMesh(GPUMesh& other) = delete;
	GPUMesh(GPUMesh&& other) = delete;
	GPUMesh& operator=(GPUMesh& other) = delete;
	~GPUMesh();

	// public functionality
	void load();
	void draw(VkCommandBuffer commandBuffer);

private:
	// used in the load() function
	struct DataVectors{
		std::vector<glm::vec3> position;
		std::vector<uint32_t> index;
	};

	bool loadFileData(DataVectors& data);
	bool createBuffers(DataVectors& data);

	// private helper functions
	void ensureFenceExists();
	static VkDeviceSize getBufferDataSize(DataVectors& data);

	// private member variables
	std::string mName;
	GPUEngine* mEngine;
	VkFence mFence = VK_NULL_HANDLE;
	VkBuffer mPositionBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mPositionMemory = VK_NULL_HANDLE;
	VkBuffer mIndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mIndexMemory = VK_NULL_HANDLE;
	size_t positionOffset = 0;
	size_t indexOffset = 0;
	size_t mNumIndices = 0;
};

#endif