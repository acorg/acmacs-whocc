#include "acmacs-base/argv.hh"
// #include "acmacs-base/range-v3.hh"
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
