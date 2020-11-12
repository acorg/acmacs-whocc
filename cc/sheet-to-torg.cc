#include "acmacs-base/log.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/sheet-to-torg.hh"

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::preprocess()
{
    find_titers();

                // fmt::print("{:2d} \"{}\"  {}:{}\n", sheet_no, sheet.name(), sheet.number_of_rows(), sheet.number_of_columns());
                // for (const auto row : range_from_0_to(sheet.number_of_rows())) {
                //     for (const auto column : range_from_0_to(sheet.number_of_columns())) {
                //         if (const auto cell = sheet.cell(row, column); !acmacs::xlsx::is_empty(cell))
                //             fmt::print("    {:3d}:{:2d}  {}\n", row, column, cell);
                //     }
                // }
                // fmt::print("\n\n");

} // acmacs::sheet::v1::SheetToTorg::preprocess

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::find_titers()
{
    for (const auto row : range_from_0_to(sheet().number_of_rows())) {
        const auto titers = sheet().titer_range(row);
        if (!titers.empty())
            AD_DEBUG("row:{:2d} {:c}:{:c}", row + 1, titers.first + 'A', titers.second - 1 + 'A');
    }

} // acmacs::sheet::v1::SheetToTorg::find_titers

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
