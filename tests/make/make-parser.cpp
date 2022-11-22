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

// These are tests of basic properties of a parser.  Things like:
//  - whether some constructs can be parsed
//  - whether elements are identified correctly

#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>

#include "make/MakeSType.hpp"
#include "utils/strings.hpp"
#include "STree.hpp"
#include "TermHighlighter.hpp"
#include "tree.hpp"

#include "tests.hpp"

using namespace makestypes;

TEST_CASE("Error is printed on incorrect Makefile syntax", "[make][parser]")
{
    StreamCapture cerrCapture(std::cerr);
    CHECK_FALSE(makeIsParsed(":"));
    REQUIRE(boost::starts_with(cerrCapture.get(), "<input>:1:1:"));
}

TEST_CASE("Empty Makefile is OK", "[make][parser]")
{
    CHECK(makeIsParsed(""));
    CHECK(makeIsParsed("      "));
    CHECK(makeIsParsed("   \n   "));
    CHECK(makeIsParsed("\t\n \t \n"));
}

TEST_CASE("Root always has Makefile stype", "[make][parser]")
{
    Tree tree;

    tree = parseMake("");
    CHECK(tree.getRoot()->stype == +MakeSType::Makefile);

    tree = parseMake("a = b");
    CHECK(tree.getRoot()->stype == +MakeSType::Makefile);
}

TEST_CASE("Useless empty temporary containers are dropped", "[make][parser]")
{
    auto pred = [](const Node *node) {
        return node->stype == +MakeSType::TemporaryContainer
            && node->label.empty();
    };

    Tree tree = parseMake("set := to this");
    CHECK(findNode(tree, pred) == nullptr);
}

TEST_CASE("Comments are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed(" # comment"));
    CHECK(makeIsParsed("# comment "));
    CHECK(makeIsParsed(" # comment "));

    const char *const multiline = R"(
        a := a#b \
               asdf
    )";
    CHECK(makeIsParsed(multiline));
}

