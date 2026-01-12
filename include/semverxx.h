#ifndef SEMVERXX_H
#define SEMVERXX_H

#include <cctype>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace semverxx {
class version {
public:
    version() = default;

    version(int major, int minor, int patch, const std::string& prerelease = "",
            const std::string& build = "") {
        set_major(major);
        set_minor(minor);
        set_patch(patch);
        set_prerelease(prerelease);
        set_build(build);
    }

    explicit version(const std::string& str) { parse(str); }

    version(const version&) = default;
    version(version&&) noexcept = default;

    version& operator=(const version&) = default;
    version& operator=(version&&) noexcept = default;

    int major() const noexcept { return major_; }

    int minor() const noexcept { return minor_; }

    int patch() const noexcept { return patch_; }

    const std::string& prerelease() const noexcept { return prerelease_; }

    const std::string& build() const noexcept { return build_; }

    version core() const { return {major_, minor_, patch_}; }

    void set_major(int major) {
        if (major < 0) {
            throw std::invalid_argument("invalid major version");
        }
        major_ = major;
    }

    void set_minor(int minor) {
        if (minor < 0) {
            throw std::invalid_argument("invalid minor version");
        }
        minor_ = minor;
    }

    void set_patch(int patch) {
        if (patch < 0) {
            throw std::invalid_argument("invalid patch version");
        }
        patch_ = patch;
    }

    void set_prerelease(const std::string& prerelease) {
        if (prerelease.empty()) {
            clear_prerelease();
        } else {
            parse(prerelease, state::prerelease_begin);
        }
    }

    void set_build(const std::string& build) {
        if (build.empty()) {
            clear_build();
        } else {
            parse(build, state::build_begin);
        }
    }

    void bump_major() noexcept {
        ++major_;
        minor_ = 0;
        patch_ = 0;
        clear_prerelease();
        clear_build();
    }

    void bump_minor() noexcept {
        ++minor_;
        patch_ = 0;
        clear_prerelease();
        clear_build();
    }

    void bump_patch() noexcept {
        ++patch_;
        clear_prerelease();
        clear_build();
    }

    void append_prerelease(const std::string& identifier) {
        set_prerelease(prerelease_.empty() ? identifier : prerelease_ + "." + identifier);
    }

    void append_build(const std::string& identifier) {
        set_build(build_.empty() ? identifier : build_ + "." + identifier);
    }

    void clear_prerelease() noexcept { prerelease_.clear(); }

    void clear_build() noexcept { build_.clear(); }

    void clear() noexcept {
        major_ = minor_ = patch_ = 0;
        clear_prerelease();
        clear_build();
    }

    std::string to_string() const {
        std::string str = std::to_string(major_) + "." + std::to_string(minor_) + "." + std::to_string(patch_);
        if (!prerelease_.empty()) {
            str += "-" + prerelease_;
        }
        if (!build_.empty()) {
            str += "+" + build_;
        }
        return str;
    }

    std::size_t length() const noexcept {
        return 2 + integral_length(major_) + integral_length(minor_) + integral_length(patch_) +
            (prerelease_.empty() ? 0 : prerelease_.size() + 1) + (build_.empty() ? 0 : build_.size() + 1);
    }

private:
    // States of the finite state machine
    enum class state {
        major_begin,
        major_zero,
        major,
        minor_begin,
        minor_zero,
        minor,
        patch_begin,
        patch_zero,
        patch,
        prerelease_begin,
        prerelease,
        build_begin,
        build
    };

