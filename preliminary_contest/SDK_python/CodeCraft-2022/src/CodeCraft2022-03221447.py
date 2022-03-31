import csv
import math
import sys
import os
import os.path as osp
import numpy as np

# NOTE 提交前记得修改路径
input_path = "../../data/"
output_path = "../solutions/output/solution.txt"

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

def getQoS():
    qos = {}
    with open(osp.join(input_path, "qos.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        M = len(headers) - 1
        for row in f_csv:
            for i in range(M):
                qos[(row[0], headers[i+1])] = int(row[i+1])
    return qos

def getDemand():
    demand = {}
    with open(osp.join(input_path, "demand.csv")) as f:
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

def getQoSRelationship():
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

    # 每一个边缘节点可以服务的客户
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
    return site4client, client4site

demand, timestamps = getDemand()
qos_constraint = getQoSConstraint()
qos = getQoS()
site_bandwidth, site_number = getSiteBandwidth()
site4client, client4site = getQoSRelationship()

def demandAnalysis():
    number_of_big_bandwidth = site_number * math.floor(0.05 * timestamps) #可放的大流量个数

    big_bandwidth_record = [] #list of tuple (timestamps, client) 统计大流量，在哪个时刻，由哪个客户产生
    least_big_bandwidth = 999999999

    client_name_order = []
    demand_list = []
    for client in demand.keys():
        client_name_order.append(client)
        demand_list.append(demand[client])
    demand_list_np = np.array(demand_list)
    #print(demand_list_np.shape)
    index = np.argpartition(demand_list_np.ravel(),-number_of_big_bandwidth)[-number_of_big_bandwidth:]
    index_2d = np.unravel_index(index,demand_list_np.shape)
    #print(index_2d)
    for id in range(len(index_2d[0])):
        big_bandwidth_record.append((index_2d[1][id],client_name_order[index_2d[0][id]]))
        if demand_list_np[index_2d[0][id]][index_2d[1][id]] < least_big_bandwidth:
            least_big_bandwidth = demand_list_np[index_2d[0][id]][index_2d[1][id]]
    big_bandwidth_per_site = number_of_big_bandwidth / site_number
    #print("big_bandwidth_record")
    #print(big_bandwidth_record)

    client_total_demand = np.sum(demand_list_np,1)
    #print(client_total_demand)
    client_site_number = []
    for client in client_name_order:
        client_site_number.append(len(site4client[client]))
    #print(client_site_number)
    client_per_server_exp_demand = client_total_demand / np.array(client_site_number)
    #print(client_per_server_exp_demand)

    #这里 site_per_t_exp算上了大流量。也许不算大流量可能会优化一点？？？？
    site_per_t_exp = {}
    for site in client4site.keys():
        site_total = 0
        for i in range(len(client_name_order)):
            client = client_name_order[i]
            if client in client4site[site]:
                site_total += client_per_server_exp_demand[i]
        site_per_t_exp[site] = int(site_total / timestamps)
    
    #print(site_per_t_exp)

    return big_bandwidth_record, big_bandwidth_per_site, least_big_bandwidth, site_per_t_exp

#大流量记录， 每个站点可容纳的大流量数目，最小的大流量，每个时间步站点期望处理的流量数目。
big_bandwidth_record, big_bandwidth_per_site, least_big_bandwidth, site_per_t_exp = demandAnalysis()

if __name__ == '__main__':

    solution = open(output_path,mode='w')
    #记录每个site已经接收的大带宽的客户数目
    site_big_bandwidth_in_number = {}
    
    for t in range(timestamps):
        client_info = {}
        for client in list(demand.keys()):
            client_info[client] = [demand[client][t]] #total demand
            client_info[client].append(demand[client][t]) #remaining demand
            client_info[client].append(len(site4client[client]))
            client_info[client].append({})

        site_info = {}
        for site in list(site_bandwidth.keys()):
            site_info[site] = [site_bandwidth[site]] # max bandwidth
            site_info[site].append(site_bandwidth[site]) #remaining bandwidth
            site_info[site].append(len(client4site[site]))
            site_info[site].append({}) #dict 记录 client->bandwidth

        #仍然按照可用服务器最少的客户开始
        client_info_order = sorted(client_info.items(), key=lambda x:x[1][2], reverse=False)

        processed_client = [] #已经分配完流量的大流量客户
        site_big_bandwidth_client = {} #当前时间步服务器对应的大流量客户
        for client in [x[0] for x in client_info_order]:
            if (t, client) in big_bandwidth_record:
                for site in site4client[client]:
                    if site not in site_big_bandwidth_client.keys():
                        site_big_bandwidth_client[site] = [client]
                    else:
                        site_big_bandwidth_client[site].append(client)
        
        for client in [x[0] for x in client_info_order]:
            debug_client_got_bandwidth = 0
            #首先判断是否大流量，若是，选择一个“最佳的”服务器进行处理
            if (t, client) in big_bandwidth_record:
                #print("big bandwidth at timestamp=",t, " from client=",client, end="")
                #先找服务器
                chosen_site = ""
                for site in site4client[client]:
                    site_serve_big_client_list = site_big_bandwidth_client[site]
                    site_serve_big_client_num = len(site_serve_big_client_list)
                    for c in site_serve_big_client_list:
                        if c == client:
                            continue
                        elif c in processed_client:
                            site_serve_big_client_num -= 1
                    #先找找哪个站点只用处理这一个大流量
                    if(site_serve_big_client_num == 1):
                        chosen_site = site
                        break
                    elif site_info[site][1] > int(client_info[client][1] * 0.7):
                        chosen_site = site
                        break
                #print(" chosen site=",chosen_site)
                if chosen_site != "":
                    allocate_bandwidth = 0
                    if site_info[chosen_site][1] >= client_info[client][1]:
                        allocate_bandwidth = client_info[client][1]
                    else:
                        allocate_bandwidth = site_info[chosen_site][1]
                    debug_client_got_bandwidth += allocate_bandwidth
                    client_info[client][1] -= allocate_bandwidth
                    site_info[chosen_site][1] -= allocate_bandwidth
                    if chosen_site not in client_info[client][3].keys():
                        client_info[client][3][chosen_site] = allocate_bandwidth
                        site_info[chosen_site][3][client] = allocate_bandwidth
                    else:
                        client_info[client][3][chosen_site] += allocate_bandwidth
                        site_info[chosen_site][3][client] += allocate_bandwidth
            #如果它不是大流量，或者是大流量但是没找到合适的服务器或者是没分配完流量，就用while进行分配
            #每次分配到服务器不大于其期望服务带宽
            actual_site = list(site4client[client])
            while client_info[client][1] > 0:
                #print("allocating bandwidth for client=",client," max demand=",client_info[client][0], "remaining demand=",client_info[client][1])
                if(len(actual_site) == 0):
                    print("ERROR! NO AVAILABLE SITE.")
                    solution.close()
                    os.remove(output_path)
                    sys.exit(1)
                for site in list(actual_site):
                    if client_info[client][1] == 0:
                        break
                    elif client_info[client][1] < 0:
                        print("ERROR! CHECK CLIENT REMAINING BANDWIDTH")
                    onetime_allocate_bandwidth = site_per_t_exp[site]
                    if site_info[site][1] == 0:
                        actual_site.remove(site)
                        continue
                    elif site_info[site][1] < 0:
                        print("ERROR!! CHECK SITE REMAINING BANDWIDTH.")
                    allocate_bandwidth = 0
                    if site_info[site][1] >= onetime_allocate_bandwidth:
                        if client_info[client][1] > onetime_allocate_bandwidth:
                            allocate_bandwidth = onetime_allocate_bandwidth
                        else:
                            allocate_bandwidth = client_info[client][1]
                    debug_client_got_bandwidth += allocate_bandwidth
                    #print("allocate bandwidth for client, from site = ",site,"bandwidth=",allocate_bandwidth )
                    client_info[client][1] -= allocate_bandwidth
                    site_info[site][1] -= allocate_bandwidth
                    if site not in client_info[client][3].keys():
                        client_info[client][3][site] = allocate_bandwidth
                        site_info[site][3][client] = allocate_bandwidth
                    else:
                        client_info[client][3][site] += allocate_bandwidth
                        site_info[site][3][client] += allocate_bandwidth
            if debug_client_got_bandwidth != client_info[client][0]:
                print("ERROR! CLIENT BANDWIDTH NOT SATISFIED. time=",t,"client=",client)
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
    
    solution.close()
                    




                
                    
                        
                





