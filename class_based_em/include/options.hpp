#pragma once
#include <stdlib.h>
#include <string>

struct Options {
	std::string class_file;
	double init_alpha;
	double init_beta;
	double init_rho;
	int max_iters;
	int randomize_rcs;
	int num_random_collectors;
	unsigned int seed;
	std::string output_file;
};

Options parse_args(int argc, char** argv) {
	Options o;

	//set defaults
	o.class_file = "NONE";
	o.init_alpha = 0.9;
	o.init_beta = 0.1;
	o.init_rho = 0.1;
	o.max_iters = 10;
	o.randomize_rcs = 0;
	o.num_random_collectors = 1;
	o.seed = 1;
	o.output_file = "NONE";

	for(int i = 1; i < argc; i++) {
		std::string argument = argv[i];
		std::string indicator = "--";
		if(argument.find(indicator) != std::string::npos) {
			if(argument == "--class_file") {
				o.class_file = argv[i+1];
			}
			if(argument == "--init_alpha") {
				o.init_alpha = atof(argv[i+1]);
			}
			if(argument == "--init_beta") {
				o.init_beta = atof(argv[i+1]);
			}
			if(argument == "--init_rho") {
				o.init_beta = atof(argv[i+1]);
			}
			if(argument == "--max_iters") {
				o.max_iters = atoi(argv[i+1]);
			}
			if(argument == "--randomize_rcs") {
				o.randomize_rcs = atoi(argv[i+1]);
			}
			if(argument == "--num_random_collectors") {
				o.num_random_collectors = atoi(argv[i+1]);
			}
			if(argument == "--seed") {
				o.seed = atoi(argv[i+1]);
			}
			if(argument == "--output_file") {
				o.output_file = argv[i+1];
			}
		}
	}


	return o;
}