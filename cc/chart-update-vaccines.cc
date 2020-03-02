#include "acmacs-base/argv.hh"
#include "hidb-5/vaccines.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    description desc{*this, "Updates plot specification of the chart with the current vaccine data."};

    argument<str> input_chart{*this, arg_name{"input-chart-file"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.input_chart)};
        fmt::print("{}\n", chart.make_info());
        hidb::update_vaccines(chart);
        acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name());
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
