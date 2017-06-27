#include "Printer.hpp"

#include <boost/algorithm/string/trim.hpp>

#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "decoration.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

// TODO: colors should reside in some configuration file, it's very inconvenient
//       to have to recompile the tool to experiment with coloring.

class IntMatrix
{
public:
    IntMatrix(int n, int m) : m(m), data(new int[n*m])
    {
    }

    int & operator()(int i, int j)
    {
        return data.get()[i*m + j];
    }

private:
    const int m;
    std::unique_ptr<int[]> data;
};

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
static std::deque<DiffLine> compare(const std::vector<std::string> &l,
                                    const std::vector<std::string> &r);
static decor::Decoration getHighlight(const Node &node);
static unsigned int measureWidth(const std::string &s);

Printer::Printer(Node &left, Node &right) : left(left), right(right)
{
}

void
Printer::print()
{
    using namespace decor::literals;

    static std::string empty;

    std::vector<std::string> l = split(printSource(left), '\n');
    std::vector<std::string> r = split(printSource(right), '\n');

    std::deque<DiffLine> diff = compare(l, r);

    unsigned int maxLeftWidth = 0U;
    unsigned int maxRightWidth = 0U;
    std::vector<unsigned int> leftWidths;

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
                break;
        }
    }

    const int lWidth = countWidth(l.size()) + 1;
    const int rWidth = countWidth(r.size()) + 1;

    decor::Decoration lineNo = decor::white_bg + decor::black_fg;

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
                marker = " << ";
                break;
            case Diff::Right:
                rl = &r[j++];
                marker = " >> ";
                break;
            case Diff::Identical:
                ll = &l[i++];
                rl = &r[j++];
                marker = " || ";
                break;
            case Diff::Different:
                ll = &l[i++];
                rl = &r[j++];
                marker = " <> ";
                break;
            case Diff::Fold:
                i += d.data;
                j += d.data;
                {
                    std::string msg = " @@ folded " + std::to_string(d.data)
                                    + " identical lines @@";
                    std::cout << std::right
                              << std::setw(maxLeftWidth + lWidth + 4 +
                                           msg.size()/2)
                              << msg << '\n';
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

static std::deque<DiffLine>
compare(const std::vector<std::string> &l, const std::vector<std::string> &r)
{
    enum { Wins = 1, Wdel = 1, Wren = 1 };

    using size_type = std::vector<std::string>::size_type;

    // Narrow portion of lines that should be compared by throwing away matching
    // leading and trailing lines.
    size_type ol = 0U, nl = 0U, ou = l.size(), nu = r.size();
    while (ol < ou && nl < nu && l[ol] == r[nl]) {
        ++ol;
        ++nl;
    }
    while (ou > ol && nu > nl && l[ou - 1U] == r[nu - 1U]) {
        --ou;
        --nu;
    }

    IntMatrix d(ou - ol + 1U, nu - nl + 1U);

    std::vector<std::string> rightTrimmed;
    rightTrimmed.reserve(nu - nl);
    for (size_type i = nl; i < nu; ++i) {
        rightTrimmed.emplace_back(boost::trim_copy(r[i]));
    }

    // Edit distance finding.
    for (size_type j = 0U; j <= nu - nl; ++j) {
        d(0, j) = j;
    }
    for (size_type i = 0U; i <= ou - ol; ++i) {
        d(i, 0) = i;
    }
    for (size_type i = 1U; i <= ou - ol; ++i) {
        const std::string leftTrimmed = boost::trim_copy(l[ol + i - 1U]);
        for (size_type j = 1U; j <= nu - nl; ++j) {
            // XXX: should we compare tokens here instead?
            const bool same =
                diceCoefficient(leftTrimmed, rightTrimmed[j - 1U]) >= 0.8f;
            d(i, j) = std::min(std::min(d(i - 1U, j) + Wren,
                                        d(i, j - 1U) + Wins),
                               d(i - 1U, j - 1U) + (same ? 0 : Wren));
        }
    }

    size_type identicalLines = 0U;

    const size_type minFold = 3;
    const size_type ctxSize = 2;

    std::deque<DiffLine> diffSeq;

    auto foldIdentical = [&](bool last) {
        size_type startContext = (last ? 0 : ctxSize);
        size_type endContext = (identicalLines == diffSeq.size() ? 0 : ctxSize);
        size_type context = startContext + endContext;

        if (identicalLines >= context && identicalLines - context > minFold) {
            diffSeq.erase(diffSeq.cbegin() + startContext,
                          diffSeq.cbegin() + (identicalLines - endContext));
            diffSeq.emplace(diffSeq.cbegin() + startContext, Diff::Fold,
                            identicalLines - context);
        }
        identicalLines = 0U;
    };

    auto handleSameLines = [&](size_type i, size_type j) {
        if (l[i] == r[j]) {
            ++identicalLines;
            diffSeq.emplace_front(Diff::Identical);
        } else {
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Different);
        }
    };

    // Compose results with folding of long runs of identical lines (longer
    // than two lines).  Mind that we go from last to first element and loops
    // below process tail, middle and then head parts of the files.

    for (size_type k = l.size(), l = r.size(); k > ou; --k, --l) {
        handleSameLines(k - 1U, l - 1U);
    }

    int i = ou - ol, j = nu - nl;
    while (i != 0U || j != 0U) {
        if (i == 0) {
            --j;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Right);
        } else if (j == 0) {
            --i;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Left);
        } else if (d(i, j) == d(i, j - 1) + Wins) {
            --j;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Right);
        } else if (d(i, j) == d(i - 1, j) + Wdel) {
            --i;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Left);
        } else {
            --i;
            --j;
            handleSameLines(ol + i, nl + j);
        }
    }

    for (size_type i = ol; i != 0U; --i) {
        handleSameLines(i - 1U, i - 1U);
    }

    foldIdentical(true);

    return diffSeq;
}

std::string
printSource(Node &root)
{
    std::ostringstream oss;

    int line = 1, col = 1;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.line != 0 && node.col != 0) {
            while (node.line > line) {
                oss << '\n';
                ++line;
                col = 1;
            }

            while (node.col > col) {
                oss << ' ';
                ++col;
            }

            decor::Decoration dec = getHighlight(node);

            // if (node.state != State::Unchanged) {
            //     oss << (dec << '[') << node.label << (dec << ']');
            // } else {
            //     oss << node.label;
            // }

            const std::vector<std::string> lines = split(node.label, '\n');
            oss << (dec << lines.front());
            for (std::size_t i = 1U; i < lines.size(); ++i) {
                oss << '\n' << (dec << lines[i]);
                ++line;
            }

            col += node.label.size();
        }

        for (Node &child : node.children) {
            // if (child.satellite) {
            //     child.state = node.state;
            // }
            visit(child);
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
        case State::Inserted: return  84_fg + inv + black_bg + bold;
        case State::Updated:  return 228_fg + inv + black_bg + bold;

        case State::Unchanged:
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
            return 83_fg;
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

