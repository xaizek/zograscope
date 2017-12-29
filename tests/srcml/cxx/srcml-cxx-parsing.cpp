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

TEST_CASE("Functions are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void readFile(const std::string &path);    /// Deletions
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(argv, options);  /// Deletions
        }
    )", R"(
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(args, options);  /// Additions
        }
    )");
}
