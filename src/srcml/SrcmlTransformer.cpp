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

#include "SrcmlTransformer.hpp"

#include <cstdlib>

#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/utility/string_ref.hpp>
#include "tinyxml2/tinyxml2.h"

#include "TreeBuilder.hpp"
#include "integration.hpp"
#include "types.hpp"

namespace ti = tinyxml2;

// XXX: hard-coded width of a tabulation character.
const int tabWidth = 4;

namespace {

// Temporary file in RAII-style.
class TempFile
{
public:
    // Makes temporary file whose name is a mangled version of an input name.
    // The file is removed in destructor.
    explicit TempFile(boost::filesystem::path baseName)
    {
        namespace fs = boost::filesystem;

        path = (
            fs::temp_directory_path()
         /  fs::unique_path(baseName.stem().string() + '-' +
                            "-%%%%-%%%%" + baseName.extension().string())
        ).string();
    }

    // Make sure temporary file is deleted only once.
    TempFile(const TempFile &rhs) = delete;
    TempFile & operator=(const TempFile &rhs) = delete;

    // Removes temporary file, if it still exists.
    ~TempFile()
    {
        static_cast<void>(std::remove(path.c_str()));
    }

public:
    // Provides implicit conversion to a file path string.
    operator std::string() const
    {
        return path;
    }

    // Explicit conversion to a file path string.
    const std::string & str() const
    {
        return path;
    }

private:
    std::string path; // Path to the temporary file.
};

}

static boost::string_ref processValue(boost::string_ref str);
static void updatePosition(boost::string_ref str, int &line, int &col);

SrcmlTransformer::SrcmlTransformer(const std::string &contents,
                                   const std::string &path, TreeBuilder &tb,
                                   const std::string &language,
                              const std::unordered_map<std::string, SType> &map,
                                const std::unordered_set<std::string> &keywords)
    : contents(contents), path(path), tb(tb), language(language),
      map(map), keywords(keywords)
{ }

void
SrcmlTransformer::transform()
{
    TempFile tmpFile(path);

    std::ofstream ofs(tmpFile);
    if (!ofs) {
        throw std::runtime_error("Failed to open temporary file: " +
                                 tmpFile.str());
    }
    ofs << contents;
    ofs.close();

    std::vector<std::string> cmd = {
        "srcml", "--language=" + language, "--src-encoding=utf8", tmpFile
    };

    std::string xml = readCommandOutput(cmd, std::string());

    ti::XMLDocument doc;
    if (doc.Parse(xml.data(), xml.size()) == ti::XML_SUCCESS &&
        doc.RootElement() == nullptr) {

        // Work around srcml's issues with parsing files.  Sometimes it can't
        // read them from file (when extension is ".z" it thinks it's an
        // archive) and sometimes from stdin (bugs of previous versions), so try
        // both ways.
        cmd.pop_back();
        xml = readCommandOutput(cmd, contents);
        doc.Parse(xml.data(), xml.size());
    }

    if (doc.Error()) {
        throw std::runtime_error("Failed to parse: " +
                                 std::string(doc.ErrorStr()));
    }

    left = contents;
    line = 1;
    col = 1;
    inCppDirective = 0;

    tb.setRoot(visit(doc.RootElement(), 0));
}

PNode *
SrcmlTransformer::visit(ti::XMLNode *node, int level)
{
    if (node == nullptr) {
        return tb.addNode();
    }

    SType stype = {};
    auto it = map.find(node->Value());
    if (it != map.end()) {
        stype = it->second;
    }

    bool cppDirective = boost::starts_with(node->Value(), "cpp:");
    inCppDirective += cppDirective;

    PNode *pnode = tb.addNode({}, stype);

    for (ti::XMLNode *child = node->FirstChild();
         child != nullptr;
         child = child->NextSibling()) {
        if (ti::XMLElement *e = child->ToElement()) {
            tb.append(pnode, visit(e, level + 1));
        } else if (child->ToText() != nullptr) {
            visitLeaf(node, pnode, child);
        }
    }

    inCppDirective -= cppDirective;

    return pnode;
}

void
SrcmlTransformer::visitLeaf(ti::XMLNode *parent, PNode *pnode,
                            ti::XMLNode *leaf)
{
    boost::string_ref fullVal = processValue(leaf->Value());

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
SrcmlTransformer::determineType(ti::XMLElement *elem, boost::string_ref value)
{
    boost::string_ref elemValue = elem->Value();
    if (elemValue == "literal") {
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
    } else if (elemValue == "specifier") {
        return Type::Specifiers;
    } else if (elemValue == "comment") {
        return Type::Comments;
    } else if (inCppDirective) {
        return Type::Directives;
    } else if (value[0] == '(' || value[0] == '{' || value[0] == '[') {
        return Type::LeftBrackets;
    } else if (value[0] == ')' || value[0] == '}' || value[0] == ']') {
        return Type::RightBrackets;
    } else if (elemValue == "operator") {
        return Type::Operators;
    } else if (elemValue == "name") {
        const ti::XMLNode *parent = elem;
        std::string parentValue;
        std::string grandParentValue;
        do {
            parent = parent->Parent();
            parentValue = (parent == nullptr ? "" : parent->Value());
            grandParentValue = (parent->Parent() == nullptr)
                             ? ""
                             : parent->Parent()->Value();
        } while (parentValue == "name" &&
                 (elem != parent->FirstChild() ||
                  (grandParentValue != "function" &&
                   grandParentValue != "call")));

        if (keywords.find(value.to_string()) != keywords.cend()) {
            return Type::Keywords;
        }
        if (parentValue == "type") {
            return Type::UserTypes;
        }
        if (parentValue == "function" || parentValue == "call") {
            return Type::Functions;
        }
        return Type::Identifiers;
    } else if (keywords.find(value.to_string()) != keywords.cend()) {
        return Type::Keywords;
    }
    return Type::Other;
}
