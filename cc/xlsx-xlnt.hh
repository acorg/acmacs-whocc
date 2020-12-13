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
            sheet::nrow_t number_of_rows() const override { return sheet::nrow_t{sheet_.highest_row()}; }
            sheet::ncol_t number_of_columns() const override { return sheet::ncol_t{sheet_.highest_column().index}; }

            static inline date::year_month_day make_date(const ::xlnt::datetime& dt, sheet::nrow_t /*row*/, sheet::ncol_t /*col*/)
            {
                // if (dt.hour || dt.minute || dt.second || dt.microsecond)
                //     AD_WARNING("xlnt datetime at {:c}{} contains time: {}", col + 'A', row + 1, dt.to_string());
                return date::year{dt.year} / date::month{static_cast<unsigned>(dt.month)} / dt.day;
            }

            acmacs::sheet::cell_t cell(sheet::nrow_t row, sheet::ncol_t col) const override // row and col are zero based
            {
                const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(*col + 1), static_cast<::xlnt::row_t>(*row + 1)};
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

            // acmacs::sheet::cell_spans_t cell_spans(nrow_t row, ncol_t col) const override
            // {
            //     const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(col + 1), static_cast<::xlnt::row_t>(row + 1)};
            //     if (!sheet_.has_cell(ref))
            //         return {};
            //     acmacs::sheet::cell_spans_t spans;
            //     // const auto cell = sheet_.cell(ref);
            //     // if (const auto fill = cell.fill(); fill.type() == ::xlnt::fill_type::pattern) {
            //     //     const auto fill_pattern = fill.pattern_fill();

            //     //     const auto get_color = [](const auto& col) -> std::string {
            //     //         if (col.is_set() && col.get().type() == ::xlnt::color_type::rgb) {
            //     //             const auto rgb = col.get().rgb();
            //     //             return fmt::format("#{:02x}{:02x}{:02x}", rgb.red(), rgb.green(), rgb.blue());
            //     //         }
            //     //         else
            //     //             return {};
            //     //     };

            //     //     const std::string foreground = get_color(fill_pattern.foreground()), background = get_color(fill_pattern.background());
            //     //     if (!foreground.empty() || !background.empty())
            //     //         spans.push_back(acmacs::sheet::cell_span_t{0, 1, foreground, background});
            //     // }

            //     // size_t first{0};
            //     // for (const auto& run : cell.value<::xlnt::rich_text>().runs()) {
            //     //     if (run.second.is_set()) {
            //     //         std::string color_value;
            //     //         if (run.second.get().has_color()) {
            //     //             if (const auto color = run.second.get().color(); color.type() == ::xlnt::color_type::rgb)
            //     //                 color_value = color.rgb().hex_string();
            //     //         }
            //     //         spans.push_back(acmacs::sheet::cell_span_t{first, run.first.size(), color_value});
            //     //     }
            //     //     first += run.first.size();
            //     // }
            //     return spans;
            // }

          private:
            ::xlnt::worksheet sheet_;
        };

        class Doc
        {
          public:
            size_t number_of_sheets() const { return workbook_.sheet_count(); }
            std::shared_ptr<Sheet> sheet(size_t sheet_no) { return std::make_shared<Sheet>(workbook_.sheet_by_index(sheet_no)); }

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
