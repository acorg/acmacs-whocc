#include "acmacs-base/pybind11.hh"

// chart
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// merge
#include "acmacs-chart-2/merge.hh"

// ======================================================================

inline unsigned make_info_data(bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates)
{
    using namespace acmacs::chart;
    return (column_bases ? info_data::column_bases : 0)         //
           | (tables ? info_data::tables : 0)                   //
           | (tables_for_sera ? info_data::tables_for_sera : 0) //
           | (antigen_dates ? info_data::dates : 0);
}

// ----------------------------------------------------------------------

inline acmacs::chart::ChartClone::clone_data clone_type(const std::string& type)
{
    using namespace acmacs::chart;
    if (type == "titers")
        return ChartClone::clone_data::titers;
    else if (type == "projections")
        return ChartClone::clone_data::projections;
    else if (type == "plot_spec")
        return ChartClone::clone_data::plot_spec;
    else if (type == "projections_plot_spec")
        return ChartClone::clone_data::projections_plot_spec;
    else
        throw std::invalid_argument{fmt::format("Unrecognized clone \"type\": \"{}\"", type)};
}

// ----------------------------------------------------------------------

template <typename AgSr> struct AgSrIndexes
{
    AgSrIndexes() = default;
    AgSrIndexes(std::shared_ptr<AgSr> a_ag_sr) : ag_sr{a_ag_sr}, indexes{a_ag_sr->all_indexes()} {}
    bool empty() const { return indexes.empty(); }

    std::shared_ptr<AgSr> ag_sr;
    acmacs::chart::Indexes indexes;
};

struct AntigenIndexes : public AgSrIndexes<acmacs::chart::Antigens> { using AgSrIndexes<acmacs::chart::Antigens>::AgSrIndexes; };
struct SerumIndexes : public AgSrIndexes<acmacs::chart::Sera> { using AgSrIndexes<acmacs::chart::Sera>::AgSrIndexes; };

// ----------------------------------------------------------------------

