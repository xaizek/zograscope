// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#include "Printer.hpp"

#include <functional>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/utility/string_ref.hpp>

#include "utils/nums.hpp"
#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "ColorScheme.hpp"
#include "TermHighlighter.hpp"
#include "align.hpp"
#include "decoration.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

namespace {

class LayoutBuilder;

// Provides parameters of layout.
class Layout
{
    friend class LayoutBuilder;

private:
    // Reference to the builder is saved, so it should outlive layout.
    explicit Layout(const LayoutBuilder &builder,
                    std::vector<int> &&leftWidths);

public:
    // Retrieves width of a marker used in left part of header line.
    int getLeftMarkerWidth() const { return leftNumWidth; }
    // Retrieves width of a marker used in right part of header line.
    int getRightMarkerWidth() const { return rightNumWidth; }

    // Retrieves width of left part of header line excluding middle separator.
    int getLeftHeaderWidth() const { return leftHeaderWidth; }
    // Retrieves width of right part of header line excluding middle separator.
    int getRightHeaderWidth() const { return (usefulWidth - leftHeaderWidth); }

    // Retrieves width of number for the left part including left padding.
    int getLeftNumWidth() const { return 1 + leftNumWidth; }
    // Retrieves width of number for the right part including left padding.
    int getRightNumWidth() const { return 1 + rightNumWidth; }

    // Whether left part should be printed.
    bool isLeftVisible() const { return leftVisible; }
    // Whether right part should be printed.
    bool isRightVisible() const { return rightVisible; }

    // Retrieves maximum width of line on the left part.
    int getMaxLeftWidth() const { return maxLeftWidth; }
    // Retrieves maximum width of line on the right part.
    int getMaxRightWidth() const { return maxRightWidth; }

    // Retrieves width for string on the left.
    int getLeftWidth(boost::string_ref ll, int idx) const
    {
        const int extraWidth = ll.size() - leftWidths[idx];
        if (rightVisible) {
            return maxLeftWidth + extraWidth;
        }
        return wholeWidth              // Total width being used.
             - 1 - 1 - 1               // Marker and left&right padding.
             - (getLeftNumWidth() + 1) // Width of line number column.
             + extraWidth;             // Account for escape sequences.
    }

    // Computes message centering parameters.
    void centerMsg(const std::string &msg, int &leftFill, int &rightFill) const
    {
        const int msgLen = msg.size();
        leftFill = ((leftVisible && rightVisible) ? leftWidth : wholeWidth/2)
                 + 2 - msgLen/2;
        rightFill = wholeWidth - (leftFill + msgLen);
    }

private:
    // Computes whole width of a result.
    int computeWholeWidth() const
    {
        if (leftVisible && rightVisible) {
            return leftPart + 1 + 1 + rightWidth + 1;
        }

        int left = 1 + getLeftMarkerWidth() + 2 + maxLeftHeaderWidth;
        int right = 1 + getRightMarkerWidth() + 2 + maxRightHeaderWidth;

        int headerWidth = left + 3 + right;
        return std::max({ leftPart + 2, rightWidth + 2, headerWidth });
    }

    // Computes width of left header.
    int computeLeftHeaderWidth() const
    {
        if (leftVisible && rightVisible) {
            return leftWidth;
        }

        const int left = 1 + getLeftMarkerWidth() + 2 + maxLeftHeaderWidth;
        const int right = 1 + getRightMarkerWidth() + 2 + maxRightHeaderWidth;

        int width = std::max(left, usefulWidth/2 - 1);
        if (width < right) {
            width = usefulWidth - right;
        }
        return width;
    }

private:
    bool leftVisible, rightVisible;
    int maxLeftWidth, maxRightWidth;
    int leftNumWidth, rightNumWidth;

    int maxLeftHeaderWidth, maxRightHeaderWidth;

    int leftHeaderWidth;
    int leftPart;

    int wholeWidth, usefulWidth;
    int leftWidth, rightWidth;

