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

    option<str>       output_dir{ *this, 'o',            desc{"output-dir"}};
    option<str> assay_data_format{*this, 'n',
                                  desc{"print assay information fields: {virus_type} {lineage} {virus_type_lineage} {virus_type_lineage_subset_short_low} {assay_full} {assay_low} "
                                       "{assay_low_rbc} {lab} {lab_low} {rbc} {table_date}"}};
    option<str_array> verbose{    *this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str_array> xlsx{*this, arg_name{".xlsx"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        for (auto& xlsx : opt.xlsx) {
            auto doc = acmacs::xlsx::open(xlsx);
            for ([[maybe_unused]] auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
                auto converter = acmacs::sheet::SheetToTorg{doc.sheet(sheet_no)};
                converter.preprocess();
                if (converter.valid()) {
                    // AD_LOG(acmacs::log::xlsx, "Sheet {:2d} {}", sheet_no + 1, converter.name());
                    if (opt.assay_data_format) {
                        fmt::print("{}\n", converter.format_assay_data(opt.assay_data_format));
                    }
                    else if (opt.output_dir) {
                        const auto filename = fmt::format("{}/{}.torg", opt.output_dir, converter.name());
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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
