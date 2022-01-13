#include "count.hpp"
#include "options.hpp"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>

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

	//read in edge file
	printf("#Reading Edges...\n");
	std::cout << std::flush;
	std::set< std::pair<long,long> > edges;
	std::ifstream fp(opt.edge_list_file.c_str());
	std::string line;
	while(std::getline(fp, line))
	{
		std::istringstream ss(line);
		long a, b;
		ss >> a >> b;
		if(a < b) {
			std::pair<long, long> p(a,b);
			edges.insert(p);
		}
		else if(b < a) {
			std::pair<long, long> p(b,a);
			edges.insert(p);
		}
	}
	fp.close();
	printf("#Finished Reading Edges...\n");

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

	// #pragma omp parallel for schedule(dynamic, 1000)
	// for(unsigned int i = 0; i < edges.size(); i++) {
	// 	std::vector<int> record = get_edge_class(edges[i].first,
	// 		edges[i].second, data, opt.num_periods);
	// 	auto it = class_qs.find(record);
	// 	if(it == class_qs.end()) {
	// 		edge_qs[i] = opt.rho;
	// 		printf("#WARN: Couldn't find class for edge %ld, %ld\n",
	// 				edges[i].first, edges[i].second);
	// 		std::cout << std::flush;
	// 	} else {
	// 		edge_qs[i] = class_qs[record];
	// 	}
	// }

	double log_q = 0.0;
	double prec_num = 0.0;
	double prec_denom = 0.0;
	double recall_num = 0.0;
	double recall_denom = 0.0;

	double eps = 10e-8;

	unsigned int num_nodes = data->node_list.size();
	#pragma omp parallel for schedule(dynamic, 1000) reduction(+:log_q,prec_num,prec_denom,recall_num,recall_denom)
	for(unsigned int i = 0; i < num_nodes; i++) {
		printf("#%d\n",i);
		std::cout << std::flush;
		for(unsigned int j = i+1; j < num_nodes; j++) {
			std::vector<int> record = get_edge_class(data->node_list[i],
													data->node_list[j],
													data, opt.num_periods);
			//get edge Q
			auto it = class_qs.find(record);
			double Q = opt.rho;
			if(it == class_qs.end()) {
				printf("#WARN: Couldn't find class for edge %ld, %ld\n",
					data->node_list[i], data->node_list[j]);
			} else {
				Q = class_qs[record];
			}

			//check if it is included in the reconstruction
			double A = 0.0;
			std::pair<long, long> edge(-1,-1);
			if(data->node_list[i] < data->node_list[j]) {
				edge.first = data->node_list[i];
				edge.second = data->node_list[j];
			} else if (data->node_list[i] > data->node_list[j]) {
				edge.first = data->node_list[j];
				edge.second = data->node_list[i];
			} else {
				edge.first = data->node_list[i];
				edge.second = data->node_list[j];
			}
			auto it_edges = edges.find( edge );
			if(it_edges != edges.end()) {
				A = 1.0;
			}

			//calculate reduction values
			double eQ = Q;
			if(eQ < eps) {
				eQ = eps;
			}
			else if((1.0 - eQ) < eps) {
				eQ = 1.0 - eps;
			}

			prec_num += A*eQ;
			prec_denom += A;

			recall_num += A*eQ;
			recall_denom += eQ;

		}
	}

	//print results:
	printf("log_q = %.8f\n", log_q);
	printf("prec_num = %.8f\n", prec_num);
	printf("prec_denom = %.8f\n", prec_denom);
	printf("recall_num = %.8f\n", recall_num);
	printf("recall_denom = %.8f\n", recall_denom);
	printf("prec(A) = %.8f\n", prec_num/prec_denom);
	printf("rec(A) = %.8f\n",recall_num/recall_denom);

	//free mem
	delete data;

	return 0;
}