CXX := g++

objects := $(wildcard src/*.cpp)

$(bin): $(objects)
	$(CXX) $(LDFLAGS) $$^ -o $$@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