    std::vector<int> leftWidths;
};

// Collects information needed to compute layout.
class LayoutBuilder
{
    friend class Layout;

public:
    // Starts computing layout by analyzing headers and contents of parts.
    LayoutBuilder(const DiffSource &lsrc, const DiffSource &rsrc,
                  const std::vector<Header> &headers)
    {
        leftVisible = std::find(lsrc.modified.cbegin(), lsrc.modified.cend(),
                                true) != lsrc.modified.cend();
        rightVisible = std::find(rsrc.modified.cbegin(), rsrc.modified.cend(),
                                 true) != rsrc.modified.cend();

        if (!leftVisible && !rightVisible) {
            leftVisible = true;
            rightVisible = true;
        }

        for (const Header &hdr : headers) {
            maxLeftHeaderWidth = std::max<int>(hdr.left.size(),
                                               maxLeftHeaderWidth);
            maxRightHeaderWidth = std::max<int>(hdr.right.size(),
                                                maxRightHeaderWidth);
        }
    }

public:
    // Whether left part should be printed.
    bool isLeftVisible() const { return leftVisible; }
    // Whether right part should be printed.
    bool isRightVisible() const { return rightVisible; }

    // Records maximum line numbers at both sides.
    void setMaxLineNums(int l, int r)
    {
        maxLeftNum = l;
        maxRightNum = r;
    }

    // Records width of a line on the left part.
    void measureLeft(const std::string &line)
    {
        if (!leftVisible) {
            leftWidths.push_back(0);
            return;
        }

        const int width = measureWidth(line);
        leftWidths.push_back(width);
        maxLeftWidth = std::max(width, maxLeftWidth);
    }

    // Records width of a line on the right part.
    void measureRight(const std::string &line)
    {
        if (rightVisible) {
            const int width = measureWidth(line);
            maxRightWidth = std::max(width, maxRightWidth);
        }
    }

    // Produces layout containing all necessary data.
    Layout compute()
    {
        maxLeftWidth = std::max(maxLeftWidth, maxLeftHeaderWidth);
        maxRightWidth = std::max(maxRightWidth, maxRightHeaderWidth);
        return Layout(*this, std::move(leftWidths));
    }

private:
    // Calculates width of a string ignoring embedded escape sequences.
    static unsigned int measureWidth(boost::string_ref s)
    {
        // XXX: we actually print lines without formatting and should be able to
        //      avoid using this function.
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

private:
    bool leftVisible, rightVisible;

    int maxLeftHeaderWidth = 0, maxRightHeaderWidth = 0;
    int maxLeftWidth = 0, maxRightWidth = 0;
    int maxLeftNum = 0, maxRightNum = 0;

    std::vector<int> leftWidths;
};

Layout::Layout(const LayoutBuilder &builder, std::vector<int> &&leftWidths)
    : leftWidths(std::move(leftWidths))
{
    leftVisible = builder.leftVisible;
    rightVisible = builder.rightVisible;

    maxLeftHeaderWidth = builder.maxLeftHeaderWidth;
    maxRightHeaderWidth = builder.maxRightHeaderWidth;

    maxLeftWidth = builder.maxLeftWidth;
    maxRightWidth = builder.maxRightWidth;

    leftNumWidth = countWidth(builder.maxLeftNum);
    rightNumWidth = countWidth(builder.maxRightNum);

    leftPart = leftVisible ? (leftNumWidth + 1) + 1 + 1 + maxLeftWidth : 0;
    rightWidth = rightVisible ? (rightNumWidth + 1) + 1 + 1 + maxRightWidth : 0;
    wholeWidth = computeWholeWidth();
    usefulWidth = wholeWidth - 3;
    leftWidth = rightVisible ? leftPart : wholeWidth - 2;

    leftHeaderWidth = computeLeftHeaderWidth();
}

// This class is responsible for formatting output while printing it to a
// stream.
class Outliner
{
public:
    // Reference to layout is saved, so it should outlive outliner.
    Outliner(std::ostream &os, const Layout &layout) : os(os), layout(layout)
    {
        leftMarker = ' '
                   + std::string(layout.getLeftMarkerWidth(), '-')
                   + "  ";
        rightMarker = ' '
                    + std::string(layout.getRightMarkerWidth(), '+')
                    + "  ";
    }

public:
    // Prints horizontal separator.
    void printSeparator()
    {
        using namespace decor::literals;

        os << std::setfill('~')
           << std::setw(layout.getLeftHeaderWidth() + 1) << (231_fg << "")
           << (decor::bold << '!')
           << std::setw(layout.getRightHeaderWidth() + 1) << (231_fg << "")
           << std::setfill(' ')
           << '\n';
    }

