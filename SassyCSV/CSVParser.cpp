#include "CSVParser.hpp"
#include "TypeDeduce.hpp"
#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/typing.h>
#include <pybind11/stl.h>
#include <memory>
#include <fstream>
#include <exception>
#include <ranges>
#include <iostream>

namespace py = pybind11;
// constructors

// Entry
CSVEntry::CSVEntry(CSV_datavar const& data) : data{ data } {};

// Data
std::vector<std::string> CSVData::read_headers() {
	return this->headers;
}

CSVData::data_t CSVData::read_column(std::string const & key) {
	if (!this->data.contains(key))
		throw py::key_error("Provided key does not exist in data.");
	return this->data.at(key);
}

// Parser
// Parser Options
//CSVParser::CSVOptions::CSVOptions() {};

CSVParser::CSVOptions::CSVOptions(CSVParser::CSVOptions const & opts) : 
	delimiter{ opts.delimiter },
	quote{ opts.quote },
	parse_numbers{ opts.parse_numbers },
	float_delimiter{ opts.float_delimiter },
	expected_delimiters{ opts.expected_delimiters },
	header_lines{ opts.header_lines } {};

CSVParser::CSVOptions::CSVOptions(
	std::string_view const& delimiter,
	std::string_view const& quote,
	std::string_view const& newline,
	bool const & parse_numbers,
	std::string_view const& float_delimiter,
	std::string_view const& float_ignore,
	int const & expected_delimiters,
	int const & header_lines
) : delimiter{ delimiter },
	quote{ quote }, 
	newline{newline},
	parse_numbers{ parse_numbers }, 
	float_delimiter{ float_delimiter }, 
	expected_delimiters{ expected_delimiters },
	header_lines{ header_lines } {};

// Parser Main
CSVParser::CSVParser(
	std::string_view const & delimiter,
	std::string_view const & quote,
	std::string_view const& newline,
	bool const & parse_numbers,
	std::string_view const & float_delimiter,
	std::string_view const& float_ignore,
	int const & expected_delimiters,
	int const & header_lines
) : options{ CSVOptions(delimiter, quote, newline, parse_numbers, float_delimiter, float_ignore ,expected_delimiters, header_lines) } {};

// class member functions
// Entry
py::object CSVEntry::py_read() {
	// using CSV_datavar = std::variant<std::string_view, int, double>;
	switch (data.index()) {
	case 0:
		return py::str(std::get<0>(data));
	case 1:
		return py::int_(std::get<1>(data));
	case 2:
		return py::float_(static_cast<double>(std::get<2>(data)));
	}
	return py::none();
}

void CSVEntry::set_data(CSV_datavar const& data) {
	this->data = data;
}
// parser

std::shared_ptr<CSVData> CSVParser::parse(std::string_view const& file) {
	std::shared_ptr<CSVData> csv_data = std::make_shared<CSVData>();
	int headers = options.header_lines;
	int cur_header = 0;
	int idx_header = 0;
	bool get_headers = true;
	// we can already set the header count for unrolling header vector
	csv_data->header_count = headers; 

	int expected_delimiters = options.expected_delimiters;
	int delimiters_togo = 0;
	// sentinel value should indicate comma counting	
	bool count_commas = (expected_delimiters == -1);
	if (count_commas)
		expected_delimiters = 0;

	std::ifstream file_stream{};
	file_stream.open(file.data());
	char cur_char{};
	std::string collect;
	std::vector<std::string> collect_line{};

	char quote = options.quote.data()[0];
	char delimiter = options.delimiter.data()[0];
	char newline = options.newline.data()[0];

	char float_delimiter = options.float_delimiter.data()[0];
	char float_ignore = options.float_ignore.data()[0];

	std::cout << quote << " " << delimiter << " " << newline;
	int size = 0;
	std::cout << "reading file" << "\n";
	while (file_stream){
		file_stream.get(cur_char);
		//std::cout << std::string{ cur_char } << " ";
		if (cur_char == delimiter || (cur_char == newline && collect.ends_with(quote)) || (cur_char == newline && collect.ends_with(delimiter)) || file_stream.eof()) {
			if (cur_char == newline && collect.ends_with(delimiter)) {
				collect.pop_back();
			}

			while (!collect.starts_with(quote)) {
				collect.erase(0, 1);
			}
			while (!collect.ends_with(quote)) {
				collect.pop_back();
			}
			
			if (collect.ends_with(quote) && collect.starts_with(quote)) {
				collect.pop_back();
				collect.erase(0,1);
				collect_line.push_back(collect);
				py::print("collect:", collect);
				collect.clear();
				
				// when a word gets added, that's the moment we count a delimiter.
				if (count_commas) {
					expected_delimiters += 1;
				}
				else {
					delimiters_togo -= 1;
					if (delimiters_togo < 0) {
						throw std::runtime_error("Delimiters amount need to be similar across lines");
					}
				}
				if (cur_char != newline)
					continue;
				else {
					std::cout << "new line!" << get_headers << " " << delimiters_togo << "\n";
				}
			}
		}

		if (cur_char == newline) {
			if (cur_header < headers && get_headers) {
				// header phase
				if (cur_header == 0) {
					count_commas = false;
					cur_header += 1;
				}
				if (cur_header == headers - 1) {
					get_headers = false;
					
				}
				if (delimiters_togo == 0) {
					std::cout << "collecting headers\n";
					for (auto el : collect_line) {
						py::print(el);
						
						csv_data->headers.push_back(el);
						if (csv_data->data.contains(el)) {
							// this will eventually have to be solved by somehow concatenating multiple headers.
							// a double header with
							// "Polish","Blocking"
							// "Assigned","Assigned"
							// is still valid, just need to find some uncommon character to split them.
							throw std::runtime_error("Headers with same name encountered.");
						}
						auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
						csv_data->data.insert({ el, *vec });
					}
					collect_line.clear();
					cur_header += 1;
					delimiters_togo = expected_delimiters;
					continue;
				}
			}
			else {
				// reset delimiters if appropriate
				if (delimiters_togo == 0 || file_stream.eof()) {
					std::cout << "adding line\n";
					delimiters_togo = expected_delimiters;
					int c = 0;
					for (auto& el : collect_line) {
						auto data = process_entry(el, float_ignore, float_delimiter);
						std::cout << data.index() << " <- variant index\n";
						auto entry = std::make_shared<CSVEntry>(data);
						auto header_entry = csv_data->headers[c];
						py::print(header_entry,">", entry->py_read());
						c += 1;
						auto& vec = csv_data->data.at(header_entry);
						vec.push_back(entry);
					}
					size += 1;
					collect_line.clear();
					continue;
				}
				std::cout << "current char is newline\n";
				collect += cur_char;
				continue;
			}
			
		}
		if ((collect.ends_with(quote) || collect.ends_with(delimiter)) && cur_char == newline) {
			continue;
		}

        // passed all edge cases, proceed collecting ...
		collect += cur_char;
	}
	
	file_stream.close();
	
	// for (auto& h : csv_data->headers) {
	// 	std::cout << h << " | ";
	// }
	// std::cout << "\n";
	// for (int i{ 0 }; i < size; i ++) {
	// 	for (auto& el : csv_data->headers ) {
	// 		auto& vec = csv_data->data.at(el);
	// 		std::cout << std::get<0>(vec[i]->data) << " | ";
	// 	}
	// 	std::cout << "\n";
	// }

	std::cout << "done parsing?\n";
	return csv_data;
}