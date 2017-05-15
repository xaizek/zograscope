#ifndef PRINTER_HPP__
#define PRINTER_HPP__

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

#endif // PRINTER_HPP__
