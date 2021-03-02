#include <stack>

#include "acmacs-base/read-file.hh"
#include "acmacs-whocc/csv-parser.hh"

enum class state {
    cell,
    quoted,
    escaped
};

constexpr const char separator{','};
constexpr const char quote{'"'};
constexpr const char escape{'\\'};

// ----------------------------------------------------------------------

acmacs::xlsx::v1::csv::Sheet::Sheet(std::string_view filename)
{
    const std::string src{acmacs::file::read(filename)};

    const auto convert_cell = [&]() {};

    const auto new_cell = [&]() {
        convert_cell();
        data_.back().emplace_back(std::string{});
    };

    const auto new_row = [&]() {
        convert_cell();
        number_of_columns_ = std::max(number_of_columns_, sheet::ncol_t{data_.back().size()});
        data_.emplace_back().emplace_back(std::string{});
    };

    const auto append = [&](char sym) {
        std::visit(
            [sym]<typename Content>(Content& content) {
                if constexpr (std::is_same_v<Content, std::string>)
                    content.push_back(sym);
            },
            data_.back().back());
    };

    std::stack<enum state> states;
    states.push(state::cell);
    data_.emplace_back().emplace_back(std::string{});
    for (const char sym : src) {
        if (states.top() == state::escaped) {
            states.pop();
            append(sym);
        }
        else {
            switch (sym) {
                case separator:
                    if (states.top() == state::quoted)
                        append(sym);
                    else
                        new_cell();
                    break;
                case '\n':
                    if (states.top() == state::quoted)
                        append(sym);
                    else
                        new_row();
                    break;
                case quote:
                    if (states.top() == state::quoted)
                        states.pop();
                    else
                        states.push(state::quoted);
                    break;
                case escape:
                    states.push(state::escaped);
                    break;
                default:
                    append(sym);
                    break;
            }
        }
    }

    AD_DEBUG("rows: {} cols: {}", number_of_rows(), number_of_columns());
    for (const auto& row : data_) {
        bool first{true};
        for (const auto& cell : row) {
            if (first)
                first = false;
            else
                fmt::print("|");
            fmt::print("{}", cell);
        }
        fmt::print("\n");
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
