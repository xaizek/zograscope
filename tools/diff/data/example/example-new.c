/* This file is an example used
 * to compare diffs. */
int check(pid_t pid, const char info[], time_t start) {
    int status = 0;
    if (start != (time_t)-1) {
        waitpid(&status);
    }
    return status;
}