TEST_CASE("Assignments are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around assignment statement") {
        CHECK(makeIsParsed("OS := os "));
        CHECK(makeIsParsed(" OS := os "));
        CHECK(makeIsParsed(" OS := os"));
    }
    SECTION("Whitespace around assignment") {
        CHECK(makeIsParsed("OS := os"));
        CHECK(makeIsParsed("OS:= os"));
        CHECK(makeIsParsed("OS :=os"));
    }
    SECTION("Various assignment kinds") {
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS += -std=c++11 a:b"));
        CHECK(makeIsParsed("CXX ?= g++"));
        CHECK(makeIsParsed("CXX ::= g++"));
        CHECK(makeIsParsed("CXX != whereis g++"));
    }
    SECTION("One variable") {
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)suffix"));
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)suffix"));
    }
    SECTION("Two variables") {
        CHECK(makeIsParsed("CXXFLAGS = $(var1)$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = $(var1)val$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)$(var2)val"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)val$(var2)val"));
    }
    SECTION("Override and/or export") {
        CHECK(makeIsParsed("override CXXFLAGS ::="));
        CHECK(makeIsParsed("override CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("override export CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("export override CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("export CXXFLAGS = $(CFLAGS)suffix"));
    }
    SECTION("Keywords in the value") {
        CHECK(makeIsParsed("KEYWORDS := ifdef/ifndef/ifeq/ifneq/else/endif"));
        CHECK(makeIsParsed("KEYWORDS += include"));
        CHECK(makeIsParsed("KEYWORDS += override/export/unexport"));
        CHECK(makeIsParsed("KEYWORDS += define/endef/undefine"));
    }
    SECTION("Keywords in the name") {
        CHECK(makeIsParsed("a.ifndef.b = a"));
        CHECK(makeIsParsed("ifndef.b = a"));
        CHECK(makeIsParsed("ifndef/b = a"));
        CHECK(makeIsParsed(",ifdef,ifndef,ifeq,ifneq,else,endif = b"));
        CHECK(makeIsParsed(",override,include,define,endef,undefine = b"));
        CHECK(makeIsParsed(",export,unexport = b"));
    }
    SECTION("Functions in the name") {
        CHECK(makeIsParsed(R"($(1).stuff :=)"));
    }
    SECTION("Special symbols in the value") {
        CHECK(makeIsParsed(R"(var += sed_first='s,^\([^/]*\)/.*$$,\1,';)"));
        CHECK(makeIsParsed(R"(var != test $$# -gt 0)"));
    }
    SECTION("Whitespace around assignment statement") {
        Tree tree;

        tree = parseMake("var := #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("var := val#comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("override var := #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("export var := val #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);
    }
}

TEST_CASE("Variables are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("$($(V))"));
    CHECK(makeIsParsed("$($(V)suffix)"));
    CHECK(makeIsParsed("$($(V)?endif)"));
    CHECK(makeIsParsed("$(bla?endif)"));
    CHECK(makeIsParsed("$(AT_$(V))"));
    CHECK(makeIsParsed("target: )()("));

    Tree tree = parseMake("target: $$($1.name)");
    CHECK(findNode(tree, Type::UserTypes, "$1") != nullptr);
    // This isn't a variable, it's an escape sequence.
    CHECK(findNode(tree, Type::UserTypes, "$$") == nullptr);
}

TEST_CASE("Functions are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around function statement") {
        CHECK(makeIsParsed("$(info a)"));
        CHECK(makeIsParsed(" $(info a)"));
        CHECK(makeIsParsed(" $(info a) "));
        CHECK(makeIsParsed("$(info a) "));
        CHECK(makeIsParsed("$(info a )"));
    }
    SECTION("Whitespace around comma") {
        CHECK(makeIsParsed("$(info arg1,arg2)"));
        CHECK(makeIsParsed("$(info arg1, arg2)"));
        CHECK(makeIsParsed("$(info arg1 ,arg2)"));
        CHECK(makeIsParsed("$(info arg1 , arg2)"));
    }
    SECTION("Functions in arguments") {
        CHECK(makeIsParsed("$(info $(f1)$(f2))"));
    }
    SECTION("Empty arguments") {
        CHECK(makeIsParsed("$(patsubst , ,)"));
        CHECK(makeIsParsed("$(patsubst ,,)"));
        CHECK(makeIsParsed("$(patsubst a,,)"));
        CHECK(makeIsParsed("$(patsubst a,b,)"));
    }
    SECTION("Keywords in the arguments") {
        CHECK(makeIsParsed("$(info ifdef/ifndef/ifeq/ifneq/else/endif)"));
        CHECK(makeIsParsed("$(info include)"));
        CHECK(makeIsParsed("$(info override/export/unexport)"));
        CHECK(makeIsParsed("$(info define/endef/undefine)"));
    }
    SECTION("Curly braces") {
        Tree tree = parseMake("target: ${name}");
        CHECK(findNode(tree, Type::LeftBrackets, "${") != nullptr);
        CHECK(findNode(tree, Type::RightBrackets, "}") != nullptr);
    }
    SECTION("Nested parenthesis") {
        CHECK(makeIsParsed("$(info (b), ( (a) ) (c))"));
        CHECK(makeIsParsed("$(info (b), ( (a) ) (c), ( $(substr $(a),$(b),$(c)) ) )"));
        CHECK(makeIsParsed("$(foreach src, $(stmmac-srcs), ifeq ($(shell test $(R) -gt $(REV); echo $$?),0) )"));
    }
    SECTION("String literals in the arguments") {
        CHECK(makeIsParsed(R"($(patsubst "%",%,$(VAR)))"));
        CHECK(makeIsParsed(R"_(${shell "$(CC)"})_"));
        CHECK(makeIsParsed(R"($(error "str"))"));
    }
}

TEST_CASE("Targets are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around colon") {
        CHECK(makeIsParsed("target: prereq"));
        CHECK(makeIsParsed("target : prereq"));
        CHECK(makeIsParsed("target :prereq"));
    }
    SECTION("No prerequisites") {
        CHECK(makeIsParsed("target:"));
    }
    SECTION("Multiple prerequisites") {
        CHECK(makeIsParsed("target: prereq1 prereq2"));
    }
    SECTION("Multiple targets") {
        CHECK(makeIsParsed("debug release sanitize-basic: all"));
    }
    SECTION("Double-colon") {
        CHECK(makeIsParsed("debug:: all"));
        CHECK(makeIsParsed("debug :: all"));
        CHECK(makeIsParsed("debug ::all"));
    }
    SECTION("Keywords in targets") {
        CHECK(makeIsParsed("include.b: all"));
        CHECK(makeIsParsed("x.include.b: all"));
        CHECK(makeIsParsed("all: b.override.b"));
        CHECK(makeIsParsed("all: override.b"));
    }
    SECTION("Expressions in prerequisites and targets") {
        CHECK(makeIsParsed("target: $(dependencies)"));
        CHECK(makeIsParsed("$(target): dependencies"));
        CHECK(makeIsParsed("$(target): $(dependencies)"));
        CHECK(makeIsParsed("$(tar)$(get): $(dependencies)"));
    }
}

TEST_CASE("Static pattern rules are parsed", "[make][parser]")
{
    CHECK(makeIsParsed("target: target-pattern: prereq"));
    CHECK(makeIsParsed("$(AOBJS) $(UAOBJS) $(HEAD_OBJ): %$(OBJEXT): %.S"));
    CHECK(makeIsParsed("$(COBJS) $(UCOBJS): %$(OBJEXT): CFLAGS+=-O0"));
}

TEST_CASE("Recipes are parsed in a Makefile", "[make][parser]")
{
    const char *const singleLine = R"(
target: prereq
	the only recipe
    )";
    CHECK(makeIsParsed(singleLine));

    const char *const multipleLines = R"(
target: prereq
	first recipe
	second recipe
    )";
    CHECK(makeIsParsed(multipleLines));

    const char *const withComments = R"(
target: prereq
# comment
	first recipe
# comment
	second recipe
	# comment
    )";
    CHECK(makeIsParsed(withComments));

    const char *const withParens = R"(
