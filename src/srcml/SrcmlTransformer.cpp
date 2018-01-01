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

#include "SrcmlTransformer.hpp"

#include <stdexcept>
#include <string>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/utility/string_ref.hpp>
#include "tinyxml/tinyxml.h"

#include "TreeBuilder.hpp"
#include "integration.hpp"
#include "types.hpp"

// XXX: hard-coded width of a tabulation character.
const int tabWidth = 4;

static boost::string_ref processValue(boost::string_ref str);
static void updatePosition(boost::string_ref str, int &line, int &col);

SrcmlTransformer::SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                                   const std::string &language,
                              const std::unordered_map<std::string, SType> &map,
                                const std::unordered_set<std::string> &keywords)
    : contents(contents), tb(tb), language(language),
      map(map), keywords(keywords)
{
}

void
SrcmlTransformer::transform()
{
    std::vector<std::string> cmd = {
        "srcml", "--language=" + language, "-"
    };

    std::string xml = readCommandOutput(cmd, contents);

    TiXmlDocument doc {};
    doc.SetCondenseWhiteSpace(false);
    doc.Parse(xml.c_str());
    if (doc.Error()) {
        throw std::runtime_error("Failed to parse");
    }

    left = contents;
    line = 1;
    col = 1;
    inCppDirective = 0;

    tb.setRoot(visit(doc.RootElement(), 0));
}

PNode *
SrcmlTransformer::visit(TiXmlNode *node, int level)
{
    SType stype = {};
    auto it = map.find(node->Value());
    if (it != map.end()) {
        stype = it->second;
    }

    bool cppDirective = boost::starts_with(node->Value(), "cpp:");
    inCppDirective += cppDirective;

    PNode *pnode = tb.addNode({}, stype);

    for (TiXmlNode *child = node->FirstChild();
         child != nullptr;
         child = child->NextSibling()) {
        switch (child->Type()) {
            case TiXmlNode::TINYXML_ELEMENT:
                tb.append(pnode, visit(child->ToElement(), level + 1));
                break;
            case TiXmlNode::TINYXML_TEXT:
                visitLeaf(node, pnode, child);
                break;
        }
    }

    inCppDirective -= cppDirective;

    return pnode;
}

void
SrcmlTransformer::visitLeaf(TiXmlNode *parent, PNode *pnode, TiXmlNode *leaf)
{
    boost::string_ref fullVal = processValue(leaf->ValueStr());

    SType stype = {};
    if (leaf != parent->FirstChild() || leaf->NextSibling() != nullptr) {
        stype = map.at("separator");
    }

    std::vector<boost::string_ref> vals = { fullVal };
    if (fullVal.size() > 1U &&
        (fullVal.back() == ':' || fullVal.back() == ';')) {
        vals = {
            processValue(fullVal.substr(0U, fullVal.size() - 1U)),
            processValue(fullVal.substr(fullVal.size() - 1U))
        };
    }

    for (boost::string_ref val : vals) {
        const std::size_t skipped = left.find(val);
        updatePosition(left.substr(0U, skipped), line, col);
        left.remove_prefix(skipped);

        const Type type = determineType(parent->ToElement(), val);

        const auto offset = static_cast<std::uint32_t>(&left[0] - &contents[0]);
        const auto len = static_cast<std::uint32_t>(val.size());
        tb.append(pnode,
                  tb.addNode(Text{offset, len, 0, 0, static_cast<int>(type)},
                             Location{line, col, 0, 0}, stype));

        updatePosition(left.substr(0U, len), line, col);
        left.remove_prefix(len);
    }
}

// Trims the value.
static boost::string_ref
processValue(boost::string_ref str)
{
    auto first = str.find_first_not_of(" \t\n");
    if (first != boost::string_ref::npos) {
        str.remove_prefix(first);
    }

    auto last = str.find_last_not_of(" \t\n");
    if (last != boost::string_ref::npos) {
        str.remove_suffix(str.size() - (last + 1));
    }

    return str;
}

// Goes over characters in the string and updates line and column accordingly.
static void
updatePosition(boost::string_ref str, int &line, int &col)
{
    while (!str.empty()) {
        switch (str.front()) {
            case '\n':
                ++line;
                col = 1;
                break;
            case '\t':
                col += tabWidth - (col - 1)%tabWidth;
                break;

            default:
                ++col;
                break;
        }
        str.remove_prefix(1);
    }
}

Type
SrcmlTransformer::determineType(TiXmlElement *elem, boost::string_ref value)
{
    if (elem->ValueStr() == "literal") {
        const std::string type = elem->Attribute("type");
        if (type == "boolean") {
            return Type::IntConstants;
        } else if (type == "char") {
            return Type::CharConstants;
        } else if (type == "null") {
            return Type::IntConstants;
        } else if (type == "number") {
            return Type::IntConstants;
        } else if (type == "string") {
            return Type::StrConstants;
        } else if (type == "complex") {
            return Type::FPConstants;
        }
    } else if (elem->ValueStr() == "specifier") {
        return Type::Specifiers;
    } else if (elem->ValueStr() == "comment") {
        return Type::Comments;
    } else if (inCppDirective) {
        return Type::Directives;
    } else if (value[0] == '(' || value[0] == '{' || value[0] == '[') {
        return Type::LeftBrackets;
    } else if (value[0] == ')' || value[0] == '}' || value[0] == ']') {
        return Type::RightBrackets;
    } else if (elem->ValueStr() == "operator") {
        return Type::Operators;
    } else if (elem->ValueStr() == "name") {
        const TiXmlNode *parent = elem;
        std::string parentValue;
        std::string grandParentValue;
        do {
            parent = parent->Parent();
            parentValue = (parent == nullptr ? "" : parent->ValueStr());
            grandParentValue = (parent->Parent() == nullptr)
                             ? ""
                             : parent->Parent()->ValueStr();
        } while (parentValue == "name" &&
                 (elem != parent->FirstChild() ||
                  (grandParentValue != "function" &&
                   grandParentValue != "call")));

        if (parentValue == "type") {
            return keywords.find(value.to_string()) != keywords.cend()
                 ? Type::Keywords
                 : Type::UserTypes;
        } else if (parentValue == "function" || parentValue == "call") {
            return Type::Functions;
        }
    } else if (keywords.find(value.to_string()) != keywords.cend()) {
        return Type::Keywords;
    }
    return Type::Other;
}
