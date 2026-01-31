#ifndef SEMVERXX_RANGE_H
#define SEMVERXX_RANGE_H

#include "semverxx.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace semverxx {
// A comparator is composed of an operator and a version
struct comparator {
    // Operators
    enum class op {
        eq, // =
        ne, // !=
        lt, // <
        lte, // <=
        gt, // >
        gte // >=
    };

    op oper{op::eq};
    version ver;

    comparator() = default;

    comparator(op o, const version& v) : oper(o), ver(v) {
    }

    bool satisfies(const version& v) const {
        switch (oper) {
        case op::eq:
            return v == ver;
        case op::ne:
            return v != ver;
        case op::gt:
            return v > ver;
        case op::gte:
            return v >= ver;
        case op::lt:
            return v < ver;
        case op::lte:
            return v <= ver;
        default:
            return false;
        }
    }
};

// A comparator set is a collection of comparators (AND logic)
class comparator_set {
public:
    comparator_set() = default;

    explicit comparator_set(const std::vector<comparator>& comps) : comparators_(comps) {
    }

    void add(const comparator& comp) {
        comparators_.push_back(comp);
    }

    bool satisfies(const version& v) const {
        if (comparators_.empty()) {
            return true;
        }
        for (const auto& comp : comparators_) {
            if (!comp.satisfies(v)) {
                return false;
            }
        }
        return true;
    }

    bool empty() const noexcept {
        return comparators_.empty();
    }

private:
    std::vector<comparator> comparators_;
};

// A range is composed of one or more comparator sets (OR logic)
class range {
public:
    range() = default;

    explicit range(const std::string& str) {
        parse(str);
    }

    bool satisfies(const version& v) const {
        if (sets_.empty()) {
            return true;
        }
        for (const auto& set : sets_) {
            if (set.satisfies(v)) {
                return true;
            }
        }
        return false;
    }

    bool empty() const noexcept {
        return sets_.empty();
    }

private:
    // Trim leading and trailing whitespace
    static std::string trim(const std::string& str) {
        const auto first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) {
            return {};
        }
        const auto last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }

    // Split string by delimiter
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> tokens;
        std::size_t start{}, end{str.find(delimiter)};

        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.size();
            end = str.find(delimiter, start);
        }
        tokens.push_back(str.substr(start));
        return tokens;
    }

    // Split by whitespace, handling multiple spaces
    static std::vector<std::string> split_whitespace(const std::string& str) {
        std::vector<std::string> tokens;
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

    // Parse a single comparator
    static comparator parse_comparator(const std::string& str) {
        const auto trimmed = trim(str);
        if (trimmed.empty()) {
            throw std::invalid_argument("empty comparator");
        }

        auto oper = comparator::op::eq;
        std::size_t ver_start{};

        // Check for operator prefix
        if (trimmed.size() >= 2 && trimmed[0] == '!' && trimmed[1] == '=') {
            oper = comparator::op::ne;
            ver_start = 2;
        } else if (trimmed.size() >= 2 && trimmed[0] == '>' && trimmed[1] == '=') {
            oper = comparator::op::gte;
            ver_start = 2;
        } else if (trimmed.size() >= 2 && trimmed[0] == '<' && trimmed[1] == '=') {
            oper = comparator::op::lte;
            ver_start = 2;
        } else if (trimmed[0] == '>') {
            oper = comparator::op::gt;
            ver_start = 1;
        } else if (trimmed[0] == '<') {
            oper = comparator::op::lt;
            ver_start = 1;
        } else if (trimmed[0] == '=') {
            oper = comparator::op::eq;
            ver_start = 1;
        }

        const auto ver_str = trim(trimmed.substr(ver_start));
        if (ver_str.empty()) {
            throw std::invalid_argument("missing version in comparator");
        }

        return {oper, version(ver_str)};
    }

    // Parse a comparator set (whitespace-joined comparators)
    static comparator_set parse_comparator_set(const std::string& str) {
        comparator_set set;
        const std::vector<std::string> tokens = split_whitespace(str);

        if (tokens.empty()) {
            return set;
        }

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            // Hyphen range: [version - version]
            if (i + 2 < tokens.size() && tokens[i + 1] == "-") {
                version lower(tokens[i]);
                version upper(tokens[i + 2]);

                set.add(comparator(comparator::op::gte, lower));
                set.add(comparator(comparator::op::lte, upper));

                i += 2; // Skip the hyphen and upper version
            } else {
                // Regular comparator
                set.add(parse_comparator(tokens[i]));
            }
        }

        return set;
    }

    // Parse the full range string
    void parse(const std::string& str) {
        const auto trimmed = trim(str);
        if (trimmed.empty()) {
            return;
        }

        // Get comparator sets
        for (const auto& set_str : split(trimmed, "||")) {
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
