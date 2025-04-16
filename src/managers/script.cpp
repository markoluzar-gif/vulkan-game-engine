#include "pch.h"
#include "script.hpp"
#include "window.hpp"

#include "../game_objects/camera_game_object.hpp"
#include "../game_objects/i_game_object.hpp"

static int transform_proxy_index(lua_State* lua_state)
{
    lua_getfield(lua_state, 1, "id");
    std::string id = lua_tostring(lua_state, -1);
    lua_pop(lua_state, 1);
    lua_getfield(lua_state, 1, "prop");
    std::string prop = lua_tostring(lua_state, -1);
    lua_pop(lua_state, 1);
    auto it = I_GAME_OBJECT::game_objects.find(id);
    if (it == I_GAME_OBJECT::game_objects.end())
    {
        lua_pushnil(lua_state);
        return 1;
    }
    transform t = it->second->get_local_transform();
    std::string key = lua_tostring(lua_state, 2);
    float value = 0.0f;
    if (prop == "position")
    {
        if (key == "x")
            value = t.position.x;
        else if (key == "y")
            value = t.position.y;
        else if (key == "z")
            value = t.position.z;
    }
    else if (prop == "rotation")
    {
        if (key == "x")
            value = t.rotation.x;
        else if (key == "y")
            value = t.rotation.y;
        else if (key == "z")
            value = t.rotation.z;
    }
    else if (prop == "scale")
    {
        if (key == "x")
            value = t.scale.x;
        else if (key == "y")
            value = t.scale.y;
        else if (key == "z")
            value = t.scale.z;
    }
    lua_pushnumber(lua_state, value);
    return 1;
}

static int transform_proxy_new_index(lua_State* lua_state)
{
    lua_getfield(lua_state, 1, "id");
    std::string id = lua_tostring(lua_state, -1);
    lua_pop(lua_state, 1);
    lua_getfield(lua_state, 1, "prop");
    std::string prop = lua_tostring(lua_state, -1);
    lua_pop(lua_state, 1);
    std::string key = lua_tostring(lua_state, 2);
    float newValue = static_cast<float> (lua_tonumber(lua_state, 3));
    auto it = I_GAME_OBJECT::game_objects.find(id);
    if (it == I_GAME_OBJECT::game_objects.end())
        return 0;
    transform t = it->second->get_local_transform();
    if (prop == "position")
    {
        if (key == "x")
            t.position.x = newValue;
        else if (key == "y")
            t.position.y = newValue;
        else if (key == "z")
            t.position.z = newValue;
    }
    else if (prop == "rotation")
    {
        if (key == "x")
            t.rotation.x = newValue;
        else if (key == "y")
            t.rotation.y = newValue;
        else if (key == "z")
            t.rotation.z = newValue;
    }
    else if (prop == "scale")
    {
        if (key == "x")
            t.scale.x = newValue;
        else if (key == "y")
            t.scale.y = newValue;
        else if (key == "z")
            t.scale.z = newValue;
    }
    it->second->set_local_transform(t);
    return 0;
}

static int transform_value(lua_State* lua_state)
{
    if (!lua_istable(lua_state, 1) || !lua_isstring(lua_state, 2))
        return 0;
    std::string key = lua_tostring(lua_state, 2);
    if (key == "position" || key == "rotation" || key == "scale")
    {
        lua_newtable(lua_state);
        lua_pushstring(lua_state, "id");
        lua_getfield(lua_state, 1, "id");
        lua_settable(lua_state, -3);
        lua_pushstring(lua_state, "prop");
        lua_pushstring(lua_state, key.c_str());
        lua_settable(lua_state, -3);
        lua_newtable(lua_state);
        lua_pushstring(lua_state, "__index");
        lua_pushcfunction(lua_state, transform_proxy_index);
        lua_settable(lua_state, -3);
        lua_pushstring(lua_state, "__newindex");
        lua_pushcfunction(lua_state, transform_proxy_new_index);
        lua_settable(lua_state, -3);
        lua_setmetatable(lua_state, -2);
        return 1;
    }
    else
    {
        return 0;
    }
}

static int is_key_pressed(lua_State* lua_state)
{
    if (lua_gettop(lua_state) != 1 || !lua_isnumber(lua_state, 1))
    {
        std::cout << "Failed to call is key pressed (keyValue : Key.type)!" << std::endl;
        return 0;
    }
    lua_pushnumber(lua_state, MarkoEngine::Window::get().key_pressed(lua_tonumber(lua_state, 1)));
    return 1;
}

static int is_key_held(lua_State* lua_state)
{
    if (lua_gettop(lua_state) != 1 || !lua_isnumber(lua_state, 1))
    {
        std::cout << "Failed to call is key held (keyValue : Key.type)!" << std::endl;
        return 0;
    }
    lua_pushnumber(lua_state, MarkoEngine::Window::get().key_held(lua_tonumber(lua_state, 1)));
    return 1;
}

