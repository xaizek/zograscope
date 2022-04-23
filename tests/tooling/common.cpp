// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include <stdexcept>

#include <boost/filesystem/operations.hpp>
#include <boost/optional.hpp>
#include "pmr/monolithic.hpp"

#include "tooling/common.hpp"
#include "utils/time.hpp"
#include "tree.hpp"

#include "tests.hpp"

namespace {

namespace fs = boost::filesystem;

class Chdir
{
public:
    explicit Chdir(const std::string &where) : previousPath(fs::current_path())
    {
        fs::current_path(where);
    }

    Chdir(const Chdir &rhs) = delete;
    Chdir & operator=(const Chdir &rhs) = delete;

    ~Chdir()
    {
        boost::system::error_code ec;
        fs::current_path(previousPath, ec);
    }

private:
    const fs::path previousPath;
};

}

TEST_CASE("Exception is thrown for files that don't exist", "[common]")
{
    Environment env;
    cpp17::pmr::monolithic mr;
    REQUIRE_THROWS_AS(buildTreeFromFile(env, "no-such-file", &mr),
                      std::runtime_error);
}

TEST_CASE("Directories aren't parsed", "[common]")
{
    Environment env;
    cpp17::pmr::monolithic mr;
    REQUIRE_THROWS_AS(buildTreeFromFile(env, "tests", &mr), std::runtime_error);
}

TEST_CASE("Parsing /dev/null file doesn't throw", "[common]")
{
    if (!boost::filesystem::exists("/dev/null")) {
        return;
    }

    Environment env;
    cpp17::pmr::monolithic mr;
    REQUIRE_NOTHROW(buildTreeFromFile(env, "/dev/null", &mr));
}

TEST_CASE("tab-size attr is taken into account on parsing", "[common]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/test.c", {
        "\t// comment"
    });
    makeFile(tempDir.str() + "/.zs/attributes", { "test.c tab-size=2" });

    Chdir chdirInsideTmpDir(tempDir.str());
    Environment env;
    cpp17::pmr::monolithic mr;
    Tree tree = *buildTreeFromFile(env, "test.c", &mr);

    const Node *node = findNode(tree, Type::Comments, "// comment");
    REQUIRE(node);
    CHECK(node->col == 3);
}

TEST_CASE("lang attr is taken into account on parsing", "[common]")
{
    TempDir tempDir("config");
    REQUIRE(fs::create_directory(tempDir.str() + "/.zs"));
    makeFile(tempDir.str() + "/test.c", {
        "# comment"
    });
    makeFile(tempDir.str() + "/.zs/attributes", { "test.c lang=make" });

    Chdir chdirInsideTmpDir(tempDir.str());
    Environment env;
    cpp17::pmr::monolithic mr;
    Tree tree = *buildTreeFromFile(env, "test.c", &mr);

    CHECK(findNode(tree, Type::Comments, "# comment"));
}
