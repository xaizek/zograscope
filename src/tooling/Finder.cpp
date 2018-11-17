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
#include <boost/range/adaptor/reversed.hpp>
#include <boost/optional.hpp>

#include "pmr/monolithic.hpp"

#include <iostream>
#include <string>

#include "utils/optional.hpp"
#include "ColorScheme.hpp"
#include "Grepper.hpp"
#include "Matcher.hpp"
#include "TermHighlighter.hpp"
#include "Traverser.hpp"
#include "common.hpp"
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

Finder::Finder(const CommonArgs &args, TimeReport &tr, bool countOnly)
    : args(args), tr(tr), countOnly(countOnly)
{
    auto convert = [](const std::string &str) {
        if (str == "decl") {
            return MType::Declaration;
        } else if (str == "stmt") {
            return MType::Statement;
        } else if (str == "func") {
            return MType::Function;
        } else if (str == "param") {
            return MType::Parameter;
        } else if (str == "comm") {
            return MType::Comment;
        } else if (str == "dir") {
            return MType::Directive;
        } else {
            throw std::runtime_error("Unknown type: " + str);
        }
    };

    std::vector<std::string> mpatterns, gpatterns;
    enum { PATHS, MPATTERNS, GPATTERNS } stage = PATHS;
    for (const std::string &arg : args.pos) {
        switch (stage) {
            case PATHS:
                if (arg == ":") {
                    stage = MPATTERNS;
                } else if (arg == "::") {
                    stage = GPATTERNS;
                } else {
                    paths.push_back(arg);
                }
                break;
            case MPATTERNS:
                if (arg == ":") {
                    stage = GPATTERNS;
                } else {
                    mpatterns.push_back(arg);
                }
                break;
            case GPATTERNS:
                gpatterns.push_back(arg);
                break;
        }
    }

    if (paths.empty()) {
        paths.push_back(".");
    }
    if (mpatterns.empty() && gpatterns.empty()) {
        throw std::runtime_error("Expected at least one matcher of either "
                                 "type");
    }

    Matcher *last = nullptr;
    for (const std::string &mpattern : boost::adaptors::reverse(mpatterns)) {
        matchers.push_front(Matcher(convert(mpattern), last));
        last = &matchers.front();
    }

    grepper = Grepper(gpatterns);
}

Finder::~Finder() = default;

bool
Finder::search()
{
    bool found = Traverser(paths, args.lang, [this](const std::string &path) {
                               return process(path);
                           }).search();
    report();
    return found;
}

bool
Finder::process(const std::string &path)
{
    static ColorScheme cs;

    cpp17::pmr::monolithic mr;
    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        auto timer = tr.measure("looking: " + path);

        Tree tree = *t;
        Language &lang = *tree.getLanguage();
        const bool countOnly = this->countOnly;
        const bool noMatchers = matchers.empty();

        auto grepHandler = [&](const std::vector<Node *> &match) {
            if (!noMatchers || countOnly) {
                return;
            }

            Node fakeRoot;
            fakeRoot.children.assign(match.cbegin(), match.cend());

            const Node *node = match.front();
            std::cout << (cs[ColorGroup::Path] << path) << ':'
                      << (cs[ColorGroup::LineNoPart] << node->line) << ':'
                      << (cs[ColorGroup::ColNoPart] << node->col) << ": "
                      << AutoNL { TermHighlighter(fakeRoot, lang, true,
                                                  node->line).print() }
                      << '\n';
        };

        if (noMatchers) {
            return grepper.grep(tree.getRoot(), grepHandler);
        }

        auto matchHandler = [&](Node *node) {
            if (!grepper.grep(node, grepHandler) || countOnly) {
                return;
            }

            std::cout << (cs[ColorGroup::Path] << path) << ':'
                      << (cs[ColorGroup::LineNoPart] << node->line) << ':'
                      << (cs[ColorGroup::ColNoPart] << node->col) << ": "
                      << AutoNL { TermHighlighter(*node, lang, true,
                                                  node->line).print() }
                      << '\n';
        };

        return matchers.front().match(tree.getRoot(), lang, matchHandler);
    }
    return false;
}

void
Finder::report()
{
    if (!countOnly) {
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

    if (!grepper.empty()) {
        std::cout << "++> Token match\n";
        std::cout << "seen    = " << grepper.getSeen() << '\n';
        std::cout << "matched = " << grepper.getMatched() << '\n';
    }
}
