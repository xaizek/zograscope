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

#include "utils/time.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Lines with matching nodes are aligned", "[alignment]")
{
    Tree oldTree = parseCxx(R"(
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
    )");
    Tree newTree = parseCxx(R"(
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
    )");

    TimeReport tr;
    compare(oldTree, newTree, tr, true, true);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

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

    REQUIRE(normalizeText(oss.str()) == expected);
}

TEST_CASE("Lines with matching nodes are aligned for multiline tokens",
          "[alignment]")
{
    Tree oldTree = parseC(R"(
        /* This is an example file. */
        int check(int id, const char inf[]) {
            int status;
            waitpid(&status);
            return status;
        }
    )", true);
    Tree newTree = parseC(R"(
        /* This file is an example used
         * to compare diffs. */
        int check(pid_t pid, const char info[], time_t start) {
            int status = 0;
            if (start != (time_t)-1) {
                waitpid(&status);
            }
            return status;
        }
    )", true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, false);

    std::ostringstream oss;
    Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                    *oldTree.getLanguage(), oss);
    printer.print(tr);

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

    REQUIRE(normalizeText(oss.str()) == expected);
}
