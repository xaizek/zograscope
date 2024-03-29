// Copyright (C) 2021 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "Catch/catch.hpp"

#include <boost/filesystem/operations.hpp>

#include "tooling/Config.hpp"

#include "tests.hpp"

namespace fs = boost::filesystem;

TEST_CASE("No configuration", "[tooling][config]")
{
    TempDir tempDir("config");
    Config config(tempDir.str());

    CHECK(config.shouldProcessFile("ignored.cpp"));

    Attrs attrs = config.lookupAttrs("file.c");
    CHECK(attrs.lang == "");
    CHECK(attrs.tabWidth == 4);
}

TEST_CASE("Exclude file", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", {
        "# a comment",
        "sub/ignored.cpp",
        "ignored.cpp",
        "dir",
        "",
    });

    Config config(tempDir.str());

    SECTION("Relative paths") {
        CHECK(config.shouldProcessFile("# a comment"));
        CHECK(config.shouldProcessFile(""));

        CHECK(config.shouldProcessFile("../../ignored.cpp"));
        CHECK(!config.shouldProcessFile("not/ignored.cpp"));
        CHECK(!config.shouldProcessFile("./not/to/be/../../ignored.cpp"));

        CHECK(!config.shouldProcessFile("ignored.cpp"));
        CHECK(!config.shouldProcessFile("sub/ignored.cpp"));
        CHECK(!config.shouldProcessFile("./sub/bla/../ignored.cpp"));
    }
    SECTION("Absolute paths") {
        CHECK(config.shouldProcessFile("/random-file"));
        CHECK(config.shouldProcessFile(tempDir.str() + "../"));
        CHECK(config.shouldProcessFile(tempDir.str()));
        CHECK(config.shouldProcessFile(tempDir.str() + "/not-ignored.cpp"));

        CHECK(!config.shouldProcessFile(tempDir.str() + "/ignored.cpp"));
    }
}

TEST_CASE("Globs in exclude file", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", {
        "ignored-everywhere.*",
        "sub1/ignored.*",
        "sub2/*ignored",
        "sub3/ignored.?",
        "sub4/ignored.[ch]",
        "sub5/ignored.[^C]",
        "sub6/ignored.\\[",
    });

    Config config(tempDir.str());

    CHECK(!config.shouldProcessFile("ignored-everywhere.c"));
    CHECK(!config.shouldProcessFile("sub1/ignored-everywhere.c"));
    CHECK(!config.shouldProcessFile("sub/sub/ignored-everywhere.c"));

    // The first one passing is OK, because we wouldn't even check for it on
    // real traversal (directory would be skipped).
    CHECK(config.shouldProcessFile("sub1/ignored.c/file"));
    CHECK(config.shouldProcessFile("sub1/notignored.c"));
    CHECK(!config.shouldProcessFile("sub1/ignored."));
    CHECK(!config.shouldProcessFile("sub1/ignored.c"));

    CHECK(config.shouldProcessFile("sub2/ignored.not"));
    CHECK(!config.shouldProcessFile("sub2/ignored"));
    CHECK(!config.shouldProcessFile("sub2/.ignored"));

    CHECK(config.shouldProcessFile("sub3/ignored."));
    CHECK(config.shouldProcessFile("sub3/ignored.ab"));
    CHECK(!config.shouldProcessFile("sub3/ignored.c"));
    CHECK(!config.shouldProcessFile("sub3/ignored.h"));

    CHECK(config.shouldProcessFile("sub4/ignored.ch"));
    CHECK(config.shouldProcessFile("sub4/ignored."));
    CHECK(config.shouldProcessFile("sub4/ignored.x"));
    CHECK(!config.shouldProcessFile("sub4/ignored.c"));
    CHECK(!config.shouldProcessFile("sub4/ignored.h"));

    CHECK(config.shouldProcessFile("sub5/ignored.C"));
    CHECK(!config.shouldProcessFile("sub5/ignored.c"));

    CHECK(config.shouldProcessFile("sub6/ignored.]"));
    CHECK(!config.shouldProcessFile("sub6/ignored.["));
}

TEST_CASE("Root-only matching in exclude file", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", {
        "ignore-all*",
        "/ignore-in-root",
        "/dir/file",
    });

    Config config(tempDir.str());

    CHECK(!config.shouldProcessFile("ignore-all"));
    CHECK(!config.shouldProcessFile("sub/ignore-all"));
    CHECK(!config.shouldProcessFile("sub/sub/ignore-all"));
    CHECK(!config.shouldVisitDirectory("ignore-all"));

    CHECK(config.shouldProcessFile("sub/ignore-in-root"));
    CHECK(config.shouldProcessFile("sub/sub/ignore-in-root"));
    CHECK(!config.shouldProcessFile("ignore-in-root"));

    CHECK(!config.shouldProcessFile("dir/file"));
}

TEST_CASE("Whitelisting", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", {
        "ignored",
        "/*",
        "!/src",
    });

    Config config(tempDir.str());

    CHECK(!config.shouldProcessFile("ignored"));
    CHECK(!config.shouldProcessFile("src/ignored"));
    CHECK(!config.shouldProcessFile("tests/ignored"));

    CHECK(config.shouldProcessFile("src"));
    CHECK(config.shouldProcessFile("src/file"));
    CHECK(!config.shouldProcessFile("readme"));
    CHECK(!config.shouldProcessFile("install"));
}

TEST_CASE("Directory-only matching in exclude file", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", {
        "/*/",
        "!/src/",
        "dir/",
    });

    Config config(tempDir.str());

    CHECK(config.shouldVisitDirectory("src"));
    CHECK(!config.shouldVisitDirectory("readme"));
    CHECK(!config.shouldVisitDirectory("install"));
    CHECK(!config.shouldVisitDirectory("dir"));
    CHECK(!config.shouldVisitDirectory("sub/dir"));

    CHECK(config.shouldProcessFile("src"));
    CHECK(config.shouldProcessFile("readme"));
    CHECK(config.shouldProcessFile("install"));
    CHECK(config.shouldProcessFile("dir"));
    CHECK(config.shouldProcessFile("sub/dir"));
}

TEST_CASE("Attributes file parsing", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/attributes", {
        " # a comment",
        "# another comment",
        " file.c tab-size=2  ",
        "file.c unknown-option=value ",
        " file.c unknown-option-wo-value",
    });

    Config config(tempDir.str());

    CHECK(config.lookupAttrs("file.c").tabWidth == 2);
}

TEST_CASE("Attributes file application", "[tooling][config]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/attributes", {
        " # a comment",
        "* tab-size=1",
        "#another tab-size=10",
        "file.c tab-size=2",
        "another.c tab-size=2 lang=make",
        "*.c tab-size=3",
        "another.c tab-size=5",
        "some.c",
    });

    Config config(tempDir.str());

    CHECK(config.lookupAttrs("something").tabWidth == 1);
    CHECK(config.lookupAttrs("#another").tabWidth == 1);

    CHECK(config.lookupAttrs("file.c").tabWidth == 3);
    CHECK(config.lookupAttrs("almost-any.c").tabWidth == 3);
    CHECK(config.lookupAttrs("another.c").tabWidth == 5);
    CHECK(config.lookupAttrs("some.c").tabWidth == 3);

    CHECK(config.lookupAttrs("dir/another.c").tabWidth == 5);

    CHECK(config.lookupAttrs("something").lang == "");
    CHECK(config.lookupAttrs("another.c").lang == "make");
}
