#include "tree.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "TreeBuilder.hpp"
#include "types.hpp"

static bool areIdentical(const Node &l, const Node &r);

void
print(const Node &node, int lvl)
{
    // if (node.satellite) {
    //     return;
    // }

    std::string prefix = (lvl > 0) ? "`---" : "";

    std::string suffix;
    switch (node.state) {
        case State::Unchanged: break;
        case State::Deleted:   suffix = " (deleted)";  break;
        case State::Inserted:  suffix = " (inserted)"; break;
        case State::Updated:   suffix = " (updated with " + node.relative->label + "@" + std::to_string(node.relative->poID) + ")";  break;
    }

    std::cout << std::setw(4*lvl) << prefix << node.label
              << '[' << node.poID << ']'
              << '(' << node.line << ';' << node.col << ')'
              << (node.satellite ? "{S}" : "")
              << suffix
              << '<' << static_cast<int>(node.type) << '>'
              << '\n';
    for (const Node &child : node.children) {
        print(child, lvl + 1);
    }
}

static std::string
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
    Node n;
    n.label = materializeLabel(contents, node);
    n.line = node->line;
    n.col = node->col;
    n.type = tokenToType(node->value.token);

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializeTree(contents, child));
    }

    return n;
}

static void
postOrder(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    for (Node &child : node.children) {
        child.relative = &node;
        postOrder(child, v);
    }
    node.poID = v.size();
    v.push_back(&node);
}

std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.relative = &root;
    postOrder(root, v);
    return v;
}

template <typename I1, typename I2, typename P>
std::pair<I1, I2>
mismatch(I1 f1, I1 l1, I2 f2, I2 l2, P p)
{
    while (f1 != l1 && f2 != l2 && p(*f1, *f2)) {
        ++f1;
        ++f2;
    }
    return { f1, f2 };
}

template <typename I, typename T, typename F>
I
for_each_n(I first, T n, F f)
{
    for (T i = 0; i < n; ++first, ++i) {
        f(*first);
    }
    return first;
}

static std::vector<Node>::iterator
rootPos(Node &root, Node *n)
{
    if (n == &root) {
        return root.children.begin();
    }

    while (n->relative != &root) {
        n = n->relative;
    }
    return root.children.begin() + (n - &root.children[0]);
}

static void
reduce(std::vector<Node *> &po1, std::vector<Node *> &po2)
{
    Node &T1 = *po1.back();
    Node &T2 = *po2.back();

    auto eq = [](const Node *n1, const Node *n2) {
        return n1->label == n2->label;
    };

    auto f = mismatch(po1.begin(), po1.end(), po2.begin(), po2.end(), eq);

    using rit = std::vector<Node *>::reverse_iterator;
    auto e = mismatch(po1.rbegin(), rit(f.first), po2.rbegin(), rit(f.second),
                      eq);

    auto mark = [](Node &n) {
        n.satellite = true;
    };

    int t1Front;
    if (f.first == po1.end()) {
        t1Front = T1.children.size();
    } else {
        t1Front = rootPos(T1, *f.first) - T1.children.begin();
    }
    int t2Front;
    if (f.second == po2.end()) {
        t2Front = T2.children.size();
    } else {
        t2Front = rootPos(T2, *f.second) - T2.children.begin();
    }

    for (int i = 0, n = std::min(t1Front, t2Front); i < n; ++i) {
        if (!areIdentical(T1.children[i], T2.children[i])) {
            break;
        }
        mark(T1.children[i]);
        mark(T2.children[i]);
    }

    int t1Back = 0;
    if (!T1.children.empty() &&
        (e.first != rit(f.first) || f.first != po1.begin())) {
        t1Back = T1.children.end() - ++rootPos(T1, *e.first);
    }
    int t2Back = 0;
    if (!T2.children.empty() &&
        (e.second != rit(f.second) || f.second != po2.begin())) {
        t2Back = T2.children.end() - ++rootPos(T2, *e.second);
    }

    for (int i = 0, n = std::min(t1Back, t2Back); i < n; ++i) {
        if (!areIdentical(T1.children[T1.children.size() - 1 - i],
                          T2.children[T2.children.size() - 1 - i])) {
            break;
        }
        mark(T1.children[T1.children.size() - 1 - i]);
        mark(T2.children[T2.children.size() - 1 - i]);
    }
}

void
reduceTrees(Node *&T1, Node *&T2)
{
    while (T1->children.size() == 1U && T2->children.size() == 1U) {
        T1 = &T1->children.front();
        T2 = &T2->children.front();
    }

    auto notReduced = [](const Node &n) { return !n.satellite; };


    std::vector<Node *> po1 = postOrder(*T1);
    std::vector<Node *> po2 = postOrder(*T2);
    while (true) {
        reduce(po1, po2);

        bool canReduceMore = T1->children.size() > 1U
                          && T2->children.size() > 1U
                          && std::count_if(T1->children.cbegin(),
                                           T1->children.cend(),
                                           notReduced) == 1
                          && std::count_if(T2->children.cbegin(),
                                           T2->children.cend(),
                                           notReduced) == 1;

        if (!canReduceMore) {
            po1 = postOrder(*T1);
            po2 = postOrder(*T2);
            break;
        }

        T1 = &*std::find_if(T1->children.begin(), T1->children.end(),
                            notReduced);
        T2 = &*std::find_if(T2->children.begin(), T2->children.end(),
                            notReduced);

        while (T1->children.size() == 1U && T2->children.size() == 1U) {
            T1 = &T1->children.front();
            T2 = &T2->children.front();
        }

        po1 = postOrder(*T1);
        po2 = postOrder(*T2);
    }
}

static bool
areIdentical(const Node &l, const Node &r)
{
    if (l.label != r.label || l.children.size() != r.children.size()) {
        return false;
    }

    for (unsigned int i = 0; i < l.children.size(); ++i) {
        if (!areIdentical(l.children[i], r.children[i])) {
            return false;
        }
    }

    return true;
}
