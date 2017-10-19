#ifndef PRINTER_HPP__
#define PRINTER_HPP__

#include <iosfwd>
#include <string>
#include <vector>

class Node;
class TimeReport;

class Printer
{
    struct Header
    {
        std::string left;
        std::string right;
    };

public:
    Printer(Node &left, Node &right, std::ostream &os);

public:
    void addHeader(Header header);
    void print(TimeReport &tr);

private:
    Node &left;
    Node &right;
    std::ostream &os;
    std::vector<Header> headers;
};

#endif // PRINTER_HPP__
