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
    version() : major_{}, minor_{}, patch_{} {
    }

    version(int major, int minor, int patch, const std::string& prerelease = "",
            const std::string& build = "") {
        set_major(major);
        set_minor(minor);
        set_patch(patch);
        set_prerelease(prerelease);
        set_build(build);
    }

    explicit version(const std::string& str) {
        switch (parse(str, 0, str.size())) {
        case state::success:
            break;
        case state::major_begin: // []
        case state::major_zero: // [0]
        case state::major: // [1]
            throw std::invalid_argument("invalid major version");
        case state::minor_begin: // [1.]
        case state::minor_zero: // [1.0]
        case state::minor: // [1.1]
            throw std::invalid_argument("invalid minor version");
        case state::patch_begin: // [1.1.]
        case state::patch_zero: // [1.1.0]
        case state::patch: // [1.1.1]
            throw std::invalid_argument("invalid patch version");
        case state::prerelease_begin: // [1.1.1-] | [1.1.1-alpha.]
        case state::prerelease:
            throw std::invalid_argument("invalid pre-release version");
        case state::build_begin: // [1.1.1+] | [1.1.1+aabbcc.]
        case state::build: // [1.1.1+aabbcc]
            throw std::invalid_argument("invalid build metadata");
        }
    }

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
        if (!valid_identifiers(prerelease)) {
            throw std::invalid_argument("invalid pre-release version");
        }
        prerelease_ = prerelease;
    }

    void set_build(const std::string& build) {
        if (!valid_identifiers(build)) {
            throw std::invalid_argument("invalid build metadata");
        }
        build_ = build;
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

    static version coerce(const std::string& str) {
        auto last_index = str.find_last_not_of("\t\n\v\f\r ");
        if (last_index == std::string::npos) {
            return {};
        }
        auto first_index = str.find_first_of("0123456789");
        if (first_index == std::string::npos) {
            return {};
        }
        version ver;
        ver.parse(str, first_index, last_index + 1);
        return ver;
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
        build,
        success
    };

    // Consume all valid characters in the string
    state parse(const std::string& str, std::size_t i, std::size_t end, state initial_state = state::major_begin) {
        if (initial_state == state::major_begin) {
            major_ = minor_ = patch_ = 0; // Initialize a new version
        }
        std::size_t prerelease_index{initial_state == state::prerelease_begin ? 0 : std::string::npos};
        std::size_t build_index{initial_state == state::build_begin ? 0 : std::string::npos};
        auto current_state = initial_state;

        for (; i < end; ++i) {
            const auto c = str[i];
            if (c == '0') {
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
                    major_ *= 10;
                    break;
                case state::minor: // [1.1]0
                    minor_ *= 10;
                    break;
                case state::patch: // [1.1.1]0
                    patch_ *= 10;
                    break;
                case state::prerelease: // [1.1.1-alpha]0
                case state::build: // [1.1.1+aabbcc]0
                    break;
                default:
                    return current_state;
                }
            } else if (std::isdigit(c)) {
                switch (current_state) {
                case state::major_begin: // []1
                    current_state = state::major;
                    major_ = c - '0';
                    break;
                case state::major: // [1]1
                    major_ = major_ * 10 + (c - '0');
                    break;
                case state::minor_begin: // [1.]1
                    current_state = state::minor;
                    minor_ = c - '0';
                    break;
                case state::minor: // [1.1]1
                    minor_ = minor_ * 10 + (c - '0');
                    break;
                case state::patch_begin: // [1.1.]1
                    current_state = state::patch;
                    patch_ = c - '0';
                    break;
                case state::patch: // [1.1.1]1
                    patch_ = patch_ * 10 + (c - '0');
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
                    return current_state;
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
                    return current_state;
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
                    return current_state;
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
                    return current_state;
                }
            } else if (c == '+') {
                switch (current_state) {
                case state::patch_zero: // [1.1.0]+
                case state::patch: // [1.1.1]+
                case state::prerelease: // [1.1.1-alpha]+
                    if (prerelease_index != std::string::npos) {
                        prerelease_ = str.substr(prerelease_index, i - prerelease_index);
                    }
                    current_state = state::build_begin;
                    build_index = i + 1;
                    break;
                default:
                    return current_state;
                }
            } else {
                return current_state;
            }
        }

        switch (current_state) {
        case state::patch_zero: // [1.1.0]
        case state::patch: // [1.1.1]
            return state::success;
        case state::prerelease: // [1.1.1-alpha]
            prerelease_ = str.substr(prerelease_index, std::string::npos);
            return state::success;
        case state::build: // [1.1.1+aabbcc]
            build_ = str.substr(build_index);
            return state::success;
        default:
            return current_state;
        }
    }

    static bool valid_identifiers(const std::string& str) {
        if (str.empty()) {
            return true;
        }
        if (str.front() == '.' || str.back() == '.') {
            return false;
        }
        bool prev_dot{};
        for (auto c : str) {
            if (c == '.') {
                if (prev_dot) {
                    return false; // Empty identifier
                }
                prev_dot = true;
            } else if (!std::isalnum(c) && c != '-') {
                return false; // Invalid character
            } else {
                prev_dot = false;
            }
        }
        return true;
    }

    static std::size_t integral_length(int n) noexcept {
        std::size_t digits{};
        do {
            ++digits;
            n /= 10;
        } while (n);
        return digits;
    }

    int major_;
    int minor_;
    int patch_;
    std::string prerelease_;
    std::string build_;
};

inline bool operator==(const version& lhs, const version& rhs) noexcept {
    return lhs.major() == rhs.major() && lhs.minor() == rhs.minor() && lhs.patch() == rhs.patch() &&
        lhs.prerelease() == rhs.prerelease();
}

inline bool operator!=(const version& lhs, const version& rhs) noexcept { return !(lhs == rhs); }


inline bool operator<(const version& lhs, const version& rhs) {
    if (lhs.major() != rhs.major()) {
        return lhs.major() < rhs.major();
    }
    if (lhs.minor() != rhs.minor()) {
        return lhs.minor() < rhs.minor();
    }
    if (lhs.patch() != rhs.patch()) {
        return lhs.patch() < rhs.patch();
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
#if __cpp_structured_bindings >= 201606L
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
    using type = decltype(get < I > (declval<semverxx::version>()));
};
}
#endif

#endif // SEMVERXX_H