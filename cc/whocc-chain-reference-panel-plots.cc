// For sera-antigen titres that exist in multiple reference panels, plot the titre on the y axis against table number on the x axis.
// Colors: Green is the median, yellow is 1 log from the median and red is >1 log from the median.

#include <string>
#include <iomanip>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "boost/program_options.hpp"
#pragma GCC diagnostic pop

#include "acmacs-base/stream.hh"
#include "acmacs-chart/ace.hh"

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

    std::pair<Titer,size_t> median;
    std::vector<std::pair<Titer, size_t>> titer_per_table;
    friend std::ostream& operator << (std::ostream& out, const AntigenSerumData& aData);
};

class ChartData
{
 public:
    using titer_iterator = std::vector<TiterData>::const_iterator;
    using range = std::pair<titer_iterator, titer_iterator>;

    inline ChartData() {}
    size_t add_antigen(const Antigen& aAntigen);
    size_t add_serum(const Serum& aSerum);
    size_t add_table(const Chart& aChart);
    inline void add_titer(size_t aAntigen, size_t aSerum, size_t aTable, const Titer& aTiter) { mTiters.emplace_back(aAntigen, aSerum, aTable, aTiter); mAllTiters.insert(aTiter); }

    void make_antigen_serum_data();

    inline size_t number_of_antigens() const { return mAntigens.size(); }
    inline size_t number_of_sera() const { return mSera.size(); }
    inline size_t number_of_tables() const { return mTables.size(); }
    inline std::string antigen(size_t antigen_no) const { return mAntigens[antigen_no]; }
    inline std::string serum(size_t serum_no) const { return mSera[serum_no]; }
    inline size_t longest_serum_name() const { return std::max_element(mSera.begin(), mSera.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    inline size_t longest_antigen_name() const { return std::max_element(mAntigens.begin(), mAntigens.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size(); }
    range find_range(size_t aSerum, size_t aAntigen) const;
    Titer median(const range& aRange) const;

 private:
    std::vector<std::string> mTables;
    std::vector<std::string> mSera;
    std::vector<std::string> mAntigens;
    std::vector<TiterData> mTiters;
    std::set<Titer> mAllTiters;
    std::map<Titer, size_t> mTiterLevel;
    std::vector<std::vector<AntigenSerumData>> mAntigenSerumData;

    friend std::ostream& operator << (std::ostream& out, const ChartData& aData);
    inline void sort_titers_by_serum_antigen() { std::sort(mTiters.begin(), mTiters.end()); }

};

// ----------------------------------------------------------------------

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
      // chart->find_homologous_antigen_for_sera();
    std::vector<size_t> ref_antigens;
    chart->antigens().reference_indices(ref_antigens);
    for (size_t antigen_index_in_chart: ref_antigens) {
        antigens[antigen_index_in_chart] = aData.add_antigen(chart->antigen(antigen_index_in_chart));
    }
    for (size_t serum_no = 0; serum_no < chart->sera().size(); ++serum_no) {
        const size_t serum_index_in_data = aData.add_serum(chart->serum(serum_no));
        for (const auto& antigen: antigens) {
            // std::cerr << serum_index_in_data << ' ' << aData.serum(serum_index_in_data) << " -- " << aData.antigen(antigen.second) << " -- " << chart->titers().get(antigen.first, serum_no) << std::endl;
            aData.add_titer(antigen.second, serum_index_in_data, table_no, chart->titers().get(antigen.first, serum_no));
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
        mAntigens.push_back(name);
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
        mSera.push_back(name);
        result = mSera.size() - 1;
    }
    return result;

} // ChartData::add_serum

// ----------------------------------------------------------------------

size_t ChartData::add_table(const Chart& aChart)
{
    mTables.push_back(aChart.chart_info().make_name());
    return mTables.size() - 1;

} // ChartData::add_table

// ----------------------------------------------------------------------

void ChartData::make_antigen_serum_data()
{
    sort_titers_by_serum_antigen();

    size_t level = 0;
    for (const auto& titer: mAllTiters)
        mTiterLevel[titer] = level++;

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

} // ChartData::make_antigen_serum_data

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
    return (titers.size() % 2) ? titers[titers.size() / 2] : titers[titers.size() / 2 - 1];

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
    out << "Titers: " << aData.mAllTiters << std::endl;
    // out << "Titers: " << aData.mTiterLevel << std::endl;

    const int serum_field_size = static_cast<int>(aData.longest_serum_name()), antigen_field_size = static_cast<int>(aData.longest_antigen_name());

    for (size_t antigen_no = 0; antigen_no < aData.number_of_antigens(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < aData.number_of_sera(); ++serum_no) {
            const auto& ag_sr_data = aData.mAntigenSerumData[antigen_no][serum_no];
            if (!ag_sr_data.empty()) {
                out << std::setw(serum_field_size) << std::left << aData.mSera[serum_no] << "  " << std::setw(antigen_field_size) << aData.mAntigens[antigen_no] << " " << ag_sr_data << std::endl;
            }
        }
    }
    return out;

} // operator <<

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
