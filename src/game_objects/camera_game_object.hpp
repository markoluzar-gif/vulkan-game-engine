#pragma once
#include "i_game_object.hpp"

class CAMERA_GAME_OBJECT : public I_GAME_OBJECT
{
public:
    CAMERA_GAME_OBJECT();
    void rotate(glm::vec2 mouse_delta, float sensitivity);
    void translate(glm::vec3 direction, float speed);
    glm::mat4 get_view_matrix() const;
    glm::vec3 world_up;
};