target: prereq
	(echo something)
    )";
    CHECK(makeIsParsed(withParens));

    const char *const withFunctions = R"(
target: prereq
	$a$b
    )";
    CHECK(makeIsParsed(withFunctions));

    const char *const assignment = R"(
        $(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses
    )";
    CHECK(makeIsParsed(assignment));

    const char *const withStaticPattern = R"(
targets: target: prereq
	$a$b
    )";
    CHECK(makeIsParsed(withStaticPattern ));
}

TEST_CASE("Recipes with breaks", "[make][parser]")
{
    const char *const withTab = R"(
rule:
	@echo hi
	
	@echo there
    )";
    CHECK(makeIsParsed(withTab ));

    const char *const commands = R"(
rule:
	@echo hi

	@echo there
    )";
    CHECK(makeIsParsed(commands));

    const char *const comments = R"(
rule:
	#asdf

	#asdf
    )";
    CHECK(makeIsParsed(comments));

    const char *const conditonalsInRecipe = R"(
rule:
ifeq 'a' 'a'
	adf agdf
endif

ifeq 'a' 'a'
    a=10
endif
    )";
    CHECK(makeIsParsed(conditonalsInRecipe));

    const char *const conditionalsInAndAfterRecipe = R"(
rule:
ifeq 'a' 'a'
	adf agdf
endif

ifeq 'a' 'a'
    a=10
endif
    )";
    CHECK(makeIsParsed(conditionalsInAndAfterRecipe ));
}

TEST_CASE("Conditionals are parsed in a Makefile", "[make][parser]")
{
    const char *const withBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
            bin_suffix :=
        endif
    )";
    CHECK(makeIsParsed(withBody_withoutElse));

    const char *const withoutBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withoutElse));

    const char *const withBody_withElse = R"(
        ifndef tool_template
            bin_suffix :=
        else
            bin_suffix := .exe
        endif
    )";
    CHECK(makeIsParsed(withBody_withElse));

    const char *const withoutBody_withElse = R"(
        ifeq ($(OS),Windows_NT)
            bin_suffix :=
        else
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withElse));

    const char *const nested = R"(
        ifneq ($(OS),Windows_NT)
            ifdef OS
            endif
        endif
    )";
    CHECK(makeIsParsed(nested));

    const char *const emptyArgs = R"(
        ifneq ($(OS),)
        endif
        ifneq (,$(OS))
        endif
        ifneq (,)
        endif
    )";
    CHECK(makeIsParsed(emptyArgs));

    const char *const elseIf = R"(
        ifneq ($(OS),)
        else ifndef OS
        else ifeq (,)
        endif
    )";
    CHECK(makeIsParsed(elseIf));

    const char *const withComments = R"(
        ifneq ($(OS),)  # ifneq-comment
        else            # else-comment
        endif           # endif-comment
    )";
    CHECK(makeIsParsed(withComments));

    const char *const conditionalInRecipes = R"(
reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif
    )";
    CHECK(makeIsParsed(conditionalInRecipes));

    const char *const conditionalsInRecipes = R"(
reset-coverage:
ifdef tool_template
	find $(out_dir)/ -name '*.gcda' -delete
endif
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
else
endif
    )";
    CHECK(makeIsParsed(conditionalsInRecipes));

    const char *const elseIfInRecipes = R"(
