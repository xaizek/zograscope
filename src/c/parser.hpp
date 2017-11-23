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

#ifndef ZOGRASCOPE__C__PARSER_HPP__
#define ZOGRASCOPE__C__PARSER_HPP__

#include <string>

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

class TreeBuilder;

TreeBuilder parse(const std::string &contents,
                  const std::string &fileName,
                  bool debug,
                  cpp17::pmr::monolithic &mr);

#endif // ZOGRASCOPE__C__PARSER_HPP__
