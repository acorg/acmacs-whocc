#pragma once

#include <optional>

#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{

    class Sheet;

    class Extractor
    {
      public:
        Extractor(const Sheet& a_sheet) : sheet_{a_sheet} {}
        virtual ~Extractor() = default;

        const Sheet& sheet() const { return sheet_; }

        std::string_view lab() const { return lab_; }
        std::string_view subtype() const { return subtype_; }
        std::string_view lineage() const { return lineage_; }
        std::string_view assay() const { return assay_; }
        std::string_view rbc() const { return rbc_; }

        void lab(std::string_view a_lab) { lab_ = a_lab; }
        void subtype(std::string_view a_subtype) { subtype_ = a_subtype; }
        void lineage(std::string_view a_lineage) { lineage_ = a_lineage; }
        void assay(std::string_view a_assay) { assay_ = a_assay; }
        void rbc(std::string_view a_rbc) { rbc_ = a_rbc; }

        std::optional<size_t> antigen_name_column() const { return antigen_name_column_; }
        std::optional<size_t> antigen_date_column() const { return antigen_date_column_; }
        std::optional<size_t> antigen_passage_column() const { return antigen_passage_column_; }
        size_t longest_antigen_name() const { return longest_antigen_name_; }
        size_t longest_antigen_passage() const { return longest_antigen_passage_; }
        const range& titer_columns() const { return titer_columns_; }
        const std::vector<size_t>& antigen_rows() const { return antigen_rows_; }

        void preprocess();

    protected:
        virtual void find_titers();
        virtual void find_antigen_name_column();
        virtual void find_antigen_date_column();
        virtual void find_antigen_passage_column();

      private:
        const Sheet& sheet_;
        std::string lab_;
        std::string subtype_;
        std::string lineage_;
        std::string assay_{"HI"};
        std::string rbc_;

        std::optional<size_t> antigen_name_column_, antigen_date_column_, antigen_passage_column_;
        size_t longest_antigen_name_{0}, longest_antigen_passage_{0};
        range titer_columns_;
        std::vector<size_t> antigen_rows_;

    };

    std::unique_ptr<Extractor> extractor_factory(const Sheet& sheet);

    // ----------------------------------------------------------------------

    class ExtractorCrick : public Extractor
    {
      public:
        ExtractorCrick(const Sheet& a_sheet);
    };

    class ExtractorCrickPRN : public ExtractorCrick
    {
      public:
        ExtractorCrickPRN(const Sheet& a_sheet);
    };

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
