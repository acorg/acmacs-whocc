#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/named-type.hh"
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

class SerumIds
{
 public:
    using Name = acmacs::chart::Name;
    using Passage = acmacs::chart::Passage;
    using Reassortant = acmacs::chart::Reassortant;
    using Annotations = acmacs::chart::Annotations;
    using SerumId = acmacs::chart::SerumId;
    using SerumIdRoot = acmacs::named_t<std::string, struct SerumIdRootTag>;
    using SerumEntry = std::tuple<Name, Reassortant, Annotations, SerumId, Passage>;
    using TableEntry = std::tuple<acmacs::chart::Lab, acmacs::chart::Assay, acmacs::chart::RbcSpecies, acmacs::chart::TableDate>;
    
    SerumIds() = default;

      // void add(const Name& name, const Reassortant& reassortant, const Annotations& annotations, const SerumId& serum_id, const Passage& passage);
    void add(const SerumEntry& serum, const TableEntry& table);

 private:
    std::map<SerumIdRoot, std::map<SerumId, std::vector<std::tuple<SerumEntry, TableEntry>>>> data_;

    SerumIdRoot serum_id_root(const SerumEntry& serum, const TableEntry& table) const
        {
            return SerumIdRoot(std::get<SerumId>(serum));
        }
    
};

struct NameId
{
  NameId(std::string n, std::string si, std::string d) : name(n), serum_id(si), date(d) {}
  bool operator<(const NameId& rhs) const { if (name == rhs.name) { if (serum_id == rhs.serum_id) return date < rhs.date; else return serum_id < rhs.serum_id; } else return name < rhs.name; }
  bool operator==(const NameId& rhs) const { return name == rhs.name && serum_id == rhs.serum_id && date == rhs.date; }
  bool operator!=(const NameId& rhs) const { return !operator==(rhs); }
  std::string name;
  std::string serum_id;
  std::string date;
};

struct NameIds
{
  NameIds(std::string n) : name(n) {}
  std::string name;
  std::vector<std::pair<std::string, size_t>> id_count;
};

// ----------------------------------------------------------------------

int main(int /*argc*/, const char* /*argv*/[])
{
    int exit_code = 0;
    try {
        // Options opt(argc, argv);
        std::vector<NameId> name_id;
        size_t charts_processed = 0;
        for (auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file() && is_acmacs_file(entry.path())) {
                // std::cout << entry.path() << '\n';
                auto chart = acmacs::chart::import_from_file(entry.path());
                const std::string date = chart->info()->date();
                auto sera = chart->sera();
                for (auto serum : *sera)
                    name_id.emplace_back(serum->designation_without_serum_id(), serum->serum_id(), date);
                ++charts_processed;
            }
        }
        std::cout << charts_processed << " charts processed\n";
        std::sort(name_id.begin(), name_id.end());
        // std::cout << name_id.size() << " sera found\n";
        // name_id.erase(std::unique(name_id.begin(), name_id.end()),
        // name_id.end());
        std::cout << name_id.size() << " sera found\n";

        // std::vector<std::pair<std::string, std::vector<std::string>>> name_ids;
        // for (const auto& entry : name_id) {
        //     if (name_ids.empty() || name_ids.back().first != entry.first)
        //         name_ids.emplace_back(entry.first, std::vector<std::string>{entry.second});
        //     else
        //         name_ids.back().second.push_back(entry.second);
        // }
        // std::cout << name_ids.size() << " serum names found\n";

        // for (const auto& entry : name_ids) {
        //     std::cout << entry.first << '\n';
        //     for (const auto& serum_id : entry.second) {
        //         std::cout << "  " << serum_id;
        //         if (serum_id[0] == 'F' || serum_id[0] == 'R') {
        //             if (serum_id.back() != 'D')
        //                 std::cout << "  Warning: FIX!";
        //         }
        //         else
        //             std::cout << "  Warning: not MELB";
        //         std::cout << '\n';
        //     }
        // }
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
