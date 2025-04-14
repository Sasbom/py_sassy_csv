#ifndef _CSV_PARSER
#define _CSV_PARSER

#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <memory>
#include <functional>
#include <optional>

#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using CSV_datavar = std::variant<std::string, int, double>;
using CSV_headervar = std::variant<std::string_view, std::vector<std::string_view>>;




struct CSVData;

struct CSVEntry : std::enable_shared_from_this<CSVEntry> {
	using CSV_function = std::function<void(CSVEntry*)>;
	using CSV_funcopt = std::optional<CSV_function>;
	using data_t = std::vector <std::shared_ptr<CSVEntry>>;

	CSV_datavar data{};
	data_t* origin{ nullptr };
	//void _print_data();
	CSVEntry(CSV_datavar const& data);
	
	CSV_funcopt func{};
	void update_data();

	int index{};
	void deduce_index();

	py::object py_read();
	py::str strtype();
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

struct CSVData : std::enable_shared_from_this<CSVData> {
	using data_t = std::vector <std::shared_ptr<CSVEntry>>;
	// wanting to accept a python function of `def function(other_value, index): ...`
	using CSVfunc_interface = std::function<CSV_datavar(CSV_datavar, int)>; 
	using CSVaccfunc_interface = std::function<CSV_datavar(std::vector<CSV_datavar>, int)>;
	std::vector<std::string> headers{};
	int header_count{};
	std::unordered_map<std::string, data_t> data{};

	int size{};
	int get_size();

	std::vector<std::string> read_headers();
	py::object read_headers_py();
	data_t read_column_str(std::string const & key);
	data_t read_column_py(py::tuple tup_key);
	py::dict read_row_py(int index);
	data_t read_row_elements(int index);

	void add_ID_header(std::string const& name, bool as_int);
	void add_ID_header_py(py::tuple const& name, bool as_int);
	void add_new_header(std::string const & name, std::string const& referenced_name);
	void append_empty_row();
	void prepend_empty_row();
	void add_ref_func_header(std::string const& name, std::string const& other_name, CSVfunc_interface const& function);
	void add_ref_func_header_py(py::tuple const& name, py::tuple const& other_name, CSVfunc_interface const& function);
	void add_acc_ref_func_header(std::string const& name, std::vector<std::string> const& other_names, CSVaccfunc_interface const& function);
	void add_acc_ref_func_header_py(py::tuple const& name, std::vector<py::tuple> const& other_names, CSVaccfunc_interface const& function);

	std::string format_pretty();
};

struct CSVParser {
	struct CSVOptions {
		std::string_view delimiter = ",";
		std::string_view quote = "\"";
		std::string_view newline = "\n";
		bool parse_numbers = true;
		std::string_view float_delimiter = ".";
		std::string_view float_ignore = "";
		int expected_delimiters = -1;
		int header_lines = 1;

		//CSVOptions();
		CSVOptions(CSVOptions const & opts);
		//CSVOptions(CSVOptions const&) = default;
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

		std::string_view get_delimiter();
		std::string_view get_quote();
		std::string_view get_newline();
		bool get_parse_numbers();
		std::string_view get_float_delimiter();
		std::string_view get_float_ignore();
		int get_expected_delimiters();
		int get_header_lines();
		void set_delimiter(std::string_view const& delimiter);
		void set_quote(std::string_view const& quote);
		void set_newline(std::string_view const& newline);
		void set_parse_numbers(bool const& parse_numbers);
		void set_float_delimiter(std::string_view const& float_delimiter);
		void set_float_ignore(std::string_view const& float_ignore);
		void set_expected_delimiters(int const& expected_delimiters);
		void set_header_lines(int const& header_lines);

	};

	CSVOptions options{};

	void set_options(CSVOptions const & options);
	CSVOptions get_options();

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
	std::shared_ptr<CSVData> parse_noquotes(std::string_view const& file_path);
};



#endif // !_CSV_PARSER
