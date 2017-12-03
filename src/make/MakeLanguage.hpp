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

#ifndef ZOGRASCOPE__MAKE__MAKELANGUAGE_HPP__
#define ZOGRASCOPE__MAKE__MAKELANGUAGE_HPP__

#include "make/make-parser.hpp"
#include "Language.hpp"

// Make-specific routines.
class MakeLanguage : public Language
{
public:
    // Initializes Make-specific data.
    MakeLanguage();

public:
    // Maps language-specific token to an element of Type enumeration.
    virtual Type mapToken(int token) const override;
    // Parses source file into a tree.
    virtual TreeBuilder parse(const std::string &contents,
                              const std::string &fileName,
                              bool debug,
                              cpp17::pmr::monolithic &mr) const override;

private:
    Type map[NTOKENS]; // Static token-type to Type map.
};

#endif // ZOGRASCOPE__MAKE__MAKELANGUAGE_HPP__