    // Prints single header.
    void printHeader(const Header &hdr)
    {
        using namespace decor::literals;

        auto title = 231_fg + decor::bold;
        os << (title << std::left
                     << std::setw(layout.getLeftHeaderWidth())
                     << leftMarker + hdr.left
                     << " ! "
                     << std::setw(layout.getRightHeaderWidth())
                     << rightMarker + hdr.right)
           << '\n';
    }

    // Prints a horizontally centered message.
    void printMsg(const std::string &msg)
    {
        using namespace decor::literals;

        int leftFill, rightFill;
        layout.centerMsg(msg, leftFill, rightFill);

        os << std::right << std::setfill('.')
           << (251_fg << std::setw(leftFill) << "")
           << msg
           << (251_fg << std::setw(rightFill) << "")
           << '\n' << std::setfill(' ');
    }

    // Prints line of the left part.
    void printLeftLine(int lineNum, boost::string_ref str)
    {
        const int width = layout.getLeftWidth(str, leftWidthIndex++);
        printLine(lineNum, str, layout.getLeftNumWidth(), width);
    }

    // Prints line of the right part.
    void printRightLine(int lineNum, boost::string_ref str)
    {
        printLine(lineNum, str, layout.getRightNumWidth(), 0);
    }

    // Prints blank line of the left part.
    void printLeftBlank(int lineNum)
    {
        printBlank(lineNum, layout.getLeftNumWidth(), layout.getMaxLeftWidth());
    }

    // Prints blank line of the right part.
    void printRightBlank(int lineNum)
    {
        printBlank(lineNum, layout.getRightNumWidth(),
                   layout.getMaxRightWidth());
    }

    // Prints marker between two parts.
    void printMarker(char marker)
    {
        if (layout.isLeftVisible()) {
            os << ' ';
        }
        os << marker;
        if (layout.isRightVisible()) {
            os << ' ';
        }
    }

    // Advances to next line of parts.
    void nextLine()
    {
        os << '\n';
    }

private:
    // Formats and prints single line with its line number.
    void printLine(int lineNum, boost::string_ref str,
                   int numColWidth, int width)
    {
        os << (lineNo << std::right << std::setw(numColWidth) << lineNum << ' ')
           << ' ' << std::left << std::setw(width) << str;
    }

    // Formats and prints single blank line with its line number.
    void printBlank(int lineNum, int numColWidth, int width)
    {
        using namespace decor::literals;

        os << (lineNo << std::right << std::setw(numColWidth + 1)
                      << (noLineMarker(lineNum) + ' '))
           << ' ' << std::left << std::setw(width)
           << (235_bg << "");
    }

    // Generates string that should be used instead of line number.  Takes line
    // number of the last displayed line (0 if none was printed) as argument.
    static std::string noLineMarker(int at)
    {
        return std::string(countWidth(at), '-');
    }

private:
    std::ostream &os;        // Output stream.
    const Layout &layout;    // Computed layout.
    std::string leftMarker;  // Left marker for headers.
    std::string rightMarker; // Right marker for headers.

