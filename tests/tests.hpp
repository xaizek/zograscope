#ifndef TESTS__TESTS_HPP__
#define TESTS__TESTS_HPP__

#include <ostream>
#include <sstream>
#include <string>

class Node;
class Tree;

enum class SType;
enum class State;
enum class Type;

/**
 * @brief Temporarily redirects specified stream into a string.
 */
class StreamCapture
{
public:
    /**
     * @brief Constructs instance that redirects @p os.
     *
     * @param os Stream to redirect.
     */
    StreamCapture(std::ostream &os) : os(os)
    {
        rdbuf = os.rdbuf();
        os.rdbuf(oss.rdbuf());
    }

    /**
     * @brief Restores original state of the stream.
     */
    ~StreamCapture()
    {
        os.rdbuf(rdbuf);
    }

public:
    /**
     * @brief Retrieves captured output collected so far.
     *
     * @returns String containing the output.
     */
    std::string get() const
    {
        return oss.str();
    }

private:
    std::ostream &os;       //!< Stream that is being redirected.
    std::ostringstream oss; //!< Temporary output buffer of the stream.
    std::streambuf *rdbuf;  //!< Original output buffer of the stream.
};

Tree makeTree(const std::string &str, bool coarse = false);

const Node * findNode(const Tree &tree, Type type,
                      const std::string &label = {});

int countLeaves(const Node &root, State state);

int countInternal(const Node &root, SType stype, State state);

void diffSources(const std::string &left, const std::string &right,
                 bool skipRefine);

#endif // TESTS__TESTS_HPP__
