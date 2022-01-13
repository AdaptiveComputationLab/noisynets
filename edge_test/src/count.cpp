#include "count.hpp"

Dataset* load_dataset(std::string filename) {

	Dataset* data = new Dataset();

	std::ifstream input_file(filename, std::ios::binary);
	int num_nodes;
	int num_periods;
	int num_collectors;

	unsigned int int_value = 0;
	signed long long_value = 0;

	input_file.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));
	input_file.read(reinterpret_cast<char*>(&num_periods), sizeof(num_periods));
	input_file.read(reinterpret_cast<char*>(&num_collectors), sizeof(num_collectors));

	data->num_collectors = num_collectors;
	data->num_periods = num_periods;

	//read node list
	for(int i = 0; i < num_nodes; i++) {
		input_file.read(reinterpret_cast<char*>(&long_value), sizeof(long_value));
		data->node_list.push_back(long_value);
	}

	//read graphs
	data->PGRs = new PGR**[num_periods];
	for(int T = 0; T < num_periods; T++) {
		data->PGRs[T] = new PGR*[num_collectors];
		for(int k = 0; k < num_collectors; k++) {
			PGR* PGR_ = new PGR();

			//fill PGR_s node_distances list with <node, -1> pairs
			for(long node : data->node_list) {
				std::pair<long, int> p(node, -1);
				PGR_->node_distances.insert(p);
			}

			int num_sources;
			input_file.read(reinterpret_cast<char*>(&num_sources), sizeof(num_sources));
			for(int s = 0; s < num_sources; s++) {
				//read source ASN
				signed long source_asn;
				input_file.read(reinterpret_cast<char*>(&source_asn), sizeof(source_asn));
				int num_targets;
				input_file.read(reinterpret_cast<char*>(&num_targets), sizeof(num_targets));
				std::unordered_set<long> adj;
				std::pair<long, std::unordered_set<long>> key_value(source_asn, adj);
				PGR_->positive_graph.insert(key_value);
				for(int t = 0; t < num_targets; t++) {
					signed long target_asn;
					input_file.read(reinterpret_cast<char*>(&target_asn), sizeof(target_asn));
					PGR_->positive_graph.at(source_asn).insert(target_asn);
				}
			}

			data->PGRs[T][k] = PGR_;

		}
	}

	input_file.close();

	return data;
}

Dataset::~Dataset() {
	for(int t = 0; t < num_periods; t++) {
		for(int k = 0; k < num_collectors; k++) {
			delete PGRs[t][k];
		}
		delete[] PGRs[t];
	}
	delete [] PGRs;
}

// performs BFS on the positive_graph starting from source
void PGR::bfs(long source) {

	std::queue<long> frontier;

	node_distances[source] = 0;
	long current_node = source;
	for(long next : positive_graph[current_node]) {
		if(node_distances[next] == -1) {
			node_distances[next] = node_distances[current_node] + 1;
			frontier.push(next);
		}
	}

	while(!frontier.empty()) {

		current_node = frontier.front();
		frontier.pop();

		for(long next : positive_graph[current_node]) {
			if(node_distances[next] == -1) {
				node_distances[next] = node_distances[current_node] + 1;
				frontier.push(next);
			}
		}
	}
}

std::vector<int> get_edge_class(long asn_a, long asn_b, Dataset* data, int num_periods) {
	std::vector<int> record(2*data->num_collectors);
	std::fill( record.begin(), record.end(), 0);
	for(int k = 0; k < data->num_collectors; k++) {
		for(int t = 0; t < num_periods; t++) {
			int stat = status(asn_a, asn_b,
							data->PGRs[t][k]->positive_graph,
							data->PGRs[t][k]->node_distances);
			if(stat == 0){
				record[data->num_collectors + k] += 1;
			}
			else if(stat == 1) {
				record[k] += 1;
			}
		}
	}
	return record;
}