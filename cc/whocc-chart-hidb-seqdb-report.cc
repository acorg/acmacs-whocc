#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "seqdb/seqdb.hh"
#include "hidb-5/hidb.hh"
#include "hidb-5/report.hh"

// ----------------------------------------------------------------------

struct AntigenData
{
    size_t no;
    const hidb::HiDb* hidb = nullptr;
    std::shared_ptr<acmacs::chart::Antigen> antigen_chart;
    std::shared_ptr<hidb::Antigen> antigen_hidb;
    seqdb::SeqdbEntrySeq antigen_seqdb;
};

inline std::ostream& operator<<(std::ostream& out, const AntigenData& ag)
{
    out << ag.no << ' ' << *ag.antigen_chart;
    if (ag.antigen_hidb)
        hidb::report_tables(out, *ag.hidb, ag.antigen_hidb->tables(), hidb::report_tables::recent, "\n    ");
    if (ag.antigen_seqdb) {
        if (const auto& clades = ag.antigen_seqdb.seq().clades(); !clades.empty())
            out << "\n    clades: " << clades;
    }
    return out;
}

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--sort-by-tables", false},
                           {"--clade", ""},
                           {"--db-dir", ""},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                           {"--verbose", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const bool verbose = args["-v"] || args["--verbose"];
            const bool sort_by_tables = args["--sort-by-tables"];
            const std::string clade = args["--clade"];

            seqdb::setup_dbs(args["--db-dir"], verbose ? seqdb::report::yes : seqdb::report::no);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report_time::No);
            const auto virus_type = chart->info()->virus_type(acmacs::chart::Info::Compute::Yes);
            const auto& seqdb = seqdb::get();
            const auto& hidb = hidb::get(virus_type, verbose ? report_time::Yes : report_time::No);
            auto antigens_chart = chart->antigens();
            const auto hidb_antigens = hidb.antigens()->find(*antigens_chart);
            const auto seqdb_antigens = seqdb.match(*antigens_chart, virus_type);
            std::vector<AntigenData> antigens(antigens_chart->size());
            std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(antigens_chart->size()), antigens.begin(), [&](size_t index) -> AntigenData {
                return {index, &hidb, antigens_chart->at(index), hidb_antigens[index], seqdb_antigens[index]};
                                                                                                                          });
            if (!clade.empty()) {
                antigens.erase(std::remove_if(antigens.begin(), antigens.end(), [&](const auto& ag) -> bool { return !ag.antigen_seqdb || !ag.antigen_seqdb.seq().has_clade(clade); }), antigens.end());
            }
            std::cerr << "INFO: " << antigens.size() << " antigens upon filtering\n";
            if (sort_by_tables) {
                auto hidb_tables = hidb.tables();
                std::sort(antigens.begin(), antigens.end(), [&](const auto& e1, const auto& e2) -> bool {
                                                                if (!e1.antigen_hidb)
                                                                    return false;
                                                                if (!e2.antigen_hidb)
                                                                    return true;
                                                                if (const auto nt1 = e1.antigen_hidb->number_of_tables(), nt2 = e2.antigen_hidb->number_of_tables(); nt1 == nt2) {
                                                                    auto mrt1 = hidb_tables->most_recent(e1.antigen_hidb->tables()), mrt2 = hidb_tables->most_recent(e2.antigen_hidb->tables());
                                                                    return mrt1->date() > mrt2->date();
                                                                }
                                                                else
                                                                    return nt1 > nt2;
                                                            });
            }
            for (const auto& ad : antigens)
                std::cout << ad << '\n';

        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
