#ifndef SEMVERXX_H
#define SEMVERXX_H

#include <cstdio>
#include <string>
#include <ostream>

namespace semverxx {
class version {
public:
    version() = default;

    explicit version(const std::string& str) {
    }

    version(version&&) noexcept = default;

    version& operator=(version&&) = default;

    int major() const { return major_; }
    int minor() const { return minor_; }
    int patch() const { return patch_; }

    std::string& prerelease() { return prerelease_; }
    std::string& build() { return build_; }

    const std::string& prerelease() const { return prerelease_; }
    const std::string& build() const { return build_; }

    version& bump_major() {
        ++major_;
        minor_ = 0;
        patch_ = 0;
        return *this;
    }

    version& bump_minor() {
        ++minor_;
        patch_ = 0;
        return *this;
    }

    version& bump_patch() {
        ++patch_;
        return *this;
    }

    std::string to_string() const {
        auto buf_size = length() + 1;
        auto buf = new char[buf_size];
        if (!prerelease_.empty()) {
            if (!build_.empty()) {
                std::snprintf(buf, buf_size, "%d.%d.%d-%s+%s", major_, minor_, patch_, prerelease_.c_str(),
                              build_.c_str());
            } else {
                std::snprintf(buf, buf_size, "%d.%d.%d-%s", major_, minor_, patch_, prerelease_.c_str());
            }
        } else {
            if (!build_.empty()) {
                std::snprintf(buf, buf_size, "%d.%d.%d+%s", major_, minor_, patch_, build_.c_str());
            } else {
                std::snprintf(buf, buf_size, "%d.%d.%d", major_, minor_, patch_);
            }
        }
        std::string str{buf};
        delete[] buf;
        return str;
    }

    std::size_t length() const {
        return 2 + integral_length(major_) + integral_length(minor_) + integral_length(patch_)
            + (prerelease_.empty() ? 0 : prerelease_.size() + 1)
            + (build_.empty() ? 0 : build_.size() + 1);
    }

private:
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
