#include "CSVParser.hpp"
#include "TypeDeduce.hpp"
#include "Util.hpp"
#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/typing.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <memory>
#include <fstream>
#include <exception>
#include <ranges>
#include <iostream>

namespace py = pybind11;
using namespace py::literals;
// constructors

// Entry
CSVEntry::CSVEntry(CSV_datavar const& data) : data{ data } {};

void CSVEntry::deduce_index() {
	int c{};
	if (this->origin == nullptr) {
		return;
	}
	for (auto& el : *this->origin) {
		if (this->shared_from_this() == el) {
			this->index = c;
			return;
		}
		c++;
	}
}

// Data

int CSVData::get_size() {
	return this->size;
}

std::vector<std::string> CSVData::read_headers() {
	return this->headers;
}

py::object CSVData::read_headers_py() {
	py::list lst{};
	for (auto el : this->headers) {
		if (el.contains('\x1f')) {
			auto els = split_str(el);
			auto tup = py::tuple(els.size());
			int c{ 0 };
			for (auto els_el : els) {
				tup[c] = els_el;
				c++;
			}
			lst.append(tup);
		}
		else {
			lst.append(py::str(el));
		}
	}
	return lst;
}

CSVData::data_t CSVData::read_column_str(std::string const & key) {
	if (!this->data.contains(key))
		throw py::key_error("Provided key does not exist in data.");
	return this->data.at(key);
}

CSVData::data_t CSVData::read_column_py(py::tuple tuple_key) {
	std::size_t size = tuple_key.size();
	std::string collect{};
	
	for (std::size_t i{ 0 }; i < size; i++) {
		collect += std::string(py::str(tuple_key[i]));
		
		if (i < size - 1) {
			collect += '\x1f';
		}
	}

	if (!this->data.contains(collect))
		throw py::key_error("Provided key does not exist in data.");
	return this->data.at(collect);
}

CSVData::data_t CSVData::read_row_elements(int index) {
	if (index < 0) {
		index = (this->size - 1) - index;
	}
	if (index < 0 || index >(this->size - 1)) {
		throw std::runtime_error("Requested row not in range.");
	}
	CSVData::data_t vec{};
	for (auto el : this->headers) {
		std::shared_ptr<CSVEntry> data = this->data.at(el)[index];
		vec.push_back(data);
	}
	return vec;
}

py::dict CSVData::read_row_py(int index) {
	if (index < 0) {
		index = (this->size - 1) - index;
	}
	if (index < 0 || index >(this->size - 1)) {
		throw py::index_error("Requested row not in range.");
	}
	auto d = py::dict{};

	for (auto el : this->headers) {
		if (el.contains('\x1f')) {
			auto els = split_str(el);
			auto tup = py::tuple(els.size());
			int c{ 0 };
			for (auto els_el : els) {
				tup[c] = els_el;
				c++;
			}
			auto data = this->read_column_py(tup)[index];
			d[tup] = data;
		}
		else {
			auto data = this->read_column_str(el)[index];
			d[py::str(el)] = data;
		}
	}

	return d;
}

// functional functionality
void CSVData::add_ID_header(std::string const & name, bool as_int = false) {
	std::string h{};
	for (int i{1}; i < this->header_count ; i++) {
		h += '\x1f';
	}
	h += name;

	this->headers.push_back(h);
	auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
	this->data.insert({ h, *vec });
	
	
	auto f = [nr = as_int](CSVEntry* e) {
		if (!nr) {
			e->data = (std::string("# ") + std::to_string(e->index));
		}
		else {
			e->data = e->index;
		}
	};


	auto& d_vec = this->data.at(h);
	for (std::size_t i{}; i < this->size; i++) {
		auto entry = std::make_shared<CSVEntry>(0);

		entry->origin = &d_vec;
		entry->func = f;
		d_vec.push_back(entry);
	}
}

