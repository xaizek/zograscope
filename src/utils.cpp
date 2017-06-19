#include "utils.hpp"

#include <boost/functional/hash.hpp>

#include <array>
#include <string>
#include <unordered_set>

float
diceCoefficient(const std::string &a, const std::string &b)
{
    struct Hash
    {
        std::size_t operator()(const std::array<char, 2> s) const
        {
            std::size_t h = std::hash<char>()(s[0]);
            boost::hash_combine(h, std::hash<char>()(s[1]));
            return h;
        }
    };

    if (a.length() < 2U && b.length() < 2U) {
        return (a == b) ? 1.0f : 0.0f;
    }
    if (a.length() < 2U || b.length() < 2U) {
        return 0.0f;
    }

    std::unordered_set<std::array<char, 2>, Hash> ab;
    for (std::size_t i = 0U; i < a.length() - 1U; ++i) {
        ab.insert({ a[i], a[i + 1U] });
    }

    int common = 0;

    std::unordered_set<std::array<char, 2>, Hash> bb;
    for (std::size_t i = 0U; i < b.length() - 1U; ++i) {
        if (bb.insert({ b[i], b[i + 1U] }).second) {
            common += ab.count({ b[i], b[i + 1U] });
        }
    }

    return (2.0f*common)/(ab.size() + bb.size());
}
