#pragma once

#include "acmacs-base/float.hh"
#include "acmacs-base/xlnt.hh"
#include "acmacs-whocc/xlsx-cell.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{
    namespace xlnt
    {
        class Doc;

        class Sheet
        {
          public:
            std::string name() const { return sheet_.title(); }
            size_t number_of_rows() const { return sheet_.highest_row(); }
            size_t number_of_columns() const { return sheet_.highest_column().index; }

            static inline date::year_month_day make_date(const ::xlnt::datetime& dt)
            {
                if (dt.hour || dt.minute || dt.second || dt.microsecond)
                    AD_WARNING("xlnt datetime contains time: {}", dt.to_string());
                return date::year{dt.year} / date::month{static_cast<unsigned>(dt.month)} / dt.day;
            }

            cell_t cell(size_t row, size_t col) const // row and col are zero based
            {
                const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(col + 1), static_cast<::xlnt::row_t>(row + 1)};
                if (!sheet_.has_cell(ref))
                    return cell::empty{};

                const auto cell = sheet_.cell(ref);
                switch (cell.data_type()) { // ~/AD/build/acmacs-build/build/xlnt/include/xlnt/cell/cell_type.hpp
                    case ::xlnt::cell_type::empty:
                        return cell::empty{};
                    case ::xlnt::cell_type::boolean:
                        return cell.value<bool>();
                    case ::xlnt::cell_type::inline_string:
                    case ::xlnt::cell_type::shared_string:
                    case ::xlnt::cell_type::formula_string:
                        if (const auto val = cell.value<std::string>(); !val.empty())
                            return val;
                        else
                            return cell::empty{};
                    case ::xlnt::cell_type::number:
                        if (cell.is_date())
                            return make_date(cell.value<::xlnt::datetime>());
                        else if (const auto vald = cell.value<double>(); !float_equal(vald, std::round(vald)))
                            return vald;
                        else
                            return static_cast<long>(cell.value<long long>());
                    case ::xlnt::cell_type::date:
                        return make_date(cell.value<::xlnt::datetime>());
                    case ::xlnt::cell_type::error:
                        return cell::error{};
                }
                return cell::empty{};
            }

          private:
            ::xlnt::worksheet sheet_;

            Sheet(::xlnt::worksheet&& src) : sheet_{std::move(src)} {}

            friend class Doc;
        };

        class Doc
        {
          public:
            size_t number_of_sheets() const { return workbook_.sheet_count(); }
            Sheet sheet(size_t sheet_no) { return workbook_.sheet_by_index(sheet_no); }

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
