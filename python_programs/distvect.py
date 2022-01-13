import argparse
import networkx
import os
import pickle
import networkx as nx

def load_pgr(filename):
	PGR = None
	with open(filename, "rb") as fp:
		PGR = pickle.load(fp)
	return PGR


def main():
	parser = argparse.ArgumentParser(description="Calculates the distance vector for each PGR" + \
												" and creates an order AS list file.")
	parser.add_argument("--pgr_dir",
		type=str,
		help="Directory containing subdirectories for each period containing PGRs",
		required=True)
	parser.add_argument("--output_dir",
		type=str,
		help="Directory to store subdirectories for each period containing distance vectors",
		required=True)
	parser.add_argument("--node_list_file",
		type=str,
		help="Filename to store ordered node list",
		required=True)
	parser.add_argument("--periods",
		type=int,
		help="Number of time periods",
		required=True)

	args = parser.parse_args()
	pgr_dir = args.pgr_dir
	output_dir = args.output_dir
	node_list_file = args.node_list_file
	periods = args.periods

	distance_vects = {} ### key = (collector, start_time), value = list of distances w -1 = not in graph
	all_nodes = set()
	all_route_collectors = set()

	for tp in range(periods):

		### create all nodes list
		### and all route collectors list
		for filename in os.listdir(pgr_dir + "/" + str(tp)):
			PGR = load_pgr(pgr_dir + "/" + str(tp) + "/" + filename)
			all_route_collectors.add(PGR["collector"])
			for source, target_list in PGR["G"].items():
				for target in target_list:
					s = int(source)
					t = int(target)
					if(s != -1):
						all_nodes.add(s)
					if(t != -1):
						all_nodes.add(t)

	all_nodes_list = sorted(list(all_nodes))
	all_route_collectors_list = sorted(list(all_route_collectors))

	### calculate distance vectors
	for tp in range(periods):
		for filename in os.listdir(pgr_dir + "/" + str(tp)):
			PGR = load_pgr(pgr_dir + "/" + str(tp) + "/" + filename)
			G = nx.Graph()
			for source, target_list in PGR["G"].items():
				for target in target_list:
					s = int(source)
					t = int(target)
					G.add_edge(s,t)
			D = nx.shortest_path_length(G, source = -1)
			dist_vect = []
			for n in all_nodes_list:
				try:
					dist_vect.append(D[n])
				except:
					dist_vect.append(-1)
			distance_vects[(PGR["collector"], tp)] = dist_vect

	### save distance vects
	for key, value in distance_vects.items():
		collector = key[0]
		tp = key[1]
		if not os.path.exists(output_dir + "/{}/".format(tp)):
			os.makedirs(output_dir + "/{}/".format(tp))
		with open("{}/{}/{}.dv".format(output_dir, tp, collector), "wb") as fp:
			pickle.dump(value, fp)

	### save all nodes list
	with open("{}".format(node_list_file), "wb") as fp:
		pickle.dump(all_nodes_list, fp)


if __name__ == '__main__':
	main()