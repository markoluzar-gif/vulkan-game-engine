#include "pch.h"
#include "window.hpp"

void MarkoEngine::Window::initialize()
{
    if (!glfwInit())
    {
        throw std::runtime_error("failed to initialize glfw!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window.reset(glfwCreateWindow(m_window_size.x, m_window_size.y, "MarkoEngine", nullptr, nullptr));

    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("failed to create glfw window!");
    }

    if (GLFWmonitor* primary = glfwGetPrimaryMonitor())
    {
        if (const GLFWvidmode* mode = glfwGetVideoMode(primary))
        {
            m_window_position.x = (mode->width - m_window_size.x) / 2;
            m_window_position.y = (mode->height - m_window_size.y) / 2;
            glfwSetWindowPos(m_window.get(), m_window_position.x, m_window_position.y);
        }
    }

    m_start_time = m_last_time = m_current_time = m_last_interval = std::chrono::steady_clock::now();

    m_previous_keys.reset();
    m_current_keys.reset();
    m_previous_buttons.reset();
    m_current_buttons.reset();
}

void MarkoEngine::Window::update()
{
    glfwPollEvents();

    glfwGetWindowSize(m_window.get(), &m_window_size.x, &m_window_size.y);

    const auto now = std::chrono::steady_clock::now();
    m_delta_time = std::chrono::duration<double>(now - m_last_time).count();
    m_last_time = m_current_time = now;

    m_previous_keys = m_current_keys;
    for (int key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST; ++key)
    {
        m_current_keys.set(key, glfwGetKey(m_window.get(), key) == GLFW_PRESS);
    }

    m_previous_buttons = m_current_buttons;
    for (int btn = GLFW_MOUSE_BUTTON_1; btn < GLFW_MOUSE_BUTTON_LAST; ++btn)
    {
        m_current_buttons.set(btn, glfwGetMouseButton(m_window.get(), btn));
    }

    glm::dvec2 new_pos;
    glfwGetCursorPos(m_window.get(), &new_pos.x, &new_pos.y);
    m_mouse_delta = new_pos - m_mouse_position;
    m_mouse_position = new_pos;
}

void MarkoEngine::Window::cleanup()
{
    m_window.reset();
    glfwTerminate();
}

MarkoEngine::Window& MarkoEngine::Window::get()
{
    static Window instance;
    return instance;
}

GLFWwindow* MarkoEngine::Window::window()
{
    return m_window.get();
}

glm::ivec2 MarkoEngine::Window::size()
{
    return m_window_size;
}

glm::ivec2 MarkoEngine::Window::position()
{
    return m_window_position;
}

int MarkoEngine::Window::scale_x(int percent)
{
    return static_cast<int>(m_window_size.x * (percent / 100.0f));
}

int MarkoEngine::Window::scale_y(int percent)
{
    return static_cast<int>(m_window_size.y * (percent / 100.0f));
}

bool MarkoEngine::Window::should_close()
{
    return glfwWindowShouldClose(m_window.get());
}

double MarkoEngine::Window::delta_time()
{
    return m_delta_time;
}

double MarkoEngine::Window::time_since_start()
{
    return std::chrono::duration<double>(m_current_time - m_start_time).count();
}

bool MarkoEngine::Window::interval_elapsed(double seconds)
{
    const auto elapsed = std::chrono::duration<double>(m_current_time - m_last_interval).count();
    if (elapsed >= seconds)
    {
        m_last_interval = m_current_time;
        return true;
    }
    return false;
}

bool MarkoEngine::Window::key_pressed(int key)
{
    return m_current_keys.test(key) && !m_previous_keys.test(key);
}

bool MarkoEngine::Window::key_held(int key)
{
    return m_current_keys.test(key);
}

bool MarkoEngine::Window::mouse_pressed(int button)
{
    return m_current_buttons.test(button) && !m_previous_buttons.test(button);
}

bool MarkoEngine::Window::mouse_held(int button)
{
    return m_current_buttons.test(button);
}

glm::dvec2 MarkoEngine::Window::mouse_position()
{
    return m_mouse_position;
}

glm::dvec2 MarkoEngine::Window::mouse_delta()
{
    return m_mouse_delta;
}

void MarkoEngine::Window::close()
{
    glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE);
}

void MarkoEngine::Window::set_mouse_position(const glm::dvec2& position)
{
    glfwSetCursorPos(m_window.get(), position.x, position.y);
    m_mouse_position = position;
    m_mouse_delta = glm::dvec2(0.0);
}

void MarkoEngine::Window::set_mouse_mode(int mode)
{
    glfwSetInputMode(m_window.get(), GLFW_CURSOR, mode);
}
