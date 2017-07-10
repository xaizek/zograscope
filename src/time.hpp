#ifndef TIME_HPP__
#define TIME_HPP__

#include <boost/optional.hpp>

#include <chrono>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

class TimeReport
{
    using clock = std::chrono::steady_clock;

    struct Measure
    {
        Measure(std::string &&stage, clock::time_point start)
            : stage(std::move(stage)), start(start)
        {
        }

        std::string stage;
        clock::time_point start;
        clock::time_point end;
    };

    class ProxyTimer
    {
    public:
        ProxyTimer(TimeReport &tr) : tr(tr) { }
        ~ProxyTimer() try
        {
            tr.stop();
        } catch (...) {
            // Do not throw from a destructor.
        }

    private:
        TimeReport &tr;
    };

    friend inline std::ostream &
    operator<<(std::ostream &os, const TimeReport &tr)
    {
        using msf = std::chrono::duration<float, std::milli>;

        msf total(0);

        for (const Measure &measure : tr.measures) {
            msf duration = measure.end - measure.start;
            total += duration;
            os << measure.stage << " -- " << duration.count() << "ms\n";
        }

        os << "TOTAL == " << total.count() << "ms\n";

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
        currentMeasure.emplace(std::move(stage), clock::now());
    }

    void stop()
    {
        if (!currentMeasure) {
            throw std::logic_error("Can't stop timer that isn't running");
        }

        currentMeasure->end = clock::now();
        measures.push_back(*currentMeasure);
        currentMeasure.reset();
    }

private:
    boost::optional<Measure> currentMeasure;
    std::vector<Measure> measures;
};

#endif // TIME_HPP__
