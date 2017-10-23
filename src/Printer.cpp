#include "Printer.hpp"

#include <functional>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include "dtl/dtl.hpp"

#include "Highlighter.hpp"
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
static std::vector<DiffLine> dtlCompare(std::vector<std::string> &&l,
                                        std::vector<std::string> &&r);
static unsigned int measureWidth(boost::string_ref s);

static std::string empty;

Printer::Printer(Node &left, Node &right, std::ostream &os)
    : left(left), right(right), os(os)
{
}

void
Printer::addHeader(Header header)
{
    headers.emplace_back(std::move(header));
}

static std::vector<std::string>
treePrint(Node &root)
{
    std::vector<std::string> lines;

    int line = 0, col = 1;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.next != nullptr) {
            return visit(*node.next);
        }

        if (node.line != 0 && node.col != 0) {
            if (node.line > line) {
                lines.insert(lines.cend(), node.line - line, empty);
                line = node.line;
                col = 1;
            }

            if (node.col > col) {
                lines.back().append(node.col - col, ' ');
                col = node.col;
            }

            std::vector<boost::string_ref> spell = split(node.spelling, '\n');
            col += spell.front().size();
            lines.back() += spell.front().to_string();

            for (std::size_t i = 1U; i < spell.size(); ++i) {
                ++line;
                col = 1 + spell[i].size();
                lines.emplace_back(spell[i]);
            }
        }

        for (Node *child : node.children) {
            visit(*child);
        }
    };
    visit(root);

    return lines;
}

void
Printer::print(TimeReport &tr)
{
    using namespace decor::literals;

    auto diffingTimer = tr.measure("printing");

    // Do comparison without highlighting as it skews alignment results.
    std::vector<std::string> lp = (tr.measure("left-print"),
                                   treePrint(left));
    std::vector<std::string> rp = (tr.measure("right-print"),
                                   treePrint(right));
    std::vector<DiffLine> diff = (tr.measure("compare"),
                                  dtlCompare(std::move(lp), std::move(rp)));

    // TODO: don't highlight lines that won't be displayed (it takes extra
    //       time).
    const std::string ls = (tr.measure("left-highlight"),
                            Highlighter(true).print(left));
    const std::string rs = (tr.measure("right-highlight"),
                            Highlighter(false).print(right));
    std::vector<boost::string_ref> l = split(ls, '\n');
    std::vector<boost::string_ref> r = split(rs, '\n');

    auto timer = tr.measure("printing");

    unsigned int maxLeftWidth = 0U;
    unsigned int maxRightWidth = 0U;
    std::vector<unsigned int> leftWidths;

    for (const Header &hdr : headers) {
        maxLeftWidth = std::max<unsigned int>(hdr.left.size(), maxLeftWidth);
        maxRightWidth = std::max<unsigned int>(hdr.right.size(), maxRightWidth);
    }

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
        os << std::setfill('~')
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
        const std::string left = leftMarker + hdr.left;
        os << (title << std::left
                     << std::setw(lWidth + 1 + 1 + maxLeftWidth)
                     << left
                     << " ! "
                     << std::setw(rWidth + 1 + 1 + maxRightWidth)
                     << rightMarker + hdr.right)
           << '\n';
    }

    decor::Decoration lineNo = decor::white_bg + decor::black_fg;

    separator();

    i = 0U;
    j = 0U;
    unsigned int ii = 0U;
    for (DiffLine d : diff) {
        boost::string_ref ll = empty;
        boost::string_ref rl = empty;

        const char *marker;
        switch (d.type) {
            case Diff::Left:
                ll = l[i++];
                marker = " < ";
                break;
            case Diff::Right:
                rl = r[j++];
                marker = " > ";
                break;
            case Diff::Identical:
                ll = l[i++];
                rl = r[j++];
                marker = " | ";
                break;
            case Diff::Different:
                ll = l[i++];
                rl = r[j++];
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

                    os << std::right << std::setfill('.')
                       << (251_fg << std::setw(leftFill) << "")
                       << msg
                       << (251_fg << std::setw(rightFill) << "")
                       << '\n' << std::setfill(' ');
                }
                continue;
        }

        unsigned int width = (d.type == Diff::Right ? 0U : leftWidths[ii++]);
        width = maxLeftWidth + (ll.size() - width);

        if (d.type != Diff::Right) {
            os << (lineNo << std::right << std::setw(lWidth) << i << ' ')
               << ' ' << std::left << std::setw(width) << ll;
        } else {
            os << (lineNo << std::right << std::setw(lWidth + 1)
                          << (noLineMarker(i) + ' '))
               << ' ' << std::left << std::setw(width)
               << (235_bg << ll);
        }

        os << marker;

        if (d.type != Diff::Left) {
            os << (lineNo << std::right << std::setw(rWidth) << j << ' ')
               << ' ' << rl;
        } else {
            os << (lineNo << std::right << std::setw(rWidth + 1)
                          << (noLineMarker(j) + ' '))
               << ' ' << std::left << std::setw(maxRightWidth)
               << (235_bg << rl);
        }

        os << '\n';
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
dtlCompare(std::vector<std::string> &&l, std::vector<std::string> &&r)
{
    using size_type = std::vector<std::string>::size_type;

    std::vector<DiceString> lt;
    lt.reserve(l.size());
    for (const auto &s : l) {
        lt.emplace_back(std::move(s));
    }

    std::vector<DiceString> rt;
    rt.reserve(r.size());
    for (const auto &s : r) {
        rt.emplace_back(std::move(s));
    }

    auto cmp = [](DiceString &a, DiceString &b) {
        return (a.compare(b) >= 0.8f);
    };

    dtl::Diff<DiceString, std::vector<DiceString>, decltype(cmp)>
        diff(lt, rt, cmp);
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
        if (lt[i].str() == rt[j].str()) {
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

/**
 * @brief Calculates width of a string ignoring embedded escape sequences.
 *
 * @param s String to measure.
 *
 * @returns The width.
 */
static unsigned int
measureWidth(boost::string_ref s)
{
    unsigned int valWidth = 0U;
    while (!s.empty()) {
        if (s.front() != '\033') {
            ++valWidth;
            s.remove_prefix(1);
            continue;
        }

        const auto width = s.find('m');
        if (width == std::string::npos) {
            break;
        }
        s.remove_prefix(width + 1U);
    }
    return valWidth;
}
