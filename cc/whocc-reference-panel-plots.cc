// For sera-antigen titres that exist in multiple reference panels, plot the titre on the y axis against table number on the x axis.
// Colors: Green is the median, yellow is 1 log from the median and red is >1 log from the median.

#include <string>
#include <iomanip>
#include <cmath>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "boost/program_options.hpp"
#pragma GCC diagnostic pop

#include "acmacs-base/stream.hh"
#include "acmacs-chart/ace.hh"
#include "acmacs-draw/surface-cairo.hh"

// sNumberOfAllTiters, sAllTiters, sMedianTiterColors
// transparent, black, green, yellow, red, homologous_background
#include "whocc-reference-panel-plot-colors.hh"

// ----------------------------------------------------------------------

class Options
{
 public:
    std::vector<std::string> source_charts;
    std::string output_filename;
};

class ChartData;

static int get_args(int argc, const char *argv[], Options& aOptions);
static void process_source(ChartData& aData, std::string filename);

// ----------------------------------------------------------------------

class TiterData
{
 public:
    inline TiterData(size_t aAntigen, size_t aSerum, size_t aTable, const Titer& aTiter) : antigen(aAntigen), serum(aSerum), table(aTable), titer(aTiter) {}
    inline TiterData(size_t aAntigen, size_t aSerum, size_t aTable) : antigen(aAntigen), serum(aSerum), table(aTable) {}
    inline bool operator < (const TiterData& a) const
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
    Titer titer;
};

class AntigenSerumData
{
 public:
    inline AntigenSerumData() {}
    inline bool empty() const { return titer_per_table.empty(); }
    inline size_t number_of_tables() const { return std::accumulate(titer_per_table.begin(), titer_per_table.end(), 0U, [](size_t acc, auto& element) { return element.first.is_dont_care() ? acc : (acc + 1); }); }

    std::pair<Titer, int> median;
    std::vector<std::pair<Titer, int>> titer_per_table;
    friend std::ostream& operator << (std::ostream& out, const AntigenSerumData& aData);
};

struct CellParameters
{
    inline CellParameters(size_t aNumberOfTables, size_t aNumberOfTiters)
        : cell_top_title_height(1.3), hstep(aNumberOfTables + 2.0), vstep(hstep), voffset_base(0.1), voffset_per_level((vstep - voffset_base * 2 - cell_top_title_height) / (aNumberOfTiters - 1)) {}

    double cell_top_title_height;
    double hstep;
    double vstep;
    double voffset_base;
    double voffset_per_level;
};

struct AgSr
{
    inline AgSr(std::string aName) : name(aName), enabled(true) {}
    inline operator std::string() const { return name; }
    inline size_t size() const { return name.size(); }
    inline bool operator==(const AgSr& a) const { return name == a.name; }
    inline bool operator==(std::string a) const { return name == a; }

    std::string name;
    bool enabled;
    std::vector<size_t> homologous;
};

class ChartData
{
 public:
    using titer_iterator = std::vector<TiterData>::const_iterator;
    using range = std::pair<titer_iterator, titer_iterator>;

    inline ChartData() : mYAxisLabels{"5", "10", "20", "40", "80", "160", "320", "640", "1280", "2560", "5120", "10240", "20480", "40960"} {}

    size_t add_antigen(const Antigen& aAntigen);
    size_t add_serum(const Serum& aSerum);
    size_t add_table(const Chart& aChart);
    inline void add_titer(size_t aAntigen, size_t aSerum, size_t aTable, const Titer& aTiter) { mTiters.emplace_back(aAntigen, aSerum, aTable, aTiter); mAllTiters.insert(aTiter); }

    void make_antigen_serum_data();
    void plot(std::string output_filename);

