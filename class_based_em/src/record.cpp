#include "record.hpp"

ObservationDataset* read_records(std::string class_file) {
	std::ifstream input_file(class_file);
	json input_json;
	input_file >> input_json;

	int num_classes = (int)input_json["num_classes"];
	int num_collectors = (int)input_json["num_collectors"];
	int num_nodes = (int)input_json["num_nodes"];

	ObservationDataset* dataset = new ObservationDataset();
	dataset->num_classes = num_classes;
	dataset->num_collectors = num_collectors;

	for(int i = 0; i < num_classes; i++) {
		ObservationRecord* record = new ObservationRecord();
		record->count = (int)input_json["class_counts"][i];
		for(int k = 0; k < num_collectors; k++) {
			record->E.push_back((int)input_json["E"][i][k]);
			record->F.push_back((int)input_json["F"][i][k]);
		}
		dataset->records.push_back(record);
	}

	return dataset;
}

ObservationDataset* read_records(std::string class_file, int my_rank, int num_ranks) {
	std::ifstream input_file(class_file);
	json input_json;
	input_file >> input_json;

	int total_classes = (int)input_json["num_classes"];
	int num_collectors = (int)input_json["num_collectors"];
	int num_nodes = (int)input_json["num_nodes"];

	int classes_per_rank = total_classes/(num_ranks);
	int extra_classes = total_classes%(num_ranks);
	int my_start = classes_per_rank*my_rank;
	int my_end = classes_per_rank*my_rank + classes_per_rank;
	if(my_rank == num_ranks - 1) {
		my_end += extra_classes; //in case processes do not evenly divide classes
		classes_per_rank += extra_classes;
	}

	ObservationDataset* dataset = new ObservationDataset();
	dataset->num_classes = classes_per_rank;
	dataset->num_collectors = num_collectors;
	dataset->num_nodes = num_nodes;

	for(int i = my_start; i < my_end; i++) {
		ObservationRecord* record = new ObservationRecord();
		record->count = (int)input_json["class_counts"][i];
		for(int k = 0; k < num_collectors; k++) {
			record->E.push_back((int)input_json["E"][i][k]);
			record->F.push_back((int)input_json["F"][i][k]);
		}
		dataset->records.push_back(record);
	}

	return dataset;
}

ObservationDataset* read_records_bin(std::string class_file, int my_rank, int num_ranks) {

	std::ifstream input_file(class_file, std::ios::binary);
	
	//read header
	int total_classes = 0;
	int num_collectors = 0;
	int num_nodes = 0;

	input_file.read(reinterpret_cast<char*>(&total_classes), sizeof(total_classes));
	input_file.read(reinterpret_cast<char*>(&num_collectors), sizeof(num_collectors));
	input_file.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));

	int classes_per_rank = total_classes/(num_ranks);
	int extra_classes = total_classes%(num_ranks);
	int my_start = classes_per_rank*my_rank;
	int my_end = classes_per_rank*my_rank + classes_per_rank;
	if(my_rank == num_ranks - 1) {
		my_end += extra_classes; //in case processes do not evenly divide classes
		classes_per_rank += extra_classes;
	}

	ObservationDataset* dataset = new ObservationDataset();
	dataset->num_classes = classes_per_rank;
	dataset->num_collectors = num_collectors;
	dataset->num_nodes = num_nodes;

	//move stream to my location of the file.
	int value = 0;
	input_file.seekg((3 + (1 + 2*num_collectors)*my_start)*sizeof(int), std::ios::beg);

	for(int i = my_start; i < my_end; i++) {
		ObservationRecord* record = new ObservationRecord();
		// read class counts
		input_file.read(reinterpret_cast<char*>(&value), sizeof(value));
		record->count = value;
		for(int k = 0; k < num_collectors; k++) {
			input_file.read(reinterpret_cast<char*>(&value), sizeof(value));
			record->E.push_back(value);
			input_file.read(reinterpret_cast<char*>(&value), sizeof(value));
			record->F.push_back(value);
		}
		dataset->records.push_back(record);
	}

	return dataset;
}

ObservationDataset::~ObservationDataset() {
	for(int i = 0; i < records.size(); i++) {
		delete records[i];
	}
}