reset-coverage:
ifdef tool_template
	find $(out_dir)/ -name '*.gcda' -delete
else ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
else ifdef something
else
endif
    )";
    CHECK(makeIsParsed(elseIfInRecipes));

    const char *const conditionalInRecipesWithComments = R"(
reset-coverage:
ifeq ($(with_cov),1)  # ifeq-comment
	find $(out_dir)/ -name '*.gcda' -delete
else                  # else-comment
endif                 # endif-comment
    )";
    CHECK(makeIsParsed(conditionalInRecipesWithComments));

    const char *const alternativeForm = R"(
        ifneq 'a' 'b'
        endif
        ifeq 'a' "b"
        endif
        ifneq "a" 'b'
        endif
        ifneq "a" "b"
        endif

        ifneq 'a''b'
        endif
        ifeq 'a'"b"
        endif
        ifneq "a"'b'
        endif
        ifneq "a""b"
        endif
    )";
    CHECK(makeIsParsed(alternativeForm));
}

TEST_CASE("Defines are parsed in a Makefile", "[make][parser]")
{
    const char *const noBody = R"(
        define pattern
        endef
    )";
    CHECK(makeIsParsed(noBody));

    const char *const noSuffix = R"(
        define pattern
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(noSuffix));

    const char *const weirdName = R"(
        define )name
        endef
    )";
    CHECK(makeIsParsed(weirdName));

    const char *const withOverride = R"(
        override define pattern ?=
        endef
    )";
    CHECK(makeIsParsed(withOverride));

    const char *const withExport = R"(
        export define pattern
        endef
    )";
    CHECK(makeIsParsed(withExport));

    const char *const withOverrideAndExport = R"(
        export override define pattern
        endef
    )";
    CHECK(makeIsParsed(withOverrideAndExport));

    const char *const eqSuffix = R"(
        define pattern =
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(eqSuffix));

    const char *const immSuffix = R"(
        define pattern :=
        endef

        define pattern ::=
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(immSuffix));

    const char *const appendSuffix = R"(
        define pattern +=
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(appendSuffix));

    const char *const bangSuffix = R"(
        define pattern +=
        endef
    )";
    Tree tree = parseMake(bangSuffix);
    CHECK(findNode(tree, Type::Assignments, "+=") != nullptr);

    const char *const expresion = R"(
        define tool_template
            $1.bin
        endef
    )";
    CHECK(makeIsParsed(expresion));

    const char *const expresions = R"(
        define vifm_SOURCES :=
            $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(expresions));

    const char *const textFirstEmptyLine = R"(
        define vifm_SOURCES :=

            bla $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(textFirstEmptyLine));

    const char *const multipleEmptyLines = R"(
        define vifm_SOURCES :=


            bla$(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(multipleEmptyLines));

    const char *const emptyLinesEverywhere = R"(
        define vifm_SOURCES :=

            $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)

            something

        endef
    )";
    CHECK(makeIsParsed(emptyLinesEverywhere));

    const char *const keywordsInName = R"(
        define keywords
            (
            )
            ,
            # comment
            include
            override
            export
            unexport
            ifdef
            ifndef
            ifeq
            ifneq
            else
            endif
            define
            undefine
        endef
    )";
    CHECK(makeIsParsed(keywordsInName));
}

TEST_CASE("Line escaping works in a Makefile", "[make][parser]")
{
    const char *const multiline = R"(
        pos = $(strip $(eval T := ) \
                      $(eval i := -1) \
                      $(foreach elem, $1, \
                                $(if $(filter $2,$(elem)), \
                                              $(eval i := $(words $T)), \
                                              $(eval T := $T $(elem)))) \
                      $i)
    )";
    CHECK(makeIsParsed(multiline));
}

TEST_CASE("Substitutions are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("lib_objects := $(lib_sources:%.cpp=$(out_dir)/%.o)"));
    CHECK(makeIsParsed("am__test_logs1 = $(TESTS:=.log)"));
}

TEST_CASE("Arguments are not treated as substitutions", "[make][parser]")
{
    CHECK(makeIsParsed("$(a :)"));
    CHECK(makeIsParsed("$(a ::)"));
    CHECK(makeIsParsed("$(a ::,:)"));
    CHECK(makeIsParsed("$(a ::, :)"));
    CHECK(makeIsParsed("$(subst :,,$(VPATH))"));
    CHECK(makeIsParsed("$(subst :,:,$(VPATH))"));
}

