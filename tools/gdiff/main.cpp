#include <QApplication>

#include <cstdlib>

#include <iostream>

#include "tooling/common.hpp"

#include "ZSDiff.hpp"

int
main(int argc, char *argv[]) try
{
    QApplication app(argc, argv);

    Environment env;
    env.setup({ argv + 1, argv + argc });

    const CommonArgs &args = env.getCommonArgs();

    if (args.help) {
        std::cout << "Usage: zs-gdiff [options...] old-file new-file\n"
                  << "\n"
                  << "Options:\n";
        env.printOptions();
        return EXIT_SUCCESS;
    }
    if (args.pos.size() != 2U) {
        env.teardown(true);
        std::cerr << "Wrong positional arguments\n"
                  << "Expected exactly 2\n";
        return EXIT_FAILURE;
    }

    ZSDiff w(args.pos[0], args.pos[1], env.getTimeKeeper());
    w.show();

    int result = app.exec();
    env.teardown();
    return result;
} catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
}
