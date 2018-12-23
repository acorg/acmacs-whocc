#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/named-type.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

inline bool is_acmacs_file(const fs::path& path)
{
    if (path.extension() == ".ace")
        return true;
    if (path.extension() == ".bz2" && path.stem().extension() == ".acd1")
        return true;
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
    using Entry = std::tuple<SerumIdRoot, SerumId, Name, Reassortant, Annotations, acmacs::chart::VirusType, acmacs::chart::Lab, acmacs::chart::Assay, acmacs::chart::RbcSpecies,
                             acmacs::chart::TableDate, Passage>;
    using Entries = std::vector<Entry>;
    using EntryPtr = typename Entries::const_iterator;
    using PerSerumIdEntry = std::tuple<EntryPtr, EntryPtr>;
    using PerSerumIdRootEntry = std::tuple<EntryPtr, EntryPtr, std::vector<PerSerumIdEntry>>;
    using PerSerumIdRootEntries = std::vector<PerSerumIdRootEntry>;

    SerumIds() = default;

    size_t size() const { return data_.size(); }

    // returns number of files processed
    size_t scan_directory(std::string dirname)
    {
        size_t charts_processed = 0;
        for (auto& entry : fs::directory_iterator(dirname)) {
            if (const auto pathname = entry.path(); entry.is_regular_file() && is_acmacs_file(pathname)) {
                auto chart = acmacs::chart::import_from_file(pathname);
                std::tuple table(chart->info()->virus_type(), chart->info()->lab(), chart->info()->assay(), chart->info()->rbc_species(), chart->info()->date());
                auto sera = chart->sera();
                for (auto serum : *sera)
                    add({serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id(), serum->passage()}, table);
                ++charts_processed;
            }
        }
        sort();
        scan();
        return charts_processed;
    }

    void print(bool print_good) const
    {
        const bool show_assay = std::get<acmacs::chart::VirusType>(*std::get<0>(per_root_.front())) == "A(H3N2)";
        const bool show_rbc = show_assay;
        for (const auto& per_root_entry : per_root_) {
            const auto name = make_name(std::get<0>(per_root_entry)), name_last = make_name(std::get<1>(per_root_entry) - 1);
            if (const bool good = std::get<2>(per_root_entry).size() == 1; !good || print_good) {
                std::cout << std::get<SerumIdRoot>(*std::get<0>(per_root_entry)) << ' ' << (std::get<1>(per_root_entry) - std::get<0>(per_root_entry)) << '\n';
                for (const auto& per_serum_id_entry : std::get<2>(per_root_entry)) {
                    const auto tabs = tables(std::get<0>(per_serum_id_entry), std::get<1>(per_serum_id_entry), show_assay, show_rbc);
                    std::cout << "    " << std::get<SerumId>(*std::get<0>(per_serum_id_entry)) << ' ' << tabs.size() << " [" << make_name(std::get<0>(per_serum_id_entry)) << ']';
                    for (const auto& table : tabs)
                        std::cout << ' ' << table;
                    std::cout << '\n';
                }
                if (!good) {
                    const auto& sids = std::get<2>(per_root_entry);
                    if (std::get<SerumId>(*std::get<0>(sids.front())) != std::get<SerumId>(*std::get<0>(sids.back()))) {
                        const auto sid = std::max_element(sids.begin(), sids.end(),
                                                          [](const auto& e1, const auto& e2) -> bool { return (std::get<1>(e1) - std::get<0>(e1)) < (std::get<1>(e2) - std::get<0>(e2)); });
                        for (auto ep = sids.begin(); ep != sids.end(); ++ep) {
                            if (ep != sid) {
                                std::cout << "  --fix ";
                                const auto sid1 = std::get<SerumId>(*std::get<0>(*ep)), sid2 = std::get<SerumId>(*std::get<0>(*sid));
                                if ((std::get<1>(*ep) - std::get<0>(*ep)) == (std::get<1>(*sid) - std::get<0>(*sid)) && sid2.size() < sid1.size())
                                    std::cout << sid2 << '^' << sid1;
                                else
                                    std::cout << sid1 << '^' << sid2;
                                std::cout << '\n';
                            }
                        }
                    }
                }
            }
        }
    }

  private:
    Entries data_;
    PerSerumIdRootEntries per_root_;

    void sort() { std::sort(data_.begin(), data_.end()); /* data_.erase(std::unique(data_.begin(), data_.end()), data_.end()); */ }

    void add(const SerumEntry& serum, const TableEntry& table)
    {
        data_.emplace_back(serum_id_root(serum, table), std::get<SerumId>(serum), std::get<Name>(serum), std::get<Reassortant>(serum), std::get<Annotations>(serum),
                           std::get<acmacs::chart::VirusType>(table), std::get<acmacs::chart::Lab>(table), std::get<acmacs::chart::Assay>(table), std::get<acmacs::chart::RbcSpecies>(table),
                           std::get<acmacs::chart::TableDate>(table), std::get<Passage>(serum));
    }

    void scan()
    {
        for (EntryPtr entry_ptr = data_.begin(); entry_ptr != data_.end(); ++entry_ptr) {
            if (per_root_.empty() || std::get<SerumIdRoot>(*entry_ptr) != std::get<SerumIdRoot>(*std::get<0>(per_root_.back()))) {
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

    static inline std::string make_name(EntryPtr ptr) { return string::join({std::get<Name>(*ptr), std::get<Reassortant>(*ptr), string::join(" ", std::get<Annotations>(*ptr))}); }

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

}; // class SerumIds

// ======================================================================

class FixSerumIds
{
  public:
    FixSerumIds(std::string_view program_name) : program_name_(program_name) {}
    void add_fix(std::string_view fix_entry)
    {
        const auto fields = acmacs::string::split(fix_entry, "^");
        data_.emplace_back(fields[0], fields[1]);
    }

    // returns number of files updated
    size_t fix_charts_in_directory(std::string dirname)
    {
        size_t fixed_charts = 0;
        for (auto& entry : fs::directory_iterator(dirname)) {
            if (const auto pathname = entry.path(); entry.is_regular_file() && is_acmacs_file(pathname)) {
                acmacs::chart::ChartModify chart(acmacs::chart::import_from_file(pathname));
                if (fix_in_chart(chart)) {
                    acmacs::file::backup(pathname);
                    acmacs::chart::export_factory(chart, make_export_filename(pathname), fs::path(program_name_).filename(), report_time::no);
                    ++fixed_charts;
                }
            }
        }
        return fixed_charts;
    }

  private:
    std::string_view program_name_;
    std::vector<std::pair<std::string, std::string>> data_;

    bool fix_in_chart(acmacs::chart::ChartModify& chart)
    {
        auto sera = chart.sera_modify();
        bool modified = false;
        for (auto serum : *sera) {
            const auto sid = serum->serum_id();
            for (const auto& fix : data_) {
                if (sid == fix.first) {
                    std::cout << chart.info()->make_name() << " FIX " << serum->name() << ' ' << serum->reassortant() << ' ' << string::join(" ", serum->annotations()) << ' ' << serum->serum_id()
                              << " --> " << fix.second << '\n';
                    break;
                }
            }
        }
        return modified;
    }

    fs::path make_export_filename(const fs::path& pathname) const
    {
        if (pathname.extension() == ".ace")
            return pathname;
        if (pathname.extension() == ".bz2" && pathname.stem().extension() == ".acd1")
            return pathname.stem().stem() += ".ace";
        throw std::runtime_error("cannot figure out export filename for: " + pathname.string());
    }

}; // class FixSerumIds

// ======================================================================

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> fix{*this, "fix"};
    // argument<str> period{*this, arg_name{"monthly|yearly|weekly"}, mandatory};
    // argument<Date> start{*this, arg_name{"start-date"}, mandatory};
    // argument<Date> end{*this, arg_name{"end-date"}, mandatory};
    // argument<str> output{*this, arg_name{"output.json"}, dflt{""}};
};

// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::vector<std::pair<std::string, std::string>> fix_data;
        if (opt.fix.has_value()) {
            FixSerumIds fix_serum_ids(opt.program_name());
            for (const auto& fix_entry : opt.fix.get())
                fix_serum_ids.add_fix(fix_entry);
            const auto charts_modified = fix_serum_ids.fix_charts_in_directory(".");
            std::cerr << "INFO: charts modified: " << charts_modified << '\n';
        }
        else {
            SerumIds serum_ids;
            const auto charts_processed = serum_ids.scan_directory(".");
            std::cout << charts_processed << " charts processed\n";
            serum_ids.print(false);
        }
        // for (auto& entry : fs::directory_iterator(".")) {
        //     if (const auto pathname = entry.path(); entry.is_regular_file() && is_acmacs_file(pathname)) {
        //         // std::cout << entry.path() << '\n';
        //         auto chart = acmacs::chart::import_from_file(pathname);
        //         if (!opt.fix.has_value()) { // scan
        //             std::tuple table(chart->info()->virus_type(), chart->info()->lab(), chart->info()->assay(), chart->info()->rbc_species(), chart->info()->date());
        //             auto sera = chart->sera();
        //             for (auto serum : *sera)
        //                 serum_ids.add({serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id(), serum->passage()}, table);
        //         }
        //         else { // fix
        //             acmacs::chart::ChartModify chart_modify{chart};
        //             auto sera = chart_modify.sera_modify();
        //             for (auto serum : *sera) {
        //                 const auto sid = serum->serum_id();
        //                 for (const auto& fix : fix_data) {
        //                     if (sid == fix.first) {
        //                         std::cout << chart->info()->make_name() << " FIX " << serum->name() << ' ' << serum->reassortant() << ' ' << string::join(" ", serum->annotations()) << ' '
        //                                   << serum->serum_id() << " --> " << fix.second << '\n';
        //                         break;
        //                     }
        //                 }
        //             }
        //         }
        //         ++charts_processed;
        //     }
        // }
        // std::cout << charts_processed << " charts processed\n";
        // if (!opt.fix.has_value()) { // scan
        //     std::cout << serum_ids.size() << " entries\n";
        //     serum_ids.sort();
        //     serum_ids.scan();
        //     // std::cout << serum_ids.size() << " entries\n";
        //     serum_ids.print(false);
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
