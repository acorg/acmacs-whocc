#include <limits>

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
        "date_fix",
        [](std::string& rex, std::string& replacement) {
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::Date>(std::move(rex), std::move(replacement)));
        },
        "rex"_a, "replacement"_a);

    mdl.def(
        "serum_id_fix",
        [](std::string& rex, std::string& replacement) {
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::SerumId>(std::move(rex), std::move(replacement)));
        },
        "rex"_a, "replacement"_a);

    mdl.def(
        "titer_fix",
        [](std::string& rex, std::string& replacement) {
            // AD_DEBUG("titer_fix \"{}\" \"{}\"", rex, replacement);
            acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::Titer>(std::move(rex), std::move(replacement)));
        },
        "rex"_a, "replacement"_a);
}

// ----------------------------------------------------------------------

PYBIND11_EMBEDDED_MODULE(xlsx_access_builtin_module, mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::sheet;

    py::class_<Sheet, std::shared_ptr<Sheet>>(mdl, "Sheet")                                      //
        .def("name", &Sheet::name)                                                               //
        .def("number_of_rows", [](const Sheet& sheet) { return *sheet.number_of_rows(); })       //
        .def("number_of_columns", [](const Sheet& sheet) { return *sheet.number_of_columns(); }) //

        .def(
            "cell_as_str", [](const Sheet& sheet, size_t row, size_t column) { return fmt::format("{}", sheet.cell(nrow_t{row}, ncol_t{column})); }, "row"_a, "column"_a) //

        .def(
            "grep",
            [](const Sheet& sheet, const std::string& rex, size_t min_row, size_t max_row, size_t min_col, size_t max_col) {
                if (max_row == max_row_col)
                    max_row = *sheet.number_of_rows();
                else
                    ++max_row;
                if (max_col == max_row_col)
                    max_col = *sheet.number_of_columns();
                else
                    ++max_col;
                return sheet.grep(std::regex(rex, acmacs::regex::icase), {nrow_t{min_row}, ncol_t{min_col}}, {nrow_t{max_row}, ncol_t{max_col}});
            },                                                                                                 //
            "regex"_a, "min_row"_a = 0, "max_row"_a = max_row_col, "min_col"_a = 0, "max_col"_a = max_row_col, //
            py::doc("max_row and max_col are the last row and col to look in"))                                //
        ;

    py::class_<cell_match_t>(mdl, "cell_match_t") //
        .def_property_readonly("row", [](const cell_match_t& cm) { return *cm.row; })
        .def_property_readonly("col", [](const cell_match_t& cm) { return *cm.col; })
        .def_readonly("matches", &cell_match_t::matches)
        .def("__repr__", [](const cell_match_t& cm) { return fmt::format("<cell_match_t: {}:{} {}>", cm.row, cm.col, cm.matches); });
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
    // AD_DEBUG("detected: {}", detected);
    detect_result_t result;
    std::string date;
    for (const auto key : detected) {
        if (const auto key_s = key.cast<std::string>(); key_s == "lab")
            result.lab = detected[key].cast<std::string>();
        else if (key_s == "assay")
            result.assay = detected[key].cast<std::string>();
        else if (key_s == "subtype")
            result.subtype = detected[key].cast<std::string>();
        else if (key_s == "lineage")
            result.lineage = detected[key].cast<std::string>();
       else if (key_s == "rbc")
            result.rbc = detected[key].cast<std::string>();
        else if (key_s == "date")
            date = detected[key].cast<std::string>();
        else if (key_s == "sheet_format")
            result.sheet_format = detected[key].cast<std::string>();
        else if (key_s == "ignore")
            result.ignore = detected[key].cast<bool>();
        else
            AD_WARNING("py function detect returned unrecognized key/value: \"{}\": {}", key_s, static_cast<std::string>(py::str(detected[key])));
    }
    if (!date.empty())
        result.date = date::from_string(date, date::allow_incomplete::no, date::throw_on_error::no, result.lab == "CDC" ? date::month_first::yes : date::month_first::no);
    return result;

} // acmacs::whocc_xlsx::v1::py_sheet_detect

// ----------------------------------------------------------------------