    inline size_t number_of_antigens() const { return mAntigens.size(); }
    inline size_t number_of_sera() const { return mSera.size(); }
    inline size_t number_of_enabled_antigens() const { return std::accumulate(mAntigens.begin(), mAntigens.end(), 0U, [](size_t acc, auto& elt) { return elt.enabled ? (acc + 1) : acc; }); }
    inline size_t number_of_enabled_sera() const { return std::accumulate(mSera.begin(), mSera.end(), 0U, [](size_t acc, auto& elt) { return elt.enabled ? (acc + 1) : acc; }); }
    inline size_t number_of_tables() const { return mTables.size(); }
    inline const AgSr& antigen(size_t antigen_no) const { return mAntigens[antigen_no]; }
    inline const AgSr& serum(size_t serum_no) const { return mSera[serum_no]; }
    inline AgSr& serum(size_t serum_no) { return mSera[serum_no]; }
    inline size_t longest_serum_name() const { return std::max_element(mSera.begin(), mSera.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    inline size_t longest_antigen_name() const { return std::max_element(mAntigens.begin(), mAntigens.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    range find_range(size_t aSerum, size_t aAntigen) const;
    Titer median(const range& aRange) const;

 private:
    std::vector<std::string> mTables;
    std::vector<AgSr> mSera;
    std::vector<AgSr> mAntigens;
    std::vector<TiterData> mTiters;
    std::set<Titer> mAllTiters;
    std::map<Titer, size_t> mTiterLevel;
    std::vector<std::vector<AntigenSerumData>> mAntigenSerumData;
    std::string mLab, mVirusType, mAssay, mFirstDate, mLastDate;
    std::vector<std::string> mYAxisLabels;

    friend std::ostream& operator << (std::ostream& out, const ChartData& aData);
    inline void sort_titers_by_serum_antigen() { std::sort(mTiters.begin(), mTiters.end()); }

    inline size_t titer_index_in_sAllTiters(Titer aTiter) const
        {
            const auto iter = std::find(std::begin(sAllTiters), std::end(sAllTiters), aTiter);
            return static_cast<size_t>(iter - std::begin(sAllTiters));
        }

    inline Color color_for_titer(Titer aTiter, size_t aMedianIndex) const
        {
            const auto titer_index = titer_index_in_sAllTiters(aTiter);
            if (aMedianIndex == sNumberOfAllTiters || titer_index == sNumberOfAllTiters)
                std::cerr << "Invalid median or titer index: " << aMedianIndex << " " << titer_index << std::endl;
            return sMedianTiterColors[aMedianIndex][titer_index];
        }

    inline void text(Surface& aSurface, const Location& aOffset, std::string aText, Color aColor, Rotation aRotation, double aFontSize, double aMaxWidth) const
        {
            const auto size = aSurface.text_size(aText, Scaled{aFontSize});
            if (size.width > aMaxWidth)
                aFontSize *= aMaxWidth / size.width;
            aSurface.text(aOffset, aText, aColor, Scaled{aFontSize}, TextStyle(), aRotation);
        }

    void plot_antigen_serum_cell(size_t antigen_no, size_t serum_no, Surface& aCell, const CellParameters& aParameters);
    void plot_antigen_serum_cell_with_fixed_titer_range(size_t antigen_no, size_t serum_no, Surface& aCell, const CellParameters& aParameters);

    void disable_antigens_sera(size_t aThreshold = 4);
};

// ======================================================================

int main(int argc, const char *argv[])
{
    Options options;
    int exit_code = get_args(argc, argv, options);
    if (exit_code == 0) {
        try {
            ChartData data;
            for (const auto& source_name: options.source_charts)
                process_source(data, source_name);
            data.make_antigen_serum_data();
            std::cout << data << std::endl;
            data.plot(options.output_filename);
        }
        catch (std::exception& err) {
            std::cerr << err.what() << std::endl;
            exit_code = 1;
        }
    }
    return exit_code;
}

// ----------------------------------------------------------------------

static int get_args(int argc, const char *argv[], Options& aOptions)
{
    using namespace boost::program_options;
    options_description desc("Options");
    desc.add_options()
            ("help", "Print help messages")
            ("output,o", value<std::string>(&aOptions.output_filename)->required(), "output pdf")
            ("sources,s", value<std::vector<std::string>>(&aOptions.source_charts), "source chart in the proper order")
            ;
    positional_options_description pos_opt;
    pos_opt.add("output", 1);
    pos_opt.add("sources", -1);

    variables_map vm;
    try {
        store(command_line_parser(argc, argv).options(desc).positional(pos_opt).run(), vm);
        if (vm.count("help")) {
            std::cerr << desc << std::endl;
            return 1;
        }
        notify(vm);
        return 0;
    }
    catch(required_option& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
          // std::cerr << "Usage: " << argv[0] << " <tree.json> <output.pdf>" << std::endl;
        return 2;
    }
    catch(error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        return 3;
    }

} // get_args

// ----------------------------------------------------------------------

void process_source(ChartData& aData, std::string filename)
{
    std::map<size_t, size_t> antigens; // index in chart to index in aData.mAntigens|mSera
    std::unique_ptr<Chart> chart{import_chart(filename)};
    size_t table_no = aData.add_table(*chart);
    chart->find_homologous_antigen_for_sera();
    std::vector<size_t> ref_antigens;
    chart->antigens().reference_indices(ref_antigens);
    for (size_t antigen_index_in_chart: ref_antigens) {
        antigens[antigen_index_in_chart] = aData.add_antigen(static_cast<const Antigen&>(chart->antigen(antigen_index_in_chart)));
    }
    for (size_t serum_no = 0; serum_no < chart->sera().size(); ++serum_no) {
        const size_t serum_index_in_data = aData.add_serum(static_cast<const Serum&>(chart->serum(serum_no)));
        for (const auto& antigen: antigens) {
            // std::cerr << serum_index_in_data << ' ' << aData.serum(serum_index_in_data) << " -- " << aData.antigen(antigen.second) << " -- " << chart->titers().get(antigen.first, serum_no) << std::endl;
            aData.add_titer(antigen.second, serum_index_in_data, table_no, chart->titers().get(antigen.first, serum_no));
        }
        for (size_t homologous_antigen: chart->serum(serum_no).homologous()) {
            aData.serum(serum_index_in_data).homologous.push_back(antigens[homologous_antigen]);
        }
    }

} // process_source

// ======================================================================

size_t ChartData::add_antigen(const Antigen& aAntigen)
{
    const std::string name = aAntigen.full_name();
    const auto pos = std::find(mAntigens.begin(), mAntigens.end(), name);
    size_t result = static_cast<size_t>(pos - mAntigens.begin());
    if (pos == mAntigens.end()) {
        mAntigens.emplace_back(name);
        result = mAntigens.size() - 1;
    }
    return result;

} // ChartData::add_antigen

// ----------------------------------------------------------------------

size_t ChartData::add_serum(const Serum& aSerum)
{
    const std::string name = aSerum.full_name_without_passage();
    const auto pos = std::find(mSera.begin(), mSera.end(), name);
    size_t result = static_cast<size_t>(pos - mSera.begin());
    if (pos == mSera.end()) {
        mSera.emplace_back(name);
        result = mSera.size() - 1;
    }
    return result;

} // ChartData::add_serum

// ----------------------------------------------------------------------

size_t ChartData::add_table(const Chart& aChart)
{
    mLab = aChart.chart_info().lab();
    mVirusType = aChart.chart_info().virus_type();
    if (mVirusType == "B")
        mVirusType += "/" + aChart.lineage().substr(0, 3);
    mAssay = aChart.chart_info().assay();
    if (mFirstDate.empty() || aChart.chart_info().date() < mFirstDate)
        mFirstDate = aChart.chart_info().date();
    if (mLastDate.empty() || aChart.chart_info().date() > mLastDate)
        mLastDate = aChart.chart_info().date();
    mTables.push_back(aChart.chart_info().make_name());
    return mTables.size() - 1;

} // ChartData::add_table

// ----------------------------------------------------------------------

void ChartData::make_antigen_serum_data()
{
    sort_titers_by_serum_antigen();

    mAllTiters.insert(Titer());
    size_t level = mAllTiters.size() - 1;
    for (const auto& titer: mAllTiters) {
        mTiterLevel[titer] = level;
        --level;
    }
    // std::cerr << "mTiterLevel: " << mTiterLevel << std::endl;

    for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        mAntigenSerumData.emplace_back(number_of_sera());
        for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
            auto range = find_range(serum_no, antigen_no);
            if (range.first->antigen == antigen_no && range.first->serum == serum_no) {
                const auto median_titer = median(range);
                auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
                ag_sr_data.median = std::make_pair(median_titer, mTiterLevel[median_titer]);
                for (size_t table_no = 0; table_no < number_of_tables(); ++ table_no) {
                    if (range.first != range.second && range.first->table == table_no) {
                        ag_sr_data.titer_per_table.emplace_back(range.first->titer, mTiterLevel[range.first->titer]);
                        ++range.first;
                    }
                    else
                        ag_sr_data.titer_per_table.emplace_back(Titer{}, mTiterLevel[Titer{}]);
                }
            }
        }
    }
    disable_antigens_sera(4);

} // ChartData::make_antigen_serum_data

// ----------------------------------------------------------------------

void ChartData::disable_antigens_sera(size_t aThreshold)
{
    for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        size_t max_number_of_tables = 0;
        for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
            max_number_of_tables = std::max(max_number_of_tables, mAntigenSerumData[antigen_no][serum_no].number_of_tables());
        }
        mAntigens[antigen_no].enabled = max_number_of_tables > aThreshold;
        // std::cerr << "AG " << mAntigens[antigen_no].name << " tables: " << max_number_of_tables << std::endl;
    }

