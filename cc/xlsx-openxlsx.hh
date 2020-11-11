#pragma once

#include "acmacs-base/openxlsx.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{
    namespace openxlsx
    {
        class Doc;

        class Sheet
        {
          public:
            std::string name() const { return sheet_.name(); }

            size_t number_of_rows() const
            {
                size_t cells{0};
                fmt::print("lastCell {}\n", sheet_.lastCell().address());
                for (auto& cell : sheet_.range(OpenXLSX::XLCellReference{1, 1},  sheet_.lastCell())) {
                    fmt::print("cell {}:{}:{}\n", cell.cellReference().address(), cell.cellReference().row(), cell.cellReference().column());
                    ++cells;
                }
                return cells;
            }

            size_t number_of_columns() const { return static_cast<size_t>(sheet_.range().numColumns()); }

          private:
            mutable OpenXLSX::XLWorksheet sheet_;

            Sheet(OpenXLSX::XLWorksheet&& src) : sheet_{std::move(src)} {}

            friend class Doc;
        };

        class Doc
        {
          public:
            size_t number_of_sheets() const { return sheet_names_.size(); }
            Sheet sheet(size_t sheet_no) { return workbook_.worksheet(sheet_names_[sheet_no]); }

          protected:
            Doc(std::string_view filename) : doc_{std::string{filename}}, workbook_{doc_.workbook()}, sheet_names_{workbook_.worksheetNames()} {}

          private:
            OpenXLSX::XLDocument doc_;
            OpenXLSX::XLWorkbook workbook_;
            const std::vector<std::string> sheet_names_;
        };

    } // namespace openxlsx

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
