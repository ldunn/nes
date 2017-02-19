CXX=clang++
CXXFLAGS=-g -std=c++1y

nes: nes.cpp
	$(CXX) $(CXXFLAGS) nes.cpp -o nes