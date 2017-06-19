#include "tree-edit-distance.hpp"

#define BOOST_DISABLE_ASSERTS
#include <boost/multi_array.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "tree.hpp"

struct Change
{
    int cost;
    int i, j;
};

enum { Wdel = 1, Wins = 1, Wren = 1, Wch = 3 };

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

static std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.relative = &root;
    postOrder(root, v);
    return v;
}

static void
lmld(Node &node, std::vector<int> &l)
{
    if (node.satellite) {
        return;
    }

    auto isSatellite = [](const Node &n) { return n.satellite; };
    std::vector<Node> n = node.children;
    n.erase(std::remove_if(n.begin(), n.end(), isSatellite), n.end());

    if (n.empty()) {
        l[node.poID] = node.poID;
        return;
    }

    for (Node &child : n) {
        lmld(child, l);
    }
    l[node.poID] = l[n[0].poID];
}

static std::vector<int>
lmld(Node &root)
{
    // post-order ID of root is equal to number of nodes - 1
    std::vector<int> l(root.poID + 1);
    lmld(root, l);
    return l;
}

static int
countLeaves(Node &node)
{
    auto notSatellite = [](const Node &n) { return !n.satellite; };

    int n = std::count_if(node.children.cbegin(), node.children.cend(),
                          notSatellite) == 0;
    for (Node &child : node.children) {
        if (!child.satellite) {
            n += countLeaves(child);
        }
    }
    return n;
}

static std::vector<int>
makeKr(Node &root, std::vector<int> &l)
{
    int k = countLeaves(root);

    std::vector<bool> visited(l.size());

    std::vector<int> kr;
    kr.reserve(k);

    int i = l.size() - 1U;
    while (k >= 1) {
        if (!visited[l[i]]) {
            kr.push_back(i);
            --k;
            visited[l[i]] = true;
        }
        --i;
    }

    std::sort(kr.begin(), kr.end());
    return kr;
}

