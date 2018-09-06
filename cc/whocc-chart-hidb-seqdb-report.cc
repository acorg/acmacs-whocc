#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "seqdb/seqdb.hh"
#include "hidb-5/hidb.hh"
#include "hidb-5/report.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
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
            seqdb::setup_dbs(args["--db-dir"], verbose ? seqdb::report::yes : seqdb::report::no);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report_time::No);
            const auto virus_type = chart->info()->virus_type(acmacs::chart::Info::Compute::Yes);
            const auto& seqdb = seqdb::get();
            const auto& hidb = hidb::get(virus_type, verbose ? report_time::Yes : report_time::No);
            auto antigens = chart->antigens();
            const auto hidb_antigens = hidb.antigens()->find(*antigens);
            const auto seqdb_antigens = seqdb.match(*antigens, virus_type);
            for (size_t ag_no = 0; ag_no < antigens->size(); ++ag_no) {
                std::cout
                          // << std::setw(5) << std::right
                        << ag_no << ' ' << antigens->at(ag_no)->full_name() << '\n';
                if (const auto& hidb_entry = hidb_antigens[ag_no]; hidb_entry) {
                    hidb::report_antigen(hidb, *hidb_entry, true);
                }
                if (const auto& entry_seq = seqdb_antigens[ag_no]; entry_seq) {
                    if (const auto& clades = entry_seq.seq().clades(); !clades.empty())
                        std::cout << "    clades: " << clades << '\n';
                }
            }
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
