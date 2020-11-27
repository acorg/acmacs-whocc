#include "acmacs-base/fmt.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wdeprecated-volatile"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
// #pragma GCC diagnostic ignored ""
// #pragma GCC diagnostic ignored ""
// #pragma GCC diagnostic ignored ""
// #pragma GCC diagnostic ignored ""
#endif

#ifdef __GNUG__
#endif

#include <libguile.h>

namespace guile
{
    struct Error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    const inline auto VOID = SCM_UNSPECIFIED;

    template <typename Func> constexpr inline auto subr(Func func) { return reinterpret_cast<scm_t_subr>(func); }

    inline void load(std::string_view filename) { scm_c_primitive_load(filename.data()); }

    inline void load(const std::vector<std::string_view>& filenames)
    {
        for (const auto& filename : filenames)
            scm_c_primitive_load(filename.data());
    }

    inline std::string to_string(SCM src)
    {
        // if (!scm_is_string(src))
        //     throw Error{fmt::format("expected string but passed {}", "?")};
        std::string result(scm_c_string_length(src), '?');
        scm_to_locale_stringbuf(src, result.data(), result.size());
        return result;
    }

    // https://devblogs.microsoft.com/oldnewthing/20200713-00/?p=103978
    template <typename F> struct FunctionTraits;
    template <typename R, typename... Args> struct FunctionTraits<R (*)(Args...)>
    {
        using Pointer = R (*)(Args...);
        using RetType = R;
        using ArgTypes = std::tuple<Args...>;
        static constexpr std::size_t ArgCount = sizeof...(Args);
        // template <std::size_t N> using NthArg = std::tuple_element_t<N, ArgTypes>;
    };

    template <typename Arg> inline SCM to_scm(Arg arg)
    {
        static_assert(std::is_same_v<std::decay<Arg>, int>, "no to_scm specialization defined");
        return scm_from_signed_integer(arg);
    }

    template <> inline SCM to_scm<double>(double arg) { return scm_from_double(arg); }
    inline SCM to_scm() { return VOID; }

    template <typename Value> inline Value from_scm(SCM arg)
    {
        static_assert(std::is_same_v<std::decay<Value>, int>, "no from_scm specialization defined");
        return scm_to_int(arg);
    }

    template <> inline double from_scm<double>(SCM arg) { return scm_to_double(arg); }
    template <> inline std::string from_scm<std::string>(SCM arg) { return to_string(arg); }

    template <typename Func> void define(std::string_view name, Func func)
    {
        scm_c_define_gsubr(name.data(), FunctionTraits<Func>::ArgCount, 0, 0, guile::subr(func));
    }

    // template <typename Func> void define_scm(std::string_view name, Func func)
    // {
    //     int num_args{0};
    //     if constexpr (std::is_invocable_v<Func>)
    //         num_args = 0;
    //     if constexpr (std::is_invocable_v<Func, SCM>)
    //         num_args = 1;
    //     else if constexpr (std::is_invocable_v<Func, SCM, SCM>)
    //         num_args = 2;
    //     else
    //         static_assert(std::is_invocable_v<Func, void>, "guile::define: unsupported function");
    //     scm_c_define_gsubr(name.data(), num_args, 0, 0, guile::subr(func));
    // }

    // initialize guile and call passed function to define functions
    template <typename Func> inline void init(Func func)
    {
        scm_init_guile();
        func();
    }

    template <typename Func> inline void init(const std::vector<std::string_view>& filenames_to_load, Func func)
    {
        init(func);
        load(filenames_to_load);
    }

} // namespace guile

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-whocc/log.hh"
#include "acmacs-whocc/sheet-to-torg.hh"
#include "acmacs-whocc/xlsx.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> output_dir{*this, 'o', desc{"output-dir"}};
    option<str> format{*this, 'f', "format", dflt{"{virus_type_lineage}-{assay_low_rbc}-{lab_low}-{table_date}"},
                       desc{"print assay information fields: {virus_type} {lineage} {virus_type_lineage} {virus_type_lineage_subset_short_low} {assay_full} {assay_low} "
                            "{assay_low_rbc} {lab} {lab_low} {rbc} {table_date}"}};
    option<bool> assay_information{*this, 'n', desc{"print assay information fields according to format (-f or --format)"}};
    option<str_array> scripts{*this, 's', desc{"run scheme script (multiple switches allowed) before processing files"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str_array> xlsx{*this, arg_name{".xlsx"}, mandatory};
};

static SCM name_antigen_serum_fix(SCM arg, SCM to);

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        guile::init(opt.scripts, [&] {
            using namespace guile;
            define("name-antigen-serum-fix"sv, name_antigen_serum_fix);
        });

        for (auto& xlsx : opt.xlsx) {
            auto doc = acmacs::xlsx::open(xlsx);
            for ([[maybe_unused]] auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
                auto converter = acmacs::sheet::SheetToTorg{doc.sheet(sheet_no)};
                converter.preprocess();
                if (converter.valid()) {
                    // AD_LOG(acmacs::log::xlsx, "Sheet {:2d} {}", sheet_no + 1, converter.name());
                    if (opt.assay_information) {
                        fmt::print("{}\n", converter.format_assay_data(opt.format));
                    }
                    else if (opt.output_dir) {
                        const auto filename = fmt::format("{}/{}.torg", opt.output_dir, converter.format_assay_data(opt.format));
                        AD_INFO("{}", filename);
                        acmacs::file::write(filename, converter.torg());
                    }
                    else {
                        fmt::print("\n{}\n\n", converter.torg());
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

SCM name_antigen_serum_fix(SCM from, SCM to)
{
    fmt::print("name_antigen_serum_fix \"{}\" -> \"{}\"\n", guile::from_scm<std::string>(from), guile::from_scm<std::string>(to));
    return guile::VOID;

} // name_antigen_serum_fix

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
