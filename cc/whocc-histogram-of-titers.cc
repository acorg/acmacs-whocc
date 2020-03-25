#include <string>
#include <map>
#include <algorithm>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-draw/surface-cairo.hh"

// ----------------------------------------------------------------------

class TiterData
{
 public:
    TiterData() {}

    void add(std::string aTiter)
        {
            auto iter_inserted = mTiters.emplace(aTiter, 1U);
            if (!iter_inserted.second)
                ++iter_inserted.first->second;
        }

    size_t max_number() const
        {
            return std::max_element(mTiters.begin(), mTiters.end(), [](const auto& a, const auto& b) { return a.second < b.second; })->second;
        }

    const acmacs::chart::Titer& longest_label() const
        {
            return std::max_element(mTiters.begin(), mTiters.end(), [](const auto& a, const auto& b) { return a.first.size() < b.first.size(); })->first;
        }

    void histogram(std::string_view filename);

 private:
    std::map<acmacs::chart::Titer, size_t> mTiters;

    friend inline std::ostream& operator<<(std::ostream& out, const TiterData& aData)
    {
        out << '{';
        std::transform(std::begin(aData.mTiters), std::end(aData.mTiters), polyfill::make_ostream_joiner(out, ", "), [](const auto& elt) {
            std::ostringstream os;
            os << '<' << *elt.first << ">: <" << elt.second << '>';
            return os.str();
        });
        return out << '}';
    }
};

// ----------------------------------------------------------------------

void process_source(TiterData& aData, std::string_view filename);

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {{"-h", false}, {"--help", false}, {"-v", false}, {"--verbose", false}});
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> ... <output.pdf>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            TiterData data;
            for (size_t file_no = 0; file_no < (args.number_of_arguments() - 1); ++file_no) {
                process_source(data, args[file_no]);
                std::cout << data << std::endl;
                std::cout << "max: " << data.max_number() << '\n';
                data.histogram(args[args.number_of_arguments() - 1]);
            }
        }
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void process_source(TiterData& aData, std::string_view filename)
{
    auto chart = acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report_time::no);
    auto chart_titers = chart->titers();
    const auto number_of_antigens = chart_titers->number_of_antigens(), number_of_sera = chart_titers->number_of_sera();
    for (size_t antigen_no = 0; antigen_no < number_of_antigens; ++antigen_no) {
        for (size_t serum_no = 0; serum_no < number_of_sera; ++serum_no) {
            aData.add(*chart_titers->titer(antigen_no, serum_no));
        }
    }

} // process_source

// ----------------------------------------------------------------------

void TiterData::histogram(std::string_view filename)
{
    const double hsize = 1000.0, vsize = hsize / 1.6;
    acmacs::surface::PdfCairo surface(std::string(filename), hsize, vsize, hsize);
    const double font_size = 10;
    const size_t max_y = max_number();
    const double y_label_max_width = surface.text_size(std::to_string(max_y), Pixels{font_size}).width;
    const size_t y_num_check_marks = 10;
    const double padding = 5.0;
    const double mark_size = padding * 0.5;
    const double bottom = vsize - padding - font_size - padding;
    const double y_extension = 1.03;
      // const double y_range = max_y * y_extension;
    const double top = padding + (bottom - padding) * (y_extension - 1.0);
    const double y_mark_step = (bottom - top) / y_num_check_marks;

    const double left = padding + y_label_max_width + padding;
    const double right = hsize - padding;

    surface.line({left, padding}, {left, bottom}, BLACK, Pixels{1});
    for (size_t y_check_mark = 0; y_check_mark < y_num_check_marks; ++y_check_mark) {
        const double mark_y = bottom - (y_check_mark + 1) * y_mark_step;
        surface.line({left - mark_size, mark_y}, {left, mark_y}, BLACK, Pixels{1});
        surface.text_right_aligned({left - mark_size, mark_y + font_size / 2}, std::to_string(size_t(double(max_y) / double(y_num_check_marks) * (y_check_mark + 1))), BLACK, Pixels{font_size});
    }

      // const size_t x_num_check_marks = mTiters.size();
    const double x_mark_step = (right - left) / mTiters.size();
    surface.line({left, bottom}, {right, bottom}, BLACK, Pixels{1});
    size_t x_check_mark = 0;
    double label_font_size = font_size;
    const double longest_label_width = surface.text_size(*longest_label(), Pixels{label_font_size}).width;
    if (longest_label_width > x_mark_step) {
        label_font_size *= x_mark_step / longest_label_width; // * 0.9;
    }
    for (const auto& entry: mTiters) {
        const double bar_left = left + x_check_mark * x_mark_step; //, bar_right = bar_left + x_mark_step;
        const double bar_height = double(entry.second) / double(max_y) * (bottom - top);
        double label_width = surface.text_size(*entry.first, Pixels{label_font_size}).width;
        surface.text({bar_left + 0.5 * x_mark_step - label_width / 2, bottom + label_font_size * 1.5}, *entry.first, BLACK, Pixels{label_font_size});
        surface.rectangle_filled({bar_left, bottom - bar_height}, {x_mark_step, bar_height}, BLACK, Pixels{0.5}, ORANGE);
        ++x_check_mark;
    }

} // TiterData::histogram

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
