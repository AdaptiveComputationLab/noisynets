#pragma once

#ifndef RECORD_H
#define RECORD_H
#include <json.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#endif

using json = nlohmann::json;

class ObservationRecord {
	public:
		int count; // how many AS pairs fall into this class?
		// E[k] -> number of positive observations for kth collector
		std::vector<int> E;
		// F[k] -> number of negative observations for the kth collector
		std::vector<int> F;
};

class ObservationDataset {

	public:
		int num_classes;
		int num_collectors;
		int num_nodes;
		std::vector<ObservationRecord*> records;
		~ObservationDataset();

};

// for serial loads
ObservationDataset* read_records(std::string class_file);

// for parallel loads
ObservationDataset* read_records(std::string class_file, int my_rank, int num_ranks);

// reading dense binary instead of json
ObservationDataset* read_records_bin(std::string class_file, int my_rank, int num_ranks);