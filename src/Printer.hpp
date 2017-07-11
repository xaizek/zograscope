#ifndef PRINTER_HPP__
#define PRINTER_HPP__

#include <string>
#include <vector>

class Node;

class Printer
{
    struct Header
    {
        std::string left;
        std::string right;
    };

public:
    Printer(Node &left, Node &right);

public:
    void addHeader(Header header);
    void print();

private:
    Node &left;
    Node &right;
    std::vector<Header> headers;
};

std::string printSource(Node &root);

#endif // PRINTER_HPP__
