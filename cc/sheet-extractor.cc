#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-whocc/sheet-extractor.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_table_title_crick{R"(^Table\s+[XY0-9-]+\.\s*Antigenic analys[ie]s of influenza ([AB](?:\(H3N2\)|\(H1N1\)pdm09)?) viruses\s*-?\s*\(?(Plaque Reduction Neutralisation \(MDCK-SIAT\)|(?:Victoria|Yamagata)\s+lineage)?\)?\s*\(?(20[0-2][0-9]-[01][0-9]-[0-3][0-9])\)?)", acmacs::regex::icase};

static const std::regex re_antigen_name{"^[AB]/[A-Z '_-]+/[^/]+/[0-9]+", acmacs::regex::icase};
static const std::regex re_antigen_passage{"^(MDCK|SIAT|E|HCK)[0-9X]", acmacs::regex::icase};
static const std::regex re_serum_passage{"^(MDCK|SIAT|E|HCKCELL|EGG)", acmacs::regex::icase};

static const std::regex re_crick_serum_name_1{"^[AB]/[A-Z '_-]+$", acmacs::regex::icase};
static const std::regex re_crick_serum_name_2{"^[A-Z0-9-]+(/[0-9]+)?$", acmacs::regex::icase};
static const std::regex re_crick_serum_id{"^F[0-9][0-9]/[0-2][0-9]$", acmacs::regex::icase};

static const std::regex re_crick_prn_2fold{"^2-fold$", acmacs::regex::icase};
static const std::regex re_crick_prn_read{"^read$", acmacs::regex::icase};

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
            auto subtype = match.str(1);
            extractor->subtype(subtype);
            if (subtype == "B" && match.length(2) > 4) {
                switch (match.str(2)[0]) {
                    case 'V':
                    case 'v':
                        extractor->lineage("VICTORIA");
                        break;
                    case 'Y':
                    case 'y':
                        extractor->lineage("YAMAGATA");
                        break;
                }
            }
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

std::string acmacs::sheet::v1::Extractor::antigen_name(size_t ag_no) const
{
    if (antigen_name_column().has_value())
        return fmt::format("{}", sheet().cell(antigen_rows().at(ag_no), *antigen_name_column()));
    else
        return {};

} // acmacs::sheet::v1::Extractor::antigen_name

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::Extractor::antigen_date(size_t ag_no) const
{
    if (antigen_date_column().has_value())
        return fmt::format("{}", sheet().cell(antigen_rows().at(ag_no), *antigen_date_column()));
    else
        return {};

} // acmacs::sheet::v1::Extractor::antigen_date

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::Extractor::antigen_passage(size_t ag_no) const
{
    if (antigen_passage_column().has_value())
        return fmt::format("{}", sheet().cell(antigen_rows().at(ag_no), *antigen_passage_column()));
    else
        return {};

} // acmacs::sheet::v1::Extractor::antigen_passage

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::preprocess()
{
    find_titers();
    find_antigen_name_column();
    find_antigen_date_column();
    find_antigen_passage_column();
    find_serum_rows();

} // acmacs::sheet::v1::Extractor::preprocess

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_titers()
{
    std::vector<std::pair<size_t, range>> rows;
    range common;
    for (const auto row : range_from_0_to(sheet().number_of_rows())) {
        if (auto titers = sheet().titer_range(row); !titers.empty() && titers.first > 0) {
            // AD_DEBUG("row:{:2d} {:c}:{:c}", row + 1, titers.first + 'A', titers.second - 1 + 'A');
            if (!common.valid())
                common = titers;
            else if (common != titers)
                AD_WARNING("variable titer row ranges: {:c}:{:c} vs. {:c}:{:c}", titers.first + 'A', titers.second - 1 + 'A', common.first + 'A', common.second - 1 + 'A');
            rows.emplace_back(row, std::move(titers));
        }
    }

    serum_columns_.resize(common.size());
    ranges::copy(range_from_to(common), serum_columns_.begin());
    antigen_rows_.resize(rows.size());
    ranges::transform(rows, std::begin(antigen_rows_), [](const auto& row) { return row.first; });

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

    for (const auto col : range_from_0_to(serum_columns()[0])) { // to the left from titers
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_name](size_t row) { return is_name(row, col); })) > (antigen_rows_.size() / 2)) {
            antigen_name_column_ = col;
            break;
        }
    }

    if (antigen_name_column_.has_value()) {
        AD_INFO("Antigen name column: {:c}", *antigen_name_column_ + 'A');
        // remote antigen rows that have no name
        ranges::actions::remove_if(antigen_rows_, [this, is_name](size_t row) { return !is_name(row, *antigen_name_column_); });
    }
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

