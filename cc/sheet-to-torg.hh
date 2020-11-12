#pragma once

#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{

    class SheetToTorg
    {
      public:
        SheetToTorg(std::unique_ptr<Sheet> a_sheet) : sheet_{std::move(a_sheet)} {}

        void preprocess();

      private:
        std::unique_ptr<Sheet> sheet_;

        const Sheet& sheet() const { return *sheet_; }

        void find_titers();
    };

} // namespace acmacs::xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
