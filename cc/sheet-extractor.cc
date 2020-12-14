#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/string-split.hh"
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


static const std::regex re_CDC_antigen_passage{R"(^((?:MDCK|SIAT|S|E|HCK|QMC|C)[0-9X][^\s\(]*)\s*(?:\(([\d/]+)\))?[A-Z]*$)", acmacs::regex::icase};
static const std::regex re_CDC_antigen_lab_id{"^[0-9]{10}$", acmacs::regex::icase};

static const std::regex re_CRICK_serum_name_1{"^([AB]/[A-Z '_-]+|NYMC\\s+X-[0-9]+[A-Z]*)$", acmacs::regex::icase};
static const std::regex re_CRICK_serum_name_2{"^[A-Z0-9-/]+$", acmacs::regex::icase};
static const std::regex re_CRICK_serum_id{"^F[0-9][0-9]/[0-2][0-9]$", acmacs::regex::icase};

static const std::regex re_CRICK_prn_2fold{"^2-fold$", acmacs::regex::icase};
static const std::regex re_CRICK_prn_read{"^read$", acmacs::regex::icase};


static const std::regex re_VIDRL_serum_name{"^([A-Z][A-Z ]+)([0-9]+)$", acmacs::regex::icase};
static const std::regex re_VIDRL_serum_id{"^[AF][0-9][0-9][0-9][0-9](?:-[0-9]+D)?$", acmacs::regex::icase};

static const std::regex re_human_who_serum{"(HUMAN|WHO|NORMAL)", acmacs::regex::icase};

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

        AD_INFO("{}", detected);
        if (detected.lab == "CDC") {
            extractor = std::make_unique<ExtractorCDC>(sheet);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->rbc(detected.rbc);
        }
        else if (detected.lab == "CRICK") {
            if (detected.assay == "HI") {
                extractor = std::make_unique<ExtractorCrick>(sheet);
                extractor->subtype(detected.subtype);
                extractor->lineage(detected.lineage);
                extractor->rbc(detected.rbc);
            }
            else if (detected.assay == "PRN")
                extractor = std::make_unique<ExtractorCrickPRN>(sheet);
            else
                throw std::exception{};
        }
        else if (detected.lab == "VIDRL") {
            extractor = std::make_unique<ExtractorVIDRL>(sheet);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->rbc(detected.rbc);
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

} // acmacs::sheet::v1::extractor_factory

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::Extractor::subtype_short() const
{
    if (subtype_ == "A(H1N1)")
        return "h1";
    if (subtype_ == "A(H1N1)PDM09")
        return "h1pdm";
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
    const auto make = [this, row = antigen_rows().at(ag_no)](std::optional<ncol_t> col) -> std::string {
        if (col.has_value()) {
            if (const auto cell = sheet().cell(row, *col); !is_empty(cell))
                return fmt::format("{}", cell);
        }
        return {};
    };

    return antigen_fields_t{
        .name = make(antigen_name_column()),       //
        .date = make(antigen_date_column()),       //
        .passage = make_passage(make(antigen_passage_column())), //
        .lab_id = make(antigen_lab_id_column())    //
    };

} // acmacs::sheet::v1::Extractor::antigen

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::preprocess(warn_if_not_found winf)
{
    find_titers(winf);
    find_antigen_name_column(winf);
    find_antigen_date_column(winf);
    find_antigen_passage_column(winf);
    find_antigen_lab_id_column(winf);
    find_serum_rows(winf);
    exclude_control_sera(winf); // remove human, WHO, pooled sera

} // acmacs::sheet::v1::Extractor::preprocess

// ----------------------------------------------------------------------

template <acmacs::sheet::NRowCol nrowcol> using number_ranges = std::vector<std::pair<nrowcol, nrowcol>>;

template <acmacs::sheet::NRowCol nrowcol> inline number_ranges<nrowcol> make_ranges(const std::vector<nrowcol>& numbers)
{
    number_ranges<nrowcol> rngs;
    for (const auto num : numbers) {
        if (rngs.empty() || num != (rngs.back().second + nrowcol{1}))
            rngs.emplace_back(num, num);
        else
            rngs.back().second = num;
    }
    return rngs;

} // make_ranges

