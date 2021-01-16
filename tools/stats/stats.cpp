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

#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <iomanip>
#include <iostream>

#include "pmr/monolithic.hpp"
#include "tooling/FunctionAnalyzer.hpp"
#include "tooling/Traverser.hpp"
#include "tooling/common.hpp"
#include "utils/nums.hpp"
#include "utils/optional.hpp"
#include "utils/strings.hpp"
#include "ColorScheme.hpp"
#include "NodeRange.hpp"
#include "TermHighlighter.hpp"
#include "decoration.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

// TODO: ability to skip leading (copyright) and trailing (modeline) comments.

namespace {

enum class LineContent
{
    Blank,
    Code,
    Comment,
    Structural,
};

// Tool-specific arguments.
struct Args : CommonArgs
{
    bool annotate; // Print source code annotated with line types.
};

// Strong typing for output stream overload.
struct Header { const boost::string_ref data; };
struct SubHeader { const boost::string_ref data; };
struct Bullet { const boost::string_ref data; };
struct Part { const int part; const int whole; };
struct Count { const int data; };

// Formatted value printers.
inline std::ostream &
operator<<(std::ostream &os, const Header &val)
{
    return os << (decor::bold << val.data) << '\n'
              << (decor::bold << std::setw(val.data.length())
                              << std::setfill('-') << "") << '\n';
}
inline std::ostream &
operator<<(std::ostream &os, const SubHeader &val)
{
    return os << " * " << (decor::bold << val.data) << '\n';
}
inline std::ostream &
operator<<(std::ostream &os, const Bullet &val)
{
    return os << " * " << (decor::bold << val.data) << ": ";
}

inline std::ostream &
operator<<(std::ostream &os, const Part &val)
{
    float percent = (val.whole == 0 ? 1.0f : 1.0f*val.part/val.whole);
    return os << '\t' << std::setfill(' ') << std::setw(countWidth(val.whole))
              << val.part << " ("
              << std::fixed << std::setw(7) << std::setprecision(4)
              << percent*100.0f
              << "%)";
}

inline std::ostream &
operator<<(std::ostream &os, const Count &val)
{
    return os << '\t' << val.data;
}

class LineAnalyzer
{
public:
    explicit LineAnalyzer(Language &lang);

public:
    void countIn(const Node *node);

    const std::vector<LineContent> & getMap() const;

private:
    void updateMap(unsigned int line, const Node &node);

private:
    std::vector<LineContent> map;
    const Language &lang;
};

class StatsAggregator
{
public:
    void aggregate(int value);

    bool isEmpty() const;

    int getSampleSize() const;
    int getMin() const;
    int getMax() const;
    int getMedian() const;

private:
    int sampleSize = 0;
    int min = 0;
    int max = 0;
    mutable std::vector<int> data;
    mutable bool sorted;
};

class FileProcessor
{
public:
    FileProcessor(const Args &args, TimeReport &tr);

public:
    bool operator()(const std::string &path);
    void printReport() const;

private:
    const Args &args;
    TimeReport &tr;

    decor::Decoration lineNoHi;
    decor::Decoration pathHi;

    decor::Decoration blankHi;
    decor::Decoration commentHi;
    decor::Decoration codeHi;
    decor::Decoration structuralHi;

    int files = 0, blank = 0, code = 0, comment = 0, structural = 0;

    StatsAggregator funcSizes;
    StatsAggregator paramCounts;
};

}

inline
LineAnalyzer::LineAnalyzer(Language &lang) : lang(lang)
{ }

inline void
LineAnalyzer::countIn(const Node *node)
{
    unsigned int line = node->line - 1;
    std::vector<boost::string_ref> lines = split(node->label, '\n');
    updateMap(line, *node);
    for (std::size_t i = 1U; i < lines.size(); ++i) {
        updateMap(++line, *node);
    }

    if (lang.isEolContinuation(node)) {
        updateMap(line + 1, *node);
    }
}

inline const std::vector<LineContent> &
LineAnalyzer::getMap() const
{
    return map;
}

