#include "pch.h"
#include "model_game_object.hpp"
#include "../managers/renderer.hpp"


MODEL_GAME_OBJECT::MODEL_GAME_OBJECT(const std::string& model) : I_GAME_OBJECT(game_object_type::MODEL), model(model)
{
	renderer_model = Renderer::get().create_model(model);
}

void MODEL_GAME_OBJECT::Draw() 
{
    Renderer::get().draw_model(renderer_model);
}

void MODEL_GAME_OBJECT::reload()
{
    renderer_model = Renderer::get().create_model(model);
}


