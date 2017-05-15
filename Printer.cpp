#include "Printer.hpp"

#include <functional>
#include <iostream>

#include "decoration.hpp"
#include "tree-edit-distance.hpp"

static void printSource(Node &root);

Printer::Printer(Node &left, Node &right) : left(left), right(right)
{
}

void
Printer::print()
{
    std::cout << "---------------\n";
    printSource(left);
    std::cout << "---------------\n";
    std::cout << "---------------\n";
    printSource(right);
    std::cout << "---------------\n";
}

static void
printSource(Node &root)
{
    int line = 1, col = 1;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.line != 0 && node.col != 0) {
            while (node.line > line) {
                std::cout << '\n';
                ++line;
                col = 1;
            }

            while (node.col > col) {
                std::cout << ' ';
                ++col;
            }

            decor::Decoration dec;
            switch (node.state) {
                case State::Unchanged: break;
                case State::Deleted: dec = decor::red_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Inserted: dec = decor::green_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Updated: dec = decor::yellow_fg + decor::inv + decor::black_bg + decor::bold; break;
            }

            // if (node.state != State::Unchanged) {
            //     std::cout << (dec << '[') << node.label << (dec << ']');
            // } else {
            //     std::cout << node.label;
            // }
                std::cout << (dec << node.label);

            col += node.label.size();
        }

        for (Node &child : node.children) {
            if (child.satellite) {
                child.state = node.state;
            }
            visit(child);
        }
    };
    visit(root);

    if (col != 1) {
        std::cout << '\n';
    }
}

