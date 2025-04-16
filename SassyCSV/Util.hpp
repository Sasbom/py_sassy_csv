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

std::size_t longest_entry(std::vector<std::string>& entries) {
    std::size_t maxsize{ 0 };
    for (auto& entry : entries) {
        if (maxsize < entry.size()) {
            maxsize = entry.size();
        }
    }
    return maxsize;
}

void rightpad_string(std::string& string, std::size_t desired_length) {
    while (string.size() < desired_length) {
        string.push_back(' ');
    }
}

void exclude_char_string(std::string& string, char c = '\n', char const * r = "¶") {
    while (string.find(c) != string.npos) {
        auto pos = string.find(c);
        string.replace(string.begin() + pos, string.begin() + pos + 1, r);
    }
}

std::string entry_as_string(std::shared_ptr<CSVEntry>& entry) {
    auto& data = entry->data;

    switch (data.index()) {
    case 0:
        return std::get<0>(data);
    case 1:
        return std::to_string(std::get<1>(data));
    case 2:
        return std::to_string(std::get<2>(data));
    }
}

#endif