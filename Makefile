NAME := zograscope

CXXFLAGS += -std=c++11 -Wall -Wextra -MMD -MP -Isrc/ -Ithird-party/ -DYYDEBUG
CXXFLAGS += -pthread
LDFLAGS  += -g -lboost_iostreams -lboost_program_options -lboost_filesystem
LDFLAGS  += -lboost_system -pthread

INSTALL := install
DESTDIR :=
PREFIX  := /usr

# a variable that can be overridden to control which tests to run
TESTS :=

# optional build-time dependencies
QT5_PROG :=
HAVE_LIBGIT2 :=

ifneq ($(OS),Windows_NT)
    bin_suffix :=
else
    bin_suffix := .exe
endif

# this function of two arguments (array and element) returns index of the
# element in the array; return -1 if item not found in the list
pos = $(strip $(eval T := ) \
              $(eval i := -1) \
              $(foreach elem, $1, \
                        $(if $(filter $2,$(elem)), \
                                      $(eval i := $(words $T)), \
                                      $(eval T := $T $(elem)))) \
              $i)

# determine output directory and build target; "." is the directory by default
# or "release"/"debug" for corresponding targets
is_release := 0
ifneq ($(call pos,release,$(MAKECMDGOALS)),-1)
    is_release := 1
endif
ifneq ($(is_release),0)
    EXTRA_CXXFLAGS := -O3
    # EXTRA_LDFLAGS  := -Wl,--strip-all

    out_dir := release
else
    EXTRA_CXXFLAGS := -O0 -g
    EXTRA_LDFLAGS  := -g

    ifneq ($(call pos,debug,$(MAKECMDGOALS)),-1)
        out_dir := debug
    else ifneq ($(call pos,sanitize-basic,$(MAKECMDGOALS)),-1)
        out_dir := sanitize-basic
        EXTRA_CXXFLAGS += -fsanitize=address -fsanitize=undefined
        EXTRA_LDFLAGS  += -fsanitize=address -fsanitize=undefined -pthread
    else
        with_cov := 0
        ifneq ($(call pos,coverage,$(MAKECMDGOALS)),-1)
            with_cov := 1
        endif

        ifneq ($(with_cov),0)
            out_dir := coverage
            EXTRA_CXXFLAGS += --coverage
            EXTRA_LDFLAGS  += --coverage
        else
            EXTRA_CXXFLAGS := -g
            out_dir := .
        endif
    endif
endif

-include config.mk

CXXFLAGS := -I$(out_dir)/src/ $(CXXFLAGS)

# traverse directories ($1) recursively looking for a pattern ($2) to make list
# of matching files
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) \
                                        $(filter $(subst *,%,$2),$d))

lib := $(out_dir)/lib$(NAME).a

lib_sources := $(call rwildcard, src/, *.cpp) \
               $(call rwildcard, third-party/, *.cpp)
lib_sources := $(filter-out %.gen.cpp,$(lib_sources))

lib_autocpp := $(addprefix $(out_dir)/src/c/, \
                           c11-lexer.gen.cpp c11-parser.gen.cpp)
lib_autocpp += $(addprefix $(out_dir)/src/make/, \
                           make-lexer.gen.cpp make-parser.gen.cpp)
lib_autohpp := $(addprefix $(out_dir)/src/c/, c11-lexer.hpp c11-parser.hpp)
lib_autohpp += $(addprefix $(out_dir)/src/make/, make-lexer.hpp make-parser.hpp)

lib_objects := $(sort $(lib_sources:%.cpp=$(out_dir)/%.o) \
                      $(lib_autocpp:%.cpp=%.o))
