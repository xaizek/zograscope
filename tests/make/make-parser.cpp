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

// These are tests of basic properties of a parser.  Things like:
//  - whether some constructs can be parsed
//  - whether elements are identified correctly

#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>

#include "tests.hpp"

TEST_CASE("Error is printed on incorrect Makefile syntax", "[make][parser]")
{
    StreamCapture coutCapture(std::cout);
    CHECK_FALSE(makeIsParsed(":"));
    REQUIRE(boost::starts_with(coutCapture.get(), "<input>:1:1:"));
}

TEST_CASE("Empty Makefile is OK", "[make][parser]")
{
    CHECK(makeIsParsed(""));
    CHECK(makeIsParsed("      "));
    CHECK(makeIsParsed("   \n   "));
    CHECK(makeIsParsed("\t\n \t \n"));
}

TEST_CASE("Comments are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed(" # comment"));
    CHECK(makeIsParsed("# comment "));
    CHECK(makeIsParsed(" # comment "));
}

TEST_CASE("Assignments are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around assignment statement") {
        CHECK(makeIsParsed("OS := os "));
        CHECK(makeIsParsed(" OS := os "));
        CHECK(makeIsParsed(" OS := os"));
    }
    SECTION("Whitespace around assignment") {
        CHECK(makeIsParsed("OS := os"));
        CHECK(makeIsParsed("OS:= os"));
        CHECK(makeIsParsed("OS :=os"));
    }
    SECTION("Various assignment kinds") {
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS += -std=c++11 a:b"));
        CHECK(makeIsParsed("CXX ?= g++"));
        CHECK(makeIsParsed("CXX ::= g++"));
        CHECK(makeIsParsed("CXX != whereis g++"));
    }
    SECTION("One variable") {
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)suffix"));
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)suffix"));
    }
    SECTION("Two variables") {
        CHECK(makeIsParsed("CXXFLAGS = $(var1)$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = $(var1)val$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)$(var2)val"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)val$(var2)val"));
    }
}

TEST_CASE("Functions are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around function statement") {
        CHECK(makeIsParsed("$(info a)"));
        CHECK(makeIsParsed(" $(info a)"));
        CHECK(makeIsParsed(" $(info a) "));
        CHECK(makeIsParsed("$(info a) "));
        CHECK(makeIsParsed("$(info a )"));
    }
    SECTION("Whitespace around comma") {
        CHECK(makeIsParsed("$(info arg1,arg2)"));
        CHECK(makeIsParsed("$(info arg1, arg2)"));
        CHECK(makeIsParsed("$(info arg1 ,arg2)"));
        CHECK(makeIsParsed("$(info arg1 , arg2)"));
    }
    SECTION("Empty arguments") {
        CHECK(makeIsParsed("$(patsubst , ,)"));
        CHECK(makeIsParsed("$(patsubst ,,)"));
        CHECK(makeIsParsed("$(patsubst a,,)"));
        CHECK(makeIsParsed("$(patsubst a,b,)"));
    }
}

TEST_CASE("Targets are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around colon") {
        CHECK(makeIsParsed("target: prereq"));
        CHECK(makeIsParsed("target : prereq"));
        CHECK(makeIsParsed("target :prereq"));
    }
    SECTION("No prerequisites") {
        CHECK(makeIsParsed("target:"));
    }
    SECTION("Multiple prerequisites") {
        CHECK(makeIsParsed("target: prereq1 prereq2"));
    }
    SECTION("Multiple targets") {
        CHECK(makeIsParsed("debug release sanitize-basic: all"));
    }
    SECTION("Expressions in prerequisites and targets") {
        CHECK(makeIsParsed("target: $(dependencies)"));
        CHECK(makeIsParsed("$(target): dependencies"));
        CHECK(makeIsParsed("$(target): $(dependencies)"));
    }
}

TEST_CASE("Recipes are parsed in a Makefile", "[make][parser]")
{
    const char *const singleLine = R"(
target: prereq
	the only recipe
    )";
    CHECK(makeIsParsed(singleLine));

    const char *const multipleLines = R"(
target: prereq
	first recipe
	second recipe
    )";
    CHECK(makeIsParsed(multipleLines));

    const char *const withParens = R"(
target: prereq
	(echo something)
    )";
    CHECK(makeIsParsed(withParens));

    const char *const assignment = R"(
        $(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses
    )";
    CHECK(makeIsParsed(assignment));
}

TEST_CASE("Conditionals are parsed in a Makefile", "[make][parser]")
{
    const char *const withBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
            bin_suffix :=
        endif
    )";
    CHECK(makeIsParsed(withBody_withoutElse));

    const char *const withoutBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withoutElse));

    const char *const withBody_withElse = R"(
        ifeq ($(OS),Windows_NT)
            bin_suffix :=
        else
            bin_suffix := .exe
        endif
    )";
    CHECK(makeIsParsed(withBody_withElse));

    const char *const withoutBody_withElse = R"(
        ifeq ($(OS),Windows_NT)
            bin_suffix :=
        else
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withElse));

    const char *const nested = R"(
        ifneq ($(OS),Windows_NT)
            ifneq ($(OS),Windows_NT)
            endif
        endif
    )";
    CHECK(makeIsParsed(nested));

    const char *const conditionalInRecipes = R"(
reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif
    )";
    CHECK(makeIsParsed(conditionalInRecipes));

    const char *const conditionalsInRecipes = R"(
reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
else
endif
    )";
    CHECK(makeIsParsed(conditionalsInRecipes));
}

TEST_CASE("Defines are parsed in a Makefile", "[make][parser]")
{
    const char *const noBody = R"(
        define pattern
        endef
    )";
    CHECK(makeIsParsed(noBody));

    const char *const noSuffix = R"(
        define pattern
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(noSuffix));
}

TEST_CASE("Line escaping works in a Makefile", "[make][parser]")
{
    const char *const multiline = R"(
        pos = $(strip $(eval T := ) \
                      $(eval i := -1) \
                      $(foreach elem, $1, \
                                $(if $(filter $2,$(elem)), \
                                              $(eval i := $(words $T)), \
                                              $(eval T := $T $(elem)))) \
                      $i)
    )";
    CHECK(makeIsParsed(multiline));
}

TEST_CASE("Substitutions are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("lib_objects := $(lib_sources:%.cpp=$(out_dir)/%.o)"));
}

TEST_CASE("Makefile keywords are not found inside text/ids", "[make][parser]")
{
    CHECK(makeIsParsed("EXTRA_CXXFLAGS += -fsanitize=undefined"));
}
