#include <string>
#include <map>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "boost/program_options.hpp"
#pragma GCC diagnostic pop

#include "acmacs-base/stream.hh"
#include "acmacs-chart/ace.hh"

// ----------------------------------------------------------------------

class TiterData
{
 public:
    inline TiterData() {}

    inline void add(std::string aTiter)
        {
            auto iter_inserted = mTiters.emplace(aTiter, 1U);
            if (!iter_inserted.second)
                ++iter_inserted.first->second;
        }

 private:
    std::map<Titer, size_t> mTiters;

    friend inline std::ostream& operator << (std::ostream& out, const TiterData& aData)
        {
            return out << aData.mTiters;
        }
};

// ----------------------------------------------------------------------

void process_source(TiterData& aData, std::string filename);

class Options
{
 public:
    std::vector<std::string> source_charts;
    std::string output_filename;
};

static int get_args(int argc, const char *argv[], Options& aOptions);

int main(int argc, const char *argv[])
{
    Options options;
    int exit_code = get_args(argc, argv, options);
    if (exit_code == 0) {
        try {
            TiterData data;
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

static int get_args(int argc, const char *argv[], Options& aOptions)
{
    using namespace boost::program_options;
    options_description desc("Options");
    desc.add_options()
            ("help", "Print help messages")
            ("output,o", value<std::string>(&aOptions.output_filename)->required(), "output pdf")
            ("sources,s", value<std::vector<std::string>>(&aOptions.source_charts), "source charts")
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

void process_source(TiterData& aData, std::string filename)
{
    std::unique_ptr<Chart> chart{import_chart(filename)};
    if (chart->titers().list().empty()) {
        for (const auto& row: chart->titers().dict()) {
            for (const auto& serum_titer: row)
                aData.add(serum_titer.second);
        }
    }
    else {
        for (const auto& row: chart->titers().list()) {
            for (const auto& titer: row)
                aData.add(titer);
        }
    }

} // process_source

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
