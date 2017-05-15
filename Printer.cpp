#include "Printer.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "decoration.hpp"
#include "tree-edit-distance.hpp"

static unsigned int getTerminalWidth();
static std::vector<std::string> split(const std::string &str, char with);
static std::string printSource(Node &root);
static unsigned int measureWidth(const std::string &s);

Printer::Printer(Node &left, Node &right) : left(left), right(right)
{
}

void
Printer::print()
{
    static std::string empty;

    const unsigned int halfWidth = getTerminalWidth()/2 - 2;

    std::vector<std::string> l = split(printSource(left), '\n');
    std::vector<std::string> r = split(printSource(right), '\n');

    unsigned int maxWidth = 0U;
    std::vector<unsigned int> widths;
    widths.reserve(l.size());
    for (const std::string &ll : l) {
        const unsigned int width = measureWidth(ll);
        widths.push_back(width);

        maxWidth = std::max(width, maxWidth);
    }

    for (unsigned int i = 0U; i < l.size() || i < r.size(); ++i) {
        const std::string &ll = (i < l.size() ? l[i] : empty);
        const std::string &rl = (i < r.size() ? r[i] : empty);

        unsigned int width = (ll.empty() ? 0U : widths[i]);
        width = maxWidth + (ll.size() - width);
        std::cout << std::left << std::setw(width) << ll << " || " << rl << '\n';
    }
}

static unsigned int
getTerminalWidth()
{
    winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
        return std::numeric_limits<unsigned int>::max();
    }

    return ws.ws_col;
}

/**
 * @brief Splits string in a range-for loop friendly way.
 *
 * @param str String to split into substrings.
 * @param with Character to split at.
 *
 * @returns Array of results, empty on empty string.
 */
static std::vector<std::string>
split(const std::string &str, char with)
{
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return results;
}

static std::string
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

            decor::Decoration dec;
            switch (node.state) {
                case State::Unchanged: break;
                case State::Deleted: dec = decor::red_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Inserted: dec = decor::green_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Updated: dec = decor::yellow_fg + decor::inv + decor::black_bg + decor::bold; break;
            }

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
            if (child.satellite) {
                child.state = node.state;
            }
            visit(child);
        }
    };
    visit(root);

    return oss.str();
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

