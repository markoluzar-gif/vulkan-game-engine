#include "pch.h"
#include "i_game_object.hpp"

#include "game_objects/camera_game_object.hpp"
#include "game_objects/mesh_game_object.hpp"
#include "game_objects/model_game_object.hpp"
#include "game_objects/box_collider_game_object.hpp"
#include "game_objects/animated_game_object.hpp"

std::unordered_map<std::string, std::unique_ptr<I_GAME_OBJECT>> I_GAME_OBJECT::game_objects;

I_GAME_OBJECT::I_GAME_OBJECT(): type(game_object_type::EMPTY), parent("Root"), children(), local_transform(), script() {}

I_GAME_OBJECT::I_GAME_OBJECT(const game_object_type& type) : type(type), parent("Root"), children(), local_transform(), script() {}

I_GAME_OBJECT::~I_GAME_OBJECT() {}

void I_GAME_OBJECT::set_parent(const std::string& parent_id)
{
	parent = parent_id;
}

void I_GAME_OBJECT::set_child(const std::string& child_id)
{
	children.push_back(child_id);
}

void I_GAME_OBJECT::set_script(const std::string& script)
{
	this->script = script;
}

void I_GAME_OBJECT::translate_local_transform(glm::vec3 axis, float factor)
{
	if (glm::length(axis) > 0.0f)
	{
		local_transform.position += glm::normalize(axis) * factor;
	}
}

void I_GAME_OBJECT::rotate_local_transform(glm::vec3 axis, float factor)
{
	if (glm::length(axis) > 0.0f)
	{
		local_transform.rotation += glm::normalize(axis) * factor;
	}
}

void I_GAME_OBJECT::scale_local_transform(glm::vec3 axis, float factor)
{
	if (glm::length(axis) > 0.0f)
	{
		local_transform.scale *= (1.0f + glm::normalize(axis) * factor);
	}
}

void I_GAME_OBJECT::set_local_transform(const transform new_local_transform)
{
	local_transform = new_local_transform;
}

void I_GAME_OBJECT::set_world_transform(const transform new_world_transform)
{
	world_transform = new_world_transform;
}

game_object_type I_GAME_OBJECT::get_type() const
{
	return type;
}

std::string I_GAME_OBJECT::get_script() const
{
	return script;
}

std::string I_GAME_OBJECT::get_parent() const
{
	return parent;
}

transform I_GAME_OBJECT::get_local_transform() const
{
	return local_transform;
}

transform I_GAME_OBJECT::get_world_transform() const
{
	return world_transform;
}

std::vector<std::string>& I_GAME_OBJECT::get_children()
{
	return children;
}

void I_GAME_OBJECT::save_to_binary(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Failed to open file for saving: " << filename << std::endl;
        return;
    }

    size_t game_objects_size = game_objects.size();
    ofs.write(reinterpret_cast<const char*>(&game_objects_size), sizeof(game_objects_size));

    for (const auto& obj_pair : game_objects) {
        I_GAME_OBJECT* obj = obj_pair.second.get();

        game_object_type type = obj->type;
        ofs.write(reinterpret_cast<const char*>(&type), sizeof(type));


        size_t id_size = obj_pair.first.size();
        ofs.write(reinterpret_cast<const char*>(&id_size), sizeof(id_size));
        ofs.write(obj_pair.first.c_str(), id_size);


        size_t script_size = obj->get_script().size();
        ofs.write(reinterpret_cast<const char*>(&script_size), sizeof(script_size));
        ofs.write(obj->get_script().c_str(), script_size);


        size_t parent_size = obj->get_parent().size();
        ofs.write(reinterpret_cast<const char*>(&parent_size), sizeof(parent_size));
        ofs.write(obj->get_parent().c_str(), parent_size);


        size_t children_size = obj->get_children().size();
        ofs.write(reinterpret_cast<const char*>(&children_size), sizeof(children_size));
        for (const auto& child_id : obj->get_children()) {
            size_t child_id_size = child_id.size();
            ofs.write(reinterpret_cast<const char*>(&child_id_size), sizeof(child_id_size));
            ofs.write(child_id.c_str(), child_id_size);
        }


        ofs.write(reinterpret_cast<const char*>(&obj->world_transform), sizeof(transform));
        ofs.write(reinterpret_cast<const char*>(&obj->world_transform), sizeof(transform));


        switch (type) {
        case MESH: {
            MESH_GAME_OBJECT* mesh_obj = static_cast<MESH_GAME_OBJECT*>(obj);


            const std::string& modelPath = mesh_obj->get_mesh_filename();
            size_t modelPathLength = modelPath.size();
            ofs.write(reinterpret_cast<const char*>(&modelPathLength), sizeof(modelPathLength));
            ofs.write(modelPath.c_str(), modelPathLength);


            const std::vector<Vertex>& vertices = mesh_obj->get_vertices();
            size_t vertexCount = vertices.size();
            ofs.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            ofs.write(reinterpret_cast<const char*>(vertices.data()), vertexCount * sizeof(Vertex));


            const std::vector<uint32_t>& indices = mesh_obj->get_indices();
            size_t indexCount = indices.size();
            ofs.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
            ofs.write(reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(uint32_t));

            break;
        }
        case MODEL: {
            MODEL_GAME_OBJECT* model_obj = static_cast<MODEL_GAME_OBJECT*>(obj);
            size_t modelPathLength = model_obj->model.size();
            ofs.write(reinterpret_cast<const char*>(&modelPathLength), sizeof(modelPathLength));
            ofs.write(model_obj->model.c_str(), modelPathLength);
            break;
        }
        case CAMERA: {
            CAMERA_GAME_OBJECT* camera_obj = static_cast<CAMERA_GAME_OBJECT*>(obj);
            ofs.write(reinterpret_cast<const char*>(&camera_obj->world_up), sizeof(camera_obj->world_up));
            break;
        }
        case BOX_COLLIDER: {
            BOX_COLLIDER_GAME_OBJECT* box_obj = static_cast<BOX_COLLIDER_GAME_OBJECT*>(obj);
            ofs.write(reinterpret_cast<const char*>(&box_obj->min), sizeof(box_obj->min));
            ofs.write(reinterpret_cast<const char*>(&box_obj->max), sizeof(box_obj->max));
            break;
        }
        case ANIMATED: {
            ANIMATED_GAME_OBJECT* model_obj = static_cast<ANIMATED_GAME_OBJECT*>(obj);
            size_t modelPathLength = model_obj->model.size();
            ofs.write(reinterpret_cast<const char*>(&modelPathLength), sizeof(modelPathLength));
            ofs.write(model_obj->model.c_str(), modelPathLength);
            break;
        }
        default:
            break;
        }
    }

    ofs.close();
}

