// For sera-antigen titres that exist in multiple reference panels, plot the titre on the y axis against table number on the x axis.
// Colors: Green is the median, yellow is 1 log from the median and red is >1 log from the median.

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-draw/surface-cairo.hh"

// sNumberOfAllTiters, sAllTiters, sMedianTiterColors
// transparent, black, green, yellow, red, homologous_background
#include "whocc-reference-panel-plot-colors.hh"

// ----------------------------------------------------------------------

class ChartData;

static void process_source(ChartData& aData, std::string_view filename, bool only_existing_antigens_sera);
static void make_antigen_serum_set(ChartData& aData, std::string_view filename);

// ----------------------------------------------------------------------

class TiterData
{
 public:
    TiterData(size_t aAntigen, size_t aSerum, size_t aTable, const acmacs::chart::Titer& aTiter) : antigen(aAntigen), serum(aSerum), table(aTable), titer(aTiter) {}
    TiterData(size_t aAntigen, size_t aSerum, size_t aTable) : antigen(aAntigen), serum(aSerum), table(aTable) {}
    bool operator < (const TiterData& a) const
        {
            if (serum != a.serum)
                return serum < a.serum;
            if (antigen != a.antigen)
                return antigen < a.antigen;
            return table < a.table;
        }

    size_t antigen;
    size_t serum;
    size_t table;
    acmacs::chart::Titer titer;
};

class AntigenSerumData
{
 public:
    AntigenSerumData() {}
    bool empty() const { return titer_per_table.empty(); }
    size_t number_of_tables() const { return std::accumulate(titer_per_table.begin(), titer_per_table.end(), 0U, [](size_t acc, auto& element) { return element.first.is_dont_care() ? acc : (acc + 1); }); }
    size_t first_table_no() const { for (auto [no, entry] : acmacs::enumerate(titer_per_table)) if (!entry.first.is_dont_care()) return no; return 0; }

    std::pair<acmacs::chart::Titer, int> median;
    std::vector<std::pair<acmacs::chart::Titer, int>> titer_per_table;
};

struct CellParameters
{
    CellParameters(size_t aNumberOfTables, size_t aNumberOfTiters)
        : cell_top_title_height(1.3), hstep(static_cast<double>(aNumberOfTables) + 2.0), vstep(hstep), voffset_base(0.1), voffset_per_level((vstep - voffset_base * 2 - cell_top_title_height) / static_cast<double>(aNumberOfTiters - 1)) {}

    double cell_top_title_height;
    double hstep;
    double vstep;
    double voffset_base;
    double voffset_per_level;
};

struct AgSr
{
    AgSr(const std::string& aName) : name(aName) {}
    operator std::string() const { return name; }
    size_t size() const { return name.size(); }
    bool operator==(const AgSr& a) const { return name == a.name; }
    bool operator==(const std::string& a) const { return name == a; }

    std::string name;
    bool enabled{true};
    std::vector<size_t> homologous;
};

class AntigenSerumDoesNotPresent : public std::exception { public: using std::exception::exception; };

class ChartData
{
 public:
    using titer_iterator = std::vector<TiterData>::const_iterator;
    using range = std::pair<titer_iterator, titer_iterator>;

    ChartData() : mYAxisLabels{"5", "10", "20", "40", "80", "160", "320", "640", "1280", "2560", "5120", "10240", "20480", "40960"} {}

    size_t add_antigen(acmacs::chart::AntigenP aAntigen, bool only_existing);
    size_t add_serum(acmacs::chart::SerumP aSerum, bool only_existing);
    size_t add_table(acmacs::chart::ChartP aChart);
    void add_titer(size_t aAntigen, size_t aSerum, size_t aTable, const acmacs::chart::Titer& aTiter) { mTiters.emplace_back(aAntigen, aSerum, aTable, aTiter); mAllTiters.insert(aTiter); }

