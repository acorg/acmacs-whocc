#pragma once

#include <variant>
#include <memory>

#include "acmacs-base/string-compare.hh"
#include "acmacs-whocc/xlsx-xlnt.hh"
#include "acmacs-whocc/csv-parser.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{

    // using XlSheet = xlnt::Sheet;
    using XlDoc = xlnt::Doc;

    // ----------------------------------------------------------------------

    struct Error : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
    };

    // ----------------------------------------------------------------------

    class Doc
    {
      public:
        size_t number_of_sheets() const
            {
                return std::visit([](const auto& ptr) { return ptr->number_of_sheets(); }, doc_);
            }

        std::shared_ptr<acmacs::sheet::Sheet> sheet(size_t sheet_no) {
            return std::visit([sheet_no](const auto& ptr) { return ptr->sheet(sheet_no); }, doc_);
        }

      protected:
        Doc(std::string_view filename)
        {
            if (acmacs::string::endswith_ignore_case(filename, ".csv"))
                doc_ = std::make_unique<csv::Doc>(filename);
            else if (acmacs::string::endswith_ignore_case(filename, ".xlsx"))
                doc_ = std::make_unique<XlDoc>(filename);
            else
                throw Error{fmt::format("unsupported suffix in {}", filename)};
        }

      private:
        std::variant<std::unique_ptr<XlDoc>, std::unique_ptr<csv::Doc>> doc_;

        friend Doc open(std::string_view filename);
    };

    // ----------------------------------------------------------------------

    inline Doc open(std::string_view filename) { return filename; }

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
