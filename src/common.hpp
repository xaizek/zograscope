#ifndef COMMON_HPP__
#define COMMON_HPP__

#include <string>

#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "integration.hpp"

class CommonArgs;
class Tree;

namespace cpp17 {
    namespace pmr {
        class memory_resource;
    }
}

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

// Reads and parses a file to build its tree.
optional_t<Tree> buildTreeFromFile(const std::string &path,
                                   const CommonArgs &args, TimeReport &tr,
                                   cpp17::pmr::memory_resource *mr);

void dumpTree(const CommonArgs &args, Tree &tree);

void dumpTrees(const CommonArgs &args, Tree &treeA, Tree &treeB);

#endif // COMMON_HPP__