    void make_antigen_serum_data(size_t aMinNumberOfTables);
    void plot(std::string_view output_filename, bool for_ref_in_last_table_only);

    size_t number_of_antigens() const { return mAntigens.size(); }
    size_t number_of_sera() const { return mSera.size(); }
    size_t number_of_enabled_antigens() const { return std::accumulate(mAntigens.begin(), mAntigens.end(), 0U, [](size_t acc, auto& elt) { return elt.enabled ? (acc + 1) : acc; }); }
    size_t number_of_enabled_sera() const { return std::accumulate(mSera.begin(), mSera.end(), 0U, [](size_t acc, auto& elt) { return elt.enabled ? (acc + 1) : acc; }); }
    size_t number_of_tables() const { return mTables.size(); }
    size_t first_table_no() const;
    const AgSr& antigen(size_t antigen_no) const { return mAntigens[antigen_no]; }
    const AgSr& serum(size_t serum_no) const { return mSera[serum_no]; }
    AgSr& serum(size_t serum_no) { return mSera[serum_no]; }
    size_t longest_serum_name() const { return std::max_element(mSera.begin(), mSera.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    size_t longest_antigen_name() const { return std::max_element(mAntigens.begin(), mAntigens.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    range find_range(size_t aSerum, size_t aAntigen) const;
    acmacs::chart::Titer median(const range& aRange) const;

    bool empty() const { return mTiters.empty(); }

    template <typename FormatCtx> auto format(FormatCtx& ctx) const
    {
        return fmt::format_to(ctx.out(), "Tables:{} Sera:{} Antigens:{} Titers:{}\nTiters: {}", mTables.size(), mSera.size(), mAntigens.size(), mTiters.size(), mAllTiters);
    }

  private:
    std::vector<std::string> mTables;
    std::vector<AgSr> mSera;
    std::vector<AgSr> mAntigens;
    std::vector<TiterData> mTiters;
    std::set<acmacs::chart::Titer> mAllTiters;
    std::map<acmacs::chart::Titer, size_t> mTiterLevel;
    std::vector<std::vector<AntigenSerumData>> mAntigenSerumData;
    std::string mLab, mVirusType, mAssay, mFirstDate, mLastDate;
    std::vector<std::string> mYAxisLabels;

    void sort_titers_by_serum_antigen() { std::sort(mTiters.begin(), mTiters.end()); }

    size_t titer_index_in_sAllTiters(acmacs::chart::Titer aTiter) const
        {
            const auto iter = std::find(std::begin(sAllTiters), std::end(sAllTiters), aTiter);
            return static_cast<size_t>(iter - std::begin(sAllTiters));
        }

    Color color_for_titer(acmacs::chart::Titer aTiter, size_t aMedianIndex) const
        {
            const auto titer_index = titer_index_in_sAllTiters(aTiter);
            if (aMedianIndex == sNumberOfAllTiters || titer_index == sNumberOfAllTiters)
                AD_WARNING("Invalid median ({}) or titer ({}) index, sNumberOfAllTiters: {}", aMedianIndex, titer_index, sNumberOfAllTiters);
            return sMedianTiterColors[aMedianIndex][titer_index];
        }

    void text(acmacs::surface::Surface& aSurface, const acmacs::PointCoordinates& aOffset, std::string aText, Color aColor, Rotation aRotation, double aFontSize, double aMaxWidth) const
        {
            const auto size = aSurface.text_size(aText, Scaled{aFontSize});
            if (size.width > aMaxWidth)
                aFontSize *= aMaxWidth / size.width;
            aSurface.text(aOffset, aText, aColor, Scaled{aFontSize}, acmacs::TextStyle(), aRotation);
        }

    void plot_antigen_serum_cell(size_t antigen_no, size_t serum_no, acmacs::surface::Surface& aCell, const CellParameters& aParameters, size_t first_tab_no);
    void plot_antigen_serum_cell_with_fixed_titer_range(size_t antigen_no, size_t serum_no, acmacs::surface::Surface& aCell, const CellParameters& aParameters, size_t first_tab_no);

    void disable_antigens_sera(size_t aMinNumberOfTables);
};

// ----------------------------------------------------------------------

template <> struct fmt::formatter<AntigenSerumData> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const AntigenSerumData& aData, FormatCtx& ctx) const
    {
        constexpr const int titer_width = 7;
        fmt::format_to(ctx.out, "[{:>{}}({})]", *aData.median.first, titer_width, aData.median.second);
        for (const auto& element : aData.titer_per_table)
            fmt::format_to(ctx.out, "{:>{}}({})", *element.first, titer_width, element.second);
        return ctx.out();
    }
};

template <> struct fmt::formatter<ChartData> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ChartData& aData, FormatCtx& ctx) const
    {
        return aData.format(ctx);
    }
};


