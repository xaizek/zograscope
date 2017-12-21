// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

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
lmld(Node &node, std::vector<int> &l)
{
    if (node.satellite) {
        return;
    }

    int satelliteCount = 0;
    for (Node *child : node.children) {
        if (!child->satellite) {
            ++satelliteCount;
            if (satelliteCount == 1) {
                l[node.poID] = l[child->poID];
            }
            lmld(*child, l);
        }
    }

    if (satelliteCount == 0) {
        l[node.poID] = node.poID;
    }
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
    auto notSatellite = [](const Node *n) { return !n->satellite; };

    int n = std::count_if(node.children.cbegin(), node.children.cend(),
                          notSatellite) == 0;
    for (Node *child : node.children) {
        if (!child->satellite) {
            n += countLeaves(*child);
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
printTree(const std::string &name, Tree &tree)
{
    std::cout << name << '\n';
    std::cout << std::setw(name.length())
              << std::setfill('=') << "" << std::setfill(' ') << '\n';
    tree.dump();

    std::cout << '\n';

    std::vector<Node *> po = postOrder(*tree.getRoot());
    for (Node *node : po) {
        std::cout << "po(" << node->label << ") = " << node->poID + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> l = lmld(*tree.getRoot());
    for (unsigned int i = 0U; i < l.size(); ++i) {
        std::cout << "l(" << po[i]->label << ") = " << l[i] + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> kr = makeKr(*tree.getRoot(), l);
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

    const Type type1 = canonizeType(n1->type);
    const Type type2 = canonizeType(n2->type);

    if (type1 >= Type::NonInterchangeable ||
        type2 >= Type::NonInterchangeable ||
        type1 != type2) {
        return Wch;
    }

    if (type1 == Type::Virtual) {
        return (n1->stype == n2->stype) ? Wren : Wch;
    }

    return Wren;

    // std::string n1l;
    // int a = 0, b = 0;
    // for (const Node *child : n1->children) {
    //     if (child->satellite) {
    //         n1l += child->label;
    //         ++a;
    //     }
    // }
    // std::string n2l;
    // for (const Node *child : n2->children) {
    //     if (child->satellite) {
    //         n2l += child->label;
    //         ++b;
    //     }
    // }
    // const bool identicalRename = (n1->label == n2->label && n1l == n2l && n1->children.size() == n2->children.size());
    // return (identicalRename ? 0 : Wren);
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

int
ted(Node &T1, Node &T2)
{
    std::vector<Node *> po1 = postOrder(T1);
    std::vector<Node *> po2 = postOrder(T2);

    std::vector<int> l1 = lmld(T1);
    std::vector<int> l2 = lmld(T2);

    boost::multi_array<Change, 2> td(boost::extents[l1.size()][l2.size()]);
    for (unsigned int i = 0; i < l1.size(); ++i) {
        for (unsigned int j = 0; j < l2.size(); ++j) {
            td[i][j].cost = -1;
        }
    }

    std::vector<int> k1 = makeKr(T1, l1);
    std::vector<int> k2 = makeKr(T2, l2);

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
