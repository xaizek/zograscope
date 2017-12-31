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

#include <boost/utility/string_ref.hpp>
#include "tinyxml/tinyxml.h"

#include "TreeBuilder.hpp"
#include "integration.hpp"
#include "types.hpp"

// XXX: hard-coded width of a tabulation character.
const int tabWidth = 4;

static boost::string_ref processValue(boost::string_ref str);
static void updatePosition(boost::string_ref str, int &line, int &col);
static Type determineType(TiXmlElement *elem);

SrcmlTransformer::SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                                   const std::string &language,
                              const std::unordered_map<std::string, SType> &map)
    : contents(contents), tb(tb), language(language), map(map)
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

    PNode *pnode = tb.addNode({}, stype);

    for (TiXmlNode *child = node->FirstChild();
         child != nullptr;
         child = child->NextSibling()) {
        boost::string_ref val;
        std::size_t skipped;
        Type type;

        switch (child->Type()) {
            case TiXmlNode::TINYXML_ELEMENT:
                tb.append(pnode, visit(child->ToElement(), level + 1));
                break;
            case TiXmlNode::TINYXML_TEXT:
                stype = {};
                val = processValue(child->ValueStr());

                if (child != node->FirstChild() ||
                    child->NextSibling() != nullptr) {
                    stype = map.at("separator");
                }

                skipped = left.find(val);
                updatePosition(left.substr(0U, skipped), line, col);
                left.remove_prefix(skipped);

                type = determineType(node->ToElement());

                auto offset =
                    static_cast<std::uint32_t>(&left[0] - &contents[0]);
                const auto len = static_cast<std::uint32_t>(val.size());
                tb.append(pnode,
                          tb.addNode(Text{offset, len, 0, 0,
                                          static_cast<int>(type)},
                                     Location{line, col, 0, 0}, stype));

                updatePosition(left.substr(0U, len), line, col);
                left.remove_prefix(len);
                break;
        }
    }

    return pnode;
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

// Determines type of a child of the specified element.
static Type
determineType(TiXmlElement *elem)
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
    }
    return Type::Other;
}
