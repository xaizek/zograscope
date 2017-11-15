#ifndef PARSER_HPP__
#define PARSER_HPP__

#include <string>

#include "pmr/polymorphic_allocator.hpp"

class TreeBuilder;

TreeBuilder parse(const std::string &contents,
                  const std::string &fileName = "<input>",
                  bool debug = false,
                  cpp17::pmr::polymorphic_allocator<cpp17::byte> al = {});

#endif // PARSER_HPP__
