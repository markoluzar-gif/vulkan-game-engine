#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

#include "../game_objects/i_game_object.hpp"
#include "../game_objects/camera_game_object.hpp"
#include "../game_objects/model_game_object.hpp"
#include "../game_objects/box_collider_game_object.hpp"
#include "../game_objects/mesh_game_object.hpp"
#include "../game_objects/animated_game_object.hpp"




static void check_vulkan_result(VkResult result, std::string message)
{
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("vulkan result failed: " + message + " code: ");
	}
}

static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((type & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

static VkFormat find_depth_format(VkPhysicalDevice physical_device)
{
	const std::array<VkFormat, 3> candidates = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	std::string failed_candidates = "failed candidates: ";
	for (VkFormat format : candidates)
	{
		failed_candidates += std::to_string(format) + " ";
	}

	throw std::runtime_error("failed to find supported depth format: " + failed_candidates);
}

static VkImage create_image(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags)
{
	VkImage image{};

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = format;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = tiling;
	image_info.usage = usage_flags;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	check_vulkan_result(vkCreateImage(device, &image_info, nullptr, &image), "failed to create image");

	return image;
}

static VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageView image_view{};

	VkComponentMapping components = {};
	components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.components = components;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	check_vulkan_result(vkCreateImageView(device, &view_info, nullptr, &image_view), "failed to create image view");

	return image_view;
}

static VkDeviceMemory allocate_image_memory(VkPhysicalDevice physical_device, VkDevice device, VkImage image, VkMemoryPropertyFlags property_flags)
{
	VkDeviceMemory image_memory;

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, image, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, property_flags);

	check_vulkan_result(vkAllocateMemory(device, &allocate_info, nullptr, &image_memory), "failed to allocate image memory");
	check_vulkan_result(vkBindImageMemory(device, image, image_memory, 0), "failed to bind image memory");

	return image_memory;
}

static VkBuffer create_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage_flags)
{
	VkBuffer buffer{};

	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage_flags;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	check_vulkan_result(vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "failed to create buffer");

	return buffer;
}

static VkDeviceMemory allocate_buffer_memory(VkPhysicalDevice physical_device, VkDevice device, VkBuffer buffer, VkMemoryPropertyFlags property_flags)
{
	VkDeviceMemory buffer_memory;

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, property_flags);

	check_vulkan_result(vkAllocateMemory(device, &memory_allocate_info, nullptr, &buffer_memory), "failed to allocate buffer memory");
	check_vulkan_result(vkBindBufferMemory(device, buffer, buffer_memory, 0), "failed to bind buffer memory");

	return buffer_memory;
}

static VkCommandBuffer begin_command_buffer(VkDevice device, VkCommandPool command_pool)
{
	VkCommandBuffer command_buffer{};

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;

	check_vulkan_result(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer), "failed to allocate command buffer");

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	check_vulkan_result(vkBeginCommandBuffer(command_buffer, &begin_info), "failed to begin command buffer");

	return command_buffer;
}

static void submit_command_buffer(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkCommandBuffer command_buffer)
{
	check_vulkan_result(vkEndCommandBuffer(command_buffer), "failed to end command buffer");

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	check_vulkan_result(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE), "failed to submit queue");
	check_vulkan_result(vkQueueWaitIdle(queue), "failed to wait for queue");

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

static void transition_image_layout(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = begin_command_buffer(device, command_pool);

	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.oldLayout = old_layout;
	image_memory_barrier.newLayout = new_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags src_stage = 0;
	VkPipelineStageFlags dst_stage = 0;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = 0;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	submit_command_buffer(device, command_pool, queue, command_buffer);
}

static void copy_buffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer source_buffer, VkBuffer destination_buffer, VkDeviceSize size)
{
	VkCommandBuffer transfer_command_buffer = begin_command_buffer(device, command_pool);

	VkBufferCopy buffer_copy = {};
	buffer_copy.srcOffset = 0;
	buffer_copy.dstOffset = 0;
	buffer_copy.size = size;

	vkCmdCopyBuffer(transfer_command_buffer, source_buffer, destination_buffer, 1, &buffer_copy);

	submit_command_buffer(device, command_pool, queue, transfer_command_buffer);
}

static VkShaderModule create_shader_module(VkDevice device, std::vector<char> code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module: " + std::to_string(result));
	}

	return shader_module;
}

static std::vector<char> read_shader(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open shader file: " + filename);
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);

	if (file.fail())
	{
		throw std::runtime_error("failed to read shader file: " + filename);
	}

	file.close();
	return buffer;
}

static VkDeviceSize aligned_size(VkDeviceSize size, VkDeviceSize alignment = 256)
{
	if ((alignment & (alignment - 1)) != 0)
	{
		throw std::runtime_error("alignment must be a power of 2");
	}

	return (size + alignment - 1) & ~(alignment - 1);
}

static void create_uniform_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	check_vulkan_result(vkCreateBuffer(device, &buffer_info, nullptr, buffer), "failed to create uniform buffer");

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = find_memory_type(physical_device, mem_requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	check_vulkan_result(vkAllocateMemory(device, &alloc_info, nullptr, memory), "failed to allocate uniform buffer memory");
	check_vulkan_result(vkBindBufferMemory(device, *buffer, *memory, 0), "failed to bind uniform buffer memory");
}

inline static int load_texture_from_file(const std::string& filename, int& width, int& height, VkDeviceSize& image_size, stbi_uc*& image)
{
	int channels;
	image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	image_size = width * height * 4;

	if (image == nullptr)
	{
		std::string msg = "failed to load texture file:" + filename + "!";
		std::cout << msg << std::endl;
		return 0;
	}
	return 1;
}

inline static void copy_buffer_to_image(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer transfer_command_buffer = begin_command_buffer(device, command_pool);

	VkBufferImageCopy image_region = {};
	image_region.bufferOffset = 0;
	image_region.bufferRowLength = 0;
	image_region.bufferImageHeight = 0;
	image_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_region.imageSubresource.mipLevel = 0;
	image_region.imageSubresource.baseArrayLayer = 0;
	image_region.imageSubresource.layerCount = 1;
	image_region.imageOffset = { 0, 0, 0 };
	image_region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(transfer_command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_region);

	submit_command_buffer(device, command_pool, queue, transfer_command_buffer);
}

