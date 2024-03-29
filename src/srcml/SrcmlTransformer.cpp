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
#include <boost/utility/string_ref.hpp>
#include "tinyxml2/tinyxml2.h"

#ifdef HAVE_LIBSRCML
#include <srcml.h>
#endif

#include "utils/fs.hpp"
#include "utils/strings.hpp"
#include "TreeBuilder.hpp"
#include "integration.hpp"
#include "types.hpp"

namespace ti = tinyxml2;

static void toSrcmlForm(const std::string &contents,
                        const std::string &path,
                        const std::string &language,
                        ti::XMLDocument &doc);
static bool toLibSrcmlForm(const std::string &contents,
                           const std::string &language,
                           ti::XMLDocument &doc);
static boost::string_ref processValue(boost::string_ref str);

SrcmlTransformer::SrcmlTransformer(const std::string &contents,
                                   const std::string &path,
                                   TreeBuilder &tb,
                                   const std::string &language,
                              const std::unordered_map<std::string, SType> &map,
                                const std::unordered_set<std::string> &keywords,
                                   int tabWidth)
    : contents(contents), path(path), tb(tb), language(language),
      map(map), keywords(keywords), tabWidth(tabWidth)
{ }

void
SrcmlTransformer::transform()
{
    ti::XMLDocument doc;
    toSrcmlForm(contents, path, language, doc);
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

// Parses source file via SrcML and gets result in a form of XML DOM.
static void toSrcmlForm(const std::string &contents,
                        const std::string &path,
                        const std::string &language,
                        ti::XMLDocument &doc)
{
    if (toLibSrcmlForm(contents, language, doc)) {
        return;
    }
#ifdef HAVE_LIBSRCML
    throw std::runtime_error("Failed to parse " + path);
#endif

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

    if (doc.Parse(xml.data(), xml.size()) == ti::XML_SUCCESS &&
        doc.RootElement() == nullptr) {

        // Work around srcml's issues with parsing files.  Sometimes it can't
        // read them from file (when extension is ".z" it thinks it's an
        // archive) and sometimes from stdin (bugs of previous versions), so try
        // both ways.
        cmd.pop_back();
        xml = readCommandOutput(cmd, contents);
        (void)doc.Parse(xml.data(), xml.size());
    }
}

template <typename T, typename D>
std::unique_ptr<T, D>
asUniquePtr(T *p, D &&d)
{
    return std::unique_ptr<T, D>(p, std::forward<D>(d));
}

static bool
toLibSrcmlForm(const std::string &contents,
               const std::string &language,
               ti::XMLDocument &doc)
{
#ifdef HAVE_LIBSRCML
    auto archive = asUniquePtr(srcml_archive_create(), &srcml_archive_free);
    if (archive == nullptr) {
        return false;
    }

    int status;

    status = srcml_archive_set_src_encoding(archive.get(), "utf8");
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    char *buffer;
    size_t size;

    status = srcml_archive_write_open_memory(archive.get(), &buffer, &size);
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    auto unit = asUniquePtr(srcml_unit_create(archive.get()), &srcml_unit_free);
    if (unit == nullptr) {
        return false;
    }

    // single input file, so non-archive unit
    status = srcml_archive_enable_solitary_unit(archive.get());
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    status = srcml_unit_set_language(unit.get(), language.c_str());
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    status = srcml_unit_parse_memory(unit.get(),
                                     contents.data(),
                                     contents.size());
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    status = srcml_archive_write_unit(archive.get(), unit.get());
    if (status != SRCML_STATUS_OK) {
        return false;
    }

    srcml_archive_close(archive.get());
    auto bufGuard = asUniquePtr(buffer, &std::free);

    (void)doc.Parse(buffer, size);
    return true;
#else
    (void)contents, (void)language, (void)doc;
    return false;
#endif
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
        updatePosition(left.substr(0U, skipped), tabWidth, line, col);
        left.remove_prefix(skipped);

        const Type type = determineType(parent->ToElement(), val);

        const auto offset = static_cast<std::uint32_t>(&left[0] - &contents[0]);
        const auto len = static_cast<std::uint32_t>(val.size());
        tb.append(pnode,
                  tb.addNode(Text{offset, len, 0, 0, static_cast<int>(type)},
                             Location{line, col, 0, 0}, stype));

        updatePosition(left.substr(0U, len), tabWidth, line, col);
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
