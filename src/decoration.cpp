#include "decoration.hpp"

#include <unistd.h>

#include <cstdio>

#include <ostream>

namespace {

/**
 * @brief Actual implementation of color attributes.
 */
class Colors
{
public:
    /**
     * @brief Constructs the class checking whether stdout is a terminal.
     */
    void disable() { isAscii = false; }

    const char * bold () { return isAscii ? "\033[1m" : ""; }
    const char * inv  () { return isAscii ? "\033[7m" : ""; }
    const char * def  () { return isAscii ? "\033[1m\033[0m" : ""; }

    const char * black_fg   () { return isAscii ? "\033[30m" : ""; }
    const char * red_fg     () { return isAscii ? "\033[31m" : ""; }
    const char * green_fg   () { return isAscii ? "\033[32m" : ""; }
    const char * yellow_fg  () { return isAscii ? "\033[33m" : ""; }
    const char * blue_fg    () { return isAscii ? "\033[34m" : ""; }
    const char * magenta_fg () { return isAscii ? "\033[35m" : ""; }
    const char * cyan_fg    () { return isAscii ? "\033[36m" : ""; }
    const char * white_fg   () { return isAscii ? "\033[37m" : ""; }

    const char * black_bg   () { return isAscii ? "\033[40m" : ""; }
    const char * red_bg     () { return isAscii ? "\033[41m" : ""; }
    const char * green_bg   () { return isAscii ? "\033[42m" : ""; }
    const char * yellow_bg  () { return isAscii ? "\033[43m" : ""; }
    const char * blue_bg    () { return isAscii ? "\033[44m" : ""; }
    const char * magenta_bg () { return isAscii ? "\033[45m" : ""; }
    const char * cyan_bg    () { return isAscii ? "\033[46m" : ""; }
    const char * white_bg   () { return isAscii ? "\033[47m" : ""; }

    std::string fg(int n)
    {
        return isAscii ? "\033[38;5;" + std::to_string(n) + "m" : "";
    }
    std::string bg(int n)
    {
        return isAscii ? "\033[48;5;" + std::to_string(n) + "m" : "";
    }

private:
    /**
     * @brief Whether outputting of ASCII escape sequences is enabled.
     */
    bool isAscii = isatty(fileno(stdout));
} C;

}

// Shorten type name to fit into 80 columns limit.
using ostr = std::ostream;

using namespace decor;

const Decoration
    decor::none,
    decor::bold       ([](ostr &os) -> ostr & { return os << C.bold();       }),
    decor::inv        ([](ostr &os) -> ostr & { return os << C.inv();        }),
    decor::def        ([](ostr &os) -> ostr & { return os << C.def();        }),

    decor::black_fg   ([](ostr &os) -> ostr & { return os << C.black_fg();   }),
    decor::red_fg     ([](ostr &os) -> ostr & { return os << C.red_fg();     }),
    decor::green_fg   ([](ostr &os) -> ostr & { return os << C.green_fg();   }),
    decor::yellow_fg  ([](ostr &os) -> ostr & { return os << C.yellow_fg();  }),
    decor::blue_fg    ([](ostr &os) -> ostr & { return os << C.blue_fg();    }),
    decor::magenta_fg ([](ostr &os) -> ostr & { return os << C.magenta_fg(); }),
    decor::cyan_fg    ([](ostr &os) -> ostr & { return os << C.cyan_fg();    }),
    decor::white_fg   ([](ostr &os) -> ostr & { return os << C.white_fg();   }),

    decor::black_bg   ([](ostr &os) -> ostr & { return os << C.black_bg();   }),
    decor::red_bg     ([](ostr &os) -> ostr & { return os << C.red_bg();     }),
    decor::green_bg   ([](ostr &os) -> ostr & { return os << C.green_bg();   }),
    decor::yellow_bg  ([](ostr &os) -> ostr & { return os << C.yellow_bg();  }),
    decor::blue_bg    ([](ostr &os) -> ostr & { return os << C.blue_bg();    }),
    decor::magenta_bg ([](ostr &os) -> ostr & { return os << C.magenta_bg(); }),
    decor::cyan_bg    ([](ostr &os) -> ostr & { return os << C.cyan_bg();    }),
    decor::white_bg   ([](ostr &os) -> ostr & { return os << C.white_bg();   });

Decoration::Decoration(const Decoration &rhs)
    : decorator(rhs.decorator), intDecorator(rhs.intDecorator), n(rhs.n),
      lhs(rhs.lhs == nullptr ? nullptr : new Decoration(*rhs.lhs)),
      rhs(rhs.rhs == nullptr ? nullptr : new Decoration(*rhs.rhs))
{
}

Decoration::Decoration(decorFunc decorator) : decorator(decorator)
{
}

Decoration::Decoration(intDecorFunc intDecorator, int n)
    : intDecorator(intDecorator), n(n)
{
}

Decoration::Decoration(const Decoration &lhs, const Decoration &rhs)
    : lhs(new Decoration(lhs)),
      rhs(new Decoration(rhs))
{
}

Decoration &
Decoration::operator=(const Decoration &rhs)
{
    Decoration copy(rhs);
    std::swap(copy.decorator, this->decorator);
    std::swap(copy.intDecorator, this->intDecorator);
    std::swap(copy.n, this->n);
    std::swap(copy.lhs, this->lhs);
    std::swap(copy.rhs, this->rhs);
    return *this;
}

std::ostream &
Decoration::decorate(std::ostream &os) const
{
    if (decorator != nullptr || intDecorator != nullptr) {
        // Reset and preserve width field, so printing escape sequence doesn't
        // mess up formatting.
        const auto width = os.width({});

        if (decorator != nullptr) {
            decorator(os);
        } else {
            intDecorator(os, n);
        }

        static_cast<void>(os.width(width));

        return os;
    }
    if (lhs != nullptr && rhs != nullptr) {
        return os << *lhs << *rhs;
    }
    return os;
}

std::ostream &
ScopedDecoration::decorate(std::ostream &os) const
{
    os << decoration;
    for (const auto app : apps) {
        app(os);
    }
    os << def;
    return os;
}

void
decor::disableDecorations()
{
    C.disable();
}

std::ostream & literals::fg256(std::ostream &os, int n)
{
    return os << C.fg(n);
}

std::ostream & literals::bg256(std::ostream &os, int n)
{
    return os << C.bg(n);
}