inline static void create_texture(VkPhysicalDevice physical_device, VkDevice device, VkQueue queue, VkCommandPool command_pool, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkSampler sampler, std::string file_name, VkDescriptorSet& descriptor_set, VkImageView& image_view, VkImage& image, VkDeviceMemory& device_memory)
{
	int image_width, image_height;
	VkDeviceSize image_size;
	stbi_uc* image_data;
	if (!load_texture_from_file(file_name, image_width, image_height, image_size, image_data)) return;

	VkBuffer image_data_staging_buffer = create_buffer(device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	VkDeviceMemory image_data_staging_buffer_device_memory = allocate_buffer_memory(physical_device, device, image_data_staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* data;
	vkMapMemory(device, image_data_staging_buffer_device_memory, 0, image_size, 0, &data);
	memcpy(data, image_data, static_cast<size_t>(image_size));
	vkUnmapMemory(device, image_data_staging_buffer_device_memory);
	stbi_image_free(image_data);

	image = create_image(device, image_width, image_height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	device_memory = allocate_image_memory(physical_device, device, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	transition_image_layout(device, queue, command_pool, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(device, queue, command_pool, image_data_staging_buffer, image, image_width, image_height);
	transition_image_layout(device, queue, command_pool, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, image_data_staging_buffer, nullptr);
	vkFreeMemory(device, image_data_staging_buffer_device_memory, nullptr);

	image_view = create_image_view(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

	check_vulkan_result(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set), "failed to create a descriptor set!");

	VkDescriptorImageInfo image_info = {};
	image_info.sampler = sampler;
	image_info.imageView = image_view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.dstBinding = 0;
	write_descriptor_set.dstArrayElement = 0;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set.pImageInfo = &image_info;

	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}




Renderer& Renderer::get()
{
	static Renderer instance;
	return instance;
}

void Renderer::initialize()
{
	try
	{
		create_vulkan_instance();
		check_vulkan_result(glfwCreateWindowSurface(instance, MarkoEngine::Window::get().window(), nullptr, &surface),
			"Failed to create vk surface");
		choose_physical_device();
		create_vulkan_device();
		create_vulkan_swapchain();
		create_vulkan_renderpass();
		create_vulkan_descriptor_resources();
		create_vulkan_pipelines();
		create_vulkan_command_buffers();
		create_vulkan_synchronization();
		create_imgui_instance();

		projection_matrix = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 1000000.0f);
		projection_matrix[1][1] *= -1;
	}
	catch (const std::exception& e)
	{
		std::cout << "fatal error: " << e.what() << std::endl;
	}
}

void Renderer::cleanup()
{
	vkDeviceWaitIdle(device);


	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	for (auto& fence : draw_fences) vkDestroyFence(device, fence, nullptr);
	
	for (auto& semaphore : image_available_semaphores) vkDestroySemaphore(device, semaphore, nullptr);
	
	for (auto& semaphore : render_finished_semaphores) vkDestroySemaphore(device, semaphore, nullptr);
	
	vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());

	vkDestroyCommandPool(device, command_pool, nullptr);

	for (auto& framebuffer : frame_buffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
	
	vkDestroyImageView(device, depth_view, nullptr);
	vkDestroyImage(device, depth_image, nullptr);
	vkFreeMemory(device, depth_device_memory, nullptr);

	vkDestroyPipeline(device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, graphics_pipeline_layout, nullptr);

	vkDestroyRenderPass(device, renderpass, nullptr);

	vkDestroyDescriptorPool(device, uniform_pool, nullptr);
	vkDestroyDescriptorSetLayout(device, uniform_descriptor_set_layout, nullptr);

	vkDestroyDescriptorPool(device, sampler_pool, nullptr);
	vkDestroyDescriptorSetLayout(device, sampler_descriptor_set_layout, nullptr);

	for (size_t i = 0; i < uniform_buffers.size(); i++)
	{
		vkDestroyBuffer(device, uniform_buffers[i], nullptr);
		vkFreeMemory(device, uniform_device_memories[i], nullptr);
	}

	vkDestroySampler(device, sampler, nullptr);

	for (VkImageView imageView : swapchain_views) 
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (debug_messenger != VK_NULL_HANDLE) 
	{
		auto destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (destroyDebugUtilsMessengerEXT) {
			destroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}
	}

	vkDestroyInstance(instance, nullptr);
}

void Renderer::update()
{
	{

		vkWaitForFences(Renderer::get().device, 1,
			&Renderer::get().draw_fences[Renderer::get().current_frame],
			VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(Renderer::get().device, 1,
			&Renderer::get().draw_fences[Renderer::get().current_frame]);


		vkAcquireNextImageKHR(Renderer::get().device, Renderer::get().swapchain,
			std::numeric_limits<uint64_t>::max(),
			Renderer::get().image_available_semaphores[Renderer::get().current_frame],
			VK_NULL_HANDLE, &Renderer::get().image_index);


		void* data;
		vkMapMemory(Renderer::get().device,
			Renderer::get().uniform_device_memories[Renderer::get().image_index],
			0, sizeof(glm::mat4) * 2, 0, &data);
		memcpy(data, &Renderer::get().view_matrix, sizeof(glm::mat4));
		memcpy(static_cast<char*>(data) + sizeof(glm::mat4),
			&Renderer::get().projection_matrix, sizeof(glm::mat4));
		vkUnmapMemory(Renderer::get().device,
			Renderer::get().uniform_device_memories[Renderer::get().image_index]);


		VkCommandBufferBeginInfo buffer_begin_info = {};
		buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		std::array<VkClearValue, 2> clear_values = {};
		clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clear_values[1].depthStencil.depth = 1.0f;

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = Renderer::get().renderpass;
		render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_begin_info.pClearValues = clear_values.data();
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent = Renderer::get().extent;
		render_pass_begin_info.framebuffer = Renderer::get().frame_buffers[Renderer::get().image_index];

		vkBeginCommandBuffer(Renderer::get().command_buffers[Renderer::get().image_index], &buffer_begin_info);


		vkCmdBeginRenderPass(Renderer::get().command_buffers[Renderer::get().image_index],
			&render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdBindPipeline(Renderer::get().command_buffers[Renderer::get().image_index],
			VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer::get().graphics_pipeline);

		transform t;
		glm::mat4 model_mat;
		for (auto& object : I_GAME_OBJECT::game_objects)
		{
			if (!object.second->is_visible)
				continue;

			model_mat = glm::mat4(1.0f);


			if (object.second->get_parent().empty())
			{
				object.second->set_world_transform(object.second->get_world_transform());
			}
			else
			{
				auto parent = I_GAME_OBJECT::game_objects.find(object.second->get_parent());
				if (parent != I_GAME_OBJECT::game_objects.end())
				{
					object.second->set_world_transform(
						parent->second->get_world_transform() + object.second->get_local_transform()
					);
				}
				else
				{
					object.second->set_world_transform(object.second->get_local_transform());
				}
			}
			t = object.second->get_world_transform();

			static Renderer_Mesh dummy_mesh;
			static int c = 0;

			static Vertex dummy_vertex = {
				.position = glm::vec3(0.0f, 0.0f, 0.0f), 
				.color = glm::vec3(1.0f, 1.0f, 1.0f),  
				.texture = glm::vec2(0.0f, 0.0f),      
				.bone_ids = glm::ivec4(0),              
				.weights = glm::vec4(0.0f)          
			};


			static std::vector<Vertex> dummy_vertices = { dummy_vertex };
			static std::vector<unsigned int> dummy_indices = { 0 };             


			if (c == 0)dummy_mesh = create_mesh("dependencies/bela.png", dummy_vertices, dummy_indices);
			c++;

			vkCmdPushConstants(Renderer::get().command_buffers[Renderer::get().image_index],
				Renderer::get().graphics_pipeline_layout,
				VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_mat);

			draw_mesh(dummy_mesh);

			if (object.second.get()->get_type() == game_object_type::MESH)
			{
				MESH_GAME_OBJECT* model = dynamic_cast<MESH_GAME_OBJECT*>(object.second.get());

				model_mat = glm::translate(model_mat, t.position);
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
				model_mat = glm::scale(model_mat, t.scale);

				vkCmdPushConstants(Renderer::get().command_buffers[Renderer::get().image_index],
					Renderer::get().graphics_pipeline_layout,
					VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_mat);

				model->Draw();
			}

			else if (object.second.get()->get_type() == game_object_type::MODEL)
			{
				MODEL_GAME_OBJECT* model = dynamic_cast<MODEL_GAME_OBJECT*>(object.second.get());


				model_mat = glm::translate(model_mat, t.position);
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
				model_mat = glm::rotate(model_mat, glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
				model_mat = glm::scale(model_mat, t.scale);


				vkCmdPushConstants(Renderer::get().command_buffers[Renderer::get().image_index],
					Renderer::get().graphics_pipeline_layout,
					VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_mat);


				model->Draw();
			}

			else if (object.second.get()->get_type() == game_object_type::ANIMATED)
			{
				ANIMATED_GAME_OBJECT* animated = dynamic_cast<ANIMATED_GAME_OBJECT*>(object.second.get());

				animated->Draw();
			}
		}

#ifndef EXPORT

		vkCmdBindPipeline(Renderer::get().command_buffers[Renderer::get().image_index],
			VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer::get().grid_pipeline);


		glm::mat4 identity = glm::mat4(1.0f);
		vkCmdPushConstants(Renderer::get().command_buffers[Renderer::get().image_index],
			Renderer::get().grid_pipeline_layout, 
			VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &identity);


		vkCmdDraw(Renderer::get().command_buffers[Renderer::get().image_index], 6, 1, 0, 0);
#endif


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

#ifndef EXPORT
		MarkoEngine::Gui::get().update();
#endif

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
			Renderer::get().command_buffers[Renderer::get().image_index]);


		vkCmdEndRenderPass(Renderer::get().command_buffers[Renderer::get().image_index]);

		vkEndCommandBuffer(Renderer::get().command_buffers[Renderer::get().image_index]);


		std::array<VkPipelineStageFlags, 1> pipeline_stages = {};
		pipeline_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &Renderer::get().image_available_semaphores[Renderer::get().current_frame];
		submit_info.pWaitDstStageMask = pipeline_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &Renderer::get().command_buffers[Renderer::get().image_index];
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &Renderer::get().render_finished_semaphores[Renderer::get().current_frame];

		vkQueueSubmit(Renderer::get().graphics_queue, 1, &submit_info,
			Renderer::get().draw_fences[Renderer::get().current_frame]);


		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &Renderer::get().render_finished_semaphores[Renderer::get().current_frame];
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &Renderer::get().swapchain;
		present_info.pImageIndices = &Renderer::get().image_index;

		vkQueuePresentKHR(Renderer::get().presentation_queue, &present_info);


		Renderer::get().current_frame = (Renderer::get().current_frame + 1) % Renderer::get().MAX_FRAMES;
	}
}



static VKAPI_ATTR VkBool32 VKAPI_CALL messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity, VkDebugUtilsMessageTypeFlagsEXT msg_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
	constexpr auto RED = "\033[31m";
	constexpr auto YELLOW = "\033[33m";
	constexpr auto GREEN = "\033[32m";
	constexpr auto CYAN = "\033[36m";
	constexpr auto RESET = "\033[0m";

	static const std::unordered_map<VkDebugUtilsMessageSeverityFlagBitsEXT, std::pair<const char*, const char*>> severity_map = {
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, {CYAN, "[VERBOSE]"}},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, {GREEN, "[INFO]"}},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, {YELLOW, "[WARNING]"}},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, {RED, "[ERROR]"}}
	};

	static const std::unordered_map<VkDebugUtilsMessageTypeFlagsEXT, std::string> type_map = {
		{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "GENERAL"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "VALIDATION"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "PERFORMANCE"}
	};

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm buf;
	localtime_s(&buf, &in_time_t);
	std::ostringstream timestamp;
	timestamp << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");

	const auto& [severity_color, severity_text] = [&]() -> std::pair<const char*, const char*> {
		auto it = severity_map.find(msg_severity);
		return it != severity_map.end() ? it->second : std::make_pair("", "[UNKNOWN]");
		}();

	std::string type_text = [&]() {
		auto it = type_map.find(msg_type);
		return it != type_map.end() ? it->second : "UNKNOWN";
		}();

	const std::string plain_prefix = "[" + timestamp.str() + "] " + severity_text + " " + type_text + ": ";
	const std::string color_prefix = "[" + timestamp.str() + "] " + severity_color + severity_text + RESET + " " + type_text + ": ";
	const size_t prefix_length = plain_prefix.length();
	constexpr size_t MAX_LINE_WIDTH = 160;
	const size_t wrap_width = (prefix_length < MAX_LINE_WIDTH) ? (MAX_LINE_WIDTH - prefix_length) : 40;

	std::istringstream input(callback_data->pMessage);
	std::ostringstream output;
	std::string line, word;
	bool first_line = true;

	while (input >> word)
	{
		if (line.length() + word.length() + (line.empty() ? 0 : 1) > wrap_width)
		{
			if (first_line) {
				output << color_prefix << line << "\n";
				first_line = false;
			}
			else {
				output << std::string(prefix_length, ' ') << line << "\n";
			}
			line.clear();
		}

		if (!line.empty()) line += " ";
		line += word;
	}

	if (!line.empty())
	{
		if (first_line)
			output << color_prefix << line << "\n";
		else
			output << std::string(prefix_length, ' ') << line << "\n";
	}

	if (msg_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		std::cerr << output.str() << std::endl;
	else
		std::cout << output.str() << std::endl;

	return VK_FALSE;
}

void Renderer::create_vulkan_instance() {

	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "lastni_game_engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "lastni_game_engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;


	uint32_t instance_version = 0;
	vkEnumerateInstanceVersion(&instance_version);
	if ((VK_VERSION_MAJOR(instance_version) < 1) ||
		(VK_VERSION_MAJOR(instance_version) == 1 &&
			VK_VERSION_MINOR(instance_version) < 3)) {
		throw std::runtime_error("Vulkan 1.3 is not supported!");
	}


	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);


	bool enable_validation = false;
#ifdef _DEBUG
	enable_validation = true;
#endif
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
	std::vector<const char*> validation_layers;
	std::vector<const char*> validation_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	if (enable_validation) {

		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		const std::vector<const char*> requested_layers = { "VK_LAYER_KHRONOS_validation" };
		for (const char* layer : requested_layers) {
			if (std::none_of(available_layers.begin(), available_layers.end(),
				[layer](const VkLayerProperties& available) {
					return strcmp(layer, available.layerName) == 0;
				})) {
				throw std::runtime_error(std::string("Missing required layer: ") + layer);
			}
		}
		validation_layers = requested_layers;


		uint32_t ext_count;
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
		std::vector<VkExtensionProperties> available_exts(ext_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, available_exts.data());

		for (const char* ext : validation_extensions) {
			if (std::none_of(available_exts.begin(), available_exts.end(),
				[ext](const VkExtensionProperties& available) {
					return strcmp(ext, available.extensionName) == 0;
				})) {
				throw std::runtime_error(std::string("Missing required extension: ") + ext);
			}
		}
		extensions.insert(extensions.end(), validation_extensions.begin(), validation_extensions.end());


		debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_create_info.pfnUserCallback = messenger_callback;
	}


	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();
	create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
	create_info.ppEnabledLayerNames = validation_layers.data();

	if (enable_validation) {
		create_info.pNext = &debug_create_info;
	}


	check_vulkan_result(vkCreateInstance(&create_info, nullptr, &instance),
		"Failed to create Vulkan instance");


	if (enable_validation) {
		auto create_debug_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

		if (!create_debug_messenger) {
			throw std::runtime_error("Failed to load debug messenger extension!");
		}


		debug_create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		check_vulkan_result(create_debug_messenger(instance, &debug_create_info, nullptr, &debug_messenger),
			"Failed to create debug messenger");
	}
}

void Renderer::choose_physical_device() {

	if (surface == VK_NULL_HANDLE) {
		throw std::runtime_error("Surface must be created before choosing physical device!");
	}

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0) {
		throw std::runtime_error("No Vulkan-capable devices found");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	struct DeviceInfo {
		VkPhysicalDevice device;
		int score;
		VkPhysicalDeviceProperties properties;
		uint32_t graphics_family;
		uint32_t present_family;
		VkSurfaceCapabilitiesKHR surface_caps;
	};

	std::vector<DeviceInfo> suitable_devices;
	const std::vector<const char*> required_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	for (const auto& device : devices) {
		DeviceInfo info{};
		info.device = device;
		info.graphics_family = UINT32_MAX;
		info.present_family = UINT32_MAX;


		vkGetPhysicalDeviceProperties(device, &info.properties);
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);


		uint32_t queue_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);
		std::vector<VkQueueFamilyProperties> queues(queue_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queues.data());

		for (uint32_t i = 0; i < queues.size(); ++i) {
			if (queues[i].queueCount == 0) continue;


			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				info.graphics_family = i;
			}


			VkBool32 present_support = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
			if (present_support) {
				info.present_family = i;
			}


			if (info.graphics_family != UINT32_MAX && info.present_family != UINT32_MAX) {
				break;
			}
		}


		uint32_t ext_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
		std::vector<VkExtensionProperties> extensions(ext_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, extensions.data());

		bool has_extensions = std::all_of(required_extensions.begin(), required_extensions.end(),
			[&](const char* ext) {
				return std::any_of(extensions.begin(), extensions.end(),
					[ext](const VkExtensionProperties& prop) {
						return strcmp(ext, prop.extensionName) == 0;
					});
			});


		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &info.surface_caps);


		if (info.graphics_family == UINT32_MAX ||
			info.present_family == UINT32_MAX ||
			!has_extensions ||
			!features.samplerAnisotropy ||
			info.surface_caps.maxImageCount < 1) {
			continue;
		}


		info.score = 0;
		if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) info.score += 1000;
		else if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) info.score += 500;


		VkPhysicalDeviceMemoryProperties mem_props;
		vkGetPhysicalDeviceMemoryProperties(device, &mem_props);
		for (uint32_t i = 0; i < mem_props.memoryHeapCount; ++i) {
			if (mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				info.score += static_cast<int>(mem_props.memoryHeaps[i].size >> 20); // MB
			}
		}

		suitable_devices.push_back(info);
	}

	if (suitable_devices.empty()) {
		throw std::runtime_error("No suitable GPU with required features and queue families");
	}


	std::sort(suitable_devices.begin(), suitable_devices.end(),
		[](const auto& a, const auto& b) { return a.score > b.score; });

	physical_device = suitable_devices[0].device;
	graphics_queue_family = suitable_devices[0].graphics_family;
	present_queue_family = suitable_devices[0].present_family;
	surface_capabilities = suitable_devices[0].surface_caps;


	const char* device_type = "Other";
	switch (suitable_devices[0].properties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "Discrete GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "Integrated GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "Virtual GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU"; break;
	}

	std::cout << "Selected Physical Device:\n"
		<< "  Name: " << suitable_devices[0].properties.deviceName << "\n"
		<< "  Type: " << device_type << "\n"
		<< "  Graphics Queue Family: " << graphics_queue_family << "\n"
		<< "  Present Queue Family: " << present_queue_family << "\n"
		<< "  Surface Capabilities:\n"
		<< "    Min Image Count: " << surface_capabilities.minImageCount << "\n"
		<< "    Max Image Count: " << surface_capabilities.maxImageCount << "\n"
		<< "    Current Extent: "
		<< surface_capabilities.currentExtent.width << "x"
		<< surface_capabilities.currentExtent.height << "\n";
}

