#include "LexerData.hpp"

#include <cstddef>
#include <cstring>

#include <algorithm>

#include "c/c11-parser.hpp"

std::size_t
LexerData::readInput(char buf[], std::size_t maxSize)
{
    static const char *const trailing = "\n";

    if (next == nullptr) {
        return 0U;
    }

    std::size_t count = std::min<std::size_t>(finish - next, maxSize);
    char *end = std::copy_n(next, count, buf);
    const std::size_t copied = end - buf;

    if (next[copied] == '\0') {
        next = (next == trailing ? nullptr : trailing);
        finish = (next == nullptr ? nullptr : next + std::strlen(next));
    } else {
        next += copied;
    }

    return copied;
}
