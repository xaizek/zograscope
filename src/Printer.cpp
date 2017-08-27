#include "Printer.hpp"

#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "dtl/dtl.hpp"

#include "decoration.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

// TODO: colors should reside in some configuration file, it's very inconvenient
//       to have to recompile the tool to experiment with coloring.

enum class Diff
{
    Left,
    Right,
    Identical,
    Different,
    Fold,
};

struct DiffLine
{
    DiffLine(Diff type, int data = 0) : type(type), data(data)
    {
    }

    Diff type;
    int data;
};

static std::string noLineMarker(int at);
static int countWidth(int n);
static std::vector<DiffLine> dtlCompare(const std::vector<std::string> &l,
                                        const std::vector<std::string> &r);
static decor::Decoration getHighlight(const Node &node);
static unsigned int measureWidth(const std::string &s);

Printer::Printer(Node &left, Node &right) : left(left), right(right)
{
}

void
Printer::addHeader(Header header)
{
    headers.emplace_back(std::move(header));
}

void
Printer::print(TimeReport &tr)
{
    using namespace decor::literals;

    static std::string empty;

    auto diffingTimer = tr.measure("printing");

    std::vector<std::string> l = (tr.measure("left-highlight"),
                                  split(printSource(left), '\n'));
    std::vector<std::string> r = (tr.measure("right-highlight"),
                                  split(printSource(right), '\n'));

    // TODO: need to do comparison without highlighting or it skews results
    std::vector<DiffLine> diff = (tr.measure("compare"), dtlCompare(l, r));

    auto timer = tr.measure("printing");

    unsigned int maxLeftWidth = 0U;
    unsigned int maxRightWidth = 0U;
    std::vector<unsigned int> leftWidths;

    unsigned int maxLeftNum = 0U, maxRightNum = 0U;

    unsigned int i = 0U, j = 0U;
    for (DiffLine d : diff) {
        switch (d.type) {
            unsigned int width;

            case Diff::Left:
                width = measureWidth(l[i++]);
                leftWidths.push_back(width);
                maxLeftWidth = std::max(width, maxLeftWidth);
                break;
            case Diff::Right:
                width = measureWidth(r[j++]);
                maxRightWidth = std::max(width, maxRightWidth);
                break;
            case Diff::Identical:
            case Diff::Different:
                width = measureWidth(l[i++]);
                leftWidths.push_back(width);
                maxLeftWidth = std::max(width, maxLeftWidth);

                width = measureWidth(r[j++]);
                maxRightWidth = std::max(width, maxRightWidth);
                break;
            case Diff::Fold:
                i += d.data;
                j += d.data;
                continue;
        }

        // Record last non-folded indices.
        maxLeftNum = i;
        maxRightNum = j;
    }

    const int lWidth = countWidth(maxLeftNum) + 1;
    const int rWidth = countWidth(maxRightNum) + 1;

    const int wholeWidth = lWidth + 1 + 1 + maxLeftWidth
                         + 3
                         + rWidth + 1 + 1 + maxRightWidth;

    auto title = 231_fg + decor::bold;

    auto separator = [&]() {
        std::cout << std::setfill('~')
                  << std::setw(lWidth + 1 + 1 + maxLeftWidth + 1)
                  << (231_fg << "")
                  << (decor::bold << '!')
                  << std::setw(rWidth + 1 + 1 + maxRightWidth + 1)
                  << (231_fg << "")
                  << std::setfill(' ')
                  << '\n';
    };

    separator();

    std::string leftMarker = ' ' + std::string(lWidth - 1, '-') + "  ";
    std::string rightMarker = ' ' + std::string(rWidth - 1, '+') + "  ";
    for (const Header &hdr : headers) {
        const std::string left = leftMarker + hdr.left + ' ';
        std::cout << (title << std::left
                            << std::setw(lWidth + 1 + 1 + maxLeftWidth)
                            << left
                            << " ! "
                            << std::setw(rWidth + 1 + 1 + maxRightWidth)
                            << rightMarker + hdr.right + ' ')
                  << '\n';
    }

    decor::Decoration lineNo = decor::white_bg + decor::black_fg;

    separator();

    i = 0U;
    j = 0U;
    unsigned int ii = 0U;
    for (DiffLine d : diff) {
        const std::string *ll = &empty;
        const std::string *rl = &empty;

        const char *marker;
        switch (d.type) {
            case Diff::Left:
                ll = &l[i++];
                marker = " < ";
                break;
            case Diff::Right:
                rl = &r[j++];
                marker = " > ";
                break;
            case Diff::Identical:
                ll = &l[i++];
                rl = &r[j++];
                marker = " | ";
                break;
            case Diff::Different:
                ll = &l[i++];
                rl = &r[j++];
                marker = " ~ ";
                break;
            case Diff::Fold:
                i += d.data;
                j += d.data;
                {
                    std::string msg = " @@ folded " + std::to_string(d.data)
                                    + " identical lines @@ ";

                    int leftFill = lWidth + 1 + 1 + maxLeftWidth + 2
                                 - msg.size()/2;
                    int rightFill = wholeWidth - (leftFill + msg.size());

                    std::cout << std::right << std::setfill('.')
                              << (251_fg << std::setw(leftFill) << "")
                              << msg
                              << (251_fg << std::setw(rightFill) << "")
                              << '\n' << std::setfill(' ');
                }
                continue;
        }

        unsigned int width = (d.type == Diff::Right ? 0U : leftWidths[ii++]);
        width = maxLeftWidth + (ll->size() - width);

        if (d.type != Diff::Right) {
            std::cout << (lineNo << std::right << std::setw(lWidth) << i << ' ')
                      << ' ' << std::left << std::setw(width) << *ll;
        } else {
            std::cout << (lineNo << std::right << std::setw(lWidth + 1)
                                 << (noLineMarker(i) + ' '))
                      << ' ' << std::left << std::setw(width)
                      << (235_bg << *ll);
        }

        std::cout << marker;

        if (d.type != Diff::Left) {
            std::cout << (lineNo << std::right << std::setw(rWidth) << j << ' ')
                      << ' ' << *rl;
        } else {
            std::cout << (lineNo << std::right << std::setw(rWidth + 1)
                                 << (noLineMarker(j) + ' '))
                      << ' ' << std::left << std::setw(maxRightWidth)
                      << (235_bg << *rl);
        }

        std::cout << '\n';
    }
}

