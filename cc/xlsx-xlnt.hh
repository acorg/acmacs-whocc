#pragma once

#include "acmacs-base/float.hh"
#include "acmacs-base/xlnt.hh"
#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{
    namespace xlnt
    {
        class Doc;

        class Sheet : public acmacs::sheet::Sheet
        {
          public:
            Sheet(::xlnt::worksheet&& src) : sheet_{std::move(src)} {}

            std::string name() const override { return sheet_.title(); }
            size_t number_of_rows() const override { return sheet_.highest_row(); }
            size_t number_of_columns() const override { return sheet_.highest_column().index; }

            static inline date::year_month_day make_date(const ::xlnt::datetime& dt, size_t row, size_t col)
            {
                if (dt.hour || dt.minute || dt.second || dt.microsecond)
                    AD_WARNING("xlnt datetime at {:c}{} contains time: {}", col + 'A', row + 1, dt.to_string());
                return date::year{dt.year} / date::month{static_cast<unsigned>(dt.month)} / dt.day;
            }

            acmacs::sheet::cell_t cell(size_t row, size_t col) const override // row and col are zero based
            {
                const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(col + 1), static_cast<::xlnt::row_t>(row + 1)};
                if (!sheet_.has_cell(ref))
                    return acmacs::sheet::cell::empty{};

                const auto cell = sheet_.cell(ref);
                switch (cell.data_type()) { // ~/AD/build/acmacs-build/build/xlnt/include/xlnt/cell/cell_type.hpp
                    case ::xlnt::cell_type::empty:
                        return acmacs::sheet::cell::empty{};
                    case ::xlnt::cell_type::boolean:
                        return cell.value<bool>();
                    case ::xlnt::cell_type::inline_string:
                    case ::xlnt::cell_type::shared_string:
                    case ::xlnt::cell_type::formula_string:
                        if (const auto val = cell.value<std::string>(); !val.empty())
                            return val;
                        else
                            return acmacs::sheet::cell::empty{};
                    case ::xlnt::cell_type::number:
                        if (cell.is_date())
                            return make_date(cell.value<::xlnt::datetime>(), row, col);
                        else if (const auto vald = cell.value<double>(); !float_equal(vald, std::round(vald)))
                            return vald;
                        else
                            return static_cast<long>(cell.value<long long>());
                    case ::xlnt::cell_type::date:
                        return make_date(cell.value<::xlnt::datetime>(), row, col);
                    case ::xlnt::cell_type::error:
                        return acmacs::sheet::cell::error{};
                }
                return acmacs::sheet::cell::empty{};
            }

          private:
            ::xlnt::worksheet sheet_;
        };

        class Doc
        {
          public:
            size_t number_of_sheets() const { return workbook_.sheet_count(); }
            std::unique_ptr<Sheet> sheet(size_t sheet_no) { return std::make_unique<Sheet>(workbook_.sheet_by_index(sheet_no)); }

          protected:
            Doc(std::string_view filename) : workbook_{::xlnt::path{std::string{filename}}} {}

          private:
            ::xlnt::workbook workbook_;
        };

    } // namespace xlnt

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
