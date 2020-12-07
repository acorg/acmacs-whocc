#include "acmacs-base/pybind11.hh"

// chart
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ======================================================================

inline void chart_relax(acmacs::chart::ChartModify& chart, size_t number_of_optimizations, const std::string& minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, bool rough)
{
}

// ----------------------------------------------------------------------

inline void py_chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart")
        .def(py::init([](const std::string& filename) { return std::make_shared<ChartModify>(import_from_file(filename)); }), py::doc("imports chart from a file"))
        .def(
            "make_name", [](const ChartModify& chart) { return chart.make_name(std::nullopt); }, //
            py::doc("returns name of the chart"))
        .def(
            "make_name", [](const ChartModify& chart, size_t projection_no) { return chart.make_name(projection_no); }, "projection_no"_a, //
            py::doc("returns name of the chart with the stress of the passed projection"))
        .def("description", &Chart::description, py::doc("returns chart one line description"))
        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)
        .def("lineage", [](const ChartModify& chart) { return *chart.lineage(); }, py::doc("returns chart lineage: VICTORIA, YAMAGATA"))
        .def("relax", &chart_relax, "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 1, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false)
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
