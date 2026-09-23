/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2026  EdenWorX
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include "CppUTest/TestHarness.h"

#include "update.h"

// The checker thread itself (network I/O against the live GitHub API) is
// validated in-game; everything here covers the pure helpers round-tripping
// through parse/compare/extract with deterministic expectations.

TEST_GROUP(Update){};

TEST(Update, ParseFullTriple) {
    VersionTuple out;
    CHECK(parse_version_tuple("v6.7.1", out));
    CHECK_EQUAL(6, out.major);
    CHECK_EQUAL(7, out.minor);
    CHECK_EQUAL(1, out.patch);
}

TEST(Update, ParseWithoutLeadingV) {
    VersionTuple out;
    CHECK(parse_version_tuple("6.7.1", out));
    CHECK_EQUAL(6, out.major);
    CHECK_EQUAL(7, out.minor);
    CHECK_EQUAL(1, out.patch);
}

TEST(Update, ParsePartialTripleZeroFills) {
    VersionTuple out;
    CHECK(parse_version_tuple("v6", out));
    CHECK_EQUAL(6, out.major);
    CHECK_EQUAL(0, out.minor);
    CHECK_EQUAL(0, out.patch);
}

TEST(Update, ParseRejectsGarbage) {
    VersionTuple out;
    CHECK(!parse_version_tuple("", out));
    CHECK(!parse_version_tuple("abc", out));
    CHECK(!parse_version_tuple("v", out));
    CHECK(!parse_version_tuple(nullptr, out));
    CHECK(!parse_version_tuple("99999999999999999999.1", out));
}

TEST(Update, ParseStopsAtTrailingGarbage) {
    VersionTuple out;
    CHECK(parse_version_tuple("6.7.x", out));
    CHECK_EQUAL(6, out.major);
    CHECK_EQUAL(7, out.minor);
    CHECK_EQUAL(0, out.patch);
}

TEST(Update, NewerComparison) {
    CHECK(version_is_newer({6, 8, 0}, {6, 7, 1}));
    CHECK(version_is_newer({7, 0, 0}, {6, 9, 9}));
    CHECK(version_is_newer({6, 7, 2}, {6, 7, 1}));
    CHECK(version_is_newer({6, 10, 0}, {6, 9, 9}));
    CHECK(!version_is_newer({6, 7, 1}, {6, 7, 1}));
    CHECK(!version_is_newer({6, 7, 0}, {6, 7, 1}));
    CHECK(!version_is_newer({5, 9, 9}, {6, 0, 0}));
}

TEST(Update, ExtractTagFromReleaseBody) {
    std::string tag;
    CHECK(extract_tag_name("{\"tag_name\":\"v6.7.1\",\"draft\":false}", tag));
    STRCMP_EQUAL("v6.7.1", tag.c_str());
}

TEST(Update, ExtractTagRejectsBadBodies) {
    std::string tag;
    CHECK(!extract_tag_name("{\"name\":\"nothing here\"}", tag));
    CHECK(!extract_tag_name("{\"tag_name\":\"\"}", tag));
    CHECK(!extract_tag_name("{\"tag_name\":\"v6.7.1}", tag));
    CHECK(!extract_tag_name("", tag));
}