void Renderer::create_vulkan_device() {

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("Physical device must be selected before creating logical device!");
	}


	std::set<uint32_t> unique_queue_families = { graphics_queue_family, present_queue_family };
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;


	const std::array<float, 1> queue_priorities = { 1.0f };

	for (uint32_t family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = family;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = queue_priorities.data();
		queue_create_infos.push_back(queue_info);
	}


	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.geometryShader = VK_TRUE;


	const std::array<const char*, 1> required_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

	for (const char* ext : required_extensions) {
		bool found = false;
		for (const auto& available : available_extensions) {
			if (strcmp(ext, available.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			throw std::runtime_error(std::string("Required device extension not available: ") + ext);
		}
	}


	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	create_info.ppEnabledExtensionNames = required_extensions.data();


	VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &device);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device! Error code: " +
			std::to_string(result));
	}


	vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);
	vkGetDeviceQueue(device, present_queue_family, 0, &presentation_queue);


	if (graphics_queue == VK_NULL_HANDLE || presentation_queue == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to retrieve valid queue handles!");
	}
}

void Renderer::create_vulkan_swapchain() {

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
	std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data());

	auto select_format = [&]() -> VkSurfaceFormatKHR {
		const std::array preferred_formats = {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_A2B10G10R10_UNORM_PACK32
		};

		for (const auto& format : surface_formats) {
			if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
				std::find(preferred_formats.begin(), preferred_formats.end(), format.format) != preferred_formats.end()) {
				return format;
			}
		}
		return surface_formats[0]; 
		};

	best_surface_format = select_format();

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
	std::vector<VkPresentModeKHR> present_modes(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());

	auto select_present_mode = [&]() -> VkPresentModeKHR {
		const std::array preferred_modes = {
			VK_PRESENT_MODE_MAILBOX_KHR,
			VK_PRESENT_MODE_IMMEDIATE_KHR,
			VK_PRESENT_MODE_FIFO_RELAXED_KHR
		};

		for (const auto& mode : preferred_modes) {
			if (std::find(present_modes.begin(), present_modes.end(), mode) != present_modes.end()) {
				return mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; 
		};

	VkPresentModeKHR best_present_mode = select_present_mode();


	int width, height;
	glfwGetFramebufferSize(MarkoEngine::Window::get().window(), &width, &height);
	VkExtent2D new_extent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	if (surface_capabilities.currentExtent.width != UINT32_MAX) {
		new_extent = surface_capabilities.currentExtent;
	}
	else {
		new_extent.width = std::clamp(new_extent.width,
			surface_capabilities.minImageExtent.width,
			surface_capabilities.maxImageExtent.width);
		new_extent.height = std::clamp(new_extent.height,
			surface_capabilities.minImageExtent.height,
			surface_capabilities.maxImageExtent.height);
	}


	uint32_t image_count = surface_capabilities.minImageCount + 1;
	if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
		image_count = surface_capabilities.maxImageCount;
	}


	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = best_surface_format.format;
	create_info.imageColorSpace = best_surface_format.colorSpace;
	create_info.imageExtent = new_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.preTransform = surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = best_present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = swapchain;

g
	const std::array<uint32_t, 2> queue_family_indices = {
		graphics_queue_family,
		present_queue_family
	};

	if (graphics_queue_family != present_queue_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
		create_info.pQueueFamilyIndices = queue_family_indices.data();
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}


	VkSwapchainKHR new_swapchain;
	check_vulkan_result(vkCreateSwapchainKHR(device, &create_info, nullptr, &new_swapchain),
		"Failed to create swapchain");


	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	}
	swapchain = new_swapchain;
	extent = new_extent;


	uint32_t actual_image_count;
	vkGetSwapchainImagesKHR(device, swapchain, &actual_image_count, nullptr);
	std::vector<VkImage> images(actual_image_count);
	vkGetSwapchainImagesKHR(device, swapchain, &actual_image_count, images.data());


	swapchain_views.clear();
	swapchain_views.reserve(images.size());

	for (auto image : images) {
		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = best_surface_format.format;
		view_info.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		view_info.subresourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1, 0, 1
		};

		VkImageView image_view;
		check_vulkan_result(vkCreateImageView(device, &view_info, nullptr, &image_view),
			"Failed to create swapchain image view");
		swapchain_views.push_back(image_view);
	}
}

