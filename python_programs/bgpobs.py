import pybgpstream as bgp
import pickle
import time
import datetime
import argparse
import os

def save_pgr(PGR, output_directory):
	'''
	Checks if a directory exists for storing the PGRs in
	subdirectories for each time period
	'''
	fname = "{}/{}_{}_{}.PGR".format(output_directory,
											PGR["start_time"],
											PGR["end_time"],
											PGR["collector"])
	with open(fname, "wb") as fp:
		print("Writing PGR to file {}...".format(fname))
		pickle.dump(PGR, fp)

def read_until(startTime, endTime, maxRIBs = -1, collectorList = []):
	'''
	Reads RIBs and yields as-path strings

	Reads as many ribs as possible from a collector between startTime and endTime
	epoch timestamps (int).
	Yields 'empty' if there is no RIB in the provided range.
	'''
	if len(collectorList) > 0:
		stream = bgp.BGPStream(from_time = str(startTime),
			until_time=str(endTime),
			record_type="ribs", collectors=collectorList)
	else:
		stream = bgp.BGPStream(from_time = str(startTime),
			until_time=str(endTime),
			record_type="ribs")
	

	ribsFound = 0

	for rec in stream.records():
		elem = rec.get_next_elem()
		dump_position = rec.dump_position
		if(dump_position == "end"):
			ribsFound += 1
			print("Completed an RIB from {}".format(rec.collector))
			print("ribsFound: {}".format(ribsFound))
		for elem in rec:
			output = {}
			output["position"] = dump_position
			output["peer_asn"] = elem.peer_asn
			output["type"] = elem.type
			output["time"] = rec.dump_time
			output["collector"] = elem.collector

			if 'as-path' in elem.fields:
				output["as_path"] = elem.fields['as-path']
			else:
				output["as_path"] = ""
			yield output
			elem = rec.get_next_elem()
	print("total ribs found: {}".format(ribsFound))

def count_pgrs(output_directory, time_period, dataset_name):
	'''
	Records and stores "Positive Graph Records" from BGPStream
	'''
	t = time.time()
	collector = ""
	G = {}
	PGRs = {}
	st = datetime.datetime.utcfromtimestamp(time_period[0]).strftime("%Y-%m-%d %H:%M:%S UTC")
	et = datetime.datetime.utcfromtimestamp(time_period[1]).strftime("%Y-%m-%d %H:%M:%S UTC")
	for record in read_until(st,
							et,
							collectorList=[]):
		collector = record["collector"]
		if (collector, time_period[0]) not in PGRs:
			PGRs[(collector, time_period[0])] = {"start_time":time_period[0],
							  "end_time":time_period[1],
							  "G":{},
							  "collector":collector}
		pathstr = record["as_path"]
		s = pathstr.split(" ")
		for i in range(0,len(s)-1):
			a = s[i]
			b = s[i+1]
			if(("{" not in a) and ("{" not in b) and (int(a) != int(b))):
				try:
					PGRs[(collector, time_period[0])]["G"][int(a)].add(int(b))
				except:
					PGRs[(collector, time_period[0])]["G"][int(a)] = {int(b)}
				try:
					PGRs[(collector, time_period[0])]["G"][int(b)].add(int(a))
				except:
					PGRs[(collector, time_period[0])]["G"][int(b)] = {int(a)}
		try:
			PGRs[(collector, time_period[0])]["G"][-1].add(int(record["peer_asn"]))
		except:
			PGRs[(collector, time_period[0])]["G"][-1] = {int(record["peer_asn"])}
		try:
			PGRs[(collector, time_period[0])]["G"][int(record["peer_asn"])].add(-1)
		except:
			PGRs[(collector, time_period[0])]["G"][int(record["peer_asn"])] = {-1}

	tt = time.time()
	print("Time to download dataset: {} seconds".format(tt-t))
	return PGRs

def main():
	### Set up arguments
	parser = argparse.ArgumentParser(description="Downloads BGPrecords from BGPstream and compiles PGR datasets")
	parser.add_argument("--start",
						required=True,
						type=int,
						help="Start Time in UTC Epoch Timestamp format")
	parser.add_argument("--end",
						required=True,
						type=int,
						help="End Time in UTC Epoch Timestamp format")
	parser.add_argument("--dir",
						required=True,
						type=str,
						help="Directory to store dataset")
	parser.add_argument("--name",
						required=True,
						type=str,
						help="Name for dataset")


	args = parser.parse_args()
	start = args.start
	end = args.end
	dataset_dir = args.dir
	name = args.name

	time_period = (start, end)
	PGRs = count_pgrs(dataset_dir, time_period, name)
	for key, value in PGRs.items():
		PGR = value
		save_pgr(PGR, dataset_dir)

if __name__ == '__main__':
	main()