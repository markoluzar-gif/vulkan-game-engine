#pragma once
#include <string>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

struct transform
{
	transform() : position(0.0f), rotation(0.0f), scale(1.0f) {}
	transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) : position(position), rotation(rotation), scale(scale) {}
	
	transform operator+(const transform other) const
	{
		return transform(position + other.position, rotation + other.rotation, scale * other.scale);
	}
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

enum game_object_type
{
	EMPTY,
	MESH,
	MODEL,
	CAMERA,
	BOX_COLLIDER,
	ANIMATED
};

class I_GAME_OBJECT
{
public:
	I_GAME_OBJECT();
	I_GAME_OBJECT(const game_object_type& type);
	virtual ~I_GAME_OBJECT();
public:
	void translate_local_transform(glm::vec3 axis, float factor);
	void rotate_local_transform(glm::vec3 axis, float factor);
	void scale_local_transform(glm::vec3 axis, float factor);
public:
	void set_parent(const std::string& parent_id);
	void set_child(const std::string& child_id);
	void set_script(const std::string& script);
	void set_local_transform(const transform new_local_transform);
	void set_world_transform(const transform new_world_transform);
public:
	std::string get_script() const;
	std::string get_parent() const;
	game_object_type get_type() const;
	transform get_local_transform() const;
	transform get_world_transform() const;
	std::vector<std::string>& get_children();
	bool is_visible = true;
	std::string parent;
	std::vector<std::string> children;
protected:
	std::string	script;

	game_object_type type;
	transform local_transform;
	transform world_transform;
public:
	static std::unordered_map<std::string, std::unique_ptr<I_GAME_OBJECT>> game_objects;
	static void save_to_binary(const std::string& filename);
	static void load_from_binary(const std::string& filename);
};