static int get_position(lua_State* lua_state)
{

    if (lua_gettop(lua_state) != 1 || !lua_isstring(lua_state, 1))
    {
        std::cout << "Failed to call get_position (Expected ID : string)!" << std::endl;
        return 0;
    }


    std::string id = lua_tostring(lua_state, 1);


    auto it = I_GAME_OBJECT::game_objects.find(id);
    if (it == I_GAME_OBJECT::game_objects.end())
    {
        std::cout << "Game object with ID '" << id << "' not found!" << std::endl;
        return 0; 
    }


    transform t = it->second->get_local_transform();


    lua_pushnumber(lua_state, t.position.x);
    lua_pushnumber(lua_state, t.position.y);
    lua_pushnumber(lua_state, -t.position.z);

    return 3; 
}

void MarkoEngine::Script::initialize()
{
    m_script.reset(luaL_newstate());

    if (!m_script)
    {
        throw std::runtime_error("failed to initialize lua!");
    }

    luaL_openlibs(m_script.get());

    lua_register(m_script.get(), "is_key_pressed", is_key_pressed);
    lua_register(m_script.get(), "is_key_held", is_key_held);
    lua_register(m_script.get(), "get_position", get_position);

    lua_newtable(m_script.get());
    lua_setglobal(m_script.get(), "Time");
    lua_newtable(m_script.get());
    lua_setglobal(m_script.get(), "This");
    lua_newtable(m_script.get());
    lua_setglobal(m_script.get(), "Object");
    lua_newtable(m_script.get());
    lua_setglobal(m_script.get(), "Key");
}

void MarkoEngine::Script::update()
{
}

void MarkoEngine::Script::cleanup()
{
    m_script.reset();
}

MarkoEngine::Script& MarkoEngine::Script::get()
{
    static Script instance;
    return instance;
}

void MarkoEngine::Script::call_awake()
{
    call_function("awake");
}

void MarkoEngine::Script::call_start()
{
    call_function("start");
}

void MarkoEngine::Script::call_update()
{
    call_function("update");
}