inline void
LineAnalyzer::updateMap(unsigned int line, const Node &node)
{
    if (map.size() <= line) {
        map.resize(line + 1);
    }

    LineContent type;
    if (node.type == Type::Comments) {
        type = LineContent::Comment;
    } else if (lang.isStructural(&node)) {
        type = LineContent::Structural;
    } else {
        type = LineContent::Code;
    }

    if (map[line] == LineContent::Blank ||
        map[line] == LineContent::Structural) {
        map[line] = type;
    } else if (map[line] == LineContent::Code ||
               type == LineContent::Code) {
        map[line] = LineContent::Code;
    }
}

inline void
StatsAggregator::aggregate(int value)
{
    if (sampleSize == 0 || value < min) {
        min = value;
    }
    if (sampleSize == 0 || value > max) {
        max = value;
    }

    ++sampleSize;

    data.push_back(value);
    sorted = false;
}

inline bool
StatsAggregator::isEmpty() const
{
    return (sampleSize == 0);
}

inline int
StatsAggregator::getSampleSize() const
{
    return sampleSize;
}

inline int
StatsAggregator::getMin() const
{
    return min;
}

inline int
StatsAggregator::getMax() const
{
    return max;
}

inline int
StatsAggregator::getMedian() const
{
    if (data.empty()) {
        return 0;
    }

    if (!sorted) {
        std::sort(data.begin(), data.end());
        sorted = true;
    }

    if (data.size() % 2 == 1) {
        return data[data.size()/2];
    }
    return (data[data.size()/2 - 1] + data[data.size()/2])/2;
}

inline
FileProcessor::FileProcessor(const Args &args, TimeReport &tr)
    : args(args), tr(tr)
{
    ColorScheme cs;
    lineNoHi = cs[ColorGroup::LineNo];
    pathHi = cs[ColorGroup::Path];

    using namespace decor::literals;
    blankHi      = decor::bold + decor::inv + 255_fg + decor::black_bg;
    commentHi    = decor::bold + decor::inv + 227_fg;
    codeHi       = decor::bold + decor::inv + 47_fg;
    structuralHi = decor::bold + decor::inv + 81_fg + decor::black_bg;
}

inline bool
FileProcessor::operator()(const std::string &path)
{
    cpp17::pmr::monolithic mr;
    Tree tree(&mr);

    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        tree = *t;
    } else {
        std::cerr << "Failed to parse: " << path << '\n';
        return false;
    }

    dumpTree(args, tree);

    if (args.dryRun) {
        return true;
    }

    Language &lang = *tree.getLanguage();

    LineAnalyzer lineAnalyzer(lang);
    FunctionAnalyzer functionAnalyzer(lang);
    for (const Node *node : NodeRange(tree.getRoot())) {
        if (node->leaf) {
            lineAnalyzer.countIn(node);
        } else if (lang.classify(node->stype) == MType::Function) {
            funcSizes.aggregate(functionAnalyzer.getLineCount(node));
            paramCounts.aggregate(functionAnalyzer.getParamCount(node));
        }
    }

    const std::vector<LineContent> &map = lineAnalyzer.getMap();

    std::unique_ptr<TermHighlighter> hi;
    if (args.annotate) {
        hi.reset(new TermHighlighter(tree));
        std::cout << (pathHi << path) << '\n';
    }

    const int lineColWidth = 1 + countWidth(map.size());

    int line = 1;
    for (LineContent c : map) {
        std::string str;
        const decor::Decoration *dec = nullptr;
        switch (c) {
            case LineContent::Blank:
                str = " blank ";
                dec = &blankHi;
                ++blank;
                break;
            case LineContent::Code:
                str = " code ";
                dec = &codeHi;
                ++code;
                break;
            case LineContent::Comment:
                str = " comment ";
                dec = &commentHi;
                ++comment;
                break;
            case LineContent::Structural:
                str = " structural ";
                dec = &structuralHi;
                ++structural;
                break;
        }
        if (hi != nullptr) {
            std::cout << std::setw(lineColWidth)
                << std::right << (lineNoHi << line << ' ') << ' '
                << std::setw(12) << std::left
                << (*dec << str) << " "
                << hi->print(line, 1) << '\n';
        }
        ++line;
    }
    ++files;
    return true;
}

