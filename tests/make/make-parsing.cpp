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

// These are tests of more advanced properties of a parser, which are much
// easier and reliable to test by performing comparison.

#include "Catch/catch.hpp"

#include "utils/time.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Statement list in conditionals is decomposed",
          "[make][comparison][parsing]")
{
    diffMake(R"(
        ifdef a
            EXTRA_LDFLAGS := -g
            target := \
                      dbug       ## Deletions
        endif
    )", R"(
        ifdef a
            EXTRA_LDFLAGS := -g
            target := \
                      debug      ## Additions
        endif
    )");
}

TEST_CASE("Comments aren't recognized inside strings",
          "[make][comparison][parsing]")
{
    diffMake(R"(
target:
	echo '#define VERSION "0.9"' > $@; \            ## Mixed
	echo '#define WITH_BUILD_TIMESTAMP 1' >> $@; \
#	echo '#define HAVE_FILE_PROG' >> $@;
    )", R"(
target:
	echo '#define VERSION "0.9.1-beta"' > $@; \     ## Mixed
	echo '#define WITH_BUILD_TIMESTAMP 1' >> $@; \
#	echo '#define HAVE_FILE_PROG' >> $@;
    )");
}
