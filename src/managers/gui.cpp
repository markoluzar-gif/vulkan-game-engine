#include "pch.h"
#include <imgui/imgui.h>
#include "gui.hpp"
#include "window.hpp"
#include "renderer.hpp"

#include "../game_objects/i_game_object.hpp"
#include "../game_objects/camera_game_object.hpp"
#include "../game_objects/model_game_object.hpp"
#include "../game_objects/box_collider_game_object.hpp"
#include "../game_objects/mesh_game_object.hpp"

#include <shlobj.h> 
#include "../game_objects/animated_game_object.hpp"
#include "../managers/backup.hpp"

MarkoEngine::Gui& MarkoEngine::Gui::get()
{
    static Gui instance;
    return instance;
}

bool MarkoEngine::Gui::playing()
{
    return is_playing;
}

bool MarkoEngine::Gui::prev_playing()
{
    return prev_is_playing;
}

void MarkoEngine::Gui::initialize()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(10, 5);
    style.ItemInnerSpacing = ImVec2(5, 5);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 5.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 2.0f;
    style.FrameBorderSize = 2.0f;

    ImVec4* colors = style.Colors;


    colors[ImGuiColwindow_managerBg] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);


    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.32f, 1.00f);


    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);


    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);

    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.42f, 1.00f);

    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.45f, 0.90f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.38f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.20f, 0.42f, 1.00f);

    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.45f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.34f, 0.50f, 1.00f, 1.00f);

    curr_folder = "content";
    selected_game_object = "";
    is_playing = false;
#ifdef EXPORT
    is_playing = true;
#endif

    folder_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/folder.png");

    file_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/file.png");
    alert_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/alert-triangle.png");
    image_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/image.png");
    play_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/play.png");
    stop_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/stop-circle.png");


    export_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/log-out.png");
    box_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/box.png");
    cuboid_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/codepen.png");
    visible_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/eye.png");
    hidden_tex = Renderer::get().create_gui_texture("dependencies/gui_icons/eye-off.png");

    I_GAME_OBJECT::game_objects["Root"] = std::make_unique<I_GAME_OBJECT>();
}

void MarkoEngine::Gui::cleanup()
{
}

void MarkoEngine::Gui::update()
{
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        draw_list->AddLine(ImVec2(MarkoEngine::Window::get().scale_x(0), MarkoEngine::Window::get().scale_y(5)), ImVec2(MarkoEngine::Window::get().scale_x(100), MarkoEngine::Window::get().scale_y(5)), IM_COL32(60, 60, 80, 255), 3.0f);
        draw_list->AddLine(ImVec2(MarkoEngine::Window::get().scale_x(21), MarkoEngine::Window::get().scale_y(5)), ImVec2(MarkoEngine::Window::get().scale_x(21), MarkoEngine::Window::get().scale_y(60)), IM_COL32(60, 60, 80, 255), 3.0f);
        draw_list->AddLine(ImVec2(MarkoEngine::Window::get().scale_x(79), MarkoEngine::Window::get().scale_y(5)), ImVec2(MarkoEngine::Window::get().scale_x(79), MarkoEngine::Window::get().scale_y(100)), IM_COL32(60, 60, 80, 255), 3.0f);
        draw_list->AddLine(ImVec2(MarkoEngine::Window::get().scale_x(0), MarkoEngine::Window::get().scale_y(60)), ImVec2(MarkoEngine::Window::get().scale_x(79), MarkoEngine::Window::get().scale_y(60)), IM_COL32(60, 60, 80, 255), 3.0f);


        draw_inspector();
        draw_hierarchy();
        draw_content();
        draw_top_bar();

}

