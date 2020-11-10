#include "acmacs-base/fmt.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/openxlsx.hh"

// ======================================================================

int main()
{
    OpenXLSX::XLDocument doc{"/Users/eu/ac/tables-store/H3/NIMR/neut-orig-2018-2019/crick-h3-neut-20180111-20180208.xlsx"};
    auto workbook = doc.workbook();
    fmt::print("sheets: {}\nworksheets: {}\n", workbook.sheetCount(), workbook.worksheetCount());
    for (auto sheet_no : range_from_1_to_including(static_cast<uint16_t>(workbook.sheetCount()))) {
        const auto sheet = workbook.sheet(sheet_no);
        fmt::print("{:02d} {}\n", sheet_no, sheet.name());
    }
    return 0;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
