#pragma once
#include <stdlib.h>
#include <string>

struct Options {
	std::string pgr_file;
	std::string class_q_file_prefix;
	std::string alpha_beta_file;
	int num_files;
	int num_collectors;
	double rho;
	int num_sims;
};

Options parse_args(int argc, char** argv);
int check_args(Options opt);