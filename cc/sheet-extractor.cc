#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-whocc/log.hh"
#include "acmacs-whocc/sheet-extractor.hh"
#include "acmacs-whocc/whocc-xlsx-to-torg-py.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

// static const std::regex re_ac_ignore_sheet{"^AC-IGNORE", acmacs::regex::icase};

// static const std::regex re_table_title_crick{R"(^Table\s+[XY0-9-]+\s*\.\s*Antigenic analys[ie]s of influenza ([AB](?:\(H3N2\)|\(H1N1\)pdm09)?)\s*viruses\s*-?\s*\(?(Plaque\s+Reduction\s+Neutralisation\s*\(MDCK-SIAT\)|(?:Victoria|Yamagata)\s+lineage)?\)?\s*\(?(20[0-2][0-9]-[01][0-9]-[0-3][0-9])\)?)", acmacs::regex::icase};

static const std::regex re_antigen_passage{"^(MDCK|SIAT|E|HCK)[0-9X]", acmacs::regex::icase};
static const std::regex re_serum_passage{"^(MDCK|SIAT|E|HCK|CELL|EGG)", acmacs::regex::icase};

static const std::regex re_crick_serum_name_1{"^([AB]/[A-Z '_-]+|NYMC\\s+X-[0-9]+[A-Z]*)$", acmacs::regex::icase};
static const std::regex re_crick_serum_name_2{"^[A-Z0-9-/]+$", acmacs::regex::icase};
static const std::regex re_crick_serum_id{"^F[0-9][0-9]/[0-2][0-9]$", acmacs::regex::icase};

static const std::regex re_crick_prn_2fold{"^2-fold$", acmacs::regex::icase};
static const std::regex re_crick_prn_read{"^read$", acmacs::regex::icase};

#include "acmacs-base/diagnostics-pop.hh"

static const std::string_view LineageVictoria{"VICTORIA"};
static const std::string_view LineageYamagata{"YAMAGATA"};

// ----------------------------------------------------------------------

std::unique_ptr<acmacs::sheet::Extractor> acmacs::sheet::v1::extractor_factory(std::shared_ptr<Sheet> sheet, Extractor::warn_if_not_found winf)
{
    const auto detected = acmacs::whocc_xlsx::v1::py_sheet_detect(sheet);
    try {
        std::unique_ptr<Extractor> extractor;
        if (detected.ignore) {
            AD_INFO("Sheet \"{}\": ignored on request in cell A1", sheet->name());
            return nullptr;
        }
        else if (detected.lab == "CRICK") {
            if (detected.assay == "HI") {
                extractor = std::make_unique<ExtractorCrick>(sheet);
                extractor->subtype(detected.subtype);
                extractor->lineage(detected.lineage);
            }
            else if (detected.assay == "PRN")
                extractor = std::make_unique<ExtractorCrickPRN>(sheet);
            else
                throw std::exception{};
        }
        else
            throw std::exception{};
        extractor->date(detected.date);
        extractor->preprocess(winf);
        return extractor;
    }
    catch (std::exception&) {
        throw std::runtime_error{fmt::format("Sheet \"{}\": no specific extractor found, detected: {}", sheet->name(), detected)};
    }

    // AD_DEBUG("detected ignore:{} lab:\"{}\" assay:\"{}\" subtype:\"{}\" lineage:\"{}\"", detected.ignore, detected.lab, detected.assay, detected.subtype, detected.lineage);

    // std::unique_ptr<Extractor> extractor;
    // std::smatch match;
    // if (const auto cell00 = sheet->cell(0, 0), cell01 = sheet->cell(0, 1); sheet->matches(re_ac_ignore_sheet, cell00)) {
    //     AD_INFO("Sheet \"{}\": ignored on request in cell A1", sheet->name());
    //     return nullptr;
    // }
    // else if (sheet->matches(re_table_title_crick, match, cell00) || sheet->matches(re_table_title_crick, match, cell01)) {
    //     // AD_DEBUG("Sheet: {}\ncell00: {}\ncell01: {}", sheet->name(), sheet->matches(re_table_title_crick, match, cell00), sheet->matches(re_table_title_crick, match, cell01));
    //     if (acmacs::string::startswith_ignore_case(match.str(2), "Plaque")) {
    //         extractor = std::make_unique<ExtractorCrickPRN>(sheet);
    //     }
    //     else {
    //         extractor = std::make_unique<ExtractorCrick>(sheet);
    //         auto subtype = match.str(1);
    //         extractor->subtype(subtype);
    //         if (subtype == "B" && match.length(2) > 4) {
    //             switch (match.str(2)[0]) {
    //                 case 'V':
    //                 case 'v':
    //                     extractor->lineage(LineageVictoria);
    //                     break;
    //                 case 'Y':
    //                 case 'y':
    //                     extractor->lineage(LineageYamagata);
    //                     break;
    //             }
    //         }
    //     }
    //     extractor->date(date::from_string(match.str(3), date::allow_incomplete::no, date::throw_on_error::no));
    // }
    // else {
    //     throw std::runtime_error{fmt::format("Sheet \"{}\": no specific extractor found", sheet->name())};
    //     // AD_WARNING("Sheet \"{}\": no specific extractor found", sheet->name());
    //     // extractor = std::make_unique<Extractor>(sheet);
    // }
    // extractor->preprocess(winf);
    // return extractor;

} // acmacs::sheet::v1::extractor_factory

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::Extractor::subtype_short() const
{
    if (subtype_ == "A(H1N1)")
        return "h1";
    if (subtype_ == "A(H3N2)")
        return "h3";
    if (subtype_ == "B") {
        if (lineage_ == LineageVictoria)
            return "bvic";
        if (lineage_ == LineageYamagata)
            return "byam";
        return "b";
    }
    return subtype_;

} // acmacs::sheet::v1::Extractor::subtype_short

