#include "count.hpp"
#include "options.hpp"

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

ClassCountsMap*
count_classes(int start,
			int end,
			Dataset* data) {

	ClassCountsMap* ccm = new ClassCountsMap();

	int work = end - start;
	int progress = 0;
	int my_rank = upcxx::rank_me();

	for(int i = start; i < end; i++) {
		if(progress%1000 == 0) {
			printf("Rank %d: Progress: %d/%d = %f percent\n",my_rank,progress,
				work,
				100.0*float(progress)/float(work));
		}
		progress += 1;
		for(int j = i+1; j < data->node_list.size(); j++) {
			long node_a = data->node_list[i];
			long node_b = data->node_list[j];

			std::vector<int> record (2*data->num_collectors);
			std::fill(record.begin(), record.end(), 0);

			for(int k = 0; k < data->num_collectors; k++) {
				for(int t = 0; t < data->num_periods; t++) {
					int stat = status(node_a, node_b,
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

			auto it = ccm->class_map.find(record);
			if(it != ccm->class_map.end()) {
				ccm->class_map[record] += 1;
			} else {
				ccm->class_map[record] = 1;
			}
		}
	}
	return ccm;
}

void count_classes_inplace(long start,
			long end,
			Dataset* data,
			ClassCountsMap* ccm,
			Options opt) {

	int work = end - start;
	int progress = 0;
	int my_rank = upcxx::rank_me();

	for(long idx = start; idx < end; idx++) {

		long n = (long)(data->node_list.size());
		long comb = n*(n-1);
		long comb_temp = (-8*idx + 4*comb-7);
		int i = n - 2 - (int)( floor( sqrtl(comb_temp)/2.0 - 0.5 ));
		int j = (int)( idx + i + 1 - n*(n-1)/2 + (n-i)*((n-i)-1)/2 );

		long node_a = data->node_list[i];
		long node_b = data->node_list[j];

		std::vector<int> record (2*data->num_collectors);
		std::fill(record.begin(), record.end(), 0);

		for(int k = 0; k < data->num_collectors; k++) {
			for(int t = 0; t < opt.max_periods; t++) {
				int stat = status(node_a, node_b,
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

		auto it = ccm->class_map.find(record);
		if(it != ccm->class_map.end()) {
			ccm->class_map[record] += 1;
		} else {
			ccm->class_map[record] = 1;
		}
	}
}

//sparse record version
void count_classes_inplace(long start,
			long end,
			Dataset* data,
			ClassCountsMap_sparse* ccm) {

	int work = end - start;
	int progress = 0;
	int my_rank = upcxx::rank_me();

	for(long idx = start; idx < end; idx++) {

		long n = (long)(data->node_list.size());
		long comb = n*(n-1);
		long comb_temp = (-8*idx + 4*comb-7);
		int i = n - 2 - (int)( floor( sqrtl(comb_temp)/2.0 - 0.5 ));
		int j = (int)( idx + i + 1 - n*(n-1)/2 + (n-i)*((n-i)-1)/2 );

		long node_a = data->node_list[i];
		long node_b = data->node_list[j];

		obs_record record;

		for(int k = 0; k < data->num_collectors; k++) {
			for(int t = 0; t < data->num_periods; t++) {
				int stat = status(node_a, node_b,
								data->PGRs[t][k]->positive_graph,
								data->PGRs[t][k]->node_distances);
				if(stat == 0){

					auto it = find(record.nz.begin(),record.nz.end(),k);
					if(it != record.nz.end()) {
						int index = it - record.nz.begin();
						record.no[index] += 1;
					} else {
						record.nz.push_back(k);
						record.no.push_back(1);
						record.yes.push_back(0);
					}

				}
				else if(stat == 1) {
					auto it = find(record.nz.begin(),record.nz.end(),k);
					if(it != record.nz.end()) {
						int index = it - record.nz.begin();
						record.yes[index] += 1;
					} else {
						record.nz.push_back(k);
						record.yes.push_back(1);
						record.no.push_back(0);
					}
				}
			}
		}

		auto it = ccm->class_map.find(record);
		if(it != ccm->class_map.end()) {
			ccm->class_map[record] += 1;
		} else {
			ccm->class_map[record] = 1;
		}
	}
}

void dump_classes_to_file(ClassCountsMap &ccm, std::string output_file) {
	std::ofstream ofp;
	ofp.open(output_file);
	for(auto it : ccm.class_map) {
		std::string result = "";
		result += std::to_string(it.second) + ",";
		for(int i = 0; i < it.first.size() - 1; i++) {
			result += std::to_string(it.first[i]) + ",";
		}
		result += std::to_string(it.first[it.first.size()-1]) + "\n";
		ofp << result;
	}
	ofp.close();
}

void dump_classes_to_file(ClassCountsMap_sparse &ccm,
							Dataset* data,
							std::string output_file) {
	std::ofstream ofp;
	ofp.open(output_file);
	int num_collectors = data->num_collectors;
	for(auto it : ccm.class_map) {
		std::string result = "";
		result += std::to_string(it.second) + ",";
		std::vector<int> nz = it.first.nz;
		std::vector<int> yes = it.first.yes;
		std::vector<int> no = it.first.no;

		std::vector<int> record(2*num_collectors);
		std::fill(record.begin(), record.end(), 0);

		for(int i = 0; i < nz.size(); i++) {
			record[nz[i]] = yes[i];
			record[nz[i] + num_collectors] = no[i];
		}

		for(int i = 0; i < record.size() - 1; i++) {
			result += std::to_string(record[i]) + ",";
		}
		result += std::to_string(record[record.size()-1]) + "\n";
		ofp << result;
	}
	ofp.close();
}