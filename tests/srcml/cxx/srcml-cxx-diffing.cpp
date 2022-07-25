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

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("C++ functions are matched",
          "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map[1] = 1;
        }

        Type
        SrcmlCxxLanguage::mapToken(
                                int /*token*/  /// Deletions
                                ) const {
            return Type::Other;                /// Deletions
        }
    )", R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map[1] = 1;
        }

        Type
        SrcmlCxxLanguage::mapToken(
                                int token      /// Additions
                                ) const {
            return static_cast<Type>(token);   /// Additions
        }
    )");
}

TEST_CASE("Function body changes don't cause everything to be moved",
          "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void setEntry(const Entry *entry) {
            prevNode = (currNode == entry.node ? nullptr : currNode);
            currNode = entry.node;

            prevHighlight = currHighlight;
            currHighlight = &getHighlight(*currNode, entry.moved, entry.state,
                                          lang);
        }
    )", R"(
        void setEntry(const Entry *entry) {
            prevNode = (currNode == entry.node ? nullptr : currNode);
            currNode = entry.node;

            prevMoved = currMoved;    /// Additions
            currMoved = entry.moved;  /// Additions

            prevHighlight = currHighlight;
            currHighlight = &getHighlight(*currNode, entry.moved, entry.state,
                                          lang);
        }
    )");
}

TEST_CASE("Terminals tie resolution in C++", "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void f() {
            switch (id) {
                case This: return anything;
                case That: return anything;
                case Final: return anything;
            }
        }
    )", R"(
        void f() {
            switch (id) {
                case This: return anything;
                case NewThing: return anything;  /// Additions
                case OneMore: return anything;   /// Additions
                case That: return anything;
                case Final: return anything;
            }
        }
    )");
}

TEST_CASE("Complete replacement",
          "[.srcml][srcml095-broken][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        int something;  /// Deletions
    )", R"123(
        void f() {      /// Additions
            R"(         /// Additions
            )";         /// Additions
            makePred;   /// Additions
        }               /// Additions
    )123");
}

TEST_CASE("Condition removal", "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (
                -parent == SrcmlCxxSType::Function &&  /// Deletions
                child == SrcmlCxxSType::Block) {
            }
        }
    )", R"123(
        void f() {
            if (child == SrcmlCxxSType::Block) {
            }
        }
    )123");
}

TEST_CASE("C++ condition addition", "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (fullVal.back() == ';') {
            }
        }
    )", R"(
        void f() {
            if (
                fullVal.back() == ':' ||  /// Additions
                fullVal.back() == ';') {
            }
        }
    )");
}

TEST_CASE("C++ subexpr addition", "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void f() {
            return -child->stype != SrcmlCxxSType::Function;
        }
    )", R"(
        void f() {
            return
                -child->stype != SrcmlCxxSType::Parameter &&  /// Additions
                -child->stype != SrcmlCxxSType::Function;
        }
    )");
}

TEST_CASE("C++ statement deletion", "[.srcml][srcml-cxx][comparison]")
{
    diffSrcmlCxx(R"(
        void f() {
            for (int i : is) {
                if (ci[i]->satellite) {       /// Deletions
                    continue;                 /// Deletions
                }                             /// Deletions

                for (int j : js) {
                    if (cj[j]->satellite) {
                        continue;
                    }
                }
            }
        }
    )", R"(
        void f() {
            for (int i : is) {
                for (int j : js) {
                    if (cj[j]->satellite) {
                        continue;
                    }
                }
            }
        }
    )");
}
