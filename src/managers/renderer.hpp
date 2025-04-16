#pragma once

#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../game_objects/i_game_object.hpp"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texture;
	glm::ivec4 bone_ids = glm::ivec4(0.0f);
	glm::vec4 weights = glm::vec4(0.0f);
};


struct Keyframe
{
	float time;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
};

struct BoneAnimation
{
	std::string boneName;

	std::vector<Keyframe> keyframes;
};

struct Animation
{
	std::string name;
	float duration;
	float ticksPerSecond;
	std::vector<BoneAnimation> boneAnimations;
};





struct Renderer_Mesh
{
	uint32_t vertex_buffer_index;
	uint32_t index_buffer_index;
	uint32_t texture_index;
	uint32_t vertex_count;
	uint32_t index_count;
};

struct Renderer_Model
{
	std::vector<Renderer_Mesh> renderer_meshes;
};

struct Renderer_Animation
{
	std::vector<Renderer_Mesh> renderer_meshes;
	std::vector<Animation> animations;
	std::unordered_map<std::string, int> bone_mapping;
	std::vector<glm::mat4> bone_offset_matrices;
	const aiScene* scene;
	aiNode* root_node;
	glm::mat4 global_inverse_transform;
	float current_animation_time = 0.0f;
	std::vector<glm::mat4> final_bone_matrices;
	transform t;
};

struct Renderer_Gui_Texture
{
	VkDescriptorSet destriptor_set;
	int width;
	int height;
	int channels;
	VkImageView image_view;
	VkImage image;
	VkDeviceMemory image_device_memory;
	VkSampler sampler;
	VkBuffer buffer;
	VkDeviceMemory buffer_device_memory;
};

class Renderer
{
public: 
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(const Renderer&&) = delete;
private:
	Renderer() = default;
public: 
	[[nodiscard]] static Renderer& get();
	void initialize();
	void update();
	void cleanup();

private: 
	void create_vulkan_instance();
	void choose_physical_device();
	void create_vulkan_device();
	void create_vulkan_swapchain();
	void create_vulkan_renderpass();
	void create_vulkan_descriptor_resources();
	void create_vulkan_pipelines();
	void create_vulkan_command_buffers();
	void create_vulkan_synchronization();
	void create_imgui_instance();
	const uint32_t MAX_FRAMES = 2;
	const uint32_t MAX_TEXTURE_DESCRIPTORS = 1000;
	VkInstance instance {};
	VkDebugUtilsMessengerEXT debug_messenger {};
	VkSurfaceKHR surface {};
	VkPhysicalDevice physical_device {};
	uint32_t graphics_queue_family = -1;
	uint32_t present_queue_family = -1;
	std::vector<VkPresentModeKHR> physical_device_present_modes {};
	VkSurfaceCapabilitiesKHR surface_capabilities {};
	VkDevice device {};
	VkQueue	graphics_queue {};
	VkQueue	presentation_queue {};
	VkExtent2D extent {};
	VkSurfaceFormatKHR best_surface_format {};
	std::vector<VkBuffer> vertex_buffers {};
	std::vector<VkBuffer> index_buffers {};
	std::vector<VkDeviceMemory>	vertex_buffer_memories {};
	std::vector<VkDeviceMemory>	index_buffer_memories {};
	std::vector<VkImage> texture_images {};
	std::vector<VkImageView> texture_image_views {};
	std::vector<VkDeviceMemory>	texture_image_memories {};
	std::vector<VkDescriptorSet> texture_descriptor_sets {};
	VkSwapchainKHR swapchain {};
	std::vector<VkImageView> swapchain_views {};
	VkRenderPass renderpass {};
	VkDescriptorSetLayout uniform_descriptor_set_layout {};
	std::vector<VkBuffer> uniform_buffers {};
	std::vector<VkDeviceMemory> uniform_device_memories {};
	std::vector<VkBuffer> animation_buffers {};
	std::vector<VkDeviceMemory>	animation_device_memories {};
	VkDescriptorPool uniform_pool {};
	std::vector<VkDescriptorSet> uniform_descriptor_sets {};
	VkDescriptorSetLayout sampler_descriptor_set_layout {};
	VkSampler sampler {};
	VkDescriptorPool sampler_pool {};
	VkPushConstantRange	model_push_constant_range {};
	VkPipelineLayout graphics_pipeline_layout {};
	VkPipeline graphics_pipeline {};
	VkPipelineLayout grid_pipeline_layout {};
	VkPipeline grid_pipeline {};
	VkImage depth_image {};
	VkImageView	depth_view {};
	VkDeviceMemory depth_device_memory {};
	std::vector<VkFramebuffer> frame_buffers {};
	VkCommandPool command_pool {};
	std::vector<VkCommandBuffer> command_buffers {};
	std::vector<VkSemaphore> image_available_semaphores {};
	std::vector<VkSemaphore> render_finished_semaphores {};
	std::vector<VkFence> draw_fences {};
	VkDescriptorPool imgui_descriptor_pool {};
	uint32_t image_index = 0;
	uint32_t current_frame = 0;

public: 
	void draw_mesh(Renderer_Mesh& mesh);
	[[nodiscard]] Renderer_Mesh create_mesh(std::string texture_filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	void draw_model(Renderer_Model& model);
	[[nodiscard]] Renderer_Model create_model(std::string model_filename);

	void draw_animation(Renderer_Animation& animation);
	[[nodiscard]] Renderer_Animation create_animation(std::string animation_filename);

public: 
	[[nodiscard]] unsigned long long create_gui_texture(std::string filename);
private:
	std::vector<Renderer_Gui_Texture> renderer_gui_textures {};

public: 
	void set_view_matrix(glm::mat4 new_view_matrix);
private:
	glm::mat4 view_matrix {};
	glm::mat4 projection_matrix {};
};