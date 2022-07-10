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

#include "common.hpp"

#include <iostream>
#include <string>
#include <utility>

#include <boost/filesystem/operations.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/optional.hpp>
#include "pmr/monolithic.hpp"

#include "utils/fs.hpp"
#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "Language.hpp"
#include "TreeBuilder.hpp"
#include "STree.hpp"
#include "decoration.hpp"
#include "tree.hpp"

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args,
                                      po::options_description &options);
static optional_t<Tree> buildTreeFromFile(const CommonArgs &args,
                                          TimeReport &tr,
                                          const Attrs &attrs,
                                          const std::string &path,
                                          const std::string &contents,
                                          cpp17::pmr::memory_resource *mr);

Environment::Environment(const po::options_description &extraOpts)
    : options(extraOpts), args(), config(boost::filesystem::current_path())
{ }

void
Environment::setup(const std::vector<std::string> &argv)
{
    varMap = parseOptions(argv, options);
    args.pos = varMap["positional"].as<std::vector<std::string>>();
    args.help = varMap.count("help");
    args.debug = varMap.count("debug");
    args.sdebug = varMap.count("sdebug");
    args.dumpSTree = varMap.count("dump-stree");
    args.dumpTree = varMap.count("dump-tree");
    args.dryRun = varMap.count("dry-run");
    args.color = varMap.count("color");
    args.fine = varMap.count("fine-only");
    args.timeReport = varMap.count("time-report");
    args.lang = varMap["lang"].as<std::string>();

    if (args.color) {
        decor::enableDecorations();
    }
}

// Parses command line-options.
//
// Positional arguments are returned in "positional" entry, which exists even
// when there is no positional arguments.
static po::variables_map
parseOptions(const std::vector<std::string> &args,
             po::options_description &options)
{
    po::options_description hiddenOpts;
    hiddenOpts.add_options()
        ("positional", po::value<std::vector<std::string>>()
                       ->default_value({}, ""),
         "positional args");

    po::positional_options_description positionalOptions;
    positionalOptions.add("positional", -1);

    options.add_options()
        ("help,h",      "print help message")
        ("dry-run",     "just parse")
        ("debug",       "enable debugging of grammar")
        ("sdebug",      "enable debugging of strees")
        ("dump-stree",  "display stree(s)")
        ("dump-tree",   "display tree(s)")
        ("time-report", "report time spent on different activities")
        ("fine-only",   "use only fine-grained tree")
        ("color",       "force colorization of output")
        ("lang",        po::value<std::string>()->default_value({}),
                        "force specific language (c, cxx, make, lua)");

    po::options_description allOptions;
    allOptions.add(options).add(hiddenOpts);

    auto parsed_from_cmdline =
        po::command_line_parser(args)
        .options(allOptions)
        .positional(positionalOptions)
        .run();

    po::variables_map varMap;
    po::store(parsed_from_cmdline, varMap);
    return varMap;
}

void
Environment::teardown(bool error)
{
    if (error) {
        redirectToPager.discharge();
        return;
    }

    if (args.timeReport) {
        tr.stop();
        std::cout << tr;
    }
}

void
Environment::printOptions()
{
    std::cout << options;
}

optional_t<Tree> buildTreeFromFile(Environment &env,
                                   const std::string &path,
                                   cpp17::pmr::memory_resource *mr)
{
    Attrs attrs = env.getConfig().lookupAttrs(path);
    return buildTreeFromFile(env, env.getTimeKeeper(), attrs, path, mr);
}

optional_t<Tree>
buildTreeFromFile(Environment &env,
                  TimeReport &tr,
                  const Attrs &attrs,
                  const std::string &path,
                  cpp17::pmr::memory_resource *mr)
{
    return buildTreeFromFile(env.getCommonArgs(),
                             tr,
                             attrs,
                             path,
                             readFile(path),
                             mr);
}

optional_t<Tree> buildTreeFromFile(Environment &env,
                                   TimeReport &tr,
                                   const Attrs &attrs,
                                   const std::string &path,
                                   const std::string &contents,
                                   cpp17::pmr::memory_resource *mr)
{
    return buildTreeFromFile(env.getCommonArgs(),
                             tr,
                             attrs,
                             path,
                             contents,
                             mr);
}

optional_t<Tree>
buildTreeFromFile(Environment &env,
                  const std::string &path,
                  const std::string &contents,
                  cpp17::pmr::memory_resource *mr)
{
    Attrs attrs = env.getConfig().lookupAttrs(path);
    return buildTreeFromFile(env.getCommonArgs(),
                             env.getTimeKeeper(),
                             attrs,
                             path,
                             contents,
                             mr);
}

// Parses a file to build its tree.
static optional_t<Tree> buildTreeFromFile(const CommonArgs &args,
                                          TimeReport &tr,
                                          const Attrs &attrs,
                                          const std::string &path,
                                          const std::string &contents,
                                          cpp17::pmr::memory_resource *mr)
{
    auto timer = tr.measure("parsing: " + path);

    std::string langName = args.lang;
    if (langName.empty()) {
        langName = attrs.lang;
    }

    std::unique_ptr<Language> lang = Language::create(path, langName);

    cpp17::pmr::monolithic localMR;

    TreeBuilder tb =
        lang->parse(contents, path, attrs.tabWidth, args.debug, localMR);
    if (tb.hasFailed()) {
        return {};
    }

    Tree t(mr);

    if (args.fine) {
        t = Tree(std::move(lang), attrs.tabWidth, contents, tb.getRoot(), mr);
    } else {
        STree stree(std::move(tb), contents, args.dumpSTree, args.sdebug,
                    *lang, localMR);
        t = Tree(std::move(lang), attrs.tabWidth, contents, stree.getRoot(),
                 mr);
    }

    return optional_t<Tree>(std::move(t));
}

void
dumpTree(const CommonArgs &args, Tree &tree)
{
    if (args.dumpTree && !tree.isEmpty()) {
        std::cout << "Tree:\n";
        tree.dump();
    }
}

void
dumpTrees(const CommonArgs &args, Tree &treeA, Tree &treeB)
{
    if (!args.dumpTree) {
        return;
    }

    treeA.markInPreOrder();
    treeB.markInPreOrder();

    if (!treeA.isEmpty()) {
        std::cout << "Old tree:\n";
        treeA.dump();
    }

    if (!treeB.isEmpty()) {
        std::cout << "New tree:\n";
        treeB.dump();
    }
}