void CSVData::add_ID_header_py(py::tuple const& name, bool as_int = false) {
	auto h = keytuple_to_str(name);
	
	this->headers.push_back(h);
	auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
	this->data.insert({ h, *vec });


	auto f = [nr = as_int](CSVEntry* e) {
		if (!nr) {
			e->data = (std::string("# ") + std::to_string(e->index));
		}
		else {
			e->data = e->index;
		}
		};


	auto& d_vec = this->data.at(h);
	for (std::size_t i{}; i < this->size; i++) {
		auto entry = std::make_shared<CSVEntry>(0);

		entry->origin = &d_vec;
		entry->func = f;
		d_vec.push_back(entry);
	}
}

void CSVData::add_new_header(std::string const& name, std::string const& referenced_name) {
	this->headers.push_back(name);
	auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
	this->data.insert({ name, *vec });

	auto read = &this->data.at(referenced_name);

	auto f = [from = read, ref = referenced_name](CSVEntry* e) {
		auto idx = e->index;
		auto other = (*from)[idx];
		auto data = other->py_read();
		e->data = (std::string("Referencing ") + ref + std::string(": ") + std::string(py::str(data)));
	};

	auto& d_vec = this->data.at(name);
	for (std::size_t i{}; i < this->size; i++) {
		auto entry = std::make_shared<CSVEntry>(0);

		entry->origin = &d_vec;
		entry->func = f;
		d_vec.push_back(entry);
	}
}

void CSVData::append_empty_row() {

	for (auto& h : this->headers) {

		auto& vec = this->data.at(h);
		auto entry = std::make_shared<CSVEntry>("");
		entry->func = vec[0]->func; // copy func if present (auto handled by optional)
		entry->origin = &vec;

		vec.push_back(entry);
		this->size++;
	}
}

void CSVData::prepend_empty_row() {

	for (auto& h : this->headers) {

		auto& vec = this->data.at(h);
		auto entry = std::make_shared<CSVEntry>("");
		entry->func = vec[0]->func; // copy func if present (auto handled by optional)
		entry->origin = &vec;

		vec.emplace(std::begin(vec), entry);
		this->size++;
	}
}

void CSVData::add_ref_func_header(std::string const& name, std::string const& other_name, CSVfunc_interface const& function) {
	this->headers.push_back(name);
	auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
	this->data.insert({ name, *vec });

	auto read = &this->data.at(other_name);

	auto f = [from = read, ref = other_name, func = function](CSVEntry* e) {
		auto idx = e->index;
		auto other = (*from)[idx];
		other->update_data();
		auto data = other->data;
		e->data = func(data,idx);
	};

	auto& d_vec = this->data.at(name);
	for (std::size_t i{}; i < this->size; i++) {
		auto entry = std::make_shared<CSVEntry>(0);

		entry->origin = &d_vec;
		entry->func = f;
		d_vec.push_back(entry);
	}
}

void CSVData::add_ref_func_header_py(py::tuple const& name, py::tuple const& other_name, CSVfunc_interface const& function) {
	auto key =keytuple_to_str(other_name);
	auto nname = keytuple_to_str(name);
	this->add_ref_func_header(nname, key, function);
}

void CSVData::add_acc_ref_func_header(std::string const& name, std::vector<std::string> const& other_names, CSVaccfunc_interface const& function) {
	this->headers.push_back(name);
	auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
	this->data.insert({ name, *vec });

	std::vector<data_t*> reads{};
	for (auto& name : other_names) {
		reads.push_back(&this->data.at(name));
	}

	auto f = [froms = reads, refs = other_names, func = function](CSVEntry* e) {
		auto idx = e->index;
		std::vector<CSV_datavar> data{};
		for (auto from : froms) {
			auto other = (*from)[idx];
			other->update_data();
			data.push_back(other->data);
		}
		e->data = func(data, idx);
	};

	auto& d_vec = this->data.at(name);
	for (std::size_t i{}; i < this->size; i++) {
		auto entry = std::make_shared<CSVEntry>(0);

		entry->origin = &d_vec;
		entry->func = f;
		d_vec.push_back(entry);
	}
}

void CSVData::add_acc_ref_func_header_py(py::tuple const& name, std::vector<py::tuple> const& other_names, CSVaccfunc_interface const& function) {
	std::vector<std::string> keys{};
	auto nname = keytuple_to_str(name);
	for (auto& n : other_names) {
		keys.push_back(keytuple_to_str(n));
	}

	this->add_acc_ref_func_header(nname, keys, function);
}

