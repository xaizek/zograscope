objects := $(wildcard *.cpp)

$(bin): $(objects)
	$(CXX) $$^ -o $$@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ -c $(CXXFLAGS) $<
