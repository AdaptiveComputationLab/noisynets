all:
	upcxx -fopenmp -Iinclude src/count.cpp src/main.cpp -o count.out -O3
debug:
	upcxx -fopenmp -Iinclude src/count.cpp src/main.cpp -o count.out -g
clean:
	rm *.out
