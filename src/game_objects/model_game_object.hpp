#pragma once
#include "i_game_object.hpp"

class MODEL_GAME_OBJECT : public I_GAME_OBJECT
{
public:
	MODEL_GAME_OBJECT() {}
	MODEL_GAME_OBJECT(const std::string& model);
	void Draw();
	void reload();

	Renderer_Model renderer_model;
	std::string model;
};