    // Line number style.
    decor::Decoration lineNo = ColorScheme()[ColorGroup::LineNo];
    // Next line index of the left part (needed to correct maximum width).
    int leftWidthIndex = 0;
};

}

Printer::Printer(const Node &left, const Node &right, const Language &lang,
                 std::ostream &os)
    : left(left), right(right), lang(lang), os(os)
{
}

Printer::Printer(const Node &left, std::vector<std::string> &&leftAnnots,
                 const Node &right, std::vector<std::string> &&rightAnnots,
                 const Language &lang, std::ostream &os)
    : left(left), right(right),
      leftAnnots(std::move(leftAnnots)), rightAnnots(std::move(rightAnnots)),
      lang(lang), os(os)
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
    auto diffingTimer = tr.measure("printing");

    // Do comparison without highlighting as it skews alignment results.
    DiffSource lsrc = (tr.measure("left-print"), DiffSource(left));
    DiffSource rsrc = (tr.measure("right-print"), DiffSource(right));
    std::vector<DiffLine> diff = (tr.measure("compare"),
                                  makeDiff(std::move(lsrc), std::move(rsrc)));

    auto timer = tr.measure("printing");

    TermHighlighter lh(left, lang, true);
    TermHighlighter rh(right, lang, false);
    std::vector<std::string> l(lsrc.lines.size());
    std::vector<std::string> r(rsrc.lines.size());

    LayoutBuilder layoutBuilder(lsrc, rsrc, headers);

    auto annotate = [](std::string &str,
                       const std::vector<std::string> &annots,
                       std::size_t index) {
        if (index < annots.size()) {
            str.insert(str.begin(),
                       annots[index].cbegin(), annots[index].cend());
        }
    };

    unsigned int i = 0U, j = 0U;
    for (DiffLine d : diff) {
        if (d.type == Diff::Fold) {
            i += d.data;
            j += d.data;
            continue;
        }

        if (d.type != Diff::Right) {
            if (layoutBuilder.isLeftVisible()) {
                l[i] = lh.print(i + 1, 1);
                annotate(l[i], leftAnnots, i);
            }
            layoutBuilder.measureLeft(l[i++]);
        }
        if (d.type != Diff::Left) {
            if (layoutBuilder.isRightVisible()) {
                r[j] = rh.print(j + 1, 1);
                annotate(r[j], rightAnnots, j);
            }
            layoutBuilder.measureRight(r[j++]);
        }

        // Record last non-folded indices.
        layoutBuilder.setMaxLineNums(i, j);
    }

    Layout layout = layoutBuilder.compute();
    Outliner outliner(os, layout);

    outliner.printSeparator();
    for (const Header &hdr : headers) {
        outliner.printHeader(hdr);
    }
    outliner.printSeparator();

    i = 0U;
    j = 0U;
    for (DiffLine d : diff) {
        boost::string_ref ll, rl;
        char marker = '?';

        switch (d.type) {
            case Diff::Left:      ll = l[i++];              marker = '<'; break;
            case Diff::Right:                  rl = r[j++]; marker = '>'; break;
            case Diff::Identical: ll = l[i++]; rl = r[j++]; marker = '|'; break;
            case Diff::Different: ll = l[i++]; rl = r[j++]; marker = '~'; break;

            case Diff::Fold:
                i += d.data;
                j += d.data;
                outliner.printMsg(" @@ folded " + std::to_string(d.data) +
                                  " identical lines @@ ");
                continue;
        }

        if ((!layout.isLeftVisible() && d.type == Diff::Left) ||
            (!layout.isRightVisible() && d.type == Diff::Right))
        {
            // Skip blank lines if only one version is displayed.
            continue;
        }

        if (layout.isLeftVisible()) {
            if (d.type == Diff::Right) {
                outliner.printLeftBlank(i);
            } else {
                outliner.printLeftLine(i, ll);
            }
        }

        outliner.printMarker(marker);

        if (layout.isRightVisible()) {
            if (d.type == Diff::Left) {
                outliner.printRightBlank(j);
            } else {
                outliner.printRightLine(j, rl);
            }
        }

        outliner.nextLine();
    }
}
