#include "acmacs-whocc/log.hh"
#include "acmacs-whocc/data-fix.hh"
#include "acmacs-whocc/sheet-extractor.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

static std::unique_ptr<acmacs::data_fix::v1::Set> sSet;

#pragma GCC diagnostic pop

const acmacs::data_fix::v1::Set& acmacs::data_fix::v1::Set::get()
{
    if (!sSet)
        sSet.reset(new Set{}); // throw std::runtime_error { "acmacs::data_fix::v1::Set accessed via get() before creating" };
    return *sSet;

} // acmacs::data_fix::v1::Set::get

// ----------------------------------------------------------------------

acmacs::data_fix::v1::Set& acmacs::data_fix::v1::Set::update()
{
    if (!sSet)
        sSet.reset(new Set{});
    return *sSet;

} // acmacs::data_fix::v1::Set::update

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::Set::fix(acmacs::sheet::antigen_fields_t& antigen, size_t antigen_no)
{
    using namespace std::string_view_literals;

    auto& data = get().data_;

    for (const auto& en : data) {
        const auto orig = antigen.name;
        if (const auto res = en->antigen_name(antigen.name); res) {
            AD_INFO("AG {:4d} name \"{}\" <-- \"{}\"", antigen_no, antigen.name, orig);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_name = antigen.name;
        const auto orig_passage = antigen.passage;
        if (const auto res = en->antigen_passage(antigen.passage, antigen.name); res) {
            AD_INFO("AG {:4d} passage \"{}\" <-- \"{}\"   name \"{}\" <-- \"{}\"", antigen_no, antigen.passage, orig_passage, antigen.name, orig_name);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_date = antigen.date;
        if (const auto res = en->date(antigen.date); res) {
            AD_INFO("AG {:4d} date \"{}\" <-- \"{}\"", antigen_no, antigen.date, orig_date);
            break;
        }
    }

} // acmacs::data_fix::v1::Set::fix

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::Set::fix(acmacs::sheet::serum_fields_t& serum, size_t serum_no)
{
    auto& data = get().data_;

    // AD_DEBUG("fix serum.name \"{}\"", serum.name);
    for (const auto& en : data) {
        const auto orig = serum.name;
        if (const auto res = en->serum_name(serum.name); res) {
            AD_INFO("SR {:4d} name \"{}\" <-- \"{}\"", serum_no, serum.name, orig);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_name = serum.name;
        const auto orig_passage = serum.passage;
        if (const auto res = en->serum_passage(serum.passage, serum.name); res) {
            AD_INFO("SR {:4d} passage \"{}\" <-- \"{}\"   name \"{}\" <-- \"{}\"", serum_no, serum.passage, orig_passage, serum.name, orig_name);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_serum_id = serum.serum_id;
        if (const auto res = en->serum_id(serum.serum_id); res) {
            AD_INFO("SR {:4d} serum_id \"{}\" <-- \"{}\"", serum_no, serum.serum_id, orig_serum_id);
            break;
        }
    }

} // acmacs::data_fix::v1::Set::fix

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::Set::fix_titer(std::string& titer, size_t antigen_no, size_t serum_no)
{
    for (const auto& en : get().data_) {
        const auto orig = titer;
        if (const auto res = en->titer(titer); res) {
            AD_INFO("AG {:4d} SR {:4d} titer \"{}\" <-- \"{}\" ", antigen_no, serum_no, titer, orig);
            break;
        }
    }

} // acmacs::data_fix::v1::Set::fix_titer

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
