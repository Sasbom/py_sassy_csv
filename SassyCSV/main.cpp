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
	py::class_<CSVParser::CSVOptions>(m, "CSVOptions")
		.def(py::init<>())
		.def(py::init<std::string_view&,
			std::string_view&,
			std::string_view&,
			bool&,
			std::string_view&,
			std::string_view&,
			int&,
			int&>(),
			"delimiter"_a = ",",
			"quote"_a = "\"",
			"newline"_a = "\n",
			"parse_numbers"_a = true,
			"float_delimiter"_a = ".",
			"float_ignore"_a = " ",
			"expected_delimiters"_a = -1,
			"header_lines"_a = 1)
		.def_property("delimiter", &CSVParser::CSVOptions::get_delimiter, &CSVParser::CSVOptions::set_delimiter)
		.def_property("quote", &CSVParser::CSVOptions::get_quote, &CSVParser::CSVOptions::set_quote)
		.def_property("newline", &CSVParser::CSVOptions::get_newline, &CSVParser::CSVOptions::set_newline)
		.def_property("parse_numbers", &CSVParser::CSVOptions::get_parse_numbers, &CSVParser::CSVOptions::set_parse_numbers)
		.def_property("float_delimiter", &CSVParser::CSVOptions::get_float_delimiter, &CSVParser::CSVOptions::set_float_delimiter)
		.def_property("float_ignore", &CSVParser::CSVOptions::get_float_ignore, &CSVParser::CSVOptions::set_float_ignore)
		.def_property("expected_delimiters", &CSVParser::CSVOptions::get_expected_delimiters, &CSVParser::CSVOptions::set_expected_delimiters)
		.def_property("header_lines", &CSVParser::CSVOptions::get_header_lines, &CSVParser::CSVOptions::set_header_lines);

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
		.def("add_id_column", &CSVData::add_ID_header, "Add an ID header")
		.def("add_ref_column", &CSVData::add_new_header, "Add a referenced header")
		.def("add_func_column", &CSVData::add_ref_func_header, "Add a computed header")
		.def("add_func_column", &CSVData::add_acc_ref_func_header, "Add a computed header")
		.def("add_func_column", &CSVData::add_ref_func_header_py, "Add a computed header")
		.def("add_func_column", &CSVData::add_acc_ref_func_header_py, "Add a computed header")
		.def("append_row",&CSVData::append_empty_row, "Add empty row")
		.def("prepend_row", &CSVData::prepend_empty_row, "Add empty row at start")
		.def_property_readonly("headers", &CSVData::read_headers_py)
		.def_property_readonly("size", &CSVData::get_size);
}