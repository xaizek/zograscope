// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

// These are tests of more advanced properties of a parser, which are much
// easier and reliable to test by performing comparison.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Functions and their declarations are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void readFile(const std::string &path);    /// Deletions
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(argv, options);  /// Mixed
        }
    )", R"(
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(args, options);  /// Mixed
        }
    )");

    diffSrcmlCxx(R"(
        static std::string readFile(const std::string &path);     /// Deletions
    )", R"(
        void                                                      /// Additions
        Environment::setup(const std::vector<std::string> &argv)  /// Additions
        {                                                         /// Additions
        }                                                         /// Additions
    )");
}

TEST_CASE("Declarations are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        struct Struct {
            int contiguousChars = 0;
            int callNesting = 0;                            /// Deletions
        };
    )", R"(
        struct Struct {
            static constexpr bool FunctionNesting = false;  /// Additions
            static constexpr bool ArgumentNesting = true;   /// Additions

            int contiguousChars = 0;
            std::vector<bool> nesting;                      /// Additions
        };
    )");
}

TEST_CASE("Statements are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map["comment"]   = +SrcmlCxxSType::Comment;    /// Moves
            map["separator"] = +SrcmlCxxSType::Separator;
        }
    )", R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map["separator"] = +SrcmlCxxSType::Separator;
            map["comment"]   = +SrcmlCxxSType::Comment;    /// Moves
        }
    )");
}

TEST_CASE("Names are on a separate layer", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            return SrcmlCxxSType::Function;
        }
    )", R"(
        void f() {
            return
                  SrcmlCxxSType::Parameter   /// Additions
                  &&                         /// Additions
                  SrcmlCxxSType::Function;
        }
    )");
}

TEST_CASE("Lambdas are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            auto
                distillLeafs                              /// Deletions
                = [&]() {
                for (const TerminalMatch &m : matches) {
                }
            };
        }
    )", R"(
        void f() {
            auto
                matchTerminals                            /// Additions
                = [&
                  matches                                 /// Additions
                ]() {
                for (const TerminalMatch &m : matches) {
                }
            };
        }
    )");
}

TEST_CASE("Function body is spliced into function itself",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            float that;  /// Deletions
        }
    )", R"(
        void f() {
            int var;     /// Additions
        }
    )");
}

TEST_CASE("Parameter list spliced into function",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void func(
                  int old          /// Deletions
                  ) {
            return;
        }
    )", R"(
        void func(
                  float something  /// Additions
                  ) {
            return;
        }
    )");
}

TEST_CASE("Argument list is spliced into the call",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            longAndBoringName(
                  "old line"    /// Updates
            );
        }
    )", R"(
        void f() {
            longAndBoringName(
                  "new string"  /// Updates
            );
        }
    )");
}

TEST_CASE("then nodes are spliced into if statement",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            int a;            /// Moves
            if (condition) {
                int b;        /// Moves
                int c;        /// Moves
            }
        }
    )", R"(
        void f() {
            if (condition) {
                if (cond) {   /// Additions
                    int a;    /// Moves
                    int b;    /// Moves
                    int c;    /// Moves
                }             /// Additions
            }
        }
    )");

    diffSrcmlCxx(R"(
        void f() {
            if (condition) {
                something;    /// Deletions
            }
            else {            /// Deletions
                int a;        /// Moves
                int b;        /// Moves
            }                 /// Deletions
        }
    )", R"(
        void f() {
            if (condition) {
                if (cond) {   /// Additions
                    int a;    /// Moves
                    int b;    /// Moves
                }             /// Additions
            }
        }
    )");

    diffSrcmlCxx(R"(
        void g() {
            if (g)
                int a;
        }
    )", R"(
        void g() {
            if (g) {
                int a;
            }
        }
    )");
}

TEST_CASE("Parenthesis aren't mixed with operators",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            return
                  (                                     /// Deletions
                  -x->stype == SrcmlCxxSType::Comment
                  )                                     /// Deletions
                  ;
        }
    )", R"(
        void f() {
            return -x->stype == SrcmlCxxSType::Comment
                || x->type == Type::StrConstants        /// Additions
                ;
        }
    )");
}