    for (size_t serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
        size_t max_number_of_tables = 0;
        for (size_t antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
            max_number_of_tables = std::max(max_number_of_tables, mAntigenSerumData[antigen_no][serum_no].number_of_tables());
        }
        mSera[serum_no].enabled = max_number_of_tables > aThreshold;
          // std::cerr << "SR " << mSera[serum_no].name << " tables: " << max_number_of_tables << std::endl;
    }

} // ChartData::disable_antigens_sera

// ----------------------------------------------------------------------

ChartData::range ChartData::find_range(size_t aSerum, size_t aAntigen) const
{
    return std::make_pair(std::lower_bound(mTiters.begin(), mTiters.end(), TiterData(aAntigen, aSerum, 0)), std::upper_bound(mTiters.begin(), mTiters.end(), TiterData(aAntigen, aSerum, number_of_tables())));

} // ChartData::find_range

// ----------------------------------------------------------------------

Titer ChartData::median(const ChartData::range& aRange) const
{
    std::vector<Titer> titers;
    std::transform(aRange.first, aRange.second, std::back_inserter(titers), [](const auto& e) { return e.titer; });
    std::sort(titers.begin(), titers.end());
      // ignore dont-cares when finding median
    const auto first = std::find_if_not(titers.begin(), titers.end(), [](const auto& titer) { return titer.is_dont_care(); });
    const auto offset = (titers.end() - first) / 2 - ((titers.end() - first) % 2 ? 0 : 1);
    return *(first + offset);

} // ChartData::median

