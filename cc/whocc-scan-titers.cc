#include <string>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"

// ----------------------------------------------------------------------

void find_ace_files(const fs::path& source_dir, std::vector<fs::path>& ace_files);
void scan_titers(const fs::path& filename, std::set<acmacs::chart::Titer>& titers);

// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {{"-h", false}, {"--help", false}, {"-v", false}, {"--verbose", false}});
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <source-dir>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            std::vector<fs::path> ace_files;
            find_ace_files(fs::path(args[0]), ace_files);
            std::cout << "Total .ace files found: " << ace_files.size() << '\n';
            std::set<acmacs::chart::Titer> titers;
            for (const auto& filename : ace_files)
                scan_titers(filename, titers);
            std::cout << titers << std::endl;
        }
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
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
    auto chart = acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report_time::No);
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