template <acmacs::sheet::NRowCol nrowcol> inline std::string format(const number_ranges<nrowcol>& rngs)
{
    fmt::memory_buffer out;
    bool space{false};
    for (const auto& en : rngs) {
        if (space)
            format_to(out, " ");
        else
            space = true;
        format_to(out, "{}-{}", en.first, en.second);
    }
    return fmt::to_string(out);
}

// template <typename nrowcol> requires NRowCol<nrowcol> struct fmt::formatter<number_ranges<nrowcol>> : fmt::formatter<acmacs::fmt_helper::default_formatter>
// {
//     template <typename FormatCtx> auto format(const number_ranges<nrowcol>& rngs, FormatCtx& ctx)
//     {
//         std::string prefix;
//         for (const auto& en : rngs) {
//             format_to(ctx.out(), "{}", prefix);
//             format_to(ctx.out(), "{}-{}", en.first, en.second);
//             prefix = " ";
//         }
//         return ctx.out();
//     }
// };

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::report_data_anchors() const
{
    AD_INFO("Sheet Data Anchors:\n  Antigen columns:\n    Name:    {}\n    Date:    {}\n    Passage: {}\n    LabId:   {}\n  Antigen rows: {}\n\n" //
            "  Serum columns:   {}\n",                                                                                                         //
            antigen_name_column_, antigen_date_column_, antigen_passage_column_, antigen_lab_id_column_,                                     //
            format(make_ranges(antigen_rows_)),                                                                                              //
            format(make_ranges(serum_columns_)));

} // acmacs::sheet::v1::Extractor::report_data_anchors

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::Extractor::titer(size_t ag_no, size_t sr_no) const
{
    const auto cell = sheet().cell(antigen_rows().at(ag_no), serum_columns().at(sr_no));
    return std::visit(
        [&cell]<typename Content>(const Content& cont) -> std::string {
            if constexpr (std::is_same_v<Content, std::string>)
                return cont;
            else if constexpr (std::is_same_v<Content, long>)
                return fmt::format("{}", cont);
            else if constexpr (std::is_same_v<Content, double>)
                return fmt::format("{}", std::lround(cont)); // crick sometimes has real number titers
            else
                return fmt::format("{}", cell);
        },
        cell);

} // acmacs::sheet::v1::Extractor::titer

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_titers(warn_if_not_found winf)
{
    std::vector<std::pair<nrow_t, range<ncol_t>>> rows;
    // AD_DEBUG("Sheet {}", sheet().name());
    for (nrow_t row{0}; row < sheet().number_of_rows(); ++row) {
        if (auto titers = sheet().titer_range(row); !titers.empty() && titers.first > ncol_t{0})
            rows.emplace_back(row, std::move(titers));
    }

    if (!ranges::all_of(rows, [&rows](const auto& en) { return en.second == rows[0].second; })) {
        fmt::memory_buffer report; // fmt::format(rows, "{}", "\n  "));
        for (const auto& [row_no, rng] : rows)
            fmt::format_to(report, "    {}: {} ({})\n", row_no, rng, rng.second - rng.first);
        if (winf == warn_if_not_found::yes)
            AD_WARNING_IF(winf == warn_if_not_found::yes, "sheet \"{}\": variable titer row ranges:\n{}", sheet().name(), fmt::to_string(report));
    }

    for (ncol_t col{rows[0].second.first}; col < rows[0].second.second; ++col)
        serum_columns_.push_back(col);
    antigen_rows_.resize(rows.size());
    ranges::transform(rows, std::begin(antigen_rows_), [](const auto& row) { return row.first; });

} // acmacs::sheet::v1::Extractor::find_titers

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::Extractor::is_virus_name(nrow_t row, ncol_t col) const
{
    return acmacs::virus::name::is_good(fmt::format("{}", sheet().cell(row, col)));

} // acmacs::sheet::v1::Extractor::is_virus_name

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_name_column(warn_if_not_found winf)
{
    for (ncol_t col{0}; col < serum_columns()[0]; ++col) { // to the left from titers
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](nrow_t row) { return is_virus_name(row, col); })) > (antigen_rows_.size() / 2)) {
            antigen_name_column_ = col;
            break;
        }
    }

    if (antigen_name_column_.has_value())
        remove_redundant_antigen_rows(winf);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen name column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_name_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::remove_redundant_antigen_rows(warn_if_not_found winf)
{
    const auto cell_is_number_equal_to = [](const auto& cell, long num) {
        return std::visit(
            [num]<typename Content>(const Content& val) {
                if constexpr (std::is_same_v<Content, long>)
                    return val == num;
                else
                    return false;
            },
            cell);
    };

    // VIDRL has row with serum indexes
    const auto are_titers_increasing_numers = [this, cell_is_number_equal_to](nrow_t row) {
        long num{1};
        for (const auto col : serum_columns_) {
            if (!cell_is_number_equal_to(sheet().cell(row, col), num))
                return false;
            ++num;
        }
        return true;
    };

    if (antigen_name_column_.has_value()) {
        AD_LOG(acmacs::log::xlsx, "Antigen name column: {}", *antigen_name_column_);
        // remote antigen rows that have no name
        ranges::actions::remove_if(antigen_rows_, [this, are_titers_increasing_numers, winf](nrow_t row) {
            const auto no_name = !is_virus_name(row, *antigen_name_column_);
            if (no_name && !are_titers_increasing_numers(row))
                AD_WARNING_IF(winf == warn_if_not_found::yes, "row {} has titers but no name: {}", row, sheet().cell(row, *antigen_name_column_));
            return no_name;
        });
    }
    else
        AD_WARNING("Extractor::remove_redundant_antigen_rows: no antigen_name_column");

} // acmacs::sheet::v1::Extractor::remove_redundant_antigen_rows

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_date_column(warn_if_not_found winf)
{
    const auto is_date = [](const auto& cell) {
        // VIDRL uses string values DD/MM/YYYY for antigen dates
        return acmacs::sheet::is_date(cell) || (acmacs::sheet::is_string(cell) && date::from_string(fmt::format("{}", cell), date::allow_incomplete::no, date::throw_on_error::no).ok());
    };

    for (ncol_t col{0}; col < sheet().number_of_columns(); ++col) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, is_date, this](nrow_t row) { return is_date(sheet().cell(row, col)); })) >= (antigen_rows_.size() / 2)) {
            antigen_date_column_ = col;
            break;
        }
    }

    if (antigen_date_column_.has_value())
        AD_LOG(acmacs::log::xlsx, "Antigen date column: {}", *antigen_date_column_);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen date column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_date_column

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::Extractor::is_passage(nrow_t row, ncol_t col) const
{
    return sheet().matches(re_antigen_passage, row, col);

} // acmacs::sheet::v1::Extractor::is_passage

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_passage_column(warn_if_not_found winf)
{
    for (ncol_t col{0}; col < sheet().number_of_columns(); ++col) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](nrow_t row) { return is_passage(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_passage_column_ = col;
            break;
        }
    }

    if (antigen_passage_column_.has_value())
        AD_LOG(acmacs::log::xlsx, "Antigen passage column: {}", *antigen_passage_column_);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen passage column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_passage_column

