#ifndef SEMVERXX_RANGE_H
#define SEMVERXX_RANGE_H

#include "semverxx.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace semverxx {
// A comparator is composed of an operator and a version
class comparator {
public:
    // Operators
    enum class op {
        eq, // =
        ne, // !=
        lt, // <
        lte, // <=
        gt, // >
        gte // >=
    };

    comparator() = default;

    explicit comparator(std::string_view str) { parse(str); }

    comparator(op o, const version& v) : operator_(o), version_(v) {
    }

    bool satisfies(const version& v) const {
        switch (operator_) {
        case op::eq:
            return v == version_;
        case op::ne:
            return v != version_;
        case op::gt:
            return v > version_;
        case op::gte:
            return v >= version_;
        case op::lt:
            return v < version_;
        case op::lte:
            return v <= version_;
        default:
            return false;
        }
    }

private:
    void parse(std::string_view str) {
        // Trim
        str.remove_prefix(std::min(str.find_first_not_of(" \t\n\r\f\v"), str.size()));
        str.remove_suffix(str.size() - std::min(str.find_last_not_of(" \t\n\r\f\v") + 1, str.size()));

        if (str.empty()) {
            throw std::invalid_argument("empty comparator");
        }

        // Check for operator prefix
        auto prefix = str.substr(0, 2);
        std::size_t prefix_length{prefix.size()};
        if (prefix == "!=") {
            operator_ = op::ne;
        } else if (prefix == ">=") {
            operator_ = op::gte;
        } else if (str.size() >= 2 && str[0] == '<' && str[1] == '=') {
            operator_ = op::lte;
        } else if (prefix[0] == '>') {
            operator_ = op::gt;
            prefix_length = 1;
        } else if (prefix[0] == '<') {
            operator_ = op::lt;
            prefix_length = 1;
        } else if (prefix[0] == '=') {
            operator_ = op::eq;
            prefix_length = 1;
        } else {
            prefix_length = 0;
        }
        str.remove_prefix(prefix_length);

        str.remove_prefix(std::min(str.find_first_not_of(" \t\n\r\f\v"), str.size()));
        if (str.empty()) {
            throw std::invalid_argument("missing version in comparator");
        }
        version_ = version::coerce(str);
    }

    op operator_{op::eq};
    version version_;
};

// A range is composed of one or more comparator sets (OR logic)
class range {
public:
    range() = default;

    explicit range(std::string_view str) {
        parse(str);
    }

    bool satisfies(const version& v) const {
        if (sets_.empty()) {
            return true;
        }
        for (const auto& set : sets_) {
            bool all_satisfied = true;
            for (const auto& comp : set) {
                if (!comp.satisfies(v)) {
                    all_satisfied = false;
                    break;
                }
            }
            if (all_satisfied) {
                return true;
            }
        }
        return false;
    }

    bool empty() const noexcept {
        return sets_.empty();
    }

private:
    // A comparator set is a collection of comparators (AND logic)
    using comparator_set = std::vector<comparator>;

    // Trim leading and trailing whitespace
    static std::string_view trim(std::string_view str) {
        const auto first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) {
            return {};
        }
        const auto last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }

    // Split string by delimiter
    static std::vector<std::string_view> split(std::string_view str, std::string_view delimiter) {
        std::vector<std::string_view> tokens;
        std::size_t start{}, end{str.find(delimiter)};

        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.size();
            end = str.find(delimiter, start);
        }
        tokens.push_back(str.substr(start));
        return tokens;
    }

    // Split string by whitespace, handling multiple spaces
    static std::vector<std::string_view> split_whitespace(std::string_view str) {
        std::vector<std::string_view> tokens;
        std::size_t start{};
        const std::size_t len{str.size()};

        while (start < len) {
            // Skip leading whitespace
            while (start < len && std::isspace(static_cast<unsigned char>(str[start]))) {
                ++start;
            }
            if (start >= len) {
                break;
            }
            // Find end of token
            std::size_t end = start;
            while (end < len && !std::isspace(static_cast<unsigned char>(str[end]))) {
                ++end;
            }
            tokens.push_back(str.substr(start, end - start));
            start = end;
        }
        return tokens;
    }


    // Parse a comparator set (whitespace-joined comparators)
    static comparator_set parse_comparator_set(std::string_view str) {
        comparator_set set;
        const auto tokens = split_whitespace(str);
        if (tokens.empty()) {
            return set;
        }

        for (std::size_t i{}; i < tokens.size(); ++i) {
            // Hyphen range: [version - version]
            if (i + 2 < tokens.size() && tokens[i + 1] == "-") {
                // Lower and upper bounds
                set.emplace_back(comparator::op::gte, version::coerce(tokens[i]));
                set.emplace_back(comparator::op::lte, version::coerce(tokens[i + 2]));
                i += 2; // Skip the hyphen and upper version
            } else {
                // Regular comparator
                set.emplace_back(tokens[i]);
            }
        }

        return set;
    }

    // Parse the full range string
    void parse(std::string_view str) {
        const auto trimmed = trim(str);
        if (trimmed.empty()) {
            return;
        }

        // Get comparator sets
        for (const auto set_str : split(trimmed, "||")) {
            auto trimmed_set = trim(set_str);
            if (!trimmed_set.empty()) {
                auto set = parse_comparator_set(trimmed_set);
                if (!set.empty()) {
                    sets_.push_back(set);
                }
            }
        }
    }

    std::vector<comparator_set> sets_;
};
} // namespace semverxx

#endif // SEMVERXX_RANGE_H
