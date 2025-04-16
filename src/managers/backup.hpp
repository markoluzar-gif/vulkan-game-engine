#pragma once

namespace marko_engine
{

	class Backup
	{
	public:
		Backup(const Backup&) = delete;
		Backup(Backup&&) = delete;
		Backup& operator=(const Backup&) = delete;
		Backup& operator=(Backup&&) = delete;

	private:
		Backup() = default;

	public:
		void initialize();
		void cleanup();

		static Backup& get();

		void save_object_state();
		void load_object_state();
		void save_temp_object_state();
		void load_temp_object_state();
	};

} 