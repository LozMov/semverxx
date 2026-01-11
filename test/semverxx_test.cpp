#include <gtest/gtest.h>

#include "semverxx.h"

using semverxx::version;

TEST(Initialization, ValidVersions) {
    {
        // Default-initialization
        version v0;
        EXPECT_EQ(v0, version());
    }
    {
        // Copy-initialization
        version v("1.2.3-alpha+001");
        version v_copy = v;
        EXPECT_EQ(v, v_copy);
    }
    {
        // Direct-list-initialization
        version v0{};
        version v1{1, 2, 3};
        EXPECT_EQ(v0, version());
        EXPECT_EQ(v1, version("1.2.3"));
    }
    {
        // Direct-initialization
        version v1(1, 2, 3);
        version v2(1, 2, 3, "alpha");
        version v3(1, 2, 3, "alpha", "001");
        EXPECT_EQ(v1, version("1.2.3"));
        EXPECT_EQ(v2, version("1.2.3-alpha"));
        EXPECT_EQ(v3, version("1.2.3-alpha+001"));
    }
    {
        // Copy-list-initialization
        version v0 = {};
        version v1 = {1, 2, 3};
        EXPECT_EQ(v0, version());
        EXPECT_EQ(v1, version("1.2.3"));
    }
}

TEST(Parsing, ValidCore) {
    version v("1.2.3");
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 2);
    EXPECT_EQ(v.patch(), 3);
    EXPECT_TRUE(v.prerelease().empty());
    EXPECT_TRUE(v.build().empty());
}

TEST(Parsing, ValidPreRelease) {
    version v("1.0.0-alpha");
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 0);
    EXPECT_EQ(v.patch(), 0);
    EXPECT_EQ(v.prerelease(), "alpha");
}

TEST(Parsing, ValidBuild) {
    version v("1.0.0+001");
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 0);
    EXPECT_EQ(v.patch(), 0);
    EXPECT_EQ(v.build(), "001");
}

TEST(Parsing, ValidFull) {
    version v("1.0.0-beta+exp.sha.5114f85");
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 0);
    EXPECT_EQ(v.patch(), 0);
    EXPECT_EQ(v.prerelease(), "beta");
    EXPECT_EQ(v.build(), "exp.sha.5114f85");
}

TEST(Parsing, ValidCornerCases) {
    // Zero versions
    version v0("0.0.0");
    EXPECT_EQ(v0.major(), 0);
    EXPECT_EQ(v0.minor(), 0);
    EXPECT_EQ(v0.patch(), 0);

    // Large components (assuming int range)
    version v1("999.999.999");
    EXPECT_EQ(v1.major(), 999);

    // Uncommon pre-release/build identifiers
    version v2("1.0.0--");
    EXPECT_EQ(v2.prerelease(), "-");
    version v3("1.0.0+-");
    EXPECT_EQ(v3.build(), "-");
    version v4("1.0.0--+-");
    EXPECT_EQ(v4.prerelease(), "-");
    EXPECT_EQ(v4.build(), "-");
    version v5("1.0.0--.-+-.-");
    EXPECT_EQ(v5.prerelease(), "-.-");
    EXPECT_EQ(v5.build(), "-.-");
}

TEST(Parsing, InvalidVersions) {
    EXPECT_THROW(version(-1, 0, 0), std::invalid_argument);
    EXPECT_THROW(version(""), std::invalid_argument); // Missing major
    EXPECT_THROW(version("1"), std::invalid_argument); // Missing minor/patch
    EXPECT_THROW(version("1.2"), std::invalid_argument); // Missing patch
    EXPECT_THROW(version("1.2.3-"), std::invalid_argument); // Empty prerelease
    EXPECT_THROW(version("1.2.3-alpha."), std::invalid_argument); // Empty prerelease identifier
    EXPECT_THROW(version("1.2.3+"), std::invalid_argument); // Empty build
    EXPECT_THROW(version("1.2.3+1."), std::invalid_argument); // Empty build identifier
    EXPECT_THROW(version("01.2.3"), std::invalid_argument); // Leading zero major
    EXPECT_THROW(version("1.02.3"), std::invalid_argument); // Leading zero minor
    EXPECT_THROW(version("1.2.03"), std::invalid_argument); // Leading zero patch
    EXPECT_THROW(version("a.b.c"), std::invalid_argument); // Non-numeric
    EXPECT_THROW(version("1.2.3.4"), std::invalid_argument); // Too many components
    EXPECT_THROW(version(".1.2.3"), std::invalid_argument); // Unexpected dot
}

