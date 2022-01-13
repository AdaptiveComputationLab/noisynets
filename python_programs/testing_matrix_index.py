import math

n_range = [4, 5, 6, 7, 8]

for n in n_range:
	idx = 0
	for i in range(0,n):
		for j in range(i+1,n):
			k = int((n*(n-1)/2) - (n-i)*((n-i)-1)/2 + j - i - 1)
			ci = n - 2 - int(math.sqrt(-8*k + 4*n*(n-1)-7)/2.0 - 0.5)
			cj =int(k + i + 1 - n*(n-1)/2 + (n-i)*((n-i)-1)/2)
			print("Expected idx: {}, calculated idx {}".format(idx, k))
			print("Expected i,j: {},{}, calculated i,j: {},{}".format(i,j,ci,cj))
			idx += 1