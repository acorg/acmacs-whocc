#pragma once

#include <optional>

#include "acmacs-base/date.hh"
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
        std::string subtype_short() const;
        std::string_view lineage() const { return lineage_; }
        std::string_view assay() const { return assay_; }
        std::string_view rbc() const { return rbc_; }
        std::string date(const char* fmt = "%Y-%m-%d") const { return date::display(date_, fmt); }

        size_t number_of_antigens() const { return antigen_rows().size(); }
        size_t number_of_sera() const { return serum_columns().size(); }

        size_t longest_antigen_name() const { return longest_antigen_name_; }
        size_t longest_antigen_passage() const { return longest_antigen_passage_; }

        virtual std::string antigen_name(size_t ag_no) const;
        virtual std::string antigen_date(size_t ag_no) const;
        virtual std::string antigen_passage(size_t ag_no) const;
        virtual std::string antigen_lab_id(size_t ag_no) const;

        virtual std::string serum_name(size_t /*sr_no*/) const { return {}; }
        virtual std::string serum_passage(size_t sr_no) const;
        virtual std::string serum_id(size_t sr_no) const;

        void lab(std::string_view a_lab) { lab_ = a_lab; }
        void subtype(std::string_view a_subtype) { subtype_ = a_subtype; }
        void lineage(std::string_view a_lineage) { lineage_ = a_lineage; }
        void assay(std::string_view a_assay) { assay_ = a_assay; }
        void rbc(std::string_view a_rbc) { rbc_ = a_rbc; }
        void date(const date::year_month_day& a_date) { date_ = a_date; }

        virtual std::string titer_comment() const { return {}; }
        virtual std::string titer(size_t /*ag_no*/, size_t /*sr_no*/) const { return {}; }

        void preprocess();

      protected:
        virtual void find_titers();
        virtual void find_antigen_name_column();
        virtual void find_antigen_date_column();
        virtual void find_antigen_passage_column();
        virtual void find_antigen_lab_id_column() {}
        virtual void find_serum_rows() {}
        virtual void find_serum_passage_row(const std::regex& re);
        virtual void find_serum_id_row(const std::regex& re);

        std::optional<size_t> antigen_name_column() const { return antigen_name_column_; }
        std::optional<size_t> antigen_date_column() const { return antigen_date_column_; }
        std::optional<size_t> antigen_passage_column() const { return antigen_passage_column_; }
        std::optional<size_t> antigen_lab_id_column() const { return antigen_lab_id_column_; }
        const std::vector<size_t>& antigen_rows() const { return antigen_rows_; }
        const std::vector<size_t>& serum_columns() const { return serum_columns_; }

        std::optional<size_t> serum_passage_row() const { return serum_passage_row_; }
        std::optional<size_t> serum_id_row() const { return serum_id_row_; }

        std::vector<size_t>& serum_columns() { return serum_columns_; }

      private:
        const Sheet& sheet_;
        std::string lab_;
        std::string subtype_;
        std::string lineage_;
        std::string assay_{"HI"};
        std::string rbc_;
        date::year_month_day date_{date::invalid_date()};

        std::optional<size_t> antigen_name_column_, antigen_date_column_, antigen_passage_column_, antigen_lab_id_column_;
        std::optional<size_t> serum_passage_row_, serum_id_row_;
        size_t longest_antigen_name_{0}, longest_antigen_passage_{0};
        std::vector<size_t> antigen_rows_, serum_columns_;
    };

    std::unique_ptr<Extractor> extractor_factory(const Sheet& sheet);

    // ----------------------------------------------------------------------

    class ExtractorCrick : public Extractor
    {
      public:
        ExtractorCrick(const Sheet& a_sheet);

        std::string serum_name(size_t sr_no) const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

      protected:
        void find_serum_rows() override;
        void find_serum_name_rows();

      private:
        std::optional<size_t> serum_name_1_row_, serum_name_2_row_, serum_id_row_;
    };

    class ExtractorCrickPRN : public ExtractorCrick
    {
      public:
        ExtractorCrickPRN(const Sheet& a_sheet);

        std::string titer_comment() const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

      protected:
        void find_serum_rows() override;

      private:
        std::optional<size_t> two_fold_read_row_;

        void find_two_fold_read_row();
    };

} // namespace acmacs::sheet::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