// ----------------------------------------------------------------------

void acmacs::sheet::v1::Extractor::find_antigen_lab_id_column(warn_if_not_found winf)
{
    for (ncol_t col{*antigen_name_column_ + ncol_t{1}}; col < ncol_t{6}; ++col) {
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](nrow_t row) { return is_lab_id(row, col); })) >= (antigen_rows_.size() / 2)) {
            antigen_lab_id_column_ = col;
            break;
        }
    }

    if (antigen_lab_id_column_.has_value())
        AD_LOG(acmacs::log::xlsx, "Antigen lab_id column: {}", *antigen_lab_id_column_);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "Antigen lab_id column not found");

} // acmacs::sheet::v1::Extractor::find_antigen_lab_id_column

// ----------------------------------------------------------------------

std::optional<acmacs::sheet::v1::nrow_t> acmacs::sheet::v1::Extractor::find_serum_row(const std::regex& re, std::string_view row_name, warn_if_not_found winf) const
{
    std::optional<nrow_t> found;
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        if (static_cast<size_t>(ranges::count_if(serum_columns(), [row, this, re](ncol_t col) { return sheet().matches(re, row, col); })) >= (number_of_sera() / 2)) {
            found = row;
            break;
        }
    }

    if (found.has_value())
        AD_LOG(acmacs::log::xlsx, "[{}] Serum {} row: {}", lab(), row_name, *found);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[{}] Serum {} row not found", lab(), row_name);
    return found;

} // acmacs::sheet::v1::Extractor::find_serum_row

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCDC::ExtractorCDC(std::shared_ptr<Sheet> a_sheet)
    : Extractor(a_sheet)
{
    lab("CDC");

} // acmacs::sheet::v1::ExtractorCDC::ExtractorCDC

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::ExtractorCDC::is_passage(nrow_t row, ncol_t col) const
{
    // AD_DEBUG("CDC passage {}{} {} -> {}", row, col, sheet().cell(row, col), sheet().matches(re_CDC_antigen_passage, row, col));
    return sheet().matches(re_CDC_antigen_passage, row, col);

} // acmacs::sheet::v1::ExtractorCDC::is_passage

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::ExtractorCDC::make_passage(const std::string& src) const
{
    std::smatch match;
    if (std::regex_match(src, match, re_CDC_antigen_passage)) {
        if (match.length(2))
            return fmt::format("{} ({})", match.str(1), date::from_string(match.str(2), date::allow_incomplete::no, date::throw_on_error::yes, date::month_first::yes));
        else
            return match.str(1);
    }
    else {
        if (!src.empty())
            AD_WARNING("[CDC] unrecognized passage: \"{}\"", src);
        return src;
    }

} // acmacs::sheet::v1::ExtractorCDC::make_passage

