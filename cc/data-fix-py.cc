#include "acmacs-base/log.hh"
#include "acmacs-whocc/data-fix-py.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations" // 2.6.1 2020-12-06
#endif

PYBIND11_EMBEDDED_MODULE(data_fix_module, mdl) {
    mdl.attr("a") = 77;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::py_init(const std::vector<std::string_view>& scripts)
{
    AD_DEBUG("py_init");
    py::exec(R"(
import sys
from data_fix_module import *
print(f"data_fix a={a}", file=sys.stderr)
)",
             py::globals()); // , locals);

    AD_DEBUG("py_init");

} // acmacs::data_fix::v1::py_init

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
