#ifndef PRINTER_HPP__
#define PRINTER_HPP__

#include <string>

class Node;

class Printer
{
public:
    Printer(Node &left, Node &right);

public:
    void print();

private:
    Node &left;
    Node &right;
};

std::string printSource(Node &root);

#endif // PRINTER_HPP__
