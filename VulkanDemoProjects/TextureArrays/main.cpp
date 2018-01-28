#include "Common.h"

#define TEXTURE_ARRAY_SIZE 8
#define FRAMES_PER_IMAGE 60
vkh::VkhContext appContext;

struct DemoData
{
	vkh::MeshAsset quadMesh;
	vkh::TextureAsset textures[8];

	std::vector<VkFramebuffer>		frameBuffers;
	vkh::VkhRenderBuffer			depthBuffer;
	std::vector<VkCommandBuffer>	commandBuffers;

	VkRenderPass					mainRenderPass;

	//We only have 1 material, so this can be stored here.
	VkPipelineLayout				pipelineLayout;
	VkDescriptorSet					descriptorSet;
	VkPipeline						graphicsPipeline;
	VkDescriptorSetLayout			descSetLayout;
	VkSampler						sampler;
	VkDescriptorImageInfo			descriptorImageInfos[TEXTURE_ARRAY_SIZE];
	int								imageIdx;
	int								framesUntilNextImage;
};

DemoData demoData;

void mainLoop();
void setupDemo();
void shutdown();
void createMainRenderPass();
void render();
void setupDescriptorSet();
void setupGraphicsPipeline();
void writeDescriptorSet();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE pInstance, LPSTR cmdLine, int showCode)
{
	HWND wndHdl = OS::makeWindow(Instance, "Texture Array Demo", 800, 600);
	OS::initializeInput();

	vkh::VkhContextCreateInfo ctxtInfo = {};
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
	ctxtInfo.typeCounts.push_back(8);
	ctxtInfo.typeCounts.push_back(1);

	initContext(ctxtInfo, "Texture Array Demo", Instance, wndHdl, appContext);
	setupDemo();
	setupDescriptorSet();
	setupGraphicsPipeline();
	writeDescriptorSet();

	mainLoop();
	shutdown();

	return 0;
}

void setupDemo()
{
	createMainRenderPass();
	vkh::createFrameBuffers(demoData.frameBuffers, appContext.swapChain, nullptr, demoData.mainRenderPass, appContext.device);

	uint32_t swapChainImageCount = static_cast<uint32_t>(appContext.swapChain.imageViews.size());
	demoData.commandBuffers.resize(swapChainImageCount);
	for (uint32_t i = 0; i < swapChainImageCount; ++i)
	{
		vkh::createCommandBuffer(demoData.commandBuffers[i], appContext.gfxCommandPool, appContext.device);
	}

	vkh::Mesh::quad(demoData.quadMesh, appContext);

	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
	{
		char filename[32];
		sprintf_s(filename, 32, "textures\\%i.png", i);
		vkh::Texture::make(demoData.textures[i], filename, appContext);
	}

	demoData.imageIdx = 5;
	demoData.framesUntilNextImage = FRAMES_PER_IMAGE;

}

void setupDescriptorSet()
{
	VkSamplerCreateInfo createInfo = vkh::samplerCreateInfo(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f);
	VkResult res = vkCreateSampler(appContext.device, &createInfo, 0, &demoData.sampler);
	checkf(res == VK_SUCCESS, "Error creating global sampler");

	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
	{
		demoData.descriptorImageInfos[i].sampler = nullptr;
		demoData.descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		demoData.descriptorImageInfos[i].imageView = demoData.textures[i].view;
	}

	VkDescriptorSetLayoutBinding layoutBindings[2];
	layoutBindings[0] = vkh::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

	//the descriptor count in the binding is the number of elements in your array
	layoutBindings[1] = vkh::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1, TEXTURE_ARRAY_SIZE);

	VkDescriptorSetLayoutCreateInfo layoutInfo = vkh::descriptorSetLayoutCreateInfo(layoutBindings, 2);
	res = vkCreateDescriptorSetLayout(appContext.device, &layoutInfo, nullptr, &demoData.descSetLayout);
	checkf(res == VK_SUCCESS, "Error creating desc set layout");
	
	VkDescriptorSetAllocateInfo allocInfo = vkh::descriptorSetAllocateInfo(&demoData.descSetLayout, 1, appContext.descriptorPool);
	res = vkAllocateDescriptorSets(appContext.device, &allocInfo, &demoData.descriptorSet);
	checkf(res == VK_SUCCESS, "Error allocating global descriptor set");
}

void writeDescriptorSet()
{
	VkWriteDescriptorSet setWrites[2];

	VkDescriptorImageInfo samplerInfo = {};
	samplerInfo.sampler = demoData.sampler;

	setWrites[0] = {};
	setWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrites[0].dstBinding = 0;
	setWrites[0].dstArrayElement = 0;
	setWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	setWrites[0].descriptorCount = 1;
	setWrites[0].dstSet = demoData.descriptorSet;
	setWrites[0].pBufferInfo = 0;
	setWrites[0].pImageInfo = &samplerInfo;

	setWrites[1] = {};
	setWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrites[1].dstBinding = 1;
	setWrites[1].dstArrayElement = 0;
	setWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	setWrites[1].descriptorCount = TEXTURE_ARRAY_SIZE;
	setWrites[1].pBufferInfo = 0;
	setWrites[1].dstSet = demoData.descriptorSet;
	setWrites[1].pImageInfo = demoData.descriptorImageInfos;

	vkUpdateDescriptorSets(appContext.device, 2, setWrites, 0, nullptr);
}

void createMainRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = appContext.swapChain.imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	std::vector<VkAttachmentDescription> renderPassAttachments;
	renderPassAttachments.push_back(colorAttachment);

	vkh::createRenderPass(demoData.mainRenderPass, renderPassAttachments, nullptr, appContext.device);

}