TEST_CASE("Makefile keywords are not found inside text/ids", "[make][parser]")
{
    CHECK(makeIsParsed("EXTRA_CXXFLAGS += -fsanitize=undefined"));
}

TEST_CASE("Includes are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("include $(wildcard *.d)"));
    CHECK(makeIsParsed("include config.mk"));
    CHECK(makeIsParsed("-include config.mk"));
    CHECK(makeIsParsed("include include"));
}

TEST_CASE("Leading tabs are allowed not only for recipes in Makefiles",
          "[make][parser]")
{
    const char *const conditional = R"(
	ifeq (,$(findstring gcc,$(CC)))
		set := to this
	endif
    )";
    CHECK(makeIsParsed(conditional));

    const char *const emptyConditional = R"(
	ifeq (,$(findstring gcc,$(CC)))
	endif
    )";
    CHECK(makeIsParsed(emptyConditional));

    const char *const statements = R"(
	set := to this
	# comment
    )";
    CHECK(makeIsParsed(statements));
}

TEST_CASE("Column of recipe is computed correctly", "[make][parser]")
{
    std::string input = R"(
target:
	command1
	command2
    )";
    std::string expected = R"(
target:
    command1
    command2)";

    Tree tree = parseMake(input);

    std::string output = TermHighlighter(tree).print();
    CHECK(split(output, '\n') == split(expected, '\n'));
}

TEST_CASE("Export/unexport directives", "[make][parser]")
{
    CHECK(makeIsParsed("export"));
    CHECK(makeIsParsed("export var"));
    CHECK(makeIsParsed("export $(vars)"));
    CHECK(makeIsParsed("export var $(vars)"));
    CHECK(makeIsParsed("export var$(vars)"));

    CHECK(makeIsParsed("unexport"));
    CHECK(makeIsParsed("unexport var"));
    CHECK(makeIsParsed("unexport $(vars)"));
    CHECK(makeIsParsed("unexport var $(vars)"));
    CHECK(makeIsParsed("unexport var$(vars)"));
}

TEST_CASE("Undefine directive", "[make][parser]")
{
    CHECK(makeIsParsed("undefine something"));
    CHECK(makeIsParsed("undefine this and that"));
    CHECK(makeIsParsed("undefine some $(things)"));
    CHECK(makeIsParsed("undefine override something"));
    CHECK(makeIsParsed("override undefine something"));
}

TEST_CASE("Strings are recognized by the parser", "[make][parser]")
{
    Tree tree;

    tree = parseMake(R"(
target:
	echo \'define VERSION "0.9" > $@;
    )");
    CHECK(findNode(tree, Type::StrConstants, "\"0.9\"") != nullptr);

    tree = parseMake(R"(
target:
	echo \'define VERSION "0.9.1-beta"' > $@;
    )");
    CHECK(findNode(tree, Type::StrConstants,
                   "'define VERSION \"0.9.1-beta\"'") != nullptr);
}

TEST_CASE("EOL continuation is identified in GNU Make", "[make][parser]")
{
    Tree tree = parseMake(R"(
        a = \
          b
    )");
    const Node *node = findNode(tree, [](const Node *node) {
                                    return node->label == "\\";
                                });
    REQUIRE(node != nullptr);
    CHECK(tree.getLanguage()->isEolContinuation(node));
}

TEST_CASE("New line node does not appear in the tree", "[make][parser]")
{
    Tree tree = parseMake(R"(
        define suite_template
           a

        endef
    )");
    const Node *node = findNode(tree, [](const Node *node) {
                                    return node->label == "\n";
                                });
    REQUIRE(node == nullptr);
}

TEST_CASE("Tabulation of size 1 is allowed in Makefiles", "[make][parser]")
{
    int tabWidth = 1;

    const char *const str = ""
        "# 0\n"
        "\t# 1\n"
        "\t\t# 2\n"
    ;

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create("Makefile");

    TreeBuilder tb = lang->parse(str, "<input>", tabWidth, /*debug=*/false, mr);
    REQUIRE_FALSE(tb.hasFailed());

    STree stree(std::move(tb), str, false, false, *lang, mr);
    Tree tree(std::move(lang), tabWidth, str, stree.getRoot());

    const Node *node;

    node = findNode(tree, Type::Comments, "# 0");
    REQUIRE(node);
    CHECK(node->col == 1);

    node = findNode(tree, Type::Comments, "# 1");
    REQUIRE(node);
    CHECK(node->col == 2);

    node = findNode(tree, Type::Comments, "# 2");
    REQUIRE(node);
    CHECK(node->col == 3);
}
