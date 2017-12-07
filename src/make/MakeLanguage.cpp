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

#include "MakeLanguage.hpp"

#include "make/make-parser.hpp"
#include "types.hpp"

MakeLanguage::MakeLanguage() : map()
{
    map[COMMENT] = Type::Comments;
    map[ASSIGN_OP] = Type::Assignments;
    map[CALL_PREFIX] = Type::LeftBrackets;
    map[CALL_SUFFIX] = Type::LeftBrackets;
    map['('] = Type::LeftBrackets;
    map[')'] = Type::RightBrackets;
    map[':'] = Type::Operators;

    map[VAR] = Type::UserTypes;

    map[OVERRIDE] = Type::Keywords;
    map[EXPORT] = Type::Keywords;

    map[IFDEF] = Type::Directives;
    map[IFNDEF] = Type::Directives;
    map[IFEQ] = Type::Directives;
    map[IFNEQ] = Type::Directives;
    map[ELSE] = Type::Directives;
    map[ENDIF] = Type::Directives;

    map[DEFINE] = Type::Directives;
    map[ENDEF] = Type::Directives;

    map[INCLUDE] = Type::Directives;
}

Type
MakeLanguage::mapToken(int token) const
{
    return map[token];
}

TreeBuilder
MakeLanguage::parse(const std::string &contents, const std::string &fileName,
                    bool debug, cpp17::pmr::monolithic &mr) const
{
    return make_parse(contents, fileName, debug, mr);
}
