#include "acmacs-base/argv.hh"
// #include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "hidb-5/hidb-set.hh"
#include "hidb-5/hidb.hh"
#include "acmacs-whocc/log.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str_array> tables{*this, arg_name{".ace"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        for (const auto& filename : *opt.tables) {
            auto chart = acmacs::chart::import_from_file(filename);
            AD_INFO("{}", chart->make_name());
            const auto& hidb = hidb::get(chart->info()->virus_type(acmacs::chart::Info::Compute::Yes));
            auto tables = hidb.tables();

            auto antigens = chart->antigens();
            for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
                if (!antigen->distinct()) {
                    const auto hidb_antigens = hidb.antigens()->find(antigen->name(), hidb::fix_location::no);
                    if (!hidb_antigens.empty()) {
                        const auto full_name = antigen->full_name();
                        fmt::print("AG {:3d} \"{}\"\n", ag_no, full_name);
                        std::vector<std::pair<std::string, fmt::memory_buffer>> hidb_variants;
                        bool full_name_match_present{false};
                        for (const auto& [hidb_antigen, hidb_index] : hidb_antigens) {
                            auto hidb_antigen_full_name =
                                acmacs::string::join(acmacs::string::join_space, hidb_antigen->name(), acmacs::string::join(acmacs::string::join_space, hidb_antigen->annotations()),
                                                     hidb_antigen->reassortant(), hidb_antigen->passage());
                            full_name_match_present |= hidb_antigen_full_name == full_name;
                            auto& variant = hidb_variants.emplace_back(std::move(hidb_antigen_full_name), fmt::memory_buffer{});
                            const auto by_lab_assay = hidb.tables(*hidb_antigen, hidb::lab_assay_rbc_table_t::recent_first);
                            for (auto entry : by_lab_assay) {
                                fmt::format_to(variant.second, "        [{} {} {}] ({})", entry.lab, entry.assay, entry.rbc, entry.tables.size());
                                for (auto table : entry.tables)
                                    fmt::format_to(variant.second, " {}", table->date());
                                fmt::format_to(variant.second, "\n");
                            }
                        }
                        std::sort(std::begin(hidb_variants), std::end(hidb_variants), [&full_name](const auto& e1, const auto& e2) {
                            const auto f1 = e1.first == full_name, f2 = e2.first == full_name;
                            if (f1 && !f2)
                                return true;
                            else if (!f1 && f2)
                                return false;
                            else
                                return e1.first < e2.first;
                        });
                        if (!full_name_match_present)
                            hidb_variants.emplace(hidb_variants.begin(), "*** New ***", fmt::memory_buffer{});
                        for (const auto& variant : hidb_variants)
                            fmt::print("    {}\n{}", variant.first, fmt::to_string(variant.second));
                    }
                }
            }
        }
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
