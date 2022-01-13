import pickle
import sys
import json
import numpy as np
import argparse

parser = argparse.ArgumentParser(description="Turns observation class dictionaries into stan code")
parser.add_argument("--input_file",
	type=str,
	help="Input file containing class dictionary",
	required=True)
parser.add_argument("--output_file",
	type=str,
	help="output stan json file",
	required=True)

args = parser.parse_args()

input_file = args.input_file
output_file = args.output_file

with open(input_file, "rb") as fp:
	observation_classes = pickle.load(fp)
num_collectors = 0
for key, value in observation_classes.items():
	num_collectors = int(int(len(key))/int(2))
	break

num_classes = len(observation_classes.keys())

stan_dict = {"num_classes":num_classes,
			"num_collectors":num_collectors,
			"class_counts":[],
			"E":np.zeros((num_classes, num_collectors), dtype=int).tolist(),
			"F":np.zeros((num_classes, num_collectors), dtype=int).tolist()}

i = 0

for key, value in observation_classes.items():
	if i%1000 == 0:
		print("{:.4f}".format(100.0*float(i)/float(num_classes)))
	for k in range(num_collectors):
		stan_dict["E"][i][k] = key[k]
		stan_dict["F"][i][k] = key[num_collectors + k]
	stan_dict["class_counts"].append(int(value))
	i += 1
	
with open(output_file, "w") as fp:
	json.dump(stan_dict, fp)