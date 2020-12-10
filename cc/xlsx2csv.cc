#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/csv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/xlsx.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> xlsx{*this, arg_name{".xlsx"}, mandatory};
    argument<str> csv{*this, arg_name{".csv"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    using namespace acmacs;

    int exit_code = 0;
    try {
        Options opt(argc, argv);

        CsvWriter csv;
        auto doc = acmacs::xlsx::open(opt.xlsx);
        for (const auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
            if (sheet_no)
                csv << CsvWriter::end_of_row << CsvWriter::end_of_row;
            auto sheet = doc.sheet(sheet_no);
            csv << "Sheet" << sheet_no + 1 << sheet->name() << CsvWriter::end_of_row;
            csv << CsvWriter::empty_field;
            for (const auto col : range_from_0_to(sheet->number_of_columns()))
                csv << fmt::format("{:c}", col + 'A');
            csv << CsvWriter::end_of_row;
            for (const auto row : range_from_0_to(sheet->number_of_rows())) {
                csv << row + 1;
                for (const auto col : range_from_0_to(sheet->number_of_columns()))
                    csv << fmt::format("{}", sheet->cell(row, col));
                csv << CsvWriter::end_of_row;
            }
        }
        file::write(opt.csv, csv);
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
