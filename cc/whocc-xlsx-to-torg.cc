#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/xlsx.hh"

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
            auto doc = acmacs::xlsx::open(xlsx);
            for (auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
                auto sheet = doc.sheet(sheet_no);
                fmt::print("{:2d} \"{}\"  {}:{}\n", sheet_no, sheet.name(), sheet.number_of_rows(), sheet.number_of_columns());
                for (const auto row : range_from_0_to(sheet.number_of_rows())) {
                    for (const auto column : range_from_0_to(sheet.number_of_columns())) {
                        if (const auto cell = sheet.cell(row, column); !acmacs::xlsx::is_empty(cell))
                            fmt::print("    {:3d}:{:2d}  {}\n", row, column, cell);
                    }
                }
                fmt::print("\n\n");
                break;
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
