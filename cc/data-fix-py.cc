#include "acmacs-base/log.hh"
#include "acmacs-whocc/data-fix-py.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations" // 2.6.1 2020-12-06
#endif

PYBIND11_EMBEDDED_MODULE(data_fix_module, mdl)
{
    mdl.def("name_antigen_serum_fix", [](std::string_view rex, std::string_view repl) { AD_DEBUG("name_antigen_serum_fix \"{}\" \"{}\"", rex, repl); });
    mdl.def("passage_antigen_serum_fix", [](std::string_view rex, std::string_view repl) { AD_DEBUG("passage_antigen_serum_fix \"{}\" \"{}\"", rex, repl); });
    mdl.def("titer_fix", [](std::string_view rex, std::string_view repl) { AD_DEBUG("titer_fix \"{}\" \"{}\"", rex, repl); });
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::py_init(const std::vector<std::string_view>& scripts)
{
    for (const auto& script : scripts) {
        AD_DEBUG("import {}", script);
        py::eval_file(pybind11::str{script.data(), script.size()});
    }

} // acmacs::data_fix::v1::py_init

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