// ----------------------------------------------------------------------

std::ostream& operator << (std::ostream& out, const AntigenSerumData& aData)
{
    constexpr const int titer_width = 7;
    out << '[' << std::setw(titer_width) << std::right << aData.median.first << '(' << aData.median.second << ")]";
    for (const auto& element: aData.titer_per_table)
        out << std::setw(titer_width) << std::right << element.first << '(' << element.second << ')';
    return out;

} // operator <<

// ----------------------------------------------------------------------

std::ostream& operator << (std::ostream& out, const ChartData& aData)
{
    out << "Tables:" << aData.mTables.size() << " Sera:" << aData.mSera.size() << " Antigens:" << aData.mAntigens.size() << " Titers:" << aData.mTiters.size() << std::endl;
    out << "Titers: " << aData.mAllTiters; // << std::endl;
    // out << "Titers: " << aData.mTiterLevel << std::endl;

    // const int serum_field_size = static_cast<int>(aData.longest_serum_name()), antigen_field_size = static_cast<int>(aData.longest_antigen_name());

    // for (size_t antigen_no = 0; antigen_no < aData.number_of_antigens(); ++antigen_no) {
    //     for (size_t serum_no = 0; serum_no < aData.number_of_sera(); ++serum_no) {
    //         const auto& ag_sr_data = aData.mAntigenSerumData[antigen_no][serum_no];
    //         if (!ag_sr_data.empty()) {
    //             out << std::setw(serum_field_size) << std::left << aData.mSera[serum_no] << "  " << std::setw(antigen_field_size) << aData.mAntigens[antigen_no] << " " << ag_sr_data << std::endl;
    //         }
    //     }
    // }

    return out;

} // operator <<

// ======================================================================

void ChartData::plot(std::string output_filename)
{
    const size_t ns = number_of_enabled_sera(), na = number_of_enabled_antigens();
    std::cout << "Enabled: antigens: " << na << " sera: " << ns << std::endl;
    CellParameters cell_parameters{number_of_tables(), mTiterLevel.size()};
    const double title_height = cell_parameters.vstep * 0.5;

    const Viewport cell_viewport{Size{cell_parameters.hstep, cell_parameters.vstep}};

    PdfCairo surface(output_filename, ns * cell_parameters.hstep, na * cell_parameters.vstep + title_height, ns * cell_parameters.hstep);

    std::string title = mLab + " " + mVirusType + " " + mAssay + " " + mFirstDate + "-" + mLastDate + "  tables:" + std::to_string(number_of_tables()) + " sera:" + std::to_string(ns) + " antigens:" + std::to_string(na);
    text(surface, {title_height, title_height * 0.7}, title, black, NoRotation, title_height * 0.8, ns * cell_parameters.hstep - title_height * 2);

    for (size_t antigen_no = 0, enabled_antigen_no = 0; antigen_no < number_of_antigens(); ++antigen_no) {
        if (mAntigens[antigen_no].enabled) {
            for (size_t serum_no = 0, enabled_serum_no = 0; serum_no < number_of_sera(); ++serum_no) {
                if (mSera[serum_no].enabled) {
                    Surface& cell = surface.subsurface({enabled_serum_no * cell_parameters.hstep, enabled_antigen_no * cell_parameters.vstep + title_height}, Scaled{cell_parameters.hstep}, cell_viewport, true);
                      //plot_antigen_serum_cell(antigen_no, serum_no, cell, cell_parameters);
                    plot_antigen_serum_cell_with_fixed_titer_range(antigen_no, serum_no, cell, cell_parameters);
                    ++enabled_serum_no;
                }
            }
            ++enabled_antigen_no;
        }
    }

} // ChartData::plot

