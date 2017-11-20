#ifndef COMMON_HPP__
#define COMMON_HPP__

#include "integration.hpp"
#include "time.hpp"

class CommonArgs;

class Environment
{
public:
    explicit Environment(CommonArgs &args) : args(&args)
    {
    }

public:
    TimeReport & getTimeKeeper()
    {
        return tr;
    }

    void setup();

    void teardown();

private:
    RedirectToPager redirectToPager;
    TimeReport tr;
    CommonArgs *args;
};

#endif // COMMON_HPP__