void
printTree(const std::string &name, Node &root)
{
    std::cout << name << '\n';
    std::cout << std::setw(name.length())
              << std::setfill('=') << "" << std::setfill(' ') << '\n';
    print(root);

    std::cout << '\n';

    std::vector<Node *> po = postOrder(root);
    for (Node *node : po) {
        std::cout << "po(" << node->label << ") = " << node->poID + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> l = lmld(root);
    for (unsigned int i = 0U; i < l.size(); ++i) {
        std::cout << "l(" << po[i]->label << ") = " << l[i] + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> kr = makeKr(root, l);
    for (unsigned int i = 0U; i < kr.size(); ++i) {
        std::cout << "k[" << i << "] = " << kr[i] + 1 << '\n';
    }
}

static int
renameCost(const Node *n1, const Node *n2)
{
    if (n1->label == n2->label && n1->children.size() == n2->children.size()) {
        return 0;
    }

    if (n1->type >= Type::NonInterchangeable ||
        n2->type >= Type::NonInterchangeable ||
        n1->type != n2->type) {
        return Wch;
    }

    return Wren;

    std::string n1l;
    int a = 0, b = 0;
    for (const Node &child : n1->children) {
        if (child.satellite) {
            n1l += child.label;
            ++a;
        }
    }
    std::string n2l;
    for (const Node &child : n2->children) {
        if (child.satellite) {
            n2l += child.label;
            ++b;
        }
    }
    const bool identicalRename = (n1->label == n2->label && n1l == n2l && n1->children.size() == n2->children.size());
    return (identicalRename ? 0 : Wren);
}

static void
forestDist(int i, int j, const std::vector<int> &l1, const std::vector<int> &l2,
           boost::multi_array<Change, 2> &td,
           boost::multi_array<int, 2> &fd,
           const std::vector<Node *> &po1, const std::vector<Node *> &po2)
{
    fd[l1[i] - 1][l2[j] - 1] = 0;
    for (int di = l1[i]; di <= i; ++di) {
        fd[di][l2[j] - 1] = fd[di - 1][l2[j] - 1] + Wdel;
    }
    for (int dj = l2[j]; dj <= j; ++dj) {
        fd[l1[i] - 1][dj] = fd[l1[i] - 1][dj - 1] + Wins;
    }
    for (int di = l1[i]; di <= i; ++di) {
        const int ldi = l1[di];
        for (int dj = l2[j]; dj <= j; ++dj) {
            const int ldj = l2[dj];
            if (ldi == l1[i] && ldj == l2[j]) {
                fd[di][dj] = std::min({
                    fd[di - 1][dj] + Wdel,
                    fd[di][dj - 1] + Wins,
                    fd[di - 1][dj - 1] + renameCost(po1[di], po2[dj])
                });
                td[di][dj] = Change { fd[di][dj], i, j };
            } else {
                fd[di][dj] =
                    std::min({ fd[di - 1][dj] + Wdel,
                               fd[di][dj - 1] + Wins,
                               fd[ldi - 1][ldj - 1] + td[di][dj].cost });
            }
        }
    }

    // for (int di = l1[i] - 1; di <= i; ++di) {
    //     for (int dj = l2[j] - 1; dj <= j; ++dj) {
    //         std::cout << std::setw(2) << fd[di][dj] << ' ';
    //     }
    //     std::cout << '\n';
    // }

    // std::cout << "--------\n";

    // for (unsigned int i = 0; i < l1.size(); ++i) {
    //     for (unsigned int j = 0; j < l2.size(); ++j) {
    //         std::string mark;
    //         // switch (td[i][j].state) {
    //         //     case State::Unchanged: mark = "(U)"; break;
    //         //     case State::Deleted:   mark = "(D)";  break;
    //         //     case State::Inserted:  mark = "(I)"; break;
    //         //     case State::Updated:   mark = "(R)";  break;
    //         // }

    //         std::cout << std::setw(2) << td[i][j].cost << mark << ' ';
    //     }
    //     std::cout << '\n';
    // }
}

class BacktrackingQueue
{
    using pair = std::pair<int, int>;

public:
    void enqueue(int i, int j, int di, int dj)
    {
        queue[pair{ i, j }].emplace_back(di, dj);
    }

    std::vector<pair> takeCurrent()
    {
        auto lastItem = --queue.end();
        std::vector<pair> r = lastItem->second;
        queue.erase(lastItem);
        return r;
    }

    bool hasMore() const
    {
        return !queue.empty();
    }

    pair getCurrent() const
    {
        return (--queue.cend())->first;
    }

private:
    std::map<pair, std::vector<pair>> queue;
};

static void
backtrackForests(const std::vector<int> &l1, const std::vector<int> &l2,
                 const boost::multi_array<Change, 2> &td,
                 boost::multi_array<int, 2> &fd,
                 const std::vector<Node *> &po1, const std::vector<Node *> &po2,
                 BacktrackingQueue &bq)
{
    int i, j;
    std::tie(i, j) = bq.getCurrent();

    // This creates forest table identical to the one created on forward pass,
    // but it doesn't update tree table.  Tree table is fully calculated by now,
    // so we can just use its values.
    fd[l1[i] - 1][l2[j] - 1] = 0;
    for (int di = l1[i]; di <= i; ++di) {
        fd[di][l2[j] - 1] = fd[di - 1][l2[j] - 1] + Wdel;
    }
    for (int dj = l2[j]; dj <= j; ++dj) {
        fd[l1[i] - 1][dj] = fd[l1[i] - 1][dj - 1] + Wins;
    }
    for (int di = l1[i]; di <= i; ++di) {
        for (int dj = l2[j]; dj <= j; ++dj) {
            if (l1[di] == l1[i] && l2[dj] == l2[j]) {
                fd[di][dj] = std::min({
                    fd[di - 1][dj] + Wdel,
                    fd[di][dj - 1] + Wins,
                    fd[di - 1][dj - 1] + renameCost(po1[di], po2[dj])
                });
            } else {
                fd[di][dj] =
                    std::min({ fd[di - 1][dj] + Wdel,
                               fd[di][dj - 1] + Wins,
                               fd[l1[di] - 1][l2[dj] - 1] + td[di][dj].cost });
            }
        }
    }

    for (const auto &p : bq.takeCurrent()) {
        int di = p.first, dj = p.second;
        while (di > l1[i] - 1 || dj > l2[j] - 1) {
            if (di == l1[i] - 1) {
                po2[dj--]->state = State::Inserted;
            } else if (dj == l2[j] - 1) {
                po1[di--]->state = State::Deleted;
            } else if (l1[di] == l1[i] && l2[dj] == l2[j]) {
                if (fd[di][dj] == fd[di - 1][dj] + Wdel) {
                    po1[di--]->state = State::Deleted;
                } else if (fd[di][dj] == fd[di][dj - 1] + Wins) {
                    po2[dj--]->state = State::Inserted;
                } else if (fd[di][dj] != fd[di - 1][dj - 1]) {
                    po1[di]->relative = po2[dj];
                    po2[dj]->relative = po1[di];

                    po1[di--]->state = State::Updated;
                    po2[dj--]->state = State::Updated;
                } else {
                    --di;
                    --dj;
                }
            } else {
                if (fd[di][dj] == fd[di - 1][dj] + Wdel) {
                    po1[di--]->state = State::Deleted;
                } else if (fd[di][dj] == fd[di][dj - 1] + Wins) {
                    po2[dj--]->state = State::Inserted;
                } else {
                    bq.enqueue(td[di][dj].i, td[di][dj].j, di, dj);
                    di = l1[di] - 1;
                    dj = l2[dj] - 1;
                }
            }
        }
    }
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

std::vector<Node>::iterator
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

void
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

static int
ted(Node *T1, Node *T2)
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

    std::vector<int> l1 = lmld(*T1);
    std::vector<int> l2 = lmld(*T2);

    boost::multi_array<Change, 2> td(boost::extents[l1.size()][l2.size()]);
    for (unsigned int i = 0; i < l1.size(); ++i) {
        for (unsigned int j = 0; j < l2.size(); ++j) {
            td[i][j].cost = -1;
        }
    }

    std::vector<int> k1 = makeKr(*T1, l1);
    std::vector<int> k2 = makeKr(*T2, l2);

    using range = boost::multi_array_types::extent_range;
    boost::multi_array<int, 2> fd(boost::extents[range(-1, po1.size())]
                                                [range(-1, po2.size())]);
    // int step = 0;
    for (int x : k1) {
        for (int y : k2) {
            // std::cout << "step #" << ++step << '\n';
            forestDist(x, y, l1, l2, td, fd, po1, po2);
        }
    }

    // Mark nodes with states by backtracking through forest arrays.  We do this
    // in reversed order by regenerating only arrays that are actually needed to
    // recover solution.  Starting with cell containing the answer and figuring
    // out how we get there like in regular dynamic programming.  The difference
    // is that we need to process multiple arrays.  The tracing splits on steps
    // where forests are processed based on information from tree array.
    BacktrackingQueue bq;
    bq.enqueue(k1.back(), k2.back(), l1.size() - 1, l2.size() - 1);
    while (bq.hasMore()) {
        backtrackForests(l1, l2, td, fd, po1, po2, bq);
    }

    return td[l1.size() - 1][l2.size() - 1].cost;
}

int
ted(Node &T1, Node &T2)
{
    return ted(&T1, &T2);
}
