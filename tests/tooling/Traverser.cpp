// Copyright (C) 2022 xaizek <xaizek@posteo.net>
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

#include "tooling/Traverser.hpp"
#include "tooling/common.hpp"

#include "tests.hpp"

TEST_CASE("Traverser doesn't exclude passed in paths", "[tooling][traverser]")
{
    TempDir tempDir("config");
    REQUIRE(boost::filesystem::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/.zs/exclude", { "test.c" });
    std::string file = tempDir.str() + "/test.c";
    makeFile(file, { });

    std::vector<std::string> paths;
    auto handler = [&](const std::string &path) {
        paths.push_back(path);
        return true;
    };

    Chdir chdirInsideTmpDir(tempDir.str());
    Environment env;

    CHECK(!Traverser({ "." }, "", env.getConfig(), handler).search());
    CHECK(paths.size() == 0);
    CHECK(Traverser({ file }, "", env.getConfig(), handler).search());
    CHECK(paths.size() == 1);
}

TEST_CASE("Traverser accounts for lang attribute", "[tooling][traverser]")
{
    TempDir tempDir("config");
    REQUIRE(boost::filesystem::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/test.c", { });
    makeFile(tempDir.str() + "/.zs/attributes", { "test.c lang=make" });

    std::vector<std::string> paths;
    auto handler = [&](const std::string &path) {
        paths.push_back(path);
        return true;
    };

    Chdir chdirInsideTmpDir(tempDir.str());
    Environment env;

    Traverser traverser({ "." }, "make", env.getConfig(), handler);
    CHECK(traverser.search());

    CHECK(paths.size() == 1);
}
