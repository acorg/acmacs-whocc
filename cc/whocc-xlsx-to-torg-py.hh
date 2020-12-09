#pragma once

#include "acmacs-base/pybind11.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/date.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{
    class Sheet;
}

namespace acmacs::whocc_xlsx::inline v1
{
    void py_init(const std::vector<std::string_view>& scripts);

    struct detect_result_t
    {
        bool ignore{false};
        std::string lab{};
        std::string assay{};
        std::string subtype{};
        std::string lineage{};
        date::year_month_day date{date::invalid_date()};
    };

    detect_result_t py_sheet_detect(std::shared_ptr<acmacs::sheet::Sheet> sheet);

} // namespace acmacs::whocc_xlsx::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::whocc_xlsx::detect_result_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::whocc_xlsx::detect_result_t& detected, FormatCtx& ctx)
    {
        if (detected.ignore)
            return format_to(ctx.out(), "[Sheet IGNORE]");
        return format_to(ctx.out(), "[{} {}{} {} {}]", detected.lab, detected.subtype, detected.lineage, detected.assay, detected.date);
        // return format_to(ctx.out(), "ignore:{} lab:\"{}\" assay:\"{}\" subtype:\"{}\" lineage:\"{}\" date:\"{}\"", //
        //                  detected.ignore, detected.lab, detected.assay, detected.subtype, detected.lineage, detected.date);
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