TEST_CASE("Various declaration aren't mixed up", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        class SrcmlTransformer
        {
            SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                            const std::string &language                         /// Deletions
                            ,
                            const std::unordered_map<std::string, SType> &map   /// Deletions
                            ,
                            const std::unordered_set<std::string> &keywords);

        private:
            const std::unordered_map<std::string, SType> &map; // Tag -> SType
        };
    )", R"(
        enum class Type : std::uint8_t;                                        /// Additions

        class SrcmlTransformer
        {
            SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                            const std::unordered_set<std::string> &keywords);

            // Determines type of a child of the specified element.            /// Additions
            Type determineType(TiXmlElement *elem, boost::string_ref value);   /// Additions

        private:
            const std::unordered_map<std::string, SType> &map; // Tag -> SType
        };
    )");
}

TEST_CASE("Various statements aren't mixed up", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (elem->ValueStr() == "comment") {
                return Type::Comments;
            }
            else if (elem->ValueStr() == "name") {                               /// Moves
                if (parentValue == "type") {                                     /// Moves
                    return bla ? Type::Keywords : Type::UserTypes;               /// Moves
                } else if (parentValue == "function" || parentValue == "call") { /// Moves
                    return Type::Functions;                                      /// Moves
                }                                                                /// Moves
            } else if (keywords.find(value.to_string()) != keywords.cend()) {    /// Moves
                return Type::Keywords;                                           /// Moves
            }                                                                    /// Moves
            return Type::Other;
        }
    )", R"(
        void f() {
            if (elem->ValueStr() == "comment") {
                return Type::Comments;
            }
            else if (inCppDirective) {                                           /// Additions
                return Type::Directives;                                         /// Additions
            } else if (value[0] == '(' || value[0] == '{' || value[0] == '[') {  /// Additions
                return Type::LeftBrackets;                                       /// Additions
            } else if (value[0] == ')' || value[0] == '}' || value[0] == ']') {  /// Additions
                return Type::RightBrackets;                                      /// Additions
            } else if (elem->ValueStr() == "operator") {                         /// Additions
                return Type::Operators;                                          /// Additions
            }                                                                    /// Additions
            else if (elem->ValueStr() == "name") {                               /// Moves
                if (parentValue == "type") {                                     /// Moves
                    return bla ? Type::Keywords : Type::UserTypes;               /// Moves
                } else if (parentValue == "function" || parentValue == "call") { /// Moves
                    return Type::Functions;                                      /// Moves
                }                                                                /// Moves
            } else if (keywords.find(value.to_string()) != keywords.cend()) {    /// Moves
                return Type::Keywords;                                           /// Moves
            }                                                                    /// Moves
            return Type::Other;
        }
    )");
}

TEST_CASE("Initializer lists are decomposed", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        Class::Class() :
            something(value)
        {}
    )", R"(
        Class::Class() :
            newVar(value)     /// Additions
          , something(value)
        {}
    )");
}

TEST_CASE("Empty blocks are handled correctly", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (condition) { }
            else {              /// Deletions
                int a;          /// Moves
                int b;          /// Moves
            }                   /// Deletions
        }
    )", R"(
        void f() {
            if (condition) {
                if (cond) {     /// Additions
                    int a;      /// Moves
                    int b;      /// Moves
                }               /// Additions
            }
        }
    )");
}

TEST_CASE("Else-if statements are nested recursively",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (*p == 'T') {                    /// Moves
                cfg.trunc_normal_sb_msgs = 1;   /// Moves
            } else if (*p == 'p') {             /// Moves
                cfg.shorten_title_paths = 1;    /// Moves
            }                                   /// Moves
        }
    )", R"(
        void f() {
            if (*p == 'M') {                    /// Additions
                cfg.short_term_mux_titles = 1;  /// Additions
            } else                              /// Additions
            if (*p == 'T') {                    /// Moves
                cfg.trunc_normal_sb_msgs = 1;   /// Moves
            } else if (*p == 'p') {             /// Moves
                cfg.shorten_title_paths = 1;    /// Moves
            }                                   /// Moves
        }
    )");
}

TEST_CASE("Parenthesis aren't part of conditions",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            auto handler = [&](const Node *node) {
                if (
                    args.count                      /// Deletions
                   ) {
                    return;
                }

                oneMoreStatement;
            };
        }
    )", R"(
        void f() {
            auto handler = [&](const Node *node) {
                if (
                    countOnly                       /// Additions
                   ) {
                    return;
                }

                oneMoreStatement;
            };
        }
    )");
}

