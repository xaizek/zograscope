#ifndef TESTS__TESTS_HPP__
#define TESTS__TESTS_HPP__

#include <string>

class Node;
class Tree;

enum class Type;

Tree makeTree(const std::string &str, bool stree = false);

const Node * findNode(const Tree &tree, Type type,
                      const std::string &label = {});

#endif // TESTS__TESTS_HPP__
