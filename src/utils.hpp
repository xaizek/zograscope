#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/utility/string_ref.hpp>

#include <string>
#include <vector>

class DiceString
{
public:
    DiceString(std::string s);

public:
    float compare(DiceString &other);

    const std::string & str() const { return s; }

private:
    const std::vector<short> & getBigrams();

private:
    std::string s;
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

#endif // UTILS_HPP__
