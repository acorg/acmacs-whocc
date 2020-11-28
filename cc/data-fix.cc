#include "acmacs-base/regex.hh"
#include "acmacs-base/guile.hh"
#include "acmacs-whocc/data-fix.hh"

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

// ======================================================================

namespace acmacs::data_fix::inline v1
{
    class AntigenSerumName : public Base
    {
      public:
        AntigenSerumName(std::string&& from, std::string&& to) : from_{from, acmacs::regex::icase}, to_{std::move(to)} {}

        apply_result_t antigen_name(const std::string& src) const override
        {
            if (std::regex_match(src, from_))
                return to_;
            else
                return std::nullopt;
        }

        apply_result_t serum_name(const std::string& src) const override { return antigen_name(src); }

      private:
        std::regex from_;
        std::string to_;
    };
} // namespace acmacs::data_fix::inline v1

// ======================================================================

static SCM name_antigen_serum_fix(SCM arg, SCM to);

// ----------------------------------------------------------------------

void acmacs::data_fix::guile_defines()
{
    using namespace guile;
    using namespace std::string_view_literals;

    define("name-antigen-serum-fix"sv, name_antigen_serum_fix);

} // acmacs::data_fix::guile_defines

// ----------------------------------------------------------------------

SCM name_antigen_serum_fix(SCM from, SCM to)
{
    acmacs::data_fix::Set::update().add(std::make_unique<acmacs::data_fix::AntigenSerumName>(guile::from_scm<std::string>(from), guile::from_scm<std::string>(to)));
    return guile::VOID;

} // name_antigen_serum_fix

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
