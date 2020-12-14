#pragma once

#include <optional>

#include "acmacs-whocc/sheet.hh"
#include "acmacs-whocc/sheet-extractor.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{
    class SheetToTorg
    {
      public:
        SheetToTorg(std::shared_ptr<Sheet> a_sheet) : sheet_{a_sheet} {}

        bool valid() const { return bool{extractor_}; }
        void preprocess(Extractor::warn_if_not_found winf);
        std::string torg() const;
        std::string format_assay_data(std::string_view format) const;
        std::string name() const { return format_assay_data("{virus_type_lineage}-{assay_low_rbc}-{lab_low}-{table_date}"); }

        const Extractor& extractor() const { return *extractor_; }

      private:
        std::shared_ptr<Sheet> sheet_; // shared_ptr necessary for py interface
        std::unique_ptr<Extractor> extractor_;

        std::shared_ptr<Sheet> sheet() const { return sheet_; }
        std::string serum_name(const serum_fields_t& serum) const;
    };

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
