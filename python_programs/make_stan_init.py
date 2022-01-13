import argparse
import json
from scipy.special import logit

parser = argparse.ArgumentParser(description="Makes init data for stan")
parser.add_argument("--alpha",
	type=float,
	help="value for init alpha",
	required=True)
parser.add_argument("--beta",
	type=float,
	help="value for init beta",
	required=True)
parser.add_argument("--rho",
	type=float,
	help="value for init rho",
	required=True)
parser.add_argument("--k",
	type=int,
	help="number of route collectors",
	required=True)
parser.add_argument("--output",
	type=str,
	help="output file for init params",
	required=True)

args = parser.parse_args()
alpha = args.alpha
beta = args.beta
rho = args.rho
k = args.k
outputfile = args.output

rates = []
for i in range(k):
	rates.append([logit(beta), logit(alpha)])

data = {"rho":rho, "rates":rates}

with open(outputfile, "w") as fp:
	json.dump(data, fp)