void MarkoEngine::Gui::draw_inspector()
{
    if (MarkoEngine::Window::get().key_pressed(GLFW_KEY_DELETE))
    {
        if (Gui::get().selected_game_object != "")
        {
            if (I_GAME_OBJECT::game_objects[Gui::get().selected_game_object]->get_script() != "")
            {
                I_GAME_OBJECT::game_objects[Gui::get().selected_game_object]->set_script("");
            }
            else
            {
                I_GAME_OBJECT::game_objects.erase(Gui::get().selected_game_object);
                Gui::get().selected_game_object = "";
            }
        }
    }

    ImVec2 position(MarkoEngine::Window::get().scale_x(79), MarkoEngine::Window::get().scale_y(5));
    ImVec2 size(MarkoEngine::Window::get().scale_x(22), MarkoEngine::Window::get().scale_y(98));
    ImVec2 button_size(MarkoEngine::Window::get().scale_x(20), MarkoEngine::Window::get().scale_y(9));
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);

    if (ImGui::Begin("inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        if (selected_game_object != "")
        {
            ImGui::PushItemWidth(70);


            auto CenterText = [&](const char* label) {
                float text_width = ImGui::CalcTextSize(label).x;
                float total_width = (70 * 3) + (ImGui::GetStyle().ItemSpacing.x * 2);
                float window_center_x = position.x + (size.x * 0.5f);
                float text_pos_x = window_center_x - (text_width * 0.5f);
                ImGui::SetCursorPosX(text_pos_x - position.x);
                ImGui::Text("%s", label);
                };


            auto BeginCenteredGroup = [&]() {
                float group_width = (70 * 3) + (ImGui::GetStyle().ItemSpacing.x * 2) + 40;
                float window_center_x = position.x + (size.x * 0.5f);
                float group_x = window_center_x - (group_width * 0.5f);
                ImGui::SetCursorPosX(group_x - position.x);
                ImGui::BeginGroup();
                };

            auto EndCenteredGroup = [&]() {
                ImGui::EndGroup();
                };

            ImGui::SetCursorPosY(40);

            CenterText("name");
            {
                strncpy_s(buffer, selected_game_object.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                float input_width = button_size.x - 100;
                float window_center_x = position.x + (size.x * 0.5f);
                float group_x = window_center_x - (input_width * 0.5f);
                ImGui::SetCursorPosX(group_x - position.x);

                ImGui::BeginGroup();
                ImGui::SetNextItemWidth(input_width);

                if (ImGui::InputText("##name", buffer, IM_ARRAYSIZE(buffer)))
                {
                    std::string new_key = buffer;
                    if (!new_key.empty() && new_key != selected_game_object)
                    {
 
                        auto it = I_GAME_OBJECT::game_objects.find(selected_game_object);
                        if (it != I_GAME_OBJECT::game_objects.end())
                        {
                            for (auto& game_object : I_GAME_OBJECT::game_objects)
                            {
                                if (game_object.second->parent == it->first)
                                {
                                    game_object.second->parent = new_key;
                                }
                            }

                            I_GAME_OBJECT::game_objects[new_key] = std::move(it->second);
                            I_GAME_OBJECT::game_objects.erase(it);

                            selected_game_object = new_key;
                        }
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();


            glm::vec3 pos = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform().position;
            CenterText("position");
            BeginCenteredGroup();
            ImGui::Text("x"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.1f, 0.1f, 1.0f));
            ImGui::InputFloat("##pos_x", &pos.x, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("y"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.4f, 0.1f, 1.0f));
            ImGui::InputFloat("##pos_y", &pos.y, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("z"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.4f, 1.0f));
            ImGui::InputFloat("##pos_z", &pos.z, 0.0f, 0.0f, "%.3f");
            ImGui::PopStyleColor();
            EndCenteredGroup();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                transform t = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform();
                t.position = pos;
                I_GAME_OBJECT::game_objects[selected_game_object]->set_local_transform(t);
            }

            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();

            glm::vec3 rot = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform().rotation;
            CenterText("rotation");
            BeginCenteredGroup();
            ImGui::Text("x"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.1f, 0.1f, 1));
            ImGui::InputFloat("##rot_x", &rot.x, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("y"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.4f, 0.1f, 1));
            ImGui::InputFloat("##rot_y", &rot.y, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("z"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.4f, 1));
            ImGui::InputFloat("##rot_z", &rot.z, 0.0f, 0.0f, "%.3f");
            ImGui::PopStyleColor();
            EndCenteredGroup();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                transform t = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform();
                t.rotation = rot;
                I_GAME_OBJECT::game_objects[selected_game_object]->set_local_transform(t);
            }

            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();

            glm::vec3 scl = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform().scale;
            CenterText("scale");
            BeginCenteredGroup();
            ImGui::Text("x"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.1f, 0.1f, 1));
            ImGui::InputFloat("##scale_x", &scl.x, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("y"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.4f, 0.1f, 1));
            ImGui::InputFloat("##scale_y", &scl.y, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
            ImGui::PopStyleColor();

            ImGui::Text("z"); ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.4f, 1));
            ImGui::InputFloat("##scale_z", &scl.z, 0.0f, 0.0f, "%.3f");
            ImGui::PopStyleColor();
            EndCenteredGroup();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                transform t = I_GAME_OBJECT::game_objects[selected_game_object]->get_local_transform();
                t.scale = scl;
                I_GAME_OBJECT::game_objects[selected_game_object]->set_local_transform(t);
            }

            ImGui::PopItemWidth();

            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();


            CenterText("visibility");
            {
                float checkboxSize = ImGui::GetFrameHeight();
                float window_center_x = position.x + (size.x * 0.5f);
                float checkbox_x = window_center_x - (checkboxSize * 0.5f);
                ImGui::SetCursorPosX(checkbox_x - position.x);

                bool visibility = I_GAME_OBJECT::game_objects[selected_game_object]->is_visible;
                if (ImGui::Checkbox("##visibility", &visibility))
                {
   
                    I_GAME_OBJECT::game_objects[selected_game_object]->is_visible = visibility;
                }
            }

            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();


            strncpy_s(buffer, I_GAME_OBJECT::game_objects[selected_game_object]->get_script().c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            CenterText("script");
            {

                float input_width = button_size.x - 100;
                float window_center_x = position.x + (size.x * 0.5f);
                float group_x = window_center_x - (input_width * 0.5f);
                ImGui::SetCursorPosX(group_x - position.x);

                ImGui::BeginGroup();
                ImGui::SetNextItemWidth(input_width);
                if (ImGui::InputText("##script", buffer, IM_ARRAYSIZE(buffer)))
                {
                    I_GAME_OBJECT::game_objects[selected_game_object]->set_script(buffer);
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File"))
                    {
                        if (payload->DataSize > 0)
                        {
                            strncpy_s(buffer, (const char*)payload->Data, IM_ARRAYSIZE(buffer) - 1);
                            buffer[IM_ARRAYSIZE(buffer) - 1] = '\0';
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::EndGroup();
            }

            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
            ImGui::Separator();
            ImGui::PopStyleColor();



            if (I_GAME_OBJECT::game_objects[selected_game_object]->get_type() == game_object_type::MESH)
            {
                CenterText("mesh");
                {
                    MESH_GAME_OBJECT* model = dynamic_cast<MESH_GAME_OBJECT*>(I_GAME_OBJECT::game_objects[selected_game_object].get());
                    strncpy_s(buffer, model->get_mesh_filename().c_str(), sizeof(buffer) - 1);

                    buffer[sizeof(buffer) - 1] = '\0';

                    float input_width = button_size.x - 100;
                    float window_center_x = position.x + (size.x * 0.5f);
                    float group_x = window_center_x - (input_width * 0.5f);
                    ImGui::SetCursorPosX(group_x - position.x);

                    ImGui::BeginGroup();
                    ImGui::SetNextItemWidth(input_width);

                    if (ImGui::InputText("##mesh", buffer, IM_ARRAYSIZE(buffer)))
                    {
                        std::string filename = std::string(buffer);
                        if (!std::string(buffer).starts_with("content/"))
                        filename = "content/" + filename;

                        model->set_mesh_filename(filename);
                        model->reload();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File"))
                        {
                            if (payload->DataSize > 0)
                            {

                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::EndGroup();
                }
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            if (I_GAME_OBJECT::game_objects[selected_game_object]->get_type() == game_object_type::MODEL)
            {
                CenterText("model");
                {
                    MODEL_GAME_OBJECT* model = dynamic_cast<MODEL_GAME_OBJECT*>(I_GAME_OBJECT::game_objects[selected_game_object].get());
                    strncpy_s(buffer, model->model.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';

                    float input_width = button_size.x - 100;
                    float window_center_x = position.x + (size.x * 0.5f);
                    float group_x = window_center_x - (input_width * 0.5f);
                    ImGui::SetCursorPosX(group_x - position.x);

                    ImGui::BeginGroup();
                    ImGui::SetNextItemWidth(input_width);

                    if (ImGui::InputText("##mesh", buffer, IM_ARRAYSIZE(buffer)))
                    {
                        model->model = buffer;
                        model->reload();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File"))
                        {
                            if (payload->DataSize > 0)
                            {

                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::EndGroup();
                }
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            if (I_GAME_OBJECT::game_objects[selected_game_object]->get_type() == game_object_type::ANIMATED)
            {
                CenterText("animation");
                {
                    ANIMATED_GAME_OBJECT* model = dynamic_cast<ANIMATED_GAME_OBJECT*>(I_GAME_OBJECT::game_objects[selected_game_object].get());
                    strncpy_s(buffer, model->model.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';

                    float input_width = button_size.x - 100;
                    float window_center_x = position.x + (size.x * 0.5f);
                    float group_x = window_center_x - (input_width * 0.5f);
                    ImGui::SetCursorPosX(group_x - position.x);

                    ImGui::BeginGroup();
                    ImGui::SetNextItemWidth(input_width);

                    if (ImGui::InputText("##animation", buffer, IM_ARRAYSIZE(buffer)))
                    {
                        model->model = buffer;
                        model->reload();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File"))
                        {
                            if (payload->DataSize > 0)
                            {

                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::EndGroup();
                }
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 1));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }
        }
        else
        {
            if (ImGui::Button("create script", button_size))
            {
                std::ofstream datao(curr_folder + "/new_script.lua");
                if (datao.is_open())
                {
                    datao << "\nfunction start()\n\    \nend\n\nfunction update()\n\    \nend";
                }
                datao.close();
            }

            if (ImGui::Button("create folder", button_size))
            {
                std::filesystem::create_directory(curr_folder + "/new_folder");
            }

            if (ImGui::Button("create mesh", button_size))
            {
                std::vector<Vertex> vertices = {
                    {{-0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

                    {{-0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
                };

                std::vector<unsigned int> indices = { 0, 1, 2, 3, 4, 5 };

                I_GAME_OBJECT::game_objects["new_mesh"] = std::make_unique<MESH_GAME_OBJECT>(vertices, indices, "");
            }

            if (ImGui::Button("create model", button_size))
            {
                I_GAME_OBJECT::game_objects["new_model"] = std::make_unique<MODEL_GAME_OBJECT>("");
            }

            if (ImGui::Button("create camera", button_size))
            {
                I_GAME_OBJECT::game_objects["new_camera"] = std::make_unique<CAMERA_GAME_OBJECT>();
                I_GAME_OBJECT::game_objects["new_camera"]->set_child("camera_model");

                I_GAME_OBJECT::game_objects["camera_model"] = std::make_unique<MODEL_GAME_OBJECT>("dependencies/camera/CCTV Camera Low-poly 3D model.obj");
                transform t = transform(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.02f, 0.02f, 0.02f));
                I_GAME_OBJECT::game_objects["camera_model"]->set_local_transform(t);
                I_GAME_OBJECT::game_objects["camera_model"]->set_parent("new_camera");
            }

            if (ImGui::Button("create box collider", button_size))
            {
                I_GAME_OBJECT::game_objects["new_box_collider"] = std::make_unique<BOX_COLLIDER_GAME_OBJECT>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
                I_GAME_OBJECT::game_objects["new_box_collider"]->set_child("new_box_collider_model");

                I_GAME_OBJECT::game_objects["new_box_collider_model"] = std::make_unique<MODEL_GAME_OBJECT>("dependencies/crate/Crate1.obj");
                I_GAME_OBJECT::game_objects["new_box_collider_model"]->set_parent("new_box_collider");

            }

            if (ImGui::Button("create animation", button_size))
            {
                I_GAME_OBJECT::game_objects["new_animation"] = std::make_unique<ANIMATED_GAME_OBJECT>("");
            }

        }
        ImGui::End();
    }
}

void MarkoEngine::Gui::draw_hierarchy()
{
    ImVec2 position(MarkoEngine::Window::get().scale_x(0), MarkoEngine::Window::get().scale_y(5));
    ImVec2 size(MarkoEngine::Window::get().scale_x(21), MarkoEngine::Window::get().scale_y(55));
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(position);

    if (ImGui::Begin("Hierarchy", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {

        I_GAME_OBJECT::game_objects["Root"]->get_children().clear();
        for (auto& [id, object] : I_GAME_OBJECT::game_objects)
        {
            if (id != "Root" && (object->get_parent().empty() || object->get_parent() == "Root"))
            {
                I_GAME_OBJECT::game_objects["Root"]->set_child(id);
            }
        }


        std::function<void(const std::string&, const std::unique_ptr<I_GAME_OBJECT>&, int)> drawObject;
        drawObject = [&](const std::string& id, const std::unique_ptr<I_GAME_OBJECT>& object, int indent_level)
            {

                if (id == dragged_object_id)
                    return;

                ImGui::Indent(indent_level * 20);


                std::string label = id;
                if (!object->get_script().empty())
                {
                    label += " -> [" + object->get_script() + "]";
                }


                bool is_selected = (selected_game_object == id);
                if (ImGui::Selectable(label.c_str(), is_selected))
                {
                    selected_game_object = (is_selected ? "" : id);
                }


                if (id != "Root" && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
                {
                    ImGui::SetDragDropPayload("GAME_OBJECT", id.c_str(), id.size() + 1);
                    ImGui::Text("Dragging: %s", id.c_str());
                    ImGui::EndDragDropSource();
                }


                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
                    {
                        std::string dragged_id(static_cast<const char*>(payload->Data));
                        if (I_GAME_OBJECT::game_objects.find(dragged_id) == I_GAME_OBJECT::game_objects.end())
                        {
                            ImGui::EndDragDropTarget();
                            ImGui::Unindent(indent_level * 20);
                            return;
                        }

                        bool is_valid_target = true;
                        std::string current_id = id;
                        while (!current_id.empty() && current_id != "Root")
                        {
                            if (current_id == dragged_id)
                            {
                                is_valid_target = false;
                                break;
                            }
                            current_id = I_GAME_OBJECT::game_objects[current_id]->get_parent();
                        }

                        if (is_valid_target)
                        {
                            auto& dragged_obj = I_GAME_OBJECT::game_objects[dragged_id];
                            std::string old_parent_id = dragged_obj->get_parent();
                            if (!old_parent_id.empty() &&
                                I_GAME_OBJECT::game_objects.find(old_parent_id) != I_GAME_OBJECT::game_objects.end())
                            {
                                auto& old_parent = I_GAME_OBJECT::game_objects[old_parent_id];
                                auto& old_children = old_parent->get_children();
                                old_children.erase(std::remove(old_children.begin(), old_children.end(), dragged_id),
                                    old_children.end());
                            }

                            dragged_obj->set_parent(id);
                            object->set_child(dragged_id);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }


                auto children = object->get_children();
                for (const auto& child_id : children)
                {
                    if (I_GAME_OBJECT::game_objects.find(child_id) != I_GAME_OBJECT::game_objects.end())
                    {
                        drawObject(child_id, I_GAME_OBJECT::game_objects[child_id], indent_level + 1);
                    }
                }

                ImGui::Unindent(indent_level * 20);
            };


        drawObject("Root", I_GAME_OBJECT::game_objects["Root"], 0);
        ImGui::End();
    }
}

void MarkoEngine::Gui::draw_content()
{
    ImVec2 position(MarkoEngine::Window::get().scale_x(0), MarkoEngine::Window::get().scale_y(60));
    ImVec2 size(MarkoEngine::Window::get().scale_x(79), MarkoEngine::Window::get().scale_y(41));
    ImVec2 file_size(MarkoEngine::Window::get().scale_x(5), MarkoEngine::Window::get().scale_y(9));

    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(position);

    if (!ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        throw std::runtime_error("Failed to begin content window!");

    if (curr_folder != "content")
    {
        if (ImGui::Button("Back"))
        {
            curr_folder = std::filesystem::path(curr_folder).parent_path().string();
        }
        ImGui::Separator();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File"))
        {
            std::string dragged_path = (const char*)payload->Data;

            std::filesystem::path source_path = std::filesystem::absolute(dragged_path);
        }
        ImGui::EndDragDropTarget();
    }


    if (!ImGui::BeginTable("FileGrid", 12, ImGuiTableFlags_SizingStretchSame))
        throw std::runtime_error("Failed to begin file grid table!");

    for (const auto& file : std::filesystem::directory_iterator(curr_folder))
    {
        std::string filename = file.path().filename().string();
        ImGui::TableNextColumn();
        ImGui::PushID(filename.c_str());

        ImTextureID texture = alert_tex;
        if (std::filesystem::is_directory(curr_folder + "/" + filename)) texture = folder_tex;
        else if (filename.ends_with(".lua")) texture = file_tex;
        else if (filename.ends_with(".png") || filename.ends_with(".jpg")) texture = image_tex;
        else if (filename.ends_with(".mtl") || filename.ends_with(".obj")) texture = box_tex;

        if (ImGui::ImageButton(filename.c_str(), texture, file_size))
        {
            if (std::filesystem::is_directory(curr_folder + "/" + filename))
            {
                curr_folder = curr_folder + "/" + filename;
            }
            else
            {
                std::string file_path = std::filesystem::absolute(curr_folder + "/" + filename).string();
                std::wstring wide_path(file_path.begin(), file_path.end());
                ShellExecuteW(0, L"open", wide_path.c_str(), 0, 0, SW_SHOWNORMAL);
            }
        }

        if (ImGui::IsItemHovered() && MarkoEngine::Window::get().key_pressed(GLFW_KEY_DELETE))
        {
            std::filesystem::remove(curr_folder + "/" + filename);
        }

        if (ImGui::BeginDragDropSource())
        {
            std::string item_path = curr_folder + "/" + filename;
            ImGui::SetDragDropPayload("File", item_path.c_str(), item_path.size() + 1);
            ImGui::Text("Dragging %s", filename.c_str());
            ImGui::EndDragDropSource();
        }

        std::string displayed_name = filename;
        float max_text_width = file_size.x;

        if (ImGui::CalcTextSize(displayed_name.c_str()).x > max_text_width)
        {
            while (ImGui::CalcTextSize((displayed_name + "...").c_str()).x > max_text_width && !displayed_name.empty())
            {
                displayed_name.pop_back();
            }
            displayed_name += "...";
        }

        float text_width = ImGui::CalcTextSize(displayed_name.c_str()).x;
        float text_offset = (file_size.x - text_width) / 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_offset);

        if (ImGui::IsItemHovered() && MarkoEngine::Window::get().key_pressed(GLFW_KEY_F2))
        {
            file_to_rename = filename;
            strncpy_s(buffer, filename.c_str(), sizeof(buffer));
        }

        if (file_to_rename == filename)
        {
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::filesystem::rename(curr_folder + "/" + filename, curr_folder + "/" + std::string(buffer));
                file_to_rename.clear();
            }
        }
        else
        {
            ImGui::TextWrapped("%s", displayed_name.c_str());
        }

        ImGui::PopID();
    }
    ImGui::EndTable();
    ImGui::End();
}

void copy_directory_recursively(const std::wstring& source, const std::wstring& destination) {
    std::filesystem::create_directory(destination);

    for (const auto& entry : std::filesystem::directory_iterator(source)) {
        const auto& path = entry.path();
        if (entry.is_directory()) {

            copy_directory_recursively(path.wstring(), destination + L"\\" + path.filename().wstring());
        }
        else if (entry.is_regular_file()) {

            std::filesystem::copy(path, destination + L"\\" + path.filename().wstring(), std::filesystem::copy_options::overwrite_existing);
        }
    }
}

void MarkoEngine::Gui::draw_top_bar()
{
    ImVec2 position(MarkoEngine::Window::get().scale_x(0), MarkoEngine::Window::get().scale_y(0));
    ImVec2 size(MarkoEngine::Window::get().scale_x(100), MarkoEngine::Window::get().scale_y(5));
    ImVec2 button_size(40, 40);

    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(position);
    ImGui::Begin("bar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);


    ImGui::SetCursorPos(ImVec2(0, 0));

    ImTextureID play_stop_texture;
    if (is_playing)
    {
        play_stop_texture = stop_tex;
    }
    else
    {
        play_stop_texture = play_tex;
    }

    prev_is_playing = is_playing;
    if (ImGui::ImageButton("play", play_stop_texture, button_size))
    {
        is_playing = !is_playing;

        if (is_playing)
        {
            marko_engine::Backup::get().save_temp_object_state();
        }
        else
        {
            marko_engine::Backup::get().load_temp_object_state();
        }
    }


    ImGui::SetCursorPos(ImVec2(button_size.x + button_size.x / 2, 0));

    if (ImGui::ImageButton("build", export_tex, button_size))
    {
        std::wstring build_path;


        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);


        IFileDialog* pFileDialog = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileDialog))))
        {

            pFileDialog->SetTitle(L"Select a folder to save build files");


            DWORD dwOptions;
            pFileDialog->GetOptions(&dwOptions);
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);


            if (SUCCEEDED(pFileDialog->Show(NULL)))
            {

                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
                {
                    PWSTR pszPath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
                    {
                        build_path = pszPath;
                        CoTaskMemFree(pszPath);
                    }
                    pItem->Release();
                }
            }

            pFileDialog->Release();
        }

        CoUninitialize();

        if (!build_path.empty())
        {
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, build_path.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size_needed > 0)
            {
                std::string str(size_needed - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, build_path.c_str(), -1, &str[0], size_needed, nullptr, nullptr);

                if (!str.empty() && str.back() == '\\') {
                    str.pop_back();
                }


                std::string command = "msbuild ML_Engine.sln /p:Configuration=Export /p:OutDir=\"" + str + "\\";

                std::cout << "Executing: " << command << std::endl;


                system(command.c_str());

                std::wstring source_dll = L"assimp-vc143-mt.dll";
                std::wstring destination_dll = build_path + L"\\assimp-vc143-mt.dll";
                if (CopyFileW(source_dll.c_str(), destination_dll.c_str(), FALSE)) {
                    std::wcout << L"Copied DLL to: " << destination_dll << std::endl;
                }


                std::wstring source_shaders = L"shaders";
                std::wstring destination_shaders = build_path + L"\\shaders";
                copy_directory_recursively(source_shaders, destination_shaders);


                std::wstring source_saves = L"backups";
                std::wstring destination_saves = build_path + L"\\backups";
                copy_directory_recursively(source_saves, destination_saves);


                std::wstring source_dependencies = L"dependencies";
                std::wstring destination_dependencies = build_path + L"\\dependencies";
                copy_directory_recursively(source_dependencies, destination_dependencies);


                std::wstring source_content = L"content";
                std::wstring destination_content = build_path + L"\\content";
                copy_directory_recursively(source_content, destination_content);
            }
        }
    }

    ImGui::End();
}
