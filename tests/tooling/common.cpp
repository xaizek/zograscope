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

TEST_CASE("Exception is thrown for files that don't exist", "[common]")
{
    CommonArgs args = {};
    TimeReport tr;
    cpp17::pmr::monolithic mr;
    REQUIRE_THROWS_AS(buildTreeFromFile("no-such-file", args, tr, &mr),
                      std::runtime_error);
}

TEST_CASE("Directories aren't parsed", "[common]")
{
    CommonArgs args = {};
    TimeReport tr;
    cpp17::pmr::monolithic mr;
    REQUIRE_THROWS_AS(buildTreeFromFile("tests", args, tr, &mr),
                      std::runtime_error);
}

TEST_CASE("Parsing /dev/null file doesn't throw", "[common]")
{
    if (!boost::filesystem::exists("/dev/null")) {
        return;
    }

    CommonArgs args = {};
    TimeReport tr;
    cpp17::pmr::monolithic mr;
    REQUIRE_NOTHROW(buildTreeFromFile("/dev/null", args, tr, &mr));
}
