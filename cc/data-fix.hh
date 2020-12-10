#pragma once

#include "acmacs-base/regex.hh"

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{
    struct antigen_fields_t;
    struct serum_fields_t;

} // namespace acmacs::sheet::inline v1

namespace acmacs::data_fix::inline v1
{
    class Base
    {
      public:
        virtual ~Base() = default;

        // functions return if src was modified
        virtual bool antigen_name(std::string& /*src*/) const { return false; }
        virtual bool antigen_passage(std::string& /*src*/, std::string& /*name*/) const { return false; }
        virtual bool serum_name(std::string& /*src*/) const { return false; }
        virtual bool serum_passage(std::string& /*src*/, std::string& /*name*/) const { return false; }
        virtual bool date(std::string& /*src*/) const { return false; }
        virtual bool titer(std::string& /*src*/) const { return false; }
    };

    // ----------------------------------------------------------------------

    class Set
    {
      public:
        static Set& update();

        void add(std::unique_ptr<Base>&& entry) { data_.push_back(std::move(entry)); }
        static void fix(acmacs::sheet::antigen_fields_t& antigen, size_t antigen_no);
        static void fix(acmacs::sheet::serum_fields_t& serum, size_t serum_no);
        static void fix_titer(std::string& titer, size_t antigen_no, size_t serum_no);

      private:
        Set() = default; // singleton, use get() and update() to access
        static const Set& get();

        std::vector<std::unique_ptr<Base>> data_;

    };

    // ----------------------------------------------------------------------

    class FromTo : public Base
    {
      public:
        FromTo(std::string&& from, std::string&& to) : from_{from, acmacs::regex::icase}, to_{std::move(to)} {}


    protected:
        bool fix(std::string& src) const
        {
            if (std::smatch match; std::regex_search(src, match, from_)) {
                src = match.format(to_);
                return true;
            }
            return false;
        }

      private:
        std::regex from_;
        std::string to_;

    };

    // ----------------------------------------------------------------------

    class AntigenSerumName : public FromTo
    {
      public:
        using FromTo::FromTo;
        bool antigen_name(std::string& src) const override { return fix(src) || Base::antigen_name(src); }
        bool serum_name(std::string& src) const override { return fix(src) || Base::serum_name(src); }
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

    // ----------------------------------------------------------------------

    class Date : public FromTo
    {
      public:
        using FromTo::FromTo;
        bool date(std::string& src) const override { return fix(src) || Base::date(src); }
    };

    // ----------------------------------------------------------------------

    class Titer : public Base
    {
      public:
        Titer(std::string&& from, std::string&& to) : from_{from, acmacs::regex::icase}, to_{std::move(to)} {}

        bool titer(std::string& src) const override
        {
            if (std::smatch match; std::regex_search(src, match, from_)) {
                src = match.format(to_);
                return true;
            }
            return Base::titer(src);
        }

      private:
        std::regex from_;
        std::string to_;
    };

} // namespace acmacs::data_fix::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
