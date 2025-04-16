#pragma once
#include "i_game_object.hpp"

class BOX_COLLIDER_GAME_OBJECT : public I_GAME_OBJECT
{
public:
	BOX_COLLIDER_GAME_OBJECT() {}
	BOX_COLLIDER_GAME_OBJECT(const glm::vec3& min, const glm::vec3& max);
	glm::vec3 min;
	glm::vec3 max;
};