void MarkoEngine::Script::call_function(const std::string& func)
{
    for (const auto& object : I_GAME_OBJECT::game_objects)
    {
        if (object.first == "Root" || m_script.get() == nullptr) continue;
        if (object.second->get_script().empty()) continue;

        lua_getglobal(m_script.get(), "Time");
        lua_pushstring(m_script.get(), "deltaTime");
        lua_pushnumber(m_script.get(), Window::get().delta_time());
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "currentTime");
        lua_pushnumber(m_script.get(), Window::get().time_since_start());
        lua_settable(m_script.get(), -3);
        lua_pop(m_script.get(), 1);

        lua_getglobal(m_script.get(), "This");
        lua_pushstring(m_script.get(), "id");
        lua_pushstring(m_script.get(), object.first.c_str());
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "transform");
        lua_newtable(m_script.get());
        lua_pushstring(m_script.get(), "id");
        lua_pushstring(m_script.get(), object.first.c_str());
        lua_settable(m_script.get(), -3);
        lua_newtable(m_script.get());
        lua_pushstring(m_script.get(), "__index");
        lua_pushcfunction(m_script.get(), transform_value);
        lua_settable(m_script.get(), -3);
        lua_setmetatable(m_script.get(), -2);
        lua_settable(m_script.get(), -3);
        lua_pop(m_script.get(), 1);

        lua_getglobal(m_script.get(), "Object");
        lua_pushstring(m_script.get(), "camera");
        lua_pushstring(m_script.get(), "camera");
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "mesh");
        lua_pushstring(m_script.get(), "mesh");
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "model");
        lua_pushstring(m_script.get(), "model");
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "animation");
        lua_pushstring(m_script.get(), "animation");
        lua_settable(m_script.get(), -3);
        lua_pushstring(m_script.get(), "boxCollider");
        lua_pushstring(m_script.get(), "boxCollider");
        lua_settable(m_script.get(), -3);
        lua_pop(m_script.get(), 1);

        lua_getglobal(m_script.get(), "Key");

        lua_pushstring(m_script.get(), "a");
        lua_pushnumber(m_script.get(), GLFW_KEY_A);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "b");
        lua_pushnumber(m_script.get(), GLFW_KEY_B);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "c");
        lua_pushnumber(m_script.get(), GLFW_KEY_C);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "d");
        lua_pushnumber(m_script.get(), GLFW_KEY_D);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "e");
        lua_pushnumber(m_script.get(), GLFW_KEY_E);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f");
        lua_pushnumber(m_script.get(), GLFW_KEY_F);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "g");
        lua_pushnumber(m_script.get(), GLFW_KEY_G);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "h");
        lua_pushnumber(m_script.get(), GLFW_KEY_H);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "i");
        lua_pushnumber(m_script.get(), GLFW_KEY_I);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "j");
        lua_pushnumber(m_script.get(), GLFW_KEY_J);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "k");
        lua_pushnumber(m_script.get(), GLFW_KEY_K);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "l");
        lua_pushnumber(m_script.get(), GLFW_KEY_L);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "m");
        lua_pushnumber(m_script.get(), GLFW_KEY_M);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "n");
        lua_pushnumber(m_script.get(), GLFW_KEY_N);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "o");
        lua_pushnumber(m_script.get(), GLFW_KEY_O);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "p");
        lua_pushnumber(m_script.get(), GLFW_KEY_P);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "q");
        lua_pushnumber(m_script.get(), GLFW_KEY_Q);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "r");
        lua_pushnumber(m_script.get(), GLFW_KEY_R);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "s");
        lua_pushnumber(m_script.get(), GLFW_KEY_S);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "t");
        lua_pushnumber(m_script.get(), GLFW_KEY_T);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "u");
        lua_pushnumber(m_script.get(), GLFW_KEY_U);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "v");
        lua_pushnumber(m_script.get(), GLFW_KEY_V);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "w");
        lua_pushnumber(m_script.get(), GLFW_KEY_W);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "x");
        lua_pushnumber(m_script.get(), GLFW_KEY_X);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "y");
        lua_pushnumber(m_script.get(), GLFW_KEY_Y);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "z");
        lua_pushnumber(m_script.get(), GLFW_KEY_Z);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "0");
        lua_pushnumber(m_script.get(), GLFW_KEY_0);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "1");
        lua_pushnumber(m_script.get(), GLFW_KEY_1);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "2");
        lua_pushnumber(m_script.get(), GLFW_KEY_2);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "3");
        lua_pushnumber(m_script.get(), GLFW_KEY_3);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "4");
        lua_pushnumber(m_script.get(), GLFW_KEY_4);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "5");
        lua_pushnumber(m_script.get(), GLFW_KEY_5);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "6");
        lua_pushnumber(m_script.get(), GLFW_KEY_6);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "7");
        lua_pushnumber(m_script.get(), GLFW_KEY_7);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "8");
        lua_pushnumber(m_script.get(), GLFW_KEY_8);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "9");
        lua_pushnumber(m_script.get(), GLFW_KEY_9);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "space");
        lua_pushnumber(m_script.get(), GLFW_KEY_SPACE);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "enter");
        lua_pushnumber(m_script.get(), GLFW_KEY_ENTER);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "esc");
        lua_pushnumber(m_script.get(), GLFW_KEY_ESCAPE);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "tab");
        lua_pushnumber(m_script.get(), GLFW_KEY_TAB);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "left_arrow");
        lua_pushnumber(m_script.get(), GLFW_KEY_LEFT);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "right_arrow");
        lua_pushnumber(m_script.get(), GLFW_KEY_RIGHT);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "up_arrow");
        lua_pushnumber(m_script.get(), GLFW_KEY_UP);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "down_arrow");
        lua_pushnumber(m_script.get(), GLFW_KEY_DOWN);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f1");
        lua_pushnumber(m_script.get(), GLFW_KEY_F1);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f2");
        lua_pushnumber(m_script.get(), GLFW_KEY_F2);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f3");
        lua_pushnumber(m_script.get(), GLFW_KEY_F3);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f4");
        lua_pushnumber(m_script.get(), GLFW_KEY_F4);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f5");
        lua_pushnumber(m_script.get(), GLFW_KEY_F5);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f6");
        lua_pushnumber(m_script.get(), GLFW_KEY_F6);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f7");
        lua_pushnumber(m_script.get(), GLFW_KEY_F7);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f8");
        lua_pushnumber(m_script.get(), GLFW_KEY_F8);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f9");
        lua_pushnumber(m_script.get(), GLFW_KEY_F9);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f10");
        lua_pushnumber(m_script.get(), GLFW_KEY_F10);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f11");
        lua_pushnumber(m_script.get(), GLFW_KEY_F11);
        lua_settable(m_script.get(), -3);

        lua_pushstring(m_script.get(), "f12");
        lua_pushnumber(m_script.get(), GLFW_KEY_F12);
        lua_settable(m_script.get(), -3);

        const std::string& script_string = object.second->get_script();
        if (script_string.empty() || !std::filesystem::exists(script_string))
            continue;
        if (luaL_dofile(m_script.get(), script_string.c_str()) != LUA_OK)
        {
            std::cerr << "Failed to run lua script: " << script_string
                << " error: " << lua_tostring(m_script.get(), -1)
                << std::endl;
            lua_pop(m_script.get(), 1);
            continue;
        }
        lua_getglobal(m_script.get(), func.c_str());
        if (!lua_isfunction(m_script.get(), -1))
        {

            lua_pop(m_script.get(), 1);
            continue;
        }
        if (lua_pcall(m_script.get(), 0, 0, 0) != LUA_OK)
        {
            std::cerr << "Error calling function '" << func
                << "' in script: " << script_string
                << " error: " << lua_tostring(m_script.get(), -1)
                << std::endl;
            lua_pop(m_script.get(), 1);
            continue;
        }
    }
}

