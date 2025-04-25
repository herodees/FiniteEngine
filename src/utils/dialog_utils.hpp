#pragma once

#include <string>
#include <vector>

namespace fin
{
std::vector<std::string> open_file_dialog(const std::string &title,
                                          const std::string &initial_path,
                                          std::vector<std::string> filters = {"All Files", "*"},
                                          bool multiselect = false);

std::string save_file_dialog(const std::string &title,
                             const std::string &initial_path,
                             std::vector<std::string> filters = {"All Files", "*"});

std::vector<std::string> create_file_filter(const std::string &str);


void run_current_process(const std::vector<std::string>& args);

} // namespace fin
