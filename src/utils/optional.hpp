#ifndef UTILS__OPTIONAL_HPP__
#define UTILS__OPTIONAL_HPP__

#include <utility>

#include <boost/optional/optional_fwd.hpp>

template <typename T>
class MoveOnCopy
{
public:
    MoveOnCopy(T &&movable) : movable(std::move(movable))
    {
    }

    MoveOnCopy(MoveOnCopy &rhs) : movable(std::move(rhs.movable))
    {
    }

public:
    operator T&&() const
    {
        return std::move(movable);
    }

private:
    mutable T movable;
};

template <typename T>
using optional_t = boost::optional<MoveOnCopy<T>>;

#endif // UTILS__OPTIONAL_HPP__