void Renderer::create_vulkan_renderpass()
{

	VkAttachmentDescription color_attachment{};
	color_attachment.format = best_surface_format.format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_ref{};
	color_ref.attachment = 0;
	color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkFormat depth_format = find_depth_format(physical_device);
	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = depth_format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_ref{};
	depth_ref.attachment = 1;
	depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_ref;
	subpass.pDepthStencilAttachment = &depth_ref;

	std::array<VkSubpassDependency, 2> dependencies{};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
	render_pass_info.pDependencies = dependencies.data();

	check_vulkan_result(vkCreateRenderPass(device, &render_pass_info, nullptr, &renderpass),
		"Failed to create render pass");
}

void Renderer::create_vulkan_descriptor_resources() {
	size_t swapchain_image_count = swapchain_views.size();

	uniform_buffers.resize(swapchain_image_count);
	uniform_device_memories.resize(swapchain_image_count);
	animation_buffers.resize(swapchain_image_count);
	animation_device_memories.resize(swapchain_image_count);
	uniform_descriptor_sets.resize(swapchain_image_count);

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr
		}
	} };

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	check_vulkan_result(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &uniform_descriptor_set_layout),
		"Failed to create uniform descriptor set layout");

	VkDeviceSize vp_buffer_size = aligned_size(sizeof(glm::mat4) * 2);
	VkDeviceSize anim_buffer_size = aligned_size(sizeof(glm::mat4) * 100);

	for (size_t i = 0; i < swapchain_image_count; ++i) {
		create_uniform_buffer(physical_device, device, vp_buffer_size, &uniform_buffers[i], &uniform_device_memories[i]);
		create_uniform_buffer(physical_device, device, anim_buffer_size, &animation_buffers[i], &animation_device_memories[i]);
	}

	std::array<VkDescriptorPoolSize, 2> pool_sizes = { {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = static_cast<uint32_t>(swapchain_image_count * 2)
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(swapchain_image_count)
		}
	} };

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t>(swapchain_image_count),
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data()
	};

	check_vulkan_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &uniform_pool), "Failed to create descriptor pool");

	std::vector<VkDescriptorSetLayout> layouts(swapchain_image_count, uniform_descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = uniform_pool,
		.descriptorSetCount = static_cast<uint32_t>(swapchain_image_count),
		.pSetLayouts = layouts.data()
	};

	check_vulkan_result(vkAllocateDescriptorSets(device, &alloc_info, uniform_descriptor_sets.data()), "Failed to allocate descriptor sets");

	for (size_t i = 0; i < swapchain_image_count; ++i) {
		std::array<VkDescriptorBufferInfo, 2> buffer_infos = { {
			{uniform_buffers[i], 0, vp_buffer_size},
			{animation_buffers[i], 0, anim_buffer_size}
		} };

		std::array<VkWriteDescriptorSet, 2> descriptor_writes = { {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = uniform_descriptor_sets[i],
				.dstBinding = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &buffer_infos[0]
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = uniform_descriptor_sets[i],
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &buffer_infos[1]
			}
		} };

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

	VkDescriptorSetLayoutBinding sampler_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &sampler_binding
	};

	check_vulkan_result(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &sampler_descriptor_set_layout),
		"Failed to create sampler descriptor layout");

	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = supported_features.samplerAnisotropy ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = supported_features.samplerAnisotropy ? 16.0f : 1.0f,
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	check_vulkan_result(vkCreateSampler(device, &sampler_info, nullptr, &sampler), "Failed to create texture sampler");

	VkDescriptorPoolSize pool_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = MAX_TEXTURE_DESCRIPTORS
	};

	pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = MAX_TEXTURE_DESCRIPTORS,
		.poolSizeCount = 1,
		.pPoolSizes = &pool_size
	};

	check_vulkan_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &sampler_pool), "Failed to create sampler descriptor pool");

	model_push_constant_range = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(glm::mat4) + sizeof(int)
	};
}

