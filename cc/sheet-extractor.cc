#include "acmacs-base/range-v3.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-whocc/sheet-extractor.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_table_title_crick{R"(^Table [X0-9-]+\.\s*Antigenic analysis of influenza ([AB](?:\(H3N2\)|\(H1N1\)pdm09)?) viruses\s*-\s*(Plaque Reduction Neutralisation \(MDCK-SIAT\))?\s*\(?(20[0-2][0-9]-[01][0-9]-[0-3][0-9])\)?)", acmacs::regex::icase};

static const std::regex re_antigen_name{"^[AB]/[A-Z '_-]+/[^/]+/[0-9]+", acmacs::regex::icase};
static const std::regex re_antigen_passage{"^(MDCK|SIAT|E|HCK)[0-9X]", acmacs::regex::icase};

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

std::unique_ptr<acmacs::sheet::Extractor> acmacs::sheet::v1::extractor_factory(const Sheet& sheet)
{
    std::unique_ptr<Extractor> extractor;
    std::smatch match;
    if (sheet.matches(re_table_title_crick, match, 0, 0)) {
        if (acmacs::string::startswith_ignore_case(match.str(2), "Plaque")) {
            extractor = std::make_unique<ExtractorCrickPRN>(sheet);
        }
        else {
            extractor = std::make_unique<ExtractorCrick>(sheet);
            extractor->subtype(match.str(1));
        }
        extractor->date(date::from_string(match.str(3), date::allow_incomplete::no, date::throw_on_error::no));
    }
    else {
        AD_WARNING("no specific extractor found");
        extractor = std::make_unique<Extractor>(sheet);
    }
    extractor->preprocess();
    return extractor;

} // acmacs::sheet::v1::extractor_factory

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::preprocess()
{
    find_titers();
    find_antigen_name_column();
    find_antigen_date_column();
    find_antigen_passage_column();

} // acmacs::sheet::v1::Extractor::preprocess

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_titers()
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

} // acmacs::sheet::v1::Extractor::find_titers

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_name_column()
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

} // acmacs::sheet::v1::Extractor::find_antigen_name_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_date_column()
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

} // acmacs::sheet::v1::Extractor::find_antigen_date_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_passage_column()
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

} // acmacs::sheet::v1::Extractor::find_antigen_passage_column

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrick::ExtractorCrick(const Sheet& a_sheet)
    : Extractor(a_sheet)
{
    lab("CRICK");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrickPRN::ExtractorCrickPRN(const Sheet& a_sheet)
    : ExtractorCrick(a_sheet)
{
    assay("PRN");
    subtype("A(H3N2)");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
