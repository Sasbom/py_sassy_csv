#ifndef _UTIL
#define _UTIL

#include <string>
#include <string_view>
#include <vector>
#include <pybind11/pybind11.h>

namespace py = pybind11;

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

std::string keytuple_to_str(py::tuple const & tup) {
    std::size_t size = tup.size();
    std::string collect{};

    for (std::size_t i{ 0 }; i < size; i++) {
        collect += std::string(py::str(tup[i]));

        if (i < size - 1) {
            collect += '\x1f';
        }
    }
    return collect;
}


#endif