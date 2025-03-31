#include "dialog_utils.hpp"
#include "portable-file-dialogs.h"

namespace fin
{

std::vector<std::string> create_file_filter(const std::string &str)
{
    std::vector<std::string> result;
    size_t start = 0, end;

    while ((end = str.find('|', start)) != std::string::npos)
    {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(str.substr(start)); // Last part

    return result;
}

std::vector<std::string> open_file_dialog(const std::string &title,
                                          const std::string &initial_path,
                                          std::vector<std::string> filters,
                                          bool multiselect)
{
    auto res = pfd::open_file::open_file(title,
                                         initial_path,
                                         filters,
                                         multiselect ? pfd::opt::multiselect : pfd::opt::none);
    return res.result();
}

std::string save_file_dialog(const std::string &title,
          const std::string &initial_path,
          std::vector<std::string> filters)
{
    auto res = pfd::save_file::save_file(title,
                                         initial_path,
                                         filters,
                                         pfd::opt::none);
    return res.result();
}

} // namespace fin
