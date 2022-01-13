import json
import numpy as np
import networkx as nx
import time
import os
import argparse
from multiprocessing import Pool, cpu_count
from collections import Counter
import pickle

def load_pgr(filename):
	PGR = None
	with open(filename, "rb") as fp:
		PGR = pickle.load(fp)
	return PGR

def get_classes(PGRs, all_nodes_list, rc_list, time_periods, edge_set):
	
	#   0: def not an edge
	#   1: def an edge
	#   2: dunno
	def status(u,v,G,D):
		### node not seen in graph, so idk
		if u not in D or v not in D:
			return 2
		if u in G:
			if v in G[u]:
				return 1
		if v in G:
			if u in G[v]:
				return 1
		if abs(D[u]-D[v])<2: return 2
		return 0
	
	st = time.time()
	work = len(edge_set)
	print("Working on {} edges...".format(work), flush=True)
	class_dict = {}
	steps = 0
	for e in edge_set:
		if steps%100 == 0:
			progress = 100.0*float(steps)/float(work)
			et = time.time()
			print("Progress: {} of {} edges, {} percent, {:.2f} seconds...".format(steps,
				work,
				progress,
				et-st), flush=True)
		steps += 1
		node_a = e[0]
		node_b = e[1]
		record = [0]*len(rc_list)*2
		for k in range(len(rc_list)):
			for tp in range(time_periods):
				PGR = PGRs[(rc_list[k],tp)]
				stat = status(node_a, node_b, PGR["G"], PGR["D"])
				if stat == 0: ### def no
					record[len(rc_list) + k] += 1
				elif stat == 1:
					record[k] += 1
		rt = tuple(record)
		try:
			class_dict[rt].append(e)
		except:
			class_dict[rt] = []

	return class_dict

def main():
	parser = argparse.ArgumentParser(description="Creates class->edge dict")
	parser.add_argument("--periods",
		metavar="T",
		type=int,
		help="Number of Time Periods to Include",
		default=1,
		required=True)
	parser.add_argument("--dir",
		metavar="directory",
		type=str,
		help="Directory containing PGR files",
		required=True)
	parser.add_argument("--output",
		metavar="outputFile",
		type=str,
		help="File to store output pickled dictionary",
		required=True)


	args = parser.parse_args()
	dataset_dir = args.dir
	outputFile = args.output
	periods = args.periods

	PGRs = {} ### key = (collector, time_period) tuple, value = PGR
	all_nodes = set()
	route_collectors = set()
	route_collectors_nodes = {}

	edge_set = set()

	for tp in range(periods):
		
		### Generate distance_dict
		### and create all_nodes_list
		for filename in os.listdir(dataset_dir + "/" + str(tp)):
			PGR = load_pgr(dataset_dir + "/" + str(tp) + "/" + filename)
			G = nx.Graph()
			route_collectors.add(PGR["collector"])
			for source, target_list in PGR["G"].items():
				for target in target_list:
					s = int(source)
					t = int(target)
					G.add_edge(s, t)
					if(s != -1 and t != -1):
						edge_set.add(tuple(sorted([s, t])))
					if(s != -1):
						all_nodes.add(s)
						try:
							route_collectors_nodes[PGR["collector"]].add(s)
						except:
							route_collectors_nodes[PGR["collector"]] = {s}
					if(t != -1):
						all_nodes.add(t)
						try:
							route_collectors_nodes[PGR["collector"]].add(t)
						except:
							route_collectors_nodes[PGR["collector"]] = {t}
			D = nx.shortest_path_length(G, source = -1)
			PGR["D"] = D
			PGRs[(PGR["collector"], tp)] = PGR
	route_collectors_list = sorted(list(route_collectors))
	all_nodes_list = sorted(list(all_nodes))
	print("{} nodes total...".format(len(all_nodes_list)))

	class_dict = get_classes(
		PGRs,
		all_nodes_list,
		route_collectors_list,
		periods,
		edge_set)

	print("Saving Output")
	with open("{}".format(outputFile), "wb") as fp:
		pickle.dump(class_dict, fp)

if __name__ == '__main__':
	main()