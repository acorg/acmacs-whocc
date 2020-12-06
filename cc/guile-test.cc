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
    template <typename Class> struct foreign_type
    {
        // foreign_type(const char* a_name) : name{a_name} {}

        // std::string name{};
        // SCM type{UNDEFINED};

        static inline void finalize(SCM obj)
        {
            delete static_cast<Class*>(scm_foreign_object_ref(obj, 0));
            scm_foreign_object_set_x(obj, 0, nullptr);
        }

        // template <typename... Arg> static inline SCM constructor(Arg&&... arg) { return scm_make_foreign_object_1(get().type, new Class(std::forward<Arg>(arg)...)); }
    };

    template <typename Method> struct MethodRef
    {
        Method method;
    };

    template <typename Method> MethodRef(Method) -> MethodRef<Method>;
    constexpr MethodRef mm {&acmacs::chart::ChartModify::make_name};

    class Chart : public foreign_type<acmacs::chart::ChartModify>
    {
      public:
        using Class = acmacs::chart::ChartModify;

        template <auto Method, size_t index> static SCM method(SCM obj, SCM rest) { return guile::to_scm(fmt::format("method-{}", index)); }

        // template <auto Method, size_t index> static SCM method_ptr() { return guile::subr(method<Method, index>); }

        static inline SCM symbol() { return guile::symbol("Chart"); }
        static inline SCM type() { return scm_primitive_eval(symbol()); }

        static inline auto lm = [](SCM obj) {
            return guile::to_scm("lm");
                    // scm_assert_foreign_object_type(type(), obj);
                    // auto* chart = static_cast<Class*>(scm_foreign_object_ref(obj, 0));
                    // return guile::to_scm(chart->make_name());
                };

        Chart() // : foreign_type("Chart")
        {
            auto type = scm_make_foreign_object_type(symbol(), scm_list_2(guile::symbol("ptr"), guile::symbol("methods")), finalize);
            scm_define(symbol(), type);
            scm_c_define_gsubr("Chart/new", 1, 0, 0, guile::subr(constructor));

            auto methods = scm_list_n(scm_c_define_gsubr("Chart/name", 1, 0, 1, reinterpret_cast<scm_t_subr>(&lm)), // guile::subr(&method<&Class::make_name, 1>))
                                            guile::UNDEFINED);

            // std::array methods{
            //     {
            //     "Chart/name", Class::make_name
            //     },
            //     {
            //         "Chart/number-of-antigens",
            //     },
            //     {
            //         "Chart/number-of-sera",
            //     },
            // };

            // scm_c_define_gsubr("Chart/name", 1, 0, 1, guile::subr(method));
        }

        static inline SCM constructor(SCM filename)
        {
            return scm_make_foreign_object_1(type(), new acmacs::chart::ChartModify(acmacs::chart::import_from_file(guile::from_scm<std::string>(filename))));
        }

        // static inline SCM method(SCM obj, SCM rest) {
        //     scm_assert_foreign_object_type(type(), obj);
        //     auto* chart = static_cast<Class*>(scm_foreign_object_ref(obj, 0));
        //     return guile::to_scm(chart->make_name());
        // }

    };
} // namespace guile

void guile_defines()
{
    static std::tuple types{
    guile::Chart{},
    };
}

// ======================================================================
// ======================================================================

// namespace guile
// {
//     template <typename Class> struct foreign_type;

//     using foreign_types = std::tuple<foreign_type<acmacs::chart::ChartModify>>;

//     extern foreign_types all_types;

//     template <typename Class> struct foreign_type
//     {
//         constexpr foreign_type() = default;
//         foreign_type(const char* a_name) : name{a_name}, type{scm_make_foreign_object_type(symbol(name), scm_list_1(symbol("ptr")), finalize)} { get() = *this; }

//         const char* name{nullptr};
//         SCM type{UNDEFINED};

//         static inline void finalize(SCM obj)
//         {
//             delete static_cast<Class*>(scm_foreign_object_ref(obj, 0));
//             scm_foreign_object_set_x(obj, 0, nullptr);
//         }

//         static inline foreign_type<Class>& get() { return std::get<foreign_type<Class>>(all_types); }

