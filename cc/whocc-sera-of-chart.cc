#include <string>
#include <regex>
#include <optional>
#include <map>
#include <tuple>

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

enum class passage_t { egg, reassortant, cell };

struct SerumData
{
    const size_t sr_no;
    const std::string name;
    const std::string full_name;
    const acmacs::chart::Reassortant reassortant;
    const acmacs::chart::Passage passage;
    const passage_t passage_type;
    std::vector<hidb::TableStat> table_data;
    bool most_used = false;            // this name+passage-type+if-reassortant has max number of tables
    bool most_recent = false;          // this name+passage-type+if-reassortant is in more than one table and has most recent oldest table
};

static std::vector<SerumData> collect(const acmacs::chart::Chart& chart, std::optional<std::regex> name_match, const hidb::HiDb& hidb);
static passage_t passage_type(const acmacs::chart::Reassortant& reassortant, const acmacs::chart::Passage& passage);
static void find_most_used(std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc);
static bool match_assay(const hidb::TableStat& tables, std::string assay, std::string lab, std::string rbc);
static void report(const std::vector<SerumData>& serum_data);
static void report_for_serum_circles_json(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc);
static void report_for_serum_circles_html(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc);

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
        std::string assay = chart->info()->assay(acmacs::chart::Info::Compute::Yes);
        if (assay == "FOCUS REDUCTION")
            assay = "FR";
        else if (assay == "PLAQUE REDUCTION NEUTRALISATION")
            assay = "PRN";
        const auto lab = chart->info()->lab(acmacs::chart::Info::Compute::Yes);
        const auto rbc = chart->info()->rbc_species(acmacs::chart::Info::Compute::Yes);
        auto serum_data = collect(*chart, name_match, hidb::get(chart->info()->virus_type(acmacs::chart::Info::Compute::Yes), report_time::no));
        find_most_used(serum_data, assay, lab, rbc);
        if (opt.serum_circles) {
            report_for_serum_circles_json(serum_data, assay, lab, rbc);
            report_for_serum_circles_html(serum_data, assay, lab, rbc);
        }
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
                serum_data.push_back({sr_no, serum->name(), serum->full_name(), serum->reassortant(), serum->passage(), passage_type(serum->reassortant(), serum->passage()), hidb_tables->stat(hidb_serum->tables())});
            else
                std::cerr << "WARNING: not in hidb: " << serum->full_name_with_fields() << '\n';
        }
    }
    return serum_data;

} // collect

// ----------------------------------------------------------------------

passage_t passage_type(const acmacs::chart::Reassortant& reassortant, const acmacs::chart::Passage& passage)
{
    if (!reassortant.empty())
        return passage_t::reassortant;
    if (passage.is_egg())
        return passage_t::egg;
    return passage_t::cell;

} // passage_type

// ----------------------------------------------------------------------

void find_most_used(std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc)
{
    struct name_passage_t
    {
        std::string name;
        passage_t passage_type;
        bool operator<(const name_passage_t& rhs) const { return name == rhs.name ? passage_type < rhs.passage_type : name < rhs.name; }
    };
    struct most_t
    {
        size_t most_used_index;
        size_t most_used_number;
        size_t most_recent_index;
        std::string most_recent_oldest_date;
    };
    std::map<name_passage_t, most_t> index;

    for (const auto [no, entry] : acmacs::enumerate(serum_data)) {
        for (const auto& tables : entry.table_data) {
            if (match_assay(tables, assay, lab, rbc)) {
                if (const auto found = index.find({entry.name, entry.passage_type}); found != index.end()) {
                    if (tables.number > found->second.most_used_number) {
                        found->second.most_used_index = no;
                        found->second.most_used_number = tables.number;
                    }
                    if (std::string{tables.oldest->date()} > found->second.most_recent_oldest_date) {
                        found->second.most_recent_index = no;
                        found->second.most_recent_oldest_date = std::string{tables.oldest->date()};
                    }
                }
                else
                    index.emplace(name_passage_t{entry.name, entry.passage_type}, most_t{no, tables.number, no, std::string{tables.oldest->date()}});
            }
        }
    }

    for (auto [name_passage, most_entry] : index) {
        // std::cerr << "DEBUG: " << name_passage.name << ' ' << static_cast<int>(name_passage.passage_type) << " most-used:" << most_entry.most_used_index << "  most_recent:" << most_entry.most_recent_index << '\n';
        serum_data[most_entry.most_used_index].most_used = true;
        serum_data[most_entry.most_recent_index].most_recent = true;
    }

} // SerumData::find_most_used

// ----------------------------------------------------------------------

