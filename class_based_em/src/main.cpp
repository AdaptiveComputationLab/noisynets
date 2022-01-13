#include "record.hpp"
#include "em.hpp"
#include "options.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <upcxx/upcxx.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

void print_step(std::vector<double> alpha,
	std::vector<double> beta,
	double rho,
	double D,
	double H,
	int step) {

	std::ostringstream ss;
	ss << step << ",";
	ss << rho << ",";
	ss << D << ",";
	ss << H;
	for(int k = 0; k < alpha.size(); k++) {
		ss << "," << alpha[k];
	}
	for(int k = 0; k < beta.size(); k++) {
		ss << "," << beta[k];
	}
	std::cout << ss.str() << "\n";
}

void print_header(int num_collectors) {
	std::ostringstream ss;
	ss << "step" << ",";
	ss << "rho" << ",";
	ss << "D" << ",";
	ss << "H";
	for(int k = 0; k < num_collectors; k++) {
		ss << "," << "alpha_" << k;
	}
	for(int k = 0; k < num_collectors; k++) {
		ss << "," << "beta_" << k;
	}
	std::cout << ss.str() << "\n";
}

int main(int argc, char **argv) {

	//startup upc++
	upcxx::init();

	//handling options
	Options opt = parse_args(argc, argv);

	if(opt.class_file == "NONE") {
		if(upcxx::rank_me() == 0) {
			std::cout << "# NO CLASS FILE PROVIDED!\n";
		}
		return 0;
	}

	if(upcxx::rank_me() == 0){
		std::cout << "#Running EM with options:\n";
		std::cout << "#class_file " << opt.class_file << "\n";
		std::cout << "#init_alpha " << opt.init_alpha << "\n";
		std::cout << "#init_beta " << opt.init_beta << "\n";
		std::cout << "#init_rho " << opt.init_rho << "\n";\
		std::cout << "#max_iters " << opt.max_iters << "\n";
		std::cout << "#randomize_rcs " << opt.randomize_rcs << "\n";
		std::cout << "#num_rand_rcs " << opt.num_random_collectors << "\n";
		std::cout << "#seed " << opt.seed << "\n";
		std::cout << "#output_file " << opt.output_file << "\n";
	}

	ObservationDataset* dataset;
	//fields for defining rank bounds
	int classes_per_rank;
	int extra_classes;

	//Get local dataset on each rank
	dataset = read_records_bin(opt.class_file, upcxx::rank_me(), upcxx::rank_n());
	if(upcxx::rank_me() == 0) {
		printf("#Saw %d nodes...\n", dataset->num_nodes);
	}
	printf("# Rank % d Read class file...\n", upcxx::rank_me());
	printf("# Rank % d %d classes...\n", upcxx::rank_me(),dataset->num_classes);
	printf("# Rank % d %d collectors...\n", upcxx::rank_me(),dataset->num_collectors);
	upcxx::barrier();

	// set up collector indeces for randomization
	std::vector<int> rc_idxs;
	for(int i = 0; i < dataset->num_collectors; i++) {
		rc_idxs.push_back(i);
	}
	if(opt.randomize_rcs) {
		std::default_random_engine rng(opt.seed);
		std::shuffle(rc_idxs.begin(), rc_idxs.end(), rng);
		std::string order = "";
		for(unsigned int i = 0; i < dataset->num_collectors; i++) {
			order += std::to_string(rc_idxs[i]) + ",";
		}
		order += "\n";
		printf("#rc order: %s\n",order.c_str());
	} else {
		opt.num_random_collectors = dataset->num_collectors;
	}

	// Set up initial estimates
	std::vector<double> Q (dataset->num_classes);
	std::vector<double> alpha (opt.num_random_collectors);
	std::vector<double> beta (opt.num_random_collectors);
	double rho = opt.init_rho;
	double all_rho = opt.init_rho;
	double D = 0.0;
	double all_D = 0.0;
	double H = 0.0;
	double all_H = 0.0;

	std::fill(Q.begin(), Q.end(), 0.0);
	std::fill(alpha.begin(), alpha.end(), opt.init_alpha);
	std::fill(beta.begin(), beta.end(), opt.init_beta);

	//rank 0 prints results
	if(upcxx::rank_me() == 0) {
		print_header(opt.num_random_collectors);
		print_step(alpha, beta, rho, D, H, 0);
	}

	for(int step = 0; step < opt.max_iters; step++){
		//perform an update step
		//calc Q
		Q = calc_Q(alpha, beta, rho, dataset, opt.num_random_collectors,
			rc_idxs);

		//calc Alpha
		std::vector<double> alpha_num = calc_alpha_numerator(Q, dataset,
															opt.num_random_collectors,
															rc_idxs);
		std::vector<double> alpha_denom = calc_alpha_denominator(Q, dataset,
																opt.num_random_collectors,
																rc_idxs);

		//calc Beta
		std::vector<double> beta_num = calc_beta_numerator(Q, dataset,
																opt.num_random_collectors,
																rc_idxs);
		std::vector<double> beta_denom = calc_beta_denominator(Q, dataset,
																opt.num_random_collectors,
																rc_idxs);

		//calc rho
		rho = calc_rho_term(Q);

		upcxx::barrier();
		// combine results
		std::vector<double> all_alpha_num (opt.num_random_collectors);
		std::fill(all_alpha_num.begin(), all_alpha_num.end(), 0.0);

		std::vector<double> all_alpha_denom (opt.num_random_collectors);
		std::fill(all_alpha_denom.begin(), all_alpha_denom.end(), 0.0);

		std::vector<double> all_beta_num (opt.num_random_collectors);
		std::fill(all_beta_num.begin(), all_beta_num.end(), 0.0);

		std::vector<double> all_beta_denom (opt.num_random_collectors);
		std::fill(all_beta_denom.begin(), all_beta_denom.end(), 0.0);
		
		for(int k = 0; k < opt.num_random_collectors; k++) {
			all_alpha_num[k] = upcxx::reduce_all(alpha_num[k], upcxx::op_fast_add).wait();
			all_alpha_denom[k] = upcxx::reduce_all(alpha_denom[k], upcxx::op_fast_add).wait();
			all_beta_num[k] = upcxx::reduce_all(beta_num[k], upcxx::op_fast_add).wait();
			all_beta_denom[k] = upcxx::reduce_all(beta_denom[k], upcxx::op_fast_add).wait();
			alpha[k] = all_alpha_num[k]/all_alpha_denom[k];
			beta[k] = all_beta_num[k]/all_beta_denom[k];
		}
		all_rho = upcxx::reduce_all(rho, upcxx::op_fast_add).wait();
		double n_choose_2 = dataset->num_nodes*(dataset->num_nodes - 1.0)/2.0;
		rho = (1.0/n_choose_2)*all_rho;

		//calc entropy
		D = calc_D(Q, dataset, rho);
		H = calc_H(Q, dataset, rho);
		upcxx::barrier();


		all_D = upcxx::reduce_all(D, upcxx::op_fast_add).wait();
		all_H = upcxx::reduce_all(H, upcxx::op_fast_add).wait();
		all_H = all_H/n_choose_2;
		H = all_H;
		D = all_D;
		//rank 0 prints results
		if(upcxx::rank_me() == 0) {
			print_step(alpha, beta, rho, D, H, step+1);
		}
		upcxx::barrier();
	}

	//write Q to file for each rank
	if(opt.output_file != "NONE") {
		std::ofstream qfile;
		std::ostringstream outname;
		int my_rank = upcxx::rank_me();
		outname << opt.output_file << "_" << std::setw(4) << std::setfill('0') << my_rank;
		printf("# Rank %d writing to file %s ...\n", upcxx::rank_me(), outname.str().c_str());
		qfile.open(outname.str());
		std::string to_write = "";
		//write header
		to_write += "class_idx,class_count,class_Q";
		for(int k = 0; k < dataset->num_collectors; k++) {
			to_write += ",yes_" + std::to_string(k);
		}
		for(int k = 0; k < dataset->num_collectors; k++) {
			to_write += ",no_" + std::to_string(k);
		}
		to_write += "\n";
		for(int i = 0; i < dataset->num_classes; i++){
			to_write += std::to_string(i);
			to_write += "," + std::to_string( dataset->records[i]->count );
			to_write += "," + std::to_string( Q[i] );
			for(int k = 0; k < dataset->num_collectors; k++) {
			to_write += "," + std::to_string(dataset->records[i]->E[k]);
			}
			for(int k = 0; k < dataset->num_collectors; k++) {
				to_write += "," + std::to_string(dataset->records[i]->F[k]);
			}
			to_write += "\n";
		}
		qfile << to_write;
		qfile.close();
	}

	// free local memory
	delete dataset;

	upcxx::finalize();

	return 0;
}