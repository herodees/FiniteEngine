#pragma once

#include "include.hpp"

#include "dialogs.hpp"
#include "graphics.hpp"
#include "scene.hpp"
#include "file_explorer.hpp"

namespace fin
{
	class application : public std::enable_shared_from_this<application>
	{
	public:
		application();
		~application();
		SDL_AppResult on_event(SDL_Event* event);
		SDL_AppResult on_iterate();
		SDL_AppResult on_init();
		void on_deinit(SDL_AppResult result);

	private:
		void on_imgui_init(bool dark_theme);
		void on_imgui_begin();
		void on_imgui();
		void on_imgui_menu();
		void on_imgui_dialogs();
		void on_imgui_workspace();
		void on_imgui_properties();
		void on_imgui_end();

		FileEdit* createFileEdit(std::string_view filename);

		SDL_Window* _window{};
		SDL_Renderer* _renderer{};
		SDL_AudioDeviceID _audioDevice{};
		SDL_AppResult _app_quit = SDL_APP_CONTINUE;
		Uint64 _time{};
		float _delta_time{};
		bool _show_editor{true};


		Texture _txt;
		Texture _canvas;
		FileExplorer _explorer;
		scene _map;
		std::vector<const CDT::Triangle*> _triangles;
		std::vector<std::pair<Vec2f, Vec2f>> _portals;
		std::vector<Vec2f> _path;
		Vec2f _target;
	};
}