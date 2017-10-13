#ifndef HIGHLIGHTER_HPP__
#define HIGHLIGHTER_HPP__

#include <string>

class Node;

class Highlighter
{
public:
    Highlighter(bool original = true);

public:
    std::string print(Node &root) const;

private:
    const bool original;
};

#endif // HIGHLIGHTER_HPP__
