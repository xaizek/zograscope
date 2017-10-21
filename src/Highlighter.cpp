#include "Highlighter.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include "dtl/dtl.hpp"

#include "decoration.hpp"
#include "stypes.hpp"
#include "tree.hpp"
#include "utils.hpp"

static decor::Decoration getHighlight(const Node &node);
static bool isDiffable(const Node &node);
static std::string diffSpelling(const std::string &l, const std::string &r,
                             const decor::Decoration &dec, bool original);
static std::vector<boost::string_ref> toWords(const std::string &s);
static std::string getSpelling(const Node &node, const decor::Decoration &dec,
                               bool original);

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

Highlighter::Highlighter(bool original) : original(original)
{
}

std::string
Highlighter::print(Node &root) const
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

            const std::string spelling = getSpelling(node, dec, original);
            const std::vector<boost::string_ref> lines = split(spelling, '\n');
            oss << (dec << lines.front());
            col += lines.front().size();

            for (std::size_t i = 1U; i < lines.size(); ++i) {
                std::size_t whitespaceLength =
                    lines[i].find_first_not_of(" \t");
                if (whitespaceLength == std::string::npos) {
                    whitespaceLength = lines[i].size();
                }
                oss << '\n' << lines[i].substr(0, whitespaceLength)
                            << (dec << lines[i].substr(whitespaceLength));
                ++line;
                col = 1 + lines[i].size();
                colorPicker.advancedLine();
            }
        }

        for (Node *child : node.children) {
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

        case State::Deleted:
            return (210_fg + inv + black_bg + bold)
                   .prefix("{-"_lit)
                   .suffix("-}"_lit);
        case State::Inserted:
            return (85_fg + inv + black_bg + bold)
                   .prefix("{+"_lit)
                   .suffix("+}"_lit);
        case State::Updated:
            // TODO: things that were updated and moved at the same time need a
            //       special color?
            //       Or update kinda implies move, since it's obvious that there
            //       was a match between nodes?
            if (!isDiffable(node)) {
                return (228_fg + inv + black_bg + bold)
                       .prefix("{#"_lit)
                       .suffix("#}"_lit);
            }
            break;

        case State::Unchanged:
            if (node.moved) {
                return (81_fg + inv + bold)
                       .prefix("{:"_lit)
                       .suffix(":}"_lit);
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
        case Type::Functions:  return  81_fg;

        case Type::Jumps:
        case Type::Keywords:
            return 115_fg;
        case Type::LeftBrackets:
        case Type::RightBrackets:
            return 222_fg;
        case Type::Assignments:
        case Type::Operators:
        case Type::LogicalOperators:
        case Type::Comparisons:
            return 224_fg;
        case Type::StrConstants:
        case Type::IntConstants:
        case Type::FPConstants:
        case Type::CharConstants:
            return 219_fg;

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
 * @brief Checks whether node spelling can be diffed.
 *
 * @param node Node to check.
 *
 * @returns @c true if so, @c false otherwise.
 */
static bool
isDiffable(const Node &node)
{
    return node.relative != nullptr
        && (node.stype == SType::Comment || node.type == Type::StrConstants)
        && node.state == State::Updated;
}

/**
 * @brief Diffs two labels.
 *
 * @param l        Original label.
 * @param r        Updated label.
 * @param dec      Default decoration.
 * @param original Whether this is original source.
 *
 * @returns Differed label.
 */
static std::string
diffSpelling(const std::string &l, const std::string &r,
             const decor::Decoration &dec, bool original)
{
    // XXX: some kind of caching would be nice.

    using namespace decor;
    using namespace decor::literals;

    std::vector<boost::string_ref> lWords = toWords(l);
    std::vector<boost::string_ref> rWords = toWords(r);

    auto cmp = [](const boost::string_ref &a, const boost::string_ref &b) {
        return (a == b);
    };

    dtl::Diff<boost::string_ref, std::vector<boost::string_ref>,
              decltype(cmp)> diff(lWords, rWords, cmp);
    diff.compose();

    std::ostringstream oss;

    Decoration deleted = (210_fg + inv + black_bg + bold)
                         .prefix("{-"_lit)
                         .suffix("-}"_lit);
    Decoration inserted = (85_fg + inv + black_bg + bold)
                          .prefix("{+"_lit)
                          .suffix("+}"_lit);

    const char *lastL = l.data(), *lastR = r.data();

    // FIXME: could do a better job than colorizing each character.

    auto printLeft = [&](const dtl::elemInfo &info, const Decoration &dec) {
        const boost::string_ref sr = lWords[info.beforeIdx - 1];
        oss << boost::string_ref(lastL, sr.data() - lastL);
        lastL = sr.data() + sr.size();
        oss << (dec << sr);
    };
    auto printRight = [&](const dtl::elemInfo &info, const Decoration &dec) {
        const boost::string_ref sr = rWords[info.afterIdx - 1];
        oss << boost::string_ref(lastR, sr.data() - lastR);
        lastR = sr.data() + sr.size();
        oss << (dec << sr);
    };

    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            case dtl::SES_DELETE:
                if (original) {
                    printLeft(x.second, deleted);
                }
                break;
            case dtl::SES_ADD:
                if (!original) {
                    printRight(x.second, inserted);
                }
                break;
            case dtl::SES_COMMON:
                if (original) {
                    printLeft(x.second, dec);
                } else {
                    printRight(x.second, dec);
                }
                break;
        }
    }

    oss << (dec << (original ? lastL : lastR));

    return oss.str();
}

/**
 * @brief Breaks a string into words.
 *
 * @param s Initial multi-word string.
 *
 * @returns Collection of words.
 */
static std::vector<boost::string_ref>
toWords(const std::string &s)
{
    std::vector<boost::string_ref> words;
    boost::string_ref sr(s);

    bool inWord = false;
    std::size_t wordStart;
    for (std::size_t i = 0U; i <= s.size(); ++i) {
        const bool isWordChar = s[i] != '\0'
                             && !std::isspace(s[i], std::locale());
        if (isWordChar != inWord) {
            if (isWordChar) {
                wordStart = i;
            } else {
                words.emplace_back(sr.substr(wordStart, i - wordStart));
            }
        }
        inWord = isWordChar;
    }

    return words;
}

/**
 * @brief Formats spelling for a node.
 *
 * @param node     Source of the spelling.
 * @param dec      Default decoration.
 * @param original Whether this is original source.
 *
 * @returns Formatted spelling.
 */
static std::string
getSpelling(const Node &node, const decor::Decoration &dec, bool original)
{
    if (!isDiffable(node)) {
        return node.spelling;
    }

    return original
         ? diffSpelling(node.spelling, node.relative->spelling, dec, original)
         : diffSpelling(node.relative->spelling, node.spelling, dec, original);
}
