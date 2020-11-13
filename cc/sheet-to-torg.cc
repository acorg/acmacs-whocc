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

    fmt::format_to(result, "|          | {:{}s} |       date | {:{}s} |", "name", extractor_->longest_antigen_name(), "passage", extractor_->longest_antigen_passage());
    for ([[maybe_unused]] const auto col : range_from_0_to(extractor_->number_of_sera()))
        fmt::format_to(result, " |");
    fmt::format_to(result, "\n");

    for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
        fmt::format_to(result, "|          | {:{}s} | {} | {:{}s} |",                       //
                       extractor_->antigen_name(ag_no), extractor_->longest_antigen_name(), //
                       extractor_->antigen_date(ag_no),                                     //
                       extractor_->antigen_passage(ag_no), extractor_->longest_antigen_passage());
        fmt::format_to(result, "\n");
    }
    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
