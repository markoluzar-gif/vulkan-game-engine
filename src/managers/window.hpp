#pragma once
#include <glfw/glfw3.h>
#include <glm/vec2.hpp>
#include <bitset>
#include <chrono>
#include <memory>

namespace MarkoEngine
{
    class Window final
    {
    public:
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;
    private:
        Window() = default;
    public:
        void initialize();
        void update();
        void cleanup();
    public:
        static Window& get();

        GLFWwindow* window();

        glm::ivec2 size();
        glm::ivec2 position();

        int scale_x(int percent);
        int scale_y(int percent);

        double delta_time();
        double time_since_start();
        bool interval_elapsed(double seconds);

        bool key_pressed(int key);
        bool key_held(int key);

        bool mouse_pressed(int button);
        bool mouse_held(int button);

        glm::dvec2 mouse_position();
        glm::dvec2 mouse_delta();

        bool should_close();
        void close();

        void set_mouse_position(const glm::dvec2& position);
        void set_mouse_mode(int mode);

    private:
        std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window{nullptr, &glfwDestroyWindow};
        glm::ivec2 m_window_size{1920, 1080};
        glm::ivec2 m_window_position{0, 0};

        std::bitset<GLFW_KEY_LAST> m_previous_keys{};
        std::bitset<GLFW_KEY_LAST> m_current_keys{};

        glm::dvec2 m_mouse_position{0.0f, 0.0f};
        glm::dvec2 m_mouse_delta{0.0f, 0.0f};
        std::bitset<GLFW_MOUSE_BUTTON_LAST> m_previous_buttons{};
        std::bitset<GLFW_MOUSE_BUTTON_LAST> m_current_buttons{};

        std::chrono::steady_clock::time_point m_start_time{};
        std::chrono::steady_clock::time_point m_last_time{};
        std::chrono::steady_clock::time_point m_current_time{};
        std::chrono::steady_clock::time_point m_last_interval{};
        double m_delta_time = 0.0f;
    };
}