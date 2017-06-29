#include "utils.hpp"

#include <iterator>
#include <string>
#include <utility>

class CountIterator :
    public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    CountIterator & operator*() { return *this; }
    CountIterator & operator++() { return *this; }
    CountIterator & operator++(int) { return *this; }

    template <typename T>
    CountIterator & operator=(const T &)
    {
        ++count;
        return *this;
    }

    int getCount() const { return count; }

private:
    int count = 0;
};

DiceString::DiceString(std::string s) : s(std::move(s))
{
}

float
DiceString::compare(DiceString &other)
{
    if (s.length() < 2U && other.s.length() < 2U) {
        return (s == other.s) ? 1.0f : 0.0f;
    }
    if (s.length() < 2U || other.s.length() < 2U) {
        return 0.0f;
    }

    const std::vector<short> &bigrams = getBigrams();
    const std::vector<short> &otherBigrams = other.getBigrams();
    const int common = std::set_intersection(bigrams.cbegin(), bigrams.cend(),
                                             otherBigrams.cbegin(),
                                             otherBigrams.cend(),
                                             CountIterator())
                      .getCount();

    return (2.0f*common)/(bigrams.size() + otherBigrams.size());
}

const std::vector<short> &
DiceString::getBigrams()
{
    if (bigrams.empty() && s.length() >= 2U) {
        bigrams.reserve(s.length() - 1U);
        for (std::size_t i = 0U; i < s.length() - 1U; ++i) {
            const int bigram = (static_cast<unsigned char>(s[i]) << CHAR_BIT)
                             | static_cast<unsigned char>(s[i + 1U]);
            bigrams.push_back(bigram);
        }
        std::sort(bigrams.begin(), bigrams.end());
        bigrams.erase(std::unique(bigrams.begin(), bigrams.end()),
                      bigrams.end());
    }

    return bigrams;
}
