#include <iostream>
#include <memory>
#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include "CSVParser.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(_SassyCSV, m) {
	m.doc() = "Extremely lenient CSV parsing with some funny business on the side.";
	py::class_<CSVEntry, std::shared_ptr<CSVEntry>>(m, "CSVEntry")
		.def(py::init<int&>())
		.def(py::init<double&>())
		.def(py::init<std::string &>())
		.def("set_data",&CSVEntry::set_data, "Change this entry's stored value.")
		.def("read",&CSVEntry::py_read,"Read the value.")
		.def("type",&CSVEntry::strtype,"Return the stored type as a string.")
		.def("__repr__", [](CSVEntry& e) {return py::str("<CSVEntry holding: ") + py::str(e.py_read()) + py::str(" of type ") + e.strtype() + py::str(" >"); }, "")
		.def("__str__", [](CSVEntry & e) {return py::str(e.py_read()); }, "");
	py::enum_<NumberFormatting>(m, "NumberFormatting")
		.value("INTERNATIONAL",NumberFormatting::INTERNATIONAL, "International number seperation formatting")
		.value("INDIAN",NumberFormatting::INDIAN,"Indian Lakh number seperation formatting");
	py::class_<CSVOptions>(m, "CSVOptions")
		.def(py::init<>())
		.def(py::init<std::string_view&,
			std::string_view&,
			std::string_view&,
			bool&,
			std::string_view&,
			std::string_view&,
			int&,
			int&,
			NumberFormatting&,
			int&,
			bool&,
			std::string_view&,
			bool&,
		    std::string_view&>(),
			"delimiter"_a = ",",
			"quote"_a = "\"",
			"newline"_a = "\n",
			"parse_numbers"_a = true,
			"float_delimiter"_a = ".",
			"float_ignore"_a = " ",
			"expected_delimiters"_a = -1,
			"header_lines"_a = 1,
			"number_formatting"_a = NumberFormatting::INTERNATIONAL,
			"float_round_decimals"_a = 2,
			"consolidate_headers"_a = false,
			"consolidation_sep_str"_a = " > ",
			"replace_newline"_a = true,
			"newline_replacement"_a = "")
		.def_property("delimiter", &CSVOptions::get_delimiter, &CSVOptions::set_delimiter)
		.def_property("quote", &CSVOptions::get_quote, &CSVOptions::set_quote)
		.def_property("newline", &CSVOptions::get_newline, &CSVOptions::set_newline)
		.def_property("parse_numbers", &CSVOptions::get_parse_numbers, &CSVOptions::set_parse_numbers)
		.def_property("float_delimiter", &CSVOptions::get_float_delimiter, &CSVOptions::set_float_delimiter)
		.def_property("float_ignore", &CSVOptions::get_float_ignore, &CSVOptions::set_float_ignore)
		.def_property("expected_delimiters", &CSVOptions::get_expected_delimiters, &CSVOptions::set_expected_delimiters)
		.def_property("header_lines", &CSVOptions::get_header_lines, &CSVOptions::set_header_lines)
		.def_property("number_formatting",&CSVOptions::get_number_formatting, &CSVOptions::set_number_formatting)
		.def_property("float_decimals",&CSVOptions::get_float_round_decimals, &CSVOptions::set_float_round_decimals)
		.def_property("collapse_headers", &CSVOptions::get_consolidate_headers, &CSVOptions::set_consolidate_headers)
		.def_property("collapse_header_join_str",&CSVOptions::get_consolidation_sep_str,&CSVOptions::set_consolidation_sep_str)
		.def_property("replace_newline",&CSVOptions::get_replace_newline, &CSVOptions::set_replace_newline)
		.def_property("newline_replacement", &CSVOptions::get_newline_replacement,&CSVOptions::set_newline_replacement);

	py::class_<CSVParser>(m, "CSVParser")
		.def(py::init<>())
		.def("parse", &CSVParser::parse, "Parse a csv file.")
		.def_property("options", py::cpp_function(&CSVParser::get_options,py::return_value_policy::reference), 
			py::cpp_function(&CSVParser::set_options));
	py::class_<CSVData, std::shared_ptr<CSVData>>(m, "CSVData")
		.def(py::init<>())
		//.def("read_headers", &CSVData::read_headers, "Get a list of headers");
		.def("read_column", &CSVData::read_column_py, "Read a column")
		.def("read_column", &CSVData::read_column_str, "Read a column")
		.def("read_row", &CSVData::read_row_py, "Read a row from an index")
		.def("__getitem__",&CSVData::read_rows_py, "Slice data into dict")
		.def("add_id_column", &CSVData::add_ID_header, "Add an ID header")
		.def("add_id_column", &CSVData::add_ID_header_py, "Add an ID header")
		//.def("add_ref_column", &CSVData::add_new_header, "Add a referenced header")
		.def("add_func_column", &CSVData::add_ref_func_header, "Add a computed header")
		.def("add_func_column", &CSVData::add_acc_ref_func_header, "Add a computed header")
		.def("add_func_column", &CSVData::add_ref_func_header_py, "Add a computed header")
		.def("add_func_column", &CSVData::add_acc_ref_func_header_py, "Add a computed header")
		.def("append_row", &CSVData::append_empty_row, "Add empty row")
		.def("prepend_row", &CSVData::prepend_empty_row, "Add empty row at start")
		.def_property_readonly("headers", &CSVData::read_headers_py)
		.def_property_readonly("size", &CSVData::get_size)
		.def_property_readonly("formatted", &CSVData::format_pretty)
		.def("view",&CSVData::generate_view,"Provide a view into the world of CSV data.");
	py::class_<CSVDataView, std::shared_ptr<CSVDataView>>(m, "CSVDataView")
		.def(py::init<>())
		.def("filter_func", &CSVDataView::add_predicate,"Add a function to filter stuff with")
		.def("no_headers",&CSVDataView::disable_all_headers, "Remove all headers from view")
		.def("all_headers", &CSVDataView::view_all_headers, "Reset view to show all headers")
		.def("select_headers",&CSVDataView::select_headers, "Select headers to show")
		.def("hide_headers", &CSVDataView::remove_headers, "Select headers to hide")
		.def("reset", &CSVDataView::reset_view, "Reset internal state.")
		.def_property_readonly("formatted", &CSVDataView::format_pretty_view);
}