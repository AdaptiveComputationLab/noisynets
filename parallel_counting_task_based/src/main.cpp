#include "count.hpp"
#include "options.hpp"

//

// This is a function to be used in an rpc by workers to tell
// the main rank that the worker is ready to work again
master_message set_ready(upcxx::dist_object< upcxx::global_ptr<long>> &fui_g,
						upcxx::dist_object< upcxx::global_ptr<long>> &tn_g,
						upcxx::dist_object< upcxx::global_ptr<int>> &cs_g) {

	master_message msg;
	msg.finished = false;
	msg.start_index = 0;
	msg.end_index = 0;

	long* local_fui = fui_g->local();
	long* local_tn = tn_g->local();
	int* local_cs = cs_g->local();

	if(*(local_fui) >= *(local_tn)) {
		msg.finished = true;
		return msg;
	}
	msg.start_index = *(local_fui);
	if(*(local_fui) + *(local_cs) >= *(local_tn)) {
		msg.end_index = *(local_tn) - 1;
	} else {
		msg.end_index = *(local_fui) + *(local_cs);
	}

	*(local_fui) += *(local_cs);

	return msg;
}

int main(int argc, char** argv) {

	// Set up UPC++
	upcxx::init();
	int my_rank = upcxx::rank_me();

	//parse options
	Options opt = parse_args(argc, argv);

	if(opt.input_file == "NONE" || opt.output_directory == "NONE") {
		if(my_rank == 0) {
			std::cout << "Please provide an input file and output directory\n";
		}
		return 0;
	}

	double t0 = omp_get_wtime();
	Dataset* data = load_dataset(opt.input_file);
	double t1 = omp_get_wtime();
	printf("Rank %d: Read a dataset with %lu nodes and %d PGRs in %f seconds\n", my_rank, data->node_list.size(),
		data->num_collectors*data->num_periods, t1 - t0);
	fflush(stdout);

	if(opt.max_periods == -1) {
		opt.max_periods = data->num_periods;
	}

	t0 = omp_get_wtime();
	for(int t = 0; t < opt.max_periods; t++) {
		for(int k = 0; k < data->num_collectors; k++) {
			data->PGRs[t][k]->bfs(-1);
		}
	}
	t1 = omp_get_wtime();
	printf("Rank %d: Completed BFS in %f seconds!\n",my_rank, t1 - t0);
	fflush(stdout);

	//determine total number of edges to count
	int total_nodes = (int)(data->node_list.size());
	long total_edges = ((long)total_nodes*((long)total_nodes - 1))/2;
	if(my_rank == 0){
		printf("Rank %d: There are %ld edges to count...\n",my_rank, total_edges);
		fflush(stdout);
	}

	// We will use distributed pointers to allow workers
	// to interact with the master's queue
	upcxx::dist_object< upcxx::global_ptr<long> > fui_g(upcxx::new_<long>(0));
	upcxx::dist_object< upcxx::global_ptr<long> > tn_g(upcxx::new_<long>(0));
	upcxx::dist_object< upcxx::global_ptr<int> > cs_g(upcxx::new_<int>(0));

	long* local_fui = fui_g->local();
	long* local_tn = tn_g->local();
	int* local_cs = cs_g->local();


	//some set up on rank 0's shared memory
	if(my_rank == 0) {
		*(local_fui) = 0;
		*(local_tn) = total_edges;
		*(local_cs) = 10000000;
	}

	// // for testing!
	// if(my_rank == 0) {
	// 	*(local_fui) = 0;
	// 	*(local_tn) = 1000000;
	// 	*(local_cs) = 100000;
	// }

	// wait for initilization on all ranks
	upcxx::barrier();

	//worker loop
	bool finished = false;
	// ClassCountsMap_sparse* ccm = new ClassCountsMap_sparse();
	ClassCountsMap* ccm = new ClassCountsMap();
	while(!finished) {

		// figure out what to work on
		upcxx::future<master_message> msg_result = upcxx::rpc(0,
															set_ready,
															fui_g,
															tn_g,
															cs_g);
		master_message msg = msg_result.wait();
		if(msg.finished) {

			printf("Rank %d: Received message that work was finished...\n",
			my_rank);
			fflush(stdout);

			finished = false;
			break;
		}
		printf("Rank %d: Received message to work form %ld to %ld\n",
			my_rank, msg.start_index, msg.end_index);
		fflush(stdout);
		count_classes_inplace(msg.start_index,msg.end_index,data,ccm,opt);
		double progress = 100.0 * ((double)(msg.end_index) / (double)(total_edges));
		printf("Rank %d: progress: %f\n",my_rank, progress);
	}

	//wait to finish work
	upcxx::barrier();

	printf("Rank %d: Saving classes to file\n",my_rank);
	fflush(stdout);

	t0 = omp_get_wtime();
	// dump_classes_to_file((*ccm), data, output_directory + "_" + std::to_string(my_rank) + ".cls");
	dump_classes_to_file((*ccm), opt.output_directory + "_" + std::to_string(my_rank) + ".cls");
	t1 = omp_get_wtime();

	printf("Rank %d: Wrote file in %f seconds!\n",my_rank, t1-t0);
	fflush(stdout);
	
	//free memory
	delete ccm;
	delete data;

	upcxx::finalize();

	return 0;
}