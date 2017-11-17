#ifndef PARSER_HPP__
#define PARSER_HPP__

#include <string>

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

class TreeBuilder;

TreeBuilder parse(const std::string &contents,
                  const std::string &fileName,
                  bool debug,
                  cpp17::pmr::monolithic &mr);

#endif // PARSER_HPP__
