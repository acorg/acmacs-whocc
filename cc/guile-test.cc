#include "acmacs-base/argv.hh"
#include "acmacs-base/guile.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-import.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> scripts{*this, 's', desc{"run scheme script (multiple switches allowed) before processing files"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str> to_eval{*this, arg_name{"<guile-expression>"}, mandatory};
};

static void guile_defines();

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);
        guile::init(guile_defines, *opt.scripts);
        scm_c_eval_string(opt.to_eval->data());
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

static SCM load_chart(SCM args);

void guile_defines()
{
    using namespace guile;
    using namespace std::string_view_literals;

    define("load-chart"sv, load_chart);
    // scm_c_define_gsubr("load-chart", 0, 0, 1, guile::subr(load_chart));

} // guile_defines

// ----------------------------------------------------------------------

SCM load_chart(SCM filename)
{
    auto chart = std::make_unique<acmacs::chart::ChartModify>(acmacs::chart::import_from_file(guile::from_scm<std::string>(filename)));
    fmt::print("loaded: {}\n", chart->make_name());
    return guile::VOID;

} // load_chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
