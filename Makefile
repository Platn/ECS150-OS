All: main

main: main.cpp
	g++ main.cpp -o sample.o
sample.o: CXXFLAGS += -w
