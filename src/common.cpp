#include "common.hpp"

#include <iostream>

#include "CommonArgs.hpp"
#include "decoration.hpp"
#include "time.hpp"

void
Environment::setup()
{
    if (args->color) {
        decor::enableDecorations();
    }
}

void
Environment::teardown()
{
    if (args->timeReport) {
        tr.stop();
        std::cout << tr;
    }
}
