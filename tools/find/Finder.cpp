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

#include "Finder.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/optional.hpp>

#include "pmr/monolithic.hpp"

#include <iostream>
#include <string>

#include "utils/optional.hpp"
#include "Args.hpp"
#include "Highlighter.hpp"
#include "Matcher.hpp"
#include "common.hpp"
#include "decoration.hpp"
#include "mtypes.hpp"

namespace fs = boost::filesystem;

namespace {

// Strong typing of string with value for output stream overload.
struct AutoNL { const std::string &data; };

// Formatted value printer.
inline std::ostream &
operator<<(std::ostream &os, const AutoNL &val)
{
    return os << (val.data.find('\n') != std::string::npos ? "\n" : "")
              << val.data;
}

}

Finder::Finder(const Args &args, TimeReport &tr) : args(args), tr(tr)
{
    auto convert = [](const std::string &str) {
        if (str == "decl") {
            return MType::Declaration;
        } else if (str == "func") {
            return MType::Function;
        } else if (str == "comm") {
            return MType::Comment;
        } else if (str == "dir") {
            return MType::Directive;
        } else {
            throw std::runtime_error("Unknown type: " + str);
        }
    };

    Matcher *last = nullptr;
    for (std::size_t i = args.pos.size() - 1U; i > 0U; --i) {
        matchers.push_front(Matcher(convert(args.pos[i]), last));
        last = &matchers.front();
    }
}

Finder::~Finder() = default;

bool
Finder::find(const fs::path &path)
{
    bool found = false;
    auto match = [&](const std::string &file) {
        if (Language::matches(file, args.lang)) {
            found |= process(file);
        }
    };

    if (!fs::is_directory(path)) {
        match(path.string());
    } else {
        using it = fs::directory_iterator;
        for (fs::directory_entry &e :
             boost::make_iterator_range(it(path), it())) {
            if (fs::is_directory(e.path())) {
                found |= find(e.path());
            } else {
                match(e.path().string());
            }
        }
    }

    report();

    return found;
}

bool
Finder::process(const std::string &path)
{
    cpp17::pmr::monolithic mr;
    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        auto timer = tr.measure("looking: " + path);

        Tree tree = *t;
        Language &lang = *tree.getLanguage();
        const Args &args = this->args;

        auto handler = [&](const Node *node) {
            if (args.count) {
                return;
            }

            std::cout << (decor::yellow_fg << path) << ':'
                      << (decor::cyan_fg << node->line) << ':'
                      << (decor::cyan_fg << node->col) << ": "
                      << AutoNL { Highlighter(*node, lang, true,
                                              node->line).print() }
                      << '\n';
        };

        return matchers.front().match(tree.getRoot(), lang, handler);
    }
    return false;
}

void
Finder::report()
{
    if (!args.count) {
        return;
    }

    int lastSeen = 1;
    int lastMatched = 1;
    bool first = true;

    auto div = [](int a, int b) {
        if (a == 0 || b == 0) {
            return 0.0f;
        }
        return static_cast<float>(a)/b;
    };

    for (Matcher &matcher : matchers) {
        std::cout << "--> " << matcher.getMType() << '\n';
        std::cout << "seen        = " << matcher.getSeen() << '\n';
        std::cout << "matched     = " << matcher.getMatched() << '\n';

        if (!first) {
            const int matched = matcher.getMatched();
            std::cout << "per seen    = " << div(matched, lastSeen) << '\n';
            std::cout << "per matched = " << div(matched, lastMatched) << '\n';
            // TODO: consider also printing min and max
            // TODO: add average absolute deviation and dispersion too?
        }

        lastSeen = matcher.getSeen();
        lastMatched = matcher.getMatched();
        first = false;
    }
}
