#pragma once

#include <optional>

#include "acmacs-base/date.hh"
#include "acmacs-base/flat-map.hh"
#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{

    class Sheet;

    struct Error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    // ----------------------------------------------------------------------

    struct antigen_fields_t
    {
        std::string name{};
        std::string date{};
        std::string passage{};
        std::string lab_id{};
    };

    struct serum_fields_t
    {
        std::string name{};
        std::string serum_id{};
        std::string passage{};
        std::string species{};
        std::string conc{};
        std::string dilut{};
        bool boosted{false};
    };

    // ----------------------------------------------------------------------

    class Extractor
    {
      public:
        Extractor(std::shared_ptr<Sheet> a_sheet) : sheet_{a_sheet} {}
        virtual ~Extractor() = default;

        const Sheet& sheet() const { return *sheet_; }

        std::string_view lab() const { return lab_; }
        std::string_view subtype() const { return subtype_; }
        std::string_view subtype_without_lineage() const;
        std::string subtype_short() const;
        std::string_view lineage() const { return lineage_; }
        std::string_view assay() const { return assay_; }
        std::string_view rbc() const { return rbc_; }
        std::string date(const char* fmt = "%Y-%m-%d") const { return date::display(date_, fmt); }

        size_t number_of_antigens() const { return antigen_rows().size(); }
        size_t number_of_sera() const { return serum_columns().size(); }

        virtual antigen_fields_t antigen(size_t ag_no) const;
        virtual serum_fields_t serum(size_t sr_no) const = 0;

        virtual std::string titer_comment() const { return {}; }
        virtual std::string titer(size_t ag_no, size_t sr_no) const;

        void lab(std::string_view a_lab) { lab_ = a_lab; }
        void subtype(std::string_view a_subtype) { subtype_ = a_subtype; }
        void lineage(std::string_view a_lineage) { lineage_ = a_lineage; }
        void assay(std::string_view a_assay) { assay_ = a_assay; }
        void rbc(std::string_view a_rbc) { rbc_ = a_rbc; }
        void date(const date::year_month_day& a_date) { date_ = a_date; }

        enum class warn_if_not_found { no, yes };
        void preprocess(warn_if_not_found winf);

        virtual void report_data_anchors() const;
        virtual void check_export_possibility() const; // throws Error if exporting is not possible

        virtual void force_serum_name_row(nrow_t row);
        virtual void force_serum_passage_row(nrow_t row);
        virtual void force_serum_id_row(nrow_t row);

        virtual const char* extractor_name() const { return "[Extractor]"; }

      protected:
        virtual void find_titers(warn_if_not_found winf);
        virtual void find_antigen_name_column(warn_if_not_found winf);
        virtual void remove_redundant_antigen_rows(warn_if_not_found winf);
        virtual void find_antigen_date_column(warn_if_not_found winf);
        virtual void find_antigen_passage_column(warn_if_not_found winf);
        virtual void find_antigen_lab_id_column(warn_if_not_found winf);
        virtual void find_serum_rows(warn_if_not_found) {}
        virtual std::optional<nrow_t> find_serum_row(const std::regex& re, std::string_view row_name, warn_if_not_found winf, std::optional<nrow_t> ignore = std::nullopt) const;
        virtual void exclude_control_sera(warn_if_not_found winf) = 0;
        virtual void adjust_titer_range(nrow_t /*row*/, column_range& /*cr*/) {}

        std::optional<ncol_t> antigen_name_column() const { return antigen_name_column_; }
        std::optional<ncol_t> antigen_date_column() const { return antigen_date_column_; }
        std::optional<ncol_t> antigen_passage_column() const { return antigen_passage_column_; }
        std::optional<ncol_t> antigen_lab_id_column() const { return antigen_lab_id_column_; }
        const std::vector<nrow_t>& antigen_rows() const { return antigen_rows_; }
        const std::vector<ncol_t>& serum_columns() const { return serum_columns_; }

        std::vector<ncol_t>& serum_columns() { return serum_columns_; }

        virtual bool is_virus_name(nrow_t row, ncol_t col) const;
        // virtual bool is_passage(nrow_t row, ncol_t col) const;
        virtual bool is_lab_id(const cell_t& /*cell*/) const { return false; }
        virtual bool valid_titer_row(nrow_t /*row*/, const column_range& /*cr*/) const { return true; }
        virtual bool is_control_serum_cell(const cell_t& cell) const;

        virtual std::string make_passage(const std::string& src) const;
        virtual std::string make_date(const std::string& src) const;
        virtual std::string make_lab_id(const std::string& src) const;

        virtual std::string report_serum_anchors() const = 0;

        std::optional<ncol_t> antigen_name_column_, antigen_date_column_, antigen_passage_column_, antigen_lab_id_column_;
        std::vector<nrow_t> antigen_rows_;
        std::vector<ncol_t> serum_columns_;

      private:
        std::shared_ptr<Sheet> sheet_;
        std::string lab_;
        std::string subtype_;
        std::string lineage_;
        std::string assay_{"HI"};
        std::string rbc_;
        date::year_month_day date_{date::invalid_date()};

    };

    std::unique_ptr<Extractor> extractor_factory(std::shared_ptr<Sheet> sheet, Extractor::warn_if_not_found winf);

    // ----------------------------------------------------------------------

    class ExtractorCDC : public Extractor
    {
      public:
        ExtractorCDC(std::shared_ptr<Sheet> a_sheet);

        serum_fields_t serum(size_t sr_no) const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

        void check_export_possibility() const override; // throws Error if exporting is not possible

        const char* extractor_name() const override { return "[CDC]"; }

      protected:
        bool is_lab_id(const cell_t& cell) const override;
        void find_serum_rows(warn_if_not_found winf) override;
        virtual void find_serum_columns(warn_if_not_found winf);
        virtual void find_serum_name_column(warn_if_not_found winf, const std::regex& re_serum_index);
        void find_serum_column_label(const std::regex& re, std::optional<ncol_t>& col, std::string_view label_name);
        void find_serum_column_label(const std::regex& re1, const std::regex& re2, std::optional<ncol_t>& col, std::string_view label_name);
        void find_serum_index_row(warn_if_not_found winf, const std::regex& re_serum_index);
        void remove_redundant_antigen_rows(warn_if_not_found winf) override;
        void exclude_control_sera(warn_if_not_found winf) override;
        void adjust_titer_range(nrow_t row, column_range& cr) override;

        bool valid_titer_row(nrow_t row, const column_range& cr) const override;

        std::string report_serum_anchors() const override;

        virtual bool serum_index_matches(const cell_t& at_row, const cell_t& at_column) const;

        std::optional<nrow_t> serum_index_row_;
        std::vector<nrow_t> serum_rows_;
        std::optional<ncol_t> serum_index_column_, serum_name_column_, serum_id_column_, serum_treated_column_, serum_species_column_, serum_boosted_column_, serum_conc_column_, serum_dilut_column_, serum_passage_column_, serum_pool_column_;

        nrow_t find_serum_row_by_col(ncol_t col) const;
    };

    // ----------------------------------------------------------------------

    class ExtractorAc21 : public ExtractorCDC
    {
      public:
        ExtractorAc21(std::shared_ptr<Sheet> a_sheet);

        const char* extractor_name() const override { return "[AC21]"; }
        bool serum_index_matches(const cell_t& at_row, const cell_t& at_column) const override;

      protected:
        // bool is_lab_id(const cell_t& cell) const override;
        void find_antigen_lab_id_column(warn_if_not_found winf) override;
        void find_serum_rows(warn_if_not_found winf) override;
        void find_serum_columns(warn_if_not_found winf) override;
    };

    // ----------------------------------------------------------------------

    class ExtractorWithSerumRowsAbove : public Extractor
    {
    public:
        using Extractor::Extractor;

        serum_fields_t serum(size_t sr_no) const override;

        void check_export_possibility() const override; // throws Error if exporting is not possible

        void force_serum_name_row(nrow_t row) override;
        void force_serum_passage_row(nrow_t row) override;
        void force_serum_id_row(nrow_t row) override;

      protected:
        virtual void find_serum_passage_row(const std::regex& re, warn_if_not_found winf) { serum_passage_row_ = find_serum_row(re, "passage", winf); }
        virtual void find_serum_id_row(const std::regex& re, warn_if_not_found winf) { serum_id_row_ = find_serum_row(re, "id", winf); }
        void exclude_control_sera(warn_if_not_found winf) override;

        std::optional<nrow_t> serum_name_row() const { return serum_name_row_; }
        std::optional<nrow_t> serum_passage_row() const { return serum_passage_row_; }
        std::optional<nrow_t> serum_id_row() const { return serum_id_row_; }

        std::string report_serum_anchors() const override;

        std::optional<nrow_t> serum_name_row_, serum_passage_row_, serum_id_row_;
    };

    // ----------------------------------------------------------------------

    class ExtractorCrick : public ExtractorWithSerumRowsAbove
    {
      public:
        ExtractorCrick(std::shared_ptr<Sheet> a_sheet);

        serum_fields_t serum(size_t sr_no) const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

        void check_export_possibility() const override; // throws Error if exporting is not possible

        const char* extractor_name() const override { return "[Crick]"; }

      protected:
        void find_serum_rows(warn_if_not_found winf) override;
        void find_serum_name_rows(warn_if_not_found winf);
        void find_serum_less_than_substitutions(warn_if_not_found winf);

        std::string report_serum_anchors() const override;

        std::optional<nrow_t> serum_name_1_row_, serum_name_2_row_;
        small_map_with_unique_keys_t<std::string, std::string> footnote_index_subst_;
        std::vector<std::string> serum_less_than_substitutions_;
    };

    class ExtractorCrickPRN : public ExtractorCrick
    {
      public:
        ExtractorCrickPRN(std::shared_ptr<Sheet> a_sheet);

        std::string titer_comment() const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

        const char* extractor_name() const override { return "[CrickPRN]"; }

      protected:
        void find_serum_rows(warn_if_not_found winf) override;

      private:
        std::optional<nrow_t> two_fold_read_row_;

        void find_two_fold_read_row();
    };

    // ----------------------------------------------------------------------

    class ExtractorNIID : public ExtractorWithSerumRowsAbove
    {
      public:
        ExtractorNIID(std::shared_ptr<Sheet> a_sheet);

        serum_fields_t serum(size_t sr_no) const override;
        std::string titer(size_t ag_no, size_t sr_no) const override;

        const char* extractor_name() const override { return "[NIID]"; }

      protected:
        void find_antigen_lab_id_column(warn_if_not_found winf) override;
        void find_serum_rows(warn_if_not_found winf) override;
        bool is_control_serum_cell(const cell_t& cell) const override;

        std::string report_serum_anchors() const override;
    };

    // ----------------------------------------------------------------------

    class ExtractorVIDRL : public ExtractorWithSerumRowsAbove
    {
      public:
        ExtractorVIDRL(std::shared_ptr<Sheet> a_sheet);

        serum_fields_t serum(size_t sr_no) const override;

        const char* extractor_name() const override { return "[VIDRL]"; }

      protected:
        bool is_lab_id(const cell_t& cell) const override;
        void find_serum_rows(warn_if_not_found winf) override;
        std::string make_date(const std::string& src) const override;
        std::string make_lab_id(const std::string& src) const override;
        void adjust_titer_range(nrow_t row, column_range& cr) override;
    };

} // namespace acmacs::sheet::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
