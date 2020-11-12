#include "acmacs-base/regex.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::Sheet::matches(const std::regex& re, const cell_t& cell) const
{
    return std::visit(
        [&re]<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, re);
            else
                return false;
        },
        cell);

} // acmacs::sheet::v1::Sheet::matches

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_titer{"^(<|[<>]?[0-9]+)$", acmacs::regex::icase};

#include "acmacs-base/diagnostics-pop.hh"

bool acmacs::sheet::v1::Sheet::maybe_titer(const cell_t& cell) const
{
    return std::visit(
        []<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, re_titer);
            else if constexpr (std::is_same_v<Content, double> || std::is_same_v<Content, long>)
                return true;
            else
                return false;
        },
        cell);

} // acmacs::sheet::v1::Sheet::maybe_titer

// ----------------------------------------------------------------------

acmacs::sheet::v1::range acmacs::sheet::v1::Sheet::titer_range(size_t row) const
{
    range longest;
    range current;
    const auto update = [&longest, &current] {
        if (current.valid() && (!longest.valid() || longest.size() < current.size())) {
            longest = current;
            current = range{};
        }
    };

    for (const auto col : range_from_0_to(number_of_columns()))
    {
        if (maybe_titer(row, col)) {
            if (!current.valid())
                current.first = col;
            current.second = col + 1;
        }
        else
            update();
    }
    update();
    return longest;

} // acmacs::sheet::v1::Sheet::titer_range

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
