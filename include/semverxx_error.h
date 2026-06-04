#ifndef SEMVERXX_ERROR_H
#define SEMVERXX_ERROR_H

#include <system_error>

namespace semverxx {
// Error codes describing why parsing a version failed
enum class errc {
    success = 0,
    invalid_major,
    invalid_minor,
    invalid_patch,
    invalid_prerelease,
    invalid_build
};

namespace detail {
    class version_error_category final : public std::error_category {
    public:
        const char* name() const noexcept override { return "semverxx"; }

        std::string message(int condition) const override {
            switch (static_cast<errc>(condition)) {
            case errc::success:
                return "success";
            case errc::invalid_major:
                return "invalid major version";
            case errc::invalid_minor:
                return "invalid minor version";
            case errc::invalid_patch:
                return "invalid patch version";
            case errc::invalid_prerelease:
                return "invalid pre-release version";
            case errc::invalid_build:
                return "invalid build metadata";
            }
            return "unknown error";
        }
    };
} // namespace detail

inline const std::error_category& version_category() noexcept {
    static const detail::version_error_category instance;
    return instance;
}

inline std::error_code make_error_code(errc e) noexcept {
    return {static_cast<int>(e), version_category()};
}
}

// Allow semverxx::errc to implicitly convert to std::error_code
template<>
struct std::is_error_code_enum<semverxx::errc> : true_type {
};

#endif