// #include <compare>
// #include <concepts>

#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/bounding-ball.hh"
#include "acmacs-draw/surface-cairo.hh"
#include "acmacs-draw/drawi-generator.hh"

// ----------------------------------------------------------------------

// template <typename T> requires requires (const T& a, const T& b)
// {
//     a.compare(b);
//     // {
//     //     a.compare(b)
//     //         } -> concepts::convertible_to<int>;
// }
// inline std::strong_ordering operator<=>(const T& lhs, const T& rhs)
// {
//     if (const auto res = lhs.compare(rhs); res == 0)
//         return std::strong_ordering::equal;
//     else if (res < 0)
//         return std::strong_ordering::less;
//     else
//         return std::strong_ordering::greater;
// }


struct TiterRef
{
    std::string serum;
    std::string antigen;
    size_t table_no;
    acmacs::chart::Titer titer;

    bool operator<(const TiterRef& rhs) const
    {
        if (const auto r1 = serum.compare(rhs.serum); r1 != 0)
            return r1 < 0;
        if (const auto r1 = antigen.compare(rhs.antigen); r1 != 0)
            return r1 < 0;
        return table_no < rhs.table_no;
    }

    // std::strong_ordering operator<=>(const TiterRef&) const = default;
};

struct TiterRefCollapsed
{
    std::string serum;
    std::string antigen;
    std::vector<acmacs::chart::Titer> titers;

    TiterRefCollapsed(const std::string& a_serum, const std::string& a_antigen, size_t num_tables) : serum{a_serum}, antigen{a_antigen}, titers(num_tables) {}

    static inline bool valid(const acmacs::chart::Titer& titer) { return !titer.is_dont_care(); }

    auto num_tables() const
    {
        return ranges::count_if(titers, valid);
    }

    auto mean_logged_titer() const
    {
        return ranges::accumulate(titers | ranges::views::filter(valid), 0.0, [](double sum, const auto& titer) { return sum + titer.logged_with_thresholded(); }) / static_cast<double>(num_tables());
    }

    bool eq(const TiterRef& raw) const { return serum == raw.serum && antigen == raw.antigen; }
};

class ChartData
{
  public:
    void scan(const acmacs::chart::Chart& chart)
    {
        const auto table_no = tables_.size();
        tables_.push_back(chart.info()->date());
        auto chart_antigens = chart.antigens();
        auto chart_sera = chart.sera();
        auto chart_titers = chart.titers();
        for (auto [ag_no, ag] : acmacs::enumerate(*chart_antigens)) {
            for (auto [sr_no, sr] : acmacs::enumerate(*chart_sera)) {
                if (const auto& titer = chart_titers->titer(ag_no, sr_no); !titer.is_dont_care())
                    raw_.push_back(TiterRef{.serum = sr->full_name(), .antigen = ag->full_name(), .table_no = table_no, .titer = titer});
            }
        }
    }

    void collapse()
    {
        ranges::sort(raw_, &TiterRef::operator<);
        for (const auto& raw : raw_) {
            if (collapsed_.empty())
                collapsed_.emplace_back(raw.serum, raw.antigen, tables_.size());
            else if (!collapsed_.back().eq(raw)) {
                if (collapsed_.back().num_tables() < 2)
                    collapsed_.pop_back();
                collapsed_.emplace_back(raw.serum, raw.antigen, tables_.size());
            }
            collapsed_.back().titers[raw.table_no] = raw.titer;
        }
    }

    void report_deviation_from_mean() const
    {
        for (const auto& en : collapsed_) {
            fmt::print("{}\n{}\n", en.serum, en.antigen);
            const auto mean = en.mean_logged_titer();
            for (size_t t_no = 0; t_no < tables_.size(); ++t_no) {
                if (!en.titers[t_no].is_dont_care())
                    fmt::print(" {}  {:>7s}  {:.2f}\n", tables_[t_no], en.titers[t_no], std::abs(en.titers[t_no].logged_with_thresholded() - mean));
                else
                    fmt::print(" {}\n", tables_[t_no]);
            }
            fmt::print("            {:.2f}\n\n", mean);
        }
    }

    void report_average_deviation_from_mean_per_table() const
    {
        std::vector<std::pair<double, size_t>> deviations(tables_.size(), {0.0, 0});
        for (const auto& en : collapsed_) {
            const auto mean = en.mean_logged_titer();
            ranges::for_each(ranges::views::iota(0ul, deviations.size()) //
                                 | ranges::views::filter([&en](size_t t_no) { return !en.titers[t_no].is_dont_care(); }),
                             [&en, &deviations, mean](size_t t_no) {
                                 deviations[t_no].first += std::abs(en.titers[t_no].logged_with_thresholded() - mean);
                                 ++deviations[t_no].second;
                             });
        }
        for (size_t t_no = 0; t_no < tables_.size(); ++t_no)
            fmt::print("{:2d} {}  {:.2f}  {:4d}\n", t_no, tables_[t_no], deviations[t_no].first / static_cast<double>(deviations[t_no].second), deviations[t_no].second);
    }

