#include "acmacs-base/guile.hh"
#include "acmacs-whocc/data-fix.hh"
#include "acmacs-whocc/data-fix-guile.hh"

// ----------------------------------------------------------------------

static SCM name_antigen_serum_fix(SCM from, SCM to);
static SCM passage_antigen_serum_fix(SCM from, SCM to, SCM name_append);
static SCM titer_fix(SCM from, SCM to);

// ----------------------------------------------------------------------

void acmacs::data_fix::guile_defines()
{
    using namespace guile;
    using namespace std::string_view_literals;

    define("name-antigen-serum-fix"sv, name_antigen_serum_fix);
    define("passage-antigen-serum-fix"sv, passage_antigen_serum_fix);
    define("titer-fix"sv, titer_fix);

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

SCM titer_fix(SCM from, SCM to)
{
    acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::Titer>(guile::from_scm<std::string>(from), guile::from_scm<std::string>(to)));
    return guile::VOID;

} // titer_fix

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
