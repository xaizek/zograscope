#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include "decoration.hpp"

int
main(int argc, const char *argv[])
{
    decor::disableDecorations();
    return Catch::Session().run(argc, argv);
}
