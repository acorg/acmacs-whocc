#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

// ----------------------------------------------------------------------

namespace acmacs::sheet::inline v1
{
    struct antigen_fields_t;
    struct serum_fields_t;

} // namespace acmacs::sheet::inline v1

namespace acmacs::data_fix::inline v1
{
    using apply_result_t = std::optional<std::string>;

    class Base
    {
      public:
        virtual ~Base() = default;

        virtual apply_result_t antigen_name(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t antigen_passage(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t serum_name(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t serum_passage(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t titer(const std::string& /*src*/) const { return std::nullopt; }
    };

    // ----------------------------------------------------------------------

    class Set
    {
      public:
        static Set& update();

        void add(std::unique_ptr<Base>&& entry) { data_.push_back(std::move(entry)); }
        static void fix(acmacs::sheet::antigen_fields_t& antigen);
        static void fix(acmacs::sheet::serum_fields_t& serum);

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
