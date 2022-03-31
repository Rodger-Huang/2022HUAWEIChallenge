import csv
import math
from pydoc import cli
import sys
import os
import random
import os.path as osp
import numpy as np
import copy
from scipy import rand

# NOTE 提交前记得修改路径
input_path = "../../data/"
output_path = "../solutions/output/solution.txt"
#一个比平均好一些的初始解
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

    big_bandwidth_record = [] #list of tuple (timestamps, client, bandwidth) 统计大流量，在哪个时刻，由哪个客户产生
    least_big_bandwidth = 999999999

    client_name_order = []
    demand_list = []
    
    debug_big_bandwidth_variety = []
    for client in demand.keys():
        client_name_order.append(client)
        demand_list.append(demand[client])
    demand_list_np = np.array(demand_list)
    demand_list_without_95 = copy.deepcopy(demand_list_np)
    #print(demand_list_np.shape)
    index = np.argpartition(demand_list_np.ravel(),-number_of_big_bandwidth)[-number_of_big_bandwidth:]
    index_2d = np.unravel_index(index,demand_list_np.shape)
    #print(index_2d)
    for id in range(len(index_2d[0])):
        big_bandwidth_record.append((index_2d[1][id],client_name_order[index_2d[0][id]],demand_list_np[index_2d[0][id]][index_2d[1][id]]))
        if demand_list_np[index_2d[0][id]][index_2d[1][id]] < least_big_bandwidth:
            least_big_bandwidth = demand_list_np[index_2d[0][id]][index_2d[1][id]]
        demand_list_without_95[index_2d[0][id]][index_2d[1][id]] = 0
        debug_big_bandwidth_variety.append(demand_list_np[index_2d[0][id]][index_2d[1][id]])
    #还不确定是不是要减一，需要运行一下查看
    big_bandwidth_per_site = int(number_of_big_bandwidth / site_number)
    #print("big_bandwidth_record")
    #print(big_bandwidth_record)
    #print(debug_big_bandwidth_variety) 有必要对大流量先后排序进行处理，最大的先找server给它
    client_total_demand_without95 = np.sum(demand_list_without_95,1)
    #print(client_total_demand_without95)
    client_site_number = []
    for client in client_name_order:
        client_site_number.append(len(site4client[client]))
    #print(client_site_number)
    client_per_server_exp_demand = client_total_demand_without95 / np.array(client_site_number)
    #print(client_per_server_exp_demand)

    #这里 site_per_t_exp算上了大流量。也许不算大流量可能会优化一点？？？？
    site_per_t_exp = {}
    for site in client4site.keys():
        site_total = 0
        for i in range(len(client_name_order)):
            client = client_name_order[i]
            if client in client4site[site]:
                site_total += client_per_server_exp_demand[i]
        test_scale = 1 #default = 1
        site_per_t_exp[site] = int(site_total / timestamps /test_scale)
    
    #print(site_per_t_exp)
    site_per_t_exp = site_per_t_exp
    return big_bandwidth_record, big_bandwidth_per_site, least_big_bandwidth, site_per_t_exp

#大流量记录， 每个站点可容纳的大流量数目，最小的大流量，每个时间步站点期望处理的流量数目。
big_bandwidth_record, big_bandwidth_per_site, least_big_bandwidth, site_per_t_exp = demandAnalysis()


