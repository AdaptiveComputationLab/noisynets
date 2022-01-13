#pragma once
#include <stdlib.h>
#include <string>

struct Options {
	std::string edge_list_file;
	std::string pgr_file;
	std::string class_q_file_prefix;
	int num_files;
	int num_collectors;
	int num_periods;
	double rho;
};

Options parse_args(int argc, char** argv);
int check_args(Options opt);