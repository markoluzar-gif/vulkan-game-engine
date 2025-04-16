#pragma once

class CAMERA_GAME_OBJECT;

class Editor
{
public:
	Editor();
	~Editor();
public:
	void run();
private:
	CAMERA_GAME_OBJECT* editor_camera;
};

