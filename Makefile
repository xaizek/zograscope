tc: tc.cpp
	g++ -o $@ $< -lfl

tc.cpp: tc.flex
	flex -o $@ $<
