#include <cstdlib>

/* Don't export the stuff bellow.

   It's specific to this translation unit. */
namespace {

    void f()
    {
        std::system("something");
    }

    void g()
    { }

} // end of anonymous namespace
