import csv
import math
import sys
import os
import os.path as osp

# NOTE 提交前记得修改路径
input_path = "/data"
output_path = "/output/solution.txt"

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


demand, timestamps = getDemand()
qos_constraint = getQoSConstraint()
qos = getQoS()
site_bandwidth, site_number = getSiteBandwidth()

        
if __name__=='__main__':
    solution = open(output_path, mode='w')

    # 记录每一个客户可用的边缘节点
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
    
    line_count = 0
    for t in range(timestamps):
        client_info = {}
        for client in list(demand.keys()):
            client_info[client] = [demand[client][t]]
            client_info[client].append(demand[client][t])
            client_info[client].append(len(site4client[client]))
        
        # 根据可用的边缘节点数量从小到大排序
        client_info_order = sorted(client_info.items(), key=lambda x:x[1][2], reverse=False)

        site_info = {}
        for site in list(site_bandwidth.keys()):
            site_info[site] = [site_bandwidth[site]]
            site_info[site].append(site_bandwidth[site])

        site_allocation = {}
        for site in site_bandwidth.keys():
            site_allocation[site] = {'usage': 0}
        for client in [x[0] for x in client_info_order]:
            while client_info[client][1] > 0:
                actual_site = list(site4client[client])
                average_bandwidth = math.ceil(client_info[client][1] /len(actual_site))
                for site in list(actual_site):
                    if site_info[site][1] >= average_bandwidth:
                        client_info[client][1] -= average_bandwidth
                        site_info[site][1] -= average_bandwidth
                        if client_info[client][1] <= 0:
                            site_info[site][1] += (0 - client_info[client][1])
                            break
                    else:
                        actual_site.remove(site)
                    
                if len(actual_site) == 0:
                    print('No feasible solution')
                    solution.close()
                    os.remove(output_path)
                    sys.exit(1)

            # 记录边缘节点分配详情
            for site in list(site4client[client]):
                assigned_bandwidth = site_info[site][0] - site_info[site][1]
                site_info[site][0] = site_info[site][1]
                if assigned_bandwidth != 0:
                    if assigned_bandwidth < 0:
                        print("computation error")
                        # sys.exit(0)
                    site_allocation[site][client] = assigned_bandwidth
                    site_allocation[site]['usage'] += assigned_bandwidth

        # 对边缘节点的分配流量进行再分配
        site_allocation_order = sorted(site_allocation.items(), key=lambda x : x[1]['usage'], reverse=True)
        for site, client_allocation in site_allocation_order:
            for client in client_allocation:
                if client != 'usage':
                    for site_client in site4client[client]:
                        if site_allocation[site][client] <= 0:
                            break
                        if site_client != site:
                            exceed = site_allocation[site]['usage'] - site_allocation[site_client]['usage']
                            if exceed > 0:
                                move_flow = min(min(exceed // 2, site_info[site_client][0]), site_allocation[site][client])
                                if client not in site_allocation[site_client].keys():
                                    site_allocation[site_client][client] = move_flow
                                else:
                                    site_allocation[site_client][client] += move_flow
                                site_allocation[site_client]['usage'] += move_flow
                                site_allocation[site][client] -= move_flow
                                site_allocation[site]['usage'] -= move_flow
                                site_info[site_client][0] -= move_flow
                                site_info[site_client][1] -= move_flow
                                site_info[site][0] += move_flow
                                site_info[site][1] += move_flow 

        # 输出答案
        for client in [x[0] for x in client_info_order]:
            line_count += 1
            solution.write(client + ":")
            if client_info[client][0] == 0:
                solution.write('\n')
                continue
            count = 0
            writed_site_count = 0
            for site in list(site4client[client]):
                count += 1 # 当前使用的边缘节点数量
                # assigned_bandwidth = site_info[site][0] - site_info[site][1]
                # site_info[site][0] = site_info[site][1]
                assigned_bandwidth = site_allocation[site][client] if client in site_allocation[site].keys() else 0
                if assigned_bandwidth != 0:
                    if assigned_bandwidth < 0:
                        print(" computation error ")
                    # 没到最后一个边缘节点
                    if count < len(list(site4client[client])):
                        if writed_site_count == 0:
                            solution.write("<" + site + "," + str(assigned_bandwidth) + ">")
                            writed_site_count += 1
                        
                        elif writed_site_count > 0:
                            solution.write(",<" + site + "," + str(assigned_bandwidth) + ">")
                            writed_site_count += 1
                        
                    else:
                        if line_count != timestamps*len(demand):
                            solution.write(",<" + site + "," + str(assigned_bandwidth) + ">\n")
                        
                        elif line_count == timestamps*len(demand):
                            solution.write(",<" + site + "," + str(assigned_bandwidth) + ">")
                
                else:
                    if count == len(list(site4client[client])) and line_count != timestamps*len(demand):
                        solution.write("\n")
        
        # check
        # print(t)
        total_demand = 0
        for client in list(client_info.keys()):
            # print(client)
            # print(client_info[client][0], client_info[client][1])
            total_demand += client_info[client][0]
            if client_info[client][1] > 0:
                print("Insufficient allocation")
                solution.close()
                os.remove(output_path)
                sys.exit()
        
        total_assigned = 0
        for site in list(site_info.keys()):
            # print(site)
            # print(site_info[site][0], site_info[site][1])
            total_assigned += (site_bandwidth[site] - site_info[site][1])
            if site_info[site][1] < 0:
                print("Upper limit exceeded")
                solution.close()
                os.remove(output_path)
                sys.exit()

        print(total_demand)
        print(total_assigned)

        if total_demand != total_assigned:
            print("allocation mismatching")
            solution.close()
            os.remove(output_path)
            sys.exit()
        
        # breakpoint()

    solution.close()
    # os.remove(output_path)

