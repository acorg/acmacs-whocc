// For sera-antigen titres that exist in multiple reference panels, plot the titre on the y axis against table number on the x axis.
// Colors: Green is the median, yellow is 1 log from the median and red is >1 log from the median.

#include <string>
// #include <cstdlib>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "boost/program_options.hpp"
#pragma GCC diagnostic pop

#include "acmacs-base/stream.hh"
#include "acmacs-chart/ace.hh"

// ----------------------------------------------------------------------

class Options
{
 public:
    std::vector<std::string> source_charts;
    std::string output_filename;
};

class ChartData;

static int get_args(int argc, const char *argv[], Options& aOptions);
static void process_source(ChartData& aData, std::string filename);

// ----------------------------------------------------------------------

class ChartData
{
 public:
    inline ChartData() {}
    size_t add_antigen(const Antigen& aAntigen);
    size_t add_serum(const Serum& aSerum);

 private:
    std::vector<std::string> mTables;
    std::vector<std::string> mSera;
    std::vector<std::string> mAntigens;

    friend std::ostream& operator << (std::ostream& out, const ChartData& aData);

};

// ----------------------------------------------------------------------

int main(int argc, const char *argv[])
{
    Options options;
    int exit_code = get_args(argc, argv, options);
    if (exit_code == 0) {
        try {
            ChartData data;
            for (const auto& source_name: options.source_charts)
                process_source(data, source_name);
            std::cout << data << std::endl;
        }
        catch (std::exception& err) {
            std::cerr << err.what() << std::endl;
            exit_code = 1;
        }
    }
    return exit_code;
}

// ----------------------------------------------------------------------

static int get_args(int argc, const char *argv[], Options& aOptions)
{
    using namespace boost::program_options;
    options_description desc("Options");
    desc.add_options()
            ("help", "Print help messages")
            ("output,o", value<std::string>(&aOptions.output_filename)->required(), "output pdf")
            ("sources,s", value<std::vector<std::string>>(&aOptions.source_charts), "source chart in the proper order")
            ;
    positional_options_description pos_opt;
    pos_opt.add("output", 1);
    pos_opt.add("sources", -1);

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

void process_source(ChartData& aData, std::string filename)
{
    std::unique_ptr<Chart> chart{import_chart(filename)};
      // chart->find_homologous_antigen_for_sera();
    std::vector<size_t> ref_antigens;
    chart->antigens().reference_indices(ref_antigens);
    std::map<size_t, size_t> antigens; // index in chart to index in aData.mAntigens
    for (size_t antigen_index_in_chart: ref_antigens) {
        antigens[antigen_index_in_chart] = aData.add_antigen(chart->antigen(antigen_index_in_chart));
    }
    for (const Serum& serum: chart->sera()) {
    }

} // process_source

// ======================================================================

size_t ChartData::add_antigen(const Antigen& aAntigen)
{
    const std::string name = aAntigen.full_name();
    const auto pos = std::find(mAntigens.begin(), mAntigens.end(), name);
    if (pos == mAntigens.end())
        mAntigens.push_back(name);
    return static_cast<size_t>(pos - mAntigens.begin());

} // ChartData::add_antigen

// ----------------------------------------------------------------------

size_t ChartData::add_serum(const Serum& aSerum)
{
    const std::string name = aSerum.full_name_without_passage();
    const auto pos = std::find(mSera.begin(), mSera.end(), name);
    if (pos == mSera.end())
        mSera.push_back(name);
    return static_cast<size_t>(pos - mSera.begin());

} // ChartData::add_serum

// ----------------------------------------------------------------------

std::ostream& operator << (std::ostream& out, const ChartData& aData)
{
    return out << "Sera:" << aData.mSera.size() << " Antigens:" << aData.mAntigens.size();

} // operator <<

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
