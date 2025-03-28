#pragma once

#include "include.hpp"

namespace fin
{
	namespace fs = std::filesystem;

	inline std::string_view path_get_ext(std::string_view path)
	{
		size_t dotPos = path.find_last_of('.');
		size_t slashPos = path.find_last_of("/\\");

		if (dotPos == std::string_view::npos || (slashPos != std::string_view::npos && dotPos < slashPos)) {
			return {}; // No extension found
		}
		return path.substr(dotPos);
	}

	inline std::string_view path_get_file(std::string_view path)
	{
		size_t slashPos = path.find_last_of("/\\");
		if (slashPos == std::string_view::npos) {
			return path; // No directory, return the whole path
		}
		return path.substr(slashPos + 1);
	}

	inline std::string_view path_get_dir(std::string_view path)
	{
		size_t slashPos = path.find_last_of("/\\");
		if (slashPos == std::string_view::npos) {
			return {}; // No directory part
		}
		return path.substr(0, slashPos);
	}

	class File
	{
	public:
		File(File&& o) noexcept : _stream{} { std::swap(_stream, o._stream); }
		File(const File& o) = delete;
		explicit File(const std::string& path, const char* mode) {
			_stream = SDL_IOFromFile(path.c_str(), mode);
			if (!_stream) {
				SDL_LogError(0, "Failed to open file: %s - %s", path.c_str(), SDL_GetError());
			}
		}
		~File() { close(); }
		File& operator=(File&& o) noexcept { std::swap(_stream, o._stream); return *this; }
		File& operator=(const File& o) = delete;

		size_t read(void* buffer, size_t size) {
			if (!_stream) return 0;
			return SDL_ReadIO(_stream, buffer, size);
		}
		size_t write(const void* buffer, size_t size) {
			if (!_stream) return 0;
			return SDL_WriteIO(_stream, buffer, size);
		}
		bool seek(long offset, SDL_IOWhence whence) {
			if (!_stream) return false;
			return SDL_SeekIO(_stream, offset, whence) != -1;
		}
		long tell() {
			if (!_stream) return -1;
			return SDL_TellIO(_stream);
		}
		bool isOpen() const { return _stream != nullptr; }
		void close() {
			if (_stream) {
				SDL_CloseIO(_stream);
				_stream = nullptr;
			}
		}
		size_t size() {
			if (!_stream) return 0;
			long currentPos = tell();
			if (currentPos == -1) return 0;
			if (seek(0, SDL_IOWhence::SDL_IO_SEEK_END))
			{
				long fileSize = tell();
				seek(currentPos, SDL_IOWhence::SDL_IO_SEEK_SET);
				return fileSize;
			}
			return 0;
		}

		template <class T>
		bool read(std::vector<T>& out) {
			out.resize(size());
			auto n = read(out.data(), out.size() * sizeof(T));
			return n == out.size() * sizeof(T);
		}

	private:
		SDL_IOStream* _stream{};
	};


	class FileEdit
	{
	public:
		FileEdit() = default;
		virtual ~FileEdit() = default;

		virtual bool on_init(std::string_view path) = 0;
		virtual bool on_deinit() = 0;
		virtual bool on_edit() = 0;
	};


	class FileExplorer
	{
	public:
		std::string rootPath{ "./" };
		std::string currentPath;
		std::vector<std::string> folders;
		std::vector<std::string> files;
		std::unique_ptr<FileEdit> editor;
		std::function<FileEdit* (std::string_view)> createEditor;
		bool inRootFolder{};

		FileExplorer(const std::string& startPath = ".")
		{
			select(startPath);
		}

		void set_root(const std::string& startPath)
		{
			rootPath = fs::absolute(startPath).string();
		}

		template <typename CB>
		void set_editor(CB&& cb)
		{
			createEditor = [cb = std::forward<CB>(cb)](std::string_view filename) {
				return cb(filename); // Calls the function correctly
				};
		}

		void select(const std::string& startPath)
		{
			currentPath = fs::absolute(startPath).string();
			inRootFolder = fs::equivalent(currentPath, rootPath);
			update_file_list();
		}

		void update_file_list()
		{
			folders.clear();
			files.clear();

			try
			{
				for (const auto& entry : fs::directory_iterator(currentPath))
				{
					std::string name = entry.path().filename().string();
					if (fs::is_directory(entry.path()))
						folders.push_back(name);
					else
						files.push_back(name);
				}
			}
			catch (const std::exception& e)
			{
				SDL_LogError(0, "Error reading directory: %s", e.what());
			}
		}

		void render()
		{
			ImGui::Begin("Explorer");

			ImGui::Text("Current Path: %s", currentPath.c_str());
			ImGui::Separator();

			if (editor)
			{
				if (!editor->on_edit())
					editor.reset();
			}
			else
			{
				if (!inRootFolder && ImGui::Selectable("[DIR] .."))
				{
					fs::path parent = fs::path(currentPath).parent_path();
					if (!parent.empty())
					{
						select(parent.string());
					}
				}

				for (const auto& folder : folders)
				{
					std::string label = "[DIR] " + folder;
					if (ImGui::Selectable(label.c_str()))
					{
						select((fs::path(currentPath) / folder).string());
						break;
					}
				}

				for (const auto& file : files)
				{
					if (ImGui::Selectable(file.c_str()))
					{
						if (createEditor)
						{
							editor.reset(createEditor((fs::path(currentPath) / file).string()));
						}
					}
				}
			}

			ImGui::End();
		}
	};



}