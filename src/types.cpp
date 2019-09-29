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

#include "types.hpp"

#include <cassert>

#include <ostream>

#include "c/c11-parser.hpp"

std::ostream &
operator<<(std::ostream &os, Type type)
{
    switch (type) {
        case Type::Virtual:            return (os << "Virtual");
        case Type::Functions:          return (os << "Functions");
        case Type::UserTypes:          return (os << "UserTypes");
        case Type::Identifiers:        return (os << "Identifiers");
        case Type::Jumps:              return (os << "Jumps");
        case Type::Specifiers:         return (os << "Specifiers");
        case Type::Types:              return (os << "Types");
        case Type::LeftBrackets:       return (os << "LeftBrackets");
        case Type::RightBrackets:      return (os << "RightBrackets");
        case Type::Comparisons:        return (os << "Comparisons");
        case Type::Operators:          return (os << "Operators");
        case Type::LogicalOperators:   return (os << "LogicalOperators");
        case Type::Assignments:        return (os << "Assignments");
        case Type::Directives:         return (os << "Directives");
        case Type::Comments:           return (os << "Comments");
        case Type::NonInterchangeable: return (os << "NonInterchangeable");
        case Type::StrConstants:       return (os << "StrConstants");
        case Type::IntConstants:       return (os << "IntConstants");
        case Type::FPConstants:        return (os << "FPConstants");
        case Type::CharConstants:      return (os << "CharConstants");
        case Type::Keywords:           return (os << "Keywords");
        case Type::Other:              return (os << "Other");
    }

    assert(false && "Unhandled enumeration item");
    return (os << "<UNKNOWN:" << static_cast<int>(type) << ">");
}

Type
canonizeType(Type type)
{
    if (type == Type::UserTypes) {
        return Type::Types;
    }

    return type;
}
