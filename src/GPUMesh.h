#ifndef GPUMESH_H
#define GPUMESH_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "glm_includes.h"

class GPUEngine;

/**
 * @brief Manages the loading and rendering of a single mesh.
 * 
 * Loads mesh data from a file, transfers said data to the GPU, and manages
 * the associated resources. Note the difference between a mesh and a mesh isntance.
 * While a mesh contains vertex and index data, a mesh instance contains a reference
 * to a mesh and one or more parameters used to render that mesh. There can be multiple
 * instances of a single mesh, and a mesh does not track its instances in any way.
 */
class GPUMesh
{
public:
	/**
	 * @brief Enum referring to the various types of attribute data associated with a vertex.
	 * 
	 * Currently, only the position attribute is implemented, but more will be added in the future.
	 */
	enum AttributeType
	{
		MESH_ATTRIBUTE_POSITION,
		MESH_ATTRIBUTE_ENUM_LENGTH
	};

	/**
	 * @brief Contains a reference to a GPUMesh, as well as transform data and a dynamic offset into the GPUMeshWrangler's uniform buffer.
	 * 
	 * Transform data is currently the only parameter, although more may be added  later.
	 * The dynamic offset is intended to be directly used only by GPUMeshWrangler, which collects
	 * the uniform data for all mesh instances in a frame into a single uniform buffer and gives
	 * each of them an offset into said buffer.
	 */
	class Instance
	{
	public:
		GPUMesh* mMesh;
		glm::mat4 mTransform = glm::identity<glm::mat4>();
		uint32_t mDynamicOffset = 0;
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
	/**
	 * @brief A container for mesh data that has not yet been transferred to GPU memory.
	 */
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