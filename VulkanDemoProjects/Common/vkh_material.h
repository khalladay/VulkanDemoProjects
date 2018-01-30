#pragma once
#include "vkh.h"
#include "file_utils.h"
#include "vkh_initializers.h"
#include "vkh_mesh.h"
#include <vector>

namespace vkh
{
	struct VkhMaterialCreateInfo
	{
		VkRenderPass renderPass;

		std::vector<VkDescriptorSetLayout> descSetLayouts;
		VkPipelineLayout* outPipelineLayout;
		VkPipeline* outPipeline;
	};

	void createBasicMaterial(const char* vShaderPath, const char* fShaderPath, VkhContext& ctxt, VkhMaterialCreateInfo& createInfo)
	{
		VkPipelineShaderStageCreateInfo shaderStages[2];

		shaderStages[0] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
		DataBuffer* vShaderData = loadBinaryFile(vShaderPath);
		vkh::createShaderModule(shaderStages[0].module, vShaderData->data, vShaderData->size, ctxt);

		shaderStages[1] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
		DataBuffer* fShaderData = loadBinaryFile(fShaderPath);
		vkh::createShaderModule(shaderStages[1].module, fShaderData->data, fShaderData->size, ctxt);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkh::pipelineLayoutCreateInfo(createInfo.descSetLayouts.data(), createInfo.descSetLayouts.size());

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(int);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		VkResult res = vkCreatePipelineLayout(ctxt.device, &pipelineLayoutInfo, nullptr, createInfo.outPipelineLayout);
		checkf(res == VK_SUCCESS, "Error creating pipeline layout");

		VkVertexInputBindingDescription bindingDescription = vkh::vertexInputBindingDescription(0, sizeof(vkh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

		const vkh::VertexRenderData* vertexLayout = vkh::Mesh::vertexRenderData();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkh::pipelineVertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkh::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
		VkViewport viewport = vkh::viewport(0, 0, static_cast<float>(ctxt.swapChain.extent.width), static_cast<float>(ctxt.swapChain.extent.height));
		VkRect2D scissor = vkh::rect2D(0, 0, ctxt.swapChain.extent.width, ctxt.swapChain.extent.height);
		VkPipelineViewportStateCreateInfo viewportState = vkh::pipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);

		VkPipelineRasterizationStateCreateInfo rasterizer = vkh::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		VkPipelineMultisampleStateCreateInfo multisampling = vkh::pipelineMultisampleStateCreateInfo();

		VkPipelineColorBlendAttachmentState colorBlendAttachment = vkh::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlending = vkh::pipelineColorBlendStateCreateInfo(colorBlendAttachment);

		VkPipelineDepthStencilStateCreateInfo depthStencil = vkh::pipelineDepthStencilStateCreateInfo(
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = *createInfo.outPipelineLayout;
		pipelineInfo.renderPass = createInfo.renderPass;
		pipelineInfo.pDepthStencilState = &depthStencil;

		pipelineInfo.subpass = 0;

		//can use this to create new pipelines by deriving from old ones
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(ctxt.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, createInfo.outPipeline);
		checkf(res == VK_SUCCESS, "Error creating graphics pipeline");

		freeDataBuffer(vShaderData);
		freeDataBuffer(fShaderData);
	}
}