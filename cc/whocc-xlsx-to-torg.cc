#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/sheet-to-torg.hh"
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
                auto converter = acmacs::sheet::SheetToTorg{doc.sheet(sheet_no)};
                converter.preprocess();
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
