#include "dialog_utils.hpp"

#include "portable-file-dialogs.h"

#if defined(_WIN32)
#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#endif
#include <filesystem>

namespace fin
{
    std::string get_executable_path()
    {
#if defined(_WIN32)
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return std::string(buffer);
#else
        char    buffer[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1)
        {
            buffer[len] = '\0';
            return std::string(buffer);
        }
        else
        {
            perror("readlink");
            exit(1);
        }
#endif
    }

    void RunCurrentProcess(const std::vector<std::string>& args)
    {
        std::string exePath = get_executable_path();

#if defined(_WIN32)
        std::string cmd = "\"" + exePath + "\"";
        for (const auto& arg : args)
        {
            cmd += " " + arg;
        }

        STARTUPINFOA        si = {sizeof(si)};
        PROCESS_INFORMATION pi;

        if (!CreateProcessA(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
#else
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(exePath.c_str()));
            for (const auto& arg : args)
            {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            execv(exePath.c_str(), argv.data());
            perror("execv failed");
            exit(1);
        }
        else if (pid > 0)
        {
            // Parent
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            perror("fork failed");
        }
#endif
    }

    MessageButton ShowMessage(const std::string& title, const std::string& message, MessageType buttons, MessageIcon icon)
    {
        pfd::message dialog(title, message, pfd::choice(buttons), pfd::icon(icon));
        auto         res = dialog.result();
        return MessageButton(res);
    }

    void ShowInExplorer(const std::string& path)
    {
        auto pth = std::filesystem::absolute(path);
#ifdef _WIN32
        std::string command = "explorer \"" + pth.string() + "\"";
        system(command.c_str());
#elif __linux__
        std::string command = "xdg-open \"" + pth.string() + "\"";
        system(command.c_str());
#else
        // Unknown platform
#endif
    }

    std::vector<std::string> CreateFileFilter(const std::string& str)
    {
        std::vector<std::string> result;
        size_t                   start = 0, end;

        while ((end = str.find('|', start)) != std::string::npos)
        {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        result.push_back(str.substr(start)); // Last part

        return result;
    }

    std::vector<std::string> OpenFileDialog(const std::string&       title,
                                              const std::string&       initial_path,
                                              std::vector<std::string> filters,
                                              bool                     multiselect)
    {
        auto res = pfd::open_file::open_file(title, initial_path, filters, multiselect ? pfd::opt::multiselect : pfd::opt::none);
        return res.result();
    }

    std::string SaveFileDialog(const std::string& title, const std::string& initial_path, std::vector<std::string> filters)
    {
        auto res = pfd::save_file::save_file(title, initial_path, filters, pfd::opt::none);
        return res.result();
    }


    std::string OpenFolderDialog(const std::string& title, const std::string& initial_path)
    {
        auto res = pfd::select_folder(title, initial_path, pfd::opt::force_path);
        return res.result();
    }


} // namespace fin