/**
 * @brief Generates string that should be used instead of line number.
 *
 * @param at Line number of the last displayed line (0 if none was printed).
 *
 * @returns The marker.
 */
static std::string
noLineMarker(int at)
{
    return std::string(countWidth(at), '-');
}

static int
countWidth(int n)
{
    int width = 0;
    while (n > 0) {
        n /= 10;
        ++width;
    }
    return (width == 0) ? 1 : width;
}

static std::vector<DiffLine>
dtlCompare(const std::vector<std::string> &l, const std::vector<std::string> &r)
{
    using size_type = std::vector<std::string>::size_type;

    std::vector<DiceString> leftTrimmed;
    leftTrimmed.reserve(l.size());
    for (const auto &s : l) {
        leftTrimmed.emplace_back(s);
    }

    std::vector<DiceString> rightTrimmed;
    rightTrimmed.reserve(r.size());
    for (const auto &s : r) {
        rightTrimmed.emplace_back(s);
    }

    auto cmp = [](DiceString &a, DiceString &b) {
        return (a.compare(b) >= 0.8f);
    };

    dtl::Diff<DiceString, std::vector<DiceString>, decltype(cmp)>
        diff(leftTrimmed, rightTrimmed, cmp);
    diff.compose();

    size_type identicalLines = 0U;
    const size_type minFold = 3;
    const size_type ctxSize = 2;
    std::vector<DiffLine> diffSeq;

    auto foldIdentical = [&](bool last) {
        const size_type startContext =
            (identicalLines == diffSeq.size() ? 0U : ctxSize);
        const size_type endContext = (last ? 0U : ctxSize);
        const size_type context = startContext + endContext;

        if (identicalLines >= context && identicalLines - context > minFold) {
            diffSeq.erase(diffSeq.cend() - (identicalLines - startContext),
                          diffSeq.cend() - endContext);
            diffSeq.emplace(diffSeq.cend() - endContext, Diff::Fold,
                            identicalLines - context);
        }
        identicalLines = 0U;
    };

    auto handleSameLines = [&](size_type i, size_type j) {
        if (l[i] == r[j]) {
            ++identicalLines;
            diffSeq.emplace_back(Diff::Identical);
        } else {
            foldIdentical(false);
            diffSeq.emplace_back(Diff::Different);
        }
    };

    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            case dtl::SES_DELETE:
                foldIdentical(false);
                diffSeq.emplace_back(Diff::Left);
                break;
            case dtl::SES_ADD:
                foldIdentical(false);
                diffSeq.emplace_back(Diff::Right);
                break;
            case dtl::SES_COMMON:
                handleSameLines(x.second.beforeIdx - 1, x.second.afterIdx - 1);
                break;
        }
    }

    foldIdentical(true);

    return diffSeq;
}

namespace {

class ColorPicker
{
public:
    void setNode(const Node &node)
    {
        prevNode = currNode;
        currNode = &node;

        prevHighlight = std::move(currHighlight);
        currHighlight = ::getHighlight(*currNode);
    }

    void advancedLine()
    {
        prevNode = nullptr;
    }

    const decor::Decoration & getHighlight() const
    {
        return currHighlight;
    }

