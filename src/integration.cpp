#include "integration.hpp"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/scope_exit.hpp>

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace io = boost::iostreams;

class RedirectToPager::Impl
{
public:
    virtual ~Impl() = default;
};

class PagerRedirect : public RedirectToPager::Impl
{
public:
    /**
     * @brief Custom stream buffer that spawns pager for large outputs only.
     *
     * Collect up to <terminal height> lines.  If buffer is closed with this
     * limit not reached, it prints lines on std::cout.  If we hit the limit in
     * the process of output, it opens a pager and feeds it all collected output
     * and everything that comes next.
     */
    class ScreenPageBuffer
    {
    public:
        using char_type = char;
        using category = boost::iostreams::sink_tag;

    public:
        ScreenPageBuffer(unsigned int screenHeight,
                         io::stream_buffer<io::file_descriptor_sink> *out);
        ~ScreenPageBuffer();

    public:
        std::streamsize write(const char s[], std::streamsize n);

    private:
        bool put(char c);
        void openPager();

    private:
        bool redirectToPager = false;
        unsigned int nLines = 0U;
        unsigned int screenHeight;
        std::string buffer;

        /**
         * @brief Pointer to buffer stored in RedirectToPager.
         *
         * This is not by value, because ScreenPageBuffer needs to be copyable.
         */
        io::stream_buffer<io::file_descriptor_sink> *out;
        pid_t pid;
    };

public:
    PagerRedirect() : screenPageBuffer(getTerminalSize().second, &out)
    {
        rdbuf = std::cout.rdbuf(&screenPageBuffer);
    }

    ~PagerRedirect()
    {
        // Flush the stream to make sure that we put all contents we want through
        // the custom stream buffer.
        std::cout.flush();

        std::cout.rdbuf(rdbuf);
    }

private:
    io::stream_buffer<io::file_descriptor_sink> out;
    io::stream_buffer<ScreenPageBuffer> screenPageBuffer;
    std::streambuf *rdbuf;
};

using ScreenPageBuffer = PagerRedirect::ScreenPageBuffer;

ScreenPageBuffer::ScreenPageBuffer(unsigned int screenHeight,
                               io::stream_buffer<io::file_descriptor_sink> *out)
    : screenHeight(screenHeight), out(out)
{
}

ScreenPageBuffer::~ScreenPageBuffer()
{
    if (redirectToPager) {
        out->close();
        int wstatus;
        waitpid(pid, &wstatus, 0);
    } else {
        std::cout << buffer;
    }
}

std::streamsize
ScreenPageBuffer::write(const char s[], std::streamsize n)
{
    for (std::streamsize i = 0U; i < n; ++i) {
        if (!put(s[i])) {
            return i;
        }
    }
    return n;
}

bool
ScreenPageBuffer::put(char c)
{
    if (redirectToPager) {
        return boost::iostreams::put(*out, c);
    }

    if (c == '\n') {
        ++nLines;
    }

    if (nLines > screenHeight) {
        openPager();
        redirectToPager = true;
        for (char c : buffer) {
            if (!boost::iostreams::put(*out, c)) {
                return false;
            }
        }
        return boost::iostreams::put(*out, c);
    }

    buffer.push_back(c);
    return true;
}

void
ScreenPageBuffer::openPager()
{
    int pipePair[2];
    if (pipe(pipePair) != 0) {
        throw std::runtime_error("Failed to create a pipe");
    }
    BOOST_SCOPE_EXIT_ALL(pipePair) { close(pipePair[0]); };

    pid = fork();
    if (pid == -1) {
        close(pipePair[1]);
        throw std::runtime_error("Fork has failed");
    }
    if (pid == 0) {
        close(pipePair[1]);
        if (dup2(pipePair[0], STDIN_FILENO) == -1) {
            _Exit(EXIT_FAILURE);
        }
        close(pipePair[0]);
        // XXX: hard-coded invocation of less.
        execlp("less", "less", "-R", static_cast<char *>(nullptr));
        _Exit(127);
    }

    out->open(io::file_descriptor_sink(pipePair[1],
                                       boost::iostreams::close_handle));
}

RedirectToPager::RedirectToPager()
    : impl(isOutputToTerminal() ? new PagerRedirect() : nullptr)
{
}

RedirectToPager::~RedirectToPager()
{
    // Destroy impl with complete type.
}

bool
isOutputToTerminal()
{
    return isatty(fileno(stdout));
}

std::pair<unsigned int, unsigned int>
getTerminalSize()
{
    winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
        return {
            std::numeric_limits<unsigned int>::max(),
            std::numeric_limits<unsigned int>::max()
        };
    }

    return { ws.ws_col, ws.ws_row };
}
