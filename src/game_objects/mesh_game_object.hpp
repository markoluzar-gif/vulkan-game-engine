#pragma once
#include "i_game_object.hpp"
#include "../managers/renderer.hpp"

class MESH_GAME_OBJECT : public I_GAME_OBJECT
{
public:
	MESH_GAME_OBJECT() {}
	MESH_GAME_OBJECT(std::vector<Vertex> vertices, std::vector<uint32_t> indices, const std::string& texture);
	void Draw();

	void reload();

	void set_mesh_filename(std::string new_mesh_filename) { mesh_filename = new_mesh_filename; }
	std::string get_mesh_filename() { return mesh_filename; }

	std::vector<Vertex> get_vertices() { return vertices; }

	std::vector<uint32_t> get_indices() { return indices; }
private:
	std::string mesh_filename;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Renderer_Mesh renderer_mesh;
};

