#include "acmacs-base/log.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/regex.hh"
#include "acmacs-whocc/sheet-to-torg.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_antigen_name{"^[AB]/[A-Z '_-]+/[^/]+/[0-9]+", acmacs::regex::icase};
static const std::regex re_antigen_passage{"^(MDCK|SIAT|E|HCK)[0-9X]", acmacs::regex::icase};

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::preprocess()
{
    find_titers();
    find_antigen_name_column();
    find_antigen_date_column();
    find_antigen_passage_column();

                // fmt::print("{:2d} \"{}\"  {}:{}\n", sheet_no, sheet.name(), sheet.number_of_rows(), sheet.number_of_columns());
                // for (const auto row : range_from_0_to(sheet.number_of_rows())) {
                //     for (const auto column : range_from_0_to(sheet.number_of_columns())) {
                //         if (const auto cell = sheet.cell(row, column); !acmacs::xlsx::is_empty(cell))
                //             fmt::print("    {:3d}:{:2d}  {}\n", row, column, cell);
                //     }
                // }
                // fmt::print("\n\n");

} // acmacs::sheet::v1::SheetToTorg::preprocess

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::find_titers()
{
    std::vector<std::pair<size_t, range>> rows;
    range common;
    for (const auto row : range_from_0_to(sheet().number_of_rows())) {
        if (auto titers = sheet().titer_range(row); !titers.empty()) {
            // AD_DEBUG("row:{:2d} {:c}:{:c}", row + 1, titers.first + 'A', titers.second - 1 + 'A');
            if (!common.valid())
                common = titers;
            else if (common != titers)
                AD_WARNING("variable titer row ranges: {:c}:{:c} vs. {:c}:{:c}", titers.first + 'A', titers.second - 1 + 'A', common.first + 'A', common.second - 1 + 'A');
            rows.emplace_back(row, std::move(titers));
        }
    }

    titer_columns_ = common;
    antigen_rows_.resize(rows.size());
    std::transform(std::begin(rows), std::end(rows), std::begin(antigen_rows_), [](const auto& row) { return row.first; });

} // acmacs::sheet::v1::SheetToTorg::find_titers

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::find_antigen_name_column()
{
    const auto is_name = [this](size_t row, size_t col) {
        if (sheet().matches(re_antigen_name, row, col)) {
            longest_antigen_name_ = std::max(longest_antigen_name_, sheet().size(row, col));
            return true;
        }
        else
            return false;
    };

    for (const auto col : range_from_0_to(titer_columns_.first)) { // to the left from titers
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_name](size_t row) { return is_name(row, col); })) == antigen_rows_.size()) {
            antigen_name_column_ = col;
            break;
        }
    }

    if (antigen_name_column_.has_value())
        AD_INFO("Antigen name column: {:c}", *antigen_name_column_ + 'A');
    else
        AD_WARNING("Antigen name column not found");

} // acmacs::sheet::v1::SheetToTorg::find_antigen_name_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::find_antigen_date_column()
{
    for (const auto col : range_from_0_to(sheet().number_of_columns())) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](size_t row) { return sheet().is_date(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_date_column_ = col;
            break;
        }
    }

    if (antigen_date_column_.has_value())
        AD_INFO("Antigen date column: {:c}", *antigen_date_column_ + 'A');
    else
        AD_WARNING("Antigen date column not found");

} // acmacs::sheet::v1::SheetToTorg::find_antigen_date_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::find_antigen_passage_column()
{
    const auto is_passage = [this](size_t row, size_t col) {
        if (sheet().matches(re_antigen_passage, row, col)) {
            longest_antigen_passage_ = std::max(longest_antigen_passage_, sheet().size(row, col));
            return true;
        }
        else
            return false;
    };

    for (const auto col : range_from_0_to(sheet().number_of_columns())) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_passage](size_t row) { return is_passage(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_passage_column_ = col;
            break;
        }
    }

    if (antigen_passage_column_.has_value())
        AD_INFO("Antigen passage column: {:c}", *antigen_passage_column_ + 'A');
    else
        AD_WARNING("Antigen passage column not found");

} // acmacs::sheet::v1::SheetToTorg::find_antigen_passage_column

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::SheetToTorg::torg() const
{
    fmt::memory_buffer result;
    fmt::format_to(result, "# -*- Org -*-\n\n");

    fmt::format_to(result, "|          | {:{}s} |       date | {:{}s} |", "name", longest_antigen_name_, "passage", longest_antigen_passage_);
    for ([[maybe_unused]] const auto col : ranges::views::iota(titer_columns_.first, titer_columns_.second))
        fmt::format_to(result, " |");
    fmt::format_to(result, "\n");

    for (const auto row : antigen_rows_) {
        fmt::format_to(result, "|          | {:{}s} | {} | {:{}s} |",                                      //
                       fmt::format("{}", sheet().cell(row, *antigen_name_column_)), longest_antigen_name_, //
                       sheet().cell(row, *antigen_date_column_),                                           //
                       fmt::format("{}", sheet().cell(row, *antigen_passage_column_)), longest_antigen_passage_);
        fmt::format_to(result, "\n");
    }
    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