std::string CSVData::format_pretty() {
	std::string collect{};

	std::vector<std::size_t> widths{};
	std::vector<std::vector<std::string>> column_data{};
	for (auto& el : this->headers) {
		auto entrs = split_str(el);
		for (auto& entry : this->data.at(el)) {
			entry->update_data();
			entrs.push_back(entry_as_string(entry));
		}
		auto width = longest_entry(entrs);

		for (auto& str : entrs) {
			exclude_char_string(str);
			rightpad_string(str, width);
		}

		widths.push_back(width);
		column_data.push_back(entrs);
	}

	std::size_t acc_width{ 2 }; // init at 2 for starting "| "
	std::size_t c{ 0 };
	for (auto& w : widths) {
		acc_width += w;

		// account for pipes
		if (c == widths.size()-1) {
			acc_width += 2;
		}
		else {
			acc_width += 3;
		}
		c++;
	}
	

	for (std::size_t i{ 0 }; i < this->size + this->header_count; i++) {
		collect += "| ";

		for (auto v = std::begin(column_data); v != std::end(column_data); v++ ) {
			if (v == std::end(column_data) - 1){
				collect += (*v)[i] + " |";
			}
			else {
				collect += (*v)[i] + " | ";
			}
		}
		collect += '\n';
		
		if ((i == header_count - 1) || i == (this->size + this->header_count - 1)) {
			std::size_t c = 0;
			while (c < acc_width) {
				collect += '-';
				c++;
			}
			collect += '\n';
		}
	}
	return collect;
}

// Parser
// Parser Options
//CSVParser::CSVOptions::CSVOptions() {};

CSVParser::CSVOptions::CSVOptions(CSVParser::CSVOptions const & opts) : 
	delimiter{ opts.delimiter },
	quote{ opts.quote },
	newline{ opts.newline },
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

std::string_view CSVParser::CSVOptions::get_delimiter() { return this->delimiter; };
std::string_view CSVParser::CSVOptions::get_quote() { return this->quote; };
std::string_view CSVParser::CSVOptions::get_newline() { return this->newline; };
bool CSVParser::CSVOptions::get_parse_numbers() { return this->parse_numbers; };
std::string_view CSVParser::CSVOptions::get_float_delimiter() { return this->float_delimiter; };
std::string_view CSVParser::CSVOptions::get_float_ignore(){ return this->float_ignore; };
int CSVParser::CSVOptions::get_expected_delimiters() { return this->expected_delimiters; };
int CSVParser::CSVOptions::get_header_lines() { return this->header_lines; };

void CSVParser::CSVOptions::set_delimiter(std::string_view const & delimiter) { this->delimiter = delimiter;};
void CSVParser::CSVOptions::set_quote(std::string_view const& quote) { this->quote = quote; };
void CSVParser::CSVOptions::set_newline(std::string_view const& newline) { this->newline = newline; };
void CSVParser::CSVOptions::set_parse_numbers(bool const& parse_numbers) { this->parse_numbers = parse_numbers; };
void CSVParser::CSVOptions::set_float_delimiter(std::string_view const& float_delimiter) { this->float_delimiter = float_delimiter; };
void CSVParser::CSVOptions::set_float_ignore(std::string_view const& float_ignore) { this->float_ignore = float_ignore; };
void CSVParser::CSVOptions::set_expected_delimiters(int const& expected_delimiters) { this->expected_delimiters = expected_delimiters; };
void CSVParser::CSVOptions::set_header_lines(int const& header_lines) { this->header_lines = header_lines; };

CSVParser::CSVOptions CSVParser::get_options() {
	return this->options;
}

void CSVParser::set_options(CSVParser::CSVOptions const & options) {
	this->options.delimiter = options.delimiter;
	this->options.quote = options.quote;
	this->options.newline = options.newline;
	this->options.parse_numbers = options.parse_numbers;
	this->options.float_delimiter = options.float_delimiter;
	this->options.float_ignore = options.float_ignore;
	this->options.expected_delimiters = options.expected_delimiters;
	this->options.header_lines = options.header_lines;
}

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

void CSVEntry::update_data() {
	if (this->func) {
		this->deduce_index();
		CSV_function f = this->func.value();
		f(this);
	}
}

