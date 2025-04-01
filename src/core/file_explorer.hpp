#pragma once

#include "editor/file_editor.hpp"
#include "extras/IconsFontAwesome6.h"
#include "include.hpp"

namespace fin
{
namespace fs = std::filesystem;

inline std::string_view path_get_ext(std::string_view path)
{
    size_t dotPos = path.find_last_of('.');
    size_t slashPos = path.find_last_of("/\\");

    if (dotPos == std::string_view::npos ||
        (slashPos != std::string_view::npos && dotPos < slashPos))
    {
        return {}; // No extension found
    }
    return path.substr(dotPos);
}

inline std::string_view path_get_file(std::string_view path)
{
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string_view::npos)
    {
        return path; // No directory, return the whole path
    }
    return path.substr(slashPos + 1);
}

inline std::string_view path_get_dir(std::string_view path)
{
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string_view::npos)
    {
        return {}; // No directory part
    }
    return path.substr(0, slashPos);
}


class FileExplorer
{
public:
    std::string rootPath{"./"};
    std::string currentPath;
    std::vector<std::string> folders;
    std::vector<std::string> files;
    std::unique_ptr<FileEdit> editor;
    bool inRootFolder{};

    FileExplorer(const std::string &startPath = ".")
    {
        select(startPath);
    }

    void set_root(const std::string &startPath)
    {
        rootPath = startPath;
    }

    void select(const std::string &startPath)
    {
        currentPath = startPath;
        inRootFolder = fs::equivalent(currentPath, rootPath);
        update_file_list();
    }

    void update_file_list()
    {
        folders.clear();
        files.clear();

        try
        {
            for (const auto &entry : fs::directory_iterator(currentPath))
            {
                std::string name = entry.path().filename().string();
                if (fs::is_directory(entry.path()))
                    folders.push_back(name);
                else
                    files.push_back(name);
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Error reading directory: %s", e.what());
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
            if (!inRootFolder && ImGui::Selectable(ICON_FA_FOLDER " .."))
            {
                fs::path parent = fs::path(currentPath).parent_path();
                if (!parent.empty())
                {
                    select(parent.string());
                }
            }

            for (const auto &folder : folders)
            {
                std::string label = ICON_FA_FOLDER " " + folder;
                if (ImGui::Selectable(label.c_str()))
                {
                    select((fs::path(currentPath) / folder).string());
                    break;
                }
            }

            for (const auto &file : files)
            {
                if (ImGui::Selectable(file.c_str()))
                {
                    editor.reset(
                        FileEdit::create_editor_from_file((fs::path(currentPath) / file).string()));
                }
            }
        }

        ImGui::End();




        if (!ImGui::Begin("Setup"))
        {
            ImGui::End();
            return;
        }

        if (editor && editor->on_setup())
        {
        }

        ImGui::End();
    }
};


} // namespace fin
