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
#include "tree.hpp"

static std::string detectLanguage(const std::string &stem,
                                  const std::string &ext);

std::unique_ptr<Language>
Language::create(const std::string &fileName, const std::string &l)
{
    std::string lang = l;
    if (lang.empty()) {
        boost::filesystem::path path = fileName;

        using boost::algorithm::to_lower_copy;
        lang = detectLanguage(to_lower_copy(path.stem().string()),
                              to_lower_copy(path.extension().string()));
    }

    if (lang == "c") {
        return std::unique_ptr<C11Language>(new C11Language());
    }
    if (lang == "cxx" || lang == "srcml:cxx") {
        return std::unique_ptr<SrcmlCxxLanguage>(new SrcmlCxxLanguage());
    }
    if (lang == "make") {
        return std::unique_ptr<MakeLanguage>(new MakeLanguage());
    }
    throw std::runtime_error("Unknown language: \"" + lang + '"');
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

    using boost::algorithm::ends_with;
    if (ends_with(stem, "makefile") || ext == ".mk" || ext == ".mak") {
        return "make";
    }

    // Assume C by default.
    return "c";
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
