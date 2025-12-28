#ifndef SEMVERXX_H
#define SEMVERXX_H

#include <cctype>
#include <ostream>
#include <stdexcept>
#include <string>

namespace semverxx {
class version {
public:
    version() = default;

    explicit version(int major, int minor = 0, int patch = 0, const std::string& prerelease = "",
                     const std::string& build = "") {
        set_major(major);
        set_minor(minor);
        set_patch(patch);
        set_prerelease(prerelease);
        set_build(build);
    }

    explicit version(const std::string& str) {
        parse(str);
    }

    version(const version&) = default;
    version(version&&) noexcept = default;

    version& operator=(const version&) = default;
    version& operator=(version&&) noexcept = default;

    int major() const { return major_; }

    int minor() const { return minor_; }

    int patch() const { return patch_; }

    const std::string& prerelease() const { return prerelease_; }

    const std::string& build() const { return build_; }

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

    void bump_major() {
        ++major_;
        minor_ = 0;
        patch_ = 0;
        clear_prerelease();
        clear_build();
    }

    void bump_minor() {
        ++minor_;
        patch_ = 0;
        clear_prerelease();
        clear_build();
    }

    void bump_patch() {
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

    void clear_prerelease() { prerelease_.clear(); }

    void clear_build() { build_.clear(); }

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

    std::size_t length() const {
        return 2 + integral_length(major_) + integral_length(minor_) + integral_length(patch_)
            + (prerelease_.empty() ? 0 : prerelease_.size() + 1)
            + (build_.empty() ? 0 : build_.size() + 1);
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
                default:
                    break; // always valid
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
            break;
        case state::major_zero: // [0]
        case state::major: // [1]
        case state::minor_begin: // [1.]
            throw std::invalid_argument("missing minor version and patch version");
            break;
        case state::minor_zero: // [1.0]
        case state::minor: // [1.1]
        case state::patch_begin: // [1.1.]
            throw std::invalid_argument("missing patch version");
            break;
        case state::prerelease_begin: // [1.1.1-] | [1.1.1-alpha.]
            throw std::invalid_argument("incomplete pre-release version");
            break;
        case state::build_begin: // [1.1.1+] | [1.1.1+aabbcc.]
            throw std::invalid_argument("incomplete build metadata");
            break;
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

    static std::size_t integral_length(int n) {
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

inline std::ostream& operator<<(std::ostream& os, const version& ver) {
    return os << ver.to_string();
}
}

#endif // SEMVERXX_H
