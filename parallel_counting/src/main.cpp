#include "count.hpp"

int main(int argc, char** argv) {

	// Set up UPC++
	upcxx::init();
	int my_rank = upcxx::rank_me();

	std::string input_file = argv[1];
	std::string output_directory = argv[2];

	double t0 = omp_get_wtime();
	Dataset* data = load_dataset(input_file);
	double t1 = omp_get_wtime();
	printf("Rank %d: Read a dataset with %lu nodes and %d PGRs in %f seconds\n", my_rank, data->node_list.size(),
		data->num_collectors*data->num_periods, t1 - t0);
	fflush(stdout);

	t0 = omp_get_wtime();
	for(int t = 0; t <  data->num_periods; t++) {
		for(int k = 0; k < data->num_collectors; k++) {
			data->PGRs[t][k]->bfs(-1);
		}
	}
	t1 = omp_get_wtime();
	printf("Rank %d: Completed BFS in %f seconds!\n",my_rank, t1 - t0);
	fflush(stdout);

	// counting classes
	ClassCountsMap* ccm;

	int nodes_per_rank = (int)(data->node_list.size()/upcxx::rank_n());
	int start = nodes_per_rank*my_rank;
	int end = start + nodes_per_rank;
	if(my_rank == upcxx::rank_n()-1) { end += data->node_list.size()%upcxx::rank_n();}
	t0 = omp_get_wtime();
	ccm = count_classes(start,end,data);
	t1 = omp_get_wtime();
	printf("Rank %d: Computed classes over %d nodes in %f seconds!\n",my_rank, end - start, t1-t0);
	fflush(stdout);

	printf("Rank %d: Saving classes to file",my_rank);
	fflush(stdout);

	t0 = omp_get_wtime();
	dump_classes_to_file((*ccm), output_directory + "_" + std::to_string(my_rank) + ".cls");
	t1 = omp_get_wtime();

	printf("Rank %d: Wrote file in %f seconds!\n",my_rank, t1-t0);
	fflush(stdout);
	
	//free memory
	delete ccm;
	delete data;

	upcxx::finalize();

	return 0;
}