void Renderer::create_vulkan_pipelines() 
{


	std::vector<char> vertex_code = read_shader("shaders/bin/normal.vert.spv");
	std::vector<char> fragment_code = read_shader("shaders/bin/normal.frag.spv");

	VkShaderModule vertex_shader_module = create_shader_module(device, vertex_code);
	VkShaderModule fragment_shader_module = create_shader_module(device, fragment_code);

	VkPipelineShaderStageCreateInfo vertex_shader_create_info{};
	vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_create_info.module = vertex_shader_module;
	vertex_shader_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_shader_create_info{};
	fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_create_info.module = fragment_shader_module;
	fragment_shader_create_info.pName = "main";

	VkVertexInputBindingDescription binding_description{};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 5> attribute_description{};
	attribute_description[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
	attribute_description[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };
	attribute_description[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texture) };
	attribute_description[3] = { 3, 0, VK_FORMAT_R32G32B32A32_SINT, offsetof(Vertex, bone_ids) };
	attribute_description[4] = { 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weights) };

	VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
	vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
	vertex_input_create_info.pVertexAttributeDescriptions = attribute_description.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = { vertex_shader_create_info, fragment_shader_create_info };

	VkPipelineViewportStateCreateInfo viewport_state_create_info{};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_create_info{};
	rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_create_info.depthClampEnable = VK_FALSE;
	rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_create_info.depthBiasEnable = VK_FALSE;
	rasterization_create_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_state{};
	color_state.blendEnable = VK_TRUE;
	color_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_state.colorBlendOp = VK_BLEND_OP_ADD;
	color_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_state.alphaBlendOp = VK_BLEND_OP_ADD;
	color_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
	color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_create_info.logicOpEnable = VK_FALSE;
	color_blending_create_info.attachmentCount = 1;
	color_blending_create_info.pAttachments = &color_state;

	std::array<VkDescriptorSetLayout, 2> descriptor_set_layouts = { uniform_descriptor_set_layout, sampler_descriptor_set_layout };

	std::vector<VkPushConstantRange> push_constant_ranges = { model_push_constant_range };

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
	pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
	pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();

	check_vulkan_result(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &graphics_pipeline_layout),
		"failed to create the graphics pipeline layout!");

	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
	depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_create_info.depthTestEnable = VK_TRUE;
	depth_stencil_create_info.depthWriteEnable = VK_TRUE;
	depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_create_info.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages = shader_stages.data();
	pipeline_create_info.pVertexInputState = &vertex_input_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_create_info;
	pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
	pipeline_create_info.pColorBlendState = &color_blending_create_info;
	pipeline_create_info.pDynamicState = nullptr;
	pipeline_create_info.layout = graphics_pipeline_layout;
	pipeline_create_info.renderPass = renderpass;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;

	check_vulkan_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline),
		"failed to create the graphics pipeline!");

	vkDestroyShaderModule(device, fragment_shader_module, nullptr);
	vkDestroyShaderModule(device, vertex_shader_module, nullptr);


	std::vector<char> grid_vertex_code = read_shader("shaders/bin/grid.vert.spv");
	std::vector<char> grid_fragment_code = read_shader("shaders/bin/grid.frag.spv");

	VkShaderModule grid_vertex_shader_module = create_shader_module(device, grid_vertex_code);
	VkShaderModule grid_fragment_shader_module = create_shader_module(device, grid_fragment_code);

	VkPipelineShaderStageCreateInfo grid_vertex_shader_create_info{};
	grid_vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	grid_vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	grid_vertex_shader_create_info.module = grid_vertex_shader_module;
	grid_vertex_shader_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo grid_fragment_shader_create_info{};
	grid_fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	grid_fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	grid_fragment_shader_create_info.module = grid_fragment_shader_module;
	grid_fragment_shader_create_info.pName = "main";

	VkPipelineVertexInputStateCreateInfo grid_vertex_input_create_info{};
	grid_vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	grid_vertex_input_create_info.vertexBindingDescriptionCount = 0;
	grid_vertex_input_create_info.pVertexBindingDescriptions = nullptr;
	grid_vertex_input_create_info.vertexAttributeDescriptionCount = 0;
	grid_vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo grid_input_assembly_create_info{};
	grid_input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	grid_input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	grid_input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo grid_viewport_state_create_info = viewport_state_create_info;
	VkPipelineRasterizationStateCreateInfo grid_rasterization_create_info = rasterization_create_info;
	VkPipelineMultisampleStateCreateInfo grid_multisample_state_create_info = multisample_state_create_info;
	VkPipelineDepthStencilStateCreateInfo grid_depth_stencil_create_info = depth_stencil_create_info;
	VkPipelineColorBlendStateCreateInfo grid_color_blending_create_info = color_blending_create_info;

	std::array<VkDescriptorSetLayout, 2> grid_descriptor_set_layouts = { uniform_descriptor_set_layout, sampler_descriptor_set_layout };

	VkPipelineLayoutCreateInfo grid_pipeline_layout_create_info{};
	grid_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	grid_pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(grid_descriptor_set_layouts.size());
	grid_pipeline_layout_create_info.pSetLayouts = grid_descriptor_set_layouts.data();
	grid_pipeline_layout_create_info.pushConstantRangeCount = 1;
	grid_pipeline_layout_create_info.pPushConstantRanges = &model_push_constant_range;

	check_vulkan_result(vkCreatePipelineLayout(device, &grid_pipeline_layout_create_info, nullptr, &grid_pipeline_layout),
		"failed to create the grid pipeline layout!");

	std::array<VkPipelineShaderStageCreateInfo, 2> grid_shader_stages = { grid_vertex_shader_create_info, grid_fragment_shader_create_info };

	VkGraphicsPipelineCreateInfo grid_pipeline_create_info{};
	grid_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	grid_pipeline_create_info.stageCount = static_cast<uint32_t>(grid_shader_stages.size());
	grid_pipeline_create_info.pStages = grid_shader_stages.data();
	grid_pipeline_create_info.pVertexInputState = &grid_vertex_input_create_info;
	grid_pipeline_create_info.pInputAssemblyState = &grid_input_assembly_create_info;
	grid_pipeline_create_info.pViewportState = &grid_viewport_state_create_info;
	grid_pipeline_create_info.pRasterizationState = &grid_rasterization_create_info;
	grid_pipeline_create_info.pMultisampleState = &grid_multisample_state_create_info;
	grid_pipeline_create_info.pDepthStencilState = &grid_depth_stencil_create_info;
	grid_pipeline_create_info.pColorBlendState = &grid_color_blending_create_info;
	grid_pipeline_create_info.pDynamicState = nullptr;
	grid_pipeline_create_info.layout = grid_pipeline_layout;
	grid_pipeline_create_info.renderPass = renderpass;
	grid_pipeline_create_info.subpass = 0;
	grid_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	grid_pipeline_create_info.basePipelineIndex = -1;

	check_vulkan_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &grid_pipeline_create_info, nullptr, &grid_pipeline),
		"failed to create the grid graphics pipeline");

	vkDestroyShaderModule(device, grid_fragment_shader_module, nullptr);
	vkDestroyShaderModule(device, grid_vertex_shader_module, nullptr);


	VkFormat depth_format = find_depth_format(physical_device);
	depth_image = create_image(device, extent.width, extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth_device_memory = allocate_image_memory(physical_device, device, depth_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageView depth_image_view = create_image_view(device, depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
	depth_view = depth_image_view;


	frame_buffers.resize(swapchain_views.size());

	for (size_t i = 0; i < frame_buffers.size(); i++) {
		std::array<VkImageView, 2> views = { swapchain_views[i], depth_view };

		VkFramebufferCreateInfo frame_buffer_create_info{};
		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.renderPass = renderpass;
		frame_buffer_create_info.attachmentCount = static_cast<uint32_t>(views.size());
		frame_buffer_create_info.pAttachments = views.data();
		frame_buffer_create_info.width = extent.width;
		frame_buffer_create_info.height = extent.height;
		frame_buffer_create_info.layers = 1;

		check_vulkan_result(vkCreateFramebuffer(device, &frame_buffer_create_info, nullptr, &frame_buffers[i]),
			"failed to create the frame buffer");
	}
}

void Renderer::create_vulkan_command_buffers()
{
	command_buffers.resize(frame_buffers.size());

	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = graphics_queue_family;

	check_vulkan_result(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool), "failed to create the command pool!");

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

	check_vulkan_result(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data()), "failed to create command buffer!");
}

