#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-whocc/xlsx.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> xlsx{*this, arg_name{".xlsx"}, mandatory};
    argument<str> csv{*this, arg_name{".csv"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    using namespace acmacs;

    int exit_code = 0;
    try {
        Options opt(argc, argv);

        fmt::memory_buffer out;
        fmt::format_to(out, R"(<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8" />
        <title>{title}</title>
        <style>
         h1 {{ color: #0000A0; }}
         body {{ margin: 0; font-size: 0.9em; }}
         table {{ border-collapse: collapse; }}
         tr:hover {{ background: #EFF; }}
         td {{ position: relative; }}
         td:hover::after {{ content: ""; position: absolute; background-color: #EFF; left: 0; top: -5000px; height: 10000px; width: 100%; z-index: -1; }} /* https://css-tricks.com/simple-css-row-column-highlighting/ */
         td.col-row-no {{ font-weight: bold; text-align: center; background: #66A; color: white; }}
         td {{ padding: 0.3em; border: 1px solid #CCC; white-space: nowrap; }}
        </style>
    </head>
    <body>
)",
                       fmt::arg("title", *opt.xlsx));

        const auto col_names = [&out](size_t number_of_columns) {
            fmt::format_to(out, "<tr><td></td>");
            for (const auto col : range_from_0_to(number_of_columns))
                fmt::format_to(out, "<td class='col-row-no'>{:c}</td>", col + 'A');
            fmt::format_to(out, "</tr>\n");
        };

        auto doc = acmacs::xlsx::open(opt.xlsx);
        for (const auto sheet_no : range_from_0_to(doc.number_of_sheets())) {
            if (sheet_no)
                fmt::format_to(out, "<br><br><br><hr><br><br><br>\n");
            auto sheet = doc.sheet(sheet_no);
            fmt::format_to(out, "<h1>{}. {}</h1>\n", sheet_no + 1, sheet->name());
            fmt::format_to(out, "<table>\n");
            col_names(sheet->number_of_columns());
            for (const auto row : range_from_0_to(sheet->number_of_rows())) {
                fmt::format_to(out, "<tr><td class='col-row-no'>{}</td>", row + 1);
                std::string prev_cell;
                size_t colspan = 0;
                for (const auto col : range_from_0_to(sheet->number_of_columns())) {
                    auto cell = fmt::format("{}", sheet->cell(row, col));
                    // if (const auto cell_spans = sheet->cell_spans(row, col); !cell_spans.empty()) {
                    //     cell = fmt::format("<span style='color: {}; background: {}'>{}</span>", cell_spans[0].foreground, cell_spans[0].background, cell);
                    // }
                    if (!cell.empty()) {
                        if (colspan)
                            fmt::format_to(out, "<td colspan={}>{}</td>", colspan, prev_cell);
                        colspan = 1;
                        prev_cell = cell;
                    }
                    else
                        ++colspan;
                }
                fmt::format_to(out, "<td colspan={}>{}</td><td class='col-row-no'>{}</td></tr>\n", colspan, prev_cell, row + 1);
            }
            col_names(sheet->number_of_columns());
            fmt::format_to(out, "</table>\n");
        }
        file::write(opt.csv, fmt::to_string(out));
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
