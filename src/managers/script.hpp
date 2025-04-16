#pragma once
#include <lua/lua.hpp>
#include <memory>

namespace MarkoEngine
{
    class Script final
    {
    public: 
        Script(const Script&) = delete;
        Script(Script&&) = delete;
        Script& operator= (const Script&) = delete;
        Script& operator= (Script&&) = delete;
    private:
        Script() = default;

    public:
        void initialize();
        void update();
        void cleanup();

    public: 
        static Script& get();

        void call_awake();
        void call_start();
        void call_update();

    private:
        void call_function(const std::string& func);

    private: 
        std::unique_ptr <lua_State, decltype(&lua_close)> m_script{nullptr, &lua_close};
    };
}