void setupGraphicsPipeline()
{
	VkPipelineShaderStageCreateInfo shaderStages[2];

	shaderStages[0] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
	DataBuffer* vShaderData = loadBinaryFile("shaders\\vanilla_vertex.spv");
	vkh::createShaderModule(shaderStages[0].module, vShaderData->data, vShaderData->size, appContext);
	
	shaderStages[1] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
	DataBuffer* fShaderData = loadBinaryFile("shaders\\texture_array.spv");
	vkh::createShaderModule(shaderStages[1].module, fShaderData->data, fShaderData->size, appContext);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkh::pipelineLayoutCreateInfo(&demoData.descSetLayout, 1);

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(int);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	VkResult res = vkCreatePipelineLayout(appContext.device, &pipelineLayoutInfo, nullptr, &demoData.pipelineLayout);
	checkf(res == VK_SUCCESS, "Error creating pipeline layout");

	VkVertexInputBindingDescription bindingDescription = vkh::vertexInputBindingDescription(0, sizeof(vkh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
	
	const vkh::VertexRenderData* vertexLayout = vkh::Mesh::vertexRenderData();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkh::pipelineVertexInputStateCreateInfo();
	vertexInputInfo.vertexBindingDescriptionCount = 1; 
	vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkh::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	VkViewport viewport = vkh::viewport(0, 0, static_cast<float>(appContext.swapChain.extent.width), static_cast<float>(appContext.swapChain.extent.height));
	VkRect2D scissor = vkh::rect2D(0, 0, appContext.swapChain.extent.width, appContext.swapChain.extent.height);
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
	pipelineInfo.layout = demoData.pipelineLayout;
	pipelineInfo.renderPass = demoData.mainRenderPass;
	pipelineInfo.pDepthStencilState = &depthStencil;

	pipelineInfo.subpass = 0;

	//can use this to create new pipelines by deriving from old ones
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(appContext.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &demoData.graphicsPipeline);
	checkf(res == VK_SUCCESS, "Error creating graphics pipeline");



	freeDataBuffer(vShaderData);
	freeDataBuffer(fShaderData);

}

void logFPSAverage(double avg)
{
	printf("AVG FRAMETIME FOR LAST %i FRAMES: %f ms\n", FPS_DATA_FRAME_HISTORY_SIZE, avg);
}

void mainLoop()
{
	bool running = true;

	FPSData fpsData = { 0 };

	//fpsData.logCallback = logFPSAverage;

	startTimingFrame(fpsData);

	while (running)
	{
		double dt = endTimingFrame(fpsData);
		startTimingFrame(fpsData);

		demoData.framesUntilNextImage--;
		if (demoData.framesUntilNextImage == 0)
		{
			demoData.framesUntilNextImage = FRAMES_PER_IMAGE;
			demoData.imageIdx = (demoData.imageIdx + 1) % TEXTURE_ARRAY_SIZE;
		}

		OS::handleEvents();
		OS::pollInput();

		if (OS::getKey(KEY_ESCAPE))
		{
			running = false;
			break;
		}

		render();
	}
}

void shutdown()
{
	OS::shutdownInput();
}

void render()
{
	//acquire an image from the swap chain
	uint32_t imageIndex;

	//using uint64 max for timeout disables it
	VkResult res = vkAcquireNextImageKHR(appContext.device, appContext.swapChain.swapChain, UINT64_MAX, appContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkh::waitForFence(appContext.frameFences[imageIndex], appContext.device);
	vkResetFences(appContext.device, 1, &appContext.frameFences[imageIndex]);

	//record drawing
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional
	vkResetCommandBuffer(demoData.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	res = vkBeginCommandBuffer(demoData.commandBuffers[imageIndex], &beginInfo);


	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = demoData.mainRenderPass;
	renderPassInfo.framebuffer = demoData.frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = appContext.swapChain.extent;

	std::vector<VkClearValue> clearColors;

	//color
	clearColors.push_back({ 1.0f, 0.0f, 0.0f, 1.0f });

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
	renderPassInfo.pClearValues = &clearColors[0];
	vkCmdBeginRenderPass(demoData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(demoData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, demoData.graphicsPipeline);
	
	vkCmdPushConstants(
		demoData.commandBuffers[imageIndex],
		demoData.pipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(int),
		(void*)&demoData.imageIdx);

	vkCmdBindDescriptorSets(demoData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, demoData.pipelineLayout, 0, 1, &demoData.descriptorSet, 0, 0);

	VkBuffer vertexBuffers[] = { demoData.quadMesh.vBuffer };
	VkDeviceSize vertexOffsets[] = { 0 };
	vkCmdBindVertexBuffers(demoData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(demoData.commandBuffers[imageIndex], demoData.quadMesh.iBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(demoData.commandBuffers[imageIndex], static_cast<uint32_t>(demoData.quadMesh.iCount), 1, 0, 0, 0);



	vkCmdEndRenderPass(demoData.commandBuffers[imageIndex]);
	res = vkEndCommandBuffer(demoData.commandBuffers[imageIndex]);
	assert(res == VK_SUCCESS);


	//submit

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//wait on writing colours to the buffer until the semaphore says the buffer is available
	VkSemaphore waitSemaphores[] = { appContext.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;

	VkSemaphore signalSemaphores[] = { appContext.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = &demoData.commandBuffers[imageIndex];
	submitInfo.commandBufferCount = 1;

	res = vkQueueSubmit(appContext.deviceQueues.graphicsQueue, 1, &submitInfo, appContext.frameFences[imageIndex]);
	assert(res == VK_SUCCESS);

	//present

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { appContext.swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional
	res = vkQueuePresentKHR(appContext.deviceQueues.transferQueue, &presentInfo);
}
