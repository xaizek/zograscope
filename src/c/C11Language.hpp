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

#ifndef ZOGRASCOPE__C__C11LANGUAGE_HPP__
#define ZOGRASCOPE__C__C11LANGUAGE_HPP__

#include "c/c11-parser.hpp"
#include "Language.hpp"

// C11-specific routines.
class C11Language : public Language
{
public:
    // Initializes C11-specific data.
    C11Language();

public:
    // Maps language-specific token to an element of Type enumeration.
    virtual Type mapToken(int token) const override;

private:
    Type map[NTOKENS]; // Static token-type to Type map.
};

#endif // ZOGRASCOPE__C__C11LANGUAGE_HPP__
