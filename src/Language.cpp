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

#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/path.hpp>

#include "c/C11Language.hpp"
#include "make/MakeLanguage.hpp"

std::unique_ptr<Language>
Language::create(const std::string &fileName, const std::string &l)
{
    std::string lang = l;
    if (lang.empty()) {
        std::string name = boost::filesystem::path(fileName).stem().string();
        boost::algorithm::to_lower(name);
        lang = (name == "makefile" ? "make" : "c");
    }

    if (lang == "c") {
        return std::unique_ptr<C11Language>(new C11Language());
    }
    if (lang == "make") {
        return std::unique_ptr<MakeLanguage>(new MakeLanguage());
    }
    throw std::runtime_error("Unknown language: \"" + lang + '"');
}
