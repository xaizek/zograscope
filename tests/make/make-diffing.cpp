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

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Condition is a value of conditionals", "[make][comparison]")
{
    diffMake(R"(
        ifeq (,$(findstring gcc,$(CC)))  ## Deletions
            set := to this               ## Deletions
        endif                            ## Deletions

        ifneq ($(OS),Windows_NT)
        endif
    )", R"(
        ifneq ($(OS),Windows_NT)
            bin_suffix :=                ## Additions
        endif
    )");
}

TEST_CASE("Statements are not mixed up", "[make][comparison]")
{
    diffMake(R"(
        CXXFLAGS += prefix$(CFLAGS)
        CXXFLAGS += prefix$(CFLAGS)suffix    ## Mixed
        CXXFLAGS += $(CFLAGS)suffix
    )", R"(
        CXXFLAGS += prefix$(CFLAGS)
        CXXFLAGS += prefix$(CXXFLAGS)suffix  ## Mixed
        CXXFLAGS += $(CFLAGS)suffix
    )");
}

TEST_CASE("Removal export all/unexport statements is detected",
          "[make][comparison]")
{
    // The assignment there is to make Statements node match.
    diffMake(R"(
        export    ## Deletions
        unexport  ## Deletions

        a = b
    )", R"(
        a = b
    )");
}

TEST_CASE("Inversion of conditional is detected", "[make][comparison]")
{
    diffMake(R"(
        ifneq ($(OS),Windows_NT)  ## Mixed
        endif
    )", R"(
        ifeq ($(OS),Windows_NT)   ## Mixed
        endif
    )");

    diffMake(R"(
        ifdef a                   ## Mixed
        endif
    )", R"(
        ifndef a                  ## Mixed
        endif
    )");
}

TEST_CASE("Rules are on a separate layer", "[make][comparison]")
{
    diffMake(R"(
$(out_dir)/%.cpp: %.ico | $(out_dirs)
	ecppc -b -m image/x-icon -o $@ $<

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@
    )", R"(
$(out_dir)/%.cpp: %.ico | $(out_dirs)
	ecppc -b -m image/x-icon -o $@ $<

$(out_dir)/%.cpp: %.txt | $(out_dirs)  ## Additions
	ecppc -b -m text/plain -o $@ $<    ## Additions

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@
    )");
}

TEST_CASE("Rules and recipes are on a separate layers", "[make][comparison]")
{
    diffMake(R"(
.PHONY: coverage self-coverage reset-coverage

coverage: check $(bin)
	find $(out_dir)/ -name '*.o' -exec gcov -p {} + > $(out_dir)/gcov.out \  ## Deletions
	|| (cat $(out_dir)/gcov.out && false)                                    ## Deletions
	$(GCOV_PREFIX)uncov-gcov --root . --no-gcov --capture-worktree \         ## Mixed
	                         --exclude tests --exclude web \                 ## Mixed
	| $(UNCOV_PREFIX)uncov new                                               ## Mixed
	find . -name '*.gcov' -delete                                            ## Deletions

self-coverage: UNCOV_PREFIX := $(out_dir)/                                   ## Deletions
self-coverage: GCOV_PREFIX := ./                                             ## Deletions
self-coverage: coverage                                                      ## Mixed
    )", R"(
.PHONY: coverage self-coverage self-coverage-release reset-coverage  ## Mixed

coverage: check $(bin)
	uncov new-gcovi --exclude tests/ --exclude web/ \                ## Mixed
	                --capture-worktree $(out_dir)                    ## Mixed

self-coverage-release:                                               ## Mixed
	+$(MAKE) release                                                 ## Additions

self-coverage: check self-coverage-release                           ## Additions
	release/uncov new-gcovi --exclude tests/ --exclude web/ \        ## Additions
	                        --capture-worktree $(out_dir)            ## Additions
    )");
}

TEST_CASE("Terminals tie resolution in Make", "[make][comparison]")
{
    diffMake(R"(
define suite_template
ifeq ($(SANDBOX_PATH),sandbox)  ## Moves
$(SANDBOX_PATH)/$1:
	$(AT)mkdir -p $(B)$$@
else
$(SANDBOX_PATH)/$1:             ## Deletions
	$(AT)mkdir -p $$@
endif

$1: $$($1.bin)

.PHONY: $$($1.fixtures)
$$($1.fixtures): $$($1.bin)

build: $$($1.bin)

ifneq ($1,fuzz)
check: $1
endif
endef
    )", R"(
define suite_template
$(SANDBOX_PATH)/$1:
ifeq ($(SANDBOX_PATH),sandbox)  ## Moves
	$(AT)mkdir -p $(B)$$@
else
	$(AT)mkdir -p $$@
endif

$1: $$($1.bin)

.PHONY: $$($1.fixtures)
$$($1.fixtures): $$($1.bin)

build: $$($1.bin)

ifneq ($1,fuzz)
check: $1
endif
endef
    )");

    diffMake(R"(
        VAR = line2 \
              line3
    )", R"(
        VAR = line1 \  ## Mixed
              line2 \
              line3
    )");

    diffMake(R"(
        VAR = \
              line1 \
              line2 \
              line3
    )", R"(
        VAR = \
              line0 \  ## Additions
              line1 \
              line2 \
              line3
    )");

    diffMake(R"(
        VAR = line0 \
              line2 \
              line3
    )", R"(
        VAR = line0 \
              line1 \  ## Additions
              line2 \
              line3
    )");

    diffMake(R"(
        VAR = \
              line0 \
              line2 \
              line3 \
              line4
    )", R"(
        VAR = \
              line0 \
              line1 \  ## Additions
              line2 \
              line3 \
              line4
    )");
}
