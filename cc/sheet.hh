#pragma once

#include <variant>
#include <limits>

#include "acmacs-base/fmt.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/regex.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{

    namespace cell
    {
        class empty
        {
        };
        class error
        {
        };
    } // namespace cell

    using cell_t = std::variant<cell::empty, cell::error, bool, std::string, double, long, date::year_month_day>;

    inline bool is_empty(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, cell::empty>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    inline bool is_date(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, date::year_month_day>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    // ----------------------------------------------------------------------

    struct cell_span_t
    {
        size_t first;
        size_t last;
        std::string foreground{};
        std::string background{};
    };

    using cell_spans_t = std::vector<cell_span_t>;

    // ----------------------------------------------------------------------

    constexpr const auto max_row_col = std::numeric_limits<size_t>::max();

    struct cell_addr_t
    {
        size_t row{max_row_col};
        size_t col{max_row_col};
    };

    struct cell_match_t
    {
        size_t row{max_row_col};
        size_t col{max_row_col};
        std::vector<std::string> matches{}; // match groups starting with 0
    };


    struct range : public std::pair<size_t, size_t>
    {
        range() : std::pair<size_t, size_t>{max_row_col, max_row_col} {}

        constexpr bool valid() const { return first != max_row_col && first <= second; }
        constexpr bool empty() const { return !valid() || first == second; }
        constexpr size_t size() const { return valid() ? second - first : 0ul; }
    };

    class Sheet
    {
      public:
        virtual ~Sheet() = default;

        virtual std::string name() const = 0;
        virtual size_t number_of_rows() const = 0;
        virtual size_t number_of_columns() const = 0;
        virtual cell_t cell(size_t row, size_t col) const = 0;                               // row and col are zero based
        virtual cell_spans_t cell_spans(size_t /*row*/, size_t /*col*/) const { return {}; } // row and col are zero based

        static bool matches(const std::regex& re, const cell_t& cell);
        static bool matches(const std::regex& re, std::smatch& match, const cell_t& cell);
        bool matches(const std::regex& re, size_t row, size_t col) const { return matches(re, cell(row, col)); }
        bool is_date(size_t row, size_t col) const { return acmacs::sheet::is_date(cell(row, col)); }
        size_t size(const cell_t& cell) const;
        size_t size(size_t row, size_t col) const { return size(cell(row, col)); }

        bool maybe_titer(const cell_t& cell) const;
        bool maybe_titer(size_t row, size_t col) const { return maybe_titer(cell(row, col)); }
        range titer_range(size_t row) const; // returns column range, returns empty range if not found

        cell_addr_t min_cell() const { return {0, 0}; }
        cell_addr_t max_cell() const { return {number_of_rows(), number_of_columns()}; }

        std::vector<cell_match_t> grep(const std::regex& rex, const cell_addr_t& min, const cell_addr_t& max) const;
    };

} // namespace acmacs

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::sheet::cell_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::sheet::cell_t& cell, FormatCtx& ctx)
    {
        std::visit(
            [&ctx]<typename Content>(const Content& arg) {
                if constexpr (std::is_same_v<Content, acmacs::sheet::cell::empty>)
                    ; // format_to(ctx.out(), "<empty>");
                else if constexpr (std::is_same_v<Content, acmacs::sheet::cell::error>)
                    format_to(ctx.out(), "<error>");
                else if constexpr (std::is_same_v<Content, bool>)
                    format_to(ctx.out(), "{}", arg);
                else if constexpr (std::is_same_v<Content, std::string> || std::is_same_v<Content, double> || std::is_same_v<Content, long>)
                    format_to(ctx.out(), "{}", arg);
                else if constexpr (std::is_same_v<Content, date::year_month_day>)
                    format_to(ctx.out(), "{}", date::display(arg));
                else
                    format_to(ctx.out(), "<*unknown*>");
            },
            cell);
        return ctx.out();
    }
};

template <> struct fmt::formatter<acmacs::sheet::range> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::sheet::range& rng, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}:{}", rng.first + 1, rng.second + 1);
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
