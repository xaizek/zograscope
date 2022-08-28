// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#include "TSTransformer.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/utility/string_ref.hpp>
#include "tree_sitter/api.h"

#include "utils/strings.hpp"
#include "TreeBuilder.hpp"
#include "types.hpp"

static bool isSeparator(Type type);

TSTransformer::TSTransformer(const std::string &contents,
                             const TSLanguage &tsLanguage,
                             TreeBuilder &tb,
                           const std::unordered_map<std::string, SType> &stypes,
                             const std::unordered_map<std::string, Type> &types,
                             const std::unordered_set<std::string> &badNodes,
                             int tabWidth,
                             bool debug)
    : contents(contents), tsLanguage(tsLanguage), tb(tb), stypes(stypes),
      types(types), badNodes(badNodes), tabWidth(tabWidth), debug(debug)
{ }

void
TSTransformer::transform()
{
    std::unique_ptr<TSParser , void(*)(TSParser *)> parser(ts_parser_new(),
                                                           &ts_parser_delete);
    ts_parser_set_language(parser.get(), &tsLanguage);

    std::unique_ptr<TSTree, void(*)(TSTree *)> tree(
        ts_parser_parse_string(parser.get(), NULL,
                               contents.c_str(), contents.size()),
        &ts_tree_delete
    );
    if (tree == nullptr) {
        throw std::runtime_error("Failed to build a tree");
    }

    position = 0;
    line = 1;
    col = 1;

    tb.setRoot(visit(ts_tree_root_node(tree.get()), Type::Other));

    if (debug) {
        for (const std::string &type : badSTypes) {
            std::cout << "(TSTransformer) No SType for: " << type << '\n';
        }
        for (const std::string &type : badTypes) {
            std::cout << "(TSTransformer) No Type for: " << type << '\n';
        }
    }
}

PNode *
TSTransformer::visit(const TSNode &node, Type defType)
{
    SType stype = {};
    const char *type = ts_node_type(node);
    auto it = stypes.find(type);
    if (it != stypes.end()) {
        stype = it->second;
    } else if (debug) {
        uint32_t from = ts_node_start_byte(node);
        uint32_t to = ts_node_end_byte(node);
        boost::string_ref val(contents.c_str() + from, to - from);
        badSTypes.insert(type + (": `" + val.to_string() + '`'));
    }

    auto typeIt = types.find(type);
    if (typeIt != types.end()) {
        defType = typeIt->second;
    }

    PNode *pnode = tb.addNode({}, stype);

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        const TSNode child = ts_node_child(node, i);
        if (ts_node_child_count(child) == 0) {
            SType stype = {};
            auto it = stypes.find(ts_node_type(child));
            if (it != stypes.end()) {
                stype = it->second;
            }

            visitLeaf(stype, pnode, child, defType);
        } else {
            tb.append(pnode, visit(child, defType));
        }
    }

    return pnode;
}

void
TSTransformer::visitLeaf(SType stype,
                         PNode *pnode,
                         const TSNode &leaf,
                         Type defType)
{
    if (badNodes.find(ts_node_type(leaf)) != badNodes.end()) {
        return;
    }

    uint32_t from = ts_node_start_byte(leaf);
    uint32_t to = ts_node_end_byte(leaf);

    boost::string_ref skipped(contents.c_str() + position, from - position);
    updatePosition(skipped, tabWidth, line, col);

    boost::string_ref val(contents.c_str() + from, to - from);
    Type type = determineType(leaf);
    if (type == Type::Other) {
        type = defType;
    }

    if (stype == SType{} && isSeparator(type)) {
        stype = stypes.at("separator");
    }

    const std::uint32_t len = to - from;
    tb.append(pnode, tb.addNode(Text{from, len, 0, 0, static_cast<int>(type)},
                                Location{line, col, 0, 0}, stype));

    updatePosition(val, tabWidth, line, col);
    position = to;
}

Type
TSTransformer::determineType(const TSNode &node)
{
    const char *type = ts_node_type(node);
    auto it = types.find(type);
    if (it != types.cend()) {
        return it->second;
    }

    if (debug) {
        uint32_t from = ts_node_start_byte(node);
        uint32_t to = ts_node_end_byte(node);
        boost::string_ref val(contents.c_str() + from, to - from);
        badTypes.insert(type + (": `" + val.to_string() + '`'));
    }

    return Type::Other;
}

// Determines whether type is a separator.
static bool
isSeparator(Type type)
{
    switch (type) {
        case Type::Jumps:
        case Type::Types:
        case Type::LeftBrackets:
        case Type::RightBrackets:
        case Type::Comparisons:
        case Type::Operators:
        case Type::LogicalOperators:
        case Type::Assignments:
        case Type::Keywords:
        case Type::Other:
            return true;

        case Type::Virtual:
        case Type::Functions:
        case Type::UserTypes:
        case Type::Identifiers:
        case Type::Specifiers:
        case Type::Directives:
        case Type::Comments:
        case Type::StrConstants:
        case Type::IntConstants:
        case Type::FPConstants:
        case Type::CharConstants:
        case Type::NonInterchangeable:
            return false;
    }

    assert(false && "Unhandled enumeration item");
    return false;
}
