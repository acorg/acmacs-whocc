#include "acmacs-base/range-v3.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string.hh"
#include "acmacs-whocc/sheet-to-torg.hh"

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::preprocess()
{
    extractor_ = extractor_factory(sheet());

} // acmacs::sheet::v1::SheetToTorg::preprocess

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::SheetToTorg::torg() const
{
    const auto st = [](auto src) { return static_cast<size_t>(src); };

    enum class ag_col : size_t { serum_field_name = 0, name, date, passage, lab_id, base };
    enum class sr_row : size_t { antigen_field_name = 0, name, passage, serum_id, base };

    std::vector<std::vector<std::string>> data(extractor_->number_of_antigens() + st(sr_row::base), std::vector<std::string>(extractor_->number_of_sera() + st(ag_col::base)));
    data[0][st(ag_col::name)] = "name";
    data[0][st(ag_col::date)] = "date";
    data[0][st(ag_col::passage)] = "passage";
    data[0][st(ag_col::lab_id)] = "lab_id";
    data[st(sr_row::name)][0] = "name";
    data[st(sr_row::passage)][0] = "passage";
    data[st(sr_row::serum_id)][0] = "serum_id";

    for (const auto sr_no : range_from_0_to(extractor_->number_of_sera())) {
        const auto sr_col = st(ag_col::base) + sr_no;
        data[st(sr_row::name)][sr_col] = extractor_->serum_name(sr_no);
        data[st(sr_row::passage)][sr_col] = extractor_->serum_passage(sr_no);
        data[st(sr_row::serum_id)][sr_col] = extractor_->serum_id(sr_no);
    }

    for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
        const auto ag_row = st(sr_row::base) + ag_no;
        data[ag_row][st(ag_col::name)] = extractor_->antigen_name(ag_no);
        data[ag_row][st(ag_col::date)] = extractor_->antigen_date(ag_no);
        data[ag_row][st(ag_col::passage)] = extractor_->antigen_passage(ag_no);
        data[ag_row][st(ag_col::lab_id)] = extractor_->antigen_lab_id(ag_no);
        for (const auto sr_no : range_from_0_to(extractor_->number_of_sera())) {
            data[ag_row][st(ag_col::base) + sr_no] = extractor_->titer(ag_no, sr_no);
        }
    }

    // ----------------------------------------------------------------------

    std::vector<size_t> column_widths(data[0].size(), 0);
    for (const auto col : range_from_0_to(data[0].size()))
        column_widths[col] = ranges::max(data | ranges::views::transform([col](const auto& row) { return row[col].size(); }));

    // ----------------------------------------------------------------------

    fmt::memory_buffer result;
    fmt::format_to(result, "# -*- Org -*-\n\n");

    fmt::format_to(result, "- Lab: {}\n", extractor_->lab());
    fmt::format_to(result, "- Date: {}\n", extractor_->date());
    fmt::format_to(result, "- Assay: {}\n", extractor_->assay());
    fmt::format_to(result, "- Subtype: {}\n", extractor_->subtype());
    if (const auto rbc = extractor_->rbc(); !rbc.empty())
        fmt::format_to(result, "- Rbc: {}\n", rbc);
    if (const auto lineage = extractor_->lineage(); !lineage.empty())
        fmt::format_to(result, "- Lineage: {}\n", lineage);
    fmt::format_to(result, "\n");

    if (const auto titer_comment = extractor_->titer_comment(); !titer_comment.empty())
        fmt::format_to(result, "titer value in the table: {}\n\n", titer_comment);

    for (const auto& row : data) {
        fmt::format_to(result, "|");
        for (const auto col : range_from_0_to(row.size()))
            fmt::format_to(result, " {:{}s} |", row[col], column_widths[col]);
        fmt::format_to(result, "\n");
    }

    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::SheetToTorg::name() const
{
    const auto rbc_assay = [this]() -> std::string {
        if (const auto assay = extractor_->assay(); assay == "HI")
            return std::string{extractor_->rbc()};
        else if (assay == "HINT")
            return "hint";
        else
            return "neut";
    };

    return fmt::format("{}-{}-{}-{}", extractor_->subtype_short(), string::lower(extractor_->lab()), rbc_assay(), extractor_->date("%Y%m%d"));

} // acmacs::sheet::v1::SheetToTorg::name

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
