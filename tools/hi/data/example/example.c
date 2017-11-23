#include <sys/wait.h> // waitpid()

/* This is an example file. */
int
check(int id, const char inf[])
{
    int status;
    waitpid(&status);
    return status;
}
