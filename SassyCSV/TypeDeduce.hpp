#ifndef _T_DEDUCE
#define _T_DEDUCE

#include "CSVParser.hpp"
#include <cmath>

int _char_to_int(char& c) {
    int n = static_cast<int>(c) - 48;
    if ((n >= 0) && (n < 10)) {
        return n;
    }
    return -1;
}

bool _is_char_num(char& c) {
    return (_char_to_int(c) != -1);
}

int _parse_int(int gather, char c) {
    int n = _char_to_int(c);

    return (gather * 10) + n;
}

double _parse_double(bool passed_comma, double gather, char c, int* decimal_point) {
    int n = _char_to_int(c);
    if (!passed_comma) {
        gather = (gather*10.0) + static_cast<double>(n);
    }
    else {
        gather += static_cast<double>(n) / static_cast<double>(std::pow(10 , (*decimal_point)));
        (*decimal_point)++;
    }
    return gather;
}

// iterate through an entry and determine it's type
CSV_datavar process_entry(std::string& entry, char& ignore, char& float_point) {
    if (entry.empty()) {
        return entry;
    }
    
    bool is_double = true;
    bool is_int = true;

    double d = 0;
    int decimal = 1;
    int i = 0;

    int sign = 1;
    int skip = 0;
    if (entry.starts_with('-')) {
        sign = -1;
        skip = 1;
    }
    if (entry.starts_with('+')) {
        skip = 1;
    }

    bool dot = false;

    for (auto& c : entry) {
        // skip - / +
        if (skip > 0) {
            skip -= 1;
            continue;
        }

        if (c == ignore) {
            continue;
        }

        if (c == float_point && !dot) {
            dot = true;
            is_int = false;
            continue;
        }
        else if (c == float_point && dot) {
            is_int = false;
            is_double = false;
            break;
        }

        if (!_is_char_num(c)) {
            is_int = false;
            is_double = false;
            break;
        }

        if (is_int) {
            i = _parse_int(i, c);

        }
        
        if (is_double) {
            d = _parse_double(dot, d, c, &decimal);
        }

    }
    if (is_int && !dot) {
        return  (i * sign) ;
    }
    if (is_double) {
        return (d * static_cast<double>(sign));
    }
    return entry;
}

#endif