void Renderer::create_vulkan_synchronization()
{

	{
		image_available_semaphores.resize(MAX_FRAMES);
		render_finished_semaphores.resize(MAX_FRAMES);
		draw_fences.resize(MAX_FRAMES);

		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fance_create_info = {};
		fance_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fance_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES; i++)
		{
			check_vulkan_result(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores[i]), "failed to create image available semaphore!");

			check_vulkan_result(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]), "failed to create render finished semaphore!");

			check_vulkan_result(vkCreateFence(device, &fance_create_info, nullptr, &draw_fences[i]), "failed to create draw fences!");
		}
	}
}

void Renderer::create_imgui_instance()
{

	{

		ImGui::CreateContext();


		ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF("dependencies/Roboto-Medium.ttf", 18.0f);
		ImGui::GetIO().FontDefault = font;

		std::array<VkDescriptorPoolSize, 11> pool_sizes = {};
		pool_sizes[0] = { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 };
		pool_sizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 };
		pool_sizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 };
		pool_sizes[3] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 };
		pool_sizes[4] = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 };
		pool_sizes[5] = { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 };
		pool_sizes[6] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 };
		pool_sizes[7] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 };
		pool_sizes[8] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 };
		pool_sizes[9] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 };
		pool_sizes[10] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 };

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = static_cast<uint32_t>(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = 1000;

		check_vulkan_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_descriptor_pool), "failed to create imgui descriptor pool!");

		ImGui_ImplGlfw_InitForVulkan(MarkoEngine::Window::get().window(), true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
		init_info.PhysicalDevice = physical_device;
		init_info.Device = device;
		init_info.QueueFamily = graphics_queue_family;
		init_info.Queue = graphics_queue;
		init_info.DescriptorPool = imgui_descriptor_pool;
		init_info.RenderPass = renderpass;
		init_info.MinImageCount = surface_capabilities.minImageCount;
		init_info.ImageCount = swapchain_views.size();

		ImGui_ImplVulkan_Init(&init_info);
	}
}

unsigned long long Renderer::create_gui_texture(std::string filename)
{
	Renderer_Gui_Texture imgui_texture;
	imgui_texture.channels = 4;
	unsigned char* image_data = stbi_load(filename.c_str(), &imgui_texture.width, &imgui_texture.height, 0, imgui_texture.channels);

	if (image_data == nullptr)
	{
		throw std::runtime_error("failed to create imgui texture");
	}

	size_t image_size = imgui_texture.width * imgui_texture.height * imgui_texture.channels;

	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = imgui_texture.width;
		info.extent.height = imgui_texture.height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		check_vulkan_result(vkCreateImage(device, &info, nullptr, &imgui_texture.image), "failed to create image!");

		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(device, imgui_texture.image, &req);
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;

		alloc_info.memoryTypeIndex = find_memory_type(physical_device, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		check_vulkan_result(vkAllocateMemory(device, &alloc_info, nullptr, &imgui_texture.image_device_memory), "gui");

		check_vulkan_result(vkBindImageMemory(device, imgui_texture.image, imgui_texture.image_device_memory, 0), "gui");
	}


	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = imgui_texture.image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;

		check_vulkan_result(vkCreateImageView(device, &info, nullptr, &imgui_texture.image_view), "gui");
	}


	{
		VkSamplerCreateInfo sampler_info{};
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.minLod = -1000;
		sampler_info.maxLod = 1000;
		sampler_info.maxAnisotropy = 1.0f;

		check_vulkan_result(vkCreateSampler(device, &sampler_info, nullptr, &imgui_texture.sampler), "gui");
	}


	imgui_texture.destriptor_set = ImGui_ImplVulkan_AddTexture(imgui_texture.sampler, imgui_texture.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = image_size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		check_vulkan_result(vkCreateBuffer(device, &buffer_info, nullptr, &imgui_texture.buffer), "gui");

		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device, imgui_texture.buffer, &req);
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;

		alloc_info.memoryTypeIndex = find_memory_type(physical_device, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		check_vulkan_result(vkAllocateMemory(device, &alloc_info, nullptr, &imgui_texture.buffer_device_memory), "gui");

		check_vulkan_result(vkBindBufferMemory(device, imgui_texture.buffer, imgui_texture.buffer_device_memory, 0), "gui");
	}


	{
		void* map = NULL;

		if (vkMapMemory(device, imgui_texture.buffer_device_memory, 0, image_size, 0, &map))
		{
			throw std::runtime_error("failed to create imgui texture");
		}


		memcpy(map, image_data, image_size);
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = imgui_texture.buffer_device_memory;
		range[0].size = image_size;
		check_vulkan_result(vkFlushMappedMemoryRanges(device, 1, range), "gui");
		vkUnmapMemory(device, imgui_texture.buffer_device_memory);
	}


	stbi_image_free(image_data);

	VkCommandBuffer command_buffer;
	{
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = command_pool;
		alloc_info.commandBufferCount = 1;

		check_vulkan_result(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer), "gui");

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		check_vulkan_result(vkBeginCommandBuffer(command_buffer, &begin_info), "gui");
	}

	{
		VkImageMemoryBarrier copy_barrier[1] = {};
		copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].image = imgui_texture.image;
		copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_barrier[0].subresourceRange.levelCount = 1;
		copy_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = imgui_texture.width;
		region.imageExtent.height = imgui_texture.height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(command_buffer, imgui_texture.buffer, imgui_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier use_barrier[1] = {};
		use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].image = imgui_texture.image;
		use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		use_barrier[0].subresourceRange.levelCount = 1;
		use_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
	}

	{
		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &command_buffer;
		check_vulkan_result(vkEndCommandBuffer(command_buffer), "gui");

		check_vulkan_result(vkQueueSubmit(graphics_queue, 1, &end_info, VK_NULL_HANDLE), "gui");

		check_vulkan_result(vkDeviceWaitIdle(device), "gui");
	}
	std::cout << filename << std::endl;

	renderer_gui_textures.push_back(imgui_texture);

	return (unsigned long long)renderer_gui_textures.at(renderer_gui_textures.size() - 1).destriptor_set;
}