// ----------------------------------------------------------------------

bool acmacs::sheet::v1::ExtractorCDC::is_lab_id(nrow_t row, ncol_t col) const
{
    return sheet().matches(re_CDC_antigen_lab_id, row, col);

} // acmacs::sheet::v1::ExtractorCDC::is_lab_id

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::ExtractorCDC::serum(size_t sr_no) const
{
    return serum_fields_t{
        // .serum_name is set by lab specific extractor
        // .passage = make(serum_passage_row()), //
        // .serum_id = make(serum_id_row())      //
    };

} // acmacs::sheet::v1::ExtractorCDC::serum

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCDC::exclude_control_sera(warn_if_not_found /*winf*/)
{

} // acmacs::sheet::v1::ExtractorCDC::exclude_control_sera

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::ExtractorWithSerumRowsAbove::serum(size_t sr_no) const
{
    const auto make = [this, col = serum_columns().at(sr_no)](std::optional<nrow_t> row) -> std::string {
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

} // acmacs::sheet::v1::ExtractorWithSerumRowsAbove::serum

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorWithSerumRowsAbove::exclude_control_sera(warn_if_not_found /*winf*/)
{
    const auto exclude = [](const auto& cell) {
        if (is_string(cell)) {
            const auto text = fmt::format("{}", cell);
            if (std::regex_search(text, re_human_who_serum))
                return true;
        }
        return false;
    };

    ranges::actions::remove_if(serum_columns_, [this, exclude](ncol_t col) {
        if ((serum_id_row_.has_value() && exclude(sheet().cell(*serum_id_row_, col))) || (serum_passage_row_.has_value() && exclude(sheet().cell(*serum_passage_row_, col)))) {
            AD_LOG(acmacs::log::xlsx, "[{}] serum column {} excluded: HUMAN or WHO or NORMAL serum", lab(), col);
            return true;
        }
        return false;
    });

} // acmacs::sheet::v1::ExtractorWithSerumRowsAbove::exclude_control_sera

// ----------------------------------------------------------------------

acmacs::sheet::v1::ExtractorCrick::ExtractorCrick(std::shared_ptr<Sheet> a_sheet)
    : ExtractorWithSerumRowsAbove(a_sheet)
{
    lab("CRICK");

} // acmacs::sheet::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorCrick::find_serum_rows(warn_if_not_found winf)
{
    find_serum_name_rows(winf);
    find_serum_passage_row(re_serum_passage, winf);
    find_serum_id_row(re_CRICK_serum_id, winf);

} // acmacs::sheet::v1::ExtractorCrick::find_serum_rows


// ======================================================================

void acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows(warn_if_not_found winf)
{
    fmt::memory_buffer report;
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        if (const size_t matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_1, row, col); }));
            matches == number_of_sera()) {
            serum_name_1_row_ = row;
            break;
        }
        else if (matches)
            fmt::format_to(report, "    re_CRICK_serum_name_1 row:{} matches:{}\n", row, matches);
    }

    if (serum_name_1_row_.has_value())
        AD_LOG(acmacs::log::xlsx, "[Crick]: Serum name row 1: {}", *serum_name_1_row_);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[Crick]: No serum name row 1 found (number of sera: {})\n{}", number_of_sera(), fmt::to_string(report));

    if (serum_name_1_row_.has_value() &&
        static_cast<size_t>(ranges::count_if(serum_columns(), [this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_2, *serum_name_1_row_ + nrow_t{1}, col); })) == number_of_sera())
        serum_name_2_row_ = *serum_name_1_row_ + nrow_t{1};
    else
        AD_DEBUG("re_CRICK_serum_name_2 {}: {}", *serum_name_1_row_ + nrow_t{1},
                 static_cast<size_t>(ranges::count_if(serum_columns(), [this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_2, *serum_name_1_row_ + nrow_t{1}, col); })));

    if (serum_name_2_row_.has_value())
        AD_LOG(acmacs::log::xlsx, "[Crick]: Serum name row 2: {}", *serum_name_2_row_);
    else
        AD_WARNING_IF(winf == warn_if_not_found::yes, "[Crick]: No serum name row 2 found");

} // acmacs::sheet::v1::ExtractorCrick::find_serum_name_rows

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::ExtractorCrick::serum(size_t sr_no) const
{
    auto serum = ExtractorWithSerumRowsAbove::serum(sr_no);
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
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        const size_t two_fold_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_prn_2fold, row, col); }));
        const size_t read_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_prn_read, row, col); }));
        if (two_fold_matches == (serum_columns().size() / 2) && two_fold_matches == read_matches) {
            two_fold_read_row_ = row;
            break;
        }
    }

    if (two_fold_read_row_.has_value()) {
        ncol_t col_no{0};
        ranges::actions::remove_if(serum_columns(), [&col_no](auto) { const auto remove = (*col_no % 2) != 0; ++col_no; return remove; });
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
        const auto two_fold_col = sheet().matches(re_CRICK_prn_2fold, *two_fold_read_row_, left_col) ? left_col : ncol_t{left_col + ncol_t{1}};
        const auto read_col = two_fold_col == left_col ? ncol_t{left_col + ncol_t{1}} : left_col;

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

acmacs::sheet::v1::ExtractorVIDRL::ExtractorVIDRL(std::shared_ptr<Sheet> a_sheet)
    : ExtractorWithSerumRowsAbove(a_sheet)
{
    lab("VIDRL");

} // acmacs::sheet::v1::ExtractorVIDRL::ExtractorVIDRL

// ----------------------------------------------------------------------

acmacs::sheet::v1::serum_fields_t acmacs::sheet::v1::ExtractorVIDRL::serum(size_t sr_no) const
{
    auto serum = ExtractorWithSerumRowsAbove::serum(sr_no);
    if (serum_name_row_) {
        serum.name = fmt::format("{}", sheet().cell(*serum_name_row_, serum_columns().at(sr_no)));

        // TAS503 -> A(H3N2)/TASMANIA/503/2020
        if (std::smatch match; std::regex_search(serum.name, match, re_VIDRL_serum_name)) {
            for (const auto ag_no : range_from_0_to(number_of_antigens())) {
                const auto antigen_name = antigen(ag_no).name;
                const auto antigen_name_fields = acmacs::string::split(antigen_name, "/");
                if (antigen_name_fields.size() == 4 && acmacs::string::startswith_ignore_case(antigen_name_fields[1], match.str(1)) && antigen_name_fields[2] == match.str(2)) {
                    serum.name = antigen_name;
                    break;
                }
            }
        }
    }
    else
        serum.name = "*no serum_name_row_*";
    return serum;

} // acmacs::sheet::v1::ExtractorVIDRL::serum

// ----------------------------------------------------------------------

void acmacs::sheet::v1::ExtractorVIDRL::find_serum_rows(warn_if_not_found winf)
{
    serum_name_row_ = find_serum_row(re_VIDRL_serum_name, "name", winf);
    find_serum_passage_row(re_serum_passage, winf);
    find_serum_id_row(re_VIDRL_serum_id, winf);

} // acmacs::sheet::v1::ExtractorVIDRL::find_serum_rows

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
