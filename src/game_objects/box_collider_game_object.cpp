#include "pch.h"
#include "box_collider_game_object.hpp"


BOX_COLLIDER_GAME_OBJECT::BOX_COLLIDER_GAME_OBJECT(const glm::vec3& min, const glm::vec3& max) : I_GAME_OBJECT(game_object_type::BOX_COLLIDER), min(min), max(max) {}