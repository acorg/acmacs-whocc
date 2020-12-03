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

static SCM load_chart(SCM filename);
static SCM chart_name(SCM chart);
static SCM chart_type;

struct Sample
{
    Sample(std::string_view a_name) : name(a_name) { AD_DEBUG("Sample({})", name); }
    ~Sample() { AD_DEBUG("~Sample[{}]", name); }
    std::string name;

    static inline void finalize(SCM sample_obj) { delete static_cast<Sample*>(scm_foreign_object_ref(sample_obj, 0)); }
};

void guile_defines()
{
    using namespace guile;
    using namespace std::string_view_literals;

    // scm_c_eval_string("(use-modules (oop goops))");

    chart_type = scm_make_foreign_object_type(scm_from_utf8_symbol("<Chart>"), scm_list_1(scm_from_utf8_symbol("data")), Sample::finalize);
    define("load-chart"sv, load_chart);
    define("chart-name"sv, chart_name);
    // scm_c_define_gsubr("load-chart", 0, 0, 1, guile::subr(load_chart));

} // guile_defines

// ----------------------------------------------------------------------

SCM load_chart(SCM filename)
{
    // auto chart = new acmacs::chart::ChartModify(acmacs::chart::import_from_file(guile::from_scm<std::string>(filename)));
    // fmt::print("loaded: {}\n", chart->make_name());
    // return scm_make_foreign_object_1(chart_type, chart);

    auto chart = new Sample {guile::from_scm<std::string>(filename)};
    return scm_make_foreign_object_1(chart_type, chart);

} // load_chart

// ----------------------------------------------------------------------

SCM chart_name(SCM chart_obj)
{
    scm_assert_foreign_object_type(chart_type, chart_obj);
    auto* chart = static_cast<Sample*>(scm_foreign_object_ref(chart_obj, 0));
    return guile::to_scm(chart->name);

} // chart_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
