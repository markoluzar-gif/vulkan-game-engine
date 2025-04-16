#include "pch.h"
#include "camera_game_object.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>


CAMERA_GAME_OBJECT::CAMERA_GAME_OBJECT() : I_GAME_OBJECT(game_object_type::CAMERA), world_up(0.0f, 1.0f, 0.0f) {}

void CAMERA_GAME_OBJECT::rotate(glm::vec2 mouse_delta, float sensitivity)
{
    float delta_x = -mouse_delta.x * sensitivity;
    float delta_y = -mouse_delta.y * sensitivity;

    local_transform.rotation.x += delta_y;
    local_transform.rotation.y += delta_x;

    if (local_transform.rotation.x > 89.0f) local_transform.rotation.x = 89.0f;
    if (local_transform.rotation.x < -89.0f) local_transform.rotation.x = -89.0f;
}

void CAMERA_GAME_OBJECT::translate(glm::vec3 direction, float speed)
{
    glm::vec3 front;
    front.x = cos(-glm::radians(local_transform.rotation.y)) * cos(glm::radians(local_transform.rotation.x));
    front.y = sin(glm::radians(local_transform.rotation.x));
    front.z = sin(-glm::radians(local_transform.rotation.y)) * cos(glm::radians(local_transform.rotation.x));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, world_up));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    direction = glm::normalize(direction);
    glm::vec3 movement = front * direction.z + right * direction.x + up * direction.y;

    translate_local_transform(movement, speed);
}

glm::mat4 CAMERA_GAME_OBJECT::get_view_matrix() const
{
    glm::vec3 front;
    front.x = cos(glm::radians(-local_transform.rotation.y)) * cos(glm::radians(local_transform.rotation.x));
    front.y = sin(glm::radians(local_transform.rotation.x));
    front.z = sin(glm::radians(-local_transform.rotation.y)) * cos(glm::radians(local_transform.rotation.x));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, world_up));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    return glm::lookAt(local_transform.position, local_transform.position + front, up);
}