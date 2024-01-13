// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "Traverser.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "tooling/Config.hpp"
#include "utils/iterators.hpp"
#include "Language.hpp"

namespace fs = boost::filesystem;

Traverser::Traverser(const std::vector<std::string> &paths,
                     const std::string &language,
                     const Config &config,
                     std::function<callbackPrototype> callback)
    : paths(paths),
      language(language),
      config(config),
      callback(std::move(callback))
{ }

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
    auto match = [this](const std::string &file, bool passedIn) {
        if (!passedIn && !config.shouldProcessFile(file)) {
            return false;
        }

        if (Language::matches(file, language) ||
            Language::equal(config.lookupAttrs(file).lang, language)) {
            return callback(file);
        }
        return false;
    };

    if (!fs::is_directory(path)) {
        return match(path.string(), /*passedIn=*/true);
    }

    bool found = false;
    for (fs::directory_entry &e : rangeFrom<fs::directory_iterator>(path)) {
        const fs::path &path = e.path();
        if (fs::is_directory(path)) {
            if (config.shouldVisitDirectory(path.string())) {
                found |= search(path);
            }
        } else {
            found |= match(path.string(), /*passedIn=*/false);
        }
    }
    return found;
}