void Renderer::set_view_matrix(glm::mat4 new_view_matrix)
{
	view_matrix = new_view_matrix;
}


	
void Renderer::draw_mesh(Renderer_Mesh& mesh)
{
	std::vector<VkBuffer> vertex_buffers = {
		this->vertex_buffers.at(mesh.vertex_buffer_index)
	};

	std::vector<VkDeviceSize> offsets = {
		0
	};

	vkCmdBindVertexBuffers(command_buffers.at(image_index), 0, 1, vertex_buffers.data(), offsets.data());
	vkCmdBindIndexBuffer(command_buffers.at(image_index), index_buffers.at(mesh.index_buffer_index), 0, VK_INDEX_TYPE_UINT32);

	std::vector<VkDescriptorSet> descriptor_sets = {
		uniform_descriptor_sets.at(current_frame),
		texture_descriptor_sets.at(mesh.texture_index)
	};

	vkCmdBindDescriptorSets(command_buffers.at(image_index), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
	vkCmdDrawIndexed(command_buffers.at(image_index), mesh.index_count, 1, 0, 0, 0);
}

Renderer_Mesh Renderer::create_mesh(std::string texture_filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    Renderer_Mesh new_renderer_mesh;
    new_renderer_mesh.vertex_count = static_cast<uint32_t>(vertices.size());
    new_renderer_mesh.index_count = static_cast<uint32_t>(indices.size());

    VkImage texture_image;
    VkImageView texture_image_view;
    VkDeviceMemory texture_image_memory;
    VkDescriptorSet texture_descriptor_set;

    create_texture(physical_device, device, graphics_queue, command_pool, sampler_pool, sampler_descriptor_set_layout, sampler, texture_filename, texture_descriptor_set, texture_image_view, texture_image, texture_image_memory);

    texture_image_views.push_back(texture_image_view);
    texture_images.push_back(texture_image);
    texture_image_memories.push_back(texture_image_memory);
    texture_descriptor_sets.push_back(texture_descriptor_set);
    new_renderer_mesh.texture_index = static_cast<uint32_t>(texture_image_views.size() - 1);

    VkDeviceSize buffer_size = sizeof(Vertex) * vertices.size();
    VkBuffer staging_buffer = create_buffer(device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    VkDeviceMemory staging_buffer_memory = allocate_buffer_memory(physical_device, device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    VkBuffer vertex_buffer = create_buffer(device, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    VkDeviceMemory vertex_buffer_memory = allocate_buffer_memory(physical_device, device, vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(device, graphics_queue, command_pool, staging_buffer, vertex_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    buffer_size = sizeof(uint32_t) * indices.size();
    staging_buffer = create_buffer(device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    staging_buffer_memory = allocate_buffer_memory(physical_device, device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    VkBuffer index_buffer = create_buffer(device, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    VkDeviceMemory index_buffer_memory = allocate_buffer_memory(physical_device, device, index_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(device, graphics_queue, command_pool, staging_buffer, index_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    new_renderer_mesh.vertex_buffer_index = static_cast<uint32_t>(vertex_buffers.size());
    new_renderer_mesh.index_buffer_index = static_cast<uint32_t>(index_buffers.size());
    vertex_buffers.push_back(vertex_buffer);
    index_buffers.push_back(index_buffer);
    vertex_buffer_memories.push_back(vertex_buffer_memory);
    index_buffer_memories.push_back(index_buffer_memory);

    return new_renderer_mesh;
}

void Renderer::draw_model(Renderer_Model& model)
{
	for (auto& renderer_mesh : model.renderer_meshes)
	{
		draw_mesh(renderer_mesh);
	}
}

Renderer_Model Renderer::create_model(std::string model_filename)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		model_filename,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "Failed to load model: " << model_filename << "!" << std::endl;
		return Renderer_Model();
	}

	std::filesystem::path file_path(model_filename);
	std::string directory_path = file_path.parent_path().string();


	std::vector<std::string> texture_filenames(scene->mNumMaterials);
	for (size_t i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		texture_filenames[i] = "";

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
				int idx = std::string(path.data).rfind("\\");
				std::string fileName = std::string(path.data).substr(idx + 1);
				texture_filenames[i] = directory_path + "/" + fileName;
			}
		}
	}


	std::vector<uint32_t> vertex_buffer_indices;
	std::vector<uint32_t> index_buffer_indices;
	std::vector<uint32_t> texture_indices;
	std::vector<uint32_t> vertex_counts;
	std::vector<uint32_t> index_counts;


	std::function<void(aiNode*)> processNode = [&](aiNode* node) {
		for (size_t i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;


			vertices.resize(mesh->mNumVertices);
			for (size_t j = 0; j < mesh->mNumVertices; j++) {
				vertices[j].position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };

				if (mesh->mTextureCoords[0]) {
					vertices[j].texture = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
				}
				else {
					vertices[j].texture = { 0.0f, 0.0f };
				}

				vertices[j].color = { 1.0f, 1.0f, 1.0f }; 
			}


			for (size_t j = 0; j < mesh->mNumFaces; j++) {
				aiFace face = mesh->mFaces[j];
				for (size_t k = 0; k < face.mNumIndices; k++) {
					indices.push_back(face.mIndices[k]);
				}
			}


			std::string texture_filename = texture_filenames[mesh->mMaterialIndex];


			Renderer_Mesh new_renderer_mesh = create_mesh(texture_filename, vertices, indices);


			vertex_buffer_indices.push_back(new_renderer_mesh.vertex_buffer_index);
			index_buffer_indices.push_back(new_renderer_mesh.index_buffer_index);
			texture_indices.push_back(new_renderer_mesh.texture_index);
			vertex_counts.push_back(new_renderer_mesh.vertex_count);
			index_counts.push_back(new_renderer_mesh.index_count);
		}


		for (size_t i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i]);
		}
		};


	processNode(scene->mRootNode);


	Renderer_Model model;
	for (size_t i = 0; i < vertex_buffer_indices.size(); i++) {
		Renderer_Mesh mesh;
		mesh.vertex_buffer_index = vertex_buffer_indices[i];
		mesh.index_buffer_index = index_buffer_indices[i];
		mesh.texture_index = texture_indices[i];
		mesh.vertex_count = vertex_counts[i];
		mesh.index_count = index_counts[i];
		model.renderer_meshes.push_back(mesh);
	}

	return model;
}

void Renderer::draw_animation(Renderer_Animation& animation)
{

	if (!animation.animations.empty())
	{

		Animation& anim = animation.animations[0];
		animation.current_animation_time += MarkoEngine::Window::get().delta_time() * anim.ticksPerSecond;
		animation.current_animation_time = fmod(animation.current_animation_time, anim.duration);


		for (size_t i = 0; i < animation.final_bone_matrices.size(); i++)
		{
			animation.final_bone_matrices[i] = glm::mat4(1.0f);
		}


		glm::mat4 correction = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));


		std::function<void(const aiNode*, const glm::mat4&)> traverseNode =
			[&](const aiNode* node, const glm::mat4& parentTransform)
			{

				aiMatrix4x4 aiMat = node->mTransformation;
				glm::mat4 nodeTransform = glm::mat4(
					aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
					aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
					aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
					aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
				);


				glm::mat4 animTransform = glm::mat4(1.0f);
				for (auto& channel : anim.boneAnimations)
				{
					if (channel.boneName == node->mName.C_Str())
					{

						for (size_t i = 0; i < channel.keyframes.size() - 1; i++)
						{
							if (animation.current_animation_time >= channel.keyframes[i].time &&
								animation.current_animation_time < channel.keyframes[i + 1].time)
							{
								float factor = (animation.current_animation_time - channel.keyframes[i].time) /
									(channel.keyframes[i + 1].time - channel.keyframes[i].time);
								glm::vec3 interpolatedPos = glm::mix(channel.keyframes[i].position, channel.keyframes[i + 1].position, factor);
								glm::quat interpolatedRot = glm::slerp(channel.keyframes[i].rotation, channel.keyframes[i + 1].rotation, factor);
								glm::vec3 interpolatedScale = glm::mix(channel.keyframes[i].scale, channel.keyframes[i + 1].scale, factor);

								animTransform = glm::translate(glm::mat4(1.0f), interpolatedPos)
									* glm::toMat4(interpolatedRot)
									* glm::scale(glm::mat4(1.0f), interpolatedScale);
								break;
							}
						}
						break;
					}
				}

				glm::mat4 combinedLocalTransform = animTransform;
				glm::mat4 globalTransform = parentTransform * combinedLocalTransform;


				auto it = animation.bone_mapping.find(node->mName.C_Str());
				if (it != animation.bone_mapping.end())
				{
					int boneIndex = it->second;
					animation.final_bone_matrices[boneIndex] =
						animation.global_inverse_transform * globalTransform * animation.bone_offset_matrices[boneIndex];
				}


				for (unsigned int i = 0; i < node->mNumChildren; i++)
				{
					traverseNode(node->mChildren[i], globalTransform);
				}
			};


		traverseNode(animation.root_node, correction);


		void* data;
		vkMapMemory(Renderer::get().device,
			Renderer::get().animation_device_memories[Renderer::get().image_index],
			0,
			sizeof(glm::mat4) * animation.final_bone_matrices.size(),
			0,
			&data);
		memcpy(data, animation.final_bone_matrices.data(), sizeof(glm::mat4) * animation.final_bone_matrices.size());
		vkUnmapMemory(Renderer::get().device, Renderer::get().animation_device_memories[Renderer::get().image_index]);
	}


	for (size_t i = 0; i < animation.renderer_meshes.size(); i++)
	{
		const Renderer_Mesh& mesh = animation.renderer_meshes[i];
		std::array<VkBuffer, 1> vertex_buffers = { Renderer::get().vertex_buffers[mesh.vertex_buffer_index] };
		std::array<VkDeviceSize, 1> offsets = { 0 };

		vkCmdBindVertexBuffers(Renderer::get().command_buffers[Renderer::get().image_index],
			0, 1, vertex_buffers.data(), offsets.data());
		vkCmdBindIndexBuffer(Renderer::get().command_buffers[Renderer::get().image_index],
			Renderer::get().index_buffers[mesh.index_buffer_index],
			0, VK_INDEX_TYPE_UINT32);

		std::array<VkDescriptorSet, 2> descriptor_sets = {
			Renderer::get().uniform_descriptor_sets[Renderer::get().current_frame],
			Renderer::get().texture_descriptor_sets[mesh.texture_index]
		};

		vkCmdBindDescriptorSets(Renderer::get().command_buffers[Renderer::get().image_index],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			Renderer::get().graphics_pipeline_layout,
			0, static_cast<uint32_t>(descriptor_sets.size()),
			descriptor_sets.data(), 0, nullptr);


		transform t = animation.t;
		glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), t.position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), t.scale);

		glm::mat4 correction = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 correctedModelMat = correction * model_mat;

		struct PushConstants {
			glm::mat4 model;
			int isAnimated;
		} pushConstants;
		pushConstants.model = correctedModelMat;
		pushConstants.isAnimated = 1;

		vkCmdPushConstants(Renderer::get().command_buffers[Renderer::get().image_index],
			Renderer::get().graphics_pipeline_layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0, sizeof(PushConstants), &pushConstants);

		vkCmdDrawIndexed(Renderer::get().command_buffers[Renderer::get().image_index],
			mesh.index_count, 1, 0, 0, 0);
	}
}

