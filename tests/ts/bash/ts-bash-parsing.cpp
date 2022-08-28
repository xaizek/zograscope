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

// These are tests of more advanced properties of a parser, which are much
// easier and reliable to test by performing comparison.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Bash splices braces into function", "[ts-bash][comparison][parsing]")
{
    diffTsBash(R"raw(
        function _editinplace() {
            local tmp_file="$(mktemp "/tmp/bash.XXXXXX")" ### Deletions
        }
    )raw", R"raw(
        function _editinplace() {
            READLINE_LINE="$(< "$tmp_file")" ### Additions
        }
    )raw");
}
