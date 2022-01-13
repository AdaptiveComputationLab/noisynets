import argparse


parser = argparse.ArgumentParser(description="Generates sbatch script for count_classes_single")
parser.add_argument("--num_nodes",
					type=int,
					help="Number of nodes seen total",
					required=True)
parser.add_argument("--num_periods",
					type=int,
					help="Number of periods in dataset",
					required=True)
parser.add_argument("--cpu_nodes",
					type=int,
					help="Number of compute nodes",
					required=True)
parser.add_argument("--cpu_cores",
					type=int,
					help="Number of compute cores",
					required=True)
parser.add_argument("--partition",
					type=str,
					help="Partition to run on",
					required=True)
parser.add_argument("--timelimit",
					type=str,
					help="time limit for slurm",
					required=True)
parser.add_argument("--jobname",
					type=str,
					help="Slurm job name",
					required=True)
parser.add_argument("--dir",
					type=str,
					help="Dataset PGR directory",
					required=True)
parser.add_argument("--output",
					type=str,
					help="output name",
					required=True)

args = parser.parse_args()
cpu_nodes = args.cpu_nodes
cpu_cores = args.cpu_cores
partition = args.partition
timelimit = args.timelimit
jobname = args.jobname

num_nodes = args.num_nodes
num_periods = args.num_periods
dataset_dir = args.dir
outputname = args.output


r = '''#!/bin/bash

#SBATCH -N {}
#SBATCH -n {}
#SBATCH -p {}
#SBATCH -t {}

#SBATCH -J {}

#SBATCH -o {}.\%j.out
#SBATCH -e {}.\%j.err

#SBATCH --mail-type=ALL
#SBATCH --mail-user=kleyba@asu.edu

module purge
module load python/3.7.1

'''.format(cpu_nodes, cpu_cores, partition, timelimit, jobname, jobname, jobname)

nodes_per_proc = int(int(num_nodes)/int(cpu_cores))
extra = int(num_nodes)%int(cpu_cores)
for i in range(0, cpu_cores):
	start = int(i*nodes_per_proc)
	end = int(start + nodes_per_proc)
	if(i-1 == cpu_cores):
		end += extra
	r += "python ./python_programs/count_classes_single.py --start {} ".format(start) + \
		 "--end {} ".format(end) + \
		 "--proc_id {} ".format(i) + \
		 "--periods {} ".format(num_periods) + \
		 "--dir {} ".format(dataset_dir) + \
		 "--output {} &\n".format(outputname)

r += "wait"

print(r)