lib_depends := $(lib_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_depends := $(tests_objects:%.o=%.d)
tests_objects += $(lib)

all:

# includes tool-specific configuration that disables linking rule
define pull_tool_template
-include tools/$1/tool.mk
endef

# tool definition template, takes single argument: name of the tool
define tool_template

$1.bin := $(out_dir)/zs-$1$(bin_suffix)
$1.sources := $$(filter-out tools/$1/data/%, \
                            $$(call rwildcard, tools/$1/, *.cpp) \
                            $$(call rwildcard, tools/$1/, *.c))
$1.objects := $$($1.sources:%.cpp=$$(out_dir)/%.o)
$1.objects := $$(sort $$($1.objects:%.c=$$(out_dir)/%.o))
$1.depends := $$($1.objects:.o=.d)
$1.objects += $(lib)

tools_bins += $$($1.bin)
tools_objects += $$($1.objects)
tools_depends += $$($1.depends)

all: $$($1.bin)

$$($1.bin): | $(out_dirs)

ifeq (,$(wildcard tools/$1/tool.mk))
$$($1.bin): $$($1.objects)
	$(CXX) $(EXTRA_LDFLAGS) $$^ $(LDFLAGS) -o $$@
else
$$($1.bin): $$($1.sources) $$($1.extradeps) $(lib)
endif

endef

# includes ammending tool-specific configuration
define pull_tool_config_template
-include tools/$1/tool.cfg.mk
endef

tools := $(patsubst tools/%/,%,$(sort $(dir $(wildcard tools/*/*.cpp))))
$(foreach tool, $(tools), $(eval $(call pull_tool_template,$(tool))))
$(foreach tool, $(tools), $(eval $(call tool_template,$(tool))))
$(foreach tool, $(tools), $(eval $(call pull_tool_config_template,$(tool))))

out_dirs := $(sort $(dir $(lib_objects) $(tools_objects) $(tests_objects)))
# this is for gmake 3, which has troubles creating these directories in the
# processs of running other rules
$(shell mkdir -p $(out_dirs))

.PHONY: all check clean debug release sanitize-basic
.PHONY: coverage reset-coverage
.PHONY: install uninstall

all: $(lib)

debug release sanitize-basic: all

coverage: check $(all)
	uncov new-gcovi --capture-worktree --exclude tests \
	                                   --exclude third-party \
	                                   --exclude tools/tui/libs

reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(lib): | $(out_dirs)

$(lib): $(lib_objects)
	$(AR) cr $@ $^

check: $(out_dir)/tests/tests reset-coverage
	@$(out_dir)/tests/tests $(TESTS)

install: release
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) $(tools_bins) $(DESTDIR)$(PREFIX)/bin

uninstall:
	$(RM) $(addprefix $(DESTDIR)$(PREFIX)/bin/,$(notdir $(tools_bins)))

$(out_dir)/src/c/c11-lexer.hpp: $(out_dir)/src/c/c11-lexer.gen.cpp
$(out_dir)/src/c/c11-lexer.gen.cpp: src/c/c11-lexer.flex \
                                | $(out_dir)/src/c/c11-parser.gen.cpp \
                                  $(out_dir)/src/c/c11-parser.hpp
	flex --header-file=$(out_dir)/src/c/c11-lexer.hpp \
	     --outfile=$(out_dir)/src/c/c11-lexer.gen.cpp $<

$(out_dir)/src/make/make-lexer.hpp: $(out_dir)/src/make/make-lexer.gen.cpp
$(out_dir)/src/make/make-lexer.gen.cpp: src/make/make-lexer.flex \
                                | $(out_dir)/src/make/make-parser.gen.cpp \
                                  $(out_dir)/src/make/make-parser.hpp
	flex --header-file=$(out_dir)/src/make/make-lexer.hpp \
	     --outfile=$(out_dir)/src/make/make-lexer.gen.cpp $<

$(out_dir)/src/c/c11-parser.hpp: $(out_dir)/src/c/c11-parser.gen.cpp
$(out_dir)/src/c/c11-parser.gen.cpp: src/c/c11-parser.ypp
	bison --defines=$(out_dir)/src/c/c11-parser.hpp \
	      --output=$(out_dir)/src/c/c11-parser.gen.cpp $<

$(out_dir)/src/make/make-parser.hpp: $(out_dir)/src/make/make-parser.gen.cpp
$(out_dir)/src/make/make-parser.gen.cpp: src/make/make-parser.ypp
	bison --defines=$(out_dir)/src/make/make-parser.hpp \
	      --output=$(out_dir)/src/make/make-parser.gen.cpp $<

# to make build possible the first time, when dependency files aren't there yet
$(lib_objects): | $(lib_autohpp)

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Itests/
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -DCATCH_CLARA_TEXTFLOW_CONFIG_CONSOLE_WIDTH=999
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

$(out_dir)/%.gen.o: $(out_dir)/%.gen.cpp | $(out_dirs)
	$(CXX) -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $< -o $@

$(out_dir)/%.o: %.cpp | $(out_dirs)
	$(CXX) -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $< -o $@

$(out_dir)/%.o: %.c | $(out_dirs)
	$(CC) -c $(CFLAGS) $(EXTRA_CXXFLAGS) $< -o $@

$(out_dirs):
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/ sanitize-basic/
	-$(RM) $(lib_objects) $(tools_objects) $(tests_objects) \
	       $(lib_depends) $(tools_depends) $(tests_depends) \
	       $(lib_autocpp) $(lib_autohpp) \
	       $(lib) $(tools_bins) $(out_dir)/tests/tests

include $(wildcard $(lib_depends) $(tools_depends) $(tests_depends))
