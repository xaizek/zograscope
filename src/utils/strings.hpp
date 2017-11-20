#ifndef UTILS__STRINGS__HPP__
#define UTILS__STRINGS__HPP__

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/utility/string_ref.hpp>

#include <vector>

class DiceString
{
public:
    DiceString(boost::string_ref s) : s(s)
    {
    }

public:
    float compare(DiceString &other);

    boost::string_ref str() const
    {
        return s;
    }

private:
    const std::vector<short> & getBigrams();

private:
    boost::string_ref s;
    std::vector<short> bigrams;
};

/**
 * @brief Splits string in a range-for loop friendly way.
 *
 * @param str String to split into substrings.
 * @param with Character to split at.
 *
 * @returns Array of results, empty on empty string.
 */
inline std::vector<boost::string_ref>
split(boost::string_ref str, char with)
{
    class string_ref : public boost::string_ref
    {
    public:
        string_ref(const char *begin, const char *end)
            : boost::string_ref(begin, end - begin)
        {
        }
    };

    if (str.empty()) {
        return {};
    }

    std::vector<string_ref> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return { results.cbegin(), results.cend() };
}

#endif // UTILS__STRINGS__HPP__