void acmacs::sheet::v1::Extractor::find_serum_passage_row(const std::regex& re)
{
    for (const auto row : range_from_to(1ul, antigen_rows()[0])) {
        if (static_cast<size_t>(ranges::count_if(serum_columns(), [row, this, re](size_t col) { return sheet().matches(re, row, col); })) >= (number_of_sera() / 2)) {
            serum_passage_row_ = row;
            break;
        }
    }

    if (serum_passage_row_.has_value())
        AD_INFO("[{}] Serum passage row: {}", lab(), *serum_passage_row_ + 1);
    else
        AD_WARNING("[{}] Serum passage row not found", lab());

} // acmacs::sheet::v1::Extractor::find_serum_passage_row

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_serum_id_row(const std::regex& re)
{
    AD_WARNING("[{}] Serum id row not found", lab());

} // acmacs::sheet::v1::Extractor::find_serum_id_row

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_serum_rows()
{

} // acmacs::sheet::v1::Extractor::find_serum_rows

// ======================================================================

acmacs::sheet::v1::ExtractorCrick::ExtractorCrick(const Sheet& a_sheet)
    : Extractor(a_sheet)
{
    lab("CRICK");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrick::find_serum_rows()
{
    find_serum_name_rows();
    find_serum_passage_row(re_serum_passage);
    find_serum_id_row(re_crick_serum_id);

} // acmacs::sheet::v1::ExtractorCrick::find_serum_rows


// ======================================================================

void acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows()
{
    fmt::memory_buffer report;
    for (const auto row : range_from_to(1ul, antigen_rows()[0])) {
        if (const size_t matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](size_t col) { return sheet().matches(re_crick_serum_name_1, row, col); })); matches == number_of_sera()) {
            serum_name_1_row_ = row;
            break;
        }
        else if (matches)
            fmt::format_to(report, "    re_crick_serum_name_1 row:{} matches:{}\n", row, matches);
    }

    if (serum_name_1_row_.has_value())
        AD_INFO("[Crick]: Serum name row 1: {}", *serum_name_1_row_ + 1);
    else
        AD_WARNING("[Crick]: No serum name row 1 found (number of sera: {})\n{}", number_of_sera(), fmt::to_string(report));

    if (serum_name_1_row_.has_value() && static_cast<size_t>(ranges::count_if(serum_columns(), [this](size_t col) { return sheet().matches(re_crick_serum_name_2, *serum_name_1_row_ + 1, col); })) == number_of_sera())
            serum_name_2_row_ = *serum_name_1_row_ + 1;
    else
        AD_DEBUG("re_crick_serum_name_2 {}: {}", *serum_name_1_row_ + 1, static_cast<size_t>(ranges::count_if(serum_columns(), [this](size_t col) { return sheet().matches(re_crick_serum_name_2, *serum_name_1_row_ + 1, col); })));

    if (serum_name_2_row_.has_value())
        AD_INFO("[Crick]: Serum name row 2: {}", *serum_name_2_row_ + 1);
    else
        AD_WARNING("[Crick]: No serum name row 2 found");

} // acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrickPRN::ExtractorCrickPRN(const Sheet& a_sheet)
    : ExtractorCrick(a_sheet)
{
    assay("PRN");
    subtype("A(H3N2)");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrickPRN::find_serum_rows()
{
    find_two_fold_read_row();
    ExtractorCrick::find_serum_rows();

} // acmacs::sheet::v1::ExtractorCrickPRN::find_serum_rows

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrickPRN::find_two_fold_read_row()
{
    for (const auto row : range_from_to(1ul, antigen_rows()[0])) {
        const size_t two_fold_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](size_t col) { return sheet().matches(re_crick_prn_2fold, row, col); }));
        const size_t read_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](size_t col) { return sheet().matches(re_crick_prn_read, row, col); }));
        if (two_fold_matches == (serum_columns().size() / 2) && two_fold_matches == read_matches) {
            two_fold_read_row_ = row;
            break;
        }
    }

    if (two_fold_read_row_.has_value()) {
        size_t col_no{0};
        ranges::actions::remove_if(serum_columns(), [&col_no](auto) { const auto remove = (col_no % 2) != 0; ++col_no; return remove; });
        AD_INFO("[Crick PRN]: 2-fold read row: {}", *two_fold_read_row_);
    }

} // acmacs::sheet::v1::ExtractorCrickPRN::find_two_fold_read_row

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
