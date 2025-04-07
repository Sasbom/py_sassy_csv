#ifndef _UTIL
#define _UTIL

#include <string>
#include <string_view>
#include <vector>

std::vector<std::string> split_str(std::string& str, char delim = '\x1f') {
    std::vector<std::string> vec{};

    std::string_view view{ str };
    std::size_t last_pos = 0;

    while (view.find(delim, last_pos) != view.npos) {
        std::size_t pos = view.find(delim, last_pos);
        vec.push_back(std::string(view.substr(last_pos, pos - last_pos)));
        last_pos = pos + 1;
    }
    if (last_pos < view.size()) {
        vec.push_back(std::string(view.substr(last_pos, view.size() - last_pos)));
    }
    return vec;
}

#endif