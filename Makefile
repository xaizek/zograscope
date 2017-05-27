CXXFLAGS += -std=c++11 -Wall -Wextra -Werror -MMD -I$(abspath src)
LDFLAGS  += -g -lboost_iostreams

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
                EXTRA_CXXFLAGS := -O3
                out_dir := .
            endif
        endif
    endif
    target := debug
endif

# traverse directories ($1) recursively looking for a pattern ($2) to make list
# of matching files
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) \
                                        $(filter $(subst *,%,$2),$d))

bin := $(out_dir)/tc$(bin_suffix)

bin_sources := $(call rwildcard, src/, *.cpp)
bin_autocpp := $(addprefix $(out_dir)/src/, c11-lexer.cpp c11-parser.cpp)
bin_autohpp := $(addprefix $(out_dir)/src/, c11-lexer.hpp c11-parser.hpp)
bin_objects := $(sort $(bin_sources:%.cpp=$(out_dir)/%.o) \
                      $(bin_autocpp:%.cpp=%.o))
bin_depends := $(bin_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_objects += $(filter-out %/diffpp.o,$(bin_objects))
tests_depends := $(tests_objects:%.o=%.d)

out_dirs := $(sort $(dir $(bin_objects) $(tests_objects)))

.PHONY: all check clean debug release sanitize-basic man install uninstall
.PHONY: coverage reset-coverage

all: $(bin)

debug release sanitize-basic: all

coverage: check $(bin)
	find $(out_dir)/ -name '*.o' -exec gcov -p {} + > $(out_dir)/gcov.out \
	|| (cat $(out_dir)/gcov.out && false)
	uncov-gcov --root . --no-gcov --capture-worktree --exclude tests \
	| uncov new
	find . -name '*.gcov' -delete

man: docs/tc.1
# the next target doesn't depend on $(wildcard docs/*.md) to make pandoc
# optional
docs/tc.1: force | $(out_dir)/docs
	pandoc -V title=tc \
	       -V section=1 \
	       -V app=tc \
	       -V footer="tc v0.1" \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@openmailbox.org>' \
	       -s -o $@ $(sort $(wildcard docs/*.md))

# target that doesn't exist and used to force rebuild
force:

reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(bin): | $(out_dirs)

$(bin): $(bin_objects)
	$(CXX) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

check: $(target) $(out_dir)/tests/tests reset-coverage
	@$(out_dir)/tests/tests

install: release
	$(INSTALL) -t $(DESTDIR)/usr/bin/ $(bin)
	$(INSTALL) -m 644 docs/tc.1 $(DESTDIR)/usr/share/man/man1/tc.1

uninstall:
	$(RM) $(DESTDIR)/usr/bin/$(basename $(bin)) \
	      $(DESTDIR)/usr/share/man/man1/tc.1
	$(RM) -r $(DESTDIR)/usr/share/tc/

$(out_dir)/src/c11-lexer.hpp: $(out_dir)/src/c11-lexer.cpp
$(out_dir)/src/c11-lexer.cpp: src/c11-lexer.flex | $(out_dir)/src/c11-parser.hpp
	flex --header-file=$(out_dir)/src/c11-lexer.hpp \
	     --outfile=$(out_dir)/src/c11-lexer.cpp $<

$(out_dir)/src/c11-parser.hpp: $(out_dir)/src/c11-parser.cpp
$(out_dir)/src/c11-parser.cpp: src/c11-parser.ypp
	bison --defines=$(out_dir)/src/c11-parser.hpp \
	      --output=$(out_dir)/src/c11-parser.cpp $<

# to make build possible the first time, when dependency files aren't there yet
$(bin_objects): $(bin_autohpp)

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses -Itests/
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) -o $@ $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS)

$(out_dir)/%.o: %.cpp | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/
	-$(RM) $(bin_objects) $(tests_objects) \
	       $(bin_depends) $(tests_depends) \
	       $(bin_autocpp) $(bin_autohpp) \
	       $(bin) $(out_dir)/tests/tests

include $(wildcard $(bin_depends) $(tests_depends))
