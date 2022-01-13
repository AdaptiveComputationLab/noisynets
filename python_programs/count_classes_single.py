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

def count_classes(start, end, PGRs, all_nodes_list, rc_list, time_periods, process_id):
	
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
	work = end - start
	print("Working from {} to {}...".format(start, end), flush=True)
	observation_records = {}
	steps = 0
	for i in range(start, end):
		if steps%100 == 0:
			progress = 100.0*float(steps)/float(work)
			et = time.time()
			print("{}: Progress: {} of {} nodes, {} percent, {:.2f} seconds...".format(process_id, steps,
				work,
				progress,
				et-st), flush=True)
		steps += 1
		for j in range(i+1, len(all_nodes_list)):
			node_a = all_nodes_list[i]
			node_b = all_nodes_list[j]
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
				observation_records[rt] += 1
			except:
				observation_records[rt] = 1

	return observation_records

def main():
	parser = argparse.ArgumentParser(description="Counts Observation Records using Multiprocessing")
	parser.add_argument("--start",
		metavar="S",
		type=int,
		help="First node to work on",
		required=True)
	parser.add_argument("--end",
		metavar="E",
		type=int,
		help="Last node +1 to work on",
		required=True)
	parser.add_argument("--proc_id",
		metavar="P",
		type=int,
		help="Process ID",
		required=True)
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
	start = args.start
	end = args.end
	proc_id = args.proc_id
	outputFile = args.output
	periods = args.periods

	PGRs = {} ### key = (collector, time_period) tuple, value = PGR
	all_nodes = set()
	route_collectors = set()
	route_collectors_nodes = {}

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

	observation_records = count_classes(start, end, PGRs, all_nodes_list, route_collectors_list, periods, proc_id)

	print("Saving Output")
	with open("{}.{}".format(outputFile,proc_id), "wb") as fp:
		pickle.dump(observation_records, fp)

if __name__ == '__main__':
	main()