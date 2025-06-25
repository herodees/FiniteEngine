#pragma once

#include <string>
#include <vector>

namespace fin
{
    std::string              OpenFolderDialog(const std::string& title, const std::string& initial_path);
    std::vector<std::string> OpenFileDialog(const std::string&       title,
                                            const std::string&       initial_path,
                                            std::vector<std::string> filters     = {"All Files", "*"},
                                            bool                     multiselect = false);

    std::string SaveFileDialog(const std::string&       title,
                               const std::string&       initial_path,
                               std::vector<std::string> filters = {"All Files", "*"});

    std::vector<std::string> CreateFileFilter(const std::string& str);


    void RunCurrentProcess(const std::vector<std::string>& args);
    void ShowInExplorer(const std::string& path);

    enum class MessageType
    {
        Ok = 0,
        OkCancel,
        YesNo,
        YesNoCancel,
        RetryCancel,
        AbortRetryCancel,
    };

    enum class MessageButton
    {
        Cancel = -1,
        Ok,
        Yes,
        No,
        Abort,
        Retry,
        Ignore,
    };

    enum class MessageIcon
    {
        Info = 0,
        Warning,
        Error,
        Question,
    };

    MessageButton ShowMessage(const std::string& title,
                    const std::string& message,
                    MessageType        buttons = MessageType::Ok,
                    MessageIcon        icon    = MessageIcon::Info);

} // namespace fin
