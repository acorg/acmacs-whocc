#include <iostream>
#include <string>
#include <vector>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

// using namespace acmacs::argv;

// struct Options : public argv
// {
//     Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

//     argument<str> period{*this, arg_name{"monthly|yearly|weekly"}, mandatory};
//     argument<Date> start{*this, arg_name{"start-date"}, mandatory};
//     argument<Date> end{*this, arg_name{"end-date"}, mandatory};
//     argument<str> output{*this, arg_name{"output.json"}, dflt{""}};
// };

// ----------------------------------------------------------------------

inline bool is_acmacs_file(const fs::path& path)
{
  if (path.extension() == ".ace")
    return true;
  if (path.extension() == ".bz2") {
    if (path.stem().extension() == ".acd1")
      return true;
  }
  return false;
}

// ----------------------------------------------------------------------

int main(int /*argc*/, const char* /*argv*/[])
{
  int exit_code = 0;
  try {
    // Options opt(argc, argv);
    std::vector<std::pair<std::string, std::string>> name_id;
    size_t charts_processed = 0;
    for (auto& entry : fs::directory_iterator(".")) {
      if (entry.is_regular_file() && is_acmacs_file(entry.path())) {
        // std::cout << entry.path() << '\n';
        auto chart = acmacs::chart::import_from_file(entry.path());
        auto sera = chart->sera();
        for (auto serum : *sera)
          name_id.emplace_back(serum->name(), serum->serum_id());
        ++charts_processed;
      }
    }
    std::cout << charts_processed << " charts processed\n";
    std::sort(name_id.begin(), name_id.end());
    name_id.erase(std::unique(name_id.begin(), name_id.end()), name_id.end());
    std::cout << name_id.size() << " sera found\n";
    for (const auto& entry : name_id)
      std::cout << entry.first << ' ' << entry.second << '\n';
  }
  catch (std::exception& err) {
    std::cerr << "ERROR: " << err.what() << '\n';
    exit_code = 2;
  }
  return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
