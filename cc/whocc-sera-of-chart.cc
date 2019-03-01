#include <string>
#include <regex>
#include <optional>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "hidb-5/hidb.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  name{*this, "name", desc{"name (regex) to filter, matched against full_name"}};
    option<bool> serum_circles{*this, "serum-circles", desc{"output data for drawing serum circles"}};

    argument<str> chart{*this, mandatory};
};

struct SerumData
{
    const size_t sr_no;
    const std::string name;
    const acmacs::chart::Reassortant reassortant;
    const acmacs::chart::Passage passage;
    std::vector<hidb::TableStat> table_data;
};

static std::vector<SerumData> collect(const acmacs::chart::Chart& chart, std::optional<std::regex> name_match, const hidb::HiDb& hidb);
static void report(const std::vector<SerumData>& serum_data);
static void report_for_serum_circles(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc);

// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::optional<std::regex> name_match;
        if (opt.name.has_value())
            name_match = std::regex(opt.name->begin(), opt.name->end());
        auto chart = acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None, report_time::no);
        const auto serum_data = collect(*chart, name_match, hidb::get(chart->info()->virus_type(acmacs::chart::Info::Compute::Yes), report_time::no));
        if (opt.serum_circles)
            report_for_serum_circles(serum_data, chart->info()->assay(acmacs::chart::Info::Compute::Yes), chart->info()->lab(acmacs::chart::Info::Compute::Yes), chart->info()->rbc_species(acmacs::chart::Info::Compute::Yes));
        else
            report(serum_data);
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

std::vector<SerumData> collect(const acmacs::chart::Chart& chart, std::optional<std::regex> name_match, const hidb::HiDb& hidb)
{
    auto sera = chart.sera();
    const auto hidb_sera = hidb.sera()->find(*sera);
    auto hidb_tables = hidb.tables();
    std::vector<SerumData> serum_data;
    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        if (!name_match || std::regex_search(serum->full_name(), *name_match)) {
            if (auto hidb_serum = hidb_sera[sr_no]; hidb_serum)
                serum_data.push_back({sr_no, serum->full_name(), serum->reassortant(), serum->passage(), hidb_tables->stat(hidb_serum->tables())});
            else
                std::cerr << "WARNING: not in hidb: " << serum->full_name_with_fields() << '\n';
        }
    }
    return serum_data;

} // collect

// ----------------------------------------------------------------------

void report(const std::vector<SerumData>& serum_data)
{
    for (const auto& entry : serum_data) {
        std::cout << std::setw(3) << std::right << entry.sr_no << ' ' << entry.name << " P: " << entry.passage << '\n';
        for (const auto& tables : entry.table_data) {
            std::cout << "  " << tables.title() << "  tables:" << std::setw(2) << std::right << tables.number
                      << " newest:" << tables.most_recent->date()
                      << " oldest:" << tables.oldest->date()
                      << '\n';
                      // << " newest: " << std::setw(30) << std::left << tables.most_recent->name()
                      // << " oldest: " << tables.oldest->name() << '\n';
        }
    }

} // report

// ----------------------------------------------------------------------

void report_for_serum_circles(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc)
{
    if (assay == "FOCUS REDUCTION")
        assay = "FR";
    else if (assay == "PLAQUE REDUCTION NEUTRALISATION")
        assay = "PRN";

    auto report = [&](const auto& entry, std::string color) {
        std::cout << R"({"N": "serum_circle", "serum": {"full_name": ")" << entry.name << R"("}, "report": true,)" << '\n'
                  << "  \"?passage\": \"" << entry.passage << "\",\n";
        for (const auto& tables : entry.table_data) {
            // std::cerr << "DEBUG: " << tables.assay << " --- " << assay << '\n';
            if (tables.assay == assay)
                std::cout << "  \"?\": \"tables:" << tables.number << " newest:" << tables.most_recent->date() << " oldest:" << tables.oldest->date() << "\",\n";
        }
        std::cout << R"(  "empirical":   {"show": true,  "fill": "transparent", "outline": ")" << color << R"(", "outline_width": 2},)" << '\n'
                  << R"(  "theoretical": {"show": false, "fill": "transparent", "outline": ")" << color << R"(", "outline_width": 2},)" << '\n'
                  << R"(  "fallback":    {               "fill": "transparent", "outline": ")" << color << R"(", "outline_width": 2, "outline_dash": "dash2", "radius": 3},)" << '\n'
                  << R"(  "mark_serum":    {"fill": ")" << color << R"(", "outline": "black", "order": "raise", "?label": {"name_type": "full", "offset": [0, 1], "color": "black", "size": 12}},)" << '\n'
                  << R"(  "?mark_antigen": {"fill": ")" << color << R"(", "outline": "black", "order": "raise", "?label": {"name_type": "full", "offset": [0, 1], "color": "black", "size": 12}})" << '\n'
                  << "},\n\n";
    };

    std::cout << "\"? ------ EGG ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (entry.passage.is_egg() && entry.reassortant.empty())
            report(entry, "${egg_color}");
    }

    std::cout << "\"? ------ Reassortant ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (!entry.reassortant.empty())
            report(entry, "${reassortant_color}");
    }

    std::cout << "\"? ------ CELL ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (!entry.passage.is_egg())
            report(entry, "${cell_color}");
    }

} // report_for_serum_circles

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
