#pragma once

#include "acmacs-base/pybind11.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{
    class Sheet;
}

namespace acmacs::whocc_xlsx::inline v1
{
    void py_init(const std::vector<std::string_view>& scripts);

    struct detect_result_t
    {
        bool ignore{false};
        std::string lab{};
        std::string assay{};
        std::string subtype{};
        std::string lineage{};
    };

    detect_result_t py_sheet_detect(std::shared_ptr<acmacs::sheet::Sheet> sheet);

} // namespace acmacs::whocc_xlsx::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
