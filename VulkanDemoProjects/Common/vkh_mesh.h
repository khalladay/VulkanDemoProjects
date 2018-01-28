#pragma once
#include "vkh.h"

#define GLM_FORCE_RADIANS
#define GLM_FORECE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
namespace vkh
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec4 col;
	};

	struct VertexRenderData
	{
		VkVertexInputAttributeDescription* attrDescriptions;
		uint32_t attrCount;
	};

	struct MeshAsset
	{
		VkBuffer vBuffer;
		VkBuffer iBuffer;

		Allocation vBufferMemory;
		Allocation iBufferMemory;

		uint32_t vCount;
		uint32_t iCount;
	};
}

namespace vkh::Mesh
{
	const VertexRenderData* vertexRenderData()
	{
		static VertexRenderData* vkRenderData = nullptr;
		if (!vkRenderData)
		{
			vkRenderData = (VertexRenderData*)malloc(sizeof(VertexRenderData));
			vkRenderData->attrCount = 2;

			vkRenderData->attrDescriptions = (VkVertexInputAttributeDescription*)malloc(sizeof(VkVertexInputAttributeDescription) * vkRenderData->attrCount);
			vkRenderData->attrDescriptions[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
			vkRenderData->attrDescriptions[1] = { 1,0,VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) };
		}

		return vkRenderData;
	}

	void make(MeshAsset& outAsset, VkhContext& ctxt, Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount)
	{
		size_t vBufferSize = sizeof(Vertex) * vertexCount + sizeof(uint32_t) * indexCount;
		size_t iBufferSize = sizeof(uint32_t) * indexCount;

		MeshAsset& m = outAsset;
		m.iCount = indexCount;
		m.vCount = vertexCount;

		createBuffer(m.vBuffer,
			m.vBufferMemory,
			vBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			ctxt
		);

		createBuffer(m.iBuffer,
			m.iBufferMemory,
			iBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			ctxt
		);


		//transfer data to the above buffers
		VkBuffer stagingBuffer;
		Allocation stagingMemory;

		createBuffer(stagingBuffer,
			stagingMemory,
			vBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ctxt
		);

		void* data;
		vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset, vBufferSize, 0, &data);
		memcpy(data, vertices, (size_t)vBufferSize);
		vkUnmapMemory(ctxt.device, stagingMemory.handle);

		//copy to device local here
		copyBuffer(stagingBuffer, m.vBuffer, vBufferSize, 0, 0, ctxt);
		freeDeviceMemory(stagingMemory);
		vkDestroyBuffer(ctxt.device, stagingBuffer, nullptr);

		createBuffer(stagingBuffer,
			stagingMemory,
			iBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ctxt
		);

		vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset, iBufferSize, 0, &data);
		memcpy(data, indices, (size_t)iBufferSize);
		vkUnmapMemory(ctxt.device, stagingMemory.handle);

		copyBuffer(stagingBuffer, m.iBuffer, iBufferSize, 0, 0, ctxt);
		freeDeviceMemory(stagingMemory);
		vkDestroyBuffer(ctxt.device, stagingBuffer, nullptr);
	}

	void quad(MeshAsset& outAsset, VkhContext& ctxt, float width = 2.0f, float height = 2.0f, float xOffset = 0.0f, float yOffset = 0.0f)
	{
		std::vector<Vertex> verts;

		float wComp = width / 2.0f;
		float hComp = height / 2.0f;

		glm::vec3 lbCorner = glm::vec3(-wComp + xOffset, -hComp + yOffset, 0.0f);
		glm::vec3 ltCorner = glm::vec3(lbCorner.x, hComp + yOffset, 0.0f);
		glm::vec3 rbCorner = glm::vec3(wComp + xOffset, lbCorner.y, 0.0f);
		glm::vec3 rtCorner = glm::vec3(rbCorner.x, ltCorner.y, 0.0f);

		verts.push_back({ rtCorner,  glm::vec2(1.0f,1.0f), glm::vec4(1.0f,1.0f,1.0f,1.0f) });
		verts.push_back({ ltCorner, glm::vec2(0.0f,1.0f), glm::vec4(0.0f,1.0f,1.0f,1.0f) });
		verts.push_back({ lbCorner,glm::vec2(0.0f,0.0f), glm::vec4(1.0f,1.0f,1.0f,1.0f) });
		verts.push_back({ rbCorner, glm::vec2(1.0f,0.0f), glm::vec4(1.0f,1.0f,1.0f,1.0f) });

		uint32_t indices[6] = { 0,2,1,2,0,3 };

		make(outAsset, ctxt, &verts[0], static_cast<uint32_t>(verts.size()), &indices[0], 6);
	}
}