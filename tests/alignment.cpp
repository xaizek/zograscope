// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "Catch/catch.hpp"

#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Lines with matching nodes are aligned", "[alignment]")
{
    std::string printed = compareAndPrint(parseCxx(R"(
        // Bad alignment

        class RenameTagCmd : public AutoCmdLineCmd<RenameTagCmd>
        {
        public:
            RenameTagCmd() : parent("rename-tag", 0U, 1U)
            {
            }

        private:
            virtual std::pair<Action, std::string>
            execImpl(AppState &state, const std::vector<std::string> &args,
                     DataTag data) override
            {
                Navigator *const nav = state.getNavigator();
                TagStorage &tagStorage = state.getStorage();
                IOHub *const io = state.getIOHub();

                Tag *const tag = nav->dataToTag(data);
                if (tag == nullptr) {
                    return { Action::DoNothing, "No tag to rename" };
                }

                std::string newName;
                if (args.empty()) {
                    const std::string prompt = "New name: ";
                    const std::string oldName = tag->getValue();
                    if (boost::optional<std::string> o = io->promptForInput(prompt,
                                                                            oldName)) {
                        newName = *o;
                    } else {
                        return { Action::DoNothing, {} };
                    }
                } else {
                    newName = args[0];
                }

                static_cast<void>(tagStorage.replace(tag, nav->getScopeOf(data),
                                                     tag->getRole(), newName));
                return { Action::DoNothing, {} };
            }
        };
    )"), parseCxx(R"(
        // Bad alignment

        class RenameTagCmd : public AutoCmdLineCmd<RenameTagCmd>
        {
        public:
            RenameTagCmd() : parent("rename-tag", 0U, 1U)
            {
            }

        private:
            virtual std::pair<Action, std::string>
            execImpl(AppState &state, const std::vector<std::string> &args,
                     DataTag data) override
            {
                Navigator *const nav = state.getNavigator();
                TagStorage &tagStorage = state.getStorage();
                IOHub *const io = state.getIOHub();

                Tag *const tag = nav->dataToTag(data);
                if (tag == nullptr) {
                    return { Action::DoNothing, "No tag to rename" };
                }

                std::string newName;
                if (args.empty()) {
                    const std::string prompt = "New name: ";
                    const std::string oldName = tag->getValue();
                    if (boost::optional<std::string> o = io->promptForInput(prompt,
                                                                            oldName)) {
                        newName = *o;
                    } else {
                        return { Action::DoNothing, {} };
                    }
                } else {
                    newName = args[0];
                }

                static_cast<void>(tagStorage.replace(tag, tag->getAssociatedTags(),
                                                    tag->getRole(), newName));
                return { Action::DoNothing, {} };
            }
        };

        class RenameInScopeCmd : public AutoCmdLineCmd<RenameInScopeCmd>
        {
        public:
            RenameInScopeCmd() : parent("rename-in-scope", 0U, 1U)
            {
            }

        private:
            virtual std::pair<Action, std::string>
            execImpl(AppState &state, const std::vector<std::string> &args,
                     DataTag data) override
            {
                Navigator *const nav = state.getNavigator();
                TagStorage &tagStorage = state.getStorage();
                IOHub *const io = state.getIOHub();

                Tag *const tag = nav->dataToTag(data);
                if (tag == nullptr) {
                    return { Action::DoNothing, "No tag to rename" };
                }

                std::string newName;
                if (args.empty()) {
                    const std::string prompt = "New name: ";
                    const std::string oldName = tag->getValue();
                    if (boost::optional<std::string> o = io->promptForInput(prompt,
                                                                            oldName)) {
                        newName = *o;
                    } else {
                        return { Action::DoNothing, {} };
                    }
                } else {
                    newName = args[0];
                }

                static_cast<void>(tagStorage.replace(tag, nav->getScopeOf(data),
                                                     tag->getRole(), newName));
                return { Action::DoNothing, {} };
            }
        };
    )"), true);

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                                                           |   1
         2  // Bad alignment                                                         |   2  // Bad alignment
         3                                                                           |   3
         -                                                                           >   4  {+class+}{+ +}{+RenameTagCmd+}{+ +}{+:+}{+ +}{+public+}{+ +}{+AutoCmdLineCmd+}{+<+}{+RenameTagCmd+}{+>+}
         -                                                                           >   5  {+{+}
         -                                                                           >   6  {+public+}{+:+}
         -                                                                           >   7      {+RenameTagCmd+}{+(+}{+)+}{+ +}{+:+}{+ +}{+parent+}{+(+}{+"rename-tag"+}{+,+}{+ +}{+0U+}{+,+}{+ +}{+1U+}{+)+}
         -                                                                           >   8      {+{+}
         -                                                                           >   9      {+}+}
         -                                                                           >  10
         -                                                                           >  11  {+private+}{+:+}
         -                                                                           >  12      {+virtual+}{+ +}{+std+}{+:+}{+:+}{+pair+}{+<+}{+Action+}{+,+}{+ +}{+std+}{+:+}{+:+}{+string+}{+>+}
         -                                                                           >  13      {+execImpl+}{+(+}{+AppState+}{+ +}{+&+}{+state+}{+,+}{+ +}{+const+}{+ +}{+std+}{+:+}{+:+}{+vector+}{+<+}{+std+}{+:+}{+:+}{+string+}{+>+}{+ +}{+&+}{+args+}{+,+}
         -                                                                           >  14               {+DataTag+}{+ +}{+data+}{+)+}{+ +}{+override+}
         -                                                                           >  15      {+{+}
         -                                                                           >  16          {+Navigator+}{+ +}{+*+}{+const+}{+ +}{+nav+}{+ +}{+=+}{+ +}{+state+}{+.+}{+getNavigator+}{+()+}{+;+}
         -                                                                           >  17          {+TagStorage+}{+ +}{+&+}{+tagStorage+}{+ +}{+=+}{+ +}{+state+}{+.+}{+getStorage+}{+()+}{+;+}
         -                                                                           >  18          {+IOHub+}{+ +}{+*+}{+const+}{+ +}{+io+}{+ +}{+=+}{+ +}{+state+}{+.+}{+getIOHub+}{+()+}{+;+}
         -                                                                           >  19
         -                                                                           >  20          {+Tag+}{+ +}{+*+}{+const+}{+ +}{+tag+}{+ +}{+=+}{+ +}{+nav+}{+->+}{+dataToTag+}{+(+}{+data+}{+)+}{+;+}
         -                                                                           >  21          {+if+}{+ +}{+(+}{+tag+}{+ +}{+==+}{+ +}{+nullptr+}{+)+}{+ +}{+{+}
         -                                                                           >  22              {+return+}{+ +}{+{+}{+ +}{+Action+}{+:+}{+:+}{+DoNothing+}{+,+}{+ +}{+"No tag to rename"+}{+ +}{+}+}{+;+}
         -                                                                           >  23          {+}+}
         -                                                                           >  24
         -                                                                           >  25          {+std+}{+:+}{+:+}{+string+}{+ +}{+newName+}{+;+}
         -                                                                           >  26          {+if+}{+ +}{+(+}{+args+}{+.+}{+empty+}{+()+}{+)+}{+ +}{+{+}
         -                                                                           >  27              {+const+}{+ +}{+std+}{+:+}{+:+}{+string+}{+ +}{+prompt+}{+ +}{+=+}{+ +}{+"New name: "+}{+;+}
         -                                                                           >  28              {+const+}{+ +}{+std+}{+:+}{+:+}{+string+}{+ +}{+oldName+}{+ +}{+=+}{+ +}{+tag+}{+->+}{+getValue+}{+()+}{+;+}
         -                                                                           >  29              {+if+}{+ +}{+(+}{+boost+}{+:+}{+:+}{+optional+}{+<+}{+std+}{+:+}{+:+}{+string+}{+>+}{+ +}{+o+}{+ +}{+=+}{+ +}{+io+}{+->+}{+promptForInput+}{+(+}{+prompt+}{+,+}
         -                                                                           >  30                                                                      {+oldName+}{+)+}{+)+}{+ +}{+{+}
         -                                                                           >  31                  {+newName+}{+ +}{+=+}{+ +}{+*+}{+o+}{+;+}
         -                                                                           >  32              {+}+}{+ +}{+else+}{+ +}{+{+}
         -                                                                           >  33                  {+return+}{+ +}{+{+}{+ +}{+Action+}{+:+}{+:+}{+DoNothing+}{+,+}{+ +}{+{+}{+}+}{+ +}{+}+}{+;+}
         -                                                                           >  34              {+}+}
         -                                                                           >  35          {+}+}{+ +}{+else+}{+ +}{+{+}
         -                                                                           >  36              {+newName+}{+ +}{+=+}{+ +}{+args+}{+[+}{+0+}{+]+}{+;+}
         -                                                                           >  37          {+}+}
         -                                                                           >  38
         -                                                                           >  39          {+static_cast+}{+<+}{+void+}{+>+}{+(+}{+tagStorage+}{+.+}{+replace+}{+(+}{+tag+}{+,+}{+ +}{+tag+}{+->+}{+getAssociatedTags+}{+()+}{+,+}
         -                                                                           >  40                                              {+tag+}{+->+}{+getRole+}{+()+}{+,+}{+ +}{+newName+}{+)+}{+)+}{+;+}
         -                                                                           >  41          {+return+}{+ +}{+{+}{+ +}{+Action+}{+:+}{+:+}{+DoNothing+}{+,+}{+ +}{+{+}{+}+}{+ +}{+}+}{+;+}
         -                                                                           >  42      {+}+}
         -                                                                           >  43  {+}+}{+;+}
         -                                                                           >  44
         4  class {-RenameTagCmd-} : public AutoCmdLineCmd{-<-}{-RenameTagCmd-}{->-} ~  45  class {+RenameInScopeCmd+} : public AutoCmdLineCmd{+<+}{+RenameInScopeCmd+}{+>+}
         5  {                                                                        |  46  {
         6  public:                                                                  |  47  public:
         7      {-RenameTagCmd-}() : parent({#"rename-tag"#}, 0U, 1U)                ~  48      {+RenameInScopeCmd+}() : parent({#"rename-in-scope"#}, 0U, 1U)
         8      {                                                                    |  49      {
         9      }                                                                    |  50      }
        .............................................................. @@ folded 34 identical lines @@ ................................................................................................................................................................
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Lines with matching nodes are aligned for multiline tokens",
          "[alignment]")
{
    std::string printed = compareAndPrint(parseC(R"(
        /* This is an example file. */
        int check(int id, const char inf[]) {
            int status;
            waitpid(&status);
            return status;
        }
    )", true), parseC(R"(
        /* This file is an example used
         * to compare diffs. */
        int check(pid_t pid, const char info[], time_t start) {
            int status = 0;
            if (start != (time_t)-1) {
                waitpid(&status);
            }
            return status;
        }
    )", true));

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                                      |   1
         2  /* This is an example {-file-}. */                  ~   2          /* This {+file+} is an example {+used+}
         -                                                      >   3           {+*+} {+to+} {+compare+} {+diffs+}. */
         3  int check({-int-}{- -}{-id-}, const char [inf][]) { ~   4          int check({+pid_t+}{+ +}{+pid+}, const char [inf{+o+}][]{+,+}{+ +}{+time_t+}{+ +}{+start+}) {
         4      int status;                                     ~   5              int status {+=+}{+ +}{+0+};
         -                                                      >   6              {+if+}{+ +}{+(+}{+start+}{+ +}{+!=+}{+ +}{+(+}{+time_t+}{+)+}{+-+}{+1+}{+)+}{+ +}{+{+}
         5      {:waitpid:}{:(:}{:&:}{:status:}{:):}{:;:}       ~   7                  {:waitpid:}{:(:}{:&:}{:status:}{:):}{:;:}
         -                                                      >   8              {+}+}
         6      return status;                                  |   9              return status;
         7  }                                                   |  10          }
    )");

    REQUIRE(printed == expected);
}

TEST_CASE("Separators are aligned when subtree separators match",
          "[alignment]")
{
    SECTION("Simplified")
    {
        std::string printed = compareAndPrint(parseCxx(R"(
            static void
            getParent()
            {
                return x;
            }
        )"), parseCxx(R"(
            void
            Comparator::getParent()
            {
                return x;
            }
        )"));

        std::string expected = normalizeText(R"(
            ~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            ~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             1                  |  1
             2  {-static-} void ~  2  void
             3  {-getParent-}() <  -
             -                  >  3  {+Comparator+}{+:+}{+:+}{+getParent+}()
             4  {               |  4  {
             5      return x;   |  5      return x;
             6  }               |  6  }
        )");

        REQUIRE(printed == expected);
    }

    SECTION("More complicated")
    {
        std::string printed = compareAndPrint(parseCxx(R"(
            static const Node *
            getParent(const Node *x)
            {
                do {
                    x = x->parent;
                } while (x != nullptr && isUnmovable(x));
                return x;
            }
        )"), parseCxx(R"(
            const Node *
            Comparator::getParent(const Node *x)
            {
                do {
                    x = x->parentNode;
                } while (x != nullptr &&
                         lang.isUnmovable(x));
                return x;
            }
        )"));

        std::string expected = normalizeText(R"(
            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             1                                                    |   1
             2  {-static-} const Node *                           ~   2  const Node *
             3  {:getParent:}(const Node *x)                      ~   3  {+Comparator+}{+:+}{+:+}{:getParent:}(const Node *x)
             4  {                                                 |   4  {
             5      do {                                          |   5      do {
             6          x = x->[parent];                          ~   6          x = x->[parent{+Node+}];
             7      } while (x != nullptr && {:isUnmovable:}(x)); ~   7      } while (x != nullptr &&
             -                                                    >   8               {+lang+}{+.+}{:isUnmovable:}(x));
             8      return x;                                     |   9      return x;
             9  }                                                 |  10  }
        )");

        REQUIRE(printed == expected);
    }
}

TEST_CASE("Completely added/removed lines are aligned against each other",
          "[alignment]")
{
    SECTION("Addition")
    {
        std::string printed = compareAndPrint(parseC(R"(
            #include <a.h>
            #include <c.h>
        )", true), parseC(R"(
            #include <a.h>
            #include <b.h>
            #include <c.h>
        )", true));

        std::string expected = normalizeText(R"(
            ~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~
            ~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~
            |  1
            |  2              #include <a.h>
            >  3              {+#include <b.h>+}
            |  4              #include <c.h>
        )");

        REQUIRE(printed == expected);
    }

    SECTION("Deletion")
    {
        std::string printed = compareAndPrint(parseC(R"(
            #include <a.h>
            #include <b.h>
            #include <c.h>
        )", true), parseC(R"(
            #include <a.h>
            #include <c.h>
        )", true));

        std::string expected = normalizeText(R"(
            ~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~
            ~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~
             1                                 |
             2              #include <a.h>     |
             3              {-#include <b.h>-} <
             4              #include <c.h>     |
        )");

        REQUIRE(printed == expected);
    }

    SECTION("Replacement")
    {
        std::string printed = compareAndPrint(parseC(R"(
            #include <a.h>
            #include <someveryoldfilewithclumsyname.h>
            #include <c.h>
        )", true), parseC(R"(
            #include <a.h>
            #include <longnewheader.h>
            #include <c.h>
        )", true));

        std::string expected = normalizeText(R"(
            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             1                                                             |  1
             2              #include <a.h>                                 |  2              #include <a.h>
             3              {-#include <someveryoldfilewithclumsyname.h>-} ~  3              {+#include <longnewheader.h>+}
             4              #include <c.h>                                 |  4              #include <c.h>
        )");

        REQUIRE(printed == expected);
    }
}

TEST_CASE("Identical lines in different states don't match",
          "[alignment]")
{
    std::string printed = compareAndPrint(parseC(R"(
        static int
        history_cmd(const cmd_info_t *cmd_info) {
            return 1;
        }
    )", true), parseC(R"(
        static int
        hideui_cmd(const cmd_info_t *cmd_info) {
            return 666;
        }

        static int
        history_cmd(const cmd_info_t *cmd_info) {
            return 0;
        }
    )", true));

    std::string expected = normalizeText(R"(
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         1                                            |   1
         -                                            >   2  {+static+}{+ +}{+int+}
         -                                            >   3  {+hideui_cmd+}{+(+}{+const+}{+ +}{+cmd_info_t+}{+ +}{+*+}{+cmd_info+}{+)+}{+ +}{+{+}
         -                                            >   4      {+return+}{+ +}{+666+}{+;+}
         -                                            >   5  {+}+}
         -                                            >   6
         2  static int                                |   7  static int
         3  history_cmd(const cmd_info_t *cmd_info) { |   8  history_cmd(const cmd_info_t *cmd_info) {
         4      return {#1#};                         ~   9      return {#0#};
         5  }                                         |  10  }
    )");

    REQUIRE(printed == expected);
}
