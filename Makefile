tc: tc.cpp tc.tab.cpp diffpp.cpp decoration.cpp tree-edit-distance.cpp | tc.tab.hpp
	g++ -fmax-errors=3 -DYYDEBUG=1 -std=c++11 -g -o $@ $^ -lfl

tc.cpp: tc.flex
	flex -o $@ $<

tc.tab.hpp tc.tab.cpp: tc.ypp
	bison -d $<
