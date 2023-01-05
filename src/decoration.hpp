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

#ifndef ZOGRASCOPE_DECORATION_HPP_
#define ZOGRASCOPE_DECORATION_HPP_

#include <functional>
#include <iosfwd>
#include <memory>
#include <type_traits>
#include <vector>

#include <boost/variant.hpp>

class Tests;

/**
 * @brief Terminal control manipulators for output streams.
 *
 * Usage example:
 * @code
 * std::cout << decor::bold << "This is bold text." << decor::def;
 * @endcode
 *
 * Equivalent example with scope:
 * @code
 * std::cout << (decor::bold << "This is bold text.");
 * @endcode
 */
namespace decor {

/**
 * @brief Type of argument passed to extra decorators.
 */
using arg_t = boost::variant<int, std::string>;

/**
 * @brief Class describing single decoration or a combination of them.
 */
class Decoration
{
    using decorFunc = std::ostream & (*)(std::ostream &os);
    using extDecorFunc = std::ostream & (*)(std::ostream &os, arg_t arg);

public:
    /**
     * @brief Constructs possibly empty decoration.
     *
     * @param decorator Decorating function.
     */
    explicit Decoration(decorFunc decorator = nullptr);
    /**
     * @brief Constructs decoration from an unary function.
     *
     * @param extDecorator Decorating function.
     * @param arg Parameter for decorating function.
     */
    Decoration(extDecorFunc extDecorator, arg_t arg);
    /**
     * @brief Constructs a (deep) copy of a decoration.
     *
     * @param rhs Original object to be copied.
     */
    Decoration(const Decoration &rhs);
    /**
     * @brief Constructs a decoration that is a combination of others.
     *
     * @param lhs First decoration to combine.
     * @param rhs Second decoration to combine.
     */
    Decoration(const Decoration &lhs, const Decoration &rhs);
    /**
     * @brief Defaulted move constructor.
     *
     * @param rhs Object to move from.
     */
    Decoration(Decoration &&rhs) = default;

public:
    /**
     * @brief Assigns one decoration to another.
     *
     * @param rhs Assignment operand.
     *
     * @returns @c *this.
     */
    Decoration & operator=(const Decoration &rhs);

    /**
     * @brief Defaulted move assignment operator.
     *
     * @param rhs Assignment operand.
     *
     * @returns @c *this.
     */
    Decoration & operator=(Decoration &&rhs) = default;

    /**
     * @brief Actually performs the decoration of a stream.
     *
     * @param os Stream to decorate.
     *
     * @returns @p os.
     */
    std::ostream & decorate(std::ostream &os) const;

    /**
     * @brief Whether this decoration does nothing.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isEmpty() const;

    /**
     * @brief Sets prefix decoration.
     *
     * @param dec Prefix decoraiton.
     *
     * @returns @c *this.
     */
    Decoration & prefix(Decoration dec);

    /**
     * @brief Sets suffix decoration.
     *
     * @param dec Prefix decoraiton.
     *
     * @returns @c *this.
     */
    Decoration & suffix(Decoration dec);

    /**
     * @brief Retrieves prefix decoration.
     *
     * @returns The decoration or @c nullptr.
     */
    const Decoration * getPrefix() const;

