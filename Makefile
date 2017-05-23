SRC := tc.cpp tc.tab.cpp diffpp.cpp decoration.cpp tree-edit-distance.cpp types.cpp Printer.cpp integration.cpp
OBJ := $(SRC:.cpp=.o)

tc: $(OBJ)
	g++ -g -o $@ $^ -lboost_iostreams

%.o: %.cpp
	g++ -MMD -Wall -Wextra -Werror -fmax-errors=3 -DYYDEBUG=1 -O3 -std=c++11 -c -g -o $@ $<

tc.cpp: tc.flex | tc.tab.hpp
	flex -o $@ $<

tc.tab.hpp tc.tab.cpp: tc.ypp
	bison -d $<

include $(wildcard *.d)
