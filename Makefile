NAME := jposed

CXXFLAGS += -std=c++11 -Wall -Wextra -Werror -MMD -Isrc/ -Ithird-party/
CXXFLAGS += -DYYDEBUG
LDFLAGS  += -g -lboost_iostreams -lboost_program_options

INSTALL := install -D

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
ifneq ($(call pos,install,$(MAKECMDGOALS)),-1)
    is_release := 1
endif
ifneq ($(is_release),0)
    EXTRA_CXXFLAGS := -O3
    # EXTRA_LDFLAGS  := -Wl,--strip-all

    out_dir := release
    target  := release
else
    EXTRA_CXXFLAGS := -O0 -g
    EXTRA_LDFLAGS  := -g

    ifneq ($(call pos,debug,$(MAKECMDGOALS)),-1)
        out_dir := debug
    else
        ifneq ($(call pos,sanitize-basic,$(MAKECMDGOALS)),-1)
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
                EXTRA_CXXFLAGS := -Og -g
                out_dir := .
            endif
        endif
    endif
    target := debug
endif

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
lib_autohpp := $(addprefix $(out_dir)/src/c/, c11-lexer.hpp c11-parser.hpp)
lib_objects := $(sort $(lib_sources:%.cpp=$(out_dir)/%.o) \
                      $(lib_autocpp:%.cpp=%.o))
lib_objects := $(filter-out %/main.o,$(lib_objects))
lib_depends := $(lib_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_depends := $(tests_objects:%.o=%.d)
tests_objects += $(lib)

# tool definition template, takes single argument: name of the tool
define tool_template

$1.bin := $(out_dir)/$1$(bin_suffix)
$1.sources := $$(call rwildcard, tools/$1/, *.cpp)
$1.objects := $$(sort $$($1.sources:%.cpp=$$(out_dir)/%.o))
$1.depends := $$($1.objects:.o=.d)
$1.objects += $(lib)

tools_bins += $$($1.bin)
tools_objects += $$($1.objects)
tools_depends += $$($1.depends)

all: $$($1.bin)

$$($1.bin): | $(out_dirs)

$$($1.bin): $$($1.objects)
	$(CXX) -o $$@ $$^ $(LDFLAGS) $(EXTRA_LDFLAGS)

endef

tools := $(subst tools/,,$(sort $(wildcard tools/*)))
$(foreach tool, $(tools), $(eval $(call tool_template,$(tool))))

out_dirs := $(sort $(dir $(lib_objects) $(tools_objects) $(tests_objects)))

.PHONY: all check clean debug release sanitize-basic man install uninstall
.PHONY: coverage reset-coverage

all: $(lib)

debug release sanitize-basic: all

coverage: check $(all)
	find $(out_dir)/ -name '*.o' -exec gcov -p {} + > $(out_dir)/gcov.out \
	|| (cat $(out_dir)/gcov.out && false)
	uncov-gcov --root . --no-gcov --capture-worktree --exclude tests \
	    --exclude third-party | uncov new
	find . -name '*.gcov' -delete

man: docs/$(NAME).1
# the next target doesn't depend on $(wildcard docs/*.md) to make pandoc
# optional
docs/$(NAME).1: force | $(out_dir)/docs
	pandoc -V title=$(NAME) \
	       -V section=1 \
	       -V app=$(NAME) \
	       -V footer="$(NAME) v0.1" \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@posteo.net>' \
	       -s -o $@ $(sort $(wildcard docs/*.md))

# target that doesn't exist and used to force rebuild
force:

reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(lib): | $(out_dirs)

$(lib): $(lib_objects)
	$(AR) cr $@ $^

check: $(target) $(out_dir)/tests/tests reset-coverage
	@$(out_dir)/tests/tests

install: release
	$(INSTALL) -t $(DESTDIR)/usr/bin/ $(bin)
	$(INSTALL) -m 644 docs/$(NAME).1 $(DESTDIR)/usr/share/man/man1/$(NAME).1

uninstall:
	$(RM) $(DESTDIR)/usr/bin/$(basename $(bin)) \
	      $(DESTDIR)/usr/share/man/man1/$(NAME).1
	$(RM) -r $(DESTDIR)/usr/share/$(NAME)/

$(out_dir)/src/c/c11-lexer.hpp: $(out_dir)/src/c/c11-lexer.gen.cpp
$(out_dir)/src/c/c11-lexer.gen.cpp: src/c/c11-lexer.flex \
                                | $(out_dir)/src/c/c11-parser.gen.cpp \
                                  $(out_dir)/src/c/c11-parser.hpp
	flex --header-file=$(out_dir)/src/c/c11-lexer.hpp \
	     --outfile=$(out_dir)/src/c/c11-lexer.gen.cpp $<

$(out_dir)/src/c/c11-parser.hpp: $(out_dir)/src/c/c11-parser.gen.cpp
$(out_dir)/src/c/c11-parser.gen.cpp: src/c/c11-parser.ypp
	bison --defines=$(out_dir)/src/c/c11-parser.hpp \
	      --output=$(out_dir)/src/c/c11-parser.gen.cpp $<

# to make build possible the first time, when dependency files aren't there yet
$(lib_objects): | $(lib_autohpp)

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses -Itests/
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) -o $@ $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS)

$(out_dir)/%.gen.o: $(out_dir)/%.gen.cpp | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dir)/%.o: %.cpp | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/ sanitize-basic/
	-$(RM) $(lib_objects) $(tools_objects) $(tests_objects) \
	       $(lib_depends) $(tools_depends) $(tests_depends) \
	       $(lib_autocpp) $(lib_autohpp) \
	       $(lib) $(tools_bins) $(out_dir)/tests/tests

include $(wildcard $(tools_depends) $(tests_depends))
