#include <string>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "boost/program_options.hpp"
#pragma GCC diagnostic pop

#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/factory-import.hh"

// ----------------------------------------------------------------------

void find_ace_files(const fs::path& source_dir, std::vector<fs::path>& ace_files);
void scan_titers(const fs::path& filename, std::set<acmacs::chart::Titer>& titers);

// ----------------------------------------------------------------------

class Options
{
 public:
    std::string source_dir;
};

static int get_args(int argc, const char *argv[], Options& aOptions);

int main(int argc, const char *argv[])
{
    Options options;
    int exit_code = get_args(argc, argv, options);
    if (exit_code == 0) {
        try {
            std::vector<fs::path> ace_files;
            find_ace_files(fs::path(options.source_dir), ace_files);
            std::cout << "Total .ace files found: " << ace_files.size() << std::endl;
            std::set<acmacs::chart::Titer> titers;
            for (const auto& filename: ace_files)
                scan_titers(filename, titers);
            std::cout << titers << std::endl;
        }
        catch (std::exception& err) {
            std::cerr << err.what() << std::endl;
            exit_code = 1;
        }
    }
    return exit_code;
}

static int get_args(int argc, const char *argv[], Options& aOptions)
{
    using namespace boost::program_options;
    options_description desc("Options");
    desc.add_options()
            ("help", "Print help messages")
            ("source,s", value<std::string>(&aOptions.source_dir)->required(), "directory to scan for ace files (recursively)")
            ;
    positional_options_description pos_opt;
    pos_opt.add("source", 1);

    variables_map vm;
    try {
        store(command_line_parser(argc, argv).options(desc).positional(pos_opt).run(), vm);
        if (vm.count("help")) {
            std::cerr << desc << std::endl;
            return 1;
        }
        notify(vm);
        return 0;
    }
    catch(required_option& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
          // std::cerr << "Usage: " << argv[0] << " <tree.json> <output.pdf>" << std::endl;
        return 2;
    }
    catch(error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        return 3;
    }

} // get_args

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
    auto chart = acmacs::chart::import_factory(filename, acmacs::chart::Verify::None);
    auto chart_titers = chart->titers();
    for (size_t antigen_no = 0; antigen_no < chart_titers->number_of_antigens(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < chart_titers->number_of_sera(); ++serum_no) {
            titers.insert(chart_titers->titer(antigen_no, serum_no));
        }
    }

} // scan_titers

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