py::object CSVEntry::py_read() {
	// using CSV_datavar = std::variant<std::string_view, int, double>;
	this->update_data();
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
	this->update_data();
}

py::str CSVEntry::strtype() {
	switch (data.index()) {
	case 0:
		return py::str("str");
	case 1:
		return py::str("int");
	case 2:
		return py::str("float");
	}
	return py::none();
}
// parser

std::shared_ptr<CSVData> CSVParser::parse(std::string_view const& file) {
	if (options.quote.empty()) {
		std::cout << "No quotes. Parsing without.\n";
		return this->parse_noquotes(file);
	}

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
	std::string collect{};
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
				// This check adds tolerance for when a line ends with a quote,
				// but is actually properly terminated and continued on the next line.
				if (!(cur_char == newline && delimiters_togo > 1) || (cur_char == newline && get_headers)) {
					collect.pop_back();
					collect.erase(0, 1);
					collect_line.push_back(collect);
					py::print("collect:", collect);
					collect.clear();


					// when a word gets added, that's the moment we count a delimiter.
					if (count_commas) {
						expected_delimiters += 1;
					}
					else {
						delimiters_togo -= 1;
						if (delimiters_togo < 0 && !file_stream.eof()) {
							std::cout << delimiters_togo << " <- Delimiters to go\n";
							throw std::runtime_error("Delimiters amount need to be similar across lines");
						}
					}
				}
				if (cur_char != newline && !file_stream.eof())
					continue;
				else {
					std::cout << "new line!" << get_headers << " " << delimiters_togo << " " << expected_delimiters << "\n";
				}
			}
		}

		if (cur_char == newline || file_stream.eof()) {
			if (cur_header < headers && get_headers) {
				std::cout << "getting headers \n";
				// header phase
				if (cur_header == 0) {
					count_commas = false;
					this->options.expected_delimiters = expected_delimiters;
					//cur_header += 1;
				}
				if (cur_header == headers - 1) {
					std::cout << "stopping header gather \n";
					get_headers = false;
					
				}
				if (delimiters_togo == 0) {
					std::cout << "collecting headers" << cur_header << headers << "\n";
					bool append = (cur_header > 0);
					int c{ 0 };
					for (auto el : collect_line) {
						py::print(el,get_headers, append);
						if (!append) {
							csv_data->headers.push_back(el);
						}
						else {
							(&csv_data->headers.data()[c])->append('\x1f' + el);
							std::cout << csv_data->headers.data()[c] << "\n";
							
						}
						// when collection is done, compose actual headers.
						if (!get_headers) {
							auto el_new = csv_data->headers.data()[c];
							if (csv_data->data.contains(el)) {
								// this will eventually have to be solved by somehow concatenating multiple headers.
								// a double header with
								// "Polish","Blocking"
								// "Assigned","Assigned"
								// is still valid, just need to find some uncommon character to split them.
								throw std::runtime_error("Headers with same name encountered.");
							}
							auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
							std::cout << "inserting with key: " << el_new << "\n";
							csv_data->data.insert({ el_new, *vec });
						}
						c += 1;
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
						CSV_datavar proc_el{};
						if (options.parse_numbers) {
							proc_el = process_entry(el, float_ignore, float_delimiter);
						}
						else {
							proc_el = el;
						}
						auto entry = std::make_shared<CSVEntry>(
							proc_el
						);

						auto header_entry = csv_data->headers[c];
						py::print(header_entry,">", entry->py_read());
						c += 1;
						auto& vec = csv_data->data.at(header_entry);
						entry->origin = &vec;
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
	
	csv_data->size = size;
	
	std::cout << "done parsing? done parsing.\n";
	return csv_data;
}

std::shared_ptr<CSVData> CSVParser::parse_noquotes(std::string_view const& file) {
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
	std::string collect{};
	std::vector<std::string> collect_line{};

	char delimiter = options.delimiter.data()[0];
	char newline = options.newline.data()[0];

	char float_delimiter = options.float_delimiter.data()[0];
	char float_ignore = options.float_ignore.data()[0];

	int size = 0;
	std::cout << "reading file " << file << "\n";
	while (file_stream) {
		file_stream.get(cur_char);
		//std::cout << std::string{ cur_char } << " ";
		if (cur_char == delimiter || (cur_char == newline && count_commas) || (cur_char == newline && delimiters_togo == 1 && !count_commas) || file_stream.eof()) {
			//std::cout << "Delimiter\n";
			if (cur_char == newline && collect.ends_with(delimiter)) {
				collect.pop_back();
			}

			while (collect.starts_with(' ') || collect.starts_with('\n') || collect.starts_with('\t')) {
				collect.erase(0, 1);
			}
			while (collect.ends_with(' ') || collect.ends_with('\n') || collect.ends_with('\t')) {
				collect.pop_back();
			}


			collect_line.push_back(collect);
			py::print("collect:", collect, "collect line size",collect_line.size());
			collect.clear();

			// when a word gets added, that's the moment we count a delimiter.
			if (count_commas) {
				expected_delimiters += 1;
			}
			else {
				delimiters_togo -= 1;
				if (delimiters_togo < 0 && !file_stream.eof()) {
					std::cout << delimiters_togo << " <- Delimiters to go\n";
					throw std::runtime_error("Delimiters amount need to be similar across lines");
				}
			}
			
			if (cur_char != newline && !file_stream.eof()) {
				std::cout << "continuing\n";
				continue;
			}
			else {
				std::cout << "new line or EOF!" << get_headers << " " << delimiters_togo << "\n";
			}
			
		}

		if (cur_char == newline || file_stream.eof()) {
			if (cur_header < headers && get_headers) {
				std::cout << "getting headers \n";
				// header phase
				if (cur_header == 0) {
					count_commas = false;
					this->options.expected_delimiters = expected_delimiters;
					//cur_header += 1;
				}
				if (cur_header == headers - 1) {
					std::cout << "stopping header gather \n";
					get_headers = false;

				}
				if (delimiters_togo == 0) {
					std::cout << "collecting headers" << cur_header << headers << "\n";
					bool append = (cur_header > 0);
					int c{ 0 };
					for (auto el : collect_line) {
						py::print(el, get_headers, append);
						if (!append) {
							csv_data->headers.push_back(el);
						}
						else {
							(&csv_data->headers.data()[c])->append('\x1f' + el);
							std::cout << csv_data->headers.data()[c] << "\n";

						}
						// when collection is done, compose actual headers.
						if (!get_headers) {
							auto el_new = csv_data->headers.data()[c];
							if (csv_data->data.contains(el)) {
								// this will eventually have to be solved by somehow concatenating multiple headers.
								// a double header with
								// "Polish","Blocking"
								// "Assigned","Assigned"
								// is still valid, just need to find some uncommon character to split them.
								throw std::runtime_error("Headers with same name encountered.");
							}
							auto vec = new std::vector<std::shared_ptr<CSVEntry>>{};
							std::cout << "inserting with key: " << el_new << "\n";
							csv_data->data.insert({ el_new, *vec });
						}
						c += 1;
					}
					collect_line.clear();
					cur_header += 1;
					delimiters_togo = expected_delimiters;
					continue;
				}
			}
			else {
				// reset delimiters if appropriate
				std::cout << "attempting add | EOF:" << file_stream.eof() << "\n";
				if (delimiters_togo == 0 || file_stream.eof()) {
					std::cout << "adding line\n";
					delimiters_togo = expected_delimiters;
					int c = 0;

					for (auto& el : collect_line) {
						CSV_datavar proc_el{};
						if (options.parse_numbers) {
							proc_el = process_entry(el, float_ignore, float_delimiter);
						}
						else {
							proc_el = el;
						}
						auto entry = std::make_shared<CSVEntry>(
							proc_el
						);

						auto header_entry = csv_data->headers[c];
						py::print(header_entry, ">", entry->py_read());
						c += 1;
						auto& vec = csv_data->data.at(header_entry);
						entry->origin = &vec;
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
		if (collect.ends_with(delimiter) && cur_char == newline) {
			continue;
		}

		// passed all edge cases, proceed collecting ...
		collect += cur_char;
	}
	file_stream.close();

	csv_data->size = size;

	std::cout << "done parsing? done parsing.\n";
	return csv_data;
}

