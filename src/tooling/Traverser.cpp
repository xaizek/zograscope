// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "Traverser.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/range/adaptors.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Language.hpp"

namespace fs = boost::filesystem;

Traverser::Traverser(const std::vector<std::string> &paths,
                     const std::string &language,
                     std::function<callbackPrototype> callback)
    : paths(paths), language(language), callback(std::move(callback))
{
}

bool
Traverser::search()
{
    bool found = false;
    for (fs::path path : paths) {
        found |= search(path);
    }
    return found;
}

bool
Traverser::search(const boost::filesystem::path &path)
{
    auto match = [this](const std::string &file) {
        if (Language::matches(file, language)) {
            return callback(file);
        }
        return false;
    };

    if (!fs::is_directory(path)) {
        return match(path.string());
    }

    using it = fs::directory_iterator;

    bool found = false;
    for (fs::directory_entry &e : boost::make_iterator_range(it(path), it())) {
        if (fs::is_directory(e.path())) {
            found |= search(e.path());
        } else {
            found |= match(e.path().string());
        }
    }
    return found;
}