// ======================================================================

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>    output_pdf{*this, 'o', desc{"output.pdf"}};
    option<bool>   last{*this, "last", desc{"for ref antigens and sera found in the last table only"}};
    option<size_t> min_tables{*this, "min-tables", dflt{5ul}, desc{"minimum number of tables where antigen/serum appears"}};
    // option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str_array> charts{*this, arg_name{".ace"}, mandatory};
};

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        ChartData data;
        if (opt.last)
            make_antigen_serum_set(data, opt.charts->back());
        for (const auto& chart_file : *opt.charts) {
            process_source(data, chart_file, opt.last);
            // if (data.empty())
            //     AD_WARNING("{} has no reference antigens found in {}", chart_file, opt.charts->back());
        }
        if (!data.empty()) {
            data.make_antigen_serum_data(opt.min_tables);
            fmt::print("{}\n", data);
            data.plot(opt.output_pdf, opt.last);
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void make_antigen_serum_set(ChartData& aData, std::string_view filename)
{
    auto chart = acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report_time::no);
    auto chart_antigens = chart->antigens();
    for (auto antigen_index_in_chart: chart_antigens->reference_indexes()) {
        aData.add_antigen((*chart_antigens)[antigen_index_in_chart], false);
    }
    auto chart_sera = chart->sera();
    for (size_t serum_no = 0; serum_no < chart_sera->size(); ++serum_no) {
        aData.add_serum((*chart_sera)[serum_no], false);
    }

} // make_antigen_serum_set

// ----------------------------------------------------------------------

void process_source(ChartData& aData, std::string_view filename, bool only_existing_antigens_sera)
{
    // AD_DEBUG("process_source {}", filename);
    std::map<size_t, size_t> antigens; // index in chart to index in aData.mAntigens|mSera
    auto chart = acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report_time::no);
    chart->set_homologous(acmacs::chart::find_homologous::strict);
    const auto table_no = aData.add_table(chart);
    auto chart_antigens = chart->antigens();
    auto chart_sera = chart->sera();
    auto chart_titers = chart->titers();
    for (auto antigen_index_in_chart : chart_antigens->reference_indexes()) {
        try {
            antigens[antigen_index_in_chart] = aData.add_antigen((*chart_antigens)[antigen_index_in_chart], only_existing_antigens_sera);
        }
        catch (AntigenSerumDoesNotPresent&) {
            // AD_DEBUG("AntigenSerumDoesNotPresent AG {}", antigen_index_in_chart);
        }
    }
    for (size_t serum_no = 0; serum_no < chart_sera->size(); ++serum_no) {
        try {
            const size_t serum_index_in_data = aData.add_serum((*chart_sera)[serum_no], only_existing_antigens_sera);
            for (const auto& antigen : antigens) {
                aData.add_titer(antigen.second, serum_index_in_data, table_no, chart_titers->titer(antigen.first, serum_no));
            }
            for (size_t homologous_antigen : (*chart_sera)[serum_no]->homologous_antigens()) {
                aData.serum(serum_index_in_data).homologous.push_back(antigens[homologous_antigen]);
            }
        }
        catch (AntigenSerumDoesNotPresent&) {
        }
    }

} // process_source

