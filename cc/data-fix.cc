#include "acmacs-base/regex.hh"
#include "acmacs-base/guile.hh"
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
        throw std::runtime_error { "acmacs::data_fix::v1::Set accessed via get() before creating" };
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

void acmacs::data_fix::v1::Set::fix(acmacs::sheet::antigen_fields_t& antigen)
{
    using namespace std::string_view_literals;

    auto& data = get().data_;

    for (const auto& en : data) {
        const auto orig = antigen.name;
        if (const auto res = en->antigen_name(antigen.name); res) {
            AD_INFO("AG name \"{}\" <-- \"{}\"", antigen.name, orig);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_name = antigen.name;
        const auto orig_passage = antigen.passage;
        if (const auto res = en->antigen_passage(antigen.passage, antigen.name); res) {
            AD_INFO("AG passage \"{}\" <-- \"{}\"   name \"{}\" <-- \"{}\"", antigen.passage, orig_passage, antigen.name, orig_name);
            break;
        }
    }

} // acmacs::data_fix::v1::Set::fix

// ----------------------------------------------------------------------

void acmacs::data_fix::v1::Set::fix(acmacs::sheet::serum_fields_t& serum)
{
    using namespace std::string_view_literals;

    auto& data = get().data_;

    for (const auto& en : data) {
        const auto orig = serum.name;
        if (const auto res = en->serum_name(serum.name); res) {
            AD_INFO("SR name \"{}\" <-- \"{}\"", serum.name, orig);
            break;
        }
    }

    for (const auto& en : data) {
        const auto orig_name = serum.name;
        const auto orig_passage = serum.passage;
        if (const auto res = en->serum_passage(serum.passage, serum.name); res) {
            AD_INFO("SR passage \"{}\" <-- \"{}\"   name \"{}\" <-- \"{}\"", serum.passage, orig_passage, serum.name, orig_name);
            break;
        }
    }

} // acmacs::data_fix::v1::Set::fix

// ----------------------------------------------------------------------


// ======================================================================

namespace acmacs::data_fix::inline v1
{
    class AntigenSerumName : public Base
    {
      public:
        AntigenSerumName(std::string&& from, std::string&& to) : from_{from, acmacs::regex::icase}, to_{std::move(to)} {}

        bool antigen_name(std::string& src) const override
        {
            if (std::smatch match; std::regex_search(src, match, from_)) {
                src = match.format(to_);
                return true;
            }
            return Base::antigen_name(src);
        }

        bool serum_name(std::string& src) const override { return antigen_name(src); }

      private:
        std::regex from_;
        std::string to_;
    };

    // ----------------------------------------------------------------------

    class AntigenSerumPassage : public Base
    {
      public:
        AntigenSerumPassage(std::string&& from, std::string&& to, std::string&& name_append) : from_{from, acmacs::regex::icase}, to_{std::move(to)}, name_append_{std::move(name_append)} {}

        bool antigen_passage(std::string& src, std::string& name) const override
        {
            if (std::smatch match; std::regex_search(src, match, from_)) {
                const auto new_passage = match.format(to_); // do not overwrite src yet, otherwise next match.format gives nonsense (match.format copies from src)
                const auto name_append = match.format(name_append_);
                src = new_passage;
                if (!name_append.empty())
                    name += " " + name_append;
                return true;
            }
            return Base::antigen_passage(src, name);
        }

        bool serum_passage(std::string& src, std::string& name) const override { return antigen_passage(src, name); }

      private:
        std::regex from_;
        std::string to_;
        std::string name_append_;
    };

} // namespace acmacs::data_fix::inline v1

// ======================================================================

static SCM name_antigen_serum_fix(SCM from, SCM to);
static SCM passage_antigen_serum_fix(SCM from, SCM to, SCM name_append);

// ----------------------------------------------------------------------

void acmacs::data_fix::guile_defines()
{
    using namespace guile;
    using namespace std::string_view_literals;

    define("name-antigen-serum-fix"sv, name_antigen_serum_fix);
    define("passage-antigen-serum-fix"sv, passage_antigen_serum_fix);

} // acmacs::data_fix::guile_defines

// ----------------------------------------------------------------------

SCM name_antigen_serum_fix(SCM from, SCM to)
{
    // AD_DEBUG("name-antigen-serum-fix \"{}\" \"{}\"", guile::from_scm<std::string>(from), guile::from_scm<std::string>(to));
    acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::AntigenSerumName>(guile::from_scm<std::string>(from), guile::from_scm<std::string>(to)));
    return guile::VOID;

} // name_antigen_serum_fix

// ----------------------------------------------------------------------

SCM passage_antigen_serum_fix(SCM from, SCM to, SCM name_append)
{
    acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::AntigenSerumPassage>(guile::from_scm<std::string>(from), guile::from_scm<std::string>(to), guile::from_scm<std::string>(name_append)));
    return guile::VOID;

} // passage_antigen_serum_fix

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
