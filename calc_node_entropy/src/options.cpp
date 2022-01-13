#include "options.hpp"

Options parse_args(int argc, char** argv) {
	Options o;

	//set defaults
	o.pgr_file = "NONE";
	o.class_q_file_prefix = "NONE";
	o.num_files = -1;
	o.num_collectors = -1;
	o.rho = -1.0;

	for(int i = 1; i < argc; i++) {
		std::string argument = argv[i];
		std::string indicator = "--";
		if(argument.find(indicator) != std::string::npos) {
			if(argument == "--pgr_file") {
				o.pgr_file = argv[i+1];
			}
			if(argument == "--class_q_file_prefix") {
				o.class_q_file_prefix = argv[i+1];
			}
			if(argument == "--num_files") {
				o.num_files = atoi(argv[i+1]);
			}
			if(argument == "--num_collectors") {
				o.num_collectors = atoi(argv[i+1]);
			}
			if(argument == "--rho") {
				o.rho = atof(argv[i+1]);
			}
		}
	}
	return o;
}

int check_args(Options opt) {
	int result = 1;
	if(opt.pgr_file == "NONE") {
		printf("Please provide a pgr_file (--pgr_file)\n");
		result = 0;
	}
	if(opt.class_q_file_prefix == "NONE") {
		printf("Please provide a class_q_file_prefix (--class_q_file_prefix)\n");
		result = 0;
	}
	if(opt.num_files == -1) {
		printf("Please specify num_files (--num_files)\n");
		result = 0;
	}
	if(opt.num_collectors == -1) {
		printf("Please specify num_collectors (--num_collectors)\n");
		result = 0;
	}
	if(opt.rho < 0.0) {
		printf("Please specify a rho value (--rho)\n");
		result = 0;
	}
	return result;
}