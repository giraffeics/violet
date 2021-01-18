#include "GPUMesh.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "GPUEngine.h"

/**
* Passes back the stride and format of a valid attribute type.
* Returns false for an invalid type.
*/
bool GPUMesh::getAttributeProperties(uint32_t& stride, VkFormat& format, GPUMesh::AttributeType type)
{
	switch (type)
	{
	case MESH_ATTRIBUTE_POSITION:
		stride = sizeof(glm::vec3);
		format = VK_FORMAT_R32G32B32_SFLOAT;
		return true;
	}

	return false;
}

// GPUMesh member function implementations

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
}

void GPUMesh::load()
{
	DataVectors data;

	ensureFenceExists();
	loadFileData(data);
	createBuffers(data);
	mNumIndices = data.index.size();
}

void GPUMesh::draw(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mPositionBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, mNumIndices, 1, 0, 0, 0);
}

// TODO: actually load data from a file
bool GPUMesh::loadFileData(DataVectors& data)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("../assets/"+mName, aiProcess_Triangulate);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !(scene->mRootNode))
		return false;

	auto node = scene->mRootNode;
	if (node->mNumMeshes < 1)
		return false;
	auto mesh = scene->mMeshes[node->mMeshes[0]];

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
	auto posSize = sizeof(data.position[0]) * data.position.size();//getBufferDataSize(data);
	auto indexSize = sizeof(data.index[0]) * data.index.size();

	// create & fill position buffer
	mEngine->createBuffer(posSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mPositionBuffer, mPositionMemory);
	mEngine->transferToBuffer(mPositionBuffer, data.position.data(), posSize, 0);

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