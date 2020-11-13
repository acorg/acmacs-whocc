#include "acmacs-base/range-v3.hh"
#include "acmacs-base/log.hh"
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

    std::vector<std::vector<std::string>> data(extractor_->number_of_antigens() + 4, std::vector<std::string>(extractor_->number_of_sera() + 4));
    data[0][ag_name_col] = "name";
    data[0][ag_date_col] = "date";
    data[0][ag_passage_col] = "passage";
    data[sr_name_row][0] = "name";
    data[sr_passage_row][0] = "passage";
    data[sr_id_row][0] = "serum_id";

    for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
        data[ag_row_base + ag_no][ag_name_col] = extractor_->antigen_name(ag_no);
        data[ag_row_base + ag_no][ag_date_col] = extractor_->antigen_date(ag_no);
        data[ag_row_base + ag_no][ag_passage_col] = extractor_->antigen_passage(ag_no);
    }

    std::vector<size_t> column_widths(data[0].size(), 0);
    for (const auto col : range_from_0_to(data[0].size()))
        column_widths[col] = ranges::max(data | ranges::views::transform([col](const auto& row) { return row[col].size(); }));

    for (const auto& row : data) {
        fmt::format_to(result, "|");
        for (const auto col : range_from_0_to(row.size()))
            fmt::format_to(result, " {:{}s} |", row[col], column_widths[col]);
        fmt::format_to(result, "\n");
    }

    // fmt::format_to(result, "|          | {:{}s} |       date | {:{}s} |", "name", extractor_->longest_antigen_name(), "passage", extractor_->longest_antigen_passage());
    // for ([[maybe_unused]] const auto col : range_from_0_to(extractor_->number_of_sera()))
    //     fmt::format_to(result, " |");
    // fmt::format_to(result, "\n");

    // fmt::format_to(result, "|          | {:{}s} |            | {:{}s} |", "", extractor_->longest_antigen_name(), "", extractor_->longest_antigen_passage());
    // fmt::format_to(result, "\n");

    // for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
    //     fmt::format_to(result, "|          | {:{}s} | {} | {:{}s} |",                       //
    //                    extractor_->antigen_name(ag_no), extractor_->longest_antigen_name(), //
    //                    extractor_->antigen_date(ag_no),                                     //
    //                    extractor_->antigen_passage(ag_no), extractor_->longest_antigen_passage());
    //     fmt::format_to(result, "\n");
    // }

    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
