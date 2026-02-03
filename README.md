# semver++

[![Test](https://github.com/LozMov/semverxx/actions/workflows/test.yml/badge.svg)](https://github.com/LozMov/semverxx/actions/workflows/test.yml)

**semver++** is a header-only C++ library for parsing, manipulating, and
comparing [Semantic Versioning 2.0.0](https://semver.org/) version strings.

## Usage

### Basic Version Operations

```cpp
using namespace semverxx;
using namespace semverxx::literals;

// Create versions
version v1("1.2.3");
version v2(1, 2, 3, "alpha", "001");
auto v3 = "2.0.0-beta"_v;

// Coerce from arbitrary strings
auto coerced = version::coerce("v1.2.3-rc1");  // 1.2.3-rc1

// Access components
int major = v1.major();  // 1
int minor = v1.minor();  // 2
int patch = v1.patch();  // 3
std::string pre = v2.prerelease();  // "alpha"
std::string build = v2.build();  // "001"

// Compare versions
if (v1 < v3) {
    // v1 is older than v3
}

// Bump versions
v1.bump_major();  // 2.0.0
v1.bump_minor();  // 2.1.0
v1.bump_patch();  // 2.1.1

// Convert to string
std::string str = v2.to_string();  // "1.2.3-alpha+001"
```

### Structured Bindings (C++17)

```cpp
semverxx::version v("1.2.3-alpha+001");
auto [major, minor, patch, prerelease, build] = v;
// major==1, minor==2, patch==3, prerelease=="alpha", build=="001"
```

### Range Matching

```cpp
using namespace semverxx;

range r(">=1.0.0 <2.0.0");
r.contains(version("1.5.0"));  // true
r.contains(version("2.0.0"));  // false

// Hyphen ranges
range hyphen("1.0.0 - 2.0.0");
hyphen.contains(version("1.5.0"));  // true

// OR logic with ||
range or_range(">=2.0.0 || <1.0.0");
or_range.contains(version("0.5.0"));  // true
or_range.contains(version("3.0.0"));  // true
```

## CLI Utility (`semv`)

Build the CLI utility by enabling the `SEMVERXX_BUILD_CLI` option:

```sh
cmake -B build -DSEMVERXX_BUILD_CLI=ON
cmake --build build
```

### Usage

```sh
# Parse and sort versions
semv 1.2.3 2.0.0-alpha

# Coerce versions from strings
semv --coerce "v1.2.3" "release-2.0.0"

# Increment versions
semv --increment=major 1.2.3  # Output: 2.0.0
semv -i minor 1.2.3  # Output: 1.3.0
semv -i patch 1.2.3  # Output: 1.2.4
```

## Requirements

- C++17 or later
- CMake 3.23 or later
