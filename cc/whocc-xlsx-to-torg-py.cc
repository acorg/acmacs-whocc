#include "acmacs-base/log.hh"
#include "acmacs-whocc/whocc-xlsx-to-torg-py.hh"
#include "acmacs-whocc/data-fix.hh"
#include "acmacs-whocc/sheet.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations" // 2.6.1 2020-12-06
#endif

PYBIND11_EMBEDDED_MODULE(data_fix_builtin_module, mdl)
{
    using namespace pybind11::literals;

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

// ----------------------------------------------------------------------

PYBIND11_EMBEDDED_MODULE(xlsx_access_builtin_module, mdl)
{
    using namespace pybind11::literals;

    py::class_<acmacs::sheet::Sheet, std::shared_ptr<acmacs::sheet::Sheet>>(mdl, "Sheet") //
        .def("name", &acmacs::sheet::Sheet::name)
        .def("number_of_rows", &acmacs::sheet::Sheet::number_of_rows)
        .def("number_of_columns", &acmacs::sheet::Sheet::number_of_columns)
        ;
}

// ----------------------------------------------------------------------

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

void acmacs::whocc_xlsx::v1::py_init(const std::vector<std::string_view>& scripts)
{
    for (const auto& script : scripts) {
        AD_INFO("import {}", script);
        py::eval_file(pybind11::str{script.data(), script.size()});
    }

} // acmacs::data_fix::v1::py_init

// ----------------------------------------------------------------------

acmacs::whocc_xlsx::v1::detect_result_t acmacs::whocc_xlsx::v1::py_sheet_detect(std::shared_ptr<acmacs::sheet::Sheet> sheet)
{
    const auto detected = py::globals()["detect"](sheet);
    detect_result_t result;
    for (const auto key : detected) {
        if (const auto key_s = key.cast<std::string>(); key_s == "lab")
            result.lab = detected[key].cast<std::string>();
        else if (key_s == "assay")
            result.assay = detected[key].cast<std::string>();
        else if (key_s == "subtype")
            result.subtype = detected[key].cast<std::string>();
        else
            AD_WARNING("py function detect returned unrecognized key/value: \"{}\": {}", key_s, static_cast<std::string>(py::str(detected[key])));
    }
    return result;

} // acmacs::whocc_xlsx::v1::py_sheet_detect

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
