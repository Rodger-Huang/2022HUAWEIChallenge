
from pkgutil import ImpImporter
import numpy as np
import math
from os import times
import os.path as osp
import csv
import sys


input_path = "intermediary_contest/data/"
output_path = "intermediary_contest/output/solution.txt"
#output_path = "../output/test_sol.txt"

check_site_over_ts_path = "intermediary_contest/output/site_use_over_ts.txt"
check_site_detail_path = "intermediary_contest/output/site_detail.txt"

def getSiteBandwidth():
    site_bandwidth = {}
    with open(osp.join(input_path, "site_bandwidth.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        for row in f_csv:
            site_bandwidth[row[0]] = int(row[1])
    N = len(site_bandwidth) # N site
    return site_bandwidth, N

def getQoSConstraint():
    with open(osp.join(input_path, "config.ini"), mode='r') as f:
        qos_constraint = int(f.readlines()[1].split("=")[-1])
    return qos_constraint

def getBaseCost():
    with open(osp.join(input_path, "config.ini"), mode='r') as f:
        base_cost = int(f.readlines()[2].split("=")[-1])
    return base_cost


def getQoS():
    qos = {} #取的时候(site,client)
    with open(osp.join(input_path, "qos.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        M = len(headers) - 1
        for row in f_csv:
            for i in range(M):
                qos[(row[0], headers[i+1])] = int(row[i+1])
    return qos

def getDemand():
    demand = {} #map<string, vector<vector<pair<string,int>>>>
    time_stream_client = [] #vector<map<(stream,client),int>>
    ts_tag = ""
    total_timestamp = 0
    with open(osp.join(input_path, "demand.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        M = len(headers) - 2 # M client
        for row in f_csv:
            current_ts = row[0]
            if(current_ts != ts_tag):
                ts_tag = current_ts
                total_timestamp += 1
                stream_type = row[1]
                for i in range(M):
                    if i == 0:
                        tmp_dict = {(stream_type,headers[i+2]):int(row[i+2])}
                        time_stream_client.append(tmp_dict)
                    else:
                        time_stream_client[total_timestamp-1][(stream_type,headers[i+2])] = int(row[i+2])
                    if headers[i+2] not in demand.keys():
                        demand[headers[i+2]] = [[(stream_type,int(row[i+2]))]]
                    else:
                        demand[headers[i+2]].append([(stream_type,int(row[i+2]))])
            else:
                stream_type = row[1]
                for i in range(M):
                    time_stream_client[total_timestamp-1][(stream_type,headers[i+2])] = int(row[i+2])
                    if headers[i+2] not in demand.keys():
                        demand[headers[i+2]] = [[(stream_type,int(row[i+2]))]]
                    else:
                        demand[headers[i+2]][total_timestamp-1].append((stream_type,int(row[i+2])))

    return demand, total_timestamp, M, time_stream_client


demand, timestamps, client_number, time_stream_client = getDemand()
qos_constraint = getQoSConstraint()
qos = getQoS()
site_bandwidth, site_number = getSiteBandwidth()
base_cost = getBaseCost()



site_t = {}
site_usage = {}
for site in site_bandwidth.keys():
    #元组是用来记录具体分配情况的，0那里是记录总量
    site_t[site] = [[] for _ in range(timestamps)]
    site_usage[site] = [0 for _ in range(timestamps)]


read_time = 0 #每个时间应该读client_number行
read_line = 0 

with open(output_path, mode="r") as file:
    for line in file.readlines():
     
        read_line += 1
        client, site_allocations = line.split(":")
        
        if site_allocations.find('>,<') != -1: 
            for i, site_allocation in enumerate(site_allocations.split('>,<')):
                if i == 0:
                    #print("i=0,site_allo",site_allocation)
                    site_allocation = site_allocation.strip()
                    site_allocation = site_allocation.strip('<')
                    #print(site_allocation)
                elif i == len(site_allocations.split('>,<')) -1:
                    #print("i=-1,site_allo:",site_allocation)
                    site_allocation = site_allocation.strip()
                    site_allocation = site_allocation.strip('>')
                else:
                    #print("other,site_allo",site_allocation)
                    site_allocation = site_allocation.strip()
                    pass
                use_site = site_allocation.split(',')[0]
                
                for j, stream_type in enumerate(site_allocation.split(',')):
                    if j == 0:
                        continue
                    stream_bw = time_stream_client[read_time][(stream_type,client)]
                    site_t[use_site][read_time].append((stream_type,stream_bw))
                    site_usage[use_site][read_time] += stream_bw
                       
        else:
            #print("only one",site_allocations)
            site_allocation = site_allocations.strip()
            site_allocation = site_allocation.strip('<') 
            site_allocation = site_allocation.strip('>')    
            use_site = site_allocation.split(',')[0]
            #print('use_site=',use_site)
            for j, stream_type in enumerate(site_allocation.split(',')):
                if j == 0:
                    continue
                stream_bw = time_stream_client[read_time][(stream_type,client)]
                site_t[use_site][read_time].append((stream_type,stream_bw))
                site_usage[use_site][read_time] += stream_bw
        
        if(read_line % client_number == 0):
            read_time += 1

with open(check_site_over_ts_path, mode='w') as fc:
    fc.write("timestamp ")
    for site in site_bandwidth:
        fc.write(site + ",")
    fc.write('\n')
    for t in range(timestamps):
        fc.write("t="+str(t)+":")
        for site in site_bandwidth:
            fc.write(str(site_usage[site][t])+",")
        fc.write("\n")

with open(check_site_detail_path, mode='w') as fc:
    
    for t in range(timestamps):
        fc.write("t="+str(t)+"\n")
        for site in site_bandwidth:
            fc.write("site="+site + " ")
            for stream_t, stream_bw in site_t[site][t]:
                fc.write("("+stream_t+","+str(stream_bw)+") ")
            fc.write('\n')
        fc.write('\n')



print("read time=",read_time)
        

#输出各个站点的95%带宽
site_v_95 = {}
all_zero_site = []
total_cost = 0
position_95 = math.ceil(0.95*timestamps) - 1
for site in site_bandwidth:
    index = np.argsort(np.array(site_usage[site]))
    value_95 = site_usage[site][index[position_95]]
    site_v_95[site] = value_95
    #顺便检查一下是否全0
    if value_95 == 0:
        all_zero = True
        for i in range(position_95, timestamps):
            if site_usage[site][index[i]] != 0:
                all_zero = False
        if all_zero:
            all_zero_site.append(site)
            total_cost += 0
        else:
            total_cost += base_cost
    elif value_95 > base_cost:
        total_cost += value_95 + (value_95-base_cost)**2/site_bandwidth[site]
        #total_cost += value_95
    else:
        total_cost += base_cost


print("site position 95 value:")
print(site_v_95)
print("all_zero_site")
print(all_zero_site)
    
print("total_cost=",total_cost)
#计算代价
