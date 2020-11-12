#pragma once

#include "acmacs-whocc/xlsx-xlnt.hh"

// ----------------------------------------------------------------------

namespace acmacs::xlsx::inline v1
{

    using _SheetBase = xlnt::Sheet;
    using _DocBase = xlnt::Doc;

    // ----------------------------------------------------------------------

    class Sheet : public _SheetBase
    {
      public:
    };

    class Doc : public _DocBase
    {
      protected:
        Doc(std::string_view filename) : _DocBase(filename) {}

      private:
        friend Doc open(std::string_view filename);
    };

    // ----------------------------------------------------------------------

    inline Doc open(std::string_view filename) { return filename; }

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
