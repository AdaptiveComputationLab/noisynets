#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <algorithm>
#include <boost/container_hash/hash.hpp>
#include <upcxx/upcxx.hpp>
#include <omp.h>

int inline status(long node_u, long node_v,
				std::unordered_map<long, std::unordered_set<long>> &G,
				std::unordered_map<long, int> &D) {
	if(D[node_v] == -1 || D[node_u] == -1) {
		return 2;
	}
	auto it_v = G[node_u].find(node_v);
	if(it_v != G[node_u].end()) {
		return 1;
	}
	auto it_u = G[node_v].find(node_u);
	if(it_u != G[node_v].end()) {
		return 1;
	}
	if(abs(D[node_u] - D[node_v]) < 2) {
		return 2;
	}
	return 0;
}

// for storing records
struct vector_hash {
	std::size_t operator()(std::vector<int> const& c) const {
		return boost::hash_range(c.begin(), c.end());
	}
};

struct ClassCountsMap {
	std::unordered_map<std::vector<int>,int,vector_hash> class_map;
};

void dump_classes_to_file(ClassCountsMap &ccm, std::string output_file);

class PGR {
	public:
		std::unordered_map<long, std::unordered_set<long>> positive_graph;
		std::unordered_map<long, int> node_distances;

		void bfs(long source);
};

class Dataset {
	public:
		int num_periods;
		int num_collectors;
		PGR** *PGRs; //PGR[t][k] -> the PGR of the kth collector at time t
		std::vector<long> node_list;
		~Dataset();
};

Dataset* load_dataset(std::string filename);

ClassCountsMap* count_classes(int begin, int end, Dataset* data);