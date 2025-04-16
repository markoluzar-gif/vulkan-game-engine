#pragma once
#include "i_game_object.hpp"
#include "../managers/renderer.hpp"
#include <assimp/scene.h>
#include <unordered_map>
#include <vector>

class ANIMATED_GAME_OBJECT : public I_GAME_OBJECT
{
public:
    ANIMATED_GAME_OBJECT(const std::string& model);
    void Draw();
    void reload();
    std::string model;
private:
    Renderer_Animation renderer_animation;
};