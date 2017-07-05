#ifndef PARSER_HPP__
#define PARSER_HPP__

#include <string>

class TreeBuilder;

TreeBuilder parse(const std::string &contents, bool debug = false);

#endif // PARSER_HPP__
