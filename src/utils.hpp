#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

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
inline std::vector<std::string>
split(const std::string &str, char with)
{
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return results;
}

#endif // UTILS_HPP__