// ======================================================================

size_t ChartData::add_antigen(acmacs::chart::AntigenP aAntigen, bool only_existing)
{
    const auto name = aAntigen->format("{name_full}");
    const auto pos = std::find(mAntigens.begin(), mAntigens.end(), name);
    size_t result = static_cast<size_t>(pos - mAntigens.begin());
    if (pos == mAntigens.end()) {
        if (only_existing)
            throw AntigenSerumDoesNotPresent{};
        mAntigens.emplace_back(name);
        result = mAntigens.size() - 1;
    }
    // AD_DEBUG("add_antigen {} \"{}\"", result, name);
    return result;

} // ChartData::add_antigen

// ----------------------------------------------------------------------

size_t ChartData::add_serum(acmacs::chart::SerumP aSerum, bool only_existing)
{
    const std::string name = aSerum->format("{name_full}");
    const auto pos = std::find(mSera.begin(), mSera.end(), name);
    size_t result = static_cast<size_t>(pos - mSera.begin());
    if (pos == mSera.end()) {
        if (only_existing)
            throw AntigenSerumDoesNotPresent{};
        mSera.emplace_back(name);
        result = mSera.size() - 1;
    }
    return result;

} // ChartData::add_serum

// ----------------------------------------------------------------------

size_t ChartData::add_table(acmacs::chart::ChartP aChart)
{
    auto info = aChart->info();
    mLab = info->lab();
    mVirusType = info->virus_type();
    if (mVirusType == "B")
        mVirusType += "/" + aChart->lineage()->substr(0, 3);
    mAssay = info->assay();
    if (mFirstDate.empty() || *info->date() < mFirstDate)
        mFirstDate = info->date();
    if (mLastDate.empty() || *info->date() > mLastDate)
        mLastDate = info->date();
    mTables.push_back(info->make_name());
    return mTables.size() - 1;

} // ChartData::add_table

// ----------------------------------------------------------------------

void ChartData::make_antigen_serum_data(size_t aMinNumberOfTables)
{
    sort_titers_by_serum_antigen();

    mAllTiters.insert(acmacs::chart::Titer());
    size_t level = mAllTiters.size() - 1;
    for (const auto& titer: mAllTiters) {
        mTiterLevel[titer] = level;
        --level;
    }

    AD_DEBUG("mTiterLevel: {}  mTiters: {}", mTiterLevel, mTiters.size());
    for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        mAntigenSerumData.emplace_back(number_of_sera());
        for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
            auto range = find_range(serum_no, antigen_no);
            // AD_DEBUG("AG {} SR {} range {}", antigen_no, serum_no, range.second -  range.first);
            if (range.first != mTiters.end() && range.first->antigen == antigen_no && range.first->serum == serum_no) {
                const auto median_titer = median(range);
                auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
                ag_sr_data.median = std::make_pair(median_titer, mTiterLevel[median_titer]);
                for (size_t table_no = 0; table_no < number_of_tables(); ++table_no) {
                    if (range.first != range.second && range.first->table == table_no) {
                        ag_sr_data.titer_per_table.emplace_back(range.first->titer, mTiterLevel[range.first->titer]);
                        ++range.first;
                    }
                    else
                        ag_sr_data.titer_per_table.emplace_back(acmacs::chart::Titer{}, mTiterLevel[acmacs::chart::Titer{}]);
                }
            }
        }
    }
    disable_antigens_sera(aMinNumberOfTables);

} // ChartData::make_antigen_serum_data

// ----------------------------------------------------------------------

