#ifndef _UTIL
#define _UTIL

#include <string>
#include <string_view>
#include <vector>
#include <pybind11/pybind11.h>
#include <cmath>

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

std::size_t longest_entry_exclude(std::vector<std::string>& entries, std::unordered_set<std::size_t>& excludes ) {
    std::size_t maxsize{ 0 };
    std::size_t c{ 0 };
    for (auto& entry : entries) {
        if (excludes.contains(c)) {
            c++;
            continue;
        }
        if (maxsize < entry.size()) {
            maxsize = entry.size();
        }
        c++;
    }
    return maxsize;
}

void rightpad_string(std::string& string, std::size_t desired_length) {
    while (string.size() < desired_length) {
        string.push_back(' ');
    }
}

void exclude_char_string(std::string& string, char c = '\n', char const * r = "\\") {
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

double round_double(double to_round, int decimals) {
    return std::round(to_round * (std::pow(10, decimals))) / std::pow(10, decimals);
}

std::string format_int_international(long int const& value, std::string const& separator) {
    std::string collect{};
    std::string_view num = std::to_string(value);
    std::size_t start = 3 - (num.size() % 3);
    std::size_t c{ start };
    for (auto i = num.begin(); i != num.end(); i++, c++) {
        collect += *i;
        if (c % 3 == 2 && c - start != num.size() - 1) {
            collect += separator;
        }
    }

    return collect;
}

std::string format_int_india(long int const& value, std::string const& separator) {
    std::string collect{};
    std::string_view num = std::to_string(value);
    bool first = false;
    std::size_t c = 0;
    for (auto i = num.rbegin(); i != num.rend(); i++, c++) {
        collect.insert(0, std::string{ *i });
        if (!first && c == 2) {
            collect.insert(0, separator);
            first = true;
        }
        else if (first && (c + 2) % 2 == 0 && c < num.size() - 1) {
            collect.insert(0, separator);
        }
    }
    return collect;
}

std::string format_double_india(double const& value, std::string const& separator, std::string const& float_point, int round = 2) {
    long int integral_part = std::trunc(value);
    double remainder = value - static_cast<double>(integral_part);
    remainder = round_double(remainder, round);
    auto rem_str = std::to_string(remainder).erase(0, round);
    rem_str.resize(round);
    std::string collect = format_int_india(integral_part, separator) + float_point + rem_str;
    return collect;
}

std::string format_double_international(double const& value, std::string const& separator, std::string const& float_point, int round = 2) {
    long int integral_part = std::trunc(value);
    double remainder = value - static_cast<double>(integral_part);
    remainder = round_double(remainder, round);
    auto rem_str = std::to_string(remainder).erase(0, round);
    rem_str.resize(round);
    std::string collect = format_int_international(integral_part, separator) + float_point + rem_str;
    return collect;
}

#endif