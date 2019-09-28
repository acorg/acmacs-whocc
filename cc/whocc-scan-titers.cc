#include <string>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"

// ----------------------------------------------------------------------

void find_ace_files(const fs::path& source_dir, std::vector<fs::path>& ace_files);
void scan_titers(const fs::path& filename, std::set<acmacs::chart::Titer>& titers);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> source_dir{*this, arg_name{"source-dir"}, mandatory};
};

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::vector<fs::path> ace_files;
        find_ace_files(fs::path(*opt.source_dir), ace_files);
        fmt::print("Total .ace files found: {}\n", ace_files.size());
        std::set<acmacs::chart::Titer> titers;
        for (const auto& filename : ace_files)
            scan_titers(filename, titers);
        fmt::print("{}\n", titers);
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void find_ace_files(const fs::path& source_dir, std::vector<fs::path>& ace_files)
{
    if (!fs::is_directory(source_dir))
        throw std::runtime_error(source_dir.string() + " is not a directory");
    for (const auto& dirent: fs::directory_iterator(source_dir)) {
        if (fs::is_directory(dirent.status()))
            find_ace_files(dirent.path(), ace_files);
        else if (is_regular_file(dirent.status()) && dirent.path().extension().string() == ".ace")
            ace_files.push_back(dirent.path());
    }

} // find_ace_files

// ----------------------------------------------------------------------

void scan_titers(const fs::path& filename, std::set<acmacs::chart::Titer>& titers)
{
    auto chart = acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report_time::no);
    auto chart_titers = chart->titers();
    const auto number_of_antigens = chart_titers->number_of_antigens(), number_of_sera = chart_titers->number_of_sera();
    for (size_t antigen_no = 0; antigen_no < number_of_antigens; ++antigen_no) {
        for (size_t serum_no = 0; serum_no < number_of_sera; ++serum_no) {
            titers.insert(chart_titers->titer(antigen_no, serum_no));
        }
    }

} // scan_titers

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
