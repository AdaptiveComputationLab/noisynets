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
#include <random>
#include <chrono>
#include <unordered_map>
#include <cmath>

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

	printf("#Reading Alphas and Betas...\n");
	std::cout << std::flush;
	std::vector<double> alphas;
	std::vector<double> betas;
	std::ifstream fp(opt.alpha_beta_file.c_str());
	std::string line;
	while(std::getline(fp, line))
	{
		std::istringstream ss(line);
		double a, b;
		ss >> a >> b;
		alphas.push_back(a);
		betas.push_back(b);
	}
	printf("#Finished Reading Alphas and Betas\n");
	std::cout << std::flush;

	///rng
	typedef std::chrono::high_resolution_clock myclock;
	myclock::time_point beginning = myclock::now();
	myclock::duration d = myclock::now() - beginning;
	unsigned seed = d.count();
	std::uniform_real_distribution<double> unif(0.0,1.0);
	std::default_random_engine rng;
	rng.seed(seed);

	//store unoreded map of difference counts per thread
	//note, i've hard coded 28 threads, !!!!
	int num_threads = 28;
	std::unordered_map<int, long> diff_count[num_threads];

	#pragma omp parallel for schedule(dynamic, 1000) num_threads(28)
	for(unsigned int i = 0; i < num_nodes; i++) {
		std::printf("#%d\n",i);
		for(unsigned int j = i+1; j < num_nodes; j++) {
			std::vector<int> record = get_edge_class(data->node_list[i],
													data->node_list[j],
													data);

			int positive_in_data = 0;
			std::vector<int> trials;
			for(unsigned int ii = 0; ii < opt.num_collectors; ii++) {
				positive_in_data += record[ii];
				trials.push_back( record[ii] + record[opt.num_collectors + ii] );
			}

			auto it = class_qs.find(record);
			double Q = opt.rho;
			if(it == class_qs.end()) {
				printf("#WARN: Couldn't find class for edge %ld, %ld\n",
				data->node_list[i], data->node_list[j]);
			} else {
				Q = class_qs[record];
			}
			std::vector<int> sims(opt.num_sims, 0);
			for(unsigned int ii = 0; ii < opt.num_sims; ii++) {
				int exists = 0;
				double roll = unif(rng);
				if(roll < Q) {
					exists = 1;
				}
				if(exists) {
					for(unsigned int k = 0; k < alphas.size(); k++) {
						for(unsigned int t = 0; t < trials[k]; t++) {
							roll = unif(rng);
							if(roll <  alphas[k]) {
								sims[ii] += 1;
							}
						}
					}
				} else {
					for(unsigned int k = 0; k < betas.size(); k++) {
						for(unsigned int t = 0; t < trials[k]; t++) {
							roll = unif(rng);
							if(roll <  betas[k]) {
								sims[ii] += 1;
							}
						}
					}
				}

				int diff = positive_in_data - sims[ii];
				int thread_id = omp_get_thread_num();
				if(diff_count[thread_id].find(diff) != diff_count[thread_id].end()){
					diff_count[thread_id][diff] += 1;
				} else {
					diff_count[thread_id].insert( std::make_pair(diff, 1) );
				}

			}
		}
	}
	#pragma omp barrier

	std::unordered_map<int, long> combined;
	for(int i = 0; i < num_threads; i++) {
		combined.insert(diff_count[i].begin(), diff_count[i].end());
	}

	//print results
	for( auto it : combined) {
		printf("%d %ld\n",it.first, it.second);
	}

	//free mem
	delete data;

	return 0;
}