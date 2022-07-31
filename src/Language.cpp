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

#include "Language.hpp"

#include <cassert>
#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>

#include "c/C11Language.hpp"
#include "make/MakeLanguage.hpp"
#include "srcml/cxx/SrcmlCxxLanguage.hpp"
#include "ts/bash/TSBashLanguage.hpp"
#include "ts/lua/TSLuaLanguage.hpp"
#include "tree.hpp"

namespace fs = boost::filesystem;

using boost::algorithm::to_lower_copy;

static std::string simplifyLanguage(const std::string &lang);
static std::string detectLanguage(const std::string &stem,
                                  const std::string &ext);

std::unique_ptr<Language>
Language::create(const std::string &fileName, const std::string &l)
{
    std::string lang = l;
    if (lang.empty()) {
        fs::path path = fileName;

        lang = detectLanguage(to_lower_copy(path.stem().string()),
                              to_lower_copy(path.extension().string()));

        if (lang.empty()) {
            // Assume C by default.
            lang = "c";
        }
    }

    lang = simplifyLanguage(lang);

    if (lang == "c") {
        return std::unique_ptr<C11Language>(new C11Language());
    }
    if (lang == "cxx") {
        return std::unique_ptr<SrcmlCxxLanguage>(new SrcmlCxxLanguage());
    }
    if (lang == "bash") {
        return std::unique_ptr<TsBashLanguage>(new TsBashLanguage());
    }
    if (lang == "lua") {
        return std::unique_ptr<TsLuaLanguage>(new TsLuaLanguage());
    }
    if (lang == "make") {
        return std::unique_ptr<MakeLanguage>(new MakeLanguage());
    }
    throw std::runtime_error("Unknown language: \"" + lang + '"');
}

bool
Language::matches(const std::string &fileName, const std::string &lang)
{
    fs::path path = fileName;

    std::string ext = to_lower_copy(path.extension().string());
    std::string detected = detectLanguage(to_lower_copy(path.stem().string()),
                                          ext);

    if (lang.empty() ? !detected.empty() : detected == lang) {
        return true;
    }
    if ((lang == "cxx" || lang == "srcml:cxx") && ext == ".h") {
        return true;
    }

    return false;
}

bool
Language::equal(const std::string &langA, const std::string &langB)
{
    return !langA.empty()
        && !langB.empty()
        && simplifyLanguage(langA) == simplifyLanguage(langB);
}

// Removes parser prefixes from language ids or does nothing.
static std::string
simplifyLanguage(const std::string &lang)
{
    if (lang == "srcml:cxx") {
        return "cxx";
    }
    if (lang == "ts:bash") {
        return "bash";
    }
    if (lang == "ts:lua") {
        return "lua";
    }
    return lang;
}

// Determines language from normalized stem and extension of a file.
static std::string
detectLanguage(const std::string &stem, const std::string &ext)
{
    if (ext == ".c" || ext == ".h") {
        return "c";
    }

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" ||
        ext == ".hpp" || ext == ".hxx" || ext == ".hh") {
        return "cxx";
    }

    if (ext == ".lua") {
        return "lua";
    }

    if (ext == ".sh" || ext == ".bash") {
        return "bash";
    }

    using boost::algorithm::ends_with;
    if (ends_with(stem, "makefile") || ext == ".mk" || ext == ".mak") {
        return "make";
    }

    return {};
}

bool
Language::isDiffable(const Node *x) const
{
    return x->type == Type::Comments
        || x->type == Type::StrConstants
        || x->type == Type::Functions
        || x->type == Type::Identifiers
        || x->type == Type::UserTypes;
}

bool
Language::isStructural(const Node *x) const
{
    return x->type == Type::LeftBrackets
        || x->type == Type::RightBrackets;
}

bool
Language::isPayloadOfFixed(const Node *x) const
{
    return !isSatellite(x->stype)
        && !isTravellingNode(x);
}

bool
Language::hasMoveableItems(const Node *x) const
{
    return (!isUnmovable(x) || isContainer(x));
}
