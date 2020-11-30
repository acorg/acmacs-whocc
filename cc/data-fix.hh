#pragma once

#include <string>
#include <vector>
#include <memory>

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

      private:
        Set() = default; // singleton, use get() and update() to access
        static const Set& get();

        std::vector<std::unique_ptr<Base>> data_;

    };

    // ----------------------------------------------------------------------

    // pass pointer to this function to guile::init()
    void guile_defines();

} // namespace acmacs::data_fix::inline v1

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
