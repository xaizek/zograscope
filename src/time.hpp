#ifndef TIME_HPP__
#define TIME_HPP__

#include <chrono>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "trees.hpp"

class TimeReport
{
    using clock = std::chrono::steady_clock;

    struct Measure
    {
        bool measuring;
        std::string stage;
        clock::time_point start;
        clock::time_point end;
        Measure *parent;

        std::vector<Measure> children;

        Measure(std::string &&stage, Measure *parent)
            : measuring(true), stage(std::move(stage)), start(clock::now()),
              parent(parent)
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

    class ProxyTimer
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

    friend inline std::ostream &
    operator<<(std::ostream &os, const TimeReport &tr)
    {
        struct MeasureTraits
        {
            static unsigned int size(const Measure *node)
            {
                return node->children.size();
            }

            static const Measure * getChild(const Measure *node, unsigned int i)
            {
                return &node->children[i];
            }
        };

        using msf = std::chrono::duration<float, std::milli>;

        trees::printSetTraits<MeasureTraits>(os, &tr.root,
                     [](std::ostream &os, const Measure *m) {
                         msf duration = m->end - m->start;
                         os << m->stage << " -- " << duration.count() << "ms\n";
                     });

        return os;
    }

public:
    ProxyTimer measure(const std::string &stage)
    {
        start(stage);
        return ProxyTimer(*this);
    }

    void start(std::string stage)
    {
        current->children.emplace_back(std::move(stage), current);
        current = &current->children.back();
    }

    void stop()
    {
        current->stop();
        current = current->parent;
    }

private:
    Measure root {"Overall", nullptr};
    Measure *current {&root};
};

#endif // TIME_HPP__
