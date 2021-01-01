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

#include <fstream>
#include <string>
#include <vector>

#include "tooling/Config.hpp"

#include "tests.hpp"

namespace fs = boost::filesystem;

static void makeFile(const std::string &path,
                     const std::vector<std::string> &lines);

TEST_CASE("No configuration", "[tooling][config]")
{
    TempDir tempDir("config");
    Config config(tempDir.str());

    CHECK(config.shouldProcessFile("ignored.cpp"));
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

        CHECK(config.shouldProcessFile("not/ignored.cpp"));
        CHECK(config.shouldProcessFile("./not/to/be/../../ignored.cpp"));
        CHECK(config.shouldProcessFile("../../ignored.cpp"));

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

// Creates a file with specified contents.
static void
makeFile(const std::string &path, const std::vector<std::string> &lines)
{
    std::ofstream file(path);
    REQUIRE(file.is_open());
    for (const std::string &line : lines) {
        file << line << '\n';
    }
}
