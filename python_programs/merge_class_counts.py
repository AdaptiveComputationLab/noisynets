import sys
import os
import pickle
from collections import Counter
import argparse

parser = argparse.ArgumentParser(description="Merges Observation Classes")
parser.add_argument("--input_dir",
	type=str,
	help="Input directory containing classes and counts",
	required=True)
parser.add_argument("--output_file",
	type=str,
	help="name of output file",
	required=True)
parser.add_argument("--num_nodes",
	type=int,
	help="Number of nodes seen in all PGRs",
	required=True)
parser.add_argument("--num_collectors",
	type=int,
	help="Number of collectors",
	required=True)

args = parser.parse_args()

input_dir = args.input_dir
output_file = args.output_file
num_nodes = args.num_nodes
num_collectors = args.num_collectors

observation_classes = {}

for filename in os.listdir(input_dir):
	if ".cls" in filename:
		print("{}...".format(filename))
		with open("{}/{}".format(input_dir, filename), "r") as fp:
			
			line = fp.readline()
			while(line):
				if(len(line) > 0):
					s = line.split(",")
					count = int(s[0])
					record_list = [0]*num_collectors*2
					for k in range(num_collectors):
						record_list[k] = int(s[1 + k])
						record_list[num_collectors + k] = int(s[1 + num_collectors + k])
				try:
					observation_classes[tuple(record_list)] += count
				except:
					observation_classes[tuple(record_list)] = count
				line = fp.readline()
		print("Classes so far: {}".format(len(observation_classes.keys())))

print("Total Classes: {}".format(len(observation_classes.keys())))
num_classes = len(observation_classes.keys())

with open(output_file, "wb") as fp:
	fp.write(num_classes.to_bytes(4, 'little'))
	fp.write(num_collectors.to_bytes(4, 'little'))
	fp.write(num_nodes.to_bytes(4, 'little'))
	for record, count in observation_classes.items():
		fp.write(count.to_bytes(4, 'little'))
		for k in range(num_collectors):
			fp.write(record[k].to_bytes(4, 'little'))
			fp.write(record[k + num_collectors].to_bytes(4, 'little'))