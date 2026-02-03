#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <argparse/argparse.hpp>

#include "semverxx.h"
#include "range.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser program("semv", "0.1.0");

    program.add_argument("version")
           .help("Semantic version strings to parse and display")
           .nargs(argparse::nargs_pattern::any);

    program.add_argument("-c", "--coerce")
           .help("Coerce a string into SemVer if possible")
           .default_value(false)
           .implicit_value(true);

    program.add_argument("-i", "--increment")
           .help("Increment a version by the specified level. "
               "Level can be one of: major, minor, or patch. "
               "Default level is 'patch'")
           .default_value("patch");

    program.add_argument("-r", "--range")
           .help("Print versions that match the specified range");

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    try {
        const auto strings = program.get<std::vector<std::string>>("version");
        std::vector<semverxx::version> versions;
        versions.reserve(strings.size());
        if (program["--coerce"] == true) {
            for (const auto& s : strings) {
                versions.push_back(semverxx::version::coerce(s));
            }
        } else {
            for (const auto& s : strings) {
                versions.emplace_back(s);
            }
        }
        if (auto r = program.present("--range")) {
            auto range = semverxx::range(*r);
            versions.erase(
                std::remove_if(
                    versions.begin(),
                    versions.end(),
                    [&range](const semverxx::version& v) {
                        return !range.contains(v);
                    }
                ),
                versions.end()
            );
        }
        if (program.is_used("--increment")) {
            const auto level = program.get<std::string>("--increment");
            for (auto& ver : versions) {
                if (level == "major") {
                    ver.bump_major();
                } else if (level == "minor") {
                    ver.bump_minor();
                } else if (level == "patch") {
                    ver.bump_patch();
                } else {
                    std::cerr << "Invalid increment level: " << level << std::endl;
                    return 1;
                }
            }
        }
        std::sort(versions.begin(), versions.end());
        for (const auto& ver : versions) {
            std::cout << ver << std::endl;
        }
    } catch (const std::invalid_argument& err) {
        std::cerr << "Error parsing version: " << err.what() << std::endl;
        return 1;
    }

    return 0;
}
