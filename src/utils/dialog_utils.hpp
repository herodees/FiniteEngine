#pragma once

#include <string>
#include <vector>

namespace fin
{
    std::string              open_folder_dialog(const std::string& title, const std::string& initial_path);
    std::vector<std::string> open_file_dialog(const std::string&       title,
                                              const std::string&       initial_path,
                                              std::vector<std::string> filters     = {"All Files", "*"},
                                              bool                     multiselect = false);

    std::string save_file_dialog(const std::string&       title,
                                 const std::string&       initial_path,
                                 std::vector<std::string> filters = {"All Files", "*"});

    std::vector<std::string> create_file_filter(const std::string& str);


    void run_current_process(const std::vector<std::string>& args);

    int messagebox_yes_no(const std::string& title, const std::string& message);
    int messagebox_yes_no_cancel(const std::string& title, const std::string& message);
    int messagebox_ok(const std::string& title, const std::string& message);

} // namespace fin
