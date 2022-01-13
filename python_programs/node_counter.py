import pickle
import sys
import os

pgr_dir = sys.argv[1]
periods = int(sys.argv[2])

all_nodes = set()

for p in range(periods):
	for filename in os.listdir("{}/{}/".format(pgr_dir, p)):
		with open("{}/{}/{}".format(pgr_dir, p, filename), "rb") as fp:
			PGR = pickle.load(fp)
			for source, target_list in PGR["G"].items():
				if int(source) != -1:
					all_nodes.add(int(source))
				for target in target_list:
					if int(target) != -1:
						all_nodes.add(int(target))

print(len(all_nodes))