void ChartData::disable_antigens_sera(size_t aMinNumberOfTables)
{
    for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        size_t max_number_of_tables = 0;
        for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
            max_number_of_tables = std::max(max_number_of_tables, mAntigenSerumData[antigen_no][serum_no].number_of_tables());
        }
        mAntigens[antigen_no].enabled = max_number_of_tables >= aMinNumberOfTables;
        // AD_DEBUG("{} AG {} tables: {}", mAntigens[antigen_no].enabled ? "enabled " : "DISABLED", mAntigens[antigen_no].name, max_number_of_tables);
    }

    for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
        size_t max_number_of_tables = 0;
        for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
            max_number_of_tables = std::max(max_number_of_tables, mAntigenSerumData[antigen_no][serum_no].number_of_tables());
        }
        mSera[serum_no].enabled = max_number_of_tables >= aMinNumberOfTables;
        // AD_DEBUG("{} SR {} tables: {}", mSera[serum_no].enabled ? "enabled " : "DISABLED", mSera[serum_no].name, max_number_of_tables);
    }

} // ChartData::disable_antigens_sera

// ----------------------------------------------------------------------

size_t ChartData::first_table_no() const
{
    size_t first = 1000;
    for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
            first = std::min(first, mAntigenSerumData[antigen_no][serum_no].first_table_no());
        }
    }
    return first;

} // ChartData::first_table_no

// ----------------------------------------------------------------------

ChartData::range ChartData::find_range(size_t aSerum, size_t aAntigen) const
{
    return std::make_pair(std::lower_bound(mTiters.begin(), mTiters.end(), TiterData(aAntigen, aSerum, 0)),
                          std::upper_bound(mTiters.begin(), mTiters.end(), TiterData(aAntigen, aSerum, number_of_tables())));

} // ChartData::find_range

// ----------------------------------------------------------------------

acmacs::chart::Titer ChartData::median(const ChartData::range& aRange) const
{
    std::vector<acmacs::chart::Titer> titers;
    std::transform(aRange.first, aRange.second, std::back_inserter(titers), [](const auto& e) { return e.titer; });
    std::sort(titers.begin(), titers.end());
      // ignore dont-cares when finding median
    const auto first = std::find_if_not(titers.begin(), titers.end(), [](const auto& titer) { return titer.is_dont_care(); });
    const auto offset = (titers.end() - first) / 2 - ((titers.end() - first) % 2 ? 0 : 1);
    return *(first + offset);

} // ChartData::median

// ======================================================================

void ChartData::plot(std::string_view output_filename, bool for_ref_in_last_table_only)
{
    const size_t ns = number_of_enabled_sera(), na = number_of_enabled_antigens();
    fmt::print("Enabled: antigens: {} sera: {}\n", na, ns);
    if (ns == 0 || na == 0)
        throw std::runtime_error{AD_FORMAT("cannot plot: too few enabled antigens ({}) or sera ({}), total antigens: {} sera: {}", na, ns, number_of_antigens(), number_of_sera())};
    CellParameters cell_parameters{number_of_tables() - first_table_no(), mTiterLevel.size()};
    const double title_height = cell_parameters.vstep * 0.5;

    const acmacs::Viewport cell_viewport{acmacs::Size{cell_parameters.hstep, cell_parameters.vstep}};

    acmacs::surface::PdfCairo surface(output_filename, static_cast<double>(ns) * cell_parameters.hstep, static_cast<double>(na) * cell_parameters.vstep + title_height, static_cast<double>(ns) * cell_parameters.hstep);

    std::string title;
    if (for_ref_in_last_table_only)
        title = mLab + " " + mVirusType + " " + mAssay + " " + mLastDate + "  previous tables:" + std::to_string(number_of_tables() - first_table_no() - 1) + " sera:" + std::to_string(ns) + " antigens:" + std::to_string(na);
    else
        title = mLab + " " + mVirusType + " " + mAssay + " " + mFirstDate + "-" + mLastDate + "  tables:" + std::to_string(number_of_tables() - first_table_no()) + " sera:" + std::to_string(ns) + " antigens:" + std::to_string(na);
    text(surface, {title_height, title_height * 0.7}, title, BLACK, NoRotation, title_height * 0.8, static_cast<double>(ns) * cell_parameters.hstep - title_height * 2);

    const auto first_tab_no = first_table_no();
    for (size_t antigen_no = 0, enabled_antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        if (mAntigens[antigen_no].enabled) {
            for (size_t serum_no = 0, enabled_serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
                if (mSera[serum_no].enabled) {
                    acmacs::surface::Surface& cell = surface.subsurface({static_cast<double>(enabled_serum_no) * cell_parameters.hstep, static_cast<double>(enabled_antigen_no) * cell_parameters.vstep + title_height}, Scaled{cell_parameters.hstep}, cell_viewport, true);
                      //plot_antigen_serum_cell(antigen_no, serum_no, cell, cell_parameters, first_tab_no);
                    plot_antigen_serum_cell_with_fixed_titer_range(antigen_no, serum_no, cell, cell_parameters, first_tab_no);
                    ++enabled_serum_no;
                }
            }
            ++enabled_antigen_no;
        }
    }

} // ChartData::plot

