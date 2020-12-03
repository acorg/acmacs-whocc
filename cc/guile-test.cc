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

namespace guile
{
    namespace foreign_object
    {
        template <typename Class> inline void finalize(SCM obj) { delete static_cast<Class*>(scm_foreign_object_ref(obj, 0)); }

        template <typename Class> struct type_info
        {
            constexpr type_info() = default;
            type_info(SCM&& typ) : type{std::move(typ)} {}
            SCM type{UNDEFINED};
        };

        template <typename Class> inline type_info<Class> make_type(const char* name) { return scm_make_foreign_object_type(symbol(name), scm_list_1(symbol("ptr")), finalize<Class>); }

        template <typename Class, typename... Arg> inline SCM make(type_info<Class> type, Arg&&... arg) { return scm_make_foreign_object_1(type.type, new Class(std::forward<Arg>(arg)...)); }

        template <typename Class> inline Class* get(type_info<Class> type, SCM obj)
        {
            scm_assert_foreign_object_type(type.type, obj);
            return static_cast<Class*>(scm_foreign_object_ref(obj, 0));
        }

    } // namespace foreign_object
} // namespace guile

// ----------------------------------------------------------------------

static SCM load_chart(SCM filename);
static SCM chart_name(SCM chart);
static guile::foreign_object::type_info<acmacs::chart::ChartModify> chart_type;

void guile_defines()
{
    using namespace guile;
    using namespace guile::foreign_object;
    using namespace std::string_view_literals;

    chart_type = make_type<acmacs::chart::ChartModify>("<Chart>");
    define("load-chart"sv, load_chart);
    define("chart-name"sv, chart_name);

} // guile_defines

// ----------------------------------------------------------------------

SCM load_chart(SCM filename)
{
    return guile::foreign_object::make(chart_type, acmacs::chart::import_from_file(guile::from_scm<std::string>(filename)));

} // load_chart

// ----------------------------------------------------------------------

SCM chart_name(SCM chart_obj)
{
    return guile::to_scm(guile::foreign_object::get(chart_type, chart_obj)->make_name());

} // chart_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
