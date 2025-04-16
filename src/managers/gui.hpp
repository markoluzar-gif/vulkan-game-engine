#pragma once
#include <string>

namespace MarkoEngine
{
	class Gui
	{
	public: 
		Gui(const Gui&) = delete;
		Gui(Gui&&) = delete;
		Gui& operator=(const Gui&) = delete;
		Gui& operator=(const Gui&&) = delete;
	private:
		Gui() = default;

	public: 
		void initialize();
		void update();
		void cleanup();

	public: 
		static Gui& get();
		
		bool playing();
		bool prev_playing();

	private: 
		void draw_inspector();
		void draw_hierarchy();
		void draw_content();
		void draw_top_bar();

	private: 
		unsigned long long folder_tex = -1, file_tex = -1, alert_tex = -1,
		image_tex = -1, play_tex = -1, stop_tex = -1, export_tex = -1,
		box_tex = -1, cuboid_tex = -1, visible_tex = -1, hidden_tex = -1;

		char buffer[128];
		bool is_playing;
		bool prev_is_playing;
		std::string object_to_rename;
		std::string selected_game_object;
		std::string curr_folder;
		std::string dragged_object_id;
		std::string file_to_rename;
	};
}