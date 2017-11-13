#ifndef LEXERDATA_HPP__
#define LEXERDATA_HPP__

#include <cstddef>
#include <cstring>

#include <string>

#include "c11-parser.hpp"

class TreeBuilder;

struct LexerData
{
    enum { tabWidth = 4 };

    std::size_t offset = 0U;
    std::size_t lineoffset = 0U;
    std::size_t line = 1U;
    std::size_t col = 1U;

    YYSTYPE startTok = {};
    YYLTYPE startLoc = {};

    TreeBuilder *tb;

    explicit LexerData(const std::string &str, TreeBuilder &tb)
        : tb(&tb), next(str.data()), finish(str.data() + str.size())
    {
    }

    std::size_t readInput(char buf[], std::size_t maxSize);

private:
    const char *next;
    const char *finish;
};

#endif // LEXERDATA_HPP__
