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
        SheetToTorg(std::unique_ptr<Sheet> a_sheet) : sheet_{std::move(a_sheet)} {}

        void preprocess();
        std::string torg() const;
        std::string name() const;

      private:
        std::unique_ptr<Sheet> sheet_;
        std::unique_ptr<Extractor> extractor_;

        // std::optional<size_t> antigen_name_column_, antigen_date_column_, antigen_passage_column_;
        // size_t longest_antigen_name_{0}, longest_antigen_passage_{0};
        // range titer_columns_;
        // std::vector<size_t> antigen_rows_;

        const Sheet& sheet() const { return *sheet_; }

        // void find_titers();
        // void find_antigen_name_column();
        // void find_antigen_date_column();
        // void find_antigen_passage_column();
    };

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
