#ifndef _CSV_PARSER
#define _CSV_PARSER

#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <memory>

#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using CSV_datavar = std::variant<std::string, int, double>;
using CSV_headervar = std::variant<std::string_view, std::vector<std::string_view>>;

struct CSVData;

struct CSVEntry {
	std::shared_ptr<CSVData> parent;
	CSV_datavar data{};
	int row{};
	int column{};
	//void _print_data();
	CSVEntry(CSV_datavar const& data);
	
	py::object py_read();
	void set_data(CSV_datavar const & data);
};

// Not sure about CSVColumn and CSVRow yet.
struct CSVColumn {
	int column{};
	CSV_headervar key;
	std::vector<CSVEntry> data;
	//void _print_keys();
};

struct CSVRow {
	int row{};
	std::vector<std::string_view> keys;
	std::unordered_map<std::string_view, CSVEntry> data;
};

struct CSVData {
	using data_t = std::vector <std::shared_ptr<CSVEntry>>;
	
	std::vector<std::string> headers{};
	int header_count{};
	std::unordered_map<std::string, data_t> data{};

	std::vector<std::string> read_headers();
	data_t read_column(std::string const & key);
};

struct CSVParser {
	struct CSVOptions {
		std::string_view const delimiter = ",";
		std::string_view const quote = "\"";
		std::string_view const newline = "\n";
		bool parse_numbers = true;
		std::string_view const float_delimiter = ".";
		std::string_view const float_ignore = "";
		int const expected_delimiters = -1;
		int const header_lines = 1;

		//CSVOptions();
		CSVOptions(CSVOptions const & opts);
		CSVOptions(
			std::string_view const & delimiter = ",",
			std::string_view const & quote = "\"",
			std::string_view const & newline = "\n",
			bool const & parse_numbers = true,
			std::string_view const & float_delimiter = ".",
			std::string_view const & float_ignore = "",
			int const & expected_delimiters = -1,
			int const & header_lines = 1
		);
	};

	CSVOptions options{};

	CSVParser(
		std::string_view const & delimiter= ",", 
		std::string_view const & quote = "\"", 
		std::string_view const& newline = "\n",
		bool const & parse_numbers = true, 
		std::string_view const & float_delimiter = ".",
		std::string_view const & float_ignore = "",
		int const & expected_delimiters = -1,
		int const & header_lines = 1
	);

	std::shared_ptr<CSVData> parse(std::string_view const& file_path);
};



#endif // !_CSV_PARSER
