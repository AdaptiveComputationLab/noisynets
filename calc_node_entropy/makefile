inc_path=/home/kirtus/Projects/bgpobsrepo/bgpobs/calc_node_entropy/include
boost_p=/home/kirtus/Software/boost/boost_install/include

all:
	g++ -fopenmp -I${inc_path} -I${boost_p} src/options.cpp src/count.cpp src/main.cpp -o node_ent.out -O3
debug:
	g++ -fopenmp -I${inc_path} -I${boost_p} src/options.cpp src/count.cpp src/main.cpp -o node_ent.out -g
clean:
	rm *.out
