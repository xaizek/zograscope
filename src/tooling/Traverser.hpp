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

#ifndef ZOGRASCOPE__TOOLING__TRAVERSER_HPP__
#define ZOGRASCOPE__TOOLING__TRAVERSER_HPP__

#include <boost/filesystem/path.hpp>

#include <functional>
#include <string>
#include <vector>

class Config;

// Discovers files matching specified arguments and invokes callback on them.
class Traverser
{
    // Callback type.  Should return `true` to indicate positive visitation.
    using callbackPrototype = bool(const std::string &path);

public:
    // Records arguments for future use.
    Traverser(const std::vector<std::string> &paths,
              const std::string &language,
              const Config &config,
              std::function<callbackPrototype> callback);

public:
    // Processes all specified paths.  Returns `true` if something was found.
    bool search();

private:
    // Processes either single file or recursively discovered files in a
    // directory.
    bool search(const boost::filesystem::path &path);

private:
    std::vector<std::string> paths;            // List of paths to process.
    std::string language;                      // Language to accept.
    const Config &config;                      // Configuration.
    std::function<callbackPrototype> callback; // Invoked per file.
};

#endif // ZOGRASCOPE__TOOLING__TRAVERSER_HPP__
