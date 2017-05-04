#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "TreeBuilder.hpp"
#include "decoration.hpp"
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

std::string
readFile(const std::string &path)
{
    std::ifstream ifile(path);
    std::ostringstream iss;
    iss << ifile.rdbuf();
    return iss.str();
}

Node
materializeTree(const std::string &contents, const PNode *node)
{
    Node n = { contents.substr(node->value.from, node->value.len) };
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

    std::cout << "T1\n";
    print(treeA);
    printSource(treeA);
    std::cout << "T2\n";
    // print(treeB);
    printSource(treeB);

    // printTree("T1", treeA);
    // printTree("T2", treeB);
}
