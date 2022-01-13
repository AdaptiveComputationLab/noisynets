#include "options.hpp"

Options parse_args(int argc, char** argv) {
	Options o;

	//set defaults
	o.input_file = "NONE";
	o.output_directory = "NONE";
	o.max_periods = -1;

	for(int i = 1; i < argc; i++) {
		std::string argument = argv[i];
		std::string indicator = "--";
		if(argument.find(indicator) != std::string::npos) {
			if(argument == "--input_file") {
				o.input_file = argv[i+1];
			}
			if(argument == "--output_directory") {
				o.output_directory = argv[i+1];
			}
			if(argument == "--max_periods") {
				o.max_periods = atoi(argv[i+1]);
			}
		}
	}


	return o;
}