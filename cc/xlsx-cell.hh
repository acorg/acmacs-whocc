#pragma once

// #include <variant>
#include "acmacs-base/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{
    namespace cell
    {
        class empty
        {
        };
    } // namespace cell

    using cell_t = std::variant<cell::empty, bool, std::string>;

    inline bool is_empty(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, acmacs::xlsx::cell::empty>)
                    return true;
                else
                    return false;
            },
            cell);
    }

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::xlsx::cell_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::xlsx::cell_t& value, FormatCtx& ctx)
    {
        std::visit(
            [&ctx]<typename Content>(const Content& arg) {
                if constexpr (std::is_same_v<Content, acmacs::xlsx::cell::empty>)
                    format_to(ctx.out(), "<empty>");
                else if constexpr (std::is_same_v<Content, bool>)
                    format_to(ctx.out(), "{}", arg);
                else if constexpr (std::is_same_v<Content, std::string>)
                    format_to(ctx.out(), "{}", arg);
                else
                    format_to(ctx.out(), "<*unknown*>");
            },
            value);
        return ctx.out();
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