TEST(Comparison, Core) {
    EXPECT_LT(version("1.0.0"), version("2.0.0"));
    EXPECT_LT(version("2.0.0"), version("2.1.0"));
    EXPECT_LT(version("2.1.0"), version("2.1.1"));

    EXPECT_LE(version("1.0.0"), version("2.0.0"));
    EXPECT_LE(version("2.0.0"), version("2.0.0"));
    EXPECT_LE(version("2.1.0"), version("2.1.1"));

    EXPECT_GT(version("2.0.0"), version("1.0.0"));
    EXPECT_EQ(version("1.2.3"), version("1.2.3"));
}

TEST(Comparison, PreRelease) {
    // When major, minor, and patch are equal,
    // a pre-release version has lower precedence than a normal version
    EXPECT_LT(version("1.0.0-alpha"), version("1.0.0"));
    EXPECT_LT(version("1.0.0-rc.1"), version("1.0.0"));
    // When all the preceding identifiers are equal,
    // a larger set of pre-release fields has a higher precedence than a smaller set
    EXPECT_LT(version("1.0.0-alpha"), version("1.0.0-alpha.1"));
    // Numeric comparison for numeric identifiers
    EXPECT_LT(version("1.0.0-beta.2"), version("1.0.0-beta.11"));
    // ASCII sort order comparison for non-numeric identifiers
    EXPECT_LT(version("1.0.0-alpha.beta"), version("1.0.0-beta"));
    EXPECT_LT(version("1.0.0-alpha.1"), version("1.0.0-alpha.beta"));
}

TEST(Comparison, BuildIgnored) {
    // Build metadata should be ignored when determining precedence
    EXPECT_EQ(version("1.0.0+001"), version("1.0.0+002"));
    EXPECT_EQ(version("1.0.0-alpha+001"), version("1.0.0-alpha+002"));
}

TEST(Setters, BumpingVersions) {
    version v("1.2.3-alpha+build");
    // bump_* methods should clear prerelease/build
    v.bump_patch();
    EXPECT_EQ(v.to_string(), "1.2.4");

    v.bump_minor();
    EXPECT_EQ(v.to_string(), "1.3.0");

    v.bump_major();
    EXPECT_EQ(v.to_string(), "2.0.0");
}

TEST(Setters, SettingVersions) {
    version v("1.2.3-alpha+build");
    // set_* methods should keep prerelease/build
    v.set_major(4);
    EXPECT_EQ(v.to_string(), "4.2.3-alpha+build");

    v.set_minor(5);
    EXPECT_EQ(v.to_string(), "4.5.3-alpha+build");

    v.set_patch(6);
    EXPECT_EQ(v.to_string(), "4.5.6-alpha+build");

    v.set_prerelease("beta");
    EXPECT_EQ(v.to_string(), "4.5.6-beta+build");
    v.set_build("new-build");
    EXPECT_EQ(v.to_string(), "4.5.6-beta+new-build");

    v.append_prerelease("2");
    EXPECT_EQ(v.to_string(), "4.5.6-beta.2+new-build");

    v.append_build("a");
    EXPECT_EQ(v.to_string(), "4.5.6-beta.2+new-build.a");

    v.clear_prerelease();
    EXPECT_EQ(v.to_string(), "4.5.6+new-build.a");

    v.clear_build();
    EXPECT_EQ(v.to_string(), "4.5.6");
}

TEST(Misc, ToString) {
    version v(1, 4, 2, "alpha.1", "001");
    EXPECT_EQ(v.to_string(), "1.4.2-alpha.1+001");
}

TEST(Misc, UserDefinedLiterals) {
    using namespace semverxx::literals;
    auto v = "1.2.3-x.7.z.92+a"_v;
    EXPECT_EQ(v, version(1, 2, 3, "x.7.z.92", "a"));
}

#if __cpp_structured_bindings >= 201606L
TEST(Misc, StructuredBinding) {
    version v("1.2.3-x.7.z.92+a");
    auto [major, minor, patch, prerelease, build] = v;
    EXPECT_EQ(v, version(major, minor, patch, prerelease, build));
}
#endif