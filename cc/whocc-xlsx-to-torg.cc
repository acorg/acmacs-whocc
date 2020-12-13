#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-whocc/log.hh"
#include "acmacs-whocc/sheet-to-torg.hh"
#include "acmacs-whocc/xlsx.hh"
#include "acmacs-whocc/data-fix.hh"

#define ACMACS_USE_PY

#if defined(ACMACS_USE_GUILE)
#include "acmacs-whocc/data-fix-guile.hh"
#elif defined(ACMACS_USE_PY)
#include "acmacs-whocc/whocc-xlsx-to-torg-py.hh"
#endif

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

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

#if defined(ACMACS_USE_GUILE)
        guile::init(acmacs::data_fix::guile_defines, *opt.scripts);
#elif defined(ACMACS_USE_PY)
        py::scoped_interpreter guard{};
        acmacs::whocc_xlsx::py_init(*opt.scripts);
#endif

        for (auto& xlsx : opt.xlsx) {
            try {
                auto doc = acmacs::xlsx::open(xlsx);
                for ([[maybe_unused]] auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
                    auto converter = acmacs::sheet::SheetToTorg{doc.sheet(sheet_no)};
                    converter.preprocess(opt.assay_information ? acmacs::sheet::Extractor::warn_if_not_found::no : acmacs::sheet::Extractor::warn_if_not_found::yes);
                    if (converter.valid()) {
                        // AD_LOG(acmacs::log::xlsx, "Sheet {:2d} {}", sheet_no + 1, converter.name());
                        if (opt.assay_information) {
                            fmt::print("{}\n", converter.format_assay_data(opt.format));
                        }
                        else if (opt.output_dir) {
                            converter.extractor().report_data_anchors();
                            const auto filename = fmt::format("{}/{}.torg", opt.output_dir, converter.format_assay_data(opt.format));
                            AD_INFO("{}", filename);
                            acmacs::file::write(filename, converter.torg());
                        }
                        else {
                            converter.extractor().report_data_anchors();
                            fmt::print("\n{}\n\n", converter.torg());
                        }
                    }
                }
            }
            catch (std::exception& err) {
                AD_ERROR("{}: {}", xlsx, err);
                exit_code = 3;
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
