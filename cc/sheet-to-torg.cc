#include "acmacs-base/range-v3.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-whocc/sheet-to-torg.hh"
#include "acmacs-whocc/data-fix.hh"

// ----------------------------------------------------------------------

void acmacs::sheet::v1::SheetToTorg::preprocess(Extractor::warn_if_not_found winf)
{
    extractor_ = extractor_factory(sheet(), winf);

} // acmacs::sheet::v1::SheetToTorg::preprocess

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::SheetToTorg::serum_name(const serum_fields_t& serum) const
{
    return acmacs::string::join(acmacs::string::join_space, serum.name, serum.conc, serum.dilut, serum.boosted ? "BOOSTED" : "");

} // acmacs::sheet::v1::SheetToTorg::serum_name

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
        auto serum = extractor_->serum(sr_no);
        acmacs::data_fix::Set::fix(serum, sr_no);
        data[st(sr_row::name)][sr_col] = serum_name(serum);
        data[st(sr_row::passage)][sr_col] = serum.passage;
        data[st(sr_row::serum_id)][sr_col] = serum.serum_id;
    }

    for (const auto ag_no : range_from_0_to(extractor_->number_of_antigens())) {
        const auto ag_row = st(sr_row::base) + ag_no;
        auto antigen = extractor_->antigen(ag_no);
        acmacs::data_fix::Set::fix(antigen, ag_no);
        data[ag_row][st(ag_col::name)] = antigen.name;
        data[ag_row][st(ag_col::date)] = antigen.date;
        data[ag_row][st(ag_col::passage)] = antigen.passage;
        data[ag_row][st(ag_col::lab_id)] = antigen.lab_id;

        for (const auto sr_no : range_from_0_to(extractor_->number_of_sera())) {
            auto titer = extractor_->titer(ag_no, sr_no);
            acmacs::data_fix::Set::fix_titer(titer, ag_no, sr_no);
            switch (const auto fields = acmacs::string::split(titer, "/"); fields.size()) {
                case 2:
                    data[ag_row][st(ag_col::base) + sr_no] = fmt::format("{:>5s} / {:>5s}", fields[0], fields[1]);
                    break;
                case 1:
                default:
                    data[ag_row][st(ag_col::base) + sr_no] = fmt::format("{:>5s}", titer);
                    break;
            }
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
    fmt::format_to(result, "- Subtype: {}\n", extractor_->subtype_without_lineage());
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

    fmt::format_to(result, "\n* COMMENT local vars ----------------------------------------------------------------------\n"
                           ":PROPERTIES:\n:VISIBILITY: folded\n:END:\n\n"
                           "#+STARTUP: showall indent\n"
                           "Local Variables:\n"
                           "eval: (if (fboundp 'eu-whocc-torg-to-ace) (add-hook 'after-save-hook 'eu-whocc-torg-to-ace nil 'local))\n"
                           "eval: (if (fboundp 'eu-whocc-xlsx-torg-ace-hup) (add-hook 'after-save-hook 'eu-whocc-xlsx-torg-ace-hup nil 'local))\n"
                           "End:\n");

    return fmt::to_string(result);

} // acmacs::sheet::v1::SheetToTorg::torg

// ----------------------------------------------------------------------

std::string acmacs::sheet::v1::SheetToTorg::format_assay_data(std::string_view format) const
{
    using namespace fmt::literals;

    const auto assay_rbc = [this]() -> std::string {
        if (const auto assay = extractor_->assay(); assay == "HI")
            return fmt::format("hi-{}", ::string::lower(extractor_->rbc()));
        else if (assay == "HINT")
            return "hint";
        else
            return "neut";
    };

    return fmt::format(format,                                               //
                       "virus_type"_a = extractor_->subtype(),               //
                       "lineage"_a = extractor_->lineage(),                  //
                       "virus_type_lineage"_a = extractor_->subtype_short(), //
                       // "subset"_a = ,
                       "virus_type_lineage_subset_short_low"_a = extractor_->subtype_short(), //
                       "assay_full"_a = extractor_->assay(),                                  //
                       "assay"_a = extractor_->assay(),                                       //
                       "assay_low"_a = ::string::lower(extractor_->assay()),                    //
                       "assay_low_rbc"_a = assay_rbc(),                                       //
                       "lab"_a = extractor_->lab(),                                           //
                       "lab_low"_a = ::string::lower(extractor_->lab()),                        //
                       "rbc"_a = extractor_->rbc(),                                           //
                       "table_date"_a = extractor_->date("%Y%m%d")                            //
    );

} // acmacs::sheet::v1::SheetToTorg::format_assay_data

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
