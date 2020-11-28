#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

// ----------------------------------------------------------------------

namespace acmacs::data_fix::inline v1
{
    using apply_result_t = std::optional<std::string>;

    class Base
    {
      public:
        virtual ~Base() = default;

        virtual apply_result_t serum_name(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t antigen_name(const std::string& /*src*/) const { return std::nullopt; }
        virtual apply_result_t titer(const std::string& /*src*/) const { return std::nullopt; }
    };

    // ----------------------------------------------------------------------

    class Set
    {
      public:
        static Set& update();

        void add(std::unique_ptr<Base>&& entry) { data_.push_back(std::move(entry)); }

        template <typename Func> static std::string apply(const std::string& src, Func func)
            {
                for (const auto& en : get().data_) {
                    if (const auto res = func(*en); res.has_value()) {
                        AD_INFO("fixed \"{}\" <-- \"{}\"", *res, src);
                        return *res;
                    }
                }
                return src;
            }


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
