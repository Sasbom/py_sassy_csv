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
struct CSVDataView;
struct CSVWriter;
struct CSVOptions;

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

// Result of parse, raw data along with extra references and modifications.
// The flow would be parse -> add fields -> view -> add predicates -> export
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
	py::dict read_rows_py(py::slice const & slice);
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

	std::shared_ptr<CSVDataView> generate_view();

	CSVWriter writer_with_options(CSVOptions const& options);
	CSVWriter writer();
};

// A view


struct CSVDataView: std::enable_shared_from_this<CSVDataView> {
	using viewfunc_t = std::function<void(CSVDataView*)>;
	// def predicate(header, index) -> bool
	using predicate_func = std::function<bool(CSV_datavar ,std::string, std::size_t)>;

	std::shared_ptr<CSVData> data;

	std::unordered_set<std::string> exclude_headers{};
	std::unordered_set<std::size_t> exclude_indices{};

	std::vector<viewfunc_t> predicates{};

	void reset_view();
	void view_all_indices();
	void view_all_headers();
	void disable_all_headers();
	void evaluate_predicates();

	std::shared_ptr<CSVDataView> add_predicate(predicate_func const & func);
	std::shared_ptr<CSVDataView> select_headers(py::args args);
	std::shared_ptr<CSVDataView> remove_headers(py::args args);

	std::string format_pretty_view();

	CSVWriter writer_with_options(CSVOptions const& options);
	CSVWriter writer();

};

enum class NumberFormatting {
	NONE,
	INTERNATIONAL,
	INDIAN
};

struct CSVOptions {
	// parsing / global options
	std::string_view delimiter = ",";
	std::string_view quote = "\"";
	std::string_view newline = "\n";
	bool parse_numbers = true;
	std::string_view float_delimiter = ".";
	std::string_view float_ignore = "";
	int expected_delimiters = -1;
	int header_lines = 1;

	// writing specific options
	NumberFormatting number_formatting = NumberFormatting::INTERNATIONAL;
	int float_round_decimals = 2;
	bool consolidate_headers = true;
	std::string_view consolidation_sep_str = " > ";
	bool replace_newline = true;
	std::string_view newline_replacement = "";

	//CSVOptions();
	CSVOptions(CSVOptions const& opts);
	//CSVOptions(CSVOptions const&) = default;
	//CSVOptions(
	//	std::string_view const& delimiter = ",",
	//	std::string_view const& quote = "\"",
	//	std::string_view const& newline = "\n",
	//	bool const& parse_numbers = true,
	//	std::string_view const& float_delimiter = ".",
	//	std::string_view const& float_ignore = "",
	//	int const& expected_delimiters = -1,
	//	int const& header_lines = 1
	//);

	CSVOptions(
		std::string_view const& delimiter = ",",
		std::string_view const& quote = "\"",
		std::string_view const& newline = "\n",
		bool const& parse_numbers = true,
		std::string_view const& float_delimiter = ".",
		std::string_view const& float_ignore = "",
		int const& expected_delimiters = -1,
		int const& header_lines = 1,
		NumberFormatting const & number_formatting = NumberFormatting::INTERNATIONAL,
		int const & float_round_decimals = 2,
		bool const & consolidate_headers = false,
		std::string_view const & consolidation_sep_str = " > ",
		bool const & replace_newline = true,
		std::string_view const & newline_replacement = ""
	);

	std::string_view get_delimiter();
	std::string_view get_quote();
	std::string_view get_newline();
	bool get_parse_numbers();
	std::string_view get_float_delimiter();
	std::string_view get_float_ignore();
	int get_expected_delimiters();
	int get_header_lines();
	NumberFormatting get_number_formatting();
	int get_float_round_decimals();
	bool get_consolidate_headers();
	std::string_view get_consolidation_sep_str();
	bool get_replace_newline();
	std::string_view get_newline_replacement();
	void set_delimiter(std::string_view const& delimiter);
	void set_quote(std::string_view const& quote);
	void set_newline(std::string_view const& newline);
	void set_parse_numbers(bool const& parse_numbers);
	void set_float_delimiter(std::string_view const& float_delimiter);
	void set_float_ignore(std::string_view const& float_ignore);
	void set_expected_delimiters(int const& expected_delimiters);
	void set_header_lines(int const& header_lines);
	void set_number_formatting(NumberFormatting const & number_formatting);
	void set_float_round_decimals(int const & float_round_decimals);
	void set_consolidate_headers(bool const & consolidate_headers);
	void set_consolidation_sep_str(std::string_view const & consolidation_sep_str);
	void set_replace_newline(bool const & replace_newline);
	void set_newline_replacement(std::string_view const & newline_replacement);
};

struct CSVParser {

	CSVOptions options{};

	void set_options(CSVOptions const & options);
	CSVOptions& get_options();

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

struct CSVWriter {
	using source_t = std::variant<std::shared_ptr<CSVData>, std::shared_ptr<CSVDataView>>;
	
	CSVOptions options{};

	source_t source;

	void set_options(CSVOptions const& options);
	CSVOptions& get_options();

	std::string write_s();

};


#endif // !_CSV_PARSER
