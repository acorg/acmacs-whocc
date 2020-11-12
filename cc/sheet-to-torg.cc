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
    std::vector<std::pair<size_t, range>> rows;
    for (const auto row : range_from_0_to(sheet().number_of_rows())) {
        if (auto titers = sheet().titer_range(row); !titers.empty()) {
            AD_DEBUG("row:{:2d} {:c}:{:c}", row + 1, titers.first + 'A', titers.second - 1 + 'A');
            rows.emplace_back(row, std::move(titers));
        }
    }
    ranges::sort(rows, [](const auto& e1, const auto& e2) { return e1.second.first < e2.second.first; });

} // acmacs::sheet::v1::SheetToTorg::find_titers

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
