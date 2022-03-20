import csv
import copy
import math
import os.path as osp

path_input = 'data'
path_output = 'output'

def getSiteBandwidth():
    site_bandwidth = {}
    with open(osp.join(path_input, "site_bandwidth.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        for row in f_csv:
            site_bandwidth[row[0]] = int(row[1])
    N = len(site_bandwidth) # N site
    return site_bandwidth, N

def getQoSConstraint():
    with open(osp.join(path_input, "config.ini"), mode='r') as f:
        qos_constraint = int(f.readlines()[1].split("=")[-1])
    return qos_constraint

def getQoS(qos_constraint):
    qos = {}
    client_site_qos_ok = {}  #client -> [site1,site2,...]对于每个客户符合QoS约束的站点
    with open(osp.join(path_input, "qos.csv")) as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        M = len(headers) - 1
        for i in range(1,M+1):
            client_site_qos_ok[headers[i]] = []
        for row in f_csv:
            for i in range(M):
                qos[(row[0], headers[i+1])] = int(row[i+1])
                if int(row[i+1]) < qos_constraint:
                    client_site_qos_ok[headers[i+1]].append(row[0])
    return qos, client_site_qos_ok

def getDemand():
    demand = {}
    with open(osp.join(path_input, "demand.csv")) as f:
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
qos, client_site_qos_ok  = getQoS(qos_constraint)
site_bandwidth, site_number = getSiteBandwidth()


#对于某个客户client，找到满足QoS约束的，且带宽大于请求带宽的服务器
def getSitesBiggerThanRequired(site_remaining_bandwidth, required, client):
    result = []
    #看看能不能用numpy，迅速找出满足条件的服务器，改掉这个for循环
    for site_name, remaining_bandwidth in site_remaining_bandwidth.items():
        if remaining_bandwidth >= required and qos[(site_name, client)] < qos_constraint:
            result.append(site_name)

    return result

def findMaximumSpread(site_reamining_bandwidth, client_total_demand, client):
    spread_dst = []
    max_spread = 0
    #在client_site_qos_ok里面找
    qos_site_remaining_bandwidth = {}
    for s in client_site_qos_ok[client]:
        qos_site_remaining_bandwidth[s] = site_reamining_bandwidth[s]

    for i in range(len(client_site_qos_ok[client]), 0, -1):
        required = math.ceil(client_total_demand / i)
        result = []
        sort_qos_site_reamining_bandwidth = sorted(qos_site_remaining_bandwidth.items(),key=lambda x:x[1], reverse = True)
        #剩余带宽从大到小排序，如果前i个都有比请求多的带宽，说明有这么个方案，返回前i个（带宽剩余绝对值最多的i个）。如果有需要，可以再往后找找，多返回几个。
        if sort_qos_site_reamining_bandwidth[i-1][1] >= required:
            #返回元组列表（site,remaining_bandwidth)
            spread_dst = sort_qos_site_reamining_bandwidth[0:i]
            max_spread = i
            return spread_dst, max_spread
    return spread_dst, max_spread


def average_max_spread():
    #total_cost为每个时刻分配完之后，所有服务器95%带宽的总和，也就是我们需要优化的目标
    solution = open(osp.join(path_output, "solution.txt"),mode='w')

    for t in range(timestamps):
        site_remaining_bandwidth = copy.deepcopy(site_bandwidth)
        #这里处理客户的请求就按列表的顺序。
        for client in demand.keys():
            solution.write(client+":")
            client_left_demand = demand[client][t]
            if client_left_demand == 0:
                if t != timestamps-1:
                    solution.write("\n")
                continue
            spread_dst, max_spread = findMaximumSpread(site_remaining_bandwidth, client_left_demand, client)
            #如果平均分行不通，需要进行其他分配方案 To do，有待补充
            if max_spread == 0:
                print("Error! No solution using max spread.")
            for site, bandwidth in spread_dst:
                client_demand_for_current_site = math.ceil(client_left_demand/max_spread)
                site_remaining_bandwidth[site] -= client_demand_for_current_site
                if site == spread_dst[-1][0]:
                    solution.write("<" + site + "," + str(client_left_demand-client_demand_for_current_site*(len(spread_dst)-1)) + ">")
                else:
                    solution.write("<" + site + "," + str(client_demand_for_current_site) + ">,")
            if t != timestamps-1:
                solution.write("\n")
    solution.close()

'''
先直接将客户的需求平均分配给可用的边缘节点，然后再逐一检查边缘节点是否超过服务能力，
超过再通过递归的形式解决
'''
def average_recursive(timestamp):
    # 将用户需求平均分配到每个节点，记录每个节点上各个用户的使用情况
    site_client_allocation = {}
    for site in site_bandwidth.keys():
        site_client_allocation[site] = []
    for client in demand.keys():
        usage = demand[client][timestamp] // len(client_site_qos_ok[client])
        for i, site in enumerate(client_site_qos_ok[client]):
            if i != len(client_site_qos_ok[client])-1:
                site_client_allocation[site].append({client: usage})
            else:
                site_client_allocation[site].append({client: demand[client][timestamp] - i*usage})
    # 检查每个边缘节点是否超过服务能力
    for site in site_bandwidth.keys():
        allocation = 0
        for client in site_client_allocation[site]:
            for value in client.values():
                allocation += value
        if allocation > site_bandwidth[site]:
            # 广度优先遍历
            queue = []
            for client in site_client_allocation[site]:
                queue.append(client)
            for client in queue:
                for site in client_site_qos_ok[client]:
                    # TODO 未完待续
                    a = 1

if __name__=='__main__':
    # average_max_spread()
    average_recursive(0)
