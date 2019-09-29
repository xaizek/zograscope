// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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
#include <string>

#include "pmr/monolithic.hpp"

#include "Language.hpp"
#include "TreeBuilder.hpp"

static std::string cFile = R"(
    int main(int argc, char *argv[]) {
        return 0;
    }
)";

static std::string cxxFile = R"(
    Class::Class(temp<int> &ref) {
        throw new something();
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

TEST_CASE("C is detected", "[language]")
{
    std::string fileName;

    SECTION("Makefile stem of source file") {
        fileName = "Makefile.c";
    }
    SECTION("Makefile stem of header file") {
        fileName = "Makefile.h";
    }

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);
    CHECK_FALSE(lang->parse(cFile, "<input>", false, mr).hasFailed());
}

TEST_CASE("C++ is detected", "[.srcml][language]")
{
    auto names = {
        "Makefile.cpp", "file.hpp",
        "c.cxx", "main.hpp",
        "head.cc", "tail.hh",
    };

    for (const std::string &fileName : names) {
        INFO("Filename: " << fileName);

        cpp17::pmr::monolithic mr;
        std::unique_ptr<Language> lang = Language::create(fileName);
        CHECK_FALSE(lang->parse(cxxFile, "<input>", false, mr).hasFailed());
    }
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
    SECTION("With suffix") {
        fileName = "Makefile.win";
    }
    SECTION("With prefix") {
        fileName = "prefix_Makefile";
    }
    SECTION("With .mk suffix") {
        fileName = "config.mk";
    }
    SECTION("With .mak suffix") {
        fileName = "config.mak";
    }

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);
    CHECK_FALSE(lang->parse(makeFile, "<input>", false, mr).hasFailed());
}

TEST_CASE("Language matching", "[language]")
{
    SECTION("Matching against any supported language")
    {
        CHECK(Language::matches("Makefile.win", ""));
        CHECK(Language::matches("file.c", ""));
        CHECK(Language::matches("file.h", ""));
        CHECK(Language::matches("file.cpp", ""));
    }

    SECTION("Matching against any specific languages")
    {
        CHECK(Language::matches("Makefile", "make"));
        CHECK(Language::matches("file.c", "c"));
        CHECK(Language::matches("file.cpp", "cxx"));

        CHECK_FALSE(Language::matches("Makefile", "cxx"));
        CHECK_FALSE(Language::matches("file.c", "make"));
        CHECK_FALSE(Language::matches("file.cpp", "c"));
    }

    SECTION("Headers")
    {
        CHECK(Language::matches("file.h", "c"));
        CHECK(Language::matches("file.h", "cxx"));
        CHECK_FALSE(Language::matches("file.h", "make"));
    }
}
