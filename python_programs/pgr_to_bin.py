import pickle
import argparse
import os
import time

def itob(a, sign=False):
	return a.to_bytes(4,"little",signed=sign)

def itob_long(a, sign=False):
	return a.to_bytes(8,"little",signed=sign)

def main():
	help_str = "Reads PGR files and creates a single binary" +\
	"input file for class counting"
	parser = argparse.ArgumentParser(description=help_str)
	parser.add_argument("--dir",
		type=str,
		help="Directory containing PGR files stored in labeled time period directories",
		required=True)
	parser.add_argument("--output_file",
		type=str,
		help="File to store result",
		required=True)
	parser.add_argument("--T",
		type=int,
		help="number of time periods",
		required=True)

	t0 = time.time()

	args = parser.parse_args()
	pgr_dir = args.dir
	output_file = args.output_file
	num_periods = args.T

	print("Found {} time periods...".format(num_periods))

	PGRs = {}
	collectors = set()
	all_nodes = set()

	for T in range(num_periods):
		for filename in os.listdir("{}/{}/".format(pgr_dir, T)):
			with open("{}/{}/{}".format(pgr_dir,T,filename),"rb") as fp:
				PGR = pickle.load(fp)
				new_G = {} ## for making sure an undirected graph is reported
				for source, target_list in PGR["G"].items():
					for target in target_list:
						s = int(source)
						t = int(target)
						if(s != -1):
							all_nodes.add(s)
						if(t != -1):
							all_nodes.add(t)

						try:
							new_G[source].append(target)
						except:
							new_G[source] = [target]
						try:
							new_G[target].append(source)
						except:
							new_G[target] = [source]


				PGR["period"] = T
				PGR["G"] = new_G
				PGRs[(T, PGR["collector"])] = PGR
				collectors.add(PGR["collector"])

	num_collectors = len(collectors)
	collector_list = sorted(list(collectors))
	node_list = sorted(list(all_nodes))
	num_nodes = len(node_list)

	### write dataset to file
	with open(output_file, "wb") as fp:
		fp.write(itob(num_nodes))
		fp.write(itob(num_periods))
		fp.write(itob(num_collectors))
		for n in node_list:
			fp.write(itob_long(n, sign=True))
		for T in range(num_periods):
			for k in range(num_collectors):
				PGR = PGRs[(T,collector_list[k])]
				num_sources = len(PGR["G"].keys())
				fp.write(itob(num_sources))
				for source, target_list in PGR["G"].items():
					fp.write(itob_long(source, sign=True))
					num_targets = len(target_list)
					fp.write(itob(num_targets))
					for t in target_list:
						fp.write(itob_long(t, sign=True))

	t1 = time.time()

	print("Converted in {} seconds".format(t1-t0))

if __name__ == "__main__":
	main()