// ----------------------------------------------------------------------

void ChartData::plot_antigen_serum_cell(size_t antigen_no, size_t serum_no, acmacs::surface::Surface& aCell, const CellParameters& aParameters, size_t first_tab_no)
{
    const auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
    const size_t median_index = titer_index_in_sAllTiters(ag_sr_data.median.first);
    const acmacs::Viewport& cell_v = aCell.viewport();
    aCell.rectangle(cell_v.origin, cell_v.size, BLACK, Pixels{0.4});
      // serum name
    text(aCell, {aParameters.cell_top_title_height * 1.2, aParameters.cell_top_title_height}, mSera[serum_no], BLACK, NoRotation, aParameters.cell_top_title_height, (aParameters.hstep - aParameters.cell_top_title_height * 1.5));
      // antigen name
    text(aCell, {aParameters.cell_top_title_height, aParameters.vstep - aParameters.voffset_base}, mAntigens[antigen_no], BLACK, Rotation{-M_PI_2}, aParameters.cell_top_title_height, (aParameters.vstep - aParameters.voffset_base * 1.5));
      // titer value marks
    for (const auto& element: mTiterLevel) {
        aCell.text({aParameters.hstep - aParameters.cell_top_title_height * 2, aParameters.cell_top_title_height + aParameters.voffset_base + static_cast<double>(element.second) * aParameters.voffset_per_level + aParameters.voffset_per_level * 0.5}, *element.first, BLACK, Scaled{aParameters.cell_top_title_height / 2});
    }

    double table_no = 2 - static_cast<int>(first_tab_no);
    for (const auto& element: ag_sr_data.titer_per_table) {
        if (!element.first.is_dont_care()) { // do not draw dont-care titer
            const double symbol_top = aParameters.cell_top_title_height + aParameters.voffset_base + element.second * aParameters.voffset_per_level;
            const double symbol_bottom = symbol_top + aParameters.voffset_per_level;
            const Color symbol_color = color_for_titer(element.first, median_index);
            if (element.first.is_less_than()) {
                aCell.triangle_filled({table_no - 0.5, symbol_top},
                                      {table_no + 0.5, symbol_top},
                                      {table_no,       symbol_bottom},
                                      TRANSPARENT, Pixels{0}, symbol_color);
            }
            else if (element.first.is_more_than()) {
                aCell.triangle_filled({table_no - 0.5, symbol_bottom},
                                      {table_no + 0.5, symbol_bottom},
                                      {table_no,       symbol_top},
                                      TRANSPARENT, Pixels{0}, symbol_color);
            }
            else {
                aCell.rectangle_filled({table_no - 0.5, symbol_top}, {1, aParameters.voffset_per_level}, transparent, Pixels{0}, symbol_color);
            }
        }
        ++table_no;
    }

} // ChartData::plot_antigen_serum_cell

