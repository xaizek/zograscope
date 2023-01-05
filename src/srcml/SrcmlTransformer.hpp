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

#ifndef ZOGRASCOPE_SRCML_SRCMLTRANSFORMER_HPP_
#define ZOGRASCOPE_SRCML_SRCMLTRANSFORMER_HPP_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/utility/string_ref.hpp>

namespace tinyxml2 {
    class XMLElement;
    class XMLNode;
}

class PNode;
class TreeBuilder;

enum class SType : std::uint8_t;
enum class Type : std::uint8_t;

// Uses srcml to parse a file and transforms the result into PTree.
class SrcmlTransformer
{
public:
    // Remembers parameters to use them later.  `contents`, `map` and `keywords`
    // have to be lvalues.
    SrcmlTransformer(const std::string &contents,
                     const std::string &path,
                     TreeBuilder &tb,
                     const std::string &language,
                     const std::unordered_map<std::string, SType> &map,
                     const std::unordered_set<std::string> &keywords,
                     int tabWidth);

public:
    // Does all the work of transforming.
    void transform();

private:
    // Transforms single element into a node.
    PNode * visit(tinyxml2::XMLNode *node, int level);
    // Transforms text field.
    void visitLeaf(tinyxml2::XMLNode *parent, PNode *pnode,
                   tinyxml2::XMLNode *leaf);
    // Determines type of a child of the specified element.
    Type determineType(tinyxml2::XMLElement *elem, boost::string_ref value);

private:
    const std::string &contents;                       // Contents to parse.
    const std::string &path;                           // Path to the file.
    boost::string_ref left;                            // Unparsed part.
    TreeBuilder &tb;                                   // Result builder.
    std::string language;                              // Language name.
    const std::unordered_map<std::string, SType> &map; // Tag -> SType map.
    const std::unordered_set<std::string> &keywords;   // List of keywords.
    int tabWidth;                                      // Tabulation width.
    int line;                                          // Current line.
    int col;                                           // Current column.
    int inCppDirective;                                // Level of cpp nesting.
};

#endif // ZOGRASCOPE_SRCML_SRCMLTRANSFORMER_HPP_
