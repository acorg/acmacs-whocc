#pragma once

#include <variant>
#include <limits>

#include "acmacs-base/fmt.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/regex.hh"
#include "acmacs-base/named-type.hh"

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

    inline bool is_string(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, std::string>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    // ----------------------------------------------------------------------

    // struct cell_span_t
    // {
    //     size_t first;
    //     size_t last;
    //     std::string foreground{};
    //     std::string background{};
    // };

    // using cell_spans_t = std::vector<cell_span_t>;

    // ----------------------------------------------------------------------

    constexpr const auto max_row_col = std::numeric_limits<size_t>::max();

    using nrow_t = named_size_t<struct nrow_t_tag>;
    using ncol_t = named_size_t<struct ncol_t_tag>;

    template <typename nrowcol> concept NRowCol = std::is_same_v<nrowcol, nrow_t> || std::is_same_v<nrowcol, ncol_t>;

    template <NRowCol nrowcol> constexpr bool valid(nrowcol row_col) { return row_col != nrowcol{max_row_col}; }

    struct cell_addr_t
    {
        nrow_t row{max_row_col};
        ncol_t col{max_row_col};
    };

    struct cell_match_t
    {
        nrow_t row{max_row_col};
        ncol_t col{max_row_col};
        std::vector<std::string> matches{}; // match groups starting with 0
    };


    template <NRowCol nrowcol> struct range : public std::pair<nrowcol, nrowcol>
    {
        range() : std::pair<nrowcol, nrowcol>{max_row_col, max_row_col} {}

        constexpr bool valid() const { return this->first != nrowcol{max_row_col} && this->first <= this->second; }
        constexpr bool empty() const { return !valid(); }
        constexpr nrowcol length() const { return valid() ? this->second - this->first + nrowcol{1} : nrowcol{0}; }
    };

    class Sheet
    {
      public:
        virtual ~Sheet() = default;

        virtual std::string name() const = 0;
        virtual nrow_t number_of_rows() const = 0;
        virtual ncol_t number_of_columns() const = 0;
        virtual cell_t cell(nrow_t row, ncol_t col) const = 0;                               // row and col are zero based
        // virtual cell_spans_t cell_spans(nrow_t /*row*/, ncol_t /*col*/) const { return {}; } // row and col are zero based

        static bool matches(const std::regex& re, const cell_t& cell);
        static bool matches(const std::regex& re, std::smatch& match, const cell_t& cell);
        bool matches(const std::regex& re, nrow_t row, ncol_t col) const { return matches(re, cell(row, col)); }
        bool is_date(nrow_t row, ncol_t col) const { return acmacs::sheet::is_date(cell(row, col)); }
        size_t size(const cell_t& cell) const;
        size_t size(nrow_t row, ncol_t col) const { return size(cell(row, col)); }

        bool maybe_titer(const cell_t& cell) const;
        bool maybe_titer(nrow_t row, ncol_t col) const { return maybe_titer(cell(row, col)); }
        range<ncol_t> titer_range(nrow_t row) const; // returns column range, returns empty range if not found

        cell_addr_t min_cell() const { return {nrow_t{0}, ncol_t{0}}; }
        cell_addr_t max_cell() const { return {number_of_rows(), number_of_columns()}; }

        std::vector<cell_match_t> grep(const std::regex& rex, const cell_addr_t& min, const cell_addr_t& max) const;
    };

} // namespace acmacs::sheet::inline v1

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

template <> struct fmt::formatter<acmacs::sheet::nrow_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(acmacs::sheet::nrow_t row, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", *row + 1);
    }
};

template <> struct fmt::formatter<acmacs::sheet::ncol_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(acmacs::sheet::ncol_t col, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{:c}", *col + 'A');
    }
};

template <> struct fmt::formatter<acmacs::sheet::cell_match_t> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::sheet::cell_match_t& match, FormatCtx& ctx) { return format_to(ctx.out(), "{}{}:{}", match.row, match.col, match.matches); }
};

template <acmacs::sheet::NRowCol nrowcol> struct fmt::formatter<acmacs::sheet::range<nrowcol>> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::sheet::range<nrowcol>& rng, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}:{}", rng.first, rng.second);
    }
};

template <acmacs::sheet::NRowCol nrowcol> struct fmt::formatter<std::optional<nrowcol>> : fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(std::optional<nrowcol> rowcol, FormatCtx& ctx)
    {
        if (rowcol.has_value())
            return format_to(ctx.out(), "{}", *rowcol);
        else
            return format_to(ctx.out(), "{}", "**no-value**");
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
