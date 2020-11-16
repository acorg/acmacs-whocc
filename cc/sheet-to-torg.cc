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
    const size_t ag_name_col{1};
    const size_t ag_date_col{2};
    const size_t ag_passage_col{3};
    const size_t ag_row_base{4};
    const size_t sr_name_row{1};
    const size_t sr_passage_row{2};
    const size_t sr_id_row{3};
    const size_t sr_col_base{4};

    std::vector<std::vector<std::string>> data(extractor_->number_of_antigens() + 4, std::vector<std::string>(extractor_->number_of_sera() + 4));
    data[0][ag_name_col] = "name";
    data[0][ag_date_col] = "date";
    data[0][ag_passage_col] = "passage";
    data[sr_name_row][0] = "name";
    data[sr_passage_row][0] = "passage";
    data[sr_id_row][0] = "serum_id";

    for (const auto sr_no : range_from_0_to(extractor_->number_of_sera())) {
        const auto sr_col = sr_col_base + sr_no;
        data[sr_name_row][sr_col] = extractor_->serum_name(sr_no);
        data[sr_passage_row][sr_col] = extractor_->serum_passage(sr_no);
        data[sr_id_row][sr_col] = extractor_->serum_id(sr_no);
    }

    for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
        const auto ag_row = ag_row_base + ag_no;
        data[ag_row][ag_name_col] = extractor_->antigen_name(ag_no);
        data[ag_row][ag_date_col] = extractor_->antigen_date(ag_no);
        data[ag_row][ag_passage_col] = extractor_->antigen_passage(ag_no);
        for (const auto sr_no : range_from_0_to(extractor_->number_of_sera())) {
            data[ag_row][sr_col_base + sr_no] = extractor_->titer(ag_no, sr_no);
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
        if (const auto assay = extractor_->assay(); assay != "HINT")
            return "hint";
        else if (assay == "HI")
            return std::string{extractor_->rbc()};
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
