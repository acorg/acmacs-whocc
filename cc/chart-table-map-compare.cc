// Makes distance matrix based on procrustes RMS between maps

#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/bounding-ball.hh"
#include "acmacs-draw/surface-cairo.hh"
#include "acmacs-draw/drawi-generator.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "compare titers of mutiple tables"; }
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};

    option<str> output{*this, 'o', desc{"output .drawi file"}};
    option<size_t> number_of_optimizations{*this, 'n', dflt{100ul}};
    option<str> minimum_column_basis{*this, 'm', dflt{"none"}};
    option<size_t> number_of_dimensions{*this, 'd', dflt{2ul}};
    option<bool>   rough{*this, "rough"};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto precision = (opt.rough) ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;

        std::vector<acmacs::chart::ChartModify> charts;
        for (const auto& filename : opt.charts)
            charts.emplace_back(acmacs::chart::import_from_file(filename));

        const auto chart_name = [](const auto& chart) { return chart.info()->date(); };

        for (auto& chart : charts) {
            chart.projections_modify()->remove_all();
            chart.relax(acmacs::chart::number_of_optimizations_t{*opt.number_of_optimizations}, acmacs::chart::MinimumColumnBasis{opt.minimum_column_basis},
                        acmacs::number_of_dimensions_t{*opt.number_of_dimensions}, acmacs::chart::use_dimension_annealing::no, acmacs::chart::optimization_options{precision});
            chart.projections_modify()->sort();
            AD_DEBUG("{}  {:9.4f}", chart_name(chart), chart.projections()->best()->stress());
        }

        acmacs::chart::TableDistances table_distances;
        for (size_t t1 = 0; t1 < (charts.size() - 1); ++t1) {
            for (size_t t2 = t1 + 1; t2 < charts.size(); ++t2) {
                acmacs::chart::CommonAntigensSera common(charts[t1], charts[t2], acmacs::chart::CommonAntigensSera::match_level_t::strict);
                if (!common.points().empty()) {
                    const auto procrustes_data = acmacs::chart::procrustes(*charts[t1].projection(0), *charts[t2].projection(0), common.points(), acmacs::chart::procrustes_scaling_t::no);
                    table_distances.add_value(acmacs::chart::Titer::Regular, t1, t2, procrustes_data.rms);
                    AD_DEBUG("{} {}  {:6.4f}", chart_name(charts[t1]), chart_name(charts[t2]), procrustes_data.rms);
                }
            }
        }

        // ----------------------------------------------------------------------
        // make-module, also for chart-table-compare

        acmacs::chart::Stress stress(acmacs::number_of_dimensions_t{2}, charts.size());
        stress.table_distances() = table_distances;

        double best_stress{9e99};
        acmacs::Layout best_layout(charts.size(), stress.number_of_dimensions());
        for ([[maybe_unused]] auto attempt : acmacs::range(*opt.number_of_optimizations)) {
            acmacs::Layout layout(charts.size(), stress.number_of_dimensions());
            acmacs::chart::LayoutRandomizerPlain rnd(10.0, std::nullopt);
            for (auto point_no : acmacs::range(layout.number_of_points()))
                layout.update(point_no, rnd.get(stress.number_of_dimensions()));

            const auto status1 =
                acmacs::chart::optimize(acmacs::chart::optimization_method::alglib_cg_pca, stress, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
            // fmt::print("{:3d} stress: {} <- {} iterations: {}\n", attempt, status1.final_stress, status1.initial_stress, status1.number_of_iterations);
            if (status1.final_stress < best_stress) {
                best_stress = status1.final_stress;
                best_layout = layout;
            }
        }
        AD_DEBUG("best_stress: {}", best_stress);

        if (opt.output) {
            acmacs::drawi::Generator gen;
            const acmacs::BoundingBall bb{minimum_bounding_ball(best_layout)};
            gen.viewport().set_from_center_size(bb.center(), bb.hv_diameter_whole_number());
            for (size_t t1 = 0; t1 < charts.size(); ++t1) {
                gen.add<acmacs::drawi::Generator::Point>().coord(best_layout[t1]).label(chart_name(charts[t1]));
            }
            gen.add<acmacs::drawi::Generator::PointModify>().shape(acmacs::drawi::Generator::Point::Triangle).fill(GREEN).size(Pixels{10}).label_size(Pixels{7}).label_offset({0.0, 1.2});
            gen.generate(opt.output);
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
