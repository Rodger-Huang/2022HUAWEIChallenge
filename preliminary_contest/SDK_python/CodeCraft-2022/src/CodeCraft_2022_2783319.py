import csv
import math
import sys
import os
import os.path as osp
import numpy as np

# NOTE 提交前记得修改路径
input_path = "data"
output_path = "output/solution.txt"

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
    for client in demand.keys():
        for site in site_bandwidth.keys():
            # print(n, m)
            # print(qos[(n, m)])
            if qos[(site, client)] < qos_constraint:
                if client not in site4client.keys():
                    site4client[client] = [site]
                else:
                    site4client[client].append(site)

    # 每一个边缘节点可以服务的客户
    client4site = {}
    for site in site_bandwidth.keys():
        client4site[site] = []
        for client in demand.keys():
            if qos[(site, client)] < qos_constraint:
                client4site[site].append(client)
    
    line_count = 0
    site_t = {}
    for site in site_bandwidth.keys():
        site_t[site] = {'usage': []}
    site_list = list(site_bandwidth)
    interval = math.floor(timestamps * 0.05)
    cycle = timestamps // interval
    for t in range(timestamps):
        # 错峰轮流在t时刻尽量填满几个边缘节点
        # site_full_t = set()
        # start = t // interval
        # if start < site_number:
        #     site_full_t = set(site_list[start::cycle])
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
                actual_site = set(site4client[client])
                # for site2full in (site_full_t & actual_site):
                #     allocate_bandwidth = min(site_info[site2full][1], client_info[client][1])
                #     client_info[client][1] -= allocate_bandwidth
                #     site_info[site2full][1] -= allocate_bandwidth
                #     actual_site.remove(site2full)

                average_bandwidth = math.ceil(client_info[client][1] / len(actual_site))
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
                    site_allocation[site][client] = int(assigned_bandwidth)
                    site_allocation[site]['usage'] += int(assigned_bandwidth)

        # 对边缘节点的分配流量进行再分配
        # site_allocation_order = sorted(site_allocation.items(), key=lambda x : x[1]['usage'], reverse=True)
        # for site, client_allocation in site_allocation_order:
        #     for client in client_allocation:
        #         if client != 'usage':
        #             # 记录当前客户使用的流量较小的边缘节点
        #             site_client_exceed = {}
        #             for site_client in site4client[client]:
        #                 exceed = site_allocation[site]['usage'] - site_allocation[site_client]['usage']
        #                 if exceed > 0:
        #                     site_client_exceed[site_client] = exceed
        #             site_client_exceed = sorted(site_client_exceed.items(), key=lambda x : x[1], reverse=True)
        #             for site_client, _ in site_client_exceed:
        #                 exceed = site_allocation[site]['usage'] - site_allocation[site_client]['usage']
        #                 if exceed <= 0 or site_allocation[site][client] <= 0:
        #                     break
        #                 move_flow = min(min(exceed // 2, site_info[site_client][0]), site_allocation[site][client])
        #                 if client not in site_allocation[site_client].keys():
        #                     site_allocation[site_client][client] = move_flow
        #                 else:
        #                     site_allocation[site_client][client] += move_flow
        #                 site_allocation[site_client]['usage'] += move_flow
        #                 site_allocation[site][client] -= move_flow
        #                 site_allocation[site]['usage'] -= move_flow
        #                 site_info[site_client][0] -= move_flow
        #                 site_info[site_client][1] -= move_flow
        #                 site_info[site][0] += move_flow
        #                 site_info[site][1] += move_flow

        for site in site_bandwidth.keys():
            site_t[site]['usage'].append(site_allocation[site]['usage'])
            del site_allocation[site]['usage']
            site_t[site][t] = site_allocation[site]

    # 尽可能塞满每一个边缘节点：超过95%的节点就尽量塞到上限，低于95%的节点就尽量塞到95%
    # 记录处理过的节点，避免移入节点的流量在后续操作其他节点时又流出
    site_processed = set()
    # for site in site_bandwidth.keys():
    #     site_processed[site] = [t for t in range(timestamps)]
    position_95 = int(math.ceil(timestamps * 0.95) - 1)
    position_x = int(math.ceil(timestamps * 0.48) - 1)
    # 对节点根据客户数量从少到多进行排序，少客户的节点有更大概率95%值比较小
    site_info_order = sorted(client4site.items(), key=lambda x : len(x[1]))
    for site, _ in site_info_order:
        site_processed.add(site)
        index = np.argsort(site_t[site]['usage'])
        value_95 = site_t[site]['usage'][index[position_95]]
        value_x = site_t[site]['usage'][index[position_x]]
        for position, t in enumerate(index):
            if position <= position_95:
                left = value_95 - site_t[site]['usage'][t]
                # left = value_x - site_t[site]['usage'][t]

                # left = site_t[site]['usage'][t]
                # if left <= 0:
                #     continue
                # for client in client4site[site]:
                #     for site_client in site4client[client]:
                #         if left <= 0:
                #             break
                #         if site_client not in site_processed and client in site_t[site][t].keys():
                #             move_flow = min(site_t[site][t][client], site_bandwidth[site_client] - site_t[site_client]['usage'][t])
                #             if client in site_t[site_client][t].keys():
                #                 site_t[site_client][t][client] += move_flow
                #             else:
                #                 site_t[site_client][t][client] = move_flow
                #             site_t[site_client]['usage'][t] += move_flow
                #             site_t[site][t][client] -= move_flow
                #             site_t[site]['usage'][t] -= move_flow
                #             left -= move_flow

            else:
                left = site_bandwidth[site] - site_t[site]['usage'][t]

            if left <= 0:
                continue
            for client in client4site[site]:
                for site_client in site4client[client]:
                    if left <= 0:
                        break
                    if site_client not in site_processed and client in site_t[site_client][t].keys():
                        move_flow = min(left, site_t[site_client][t][client])
                        if client in site_t[site][t].keys():
                            site_t[site][t][client] += move_flow
                        else:
                            site_t[site][t][client] = move_flow
                        site_t[site]['usage'][t] += move_flow
                        site_t[site_client][t][client] -= move_flow
                        site_t[site_client]['usage'][t] -= move_flow
                        left -= move_flow

    # 输出结果
    for t in range(timestamps):
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
                # assigned_bandwidth = site_allocation[site][client] if client in site_allocation[site].keys() else 0
                assigned_bandwidth = site_t[site][t][client] if client in site_t[site][t].keys() else 0
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
                        if writed_site_count == 0:
                            if line_count != timestamps*len(demand):
                                solution.write("<" + site + "," + str(assigned_bandwidth) + ">\n")
                            
                            elif line_count == timestamps*len(demand):
                                solution.write("<" + site + "," + str(assigned_bandwidth) + ">")
                        
                        else:
                            if line_count != timestamps*len(demand):
                                solution.write(",<" + site + "," + str(assigned_bandwidth) + ">\n")
                            
                            elif line_count == timestamps*len(demand):
                                solution.write(",<" + site + "," + str(assigned_bandwidth) + ">")
                
                else:
                    if count == len(list(site4client[client])) and line_count != timestamps*len(demand):
                        solution.write("\n")

    solution.close()