//         template <typename... Arg> static inline SCM make(Arg&&... arg) { return scm_make_foreign_object_1(get().type, new Class(std::forward<Arg>(arg)...)); }

//         template <typename... Args> auto& constructor()
//         {
//             scm_c_define_gsubr(name, sizeof...(Args), 0, 0, guile::subr(make<Args...>));
//             return *this;
//         }

//         // template <typename Lambda> static SCM make_method(SCM obj, SCM args) {}

//         static SCM method_impl(SCM obj, SCM args) { AD_DEBUG("method_impl args:{}", from_scm<size_t>(scm_length(args))); return obj; }

//         template <typename Method> auto& method(const char* method_name, Method meth)
//         {
//             const auto proc = scm_c_define_gsubr(method_name, 1, 0, 1, guile::subr(method_impl));
//             // scm_set_procedure_property_x(proc, symbol(""), guile::subr(meth));
//             // scm_set_procedure_property_x(proc, symbol("method"), guile::subr(meth));
//             return *this;
//         }

//         // // https://en.cppreference.com/w/cpp/language/template_parameters#Template_template_arguments
//         // template <template <typename...> typename Method, typename... Arg> auto& method(const char* method_name, Method<Arg...> meth) {
//         //     scm_c_define_gsubr(method_name, FunctionTraits<Method<Arg...>>::ArgCount + 1, 0, 0, guile::subr(make_method<Arg...>));
//         //     // return guile::to_scm(guile::foreign_object::get(chart_type, chart_obj)->make_name());

//         //     return *this; }
//     };

// } // namespace guile

// // ----------------------------------------------------------------------

// guile::foreign_types guile::all_types;

// ======================================================================

// namespace guile
// {
//     namespace foreign_object
//     {
//         template <typename Class> inline void finalize(SCM obj) { delete static_cast<Class*>(scm_foreign_object_ref(obj, 0)); }

//         template <typename Class> struct type_info
//         {
//             constexpr type_info() = default;
//             type_info(const char* a_name, SCM&& typ) : name{a_name},  type{std::move(typ)} {}

//             const char* name{nullptr};
//             SCM type{UNDEFINED};
//         };

//         template <typename Class> inline type_info<Class> make_type(const char* name) { return {name, scm_make_foreign_object_type(symbol(name), scm_list_1(symbol("ptr")), finalize<Class>)}; }

//         template <typename Class, typename... Arg> inline SCM make(type_info<Class> type, Arg&&... arg) { return scm_make_foreign_object_1(type.type, new Class(std::forward<Arg>(arg)...)); }

//         template <typename Class> inline Class* get(type_info<Class> type, SCM obj)
//         {
//             scm_assert_foreign_object_type(type.type, obj);
//             return static_cast<Class*>(scm_foreign_object_ref(obj, 0));
//         }

//         template <typename Class> class Base
//         {
//           public:
//             static void init(const char* name) { type_ = make_type<Class>(name); }
//             Base() = default;

//           private:
//             static SCM type_; // {UNDEFINED};
//         };

//     } // namespace foreign_object


// } // namespace guile

// ----------------------------------------------------------------------

// static SCM load_chart(SCM filename);
// static SCM chart_name(SCM chart);
// static guile::foreign_object::type_info<acmacs::chart::ChartModify> chart_type;

// void guile_defines()
// {
//     using namespace guile;
//     // using namespace guile::foreign_object;
//     // using namespace std::string_view_literals;

//     foreign_type<acmacs::chart::ChartModify>("Chart")         //
//         .constructor<std::shared_ptr<acmacs::chart::Chart>>() //
//         .method("Chart/name", &acmacs::chart::ChartModify::make_name);

//     // chart_type = make_type<acmacs::chart::ChartModify>("<Chart>");
//     // define("load-chart"sv, load_chart);
//     // define("chart-name"sv, chart_name);

// } // guile_defines

// ----------------------------------------------------------------------

// SCM load_chart(SCM filename)
// {
//     return guile::foreign_object::make(chart_type, acmacs::chart::import_from_file(guile::from_scm<std::string>(filename)));

// } // load_chart

// // ----------------------------------------------------------------------

// SCM chart_name(SCM chart_obj)
// {
//     return guile::to_scm(guile::foreign_object::get(chart_type, chart_obj)->make_name());

// } // chart_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
