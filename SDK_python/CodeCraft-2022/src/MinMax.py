import csv
import math
import sys
import os


output_path = "../solutions/output/solution.txt"

def getSiteBandwidth():
    site_bandwidth = {}
    with open("../../../data/site_bandwidth.csv") as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        for row in f_csv:
            site_bandwidth[row[0]] = int(row[1])
    N = len(site_bandwidth) # N site
    return site_bandwidth, N

def getQoSConstraint():
    with open("../../../data/config.ini", mode='r') as f:
        qos_constraint = int(f.readlines()[1].split("=")[-1])
    return qos_constraint

def getQoS():
    qos = {}
    with open("../../../data/qos.csv") as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        M = len(headers) - 1
        for row in f_csv:
            for i in range(M):
                qos[(row[0], headers[i+1])] = int(row[i+1])
    return qos

def getDemand():
    demand = {}
    with open("../../../data/demand.csv") as f:
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
    return demand, T


demand, timestamps = getDemand()
qos_constraint = getQoSConstraint()
qos = getQoS()
site_bandwidth, site_number = getSiteBandwidth()


if __name__ == '__main__':

    solution = open(output_path,mode='w')
        
    site4client = {}
    for m in demand.keys():
        for n in site_bandwidth.keys():
            # print(n, m)
            # print(qos[(n, m)])
            if qos[(n, m)] < qos_constraint:
                if m not in site4client.keys():
                    site4client[m] = [n]
                else:
                    site4client[m].append(n)
    # print('site4client:', site4client)
    # print(site4client['A'])

    client4site = {}
    for n in site_bandwidth.keys():
        for m in demand.keys():
            if qos[(n, m)] < qos_constraint:
                if n not in client4site.keys():
                    client4site[n] = [m]
                else:
                    client4site[n].append(m)
        if n not in client4site.keys():
            client4site[n] = []

    for t in range(timestamps):
        client_info = {}
        for client in list(demand.keys()):
            # print(demand[client][t])
            client_info[client] = [demand[client][t]] #总请求
            client_info[client].append(demand[client][t]) #剩余请求
            client_info[client].append(len(site4client[client])) #对应的满足qos服务器数目
            client_info[client].append({}) #记录这个客户流量都分到哪些服务器上 site->bandwidth

        contain_high_bandwidth_site = []
        site_info = {}
        for site in list(site_bandwidth.keys()):
            # print(site_bandwidth[site])
            site_info[site] = [site_bandwidth[site]] # max bandwidth
            site_info[site].append(site_bandwidth[site]) #remaining bandwidth
            site_info[site].append(len(client4site[site]))
            site_info[site].append({}) #dict 记录 client->bandwidth
            if(len(client4site[site]) >= 20):
                contain_high_bandwidth_site.append(site)

        #算了一下35*0.95 = 33.25，取上界为34， 20*0.95 = 19，也就是说，只有某个站点对应的可服务客户数目大于等于20的时候，才能腾出一个位置放大流量。
        #对于那些能服务客户>=20的服务器，先从各个能服务的客户那里拉取1的流量，由于服务器最大带宽应当远远大于35，这里没有判断服务器带宽不够的情况
        for site in contain_high_bandwidth_site:
            unavailable = [] #可能有客户请求带宽为0
            for client in client4site[site]:
                if client_info[client][1] >= 1: 
                    client_info[client][1] -= 1
                    client_info[client][3][site] = 1
                    site_info[site][1] -= 1
                    site_info[site][3][client] = 1
                else:
                    unavailable.append(client)
            #如果说最后出来的服务的客户数目小于20，那就后续不给它分配高客户带宽。已分配的1留着。
            if site_info[site][2] - len(unavailable) < 20:
                contain_high_bandwidth_site.remove(site)


        
        #最多取本站带宽上限的80%分给一个大的客户流量
        threshold = 0.8
        for site in contain_high_bandwidth_site:
            client_info_order = sorted(client_info.items(), key = lambda x:x[1][2], reverse=True)
            highest_bandwidth_client = client_info_order[0]

            if math.floor(site_info[site][0] * threshold) >= client_info[highest_bandwidth_client][1]:
                allocate_bandwidth = client_info[highest_bandwidth_client][1]
            else:
                allocate_bandwidth = math.floor(site_info[site][0] * threshold)

            client_info[highest_bandwidth_client][1] -= allocate_bandwidth
            site_info[site][1] -= allocate_bandwidth
            if site in client_info[highest_bandwidth_client][3].keys():
                client_info[highest_bandwidth_client][3][site] += allocate_bandwidth
                site_info[site][3][highest_bandwidth_client] += allocate_bandwidth
                
            else:
                client_info[highest_bandwidth_client][3][site] = allocate_bandwidth
                site_info[site][3][highest_bandwidth_client] = allocate_bandwidth
        #一个可能的优化，找出所有的大流量客户，找出对应的长请求服务器（可以服务多个客户的服务器），可能一个大流量客户对应多个可用服务器

        #分完大流量之后，剩下的还是按客户执行平均分策略

        client_info_order = sorted(client_info.items(), key = lambda x:x[1][2], reverse=False)
        for client in [x[0] for x in client_info_order]:
            while(client_info[client][1] > 0):
                actual_site = list(site4client[client])
                average_bandwidth = math.ceil(client_info[client][1] / len(actual_site))
                for site in list(actual_site):
                    if client_info[client][1] == 0:
                        break
                    if site_info[site][1] >= average_bandwidth:
                        if client_info[client][1] > average_bandwidth:
                            allocate_bandwidth = average_bandwidth
                        else:
                            allocate_bandwidth = client_info[client][1]

                        client_info[client][1] -= allocate_bandwidth
                        site_info[site][1] -= allocate_bandwidth
                        if site in client_info[client][3].keys():
                            client_info[client][3][site] += allocate_bandwidth
                            site_info[site][3][client] += allocate_bandwidth
                        else:
                            client_info[client][3][site] = allocate_bandwidth
                            site_info[site][3][client] = allocate_bandwidth
                    else:
                        actual_site.remove(site)

        #根据client_info[client][3]里面存的字典写出来
        client_count = 1
        for client in client_info.keys():
            allocate_situation = client_info[client][3]
            total_allocate_site_number = len(allocate_situation.keys())
            solution.write(client + ":")
            if total_allocate_site_number == 0:
                solution.write('\n')
                continue
            site_count = 1
            for site, bandwidth in allocate_situation.items():
                if site_count < total_allocate_site_number:
                    solution.write("<" + site + "," + str(bandwidth) + ">,")
                else:
                    solution.write("<" + site + "," + str(bandwidth) + ">")
                site_count += 1
            if t == timestamps-1 and client_count == len(client_info.keys()):
                pass
            else:
                solution.write("\n")
            client_count += 1
                


        
