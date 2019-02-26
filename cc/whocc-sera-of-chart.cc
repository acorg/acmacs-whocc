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

    option<str> name{*this, "name", desc{"name (regex) to filter, matched against full_name"}};
    argument<str> chart{*this, mandatory};
};

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::optional<std::regex> name_match;
        if (opt.name.has_value())
            name_match = std::regex(opt.name->begin(), opt.name->end());
        auto chart = acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None, report_time::no);
        const auto& hidb = hidb::get(chart->info()->virus_type(acmacs::chart::Info::Compute::Yes), report_time::no);
        auto sera = chart->sera();
        const auto hidb_sera = hidb.sera()->find(*sera);
        auto hidb_tables = hidb.tables();
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            if (!name_match || std::regex_search(serum->full_name(), *name_match)) {
                std::cout << std::setw(3) << std::right << sr_no << ' ' << serum->full_name_with_passage() << '\n';
                if (auto hidb_serum = hidb_sera[sr_no]; hidb_serum) {
                    const auto stat = hidb_tables->stat(hidb_serum->tables());
                    for (const auto& entry : stat)
                        std::cout << "  " << entry.title() << "  tables:" << std::setw(3) << std::right << entry.number << " newest: " << std::setw(30) << std::left << entry.most_recent->name()
                                  << " oldest: " << entry.oldest->name() << '\n';
                }
                else
                    std::cerr << "WARNING: not in hidb: " << serum->full_name_with_fields() << '\n';
            }
        }
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
