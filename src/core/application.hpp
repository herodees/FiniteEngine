#pragma once

#include "include.hpp"

#include "shared_resource.hpp"
#include "scene.hpp"
#include "file_explorer.hpp"

namespace fin
{
	class application : public std::enable_shared_from_this<application>
	{
	public:
		application();
		~application();

        bool on_iterate();
		bool on_init();
        void on_deinit(bool result);

	private:
		void on_imgui_init(bool dark_theme);
		void on_imgui();
		void on_imgui_menu();
		void on_imgui_dialogs();
		void on_imgui_workspace();
		void on_imgui_properties();

		FileEdit* createFileEdit(std::string_view filename);

		bool _show_editor{true};

		FileExplorer _explorer;
		scene _map;
		std::vector<const CDT::Triangle*> _triangles;
		std::vector<std::pair<Vec2f, Vec2f>> _portals;
		std::vector<Vec2f> _path;
		Vec2f _target;

        int targetFPS = 60;
        float fixedFPS = 60.0f;
        float maxFPS = 1200.0f;
        double timeCounter = 0.0;
        double fixedTimeStep = 1.0f / fixedFPS;
        double maxTimeStep = 1.0f / maxFPS;
        double currentTime = GetTime();
        double accumulator = 0.0;
	};
}
