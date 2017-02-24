CXX=clang++
CXXFLAGS=-g -std=c++1y -lsfml-graphics -lsfml-window -lsfml-system -I. -O3

nes: nes.cpp cpu.cpp ppu.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@
