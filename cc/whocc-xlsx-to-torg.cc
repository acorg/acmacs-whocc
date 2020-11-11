#include "acmacs-base/argv.hh"
#include "acmacs-base/openxlsx.hh"
#include "acmacs-base/range-v3.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>       output_dir{*this, 'o'};

    argument<str_array> xlsx{*this, arg_name{"sheet"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        for (auto& xlsx : opt.xlsx) {
            OpenXLSX::XLDocument doc{std::string{xlsx}};
            auto workbook = doc.workbook();
            fmt::print("sheets: {}\nworksheets: {}\n", workbook.sheetCount(), workbook.worksheetCount());
            for (auto sheet_no : range_from_1_to_including(static_cast<uint16_t>(workbook.sheetCount()))) {
                const auto sheet = workbook.sheet(sheet_no);
                fmt::print("{:02d} {}\n", sheet_no, sheet.name());
            }
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
