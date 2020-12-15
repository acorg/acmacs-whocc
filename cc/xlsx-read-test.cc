#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/xlsx.hh"
#include "acmacs-whocc/log.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};

    argument<str_array> xlsx{*this, arg_name{".xlsx"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    using namespace acmacs;

    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        for (const auto& filename : *opt.xlsx) {
            auto doc = acmacs::xlsx::open(filename);
            AD_INFO("Sheets: {}", doc.number_of_sheets());
            for (const auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
                auto sheet = doc.sheet(sheet_no);
                AD_INFO("    {}: \"{}\" {}-{}", sheet_no + 1, sheet->name(), sheet->number_of_rows(), sheet->number_of_columns());
                for (acmacs::sheet::nrow_t row{0}; row < sheet->number_of_rows(); ++row) {
                    for (acmacs::sheet::ncol_t col{0}; col < sheet->number_of_columns(); ++col) {
                        const auto cell = fmt::format("{}", sheet->cell(row, col));
                        AD_LOG(acmacs::log::xlsx, "cell {}{}: \"{}\"", row, col, cell);
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
