#include "acmacs-base/pybind11.hh"

// chart
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

inline void py_chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart")
        .def(py::init([](const std::string& filename) { return std::make_shared<ChartModify>(import_from_file(filename)); }))
        .def("make_name", [](const ChartModify& chart) { return chart.info()->make_name(); })
        .def("make_name", &Chart::make_name, "projection_no"_a = py::none())
        ;
}

// ----------------------------------------------------------------------

// https://pybind11.readthedocs.io/en/latest/faq.html#how-can-i-reduce-the-build-time

PYBIND11_MODULE(acmacs, mdl)
{
    mdl.doc() = "Acmacs backend";
    py_chart(mdl);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