    void parse(const std::string& str, state initial_state = state::major_begin) {
        int major{};
        int minor{};
        int patch{};
        std::size_t prerelease_index{initial_state == state::prerelease_begin ? 0 : std::string::npos};
        std::size_t build_index{initial_state == state::build_begin ? 0 : std::string::npos};
        state current_state{initial_state};

        for (std::size_t i = 0; i < str.size(); ++i) {
            if (const auto c = str[i]; c == '0') {
                switch (current_state) {
                case state::major_begin: // []0
                    current_state = state::major_zero;
                    break;
                case state::minor_begin: // [1.]0
                    current_state = state::minor_zero;
                    break;
                case state::patch_begin: // [1.1.]0
                    current_state = state::patch_zero;
                    break;
                case state::prerelease_begin: // [1.1.1-]0 | [1.1.1-alpha.]0
                    current_state = state::prerelease;
                    break;
                case state::build_begin: // [1.1.1+]0 | [1.1.1+aabbcc.]0
                    current_state = state::build;
                    break;
                case state::major: // [1]0
                case state::minor: // [1.1]0
                case state::patch: // [1.1.1]0
                case state::prerelease: // [1.1.1-alpha]0
                case state::build: // [1.1.1+aabbcc]0
                    break;
                default:
                    throw std::invalid_argument("unexpected zero digit");
                }
            } else if (std::isdigit(c)) {
                switch (current_state) {
                case state::major_begin: // []1
                    current_state = state::major;
                    major = c - '0';
                    break;
                case state::major: // [1]1
                    major = major * 10 + (c - '0');
                    break;
                case state::minor_begin: // [1.]1
                    current_state = state::minor;
                    minor = c - '0';
                    break;
                case state::minor: // [1.1]1
                    minor = minor * 10 + (c - '0');
                    break;
                case state::patch_begin: // [1.1.]1
                    current_state = state::patch;
                    patch = c - '0';
                    break;
                case state::patch: // [1.1.1]1
                    patch = patch * 10 + (c - '0');
                    break;
                case state::prerelease_begin: // [1.1.1-]1 | [1.1.1-alpha.]1
                    current_state = state::prerelease;
                    break;
                case state::build_begin: // [1.1.1+]1 | [1.1.1+aabbcc.]1
                    current_state = state::build;
                    break;
                case state::major_zero: // [0]1
                case state::minor_zero: // [1.0]1
                case state::patch_zero: // [1.1.0]1
                    throw std::invalid_argument("invalid leading zero");
                default:
                    break;
                }
            } else if (std::isalpha(c)) {
                switch (current_state) {
                case state::prerelease_begin: // [1.1.1-]a | [1.1.1-alpha.]a
                    current_state = state::prerelease;
                    break;
                case state::build_begin: // [1.1.1+]a | [1.1.1+aabbcc.]a
                    current_state = state::build;
                    break;
                case state::prerelease: // [1.1.1-alpha]a
                case state::build: // [1.1.1+aabbcc]a
                    break;
                default:
                    throw std::invalid_argument("unexpected alphabetic character");
                }
            } else if (c == '.') {
                switch (current_state) {
                case state::major_zero: // [0].
                case state::major: // [1].
                    current_state = state::minor_begin;
                    break;
                case state::minor_zero: // [1.0].
                case state::minor: // [1.1].
                    current_state = state::patch_begin;
                    break;
                case state::prerelease: // [1.1.1-alpha].
                    current_state = state::prerelease_begin;
                    break;
                case state::build: // [1.1.1+aabbcc].
                    current_state = state::build_begin;
                    break;
                default:
                    throw std::invalid_argument("unexpected dot");
                }
            } else if (c == '-') {
                switch (current_state) {
                case state::patch_zero: // [1.1.0]-
                case state::patch: // [1.1.1]-
                    current_state = state::prerelease_begin;
                    prerelease_index = i + 1;
                    break;
                case state::prerelease_begin: // [1.1.1-]- | [1.1.1-alpha.]-
                    current_state = state::prerelease;
                    break;
                case state::build_begin: // [1.1.1+]- | [1.1.1+aabbcc.]-
                    current_state = state::build;
                    break;
                case state::prerelease: // [1.1.1-alpha]-
                case state::build: // [1.1.1-alpha+aabbcc]-
                    break;
                default:
                    throw std::invalid_argument("unexpected hyphen");
                }
            } else if (c == '+') {
                switch (current_state) {
                case state::patch_zero: // [1.1.0]+
                case state::patch: // [1.1.1]+
                case state::prerelease: // [1.1.1-alpha]+
                    current_state = state::build_begin;
                    build_index = i + 1;
                    break;
                default:
                    throw std::invalid_argument("unexpected plus");
                }
            } else {
                throw std::invalid_argument("invalid character");
            }
        }

        switch (current_state) {
        case state::major_begin: // []
            throw std::invalid_argument("missing major version");
        case state::major_zero: // [0]
        case state::major: // [1]
        case state::minor_begin: // [1.]
            throw std::invalid_argument("missing minor version and patch version");
        case state::minor_zero: // [1.0]
        case state::minor: // [1.1]
        case state::patch_begin: // [1.1.]
            throw std::invalid_argument("missing patch version");
        case state::prerelease_begin: // [1.1.1-] | [1.1.1-alpha.]
            throw std::invalid_argument("incomplete pre-release version");
        case state::build_begin: // [1.1.1+] | [1.1.1+aabbcc.]
            throw std::invalid_argument("incomplete build metadata");
        case state::patch_zero: // [1.1.0]
        case state::patch: // [1.1.1]
        case state::prerelease: // [1.1.1-alpha]
        case state::build: // [1.1.1+aabbcc]
            break; // valid
        }
        if (initial_state == state::major_begin) {
            major_ = major;
            minor_ = minor;
            patch_ = patch;
        }
        if (prerelease_index != std::string::npos) {
            auto prerelease_length{build_index == std::string::npos ? build_index : build_index - prerelease_index - 1};
            prerelease_ = str.substr(prerelease_index, prerelease_length);
        }
        if (build_index != std::string::npos) {
            build_ = str.substr(build_index);
        }
    }

