#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/sheet.hh"
#include "acmacs-whocc/log.hh"

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::Sheet::matches(const std::regex& re, const cell_t& cell)
{
    return std::visit(
        [&re, &cell]<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, re);
            else
                return std::regex_search(fmt::format("{}", cell), re); // CDC id is a number in CDC tables, still we want to match
        },
        cell);

} // acmacs::sheet::v1::Sheet::matches

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::Sheet::matches(const std::regex& re, std::smatch& match, const cell_t& cell)
{
    return std::visit(
        [&re, &match]<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, match, re);
            else
                return false;
        },
        cell);

} // acmacs::sheet::v1::Sheet::matches

// ----------------------------------------------------------------------

size_t acmacs::sheet::v1::Sheet::size(const cell_t& cell) const
{
    return std::visit(
        []<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return arg.size();
            else
                return 0ul;
        },
        cell);

} // acmacs::sheet::v1::Sheet::size

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_titer{R"(^(<|[<>]?\s*[1-9][0-9]{0,5}|N[DA]|QNS|\*)$)", acmacs::regex::icase};

#include "acmacs-base/diagnostics-pop.hh"

bool acmacs::sheet::v1::Sheet::maybe_titer(const cell_t& cell) const
{
    return std::visit(
        []<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, re_titer);
            else if constexpr (std::is_same_v<Content, double> || std::is_same_v<Content, long>)
                return arg > 0;
            else
                return false;
        },
        cell);

} // acmacs::sheet::v1::Sheet::maybe_titer

// ----------------------------------------------------------------------

acmacs::sheet::v1::column_range acmacs::sheet::v1::Sheet::titer_range(nrow_t row) const
{
    range<ncol_t> longest;
    range<ncol_t> current;
    const auto update = [&longest, &current] {
        if (current.valid()) {
            if (!longest.valid() || longest.length() < current.length()) {
                longest = current;
                // AD_DEBUG("longest {}: {}", row, longest);
            }
            current = range<ncol_t>{};
        }
    };

    for (auto col = ncol_t{0}; col < number_of_columns(); ++col) {
        if (maybe_titer(row, col)) {
            if (!current.valid())
                current.first = col;
            current.second = col;
            // AD_DEBUG("titer {}{}: {} --> {}", row, col, cell(row, col), current);
        }
        else
            update();
    }
    update();
    return longest;

} // acmacs::sheet::v1::Sheet::titer_range

// ----------------------------------------------------------------------

std::vector<acmacs::sheet::cell_match_t> acmacs::sheet::v1::Sheet::grep(const std::regex& rex, const cell_addr_t& min, const cell_addr_t& max) const
{
    std::vector<cell_match_t> result;
    for (auto row = min.row; row < max.row; ++row) {
        for (auto col = min.col; col < max.col; ++col) {
            const auto cl = cell(row, col);
            // AD_DEBUG("Sheet::grep {} {} \"{}\"", row, col, cl);
            std::smatch match;
            if (matches(rex, match, cl)) {
                cell_match_t cm{.row = row, .col = col, .matches = std::vector<std::string>(match.size())};
                std::transform(std::cbegin(match), std::cend(match), std::begin(cm.matches), [](const auto& submatch) { return submatch.str(); });
                result.push_back(std::move(cm));
            }
        }
    }
    return result;

} // acmacs::sheet::v1::Sheet::grep

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
