#pragma once

#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{
    namespace csv
    {
        class Sheet : public acmacs::sheet::Sheet
        {
          public:
            Sheet(std::string_view filename);

            sheet::nrow_t number_of_rows() const override { return sheet::nrow_t{data_.size()}; }
            sheet::ncol_t number_of_columns() const override { return number_of_columns_; }
            std::string name() const override { return {}; }
            acmacs::sheet::cell_t cell(sheet::nrow_t row, sheet::ncol_t col) const override { return data_.at(*row).at(*col); } // row and col are zero based

          private:
            std::vector<std::vector<acmacs::sheet::cell_t>> data_;
            sheet::ncol_t number_of_columns_{0};
        };

        class Doc
        {
          public:
            Doc(std::string_view filename) : sheet_{std::make_shared<Sheet>(filename)} {}

            size_t number_of_sheets() const { return 1; }
            std::shared_ptr<acmacs::sheet::Sheet> sheet(size_t /*sheet_no*/) { return sheet_; }

          private:
            std::shared_ptr<Sheet> sheet_;
        };

    } // namespace csv

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
