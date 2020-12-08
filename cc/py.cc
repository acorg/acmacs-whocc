#include "acmacs-base/pybind11.hh"

// chart
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// merge
#include "acmacs-chart-2/merge.hh"

// ======================================================================

inline void chart_relax(acmacs::chart::ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, const std::string& minimum_column_basis, bool dimension_annealing, bool rough,
                        size_t number_of_best_distinct_projections_to_keep)
{
    using namespace acmacs::chart;
    if (number_of_optimizations == 0)
        number_of_optimizations = 100;
    chart.relax(number_of_optimizations_t{number_of_optimizations}, MinimumColumnBasis{minimum_column_basis}, acmacs::number_of_dimensions_t{number_of_dimensions},
                use_dimension_annealing_from_bool(dimension_annealing), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
    chart.projections_modify().sort();
}

inline std::string chart_info(const acmacs::chart::ChartModify& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates)
{
    using namespace acmacs::chart;
    const unsigned inf{(column_bases ? info_data::column_bases : 0)         //
                       | (tables ? info_data::tables : 0)                   //
                       | (tables_for_sera ? info_data::tables_for_sera : 0) //
                       | (antigen_dates ? info_data::dates : 0)};
    return chart.make_info(max_number_of_projections_to_show, inf);
}

// ----------------------------------------------------------------------

inline void py_chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart") //
        .def(py::init([](const std::string& filename) { return std::make_shared<ChartModify>(import_from_file(filename)); }), py::doc("imports chart from a file"))
        .def(
            "make_name", [](const ChartModify& chart) { return chart.make_name(std::nullopt); }, py::doc("returns name of the chart"))
        .def(
            "make_name", [](const ChartModify& chart, size_t projection_no) { return chart.make_name(projection_no); }, "projection_no"_a,
            py::doc("returns name of the chart with the stress of the passed projection"))
        .def("description", &Chart::description, py::doc("returns chart one line description"))
        .def("make_info", &chart_info, "max_number_of_projections_to_show"_a = 20, "column_bases"_a = true, "tables"_a = false, "tables_for_sera"_a = false, "antigen_dates"_a = false,
             py::doc("returns detailed chart description"))
        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)
        .def(
            "lineage", [](const ChartModify& chart) { return *chart.lineage(); }, py::doc("returns chart lineage: VICTORIA, YAMAGATA"))
        .def("relax", &chart_relax, "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false,
             "number_of_best_distinct_projections_to_keep"_a = 5, py::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"})
        .def(
            "projection", [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, "projection_no"_a = 0) //
        ;

    py::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(mdl, "Projection") //
        .def("stress", [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, "recalculate"_a = false) //
        ;
}

// ======================================================================

inline void py_merge(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    mdl.def(
        "merge",
        [](std::shared_ptr<ChartModify> chart1, std::shared_ptr<ChartModify> chart2, const std::string& merge_type, const std::string& match, bool remove_distinct) {
            CommonAntigensSera::match_level_t match_level{CommonAntigensSera::match_level_t::automatic};
            if (match == "auto" || match == "automatic")
                match_level = CommonAntigensSera::match_level_t::automatic;
            else if (match == "strict")
                match_level = CommonAntigensSera::match_level_t::strict;
            else if (match == "relaxed")
                match_level = CommonAntigensSera::match_level_t::relaxed;
            else if (match == "ignored")
                match_level = CommonAntigensSera::match_level_t::ignored;
            else
                throw std::invalid_argument{fmt::format("Unrecognized \"match\": \"{}\"", match)};

            projection_merge_t merge_typ{projection_merge_t::type1};
            if (merge_type == "type1" || merge_type == "tables-only")
                merge_typ = projection_merge_t::type1;
            else if (merge_type == "type2" || merge_type == "incremental")
                merge_typ = projection_merge_t::type2;
            else if (merge_type == "type3")
                merge_typ = projection_merge_t::type3;
            else if (merge_type == "type4")
                merge_typ = projection_merge_t::type4;
            else if (merge_type == "type5")
                merge_typ = projection_merge_t::type5;
            else
                throw std::invalid_argument{fmt::format("Unrecognized \"merge_type\": \"{}\"", merge_type)};

            return merge(*chart1, *chart2, MergeSettings{match_level, merge_typ, remove_distinct});
        },                                                                                 //
        "chart1"_a, "chart2"_a, "type"_a, "match"_a = "auto", "remove_distinct"_a = false, //
        py::doc(R"(merges two charts
type: "type1" ("tables-only"), "type2" ("incremental"), "type3", "type4", "type5"
      see https://github.com/acorg/acmacs-chart-2/blob/master/doc/merge-types.org
match: "strict", "relaxed", "ignored", "automatic" ("auto")
)"));

    py::class_<MergeReport>(mdl, "MergeReport")
        ;

} // py_merge

// ----------------------------------------------------------------------

// https://pybind11.readthedocs.io/en/latest/faq.html#how-can-i-reduce-the-build-time

PYBIND11_MODULE(acmacs, mdl)
{
    mdl.doc() = "Acmacs backend";
    py_chart(mdl);
    py_merge(mdl);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