// ----------------------------------------------------------------------

void ChartData::plot_antigen_serum_cell(size_t antigen_no, size_t serum_no, Surface& aCell, const CellParameters& aParameters)
{
    const auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
    const size_t median_index = titer_index_in_sAllTiters(ag_sr_data.median.first);
    aCell.border(black, Pixels{0.2});
      // serum name
    text(aCell, {aParameters.cell_top_title_height * 1.2, aParameters.cell_top_title_height}, mSera[serum_no], black, NoRotation, aParameters.cell_top_title_height, (aParameters.hstep - aParameters.cell_top_title_height * 1.5));
      // antigen name
    text(aCell, {aParameters.cell_top_title_height, aParameters.vstep - aParameters.voffset_base}, mAntigens[antigen_no], black, Rotation{-M_PI_2}, aParameters.cell_top_title_height, (aParameters.vstep - aParameters.voffset_base * 1.5));
      // titer value marks
    for (const auto& element: mTiterLevel) {
        aCell.text({aParameters.hstep - aParameters.cell_top_title_height * 2, aParameters.cell_top_title_height + aParameters.voffset_base + element.second * aParameters.voffset_per_level + aParameters.voffset_per_level * 0.5}, element.first, black, Scaled{aParameters.cell_top_title_height / 2});
    }

    double table_no = 2;
    for (const auto& element: ag_sr_data.titer_per_table) {
        if (!element.first.is_dont_care()) { // do not draw dont-care titer
            const double symbol_top = aParameters.cell_top_title_height + aParameters.voffset_base + element.second * aParameters.voffset_per_level;
            const double symbol_bottom = symbol_top + aParameters.voffset_per_level;
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
                aCell.rectangle_filled({table_no - 0.5, symbol_top}, {1, aParameters.voffset_per_level}, transparent, Pixels{0}, symbol_color);
            }
        }
        ++table_no;
    }

} // ChartData::plot_antigen_serum_cell

// ----------------------------------------------------------------------

void ChartData::plot_antigen_serum_cell_with_fixed_titer_range(size_t antigen_no, size_t serum_no, Surface& aCell, const CellParameters& aParameters)
{
    const double logged_titer_step = (aParameters.vstep - aParameters.voffset_base - aParameters.cell_top_title_height) / mYAxisLabels.size();

    if (std::find(mSera[serum_no].homologous.begin(), mSera[serum_no].homologous.end(), antigen_no) != mSera[serum_no].homologous.end())
        aCell.background(homologous_background);     // homologous antigen and serum
    aCell.border(black, Pixels{0.2});
      // serum name
    text(aCell, {aParameters.cell_top_title_height * 1.2, aParameters.cell_top_title_height}, mSera[serum_no], black, NoRotation, aParameters.cell_top_title_height, (aParameters.hstep - aParameters.cell_top_title_height * 1.5));
      // antigen name
    text(aCell, {aParameters.cell_top_title_height, aParameters.vstep - aParameters.voffset_base}, mAntigens[antigen_no], black, Rotation{-M_PI_2}, aParameters.cell_top_title_height, (aParameters.vstep - aParameters.voffset_base * 1.5));

      // titer value marks
    const double titer_label_font_size = aParameters.cell_top_title_height * 0.7;
    size_t titer_label_vpos = 0;
    for (const auto& titer_label: mYAxisLabels) {
        aCell.text_right_aligned({aParameters.hstep - aParameters.cell_top_title_height * 0.2,
                          // aParameters.cell_top_title_height + aParameters.voffset_base + titer_label_vpos * logged_titer_step + logged_titer_step * 0.5},
                        aParameters.vstep - aParameters.voffset_base - titer_label_vpos * logged_titer_step - logged_titer_step * 0.5 + titer_label_font_size * 0.3},
                   titer_label, black, Scaled{titer_label_font_size});
        ++titer_label_vpos;
    }

    const auto& ag_sr_data = mAntigenSerumData[antigen_no][serum_no];
    const size_t median_index = titer_index_in_sAllTiters(ag_sr_data.median.first);
    double table_no = 2;
    for (const auto& element: ag_sr_data.titer_per_table) {
        if (!element.first.is_dont_care()) { // do not draw dont-care titer
            const double titer = element.first.similarity_with_thresholded();
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