    /**
     * @brief Retrieves suffix decoration.
     *
     * @returns The decoration or @c nullptr.
     */
    const Decoration * getSuffix() const;

private:
    /**
     * @brief Decoration function (can be nullptr).
     */
    decorFunc decorator = nullptr;
    /**
     * @brief Unary decoration function (can be nullptr).
     */
    extDecorFunc extDecorator = nullptr;
    /**
     * @brief Parameter for extDecorator.
     */
    arg_t arg;
    /**
     * @brief One of two decorations that compose this object.
     */
    std::unique_ptr<Decoration> lhs;
    /**
     * @brief Second decoration that composes this object.
     */
    std::unique_ptr<Decoration> rhs;
    /**
     * @brief Prefix decoration (used by ScopedDecoration).
     */
    std::unique_ptr<Decoration> pfx;
    /**
     * @brief Suffix decoration (used by ScopedDecoration).
     */
    std::unique_ptr<Decoration> sfx;
};

/**
 * @brief Combines two decorations to create a new one.
 *
 * @param lhs First decoration to combine.
 * @param rhs Second decoration to combine.
 *
 * @returns The combination.
 */
inline Decoration
operator+(const Decoration &lhs, const Decoration &rhs)
{
    return Decoration(lhs, rhs);
}

/**
 * @brief A thunk for decoration use with @c std::ostream.
 *
 * @param os Stream to output to.
 * @param d Decoration to apply.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const Decoration &d)
{
    return d.decorate(os);
}

/**
 * @brief A scoped decoration container.
 *
 * Collects data that should be output to a stream and later replays it.
 */
class ScopedDecoration
{
public:
    /**
     * @brief Constructs an object from decoration and stream action.
     *
     * @param decoration Decoration of the scope.
     * @param app First application in the scope.
     */
    ScopedDecoration(const Decoration &decoration,
                     std::function<void(std::ostream&)> &&app)
        : decoration(decoration)
    {
        apps.reserve(1U);
        apps.emplace_back(std::move(app));
    }
    /**
     * @brief Constructs an object from scope and additional stream action.
     *
     * @param scoped Pre-existing scoped decoration which is being extended.
     * @param app Additional application in the scope.
     */
    ScopedDecoration(ScopedDecoration &&scoped,
                     std::function<void(std::ostream&)> &&app)
        : decoration(scoped.decoration), apps(std::move(scoped.apps))
    {
        apps.reserve(apps.size() + 1U);
        apps.emplace_back(std::move(app));
    }

public:
    /**
     * @brief Actually performs the decoration of a stream.
     *
     * @param os Stream to decorate.
     *
     * @returns @p os.
     */
    std::ostream & decorate(std::ostream &os) const;

private:
    /**
     * @brief Decoration object of this scope.
     */
    const Decoration &decoration;
    /**
     * @brief Stream actions in this scope.
     */
    std::vector<std::function<void(std::ostream&)>> apps;
};

/**
 * @brief Constructs scoped decoration.
 *
 * @tparam T Type of stream action.
 * @param d Decoration of the scope.
 * @param v Stream action.
 *
 * @returns Scoped decoration constructed.
 */
template <typename T>
typename std::enable_if<std::is_function<T>::value, ScopedDecoration>::type
operator<<(const Decoration &d, const T &v)
{
    const T *val = v;
    return ScopedDecoration(d, [val](std::ostream &os) { os << val; });
}

/**
 * @brief Constructs scoped decoration.
 *
 * @tparam T Type of stream data.
 * @param d Decoration of the scope.
 * @param v Stream data.
 *
 * @returns Scoped decoration constructed.
 */
template <typename T>
typename std::enable_if<!std::is_function<T>::value, ScopedDecoration>::type
operator<<(const Decoration &d, const T &val)
{
    return ScopedDecoration(d, [val](std::ostream &os) { os << val; });
}

/**
 * @brief Constructs scoped decoration.
 *
 * @tparam T Type of stream data.
 * @param d Decoration of the scope.
 * @param v Stream data.
 *
 * @returns Scoped decoration constructed.
 */
template <typename T>
typename std::enable_if<!std::is_function<T>::value, ScopedDecoration>::type
operator<<(Decoration &&d, const T &val)
{
    return ScopedDecoration(std::move(d),
                            [val](std::ostream &os) { os << val; });
}

/**
 * @brief Appends to scoped decoration.
 *
 * @tparam T Type of stream data.
 * @param sd Scoped decoration.
 * @param v Stream data.
 *
 * @returns Scoped decoration constructed.
 */
template <typename T>
ScopedDecoration
operator<<(ScopedDecoration &&sd, const T &val)
{
    return ScopedDecoration(std::move(sd),
                            [val](std::ostream &os) { os << val; });
}

/**
 * @brief A thunk for scoped decoration use with @c std::ostream.
 *
 * @param os Stream to output to.
 * @param sd Scoped decoration to apply.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const ScopedDecoration &sd)
{
    return sd.decorate(os);
}

/**
 * @{
 * @name Generic attributes
 */

/**
 * @brief Convenient attribute that does nothing.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration none;
/**
 * @brief Enables bold attribute.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration bold;
/**
 * @brief Enables color inversion attribute.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration inv;
/**
 * @brief Restores default attribute of the stream.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration def;

/**
 * @}
 *
 * @{
 * @name Foreground colors
 */

/**
 * @brief Picks black as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration black_fg;
/**
 * @brief Picks red as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration red_fg;
/**
 * @brief Picks green as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration green_fg;
/**
 * @brief Picks yellow as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration yellow_fg;
/**
 * @brief Picks blue as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration blue_fg;
/**
 * @brief Picks magenta as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration magenta_fg;
/**
 * @brief Picks cyan as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration cyan_fg;
/**
 * @brief Picks white as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration white_fg;

/**
 * @}
 *
 * @{
 * @name Background colors
 */

/**
 * @brief Picks black as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration black_bg;
/**
 * @brief Picks red as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration red_bg;
/**
 * @brief Picks green as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration green_bg;
/**
 * @brief Picks yellow as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration yellow_bg;
/**
 * @brief Picks blue as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration blue_bg;
/**
 * @brief Picks magenta as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration magenta_bg;
/**
 * @brief Picks cyan as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration cyan_bg;
/**
 * @brief Picks white as foreground color.
 *
 * @param os Stream to operate on.
 *
 * @returns @p os
 */
extern const Decoration white_bg;

/**
 * @}
 *
 * @{
 * @name Control
 */

/**
 * @brief Forces enabled state of decorations.
 */
void enableDecorations();

/**
 * @brief Forces disabled state of decorations.
 */
void disableDecorations();

/**
 * @}
 */

// TODO: document this
namespace literals {

    std::ostream & fg256(std::ostream &os, arg_t arg);
    std::ostream & bg256(std::ostream &os, arg_t arg);

    std::ostream & lit(std::ostream &os, arg_t arg);

    inline Decoration operator""_fg(unsigned long long n)
    {
        return Decoration(&fg256, n);
    }

    inline Decoration operator""_bg(unsigned long long n)
    {
        return Decoration(&bg256, n);
    }

    inline Decoration operator""_lit(const char s[], std::size_t)
    {
        return Decoration(&lit, s);
    }

}

}

#endif // ZOGRASCOPE_DECORATION_HPP_
