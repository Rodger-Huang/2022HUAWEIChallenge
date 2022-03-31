from CodeCraft_2022_1373918 import *
import math

site_t = {}
for site in site_bandwidth.keys():
    site_t[site] = [0 for _ in range(timestamps)]

t = 0
count = 0
with open("output/solution.txt", mode="r") as file:
    for line in file.readlines():
        count += 1
        client, site_allocations = line.split(':')
        for i, site_allocation in enumerate(site_allocations.strip()[1:-1].split('>,<')):
            site, allocation = site_allocation.split(',')
            site_t[site][t] += int(allocation)
        
        if count == len(demand):
            t += 1
            count = 0

position95 = int(math.ceil(timestamps * 0.95) - 1)
site_95 = {}
all_95 = 0
for site in site_t.keys():
    site_t[site] = sorted(site_t[site])
    site_95[site] = site_t[site][position95]
    all_95 += site_t[site][position95]

print(all_95)
