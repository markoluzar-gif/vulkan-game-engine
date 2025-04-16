#include "pch.h"
#include "backup.hpp"
#include "window.hpp"
#include "../game_objects/i_game_object.hpp"

namespace marko_engine
{

    Backup& Backup::get()
    {
        static Backup instance;
        return instance;
    }

    void Backup::initialize()
    {
        if (!std::filesystem::exists("backups"))
        {
            std::filesystem::create_directory("backups");
        }
    }

    void Backup::cleanup()
    {
        std::filesystem::remove("backups/temp_object_state.bin");
    }

    void Backup::save_object_state()
    {
        std::vector<std::string> backup_folders;
        for (const auto& entry : std::filesystem::directory_iterator("backups"))
        {
            if (entry.is_directory())
            {
                std::string name = entry.path().filename().string();
                if (std::regex_match(name, std::regex("backup[0-9]+")))
                {
                    backup_folders.push_back(name);
                }
            }
        }

        std::sort(backup_folders.begin(), backup_folders.end(), [](const std::string& a, const std::string& b) {
            int num_a = std::stoi(a.substr(6));
            int num_b = std::stoi(b.substr(6));
            return num_a > num_b;
            });

        for (const auto& folder : backup_folders)
        {
            int num = std::stoi(folder.substr(6));
            std::string new_name = "backups/backup" + std::to_string(num + 1);
            std::filesystem::rename("backups/" + folder, new_name);
        }

        std::filesystem::create_directory("backups/backup1");

        if (std::filesystem::exists("backups/backup1/game_objects.bin"))
        {
            std::filesystem::rename("backups/backup1/game_objects.bin", "backups/backup1/game_objects.bin.bak");
        }
        I_GAME_OBJECT::save_to_binary("backups/backup1/game_objects.bin");
    }

    void Backup::load_object_state()
    {
        I_GAME_OBJECT::load_from_binary("backups/backup1/game_objects.bin");
    }

    void Backup::save_temp_object_state()
    {
        std::filesystem::remove("backups/temp_object_state.bin");

        std::ofstream datao("backups/temp_object_state.bin", std::ios::binary | std::ios::app);
        if (datao.is_open())
        {
            std::cout << "SAVE!";
            for (auto& game_object : I_GAME_OBJECT::game_objects)
            {
                transform t = game_object.second->get_local_transform();
                datao.write(reinterpret_cast<const char*>(&t.position), sizeof(glm::vec3));
                datao.write(reinterpret_cast<const char*>(&t.rotation), sizeof(glm::vec3));
                datao.write(reinterpret_cast<const char*>(&t.scale), sizeof(glm::vec3));
            }
            datao.close();
        }
    }

    void Backup::load_temp_object_state()
    {
        std::ifstream datai("backups/temp_object_state.bin", std::ios::binary);
        if (datai.is_open())
        {
            std::cout << "LOAD!";
            glm::vec3 position, rotation, scale;
            for (auto& game_object : I_GAME_OBJECT::game_objects)
            {
                datai.read(reinterpret_cast<char*>(&position), sizeof(glm::vec3));
                datai.read(reinterpret_cast<char*>(&rotation), sizeof(glm::vec3));
                datai.read(reinterpret_cast<char*>(&scale), sizeof(glm::vec3));
                game_object.second->set_local_transform(transform(position, rotation, scale));
            }
            datai.close();
        }
    }

}