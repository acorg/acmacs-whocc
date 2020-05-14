#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "seqdb-3/seqdb.hh"
#include "hidb-5/hidb.hh"
#include "hidb-5/report.hh"

// ----------------------------------------------------------------------

struct AntigenData
{
    size_t no;
    const hidb::HiDb* hidb = nullptr;
    std::shared_ptr<acmacs::chart::Antigen> antigen_chart;
    std::shared_ptr<hidb::Antigen> antigen_hidb;
    acmacs::seqdb::ref antigen_seqdb;
};

template <> struct fmt::formatter<AntigenData> : fmt::formatter<acmacs::fmt_default_formatter> {
    template <typename FormatCtx> auto format(const AntigenData& ag, FormatCtx& ctx)
    {
        format_to(ctx.out(), "{} {}", ag.no, *ag.antigen_chart);
        if (ag.antigen_hidb)
            hidb::report_tables(std::cout, *ag.hidb, ag.antigen_hidb->tables(), hidb::report_tables::recent, "\n    ");
        if (ag.antigen_seqdb) {
            if (const auto& clades = ag.antigen_seqdb.seq().clades; !clades.empty())
                format_to(ctx.out(), "\n    clades: {}", clades);
        }
        return ctx.out();
    }
};

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    // option<str> db_dir{*this, "db-dir"};
    option<str> seqdb{*this, "seqdb"};

    option<bool>      sort_by_tables{*this, "sort-by-tables"};
    option<str>       clade{*this, "clade"};
    option<str>       aa{*this, "aa", desc{"report AA at given positions (comma separated)"}};
    option<bool>      verbose{*this, 'v', "verbose"};

    argument<str> chart_file{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::seqdb::setup(opt.seqdb);
        const auto report_aa_at_pos = opt.aa ? acmacs::seqdb::extract_pos1_list(opt.aa) : acmacs::seqdb::pos1_list_t{};

        auto chart = acmacs::chart::import_from_file(opt.chart_file);
        const auto virus_type = chart->info()->virus_type(acmacs::chart::Info::Compute::Yes);
        const auto& seqdb = acmacs::seqdb::get();
        const auto& hidb = hidb::get(virus_type); // , do_report_time(verbose));
        auto antigens_chart = chart->antigens();
        const auto hidb_antigens = hidb.antigens()->find(*antigens_chart);
        const auto seqdb_antigens = seqdb.match(*antigens_chart, virus_type);
        std::vector<AntigenData> antigens(antigens_chart->size());
        std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(antigens_chart->size()), antigens.begin(), [&](size_t index) -> AntigenData {
            return {index, &hidb, antigens_chart->at(index), hidb_antigens[index], seqdb_antigens[index]};
        });
        if (opt.clade)
            antigens.erase(std::remove_if(antigens.begin(), antigens.end(), [&seqdb, clade = *opt.clade](const auto& ag) -> bool { return !ag.antigen_seqdb || !ag.antigen_seqdb.has_clade(seqdb, clade); }),
                           antigens.end());
        std::cerr << "INFO: " << antigens.size() << " antigens upon filtering\n";
        if (opt.sort_by_tables) {
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
        for (const auto& ad : antigens) {
            fmt::print("{}\n", ad);
            if (!report_aa_at_pos.empty()) {
                const auto aa = ad.antigen_seqdb.seq().amino_acids.aligned();
                fmt::print("   ");
                for (auto pos : report_aa_at_pos) {
                    fmt::print(" {}", pos);
                    if (aa.size() >= pos)
                        fmt::print("{}", aa.at(pos));
                    else
                        fmt::print("?");
                }
                fmt::print("\n");
            }
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
