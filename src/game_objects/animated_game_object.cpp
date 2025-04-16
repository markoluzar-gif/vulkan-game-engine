#include "pch.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "animated_game_object.hpp"
#include <glm/gtx/quaternion.hpp>
#include <stb/stb_image.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <iostream>
#include "../managers/window.hpp"

ANIMATED_GAME_OBJECT::ANIMATED_GAME_OBJECT(const std::string& model) : I_GAME_OBJECT(game_object_type::ANIMATED)
{

    renderer_animation = Renderer::get().create_animation(model);
    this->model = model;

    renderer_animation.final_bone_matrices.resize(100, glm::mat4(1.0f));
}

void ANIMATED_GAME_OBJECT::Draw()
{
    renderer_animation.t = get_world_transform();
    Renderer::get().draw_animation(renderer_animation);
}

void ANIMATED_GAME_OBJECT::reload()
{
    renderer_animation = Renderer::get().create_animation(model);
}