    decor::Decoration getFillHighlight()
    {
        if (prevNode == nullptr) {
            return decor::none;
        }

        if (prevHighlight == currHighlight) {
            return currHighlight;
        }

        if (prevNode->moved || currNode->moved) {
            return (currNode->moved ? prevHighlight : currHighlight);
        }

        return decor::none;
    }

private:
    decor::Decoration currHighlight;
    decor::Decoration prevHighlight;
    const Node *currNode = nullptr;
    const Node *prevNode = nullptr;
};

}

std::string
printSource(Node &root)
{
    std::ostringstream oss;

    std::function<void(Node &, State)> mark = [&](Node &node, State state) {
        node.state = state;
        for (Node *child : node.children) {
            mark(*child, state);
        }
    };

    int line = 1, col = 1;
    ColorPicker colorPicker;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.next != nullptr) {
            if (node.state != State::Unchanged) {
                mark(*node.next, node.state);
            }
            if (node.moved) {
                markTreeAsMoved(node.next);
            }
            return visit(*node.next);
        }

        if (node.line != 0 && node.col != 0) {
            colorPicker.setNode(node);

            decor::Decoration fillHighlight;
            // don't do filling across lines
            if (node.line == line) {
                fillHighlight = colorPicker.getFillHighlight();
                oss << fillHighlight;
            }

            while (node.line > line) {
                oss << '\n';
                ++line;
                colorPicker.advancedLine();
                col = 1;
            }

            while (node.col > col) {
                oss << ' ';
                ++col;
            }

            if (fillHighlight != decor::none) {
                oss << decor::def;
            }

            const decor::Decoration &dec = colorPicker.getHighlight();

            // if (node.state != State::Unchanged) {
            //     oss << (dec << '[') << node.label << (dec << ']');
            // } else {
            //     oss << node.label;
            // }

            const std::vector<std::string> lines = split(node.label, '\n');
            oss << (dec << lines.front());
            for (std::size_t i = 1U; i < lines.size(); ++i) {
                std::size_t whitespaceLength =
                    lines[i].find_first_not_of(" \t");
                if (whitespaceLength == std::string::npos) {
                    whitespaceLength = lines[i].size();
                }
                oss << '\n' << lines[i].substr(0, whitespaceLength)
                            << (dec << lines[i].substr(whitespaceLength));
                ++line;
                colorPicker.advancedLine();
            }

            col += node.label.size();
        }

        for (Node *child : node.children) {
            // if (child.satellite) {
            //     child.state = node.state;
            // }
            visit(*child);
        }
    };
    visit(root);

    return oss.str();
}

/**
 * @brief Retrieves decoration for the specified node.
 *
 * @param node Node whose decoration is being queried.
 *
 * @returns The decoration.
 */
static decor::Decoration
getHighlight(const Node &node)
{
    using namespace decor::literals;

    // Highlighting based on node state has higher priority.
    switch (node.state) {
        using namespace decor;

        case State::Deleted:  return 210_fg + inv + black_bg + bold;
        case State::Inserted: return  85_fg + inv + black_bg + bold;
        case State::Updated:
            if (node.moved) {
                return 213_fg + inv + black_bg + bold;
            } else {
                return 228_fg + inv + black_bg + bold;
            }

        case State::Unchanged:
            if (node.moved) {
                return 117_fg + inv + bold;
            }
            break;
    }

    // Highlighting based on node type follows.
    switch (node.type) {
        case Type::Specifiers: return 183_fg;
        case Type::UserTypes:  return 215_fg;
        case Type::Types:      return  85_fg;
        case Type::Directives: return 228_fg;
        case Type::Comments:   return 248_fg;
        case Type::Constants:  return 219_fg;
        case Type::Functions:  return  81_fg;

        case Type::Jumps:
        case Type::Keywords:
            return 115_fg;
        case Type::LeftBrackets:
        case Type::RightBrackets:
            return 222_fg;
        case Type::Assignments:
        case Type::Operators:
        case Type::Comparisons:
            return 224_fg;

        case Type::Identifiers:
        case Type::Other:
        case Type::Virtual:
        case Type::NonInterchangeable:
            break;
    }

    // Use default terminal colors if wasn't handled above.
    return decor::Decoration();
}

/**
 * @brief Calculates width of a string ignoring embedded escape sequences.
 *
 * @param s String to measure.
 *
 * @returns The width.
 */
static unsigned int
measureWidth(const std::string &s)
{
    unsigned int valWidth = 0U;
    const char *str = s.c_str();
    while (*str != '\0') {
        if (*str != '\033') {
            ++valWidth;
            ++str;
            continue;
        }

        const char *const next = std::strchr(str, 'm');
        str = (next == nullptr) ? (str + std::strlen(str)) : (next + 1);
    }
    return valWidth;
}

