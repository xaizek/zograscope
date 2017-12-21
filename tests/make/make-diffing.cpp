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

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Condition is a value of conditionals", "[make][comparison]")
{
    diffMake(R"(
        ifeq (,$(findstring gcc,$(CC)))  ## Deletions
            set := to this               ## Deletions
        endif                            ## Deletions

        ifneq ($(OS),Windows_NT)
        endif
    )", R"(
        ifneq ($(OS),Windows_NT)
            bin_suffix :=                ## Additions
        endif
    )");
}

TEST_CASE("Statements are not mixed up", "[make][comparison]")
{
    diffMake(R"(
        CXXFLAGS += prefix$(CFLAGS)
        CXXFLAGS += prefix$(CFLAGS)suffix    ## Mixed
        CXXFLAGS += $(CFLAGS)suffix
    )", R"(
        CXXFLAGS += prefix$(CFLAGS)
        CXXFLAGS += prefix$(CXXFLAGS)suffix  ## Mixed
        CXXFLAGS += $(CFLAGS)suffix
    )");
}

TEST_CASE("Removal export all/unexport statements is detected",
          "[make][comparison]")
{
    // The assignment there is to make Statements node match.
    diffMake(R"(
        export    ## Deletions
        unexport  ## Deletions

        a = b
    )", R"(
        a = b
    )");
}
