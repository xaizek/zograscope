#include <boost/optional.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#include "pmr/monolithic.hpp"

#include "utils/optional.hpp"
#include "Highlighter.hpp"
#include "common.hpp"
#include "tree.hpp"

static int run(const CommonArgs &args, TimeReport &tr);

int
main(int argc, char *argv[])
{
    CommonArgs args;
    int result;

    try {
        Environment env;
        env.setup({ argv + 1, argv + argc });

        args = env.getCommonArgs();
        if (args.pos.size() != 1U) {
            env.teardown(true);
            std::cerr << "Wrong positional arguments\n"
                      << "Expected exactly one\n";
            return EXIT_FAILURE;
        }

        result = run(args, env.getTimeKeeper());

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    return result;
}

static int
run(const CommonArgs &args, TimeReport &tr)
{
    cpp17::pmr::monolithic mr;
    Tree tree(&mr);

    const std::string &path = args.pos[0];
    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        tree = *t;
    } else {
        return EXIT_FAILURE;
    }

    dumpTree(args, tree);
    if (!args.dryRun) {
        std::cout << Highlighter().print(*tree.getRoot()) << '\n';
    }
    return EXIT_SUCCESS;
}
