#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/named-type.hh"
#include "acmacs-base/string.hh"
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
    using TableEntry = std::tuple<acmacs::chart::VirusType, acmacs::chart::Lab, acmacs::chart::Assay, acmacs::chart::RbcSpecies, acmacs::chart::TableDate>;
    using Entry = std::tuple<SerumIdRoot, SerumId, Name, Reassortant, Annotations, acmacs::chart::VirusType, acmacs::chart::Lab, acmacs::chart::Assay, acmacs::chart::RbcSpecies, acmacs::chart::TableDate, Passage>;
    using Entries = std::vector<Entry>;
    using EntryPtr = typename Entries::const_iterator;
    using PerSerumIdEntry = std::tuple<EntryPtr, EntryPtr>;
    using PerSerumIdRootEntry = std::tuple<EntryPtr, EntryPtr, std::vector<PerSerumIdEntry>>;
    using PerSerumIdRootEntries = std::vector<PerSerumIdRootEntry>;
    
    SerumIds() = default;

    size_t size() const { return data_.size(); }
    void sort() { std::sort(data_.begin(), data_.end()); /* data_.erase(std::unique(data_.begin(), data_.end()), data_.end()); */ }

    void add(const SerumEntry& serum, const TableEntry& table)
        {
            data_.emplace_back(serum_id_root(serum, table), std::get<SerumId>(serum),
                               std::get<Name>(serum), std::get<Reassortant>(serum), std::get<Annotations>(serum),
                               std::get<acmacs::chart::VirusType>(table), std::get<acmacs::chart::Lab>(table), std::get<acmacs::chart::Assay>(table), std::get<acmacs::chart::RbcSpecies>(table), std::get<acmacs::chart::TableDate>(table),
                               std::get<Passage>(serum));
        }

    void scan()
        {
            for (EntryPtr entry_ptr = data_.begin(); entry_ptr != data_.end(); ++entry_ptr) {
                if (per_root_.empty() || std::get<SerumIdRoot>(*entry_ptr) !=  std::get<SerumIdRoot>(*std::get<0>(per_root_.back()))) {
                    per_root_.emplace_back(entry_ptr, entry_ptr + 1, std::vector<PerSerumIdEntry>{{entry_ptr, entry_ptr + 1}});
                }
                else {
                    std::get<1>(per_root_.back()) = entry_ptr + 1;
                    const auto last = std::get<0>(std::get<2>(per_root_.back()).back());
                    const auto name = make_name(entry_ptr), name_last = make_name(last);
                    if (std::get<SerumId>(*entry_ptr) != std::get<SerumId>(*last) || name != name_last) {
                        std::get<2>(per_root_.back()).emplace_back(entry_ptr, entry_ptr + 1);
                    }
                    else {
                        std::get<1>(std::get<2>(per_root_.back()).back()) = entry_ptr + 1;
                    }
                }
            }
            std::cout << "per_root_ " << per_root_.size() << '\n';
        }

        void print(bool print_good) const
        {
            // for (const auto& entry : data_)
            //     std::cout << std::get<SerumIdRoot>(entry) << ' ' << std::get<SerumId>(entry) << ' ' << std::get<Name>(entry) << ' ' << std::get<acmacs::chart::TableDate>(entry) << '\n';

            const bool show_assay = std::get<acmacs::chart::VirusType>(*std::get<0>(per_root_.front())) == "A(H3N2)";
            const bool show_rbc = show_assay;
            for (const auto& per_root_entry : per_root_) {
                const auto name = make_name(std::get<0>(per_root_entry)), name_last = make_name(std::get<1>(per_root_entry) - 1);
                if (const bool good = std::get<2>(per_root_entry).size() == 1; !good || print_good) {
                    std::cout << std::get<SerumIdRoot>(*std::get<0>(per_root_entry)) << ' ' << (std::get<1>(per_root_entry) - std::get<0>(per_root_entry)) << '\n';
                    for (const auto& per_serum_id_entry : std::get<2>(per_root_entry)) {
                        const auto tabs = tables(std::get<0>(per_serum_id_entry), std::get<1>(per_serum_id_entry), show_assay, show_rbc);
                        std::cout << "    " << std::get<SerumId>(*std::get<0>(per_serum_id_entry)) << ' ' << tabs.size()
                                  << " [" << make_name(std::get<0>(per_serum_id_entry)) << ']';
                        for (const auto& table : tabs)
                            std::cout << ' ' << table;
                        std::cout << '\n';
                    }
                }
            }
        }

 private:
    Entries data_;
    PerSerumIdRootEntries per_root_;

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

    static inline std::string make_name(EntryPtr ptr)
        {
            return string::join({std::get<Name>(*ptr), std::get<Reassortant>(*ptr), string::join(" ", std::get<Annotations>(*ptr))});
        }

    static inline std::vector<std::string> tables(EntryPtr first, EntryPtr last, bool assay, bool rbc)
    {
        std::vector<std::string> tables(static_cast<size_t>(last - first));
        std::transform(first, last, tables.begin(), [assay, rbc](const auto& entry) {
            std::vector<std::string> fields;
            if (assay)
                fields.push_back(std::get<acmacs::chart::Assay>(entry));
            if (rbc)
                fields.push_back(std::get<acmacs::chart::RbcSpecies>(entry));
            fields.push_back(std::get<acmacs::chart::TableDate>(entry));
            return string::join(":", fields);
        });
        std::reverse(tables.begin(), tables.end());
        return tables;
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
                std::tuple table(chart->info()->virus_type(), chart->info()->lab(), chart->info()->assay(), chart->info()->rbc_species(), chart->info()->date());
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
        serum_ids.scan();
          // std::cout << serum_ids.size() << " entries\n";
        serum_ids.print(false);
        
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
