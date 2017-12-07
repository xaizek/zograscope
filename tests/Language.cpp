// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
#include <string>

#include "pmr/monolithic.hpp"

#include "Language.hpp"
#include "TreeBuilder.hpp"

static std::string cFile = R"(
    int main(int argc, char *argv[]) {
        return 0;
    }
)";

static std::string makeFile = R"(
    target1 := something
    .PHONY: all
    all: $(target1) target2
)";

TEST_CASE("Language can be forced", "[language]")
{
    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang;
    std::string str;

    SECTION("C") {
        lang = Language::create("<input>", "c");
        str = cFile;
    }
    SECTION("Make") {
        lang = Language::create("<input>", "make");
        str = makeFile;
    }

    CHECK_FALSE(lang->parse(str, "<input>", false, mr).hasFailed());
}

TEST_CASE("Unknown language causes exception", "[language]")
{
    REQUIRE_THROWS_AS(Language::create("<input>", "wrong"), std::runtime_error);
}

TEST_CASE("C is the default", "[language]")
{
    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create("<input>");
    CHECK_FALSE(lang->parse(cFile, "<input>", false, mr).hasFailed());
}

TEST_CASE("Make is detected", "[language]")
{
    std::string fileName;

    SECTION("Canonical Makefile name") {
        fileName = "Makefile";
    }
    SECTION("Lower case") {
        fileName = "makefile";
    }
    SECTION("Upper case") {
        fileName = "MAKEFILE";
    }

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);
    CHECK_FALSE(lang->parse(makeFile, "<input>", false, mr).hasFailed());
}
