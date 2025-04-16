#include "editor.hpp"
#include "../managers/renderer.hpp"
#include "../managers/window.hpp"
#include "../managers/gui.hpp"
#include "../managers/script.hpp"
#include "../managers/backup.hpp"

#include "../game_objects/camera_game_object.hpp"

#include "../game_objects/model_game_object.hpp"
#include "../game_objects/mesh_game_object.hpp"
#include "../game_objects/animated_game_object.hpp"

#include <iostream>

Editor::Editor() : editor_camera(nullptr)
{
	MarkoEngine::Window::get().initialize();
	Renderer::get().initialize();
	MarkoEngine::Script::get().initialize();
	MarkoEngine::Gui::get().initialize();
	marko_engine::Backup::get().initialize();

	#ifndef EXPORT
	editor_camera = new CAMERA_GAME_OBJECT();
    transform t = editor_camera->get_local_transform();
    t.position.y = 1;
    editor_camera->set_local_transform(t);
	#endif
}

Editor::~Editor()
{
	delete editor_camera;
    marko_engine::Backup::get().cleanup();
	MarkoEngine::Gui::get().cleanup();
	MarkoEngine::Script::get().cleanup();
	Renderer::get().cleanup();
	MarkoEngine::Window::get().cleanup();
}

void Editor::run()
{
    static glm::vec3 cam_dir(0.0f, 0.0f, 0.0f);
    static bool is_looking = false;

    marko_engine::Backup::get().load_object_state();

    while (!MarkoEngine::Window::get().should_close())
    {

        if (MarkoEngine::Window::get().key_pressed(GLFW_KEY_S) && MarkoEngine::Window::get().key_pressed(GLFW_KEY_LEFT_CONTROL) ||
            MarkoEngine::Window::get().key_held(GLFW_KEY_S) && MarkoEngine::Window::get().key_pressed(GLFW_KEY_LEFT_CONTROL) ||
            MarkoEngine::Window::get().key_pressed(GLFW_KEY_S) && MarkoEngine::Window::get().key_held(GLFW_KEY_LEFT_CONTROL))
        {
            marko_engine::Backup::get().save_object_state();
            std::cout << "save completed!" << std::endl;
        }


        if ((MarkoEngine::Gui::get().playing() != MarkoEngine::Gui::get().prev_playing()) && MarkoEngine::Gui::get().playing())
        {
            MarkoEngine::Script::get().call_awake();
            MarkoEngine::Script::get().call_start();
        }


        if (editor_camera != nullptr)
        {
            cam_dir = glm::vec3(0.0f, 0.0f, 0.0f);


            if (MarkoEngine::Window::get().key_held(GLFW_KEY_W)) cam_dir.z += 1;
            if (MarkoEngine::Window::get().key_held(GLFW_KEY_A)) cam_dir.x -= 1;
            if (MarkoEngine::Window::get().key_held(GLFW_KEY_S)) cam_dir.z -= 1;
            if (MarkoEngine::Window::get().key_held(GLFW_KEY_D)) cam_dir.x += 1;
            if (MarkoEngine::Window::get().key_held(GLFW_KEY_Q)) cam_dir.y -= 1;
            if (MarkoEngine::Window::get().key_held(GLFW_KEY_E)) cam_dir.y += 1;


            if (MarkoEngine::Window::get().mouse_held(GLFW_MOUSE_BUTTON_RIGHT))
            {
                if (MarkoEngine::Window::get().mouse_position().x >= 400 &&
                    MarkoEngine::Window::get().mouse_position().x <= 1520 &&
                    MarkoEngine::Window::get().mouse_position().y >= 50 &&
                    MarkoEngine::Window::get().mouse_position().y <= 630)
                {
                    is_looking = true;
                }
            }
            else
            {
                is_looking = false;
            }


            Renderer::get().set_view_matrix(editor_camera->get_view_matrix());


            if (is_looking)
            {
                MarkoEngine::Window::get().set_mouse_mode(GLFW_CURSOR_DISABLED);
                if (MarkoEngine::Window::get().mouse_position().x <= 400 ||
                    MarkoEngine::Window::get().mouse_position().x >= 1520 ||
                    MarkoEngine::Window::get().mouse_position().y <= 50 ||
                    MarkoEngine::Window::get().mouse_position().y >= 630)
                {
                    MarkoEngine::Window::get().set_mouse_position(glm::vec2(1920 / 2, 1080 / 2));
                }

                static glm::vec2 smooth_delta = MarkoEngine::Window::get().mouse_delta();
                smooth_delta = glm::mix(smooth_delta, glm::vec2(MarkoEngine::Window::get().mouse_delta()), 0.1f);
                editor_camera->rotate(smooth_delta, 0.2f);
                editor_camera->translate(cam_dir, MarkoEngine::Window::get().delta_time() * 30);
            }
            else
            {
                MarkoEngine::Window::get().set_mouse_mode(GLFW_CURSOR_NORMAL);
            }
        }


        if (MarkoEngine::Gui::get().playing())
        {
            for (const auto& object : I_GAME_OBJECT::game_objects)
            {
                if (object.second->get_type() == game_object_type::CAMERA)
                {
                    auto* camera = dynamic_cast<CAMERA_GAME_OBJECT*>(object.second.get());
                    Renderer::get().set_view_matrix(camera->get_view_matrix());
                }
            }
            MarkoEngine::Script::get().call_update();
        }


        if (MarkoEngine::Window::get().key_pressed(GLFW_KEY_END))
        {
            MarkoEngine::Window::get().close();
        }


        MarkoEngine::Window::get().update();
        Renderer::get().update();
        MarkoEngine::Script::get().update();
    }
}
