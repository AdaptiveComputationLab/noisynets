#include "count.hpp"
#include "options.hpp"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>

//Calculates entropy for a single node pair
// given Q_ij and rho.
double calc_D(double Q, double rho) {
	double value = 0.0;
	double eps = 10e-8;
	if(Q < eps) {
		value = (1.0 - Q)*log( (1.0 - Q) / (1.0 - rho) );
	}
	else if((1.0 - Q) < eps) {
		value = Q*log(Q/rho);
	}
	else {
		value = ( (1.0 - Q)*log( (1.0 - Q) / (1.0 - rho) ) )+ ( Q*log(Q/rho) );
	}
	return value;
}

double calc_H(double Q) {
	double value = 0.0;
	double eps = 10e-8;
	if(Q < eps) {
		return (-1.0)*( (1 - Q)*log(1 - Q) );
	}
	else if((1.0 - Q) < eps) {
		return (-1.0)*(Q*log(Q));
	}
	else {
		return ( (-1.0)*( (1 - Q)*log(1 - Q) ) ) + ((-1.0)*(Q*log(Q)));
	}
	return value;
}

int main(int argc, char** argv) {

	// params
	Options opt = parse_args(argc, argv);
	if(!check_args(opt)) {
		printf("Bad Arguments!\n");
		return 0;
	}

	//read in class_q files
	printf("#Reading classes...\n");
	std::cout << std::flush;
	std::unordered_map<std::vector<int>,double,vector_hash> class_qs;
	for(int i = 0; i < opt.num_files; i++) {
		std::ostringstream fname;
		fname << opt.class_q_file_prefix << "_" << std::setw(4) << std::setfill('0') << i;
		std::ifstream fp(fname.str().c_str());
		std::string line;
		std::getline(fp, line); //skip the header
		while(std::getline(fp, line)) {

			std::stringstream ss(line);
			// class_idx
			int class_idx;
			ss >> class_idx;
			if(ss.peek() == ',') {ss.ignore();}
			// class_couint
			int class_count;
			ss >> class_count;
			if(ss.peek() == ',') {ss.ignore();}
			//class_Q
			double Q;
			ss >> Q;
			if(ss.peek() == ',') {ss.ignore();}

			std::vector<int> record;

			for(int val; ss >> val;) {
				record.push_back(val);
				if(ss.peek() == ',') {ss.ignore();}
			}
			std::pair< std::vector<int>, double > p(record, Q);
			class_qs.insert(p);
		}
		fp.close();
	}
	printf("#Finished Reading classes...\n");
	std::cout << std::flush;

	//read in PGRs
	printf("#Reading PGRs...\n");
	std::cout << std::flush;
	Dataset* data = load_dataset(opt.pgr_file);
	printf("#Finished Reading PGRs\n");
	std::cout << std::flush;

	printf("#Running BFS...\n");
	std::cout << std::flush;
	#pragma omp parallel for
	for(int t = 0; t <  data->num_periods; t++) {
		for(int k = 0; k < data->num_collectors; k++) {
			data->PGRs[t][k]->bfs(-1);
		}
	}
	printf("#Finished Running BFS...\n");
	std::cout << std::flush;

	unsigned int num_nodes = data->node_list.size();
	std::vector<double> node_entropy(num_nodes, 0.0f);

	#pragma omp parallel for schedule(dynamic, 1000)
	for(unsigned int i = 0; i < num_nodes; i++) {
		printf("#%d\n",i);
		std::cout << std::flush;
		for(unsigned int j = 0; j < num_nodes; j++) {
			if(i != j) {
				std::vector<int> record = get_edge_class(data->node_list[i],
														data->node_list[j],
														data);
				auto it = class_qs.find(record);
				if(it == class_qs.end()) {
					printf("#WARN: Couldn't find class for edge %ld, %ld\n",
					data->node_list[i], data->node_list[j]);
				} else {
					double Q = class_qs[record];
					double H = calc_H(Q);
					node_entropy[i] += H;
				}
			}
		}
	}

	//print results
	for(unsigned int i = 0; i < num_nodes; i++) {
		printf("%ld,%f\n",data->node_list[i],node_entropy[i]);
	}

	//free mem
	delete data;

	return 0;
}