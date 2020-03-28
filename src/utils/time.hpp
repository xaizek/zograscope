// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE__UTILS__TIME_HPP__
#define ZOGRASCOPE__UTILS__TIME_HPP__

#include <chrono>
#include <iosfwd>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/trees.hpp"

class TimeReport
{
    friend std::ostream & operator<<(std::ostream &os, const TimeReport &tr);

    using clock = std::chrono::steady_clock;

    class ProxyTimer;

    struct Measure
    {
        bool measuring;
        bool foreign; // The measurement came from nested report.
        std::string stage;
        clock::time_point start;
        clock::time_point end;
        Measure *parent;

        std::vector<Measure> children;

        Measure(std::string &&stage, Measure *parent)
            : measuring(true), foreign(false), stage(std::move(stage)),
              start(clock::now()), parent(parent)
        {
        }

        void stop()
        {
            if (!measuring) {
                throw std::logic_error("Can't stop timer that isn't running");
            }

            end = clock::now();
            measuring = false;
        }
    };

public:
    TimeReport() = default;
    // Constructs nested time report object that moves its children to the
    // `parent` in destructor or in `commit()`.
    explicit TimeReport(TimeReport &parent)
        : parent(parent.current), parentIndex(parent.current->children.size())
    { }
    // For nested time report, moves measurements into linked parent time
    // report.
    ~TimeReport() try
    {
        commit();
    } catch (...) {
        // Do not throw from a destructor.
    }

public:
    ProxyTimer measure(const std::string &stage);

    void start(std::string stage)
    {
        current->children.emplace_back(std::move(stage), current);
        current = &current->children.back();
    }

    void stop()
    {
        current->stop();
        if (current->parent != nullptr) {
            current = current->parent;
        }
    }

    // Moves measurements into linked parent time report, if any.  The moved
    // measurements are marked as foreign.
    void commit()
    {
        if (parent != nullptr) {
            for (Measure &measure : root.children) {
                measure.foreign = true;
            }
            parent->children.insert(
                parent->children.cbegin() + parentIndex,
                std::make_move_iterator(root.children.begin()),
                std::make_move_iterator(root.children.end())
            );
            parent = nullptr;
        }
    }

private:
    Measure root {"Overall", nullptr};
    Measure *current {&root};

    // For nested time report object.
    Measure *parent = nullptr;
    int parentIndex = 0;
};

class TimeReport::ProxyTimer
{
public:
    ProxyTimer(TimeReport &tr) : tr(&tr), done(false) { }
    ProxyTimer(ProxyTimer &&rhs) : tr(rhs.tr), done(rhs.done)
    {
        rhs.done = true;
    }
    ~ProxyTimer() try
    {
        if (!done) {
            tr->stop();
        }
    } catch (...) {
        // Do not throw from a destructor.
    }

    ProxyTimer & operator=(ProxyTimer &&rhs)
    {
        tr = rhs.tr;
        done = rhs.done;
        rhs.done = true;
        return *this;
    }

public:
    void measure(std::string stage)
    {
        tr->stop();
        done = true;

        *this = tr->measure(std::move(stage));
    }

private:
    TimeReport *tr;
    bool done;
};

inline TimeReport::ProxyTimer
TimeReport::measure(const std::string &stage)
{
    start(stage);
    return ProxyTimer(*this);
}

#endif // ZOGRASCOPE__UTILS__TIME_HPP__
