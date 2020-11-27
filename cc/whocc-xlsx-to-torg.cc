#include "acmacs-base/argv.hh"
#include "acmacs-base/guile.hh"
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