void I_GAME_OBJECT::load_from_binary(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open file for loading: " << filename << std::endl;
        return;
    }

    size_t game_objects_size;
    ifs.read(reinterpret_cast<char*>(&game_objects_size), sizeof(game_objects_size));

    for (size_t i = 0; i < game_objects_size; ++i) {
        game_object_type type;
        ifs.read(reinterpret_cast<char*>(&type), sizeof(type));

        size_t id_size;
        ifs.read(reinterpret_cast<char*>(&id_size), sizeof(id_size));
        std::string id(id_size, '\0');
        ifs.read(&id[0], id_size);


        size_t script_size;
        ifs.read(reinterpret_cast<char*>(&script_size), sizeof(script_size));
        std::string script(script_size, '\0');
        ifs.read(&script[0], script_size);


        size_t parent_size;
        ifs.read(reinterpret_cast<char*>(&parent_size), sizeof(parent_size));
        std::string parent(parent_size, '\0');
        ifs.read(&parent[0], parent_size);


        size_t children_size;
        ifs.read(reinterpret_cast<char*>(&children_size), sizeof(children_size));
        std::vector<std::string> children(children_size);
        for (size_t j = 0; j < children_size; ++j) {
            size_t child_id_size;
            ifs.read(reinterpret_cast<char*>(&child_id_size), sizeof(child_id_size));
            std::string child_id(child_id_size, '\0');
            ifs.read(&child_id[0], child_id_size);
            children[j] = child_id;
        }


        transform local_transform;
        transform world_transform;
        ifs.read(reinterpret_cast<char*>(&local_transform), sizeof(local_transform));
        ifs.read(reinterpret_cast<char*>(&world_transform), sizeof(world_transform));

        I_GAME_OBJECT* obj = nullptr;
        switch (type) {
        case MESH: {
            

            size_t modelPathLength;
            ifs.read(reinterpret_cast<char*>(&modelPathLength), sizeof(modelPathLength));

            std::string modelPath(modelPathLength, '\0');
            ifs.read(&modelPath[0], modelPathLength);


            size_t vertexCount;
            ifs.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));

            std::vector<Vertex> vertices(vertexCount);
            ifs.read(reinterpret_cast<char*>(vertices.data()), vertexCount * sizeof(Vertex));


            size_t indexCount;
            ifs.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

            std::vector<uint32_t> indices(indexCount);
            ifs.read(reinterpret_cast<char*>(indices.data()), indexCount * sizeof(uint32_t));


            MESH_GAME_OBJECT* mesh_obj = new MESH_GAME_OBJECT(vertices, indices, modelPath);

            obj = mesh_obj;
            
            break;
        }
        case MODEL: {

            size_t modelPathLength;
            ifs.read(reinterpret_cast<char*>(&modelPathLength), sizeof(modelPathLength));
            std::string modelPath(modelPathLength, '\0');
            ifs.read(&modelPath[0], modelPathLength);
            obj = new MODEL_GAME_OBJECT(modelPath);
            break;
        }
        case ANIMATED: {

            size_t modelPathLength;
            ifs.read(reinterpret_cast<char*>(&modelPathLength), sizeof(modelPathLength));
            std::string modelPath(modelPathLength, '\0');
            ifs.read(&modelPath[0], modelPathLength);
            obj = new ANIMATED_GAME_OBJECT(modelPath);
            break;
        }
        case CAMERA: {
            obj = new CAMERA_GAME_OBJECT();
            CAMERA_GAME_OBJECT* camera_obj = static_cast<CAMERA_GAME_OBJECT*>(obj);
            ifs.read(reinterpret_cast<char*>(&camera_obj->world_up), sizeof(camera_obj->world_up));
            break;
        }
        case BOX_COLLIDER: {
            obj = new BOX_COLLIDER_GAME_OBJECT();
            BOX_COLLIDER_GAME_OBJECT* box_obj = static_cast<BOX_COLLIDER_GAME_OBJECT*>(obj);
            ifs.read(reinterpret_cast<char*>(&box_obj->min), sizeof(box_obj->min));
            ifs.read(reinterpret_cast<char*>(&box_obj->max), sizeof(box_obj->max));
            break;
        }
        default:
            break;
        }

        if (obj) {
            obj->set_parent(parent);
            obj->set_script(script);
            obj->children = children;
            obj->set_local_transform(local_transform);
            obj->set_world_transform(world_transform);
            obj->type = type;
            game_objects[id] = std::unique_ptr<I_GAME_OBJECT>(obj);
        }
    }

    ifs.close();
}
