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
    fmt::format_to(result, "- Date: {}\n", "");
    fmt::format_to(result, "- Subtype: {}\n", extractor_->subtype());
    fmt::format_to(result, "- Assay: {}\n", extractor_->assay());
    fmt::format_to(result, "\n");

    fmt::format_to(result, "|          | {:{}s} |       date | {:{}s} |", "name", extractor_->longest_antigen_name(), "passage", extractor_->longest_antigen_passage());
    for ([[maybe_unused]] const auto col : range_from_to(extractor_->titer_columns()))
        fmt::format_to(result, " |");
    fmt::format_to(result, "\n");

    for (const auto row : extractor_->antigen_rows()) {
        fmt::format_to(result, "|          | {:{}s} | {} | {:{}s} |",                                                                //
                       fmt::format("{}", sheet().cell(row, *extractor_->antigen_name_column())), extractor_->longest_antigen_name(), //
                       sheet().cell(row, *extractor_->antigen_date_column()),                                                        //
                       fmt::format("{}", sheet().cell(row, *extractor_->antigen_passage_column())), extractor_->longest_antigen_passage());
        fmt::format_to(result, "\n");
    }
    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
