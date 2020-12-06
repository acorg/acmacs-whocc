#include "acmacs-base/log.hh"
#include "acmacs-whocc/data-fix-py.hh"
#include "acmacs-whocc/data-fix.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations" // 2.6.1 2020-12-06
#endif

using namespace pybind11::literals;

PYBIND11_EMBEDDED_MODULE(data_fix_module, mdl)
{
    mdl.def(
        "name_antigen_serum_fix",
        [](std::string& rex, std::string& replacement) {
            // AD_DEBUG("name_antigen_serum_fix \"{}\" \"{}\"", rex, replacement);
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::AntigenSerumName>(std::move(rex), std::move(replacement)));
        },
        "rex"_a, "replacement"_a);

    mdl.def(
        "passage_antigen_serum_fix",
        [](std::string& rex, std::string& replacement, std::string& name_append) {
            // AD_DEBUG("passage_antigen_serum_fix \"{}\" \"{}\"", rex, replacement);
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::AntigenSerumPassage>(std::move(rex), std::move(replacement), std::move(name_append)));
        },
        "rex"_a, "replacement"_a, "name_append"_a);

    mdl.def(
        "titer_fix",
        [](std::string& rex, std::string& replacement) {
            // AD_DEBUG("titer_fix \"{}\" \"{}\"", rex, repl);
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::Titer>(std::move(rex), std::move(replacement)));
        },
        "rex"_a, "replacement"_a);
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