Renderer_Animation Renderer::create_animation(std::string animation_filename)
{
	Renderer_Animation result;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		animation_filename,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "Failed to load model: " << animation_filename << "!" << std::endl;

		return result;
	}


	result.scene = importer.GetOrphanedScene();
	result.root_node = scene->mRootNode;


	aiMatrix4x4 rootMat = scene->mRootNode->mTransformation;
	glm::mat4 rootTransform;
	rootTransform[0][0] = rootMat.a1; rootTransform[1][0] = rootMat.a2; rootTransform[2][0] = rootMat.a3; rootTransform[3][0] = rootMat.a4;
	rootTransform[0][1] = rootMat.b1; rootTransform[1][1] = rootMat.b2; rootTransform[2][1] = rootMat.b3; rootTransform[3][1] = rootMat.b4;
	rootTransform[0][2] = rootMat.c1; rootTransform[1][2] = rootMat.c2; rootTransform[2][2] = rootMat.c3; rootTransform[3][2] = rootMat.c4;
	rootTransform[0][3] = rootMat.d1; rootTransform[1][3] = rootMat.d2; rootTransform[2][3] = rootMat.d3; rootTransform[3][3] = rootMat.d4;

	result.global_inverse_transform = glm::inverse(rootTransform);


	std::filesystem::path file_path(animation_filename);
	std::string directory_path = file_path.parent_path().string();


	std::vector<std::string> texture_filenames(scene->mNumMaterials);
	for (size_t i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		texture_filenames[i] = "";

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
				std::string texturePath = path.C_Str();
				std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
				if (texturePath[0] != '*') {

					if (texturePath.find(".fbm") != std::string::npos) {
						std::string textureFileName = texturePath.substr(texturePath.find(".fbm") + 5);
						texture_filenames[i] = directory_path + "/" + textureFileName;
					}
					else {
						texture_filenames[i] = directory_path + "/" + texturePath;
					}
					std::cout << "Texture path: " << texture_filenames[i] << std::endl;
				}
				else {

					int texIndex = std::atoi(texturePath.c_str() + 1);
					if (scene->HasTextures() && texIndex < scene->mNumTextures) {
						aiTexture* embeddedTex = scene->mTextures[texIndex];
						std::string textureFilename = directory_path + "/embedded_texture_" + std::to_string(i) + ".png";
						FILE* file;
#ifdef _MSC_VER
						errno_t err = fopen_s(&file, textureFilename.c_str(), "wb");
						if (err != 0 || !file) {
							std::cerr << "Failed to open file for writing embedded texture!" << std::endl;
							throw std::runtime_error("failed to load model");
						}
#else
						file = fopen(textureFilename.c_str(), "wb");
						if (!file) {
							std::cerr << "Failed to open file for writing embedded texture!" << std::endl;
							throw std::runtime_error("failed to load model");
						}
#endif
						fwrite(embeddedTex->pcData, 1, embeddedTex->mWidth, file);
						fclose(file);
						texture_filenames[i] = textureFilename;
					}
				}
			}
		}
	}


	std::function<void(aiNode*)> processNode = [&](aiNode* node) {

		for (size_t i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];


			std::vector<Vertex> vertices(mesh->mNumVertices);
			std::vector<uint32_t> indices;


			for (size_t j = 0; j < mesh->mNumVertices; j++) {
				vertices[j].position = {
					mesh->mVertices[j].x,
					mesh->mVertices[j].y,
					mesh->mVertices[j].z
				};

				if (mesh->mTextureCoords[0]) {
					vertices[j].texture = {
						mesh->mTextureCoords[0][j].x,
						mesh->mTextureCoords[0][j].y
					};
				}
				else {
					vertices[j].texture = { 0.0f, 0.0f };
				}


				vertices[j].color = { 1.0f, 1.0f, 1.0f };
			}


			for (size_t j = 0; j < mesh->mNumFaces; j++) {
				aiFace face = mesh->mFaces[j];
				for (size_t k = 0; k < face.mNumIndices; k++) {
					indices.push_back(face.mIndices[k]);
				}
			}


			if (mesh->HasBones()) {
				for (unsigned int b = 0; b < mesh->mNumBones; b++) {
					aiBone* bone = mesh->mBones[b];
					std::string boneName(bone->mName.C_Str());
					int boneID = 0;
					if (result.bone_mapping.find(boneName) == result.bone_mapping.end()) {

						boneID = static_cast<int>(result.bone_mapping.size());
						result.bone_mapping[boneName] = boneID;


						aiMatrix4x4 boneMat = bone->mOffsetMatrix;
						glm::mat4 offsetMat;
						offsetMat[0][0] = boneMat.a1; offsetMat[1][0] = boneMat.a2; offsetMat[2][0] = boneMat.a3; offsetMat[3][0] = boneMat.a4;
						offsetMat[0][1] = boneMat.b1; offsetMat[1][1] = boneMat.b2; offsetMat[2][1] = boneMat.b3; offsetMat[3][1] = boneMat.b4;
						offsetMat[0][2] = boneMat.c1; offsetMat[1][2] = boneMat.c2; offsetMat[2][2] = boneMat.c3; offsetMat[3][2] = boneMat.c4;
						offsetMat[0][3] = boneMat.d1; offsetMat[1][3] = boneMat.d2; offsetMat[2][3] = boneMat.d3; offsetMat[3][3] = boneMat.d4;
						result.bone_offset_matrices.push_back(offsetMat);
					}
					else {
						boneID = result.bone_mapping[boneName];
					}


					for (unsigned int w = 0; w < bone->mNumWeights; w++) {
						unsigned int vertexID = bone->mWeights[w].mVertexId;
						float weight = bone->mWeights[w].mWeight;

				
						for (int slot = 0; slot < 4; slot++) {
							if (vertices[vertexID].weights[slot] == 0.0f) {
								vertices[vertexID].bone_ids[slot] = boneID;
								vertices[vertexID].weights[slot] = weight;
								break;
							}
						}
					}
				}
			}


			std::string texture_filename = texture_filenames[mesh->mMaterialIndex];
			Renderer_Mesh new_renderer_mesh = create_mesh(texture_filename, vertices, indices);
			result.renderer_meshes.push_back(new_renderer_mesh);
		}

		for (size_t i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i]);
		}
		};


	processNode(scene->mRootNode);

	if (scene->HasAnimations()) {
		for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
			aiAnimation* anim = scene->mAnimations[i];
			Animation animation;
			animation.name = anim->mName.C_Str();
			animation.duration = static_cast<float>(anim->mDuration);
			animation.ticksPerSecond = (anim->mTicksPerSecond != 0)
				? static_cast<float>(anim->mTicksPerSecond)
				: 25.0f;


			for (unsigned int j = 0; j < anim->mNumChannels; j++) {
				aiNodeAnim* nodeAnim = anim->mChannels[j];
				BoneAnimation boneAnim;
				boneAnim.boneName = nodeAnim->mNodeName.C_Str();
				unsigned int numKeys = nodeAnim->mNumPositionKeys;
				for (unsigned int k = 0; k < numKeys; k++) {
					Keyframe frame;
					frame.time = static_cast<float>(nodeAnim->mPositionKeys[k].mTime);
					frame.position = glm::vec3(
						nodeAnim->mPositionKeys[k].mValue.x,
						nodeAnim->mPositionKeys[k].mValue.y,
						nodeAnim->mPositionKeys[k].mValue.z
					);
					aiQuaternion aiRot = nodeAnim->mRotationKeys[k].mValue;
					frame.rotation = glm::quat(aiRot.w, aiRot.x, aiRot.y, aiRot.z);
					frame.scale = glm::vec3(
						nodeAnim->mScalingKeys[k].mValue.x,
						nodeAnim->mScalingKeys[k].mValue.y,
						nodeAnim->mScalingKeys[k].mValue.z
					);
					boneAnim.keyframes.push_back(frame);
				}
				animation.boneAnimations.push_back(boneAnim);
			}
			result.animations.push_back(animation);
		}
	}


	result.final_bone_matrices.resize(result.bone_offset_matrices.size(), glm::mat4(1.0f));

	return result;
}