TEST_CASE("Parenthesis of empty parameter list are decomposed and stripped",
          "[.srcml][srcml-cxx][parser]")
{
    diffSrcmlCxx(R"(
        Class::Class() {
        }
    )", R"(
        Class::Class(
                     int arg  /// Additions
                     ) {
        }
    )");

    diffSrcmlCxx(R"(
        class Blass {
            explicit
            Blass();
        }
    )", R"(
        class Class {                                     /// Additions
            Class();                                      /// Additions
        };                                                /// Additions

        class Blass {
            explicit Blass(
                           KeyboardEventHandler &handler  /// Additions
                           );
        }
    )");
}

TEST_CASE("Case labels are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            switch (state.state) {
                case State::Normal:
                    break;
            }
        }
    )", R"(
        void f() {
            if (c == '[') {                   /// Additions
                state.state = State::Normal;  /// Additions
            }                                 /// Additions

            switch (state.state) {
                case State::Normal:
                    break;
            }
        }
    )");
}

TEST_CASE("Initializers are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        std::map<int, std::function<Action(DataTag data)>> keys {
            { 'B', makeCmdThunk("browse-all")     },
        };
    )", R"(
        std::map<int, std::function<Action(DataTag data)>> keys {
            { 'b', makeCmdThunk("browse")         }                /// Additions
            ,
            { 'B', makeCmdThunk("browse-all")     },
        };
    )");
}

TEST_CASE("Classes are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        class
            RenameTagCmd                                           /// Deletions
            : public AutoCmdLineCmd
            <RenameTagCmd>                                         /// Deletions
        {
            virtual void execImpl() override
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                tag->getRole(), newName);
            }
        };
    )", R"(
        class RenameTagCmd : public AutoCmdLineCmd<RenameTagCmd>   /// Additions
        {                                                          /// Additions
            virtual void execImpl() override                       /// Additions
            {                                                      /// Additions
                tagStorage.replace(tag, tag->getAssociatedTags(),  /// Additions
                                   tag->getRole(), newName);       /// Additions
            }                                                      /// Additions
        };                                                         /// Additions

        class
            RenameInScopeCmd                                       /// Additions
            : public AutoCmdLineCmd
              <RenameInScopeCmd>                                   /// Additions
        {
            virtual void execImpl() override
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                   tag->getRole(), newName);
            }
        };
    )");
}

TEST_CASE("Structs are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        struct
            RenameTagCmd                                           /// Deletions
            : public AutoCmdLineCmd
            <RenameTagCmd>                                         /// Deletions
        {
            virtual void execImpl() override
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                tag->getRole(), newName);
            }
        };
    )", R"(
        struct RenameTagCmd : public AutoCmdLineCmd<RenameTagCmd>  /// Additions
        {                                                          /// Additions
            virtual void execImpl() override                       /// Additions
            {                                                      /// Additions
                tagStorage.replace(tag, tag->getAssociatedTags(),  /// Additions
                                   tag->getRole(), newName);       /// Additions
            }                                                      /// Additions
        };                                                         /// Additions

        struct
            RenameInScopeCmd                                       /// Additions
            : public AutoCmdLineCmd
              <RenameInScopeCmd>                                   /// Additions
        {
            virtual void execImpl() override
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                   tag->getRole(), newName);
            }
        };
    )");
}

TEST_CASE("Unions are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        union
            RenameTagCmd                                           /// Deletions
        {
            void execImpl()
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                   tag->getRole(), newName);
            }
        };
    )", R"(
        union RenameTagCmd                                         /// Additions
        {                                                          /// Additions
            void execImpl()                                        /// Additions
            {                                                      /// Additions
                tagStorage.replace(tag, tag->getAssociatedTags(),  /// Additions
                                   tag->getRole(), newName);       /// Additions
            }                                                      /// Additions
        };                                                         /// Additions

        union
            RenameInScopeCmd                                       /// Additions
        {
            void execImpl()
            {
                tagStorage.replace(tag, nav->getScopeOf(data),
                                tag->getRole(), newName);
            }
        };
    )");
}