bool match_assay(const hidb::TableStat& tables, std::string assay, std::string lab, std::string rbc)
{
    return tables.assay == assay && tables.lab == lab && tables.rbc == rbc;

} // match_assay

// ----------------------------------------------------------------------

void report(const std::vector<SerumData>& serum_data)
{
    for (const auto& entry : serum_data) {
        std::cout << std::setw(3) << std::right << entry.sr_no << ' ' << entry.full_name << " P: " << entry.passage << '\n';
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

void report_for_serum_circles_json(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc)
{
    auto report = [&](const auto& entry, std::string color) {
        std::cout << R"({"N": "serum_circle", "serum": {"full_name": ")" << entry.full_name << R"("}, "report": true,)" << '\n'
                  << "  \"?passage\": \"" << entry.passage << "\",\n";
        for (const auto& tables : entry.table_data) {
            // std::cerr << "DEBUG: " << tables.assay << " --- " << assay << '\n';
            if (match_assay(tables, assay, lab, rbc)) {
                std::cout << "  \"?\": \"";
                if (entry.most_used)
                    std::cout << "**most-used** ";
                if (entry.most_recent)
                    std::cout << "**most-recent** ";
                std::cout << "tables:" << tables.number << " newest:" << tables.most_recent->date() << " oldest:" << tables.oldest->date() << "\",\n";
            }
        }
        const double outline_width = (entry.most_used || entry.most_recent) ? 6.0 : 2.0;
        const char* outline_dash = entry.most_recent ? R"(, "outline_dash": "dash1")" : "";
        std::cout << R"(  "empirical":   {"show": true,  "fill": "transparent", "outline": ")" << color << R"(", "outline_width": )" << outline_width << outline_dash << R"(},)" << '\n'
                  << R"(  "theoretical": {"show": false, "fill": "transparent", "outline": ")" << color << R"(", "outline_width": )" << outline_width << outline_dash << R"(},)" << '\n'
                  << R"(  "fallback":    {               "fill": "transparent", "outline": ")" << color << R"(", "outline_width": )" << outline_width << R"(, "outline_dash": "dash2", "radius": 3},)" << '\n'
                  << R"(  "mark_serum":    {"fill": ")" << color << R"(", "outline": "black", "order": "raise", "?label": {"name_type": "full", "offset": [0, 1], "color": "black", "size": 12}},)" << '\n'
                  << R"(  "?mark_antigen": {"fill": ")" << color << R"(", "outline": "black", "order": "raise", "?label": {"name_type": "full", "offset": [0, 1], "color": "black", "size": 12}})" << '\n'
                  << "},\n\n";
    };

    std::cout << "\"? ------ EGG ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::egg)
            report(entry, "${egg_color}");
    }

    std::cout << "\"? ------ Reassortant ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::reassortant)
            report(entry, "${reassortant_color}");
    }

    std::cout << "\"? ------ CELL ----------------------------------------------------------------------\",\n\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::cell)
            report(entry, "${cell_color}");
    }

} // report_for_serum_circles_json

// ----------------------------------------------------------------------

void report_for_serum_circles_html(const std::vector<SerumData>& serum_data, std::string assay, std::string lab, std::string rbc)
{
    auto report = [&](const auto& entry) {
        std::cout << "<div class='serum-name'>" << entry.sr_no << ' ' << entry.full_name << "</div>\n";
        for (const auto& tables : entry.table_data) {
            if (match_assay(tables, assay, lab, rbc)) {
                std::cout << "<div class='serum-tables" << (entry.most_used ? " most-used" : "") << (entry.most_recent ? " most-recent" : "") << "'><span class='number-of-tables'>tables:" << tables.number
                          << "</span> <span class='newest'>newest:" << tables.most_recent->date()
                          << "</span> <span class='oldest'>oldest:" << tables.oldest->date() << "</span></div>\n";
            }
        }
        std::cout << "<table>\n<tr><td>titer</td><td>radius</td><td>antigen</td></tr>\n";
        std::cout << "</table>\n";
    };

    std::cout << "<span class=\"passage-egg\">EGG</span><br>\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::egg)
            report(entry);
    }
    std::cout << "<br>\n";

    std::cout << "<span class=\"passage-reassortant\">Reassortant</span><br>\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::reassortant)
            report(entry);
    }
    std::cout << "<br>\n";

    std::cout << "<span class=\"passage-cell\">CELL</span><br>\n";
    for (const auto& entry : serum_data) {
        if (entry.passage_type == passage_t::cell)
            report(entry);
    }
    std::cout << "<br>\n";

} // report_for_serum_circles_html

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