    static std::size_t integral_length(int n) noexcept {
        std::size_t digits{};
        do {
            ++digits;
            n /= 10;
        } while (n);
        return digits;
    }

    int major_{};
    int minor_{};
    int patch_{};
    std::string prerelease_;
    std::string build_;
};

inline bool operator==(const version& lhs, const version& rhs) noexcept {
    return lhs.major() == rhs.major() && lhs.minor() == rhs.minor() && lhs.patch() == rhs.patch() &&
        lhs.prerelease() == rhs.prerelease();
}

inline bool operator!=(const version& lhs, const version& rhs) noexcept { return !(lhs == rhs); }


inline bool operator<(const version& lhs, const version& rhs) {
    if (int diff; (diff = lhs.major() - rhs.major()) || (diff = lhs.minor() - rhs.minor()) ||
        (diff = lhs.patch() - rhs.patch())) {
        return diff < 0;
    }
    // Compare the pre-release versions
    const std::string &a = lhs.prerelease(), &b = rhs.prerelease();
    if (a.empty()) {
        return false;
    }
    if (b.empty()) {
        return true;
    }
    bool numeric_a{true}, numeric_b{true};
    int identifier_a{}, identifier_b{};
    int lexical_comparison{};
    std::size_t i{}, j{};
    for (; i < a.size() && j < b.size(); ++i, ++j) {
        auto char_a{a[i]}, char_b{b[j]};
        if (char_a == '.' || char_b == '.') {
            if (char_a != '.') {
                if (numeric_b) {
                    return false; // Long a > short b
                }
                if (numeric_a && std::isdigit(char_a)) {
                    return true; // Numeric a < non-numeric b
                }
                // Compare non-numeric a and non-numeric b
                return lexical_comparison < 0;
            }
            if (char_b != '.') {
                if (numeric_a) {
                    return true; // Short a < long b
                }
                if (numeric_b && std::isdigit(char_b)) {
                    return false; // Non-numeric a > numeric b
                }
                // Compare non-numeric a and non-numeric b
                return lexical_comparison <= 0;
            }
            if (numeric_a && numeric_b) {
                if (identifier_a != identifier_b) {
                    return identifier_a - identifier_b;
                }
            } else if (numeric_a) {
                return true; // Numeric a < non-numeric b
            } else if (numeric_b) {
                return false; // Non-numeric a > numeric b
            } else {
                if (lexical_comparison != 0) {
                    return lexical_comparison < 0;
                }
            }
            // Reset identifiers
            numeric_a = numeric_b = true;
            identifier_a = identifier_b = 0;
            lexical_comparison = 0;
        } else {
            numeric_a = numeric_a && std::isdigit(char_a);
            numeric_b = numeric_b && std::isdigit(char_b);
            if (numeric_a && numeric_b) {
                identifier_a = identifier_a * 10 + (char_a - '0');
                identifier_b = identifier_b * 10 + (char_b - '0');
            } else {
                // First position differed
                if (lexical_comparison == 0) {
                    lexical_comparison = char_a - char_b;
                }
                if (!numeric_a && !numeric_b && lexical_comparison != 0) {
                    return lexical_comparison < 0;
                }
            }
        }
    }
    if (i < a.size()) {
        if (numeric_b) {
            return false; // Long a > short b
        }
        return lexical_comparison < 0;
    }
    if (j < b.size()) {
        if (numeric_a) {
            return true; // Short a < long b
        }
        return lexical_comparison <= 0;
    }
    return false;
}

inline bool operator>(const version& lhs, const version& rhs) { return rhs < lhs; }

inline bool operator<=(const version& lhs, const version& rhs) { return !(rhs < lhs); }

inline bool operator>=(const version& lhs, const version& rhs) { return !(lhs < rhs); }

inline std::ostream& operator<<(std::ostream& os, const version& ver) { return os << ver.to_string(); }

// User-defined literals
namespace literals {
    inline version operator""_v(const char* str, std::size_t) {
        return version(str);
    }
}
} // namespace semverxx

// Implement the tuple protocol to provide structured binding support in C++17
#if __cpp_structured_bindings >= 20160L
namespace semverxx {
template<std::size_t I>
decltype(auto) get(const version& v) noexcept {
    static_assert(I <= 4, "Invalid index");
    if constexpr (I == 0) {
        return v.major();
    } else if constexpr (I == 1) {
        return v.minor();
    } else if constexpr (I == 2) {
        return v.patch();
    } else if constexpr (I == 3) {
        return v.prerelease();
    } else if constexpr (I == 4) {
        return v.build();
    }
}
}

#include <type_traits>

namespace std {
template<>
struct tuple_size<semverxx::version> : integral_constant<size_t, 5> {
};

template<size_t I>
struct tuple_element<I, semverxx::version> {
    using type = decltype(get<I>(declval<semverxx::version>()));
};
}
#endif

#endif // SEMVERXX_H
