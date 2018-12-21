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
    using Entry = std::tuple<SerumIdRoot, SerumId, Name, Reassortant, Annotations, acmacs::chart::Lab, acmacs::chart::Assay, acmacs::chart::RbcSpecies, acmacs::chart::TableDate, Passage>;

    SerumIds() = default;

    constexpr size_t size() const { return data_.size(); }
    void sort() { std::sort(data_.begin(), data_.end()); /* data_.erase(std::unique(data_.begin(), data_.end()), data_.end()); */ }

    void add(const SerumEntry& serum, const TableEntry& table)
        {
            data_.emplace_back(serum_id_root(serum, table), std::get<SerumId>(serum),
                               std::get<Name>(serum), std::get<Reassortant>(serum), std::get<Annotations>(serum),
                               std::get<acmacs::chart::Lab>(table), std::get<acmacs::chart::Assay>(table), std::get<acmacs::chart::RbcSpecies>(table), std::get<acmacs::chart::TableDate>(table),
                               std::get<Passage>(serum));
        }

    void print() const
        {
            for (const auto& entry : data_)
                std::cout << std::get<SerumIdRoot>(entry) << ' ' << std::get<SerumId>(entry) << ' ' << std::get<Name>(entry) << ' ' << std::get<acmacs::chart::TableDate>(entry) << '\n';
        }
    
 private:
    std::vector<Entry> data_;

    SerumIdRoot serum_id_root(const SerumEntry& serum, const TableEntry& table) const
        {
            const auto& serum_id = std::get<SerumId>(serum);
            if (std::get<acmacs::chart::Lab>(table) == "MELB") {
                if (serum_id.size() > 6 && (serum_id[0] == 'F' || serum_id[0] == 'R') && serum_id[5] == '-' && serum_id.back() == 'D')
                    return SerumIdRoot(serum_id.substr(0, 5));
                else
                    return SerumIdRoot(serum_id);
            }
            else
                return SerumIdRoot(serum_id);
        }

};

// ----------------------------------------------------------------------

int main(int /*argc*/, const char* /*argv*/[])
{
    int exit_code = 0;
    try {
        // Options opt(argc, argv);
        SerumIds serum_ids;
        size_t charts_processed = 0;
        for (auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file() && is_acmacs_file(entry.path())) {
                // std::cout << entry.path() << '\n';
                auto chart = acmacs::chart::import_from_file(entry.path());
                std::tuple table(chart->info()->lab(), chart->info()->assay(), chart->info()->rbc_species(), chart->info()->date());
                auto sera = chart->sera();
                for (auto serum : *sera)
                    serum_ids.add({serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id(), serum->passage()}, table);
                  // name_id.emplace_back(serum->designation_without_serum_id(), serum->serum_id(), date);
                ++charts_processed;
            }
        }
        std::cout << charts_processed << " charts processed\n";
        std::cout << serum_ids.size() << " entries\n";
        serum_ids.sort();
          // std::cout << serum_ids.size() << " entries\n";
        serum_ids.print();
        
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