// ----------------------------------------------------------------------

void ChartData::plot_antigen_serum_cell_with_fixed_titer_range(size_t antigen_no, size_t serum_no, acmacs::surface::Surface& aCell, const CellParameters& aParameters, size_t first_tab_no)
{
    const double logged_titer_step = (aParameters.vstep - aParameters.voffset_base - aParameters.cell_top_title_height) / static_cast<double>(mYAxisLabels.size());

    const acmacs::Viewport& cell_v = aCell.viewport();
    if (std::find(mSera[serum_no].homologous.begin(), mSera[serum_no].homologous.end(), antigen_no) != mSera[serum_no].homologous.end())
        aCell.rectangle_filled(cell_v.origin, cell_v.size, homologous_background, Pixels{0}, homologous_background);     // homologous antigen and serum
    aCell.rectangle(cell_v.origin, cell_v.size, BLACK, Pixels{0.4});
      // serum name
    text(aCell, {aParameters.cell_top_title_height * 1.2, aParameters.cell_top_title_height}, mSera[serum_no], BLACK, NoRotation, aParameters.cell_top_title_height, (aParameters.hstep - aParameters.cell_top_title_height * 1.5));
      // antigen name
    text(aCell, {aParameters.cell_top_title_height, aParameters.vstep - aParameters.voffset_base}, mAntigens[antigen_no], BLACK, Rotation{-M_PI_2}, aParameters.cell_top_title_height, (aParameters.vstep - aParameters.voffset_base * 1.5));

      // titer value marks
    const double titer_label_font_size = aParameters.cell_top_title_height * 0.7;
    size_t titer_label_vpos = 0;
    for (const auto& titer_label: mYAxisLabels) {
        aCell.text_right_aligned({aParameters.hstep - aParameters.cell_top_title_height * 0.2,
                          // aParameters.cell_top_title_height + aParameters.voffset_base + titer_label_vpos * logged_titer_step + logged_titer_step * 0.5},
                aParameters.vstep - aParameters.voffset_base - static_cast<double>(titer_label_vpos) * logged_titer_step - logged_titer_step * 0.5 + titer_label_font_size * 0.3},
                   titer_label, BLACK, Scaled{titer_label_font_size});
        ++titer_label_vpos;
    }

    const auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
    const size_t median_index = titer_index_in_sAllTiters(ag_sr_data.median.first);
    double table_no = 2 - static_cast<int>(first_tab_no);
    for (const auto& element: ag_sr_data.titer_per_table) {
        if (!element.first.is_dont_care()) { // do not draw dont-care titer
            const double titer = element.first.logged_with_thresholded();
              // const double symbol_top = aParameters.cell_top_title_height + aParameters.voffset_base + (titer + 1) * logged_titer_step;
            const double symbol_top = aParameters.vstep - aParameters.voffset_base - (titer + 2) * logged_titer_step;
            const double symbol_bottom = symbol_top + logged_titer_step;
            const Color symbol_color = color_for_titer(element.first, median_index);
            if (element.first.is_less_than()) {
                aCell.triangle_filled({table_no - 0.5, symbol_top},
                                      {table_no + 0.5, symbol_top},
                                      {table_no,       symbol_bottom},
                                      transparent, Pixels{0}, symbol_color);
            }
            else if (element.first.is_more_than()) {
                aCell.triangle_filled({table_no - 0.5, symbol_bottom},
                                      {table_no + 0.5, symbol_bottom},
                                      {table_no,       symbol_top},
                                      transparent, Pixels{0}, symbol_color);
            }
            else {
                aCell.rectangle_filled({table_no - 0.5, symbol_top}, {1, logged_titer_step}, transparent, Pixels{0}, symbol_color);
            }
        }
        ++table_no;
    }

} // ChartData::plot_antigen_serum_cell_with_fixed_titer_range

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
