#include "pch.h"
#include "mesh_game_object.hpp"


MESH_GAME_OBJECT::MESH_GAME_OBJECT(std::vector<Vertex> vertices, std::vector<uint32_t> indices, const std::string& texture) : I_GAME_OBJECT(game_object_type::MESH), mesh_filename(texture), vertices(vertices), indices(indices)
{
	renderer_mesh = Renderer::get().create_mesh(texture, vertices, indices);
}

void MESH_GAME_OBJECT::Draw()
{
	Renderer::get().draw_mesh(renderer_mesh);
}

void MESH_GAME_OBJECT::reload()
{
	renderer_mesh = Renderer::get().create_mesh(mesh_filename, vertices, indices);
}