inline void
FileProcessor::printReport() const
{
    std::cout << Header { "Input information" }
              << Bullet { "files" } << files << "\n\n";

    int nonBlank = comment + code + structural;
    int lines = blank + nonBlank;
    std::cout << Header { "Line statistics" }
              << Bullet { "blank      (B)" }
                 << Part { blank, lines } << '\n'
              << Bullet { "comment    (T)" }
                 << Part { comment, lines } << '\n'
              << Bullet { "code       (C)" }
                 << Part { code, lines } << '\n'
              << Bullet { "structural (S)" }
                 << Part { structural, lines } << '\n'
              << Bullet { "all        (A)" }
                 << Count { lines } << '\n'
              << SubHeader { "Interesting subsets" }
              << "  " << Bullet { "A-B" }
                 << Part { nonBlank, lines } << '\n'
              << "  " << Bullet { "B+T" }
                 << Part { blank + comment, lines } << '\n'
              << "  " << Bullet { "C+S" }
                 << Part { nonBlank - comment, lines } << '\n'
              << '\n';

    if (!funcSizes.isEmpty()) {
        std::cout << Header { "Function statistics" }
                  << Bullet { "count" }
                     << Count { funcSizes.getSampleSize() } << '\n'
                  << SubHeader { "Size" }
                  << "  " << Bullet { "min" }
                     << Count { funcSizes.getMin() } << '\n'
                  << "  " << Bullet { "median" }
                     << Count { funcSizes.getMedian() } << '\n'
                  << "  " << Bullet { "max" }
                     << Count { funcSizes.getMax() } << '\n'
                  << SubHeader { "Params" }
                  << "  " << Bullet { "min" }
                     << Count { paramCounts.getMin() } << '\n'
                  << "  " << Bullet { "median" }
                     << Count { paramCounts.getMedian() } << '\n'
                  << "  " << Bullet { "max" }
                     << Count { paramCounts.getMax() } << '\n';
    }
    // XXX: histograms?
    // XXX: statements per function?
    // XXX: parameters per function?
    // XXX: functions per file?
    // XXX: depth, complexity?
    // XXX: remember list of functions to list them on some option?
}

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);
static int run(const Args &args, Environment &env);

const char *const usage =
R"(Usage: zs-stats [options...] [paths...]

Paths can specify both files and directories.  When no path is specified, "." is
assumed.
)";

int
main(int argc, char *argv[])
{
    int result;

    try {
        Environment env(getLocalOpts());
        env.setup({ argv + 1, argv + argc });

        Args args = parseLocalArgs(env);
        if (args.help) {
            std::cout << usage
                      << "\n"
                      << "Options:\n";
            env.printOptions();
            return EXIT_SUCCESS;
        }
        result = run(args, env);

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    return result;
}

// Retrieves description of options specific to this tool.
static boost::program_options::options_description
getLocalOpts()
{
    boost::program_options::options_description options;
    options.add_options()
        ("annotate", "print source code annotated with line types");

    return options;
}

// Parses options specific to the tool.
static Args
parseLocalArgs(const Environment &env)
{
    Args args;
    static_cast<CommonArgs &>(args) = env.getCommonArgs();

    const boost::program_options::variables_map &varMap = env.getVarMap();

    args.annotate = varMap.count("annotate");

    return args;
}

// Runs the tool.  Returns exit code of the application.
static int
run(const Args &args, Environment &env)
{
    std::vector<std::string> paths = args.pos;
    if (paths.empty()) {
        paths.emplace_back(".");
    }

    TimeReport &tr = env.getTimeKeeper();
    Config &config = env.getConfig();
    FileProcessor processor(args, tr);

    if (!Traverser(paths, args.lang, config, std::ref(processor)).search()) {
        std::cerr << "No matching files were discovered.\n";
        return EXIT_FAILURE;
    }

    processor.printReport();
    return EXIT_SUCCESS;
}
