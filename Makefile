tc: tc.cpp tc.tab.cpp | tc.tab.hpp
	g++ -fmax-errors=3 -DYYDEBUG=1 -std=c++11 -o $@ $^ -lfl

tc.cpp: tc.flex
	flex -o $@ $<

tc.tab.hpp tc.tab.cpp: tc.ypp
	bison -d $<
