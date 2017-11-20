#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include "decoration.hpp"

int
main(int argc, char *argv[])
{
    decor::disableDecorations();
    return Catch::Session().run(argc, argv);
}
