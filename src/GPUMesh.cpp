#include "GPUMesh.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "GPUEngine.h"

/**
 * @brief Passes back the stride and format of a valid GPUMesh::AttributeType.
 * 
 * @param stride Stride, in bytes, between two vertices' data in the associated vertex buffer.
 * @param format VkFormat used to parse the associated data in shaders.
 * @param type The attribute type for which properties will be passed back.
 * @return true type is a valid GPUMesh::AttributeType and the attributes have been passed back.
 * @return false type is not a valid GPUMesh::AttributeType and nothing has been passed back.
 */
bool GPUMesh::getAttributeProperties(uint32_t& stride, VkFormat& format, GPUMesh::AttributeType type)
{
	switch (type)
	{
	case MESH_ATTRIBUTE_POSITION:
		stride = sizeof(glm::vec3);
		format = VK_FORMAT_R32G32B32_SFLOAT;
		return true;

	case MESH_ATTRIBUTE_NORMAL:
		stride = sizeof(glm::vec3);
		format = VK_FORMAT_R32G32B32_SFLOAT;
		return true;
	}

	return false;
}

// GPUMesh member function implementations

/**
 * @brief Construct a new GPUMesh object with a given file name.
 * 
 * When load() is called for this GPUMesh, the mesh data will be loaded from
 * the file with that name in the "assets" folder.
 * 
 * @param name Name of the file to load mesh data from in the "assets" folder.
 * @param engine Pointer to a GPUEngine instance that this GPUMesh will use.
 */
GPUMesh::GPUMesh(std::string name, GPUEngine* engine)
{
	mEngine = engine;
	mName = name;
}

GPUMesh::~GPUMesh()
{
	VkDevice device = mEngine->getDevice();

	vkDestroyFence(device, mFence, nullptr);
	vkFreeMemory(device, mPositionMemory, nullptr);
	vkDestroyBuffer(device, mPositionBuffer, nullptr);
	vkFreeMemory(device, mIndexMemory, nullptr);
	vkDestroyBuffer(device, mIndexBuffer, nullptr);

	if(mNormmalBuffer != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, mNormalMemory, nullptr);
		vkDestroyBuffer(device, mNormmalBuffer, nullptr);
	}
}

/**
 * @brief Load the data associated with this mesh and prepare it for rendering.
 * 
 * Mesh data is loaded from the file that was specified when this GPUMesh was created.
 */
void GPUMesh::load()
{
	DataVectors data;

	ensureFenceExists();
	loadFileData(data);
	createBuffers(data);
	mNumIndices = data.index.size();
}

/**
 * @brief Record draw commands for this mesh into a given VkCommandBuffer.
 * 
 * The associated buffers are bound and then a draw call is recorded.
 * It is assumed that commandBuffer is in a state where draw commands can be successfully recorded to it.
 * It is also assumed that all relevant descriptor sets have already been bound, I.E. through the use
 * of the engine's GPUMeshWrangler.
 * 
 * @param commandBuffer The VkCommandBuffer in which to record draw commands.
 */
void GPUMesh::draw(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mPositionBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, mNumIndices, 1, 0, 0, 0);
}

aiMesh* findMesh(const aiScene* scene, aiNode* node)
{
	if(node->mNumMeshes >= 1)
		return scene->mMeshes[node->mMeshes[0]];
	
	for(uint32_t i=0; i<node->mNumChildren; i++)
	{
		auto mesh = findMesh(scene, node->mChildren[i]);
		if(mesh != nullptr)
			return mesh;
	}

	return nullptr;
}

// TODO: actually load data from a file
bool GPUMesh::loadFileData(DataVectors& data)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("../assets/"+mName, aiProcess_Triangulate);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !(scene->mRootNode))
		return false;

	auto node = scene->mRootNode;
	auto mesh = findMesh(scene, node);

	data.position.clear();
	data.index.clear();

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		data.position.push_back({
			mesh->mVertices[i].x,
			-mesh->mVertices[i].y,
			mesh->mVertices[i].z}
		);
	}

	if(mesh->HasNormals())
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			data.normal.push_back({
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z}
			);
		}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		auto& face = mesh->mFaces[i];
		for (size_t j = 0; j < 3; j++)
			data.index.push_back(face.mIndices[j]);
	}

	return true;
}

bool GPUMesh::createBuffers(DataVectors& data)
{
	VkDevice device = mEngine->getDevice();
	auto posSize = sizeof(data.position[0]) * data.position.size();
	auto indexSize = sizeof(data.index[0]) * data.index.size();

	auto normalSize = data.normal.size();
	if(normalSize > 0)
		normalSize *= sizeof(data.normal[0]);

	// create & fill position buffer
	mEngine->createBuffer(posSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mPositionBuffer, mPositionMemory);
	mEngine->transferToBuffer(mPositionBuffer, data.position.data(), posSize, 0);

	// create & fill normal buffer
	if(normalSize > 0)
	{
		mEngine->createBuffer(normalSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mNormmalBuffer, mNormalMemory);
		mEngine->transferToBuffer(mNormmalBuffer, data.normal.data(), normalSize, 0);
	}

	// create & fill index buffer
	mEngine->createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer, mIndexMemory);
	mEngine->transferToBuffer(mIndexBuffer, data.index.data(), indexSize, 0);

	return true;
}

void GPUMesh::ensureFenceExists()
{
	if (mFence == VK_NULL_HANDLE)
		mFence = mEngine->createFence(0);
}

VkDeviceSize GPUMesh::getBufferDataSize(DataVectors& data)
{
	return sizeof(data.position[0]) * data.position.size() +
		sizeof(data.index[0]) * data.index.size();
}