    auto make_distance_matrix(bool report = false) const
    {
        std::vector<std::vector<double>> matrix(tables_.size() * tables_.size());
        for (const auto& en : collapsed_) {
            for (size_t t1 = 0; t1 < (tables_.size() - 1); ++t1) {
                for (size_t t2 = t1 + 1; t2 < tables_.size(); ++t2) {
                    if (!en.titers[t1].is_dont_care() && !en.titers[t2].is_dont_care())
                        matrix[t1 * tables_.size() + t2].push_back(std::abs(en.titers[t1].logged_with_thresholded() - en.titers[t2].logged_with_thresholded()));
                }
            }
        }

        if (report) {
            for (size_t t1 = 0; t1 < (tables_.size() - 1); ++t1) {
                for (size_t t2 = t1 + 1; t2 < tables_.size(); ++t2) {
                    if (auto& distances = matrix[t1 * tables_.size() + t2]; !distances.empty()) {
                        ranges::sort(distances);
                        fmt::print("{} {} mean:{} median:{} max:{} {}\n", tables_[t1], tables_[t2], ranges::accumulate(distances, 0.0) / static_cast<double>(distances.size()),
                                   distances[distances.size() / 2], distances.back(), distances);
                    }
                    else
                        fmt::print("{} {}: *no distances*\n", tables_[t1], tables_[t2]);
                }
            }
        }

        acmacs::chart::TableDistances table_distances;
        for (size_t t1 = 0; t1 < (tables_.size() - 1); ++t1) {
            for (size_t t2 = t1 + 1; t2 < tables_.size(); ++t2) {
                if (auto& distances = matrix[t1 * tables_.size() + t2]; !distances.empty()) {
                    const auto mean = ranges::accumulate(distances, 0.0) / static_cast<double>(distances.size());
                    table_distances.add_value(acmacs::chart::Titer::Regular, t1, t2, mean);
                }
            }
        }
        return table_distances;
    }

    size_t num_tables() const { return tables_.size(); }
    constexpr const auto& tables() const { return tables_; }

  private:
    std::vector<acmacs::chart::TableDate> tables_;
    std::vector<TiterRef> raw_;
    std::vector<TiterRefCollapsed> collapsed_;
};

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "compare titers of mutiple tables"; }
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};

    option<str> output{*this, 'o', desc{"output .drawi file"}};
    option<size_t> number_of_optimizations{*this, 'n', dflt{100ul}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        ChartData data;
        for (const auto& fn : opt.charts)
            data.scan(*acmacs::chart::import_from_file(fn));
        data.collapse();
        // data.report_deviation_from_mean();
        data.report_average_deviation_from_mean_per_table();

        acmacs::chart::Stress stress(acmacs::number_of_dimensions_t{2}, data.num_tables());
        stress.table_distances() = data.make_distance_matrix();

        double best_stress{9e99};
        acmacs::Layout best_layout(data.num_tables(), stress.number_of_dimensions());
        for ([[maybe_unused]] auto attempt : acmacs::range(*opt.number_of_optimizations)) {
            acmacs::Layout layout(data.num_tables(), stress.number_of_dimensions());
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
        fmt::print("best_stress: {}\n", best_stress);

        if (opt.output) {
            acmacs::drawi::Generator gen;
            const acmacs::BoundingBall bb{minimum_bounding_ball(best_layout)};
            gen.viewport().set_from_center_size(bb.center(), bb.diameter());
            for (size_t t1 = 0; t1 < data.num_tables(); ++t1) {
                gen.add_point().coord(best_layout[t1]).fill(GREEN).outline(BLACK).outline_width(Pixels{1}).size(Pixels{10});
                // const auto fill = t1 < colors_of_tables.size() ? colors_of_tables[t1] : GREEN;
                // rescaled_surface.circle_filled(best_layout[t1], Pixels{10}, AspectNormal, NoRotation, BLACK, Pixels{1}, acmacs::surface::Dash::NoDash, fill);
                // rescaled_surface.text(best_layout[t1] + acmacs::PointCoordinates{-0.05, 0.05}, data.tables()[t1], BLACK, Pixels{10});
            }
            gen.generate(opt.output);
        }

        // if (opt.output) {
        //     const std::array colors_of_tables{
        //         GREEN,   // 20180206 Leah G/Heidi
        //         RED,     // 20180227 Tasuola
        //         BLUE,   // 20180528 Rob
        //         RED,     // 20180605 Tasuola
        //         BLUE,   // 20180626 Rob
        //         GREEN,   // 20180821 Heidi/Malet
        //         GREEN,   // 20180918 Heidi/Tas
        //         GREEN,   // 20181127 Heidi
        //         RED,   // 20181213 Tasuola
        //         RED,   // 20190211 Tasuola/Rob
        //         RED,   // 20190409 Tasuola
        //         GREEN,   // 20190514 Leah
        //         GREEN,   // 20190521 Leah
        //         GREEN,   // 20190716 Leah/Heidi
        //         GREEN,    // 20190820 Leah
        //         GREEN,   // 20190903 Leah
        //         GREEN,    // 20190918 Leah
        //     };
        //     const double size = 800.0;
        //     acmacs::surface::PdfCairo surface(opt.output, size, size, size);
        //     const acmacs::BoundingBall bb{minimum_bounding_ball(best_layout)};
        //     acmacs::Viewport viewport;
        //     viewport.set_from_center_size(bb.center(), bb.diameter());
        //     viewport.whole_width();
        //     fmt::print("{}\n", viewport);
        //     acmacs::surface::Surface& rescaled_surface = surface.subsurface(acmacs::PointCoordinates::zero2D, Scaled{surface.viewport().size.width}, viewport, true);
        //     rescaled_surface.grid(Scaled{1.0}, GREY, Pixels{1});
        //     for (size_t t1 = 0; t1 < data.num_tables(); ++t1) {
        //         const auto fill = t1 < colors_of_tables.size() ? colors_of_tables[t1] : GREEN;
        //         rescaled_surface.circle_filled(best_layout[t1], Pixels{10}, AspectNormal, NoRotation, BLACK, Pixels{1}, acmacs::surface::Dash::NoDash, fill);
        //         rescaled_surface.text(best_layout[t1] + acmacs::PointCoordinates{-0.05, 0.05}, data.tables()[t1], BLACK, Pixels{10});
        //     }
        //     acmacs::open(opt.output, 1, 1);
        // }
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
