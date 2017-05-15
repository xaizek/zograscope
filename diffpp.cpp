#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Printer.hpp"
#include "TreeBuilder.hpp"
#include "tree-edit-distance.hpp"
#include "types.hpp"

// TODO: try marking tokens with types and accounting for them on rename
// TODO: try using string edit distance on rename
// TODO: try using token streams to find edit distance
// TODO: try using token streams to construct common diff

// TODO: try changing tree structure to get better results (join block node with { and } nodes)
//       e.g. by not feeding terminal nodes of non-terminal nodes that have
//       non-terminal children to the differ and later propagating state of
//       parent nodes to such children
static void
markSatellites(Node &node)
{
    auto nonTerminal = [](const Node &node) {
        return node.line == 0 || node.col == 0 || !node.children.empty();
    };
    auto terminal = [&nonTerminal](const Node &node) {
        return !nonTerminal(node);
    };

    if (std::any_of(node.children.cbegin(), node.children.cend(), nonTerminal)
        || std::all_of(node.children.cbegin(), node.children.cend(), terminal)) {
        for (Node &child : node.children) {
            child.satellite = !nonTerminal(child);
            markSatellites(child);
        }
    }
}

std::string
readFile(const std::string &path)
{
    std::ifstream ifile(path);
    std::ostringstream iss;
    iss << ifile.rdbuf();
    return iss.str();
}

std::string
materializeLabel(const std::string &contents, const PNode *node)
{
    // lexer also has such variable and they need to be synchronized (actually
    // we should pass this to lexer)
    enum { tabWidth = 4 };

    std::string label;
    label.reserve(node->value.len);

    int col = node->col;
    for (std::size_t i = node->value.from;
         i < node->value.from + node->value.len; ++i) {
        switch (const char c = contents[i]) {
            int width;

            case '\n':
                col = 1;
                label += '\n';
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                label.append(width, ' ');
                col += width;
                break;

            default:
                ++col;
                label += c;
                break;
        }
    }

    return label;
}

Node
materializeTree(const std::string &contents, const PNode *node)
{
    Node n = { materializeLabel(contents, node) };
    n.line = node->line;
    n.col = node->col;
    n.type = tokenToType(node->value.token);

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializeTree(contents, child));
    }

    return n;
}

TreeBuilder parse(const std::string &contents);

int
main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        std::cerr << "Wrong arguments\n";
        return 1;
    }

    Node treeA, treeB;

    std::cout << ">>> Parsing " << argv[1] << "\n";
    {
        std::string contents = readFile(argv[1]);
        TreeBuilder tb = parse(contents);
        treeA = materializeTree(contents, tb.getRoot());
    }

    std::cout << ">>> Parsing " << argv[2] << "\n";
    {
        std::string contents = readFile(argv[2]);
        TreeBuilder tb = parse(contents);
        treeB = materializeTree(contents, tb.getRoot());
    }

    if (argc == 4) {
        std::cout << ">>> Skipping diffing\n";
        return 0;
    }

    // markSatellites(treeA);
    // markSatellites(treeB);

    std::cout << "TED(T1, T2) = " << ted(treeA, treeB) << '\n';

    // std::cout << "T1\n";
    // print(treeA);
    // std::cout << "T2\n";
    // print(treeB);

    Printer printer(treeA, treeB);
    printer.print();

    // printTree("T1", treeA);
    // printTree("T2", treeB);
}