debug_dict = {'CK': 5, 'Dm': 5, 'CP': 5, 'Dx': 5, 'BC': 5, 'Bx': 5, 'Db': 5, 'C4': 5, 'DZ': 5, 'Dp': 5, 'DF': 5, 'DE': 5, 'A3': 5, 'Dj': 5, 'Dh': 5, 'Dn': 5, 'DD': 5, 'DA': 5, 'C9': 5, 'C8': 5, 'Cz': 5, 'CT': 5, 'Cq': 5, 'C': 5, 'CS': 5, 'P': 5, 'Du': 5, 'Dr': 5, 'CF': 5, 'Ak': 5, 'Cg': 5, 'Q': 5, 'Bb': 5, 'BV': 5, 'Ce': 5, 'AD': 5, 'BM': 5, 'Cb': 5, 'AF': 5, 'BW': 5, 'A': 5, 'Ao': 5, 'Be': 5, 'Dq': 5, 'AG': 5, 'BR': 5, 'A7': 5, 'Dt': 5, 'Bh': 5, 'B8': 5, 'Bo': 5, 'B': 5, 'CZ': 5, 'DU': 5, 'D': 5, 'Ab': 5, 'Br': 5, 'Bv': 5, 'B0': 5, 'E': 5, 'DV': 5, 'Bt': 5, 'Ar': 5, 'Cs': 5, 'Dw': 5, 'CB': 5, 'B7': 5, 'AJ': 5, 'Do': 5, 'Ag': 5, 'AV': 5, 'AS': 5}
if __name__ == '__main__':
    
    solution = open(output_path,mode='w')
    #记录每个site已经接收的大带宽的客户数目
    
    debug_site_over_ts = {}
    #这是对于所有时间步来进行统计的
    site_big_bandwidth_in_number = {}

    big_bandwidth_order = sorted(big_bandwidth_record, key = lambda x:x[2], reverse = True)
    #print(big_bandwidth_order)

    #记录在每个时间步，站点对应的可服务的大流量客户。
    ts_site_big_bandwidth_client = {} #dict timestamp->site->[client,...]
    for ts, client, bandwidth in big_bandwidth_order:
        if ts not in ts_site_big_bandwidth_client.keys():
            ts_site_big_bandwidth_client[ts] = {}
        for site in site4client[client]:
            if site not in ts_site_big_bandwidth_client[ts].keys():
                ts_site_big_bandwidth_client[ts][site] = [client]
            else:
                ts_site_big_bandwidth_client[ts][site].append(client)
    
    #print("ts site big bandwidth client:", ts_site_big_bandwidth_client)
    #记录时间步 站点分配流量情况，针对大流量的记录
    timestamp_site_client_allocation = {} #dict timestamp-> site->[(client,bandwidth),(client,bandwidth),...]
    #记录时间步 客户分出流量情况，针对大流量的记录
    timestamp_client_site_spread = {} #dict timestamp -> client -> [(site,bandwidth),(site,bandwidth),...]

    #首先对大流量进行分配
    #这里需要记录进行了操作的timestamps和分配情况
    ts_processed_big_client = {} # dict timestamp -> [client, clinet,...]
    for ts, client, bandwidth in big_bandwidth_order:
        #首先选择一个“较好的”服务器，按照原来的方法选
        #if ts == 2:
        #    print("client = ", client)
        chosen_site = ""
        #random.shuffle(site4client[client])
        for site in site4client[client]:
            #if site not in debug_dict.keys():
            #    print("site ", site, "not in debug_dict, it should be chosen first")
            if site in site_big_bandwidth_in_number.keys() and site_big_bandwidth_in_number[site] >= big_bandwidth_per_site:
                #print("site already full big bandwidth, site=",site)
            #    if ts == 2:
            #        print("site ", site, " already full.")
                continue
            site_serve_big_client_num = 0
            if site in ts_site_big_bandwidth_client[ts].keys():
                site_serve_big_client_list = ts_site_big_bandwidth_client[ts][site]
                site_serve_big_client_num = len(site_serve_big_client_list)
                if ts in ts_processed_big_client.keys():
                    for c in site_serve_big_client_list:
                        if c == client:
                            continue
                        elif c in ts_processed_big_client[ts]:
                            site_serve_big_client_num -= 1
            site_current_ts_used_bandwidth = 0
            if ts in timestamp_site_client_allocation.keys() and site in timestamp_site_client_allocation[ts].keys():
                for c, b in timestamp_site_client_allocation[ts][site]:
                    site_current_ts_used_bandwidth += b
            site_remaining_bandwidth = site_bandwidth[site] - site_current_ts_used_bandwidth
            #if ts == 2:
            #    print("site ",site, " remain bw=",site_remaining_bandwidth," serve big client num=",site_serve_big_client_num)
            if site_serve_big_client_num == 1 and site_remaining_bandwidth >= int(bandwidth*0.7):
                chosen_site = site
                break
            #或者找一个当前时间步剩余流量大于客户当前时间步请求流量的70%的服务器
            elif site_remaining_bandwidth >= int(bandwidth*0.7):
                chosen_site = site
        #如果找到了这么个服务器
        if chosen_site != "":
            if ts not in ts_processed_big_client.keys():
                ts_processed_big_client[ts] = [client]
            else:
                ts_processed_big_client[ts].append(client)
            if chosen_site in site_big_bandwidth_in_number.keys():
                site_big_bandwidth_in_number[chosen_site] += 1
            else:
                site_big_bandwidth_in_number[chosen_site] = 1  
            
            allocate_bandwidth = 0
            chosen_site_ts_used_bw = 0
            if ts in timestamp_site_client_allocation.keys() and chosen_site in timestamp_site_client_allocation[ts].keys():
                for c, b in timestamp_site_client_allocation[ts][chosen_site]:
                    chosen_site_ts_used_bw += b
            chosen_site_remaining_bw = site_bandwidth[chosen_site] - chosen_site_ts_used_bw
            if chosen_site_remaining_bw >= bandwidth:
                allocate_bandwidth = bandwidth
            else:
                allocate_bandwidth = chosen_site_remaining_bw
            #进行资源分配，修改相应的记录
            if ts in timestamp_site_client_allocation.keys():
                if chosen_site in timestamp_site_client_allocation.keys():
                    timestamp_site_client_allocation[ts][chosen_site].append((client,allocate_bandwidth))
                else:
                    timestamp_site_client_allocation[ts][chosen_site] = [(client,allocate_bandwidth)]
            else:
                timestamp_site_client_allocation[ts] = {}
                timestamp_site_client_allocation[ts][chosen_site] = [(client,allocate_bandwidth)]
            
            if ts in timestamp_client_site_spread.keys():
                if client in timestamp_client_site_spread.keys():
                    timestamp_client_site_spread[ts][client].append((chosen_site,allocate_bandwidth))
                else:
                    timestamp_client_site_spread[ts][client] = [(chosen_site, allocate_bandwidth)]
            else:
                timestamp_client_site_spread[ts] = {}
                timestamp_client_site_spread[ts][client] = [(chosen_site,allocate_bandwidth)]


        else:
            pass
            '''
            print("big client = ", client, " at time=",ts," bandwidth=",bandwidth," not find server")
            if ts == 2:
                print("big client = ", client, " at time=",ts," bandwidth=",bandwidth," not find server")
                print(timestamp_client_site_spread[2])
                print(timestamp_site_client_allocation[2])
                print(site4client[client])
                print(site_big_bandwidth_in_number)
                print(len(site_big_bandwidth_in_number.keys()))
            '''
    #print("ts client site spread")
    #print(timestamp_client_site_spread)
    #print("ts site client allocation")
    #print(timestamp_site_client_allocation)
    print("before")
    print(site_big_bandwidth_in_number)
    print("len=",len(site_big_bandwidth_in_number))

    #存在这么一些服务器，没有压根没有对应到任意一个大流量，这些服务器的95%后带宽要利用起来，找出时间步与这些服务器的对应关系
    consider_site = []
    possible_accu_number = 0
    for site in site_bandwidth.keys():
        if site not in site_big_bandwidth_in_number.keys():
            consider_site.append(site)
            possible_accu_number += big_bandwidth_per_site
        elif site_big_bandwidth_in_number[site] < big_bandwidth_per_site:
            consider_site.append(site)
            possible_accu_number += big_bandwidth_per_site - site_big_bandwidth_in_number[site]
    print("possible accu", possible_accu_number)
    
    print("consider site",consider_site,"len=",len(consider_site))
    consider_site_possible_serve_bw = {}
    for t in range(timestamps):
        for s in consider_site:
            max_possible_serve_bw = 0
            all_client = client4site[site]
            for c in all_client:
                #不算那些已经被服务器接收的大流量
                if t in timestamp_client_site_spread.keys() and c in timestamp_client_site_spread.keys():
                    continue
                max_possible_serve_bw += demand[c][t]
            if s not in consider_site_possible_serve_bw.keys():
                consider_site_possible_serve_bw[s] = [(t,max_possible_serve_bw)]
            else:
                consider_site_possible_serve_bw[s].append((t,max_possible_serve_bw))
    

    idx_consider_site = {}
    consider_site_for_timestamp = {} #***后续要用的。dict 每个时间步找一个吸收很多客户流量的site
    random.shuffle(consider_site)
    for s in consider_site_possible_serve_bw.keys():
        idx_consider_site[s] = 0
        tmp_list = sorted(consider_site_possible_serve_bw[s], key=lambda x:x[1], reverse=True)
        consider_site_possible_serve_bw[s] = tmp_list
    loop_counter = 0
    while loop_counter < possible_accu_number:
        this_round_max = 0
        this_round_site = ""
        this_round_ts = -1
        for s in consider_site:
            if idx_consider_site[s] == timestamps - 1:
                print("site=",s,"already reach all ts.")
                consider_site.remove(s)
                continue
            bw = consider_site_possible_serve_bw[s][idx_consider_site[s]][1]
            ts = consider_site_possible_serve_bw[s][idx_consider_site[s]][0]
            #print("idx=",idx_consider_site[s],"site=",s,"bw=",bw,"ts=",ts)
            #print("bw=",bw,"ts=",ts)
            if bw > this_round_max:
                this_round_max = bw
                this_round_site = s
                this_round_ts = ts
        #可能有那么一些站点从始至终都没有能够服务的流量
        if this_round_max == 0:
            break
        idx_consider_site[this_round_site] += 1
        
        #如果说本轮找到的这个最大值对应的时间步已经有站点了，那么就不要这个了
        if ts in consider_site_for_timestamp.keys():
            #print("ts already in.------------------------------------ ts=",ts,"site=",this_round_site,"bw=",this_round_max)
            continue
        #print("this round max bw=",this_round_max," site=",this_round_site, "ts=",this_round_ts)
        consider_site_for_timestamp[this_round_ts] = this_round_site
        if this_round_site in site_big_bandwidth_in_number.keys():
            site_big_bandwidth_in_number[this_round_site] += 1
        else:
            site_big_bandwidth_in_number[this_round_site] = 1
        if site_big_bandwidth_in_number[this_round_site] == big_bandwidth_per_site:
            consider_site.remove(this_round_site)
        loop_counter += 1

    print(site_big_bandwidth_in_number)
    print(len(site_big_bandwidth_in_number.keys()))
    print("--------")
    print(consider_site_for_timestamp)

    #-----------以上找出了大流量的分配方案，以及每个时间步吸收非大流量客户的站点k-------------
    #在每个时间步进行处理的时候 1.先根据大流量分配方案把大流量给实在地分配了 2.让吸收了大流量的站点继续吸收流量 3.对于站点k，让它们尽量吸收流量
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

        client_info_order = sorted(client_info.items(), key=lambda x:x[1][2], reverse=False)
        debug_client_got_bandwidth = {}
        for c in client_info.keys():
            debug_client_got_bandwidth[c] = 0
        current_ts_has_big_bandwidth_and_available_site = []
        #1. 落实大流量的分配
        #如果当前时间步存在大流量的分配方案, 落实本时间步的大流量分配
        if t in timestamp_site_client_allocation.keys():
            for s in timestamp_site_client_allocation[t].keys():
                for c, bw in timestamp_site_client_allocation[t][s]:
                    site_info[s][1] -= bw
                    client_info[c][1] -= bw

                    if s not in client_info[c][3].keys():
                        site_info[s][3][c] = bw
                        client_info[c][3][s] = bw
                    else:
                        site_info[s][3][c] += bw
                        client_info[c][3][s] += bw
                    debug_client_got_bandwidth[c] += bw
                    #理论上这个if判断可以去掉，只要前面的大流量分配正确，这里是不会出问题的。
                    if site_info[s][1] < 0 or client_info[c][1] < 0:
                        print("ERROR! CHECK BIG BANDWIDTH ALLOCATION!")
                if site_info[s][1] > 0:
                    current_ts_has_big_bandwidth_and_available_site.append(s)

        #2. 让吸收了大流量的站点继续吸收流量-----------------biggest first----------------------
        
        for site in current_ts_has_big_bandwidth_and_available_site:
            actual_client = list(client4site[site])
            #这个站点对应的能服务的客户，暂未排序，看看后续是否需要排序
            for client in actual_client:
                if site_info[site][1] == 0:
                    current_ts_has_big_bandwidth_and_available_site.remove(site)
                    break
                elif site_info[site][1] < 0:
                    print("ERROR!!! SITE OVER LIMIT")
                if client_info[client][1] == 0:
                    continue
                allocate_bandwidth = 0
                if site_info[site][1] >= client_info[client][1]:
                    allocate_bandwidth = client_info[client][1]
                else:
                    allocate_bandwidth = site_info[site][1]
                
                client_info[client][1] -= allocate_bandwidth
                site_info[site][1] -= allocate_bandwidth
                debug_client_got_bandwidth[client] += allocate_bandwidth
                if site not in client_info[client][3].keys():
                    client_info[client][3][site] = allocate_bandwidth
                    site_info[site][3][client] = allocate_bandwidth
                else:
                    client_info[client][3][site] += allocate_bandwidth
                    site_info[site][3][client] += allocate_bandwidth

        #--------------biggest first end----------------
        # 3.对于consider站点，尽量吸收流量
        #如果当前时间步存在这么个站点,目前只考虑一个站点的情况
        if t in consider_site_for_timestamp.keys():
            s = consider_site_for_timestamp[t]
            correspond_client = list(client4site[s])
            for c in correspond_client:
                if site_info[s][1] == 0:
                    break
                elif site_info[s][1] < 0:
                    print("ERROR!!! CONSIDER SITE OVER LIMIT!")
                if client_info[c][1] == 0:
                    continue
                allocate_bandwidth = 0
                if site_info[s][1] >= client_info[c][1]:
                    allocate_bandwidth = client_info[c][1]
                else:
                    allocate_bandwidth = site_info[s][1]
                client_info[c][1] -= allocate_bandwidth
                site_info[s][1] -= allocate_bandwidth
                debug_client_got_bandwidth[c] += allocate_bandwidth
                if s not in client_info[c][3].keys():
                    client_info[c][3][s] = allocate_bandwidth
                    site_info[s][3][c] = allocate_bandwidth
                else:
                    client_info[c][3][s] += allocate_bandwidth
                    site_info[s][3][c] += allocate_bandwidth

        #-----------------------------------
        #对于剩下的客户流量，按原来方案进行分配
        for client in [x[0] for x in client_info_order]:
            actual_site = list(site4client[client])
            #----------------weighted-----------------------------
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
                    debug_client_got_bandwidth[client] += allocate_bandwidth
                    #print("allocate bandwidth for client, from site = ",site,"bandwidth=",allocate_bandwidth )
                    client_info[client][1] -= allocate_bandwidth
                    site_info[site][1] -= allocate_bandwidth
                    if site not in client_info[client][3].keys():
                        client_info[client][3][site] = allocate_bandwidth
                        site_info[site][3][client] = allocate_bandwidth
                    else:
                        client_info[client][3][site] += allocate_bandwidth
                        site_info[site][3][client] += allocate_bandwidth
            #----------------------------weighted end---------------------
            if debug_client_got_bandwidth[client] != client_info[client][0]:
                print("ERROR! CLIENT BANDWIDTH NOT SATISFIED. time=",t,"client=",client)
        for site in site_info.keys():
            if site in debug_site_over_ts.keys():
                debug_site_over_ts[site].append(site_info[site][0] - site_info[site][1])
            else:
                debug_site_over_ts[site] = [site_info[site][0] - site_info[site][1]]
        
        
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
    print(site_big_bandwidth_in_number)
    #print(debug_site_over_ts)
    solution.close()
                    




                
                    
                        
                