// ----------------------------------------------------------------------

acmacs::sheet::v1::antigen_fields_t acmacs::sheet::v1::Extractor::antigen(size_t ag_no) const
{
    const auto make = [this, row = antigen_rows().at(ag_no)](std::optional<size_t> col) -> std::string {
        if (col.has_value()) {
            if (const auto cell = sheet().cell(row, *col); !is_empty(cell))
                return fmt::format("{}", cell);
        }
        return {};
    };

    return antigen_fields_t{
        .name = make(antigen_name_column()),       //
        .date = make(antigen_date_column()),       //
        .passage = make(antigen_passage_column()), //
        .lab_id = make(antigen_lab_id_column())    //
    };

} // acmacs::sheet::v1::Extractor::antigen

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::Extractor::serum(size_t sr_no) const
{
    const auto make = [this, col = serum_columns().at(sr_no)](std::optional<size_t> row) -> std::string {
        if (row.has_value()) {
            if (const auto cell = sheet().cell(*row, col); !is_empty(cell))
                return fmt::format("{}", cell);
        }
        return {};
    };

    return serum_fields_t{
        // .serum_name is set by lab specific extractor
        .passage = make(serum_passage_row()), //
        .serum_id = make(serum_id_row())      //
    };

} // acmacs::sheet::v1::Extractor::serum

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::preprocess(warn_if_not_found winf)
{
    find_titers(winf);
    find_antigen_name_column(winf);
    find_antigen_date_column(winf);
    find_antigen_passage_column(winf);
    find_antigen_lab_id_column(winf);
    find_serum_rows(winf);

} // acmacs::sheet::v1::Extractor::preprocess

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_titers(warn_if_not_found winf)
{
    std::vector<std::pair<size_t, range>> rows;
    // AD_DEBUG("Sheet {}", sheet().name());
    for (const auto row : range_from_0_to(sheet().number_of_rows())) {
        if (auto titers = sheet().titer_range(row); !titers.empty() && titers.first > 0)
            rows.emplace_back(row, std::move(titers));
    }

    if (!ranges::all_of(rows, [&rows](const auto& en) { return en.second == rows[0].second; })) {
        fmt::memory_buffer report; // fmt::format(rows, "{}", "\n  "));
        for (const auto& [row_no, rng] : rows)
            fmt::format_to(report, "    {}: {:c}:{:c} ({})\n", row_no + 1, rng.first + 'A', rng.second - 1 + 'A', rng.second - rng.first);
        if (winf == warn_if_not_found::yes)
            AD_WARNING_IF(winf == warn_if_not_found::yes, "sheet \"{}\": variable titer row ranges:\n{}", sheet().name(), fmt::to_string(report));
    }

    const auto common =  rows[0].second;
    serum_columns_.resize(common.size());
    ranges::copy(range_from_to(common), serum_columns_.begin());
    antigen_rows_.resize(rows.size());
    ranges::transform(rows, std::begin(antigen_rows_), [](const auto& row) { return row.first; });

} // acmacs::sheet::v1::Extractor::find_titers

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_name_column(warn_if_not_found winf)
{
    const auto is_name = [this](size_t row, size_t col) { return acmacs::virus::name::is_good(fmt::format("{}", sheet().cell(row, col))); };

    for (const auto col : range_from_0_to(serum_columns()[0])) { // to the left from titers
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_name](size_t row) { return is_name(row, col); })) > (antigen_rows_.size() / 2)) {
            antigen_name_column_ = col;
            break;
        }
    }

    if (antigen_name_column_.has_value()) {
        AD_LOG(acmacs::log::xlsx, "Antigen name column: {:c}", *antigen_name_column_ + 'A');
        // remote antigen rows that have no name
        ranges::actions::remove_if(antigen_rows_, [this, is_name, winf](size_t row) {
            const auto no_name = !is_name(row, *antigen_name_column_);
            if (no_name)
                AD_WARNING_IF(winf == warn_if_not_found::yes, "row {} has titers but no name: {}", row + 1, sheet().cell(row, *antigen_name_column_));
            return no_name;
        });
    }
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen name column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_name_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_date_column(warn_if_not_found winf)
{
    for (const auto col : range_from_0_to(sheet().number_of_columns())) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](size_t row) { return sheet().is_date(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_date_column_ = col;
            break;
        }
    }

    if (antigen_date_column_.has_value())
        AD_LOG(acmacs::log::xlsx, "Antigen date column: {:c}", *antigen_date_column_ + 'A');
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen date column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_date_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_passage_column(warn_if_not_found winf)
{
    const auto is_passage = [this](size_t row, size_t col) {
        return sheet().matches(re_antigen_passage, row, col);
    };

    for (const auto col : range_from_0_to(sheet().number_of_columns())) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_passage](size_t row) { return is_passage(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_passage_column_ = col;
            break;
        }
    }

    if (antigen_passage_column_.has_value())
        AD_LOG(acmacs::log::xlsx, "Antigen passage column: {:c}", *antigen_passage_column_ + 'A');
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen passage column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_passage_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_serum_passage_row(const std::regex& re, warn_if_not_found winf)
{
    for (const auto row : range_from_to(1ul, antigen_rows()[0])) {
        if (static_cast<size_t>(ranges::count_if(serum_columns(), [row, this, re](size_t col) { return sheet().matches(re, row, col); })) >= (number_of_sera() / 2)) {
            serum_passage_row_ = row;
            break;
        }
    }

    if (serum_passage_row_.has_value())
        AD_LOG(acmacs::log::xlsx, "[{}] Serum passage row: {}", lab(), *serum_passage_row_ + 1);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[{}] Serum passage row not found", lab());

} // acmacs::sheet::v1::Extractor::find_serum_passage_row

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_serum_id_row(const std::regex& re, warn_if_not_found winf)
{
    for (const auto row : range_from_to(1ul, antigen_rows()[0])) {
        if (static_cast<size_t>(ranges::count_if(serum_columns(), [row, this, re](size_t col) { return sheet().matches(re, row, col); })) >= (number_of_sera() / 2)) {
            serum_id_row_ = row;
            break;
        }
    }

    if (serum_id_row_.has_value())
        AD_LOG(acmacs::log::xlsx, "[{}] Serum id row: {}", lab(), *serum_id_row_ + 1);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[{}] Serum id row not found", lab());

} // acmacs::sheet::v1::Extractor::find_serum_id_row

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrick::ExtractorCrick(std::shared_ptr<Sheet> a_sheet)
    : Extractor(a_sheet)
{
    lab("CRICK");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrick::find_serum_rows(warn_if_not_found winf)
{
    find_serum_name_rows(winf);
    find_serum_passage_row(re_serum_passage, winf);
    find_serum_id_row(re_crick_serum_id, winf);

} // acmacs::sheet::v1::ExtractorCrick::find_serum_rows


// ======================================================================

void acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows(warn_if_not_found winf)
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
        AD_LOG(acmacs::log::xlsx, "[Crick]: Serum name row 1: {}", *serum_name_1_row_ + 1);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[Crick]: No serum name row 1 found (number of sera: {})\n{}", number_of_sera(), fmt::to_string(report));

    if (serum_name_1_row_.has_value() && static_cast<size_t>(ranges::count_if(serum_columns(), [this](size_t col) { return sheet().matches(re_crick_serum_name_2, *serum_name_1_row_ + 1, col); })) == number_of_sera())
            serum_name_2_row_ = *serum_name_1_row_ + 1;
    else
        AD_DEBUG("re_crick_serum_name_2 {}: {}", *serum_name_1_row_ + 1, static_cast<size_t>(ranges::count_if(serum_columns(), [this](size_t col) { return sheet().matches(re_crick_serum_name_2, *serum_name_1_row_ + 1, col); })));

    if (serum_name_2_row_.has_value())
        AD_LOG(acmacs::log::xlsx, "[Crick]: Serum name row 2: {}", *serum_name_2_row_ + 1);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[Crick]: No serum name row 2 found");

} // acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::ExtractorCrick::serum(size_t sr_no) const
{
    auto serum = Extractor::serum(sr_no);
    if (serum_name_1_row_ && serum_name_2_row_) {
        const auto n1{fmt::format("{}", sheet().cell(*serum_name_1_row_, serum_columns().at(sr_no)))}, n2{fmt::format("{}", sheet().cell(*serum_name_2_row_, serum_columns().at(sr_no)))};
        if (n1.size() > 2 && n1[1] == '/')
            serum.name = fmt::format("{}/{}", n1, n2);
        else
            serum.name = fmt::format("{} {}", n1, n2);
    }
    else
        serum.name = "*no serum_name_[12]_row_*";
    return serum;

} // acmacs::sheet::v1::ExtractorCrick::serum

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::ExtractorCrick::titer(size_t ag_no, size_t sr_no) const
{
    const auto cell = sheet().cell(antigen_rows().at(ag_no), serum_columns().at(sr_no));
    return std::visit(
        [&cell]<typename Content>(const Content& cont) -> std::string {
            if constexpr (std::is_same_v<Content, std::string>)
                return cont;
            else if constexpr (std::is_same_v<Content, long>)
                return fmt::format("{}", cont);
            else if constexpr (std::is_same_v<Content, double>)
                return fmt::format("{}", std::lround(cont));
            else
                return fmt::format("{}", cell);
        },
        cell);

} // acmacs::sheet::v1::ExtractorCrick::titer

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrickPRN::ExtractorCrickPRN(std::shared_ptr<Sheet> a_sheet)
    : ExtractorCrick(a_sheet)
{
    assay("PRN");
    subtype("A(H3N2)");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrickPRN::find_serum_rows(warn_if_not_found winf)
{
    find_two_fold_read_row();
    ExtractorCrick::find_serum_rows(winf);

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
        AD_LOG(acmacs::log::xlsx, "[Crick PRN]: 2-fold read row: {}", *two_fold_read_row_);
    }

} // acmacs::sheet::v1::ExtractorCrickPRN::find_two_fold_read_row

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::ExtractorCrickPRN::titer_comment() const
{
    if (two_fold_read_row_.has_value())
        return "<hi-like-titer> / <PRN read titer>";
    else
        return {};

} // acmacs::sheet::v1::ExtractorCrickPRN::titer_comment

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::ExtractorCrickPRN::titer(size_t ag_no, size_t sr_no) const
{
    using namespace std::string_view_literals;
    if (two_fold_read_row_.has_value()) {
        const auto left_col = serum_columns().at(sr_no);
        const auto two_fold_col = sheet().matches(re_crick_prn_2fold, *two_fold_read_row_, left_col) ? left_col : (left_col + 1);
        const auto read_col = two_fold_col == left_col ? (left_col + 1) : left_col;

        // interpretaion of < in the Crick PRN tables is not quite
        // clear, we just put < into togr and then converting it to
        // <10, <20, <40 when converting torg to ace

        const auto extract = [](const auto& cell) {
            return std::visit(
                [&cell]<typename Content>(const Content& cont) {
                    if constexpr (std::is_same_v<Content, std::string>)
                        return cont;
                    else if constexpr (std::is_same_v<Content, long>)
                        return fmt::format("{}", cont);
                    else if constexpr (std::is_same_v<Content, double>)
                        return fmt::format("{}", std::lround(cont));
                    else
                        return fmt::format("{}", cell);
                },
                cell);
        };

        return fmt::format("{}/{}", extract(sheet().cell(antigen_rows().at(ag_no), two_fold_col)), extract(sheet().cell(antigen_rows().at(ag_no), read_col)));
    }
    else
        return ExtractorCrick::titer(ag_no, sr_no);

} // acmacs::sheet::v1::ExtractorCrickPRN::titer

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
