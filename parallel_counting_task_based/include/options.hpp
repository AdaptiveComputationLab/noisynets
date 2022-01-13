#pragma once
#include <stdlib.h>
#include <string>

struct Options {
	std::string input_file;
	std::string output_directory;
	int max_periods;
};

Options parse_args(int argc, char** argv);