inline void py_chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart") //
        .def(py::init([](const std::string& filename) { return std::make_shared<ChartModify>(import_from_file(filename)); }), py::doc("imports chart from a file"))

        .def(
            "clone",                                                                                                                                           //
            [](ChartModify& chart, const std::string& type) -> std::shared_ptr<ChartModify> { return std::make_shared<ChartClone>(chart, clone_type(type)); }, //
            "type"_a = "titers",                                                                                                                               //
            py::doc(R"(type: "titers", "projections", "plot_spec", "projections_plot_spec")"))                                                                 //

        .def(
            "make_name",                                                            //
            [](const ChartModify& chart) { return chart.make_name(std::nullopt); }, //
            py::doc("returns name of the chart"))                                   //

        .def(
            "make_name",                                                                                   //
            [](const ChartModify& chart, size_t projection_no) { return chart.make_name(projection_no); }, //
            "projection_no"_a,                                                                             //
            py::doc("returns name of the chart with the stress of the passed projection"))                 //

        .def("subtype", [](const ChartModify& chart) { return *chart.info()->virus_type(); })                            //
        .def("subtype_short", [](const ChartModify& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        .def("subset", [](const ChartModify& chart) { return chart.info()->subset(); })                                  //
        .def("assay", [](const ChartModify& chart) { return *chart.info()->assay(); })                                   //
        .def("assay_hi_or_neut", [](const ChartModify& chart) { return chart.info()->assay().hi_or_neut(); })            //
        .def("lab", [](const ChartModify& chart) { return *chart.info()->lab(); })                                       //
        .def("rbc", [](const ChartModify& chart) { return *chart.info()->rbc_species(); })                               //
        .def("date", [](const ChartModify& chart) { return *chart.info()->date(Info::Compute::Yes); })                   //
        .def(
            "lineage", [](const ChartModify& chart) { return *chart.lineage(); }, py::doc("returns chart lineage: VICTORIA, YAMAGATA")) //

        .def("description",                                 //
             &Chart::description,                           //
             py::doc("returns chart one line description")) //

        .def(
            "make_info", //
            [](const ChartModify& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates) {
                return chart.make_info(max_number_of_projections_to_show, make_info_data(column_bases, tables, tables_for_sera, antigen_dates));
            },                                                                                                                                               //
            "max_number_of_projections_to_show"_a = 20, "column_bases"_a = true, "tables"_a = false, "tables_for_sera"_a = false, "antigen_dates"_a = false, //
            py::doc("returns detailed chart description"))                                                                                                   //

        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)

        .def(
            "relax", //
            [](ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, const std::string& minimum_column_basis, bool dimension_annealing, bool rough,
               size_t /*number_of_best_distinct_projections_to_keep*/) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax(number_of_optimizations_t{number_of_optimizations}, MinimumColumnBasis{minimum_column_basis}, acmacs::number_of_dimensions_t{number_of_dimensions},
                            use_dimension_annealing_from_bool(dimension_annealing), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
                chart.projections_modify().sort();
            }, //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false,
            "number_of_best_distinct_projections_to_keep"_a = 5,                                                                              //
            py::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"}) //

        .def(
            "relax_incremental", //
            [](ChartModify& chart, size_t number_of_optimizations, bool rough, size_t number_of_best_distinct_projections_to_keep, bool remove_source_projection, bool unmovable_non_nan_points) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax_incremental(0, number_of_optimizations_t{number_of_optimizations},
                                        optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                                        acmacs::chart::remove_source_projection{remove_source_projection ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no},
                                        acmacs::chart::unmovable_non_nan_points{unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no});
                chart.projections_modify().sort();
            },                                                                                                                                                                                  //
            "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true, "unmovable_non_nan_points"_a = false) //

        .def(
            "projection",                                                                                    //
            [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
            "projection_no"_a = 0)                                                                           //

        .def("remove_all_projections",                                                   //
             [](ChartModify& chart) { return chart.projections_modify().remove_all(); }) //

        .def(
            "keep_projections",                                                                               //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().keep_just(to_keep); }, //
            "keep"_a)                                                                                         //

        .def(
            "export",                                                                                                                                               //
            [](ChartModify& chart, const std::string& filename, const std::string& program_name) { acmacs::chart::export_factory(chart, filename, program_name); }, //
            "filename"_a, "program_name"_a)                                                                                                                         //

        .def("antigen_indexes",                                                                     //
             [](ChartModify& chart) { return std::make_shared<AntigenIndexes>(chart.antigens()); }) //
        .def("serum_indexes",                                                                       //
             [](ChartModify& chart) { return std::make_shared<SerumIndexes>(chart.sera()); })       //

        .def(
            "remove_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<AntigenIndexes> antigens, std::shared_ptr<SerumIndexes> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty())
                    chart.remove_antigens(acmacs::ReverseSortedIndexes{*antigens->indexes});
                if (sera && !sera->empty())
                    chart.remove_sera(acmacs::ReverseSortedIndexes{*sera->indexes});
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            py::doc(R"(Usage:
    chart.remove_antigens_sera(antigens=c.antigen_indexes().filter_lineage(\"yamagata\"), sera=c.serum_indexes().filter_lineage(\"yamagata\"))
    chart.remove_antigens_sera(sera=chart.serum_indexes().filter_serum_id("A8658-14D"))
)"))                                                                                    //

        .def(
            "modify_titers",
            [](ChartModify& chart, const std::string& look_for, const std::string& replacement, bool verbose) {
                const auto replacements = chart.titers_modify().replace_all(std::regex{look_for}, replacement);
                if (verbose) {
                    if (!replacements.empty()) {
                        AD_INFO("{} titer replacements done", replacements.size());
                        for (const auto& rep : replacements)
                            fmt::print(stderr, "    ag:{:04d} sr:{:03d} titer:{}", rep.antigen, rep.serum, rep.titer);
                    }
                    else
                        AD_WARNING("No titer replacement performed: no titer match for \"{}\"", look_for);
                }
            },                                                                          //
            "look_for"_a, "replacement"_a, "verbose"_a = false, //
            py::doc(R"(look_for is regular expression,
replacement is replacement with substitutions:
    $1 - match of the first subexpression
    $2 - match of the second subexpression
    ...
    $` - prefix before match
    $' - suffix after match
Usage:
    chart.modify_titers(look_for=">", replacement="$`$'", verbose=True)
)"))                                                                                    //
        ;

    // ----------------------------------------------------------------------

    py::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(mdl, "Projection") //
        .def(
            "stress",                                                                                                                                                      //
            [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, //
            "recalculate"_a = false)                                                                                                                                       //
        ;

    py::class_<AntigenIndexes, std::shared_ptr<AntigenIndexes>>(mdl, "AntigenIndexes") //
        .def("__str__", [](const AntigenIndexes& indexes) { return fmt::format("AntigenIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &AntigenIndexes::empty)

        .def(
            "filter_lineage",
            [](AntigenIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        ;

    py::class_<SerumIndexes, std::shared_ptr<SerumIndexes>>(mdl, "SerumIndexes") //
        .def("__str__", [](const SerumIndexes& indexes) { return fmt::format("SerumIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &SerumIndexes::empty)

        .def(
            "filter_lineage",
            [](SerumIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        .def(
            "filter_serum_id",
            [](SerumIndexes& indexes, const std::string& serum_id) {
                indexes.ag_sr->filter_serum_id(indexes.indexes, serum_id);
                return indexes;
            },            //
            "serum_id"_a) //
        ;
}

// ======================================================================

inline void py_merge(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    mdl.def(
        "merge",
        [](std::shared_ptr<ChartModify> chart1, std::shared_ptr<ChartModify> chart2, const std::string& merge_type, const std::string& match, bool a_combine_cheating_assays, bool a_remove_distinct) {
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

            return merge(*chart1, *chart2,
                         MergeSettings{match_level, merge_typ, a_combine_cheating_assays ? combine_cheating_assays::yes : combine_cheating_assays::no,
                                       a_remove_distinct ? remove_distinct::yes : remove_distinct::no});
        },                                                                                                                      //
        "chart1"_a, "chart2"_a, "type"_a, "match"_a = "auto", "combine_cheating_assays"_a = false, "remove_distinct"_a = false, //
        py::doc(R"(merges two charts
type: "type1" ("tables-only"), "type2" ("incremental"), "type3", "type4", "type5"
      see https://github.com/acorg/acmacs-chart-2/blob/master/doc/merge-types.org
match: "strict", "relaxed", "ignored", "automatic" ("auto")
)"));

    py::class_<MergeReport>(mdl, "MergeReport");

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
