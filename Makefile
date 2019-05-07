.DEFAULT_GOAL=sparrow

CXX = g++
CXXFLAGS := -O3 --std=c++14

sparrow: *.cpp *.h
	$(CXX) $(CXXFLAGS) *.cpp -o $@

clean:
	rm sparrow
