import csv

# read data
demand = {}
with open("/data/demand.csv") as f:
    f_csv = csv.reader(f)
    headers = next(f_csv)
    M = len(headers) - 1 # M client
    for row in f_csv:
        for i in range(M):
            if headers[i+1] not in demand.keys():
                demand[headers[i+1]] = [int(row[i+1])]
            else:
                demand[headers[i+1]].append(int(row[i+1]))
T = len(demand[headers[1]]) # T

site_bandwidth = {}
with open("/data/site_bandwidth.csv") as f:
    f_csv = csv.reader(f)
    headers = next(f_csv)
    for row in f_csv:
        site_bandwidth[row[0]] = int(row[1])
N = len(site_bandwidth) # N site

qos = {}
with open("/data/qos.csv") as f:
    f_csv = csv.reader(f)
    headers = next(f_csv)
    M = len(headers) - 1
    for row in f_csv:
        for i in range(M):
            qos[(row[0], headers[i+1])] = int(row[i+1])

with open("/data/config.ini", mode='r') as f:
    qos_constraint = int(f.readlines()[1].split("=")[-1])

# qos_constraint
site4client = {}
for m in demand.keys():
    for n in site_bandwidth.keys():
        if qos[(n, m)] < qos_constraint:
            if m not in site4client.keys():
                site4client[m] = [n]
            else:
                site4client[m].append(n)

# write
solution = open("/output/solution.txt", mode='w')
for t in range(T):
    for m in demand.keys():
        solution.write(m + ":")
        N = len(site4client[m])
        average = demand[m][t] // N
        for n_i in range(N):
            if n_i != N-1:
                solution.write("<" + site4client[m][n_i] + "," + str(average) + ">,")
            else:
                solution.write("<" + site4client[m][n_i] + "," + str(demand[m][t] - n_i * average) + ">")
        if t != T-1:
            solution.write("\n")
solution.close()

# print("Hello world!")
