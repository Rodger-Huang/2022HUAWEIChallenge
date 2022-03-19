import csv
import copy
import math
import thread
import threading
debug = 1
test = 1



def getSiteBandwidth():
    site_bandwidth = {}
    with open("../../data/site_bandwidth.csv") as f:
        f_csv = csv.reader(f)
        headers = next(f_csv)
        for row in f_csv:
            site_bandwidth[row[0]] = int(row[1])
    N = len(site_bandwidth) # N site
    return site_bandwidth, N

def getQoSConstraint():
    with open("../../data/config.ini", mode='r') as f:
        qos_constraint = int(f.readlines()[1].split("=")[-1])
    return qos_constraint

def getQoS():
    qos_constraint = getQoSConstraint()
    qos = {}
    client_site_qos_ok = {}  #client -> [site1,site2,...]对于每个客户符合QoS约束的站点
    with open("/data/qos.csv") as f:
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
    with open("../../data/demand.csv") as f:
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

#对于某个客户client，找到满足QoS约束的，且带宽大于请求带宽的服务器，QOS约束在getQoS那里找好了对每个客户端符合约束的站点列表
'''
def getSitesBiggerThanRequired(site_remaining_bandwidth, required, client, qos, qos_constraint):
    result = []
    for site_name, remaining_bandwidth in site_remaining_bandwidth.items():
        if remaining_bandwidth >= required and qos[(site_name, client)] < qos_constraint:
            result.append(site_name)
    if debug:
        print("required = ",required,"  sites = ",result)
    return result
'''
#-------------全局变量，都是只读的，多个线程可以共享这些全局变量，节省内存使用-------------
demand, timestamps = getDemand()
qos_constraint = getQoSConstraint()
qos, client_site_qos_ok  = getQoS()
site_bandwidth, site_number = getSiteBandwidth()

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



#---------------------------线程之间同步信号量------------------------------------
lock_1 = threading.Lock()
lock_2 = threading.Lock()
lock_3 = threading.Lock()
lock_4 = threading.Lock()
lock_5 = threading.Lock()

def thread_average_max_spread(start_index, end_index, get_lock, release_lock, output):
    if test:
        thread_total_cost = 0
    #这是一个由字典组成的数组，记录本线程负责的时间步的分配方案
    solution_array = []
    for t in range(start_index,end_index):
        current_timestamp_solution = {}
        if test:
            site_request_queue = {}
            for site in  site_bandwidth.keys():
                site_request_queue[site] = [0]

        site_remaining_bandwidth = copy.deepcopy(site_bandwidth)
        for client in demand.keys():
            client_left_demand = demand[client][t]
            spread_dst, max_spread = findMaximumSpread(site_remaining_bandwidth, client_left_demand, client, qos, qos_constraint)
            if max_spread == 0:
                print("Error! To be done.")
            current_timestamp_solution[client] = []
            average_demand = math.ceil(client_left_demand/max_spread)
            for site in spread_dst:
                if client_left_demand < average_demand:
                    site_remaining_bandwidth[site] -= client_left_demand
                    client_left_demand = 0
                    current_timestamp_solution[client].append((site,average_demand))
                    break
                site_remaining_bandwidth[site] -= average_demand
                client_left_demand -= average_demand
                current_timestamp_solution[client].append((site,average_demand))
        solution_array.append(current_timestamp_solution)

        if not get_lock:
            for ts_solution in solution_array:
                for client, site_tuples_list in ts_solution.items():
                    output.write(client+":")
                    for (site, bandwidth) in site_tuples_list:
                        if (site, bandwidth) != site_tuples_list[-1]:
                            output.write("<" + site + "," + str(bandwidth) + ">,")
                        else:
                            output.write("<" + site + "," + str(bandwidth) + ">")
            output.write("\n")
            release_lock.release()
        else:
            get_lock.acquire()
            for ts_solution in solution_array:
                for client, site_tuples_list in ts_solution.items():
                    output.write(client+":")
                    for (site, bandwidth) in site_tuples_list:
                        if (site, bandwidth) != site_tuples_list[-1]:
                            output.write("<" + site + "," + str(bandwidth) + ">,")
                        else:
                            output.write("<" + site + "," + str(bandwidth) + ">")
                output.write("\n")

            release_lock.release()
            



    pass
def average_max_spread():
    #total_cost为每个时刻分配完之后，所有服务器95%带宽的总和，也就是我们需要优化的目标
    if test:
        total_cost = 0
    

    
    solution = open("output/solutions.txt",mode='w')

    if test:
        timestamps = 3
    if debug:
        print("site bandwidth:",site_bandwidth)
    for t in range(timestamps):
        if test:
            site_request_queue = {}
            for site in  site_bandwidth.keys():
                site_request_queue[site] = [0]

        site_remaining_bandwidth = copy.deepcopy(site_bandwidth)
        #这里处理客户的请求就按列表的顺序。
        for client in demand.keys():
            solution.write(client+":")
            client_left_demand = demand[client][t]
            spread_dst, max_spread = findMaximumSpread(site_remaining_bandwidth, client_left_demand, client, qos, qos_constraint)
            #如果平均分行不通，需要进行其他分配方案 To do，有待补充
            if max_spread == 0:
                print("Error! No solution using max spread.")
            for site, bandwidth in spread_dst:
                client_demand_for_current_site = math.ceil(client_left_demand/max_spread)
                site_remaining_bandwidth[site] -= client_demand_for_current_site

                if test:
                    site_request_queue[site].append(client_demand_for_current_site)

                if site == spread_dst[-1][0]:
                    solution.write("<" + site + "," + str(client_left_demand-client_demand_for_current_site*(len(spread_dst)-1)) + ">,")
                else:
                    solution.write("<" + site + "," + str(client_demand_for_current_site) + ">")
            solution.write("\n")
        
        if test:
            current_ts_total_cost = 0
            for site, request_queue in site_request_queue.items():
                tail95_index = math.ceil(len(request_queue) * 0.95) - 1
                if debug:
                    print("tail95_index = ",tail95_index)
                request_queue.sort()
                if debug:
                    print("request queue = ",request_queue)
                tail95_bandwidth = request_queue[tail95_index]
                current_ts_total_cost += tail95_bandwidth
            total_cost += current_ts_total_cost
    if debug and test:
        print("total cost = ", total_cost)
    
    solution.close()
        
if __name__=='__main__':
    
    lock_1.acquire()
    lock_2.acquire()
    lock_3.acquire()
    lock_4.acquire()
    lock_5.acquire()
    step = math.ceil(timestamps/5)
    solution = open("output/solutions.txt",mode='w')
    th1 = threading.Thread(target=thread_average_max_spread, args=(0, step, None, lock_1, solution))
    th2 = threading.Thread(target=thread_average_max_spread, args=(step, step*2, lock_1, lock_2, solution))
    th3 = threading.Thread(target=thread_average_max_spread, args=(step*2, step*3, lock_2, lock_3, solution))
    th4 = threading.Thread(target=thread_average_max_spread, args=(step*3, step*4, lock_3, lock_4, solution))
    th5 = threading.Thread(target=thread_average_max_spread, args=(step*4, timestamps, lock_4, lock_5, solution))

    lock_5.acquire()

    solution.close()



    average_max_spread()
