#include <iostream>
#include <memory>
#define PYBIND11_CPP20
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
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
		.def("__repr__", [](CSVEntry& e) {return py::str("<CSVEntry holding: ") + py::str(e.py_read()) + py::str(" >"); }, "")
		.def("__str__", [](CSVEntry & e) {return py::str(e.py_read()); }, "");
	py::class_<CSVParser::CSVOptions>(m, "CSVOptions")
		.def(py::init<>());
	py::class_<CSVParser>(m, "CSVParser")
		.def(py::init<>())
		.def("parse", &CSVParser::parse, "Parse a csv file.")
		.def("_options", [](CSVParser const& a) {return a.options; });
	py::class_<CSVData, std::shared_ptr<CSVData>>(m, "CSVData")
		.def(py::init<>())
		//.def("read_headers", &CSVData::read_headers, "Get a list of headers");
		.def("read_column",&CSVData::read_column,"Read a column")
		.def_property_readonly("headers",&CSVData::read_headers);
}