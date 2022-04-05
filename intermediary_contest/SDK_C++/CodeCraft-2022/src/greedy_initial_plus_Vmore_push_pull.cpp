#include "helpers.hpp"
using namespace std;

// 加上第二阶段的push score = 
//-------------------全局变量------------------
int qos_constraint = getConstraint();
int base_cost = getBaseCost();
map<pair<string,string>, int> qos = getQoS();
map<string, int> site_bandwidth = getSiteBandwidth();
map<string, vector<vector<pair<string,int>>>> demand = getDemand();

map<string, vector<string>> site4client = getSiteForClient(qos,qos_constraint);
map<string, vector<string>> client4site = getClientForSite(qos,qos_constraint);
map<string, set<string>> set_site4client = getSetSiteForClient(qos,qos_constraint);
map<string, set<string>> set_client4site = getSetClientForSite(qos,qos_constraint);

map<string, int> client_site_number;
map<string, int> site_client_number;

// sort client by site number
vector<pair<string, int>> client_order;
vector<pair<string, int>> site_order;

//----------------------全局变量---------------------
//更新client_site_number等
void operateOnGlobalVariable(){
    for(auto it = site4client.begin(); it != site4client.end(); it++){
        client_site_number.insert(make_pair(it->first, it->second.size()));
        client_order.push_back(make_pair(it->first, it->second.size()));
    }
    sort(client_order.begin(), client_order.end(), cmp);
    // 对节点根据客户数量从少到多进行排序，少客户的节点有更大概率95%值比较小
    for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
        string site = it->first;
        if(client4site.find(site) != client4site.end()){
            site_order.push_back(make_pair(it->first, client4site[site].size()));
        }else{
            site_order.push_back(make_pair(it->first, 0));
        }
    }
    sort(site_order.begin(), site_order.end(), cmp);

    for(auto it = client4site.begin(); it != client4site.end(); it++){
        site_client_number.insert(make_pair(it->first, it->second.size()));
    }
}

map<string, vector<string>> getSiteOrderForClient(){
    map<string, vector<string>> result;
    //对每一个client, 对其site排序,根据site对应的可服务client数目排,由多到少
    for(auto it = site4client.begin(); it != site4client.end(); it++){
        string client = it->first;
        vector<pair<string,int>> this_client_site_order_tmp;
        for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
            string site = (*iter);
            if(client4site.find(site) != client4site.end()){
                this_client_site_order_tmp.push_back(make_pair(site,client4site[site].size()));
            }
            else{
                this_client_site_order_tmp.push_back(make_pair(site,0));
            }
        }
        sort(this_client_site_order_tmp.begin(), this_client_site_order_tmp.end(), cmp);
        vector<string> this_client_site_order;
        for(int i = 0; i < this_client_site_order_tmp.size(); i++){
            this_client_site_order.push_back(this_client_site_order_tmp[i].first);
        }
        result[client] = this_client_site_order;
    }
    if(CHECK_DATA){
        for(auto it = result.begin(); it != result.end(); it++){
            cout<<"client = "<< it->first<<" site order = ";
            for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
                cout<<(*iter)<<" ";
            }
            cout<<endl;
        }
    }
    return result;
}
map<pair<string,string>, vector<string>> getSiteCommonClient(){
    map<pair<string,string>, vector<string>> result;
    for(auto it = set_client4site.begin(); it != set_client4site.end(); it++){
        string site1 = it->first;
        for(auto it2 = set_client4site.begin(); it2 != set_client4site.end(); it2++){
            string site2 = it2->first;
            if(site1 == site2)
                continue;
            //寻找site1和site2的共同客户
            //遍历site1的所有客户
            for(auto cl = it->second.begin(); cl != it->second.end(); cl++){
                string client = *cl;
                if(set_client4site[site2].find(client) != set_client4site[site2].end()){
                    pair<string,string> key = make_pair(site1,site2);
                    if(result.find(key) == result.end()){
                        vector<string> client_list;
                        client_list.push_back(client);
                        result[key] = client_list;
                    }
                    else{
                        result[key].push_back(client);
                    }
                }
            }
        }
    }
    return result;
}
//计算每个每个时间步，每个服务器可能服务的最大流量大小。
//选出那些最大的。（优先选择对应客户数目多的？（这样可能保存较多0站点）或者优先选择对应客户数目少的？）
//记录每个时间步，大流量服务器名字，以及流量分配情况
//每个时间步一般选一个，如果发现说两个可以吸收大流量的站点在同一个时间步都能吸收大流量，那么查看这两个站点的共同客户系数：共同客户数目小于0则允许它们处于同一时间步
//map timestamp-> map site_name map client name -> vector stream type and bandwidth

map<int, vector<string>> getTimestampPossibleBig(){
    map<pair<string,string>, vector<string>> site_common_client = getSiteCommonClient();
    map<int, vector<string>> result;
    map<string, vector<int>> site_possible_serve;
    for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
        vector<int> tmp;
        site_possible_serve[si->first] = tmp;
    }
    for(int t = 0; t < timestamps; t++){
        //计算当前时间步每个客户的总需求
        map<string, int> client_current_ts_sum;
        for(auto de_iter = demand.begin(); de_iter != demand.end(); de_iter++){
            string client = de_iter->first;
            vector<pair<string,int>> current_cl_need = demand[client][t];
            int cl_sum = 0;
            for(auto it = current_cl_need.begin(); it != current_cl_need.end(); it++){
                cl_sum += it->second;
            }
            client_current_ts_sum[client] = cl_sum;
        }
        for(auto si_iter = site_bandwidth.begin(); si_iter != site_bandwidth.end(); si_iter++){
            string site = si_iter->first;
            int this_site_most_serve = 0;
            for(auto cl4si = client4site[site].begin(); cl4si != client4site[site].end(); cl4si++){
                this_site_most_serve += client_current_ts_sum[*cl4si];
            }
            if(this_site_most_serve > site_bandwidth[site]){
                this_site_most_serve = site_bandwidth[site];
            }
            site_possible_serve[site].push_back(this_site_most_serve);
        }
    }
    map<string, vector<int>> site_idx;
    map<string, int> site_idx_indicator;
    map<string, int> site_accepted_big_number;
    for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
        vector<int> index = argsort(site_possible_serve[si->first]); //排从小到大
        site_idx[si->first] = index;
        site_idx_indicator[si->first] = timestamps - 1;
        site_accepted_big_number[si->first] = 0;
    }
    set<string> not_usable_site;
    while(not_usable_site.size() < site_bandwidth.size()){
        int this_round_biggest = 0;
        string this_round_site = "";
        int this_round_ts = -1;
        for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
            if(site_idx_indicator[si->first] < 0){
                not_usable_site.insert(si->first);
                continue;
            } //如果这个站点遍历完了这个列表
                
            if(site_accepted_big_number[si->first] > floor(timestamps * 0.05)){
                not_usable_site.insert(si->first);
                continue;  //如果站点接收大流量数目太多了
            }
            int time_index = site_idx[si->first][site_idx_indicator[si->first]];
            int current_si_bw = site_possible_serve[si->first][time_index];
            if(current_si_bw > this_round_biggest){
                this_round_biggest = current_si_bw;
                this_round_site = si->first;
                this_round_ts = time_index;
            }
            site_idx_indicator[si->first]--;

        }
        //if(this_round_biggest < base_cost)
        //    break;
        if(this_round_site != ""){
            if(result.find(this_round_ts) != result.end()){
                bool can_add = true;
                for(auto ss = result[this_round_ts].begin(); ss != result[this_round_ts].end(); ss++){
                    pair<string, string> tmp_key =make_pair(*ss, this_round_site);
                    if(site_common_client[tmp_key].size() > 0){
                        can_add = false;
                        break;
                    }
                }
                if(can_add){
                    result[this_round_ts].push_back(this_round_site);
                    site_accepted_big_number[this_round_site] += 1;
                }
            }
            else{
                vector<string> tmp;
                tmp.push_back(this_round_site);
                result[this_round_ts] = tmp;
                site_accepted_big_number[this_round_site] += 1;
            }
        }
    }
    return result;
}

int main(){
    operateOnGlobalVariable();
    map<string, vector<string>> site_order_for_client = getSiteOrderForClient();
    map<pair<string,string>, vector<string>> site_common_client = getSiteCommonClient();
    //map为site到一个长度为timestamp的vector的映射,记录site每个时间步使用情况
    //对于上述vector中的每个元素,是一个客户名字到其分配到这个站点的种类的流量的情况
    //将原来能用int表示的单个客户需求,用vector<pair<string,int>>表示
    map<string, vector<map<string,vector<pair<string,int>>>>> site_t;
    map<string, vector<int>> site_t_usage;

    map<int, vector<string>> ts_possible_big = getTimestampPossibleBig();

    for(int t = 0; t < timestamps; t++){
        map<string, vector<pair<string,int>>> client_remaining;
        map<string, int> site_remaining = site_bandwidth;
        map<string, int> site_current_bw = site_bandwidth;
        map<string, map<string,vector<pair<string,int>>>> site_t_client;
        map<string, int> site_current_ts_usage;
        //找出当前时刻,客户的需求,初始化client_remaining
        for(auto it = demand.begin(); it != demand.end(); it++){
            client_remaining.insert(make_pair(it->first, it->second[t]));
        }
        //首先处理大流量
        vector<string> possible_big_site = ts_possible_big[t];
        for(auto si = possible_big_site.begin(); si != possible_big_site.end(); si++){
            for(auto cl = client4site[*si].begin(); cl != client4site[*si].end(); cl++){
                vector<pair<string, int>>& stream_list = client_remaining[*cl];
                sort(stream_list.begin(), stream_list.end(), cmp);
                for(auto st_iter = stream_list.begin(); st_iter != stream_list.end();){
                    string stream_type = (*st_iter).first;
                    int stream_bw = (*st_iter).second;
                    if(stream_bw <= site_remaining[*si]){
                        st_iter = stream_list.erase(st_iter);
                        site_remaining[*si] -= stream_bw;
                        if(site_t_client.find(*si) != site_t_client.end() && site_t_client[*si].find(*cl) != site_t_client[*si].end()){
                            site_t_client[*si][*cl].push_back(make_pair(stream_type,stream_bw));
                        }
                        else{
                            vector<pair<string,int>> tmp;
                            tmp.push_back(make_pair(stream_type,stream_bw));
                            site_t_client[*si][*cl] = tmp;
                        }
                    }     
                    else{
                        st_iter++;
                    }    
                }
            }
        }
        for(auto co = client_order.begin(); co != client_order.end(); co++){ // 先处理可用边缘节点多的客户
            string client = co->first;
            // 这里actual_site使用排序,正序,倒序都可以试一下,练习赛中目前是si=actual_site.begin()开始比较好
            vector<string> actual_site = site_order_for_client[client]; // 先分配连接客户数多的边缘节点
            // 这里暂且使用擦除vector中元素的做法,如果出错,可替换成加一个flag进行判断,去计算vector中每个元素是否为0
            auto si = actual_site.begin();
            auto remain_iter = client_remaining[client].begin();
            string stream_type = (*remain_iter).first;
            int stream_bw = (*remain_iter).second;
            while(remain_iter != client_remaining[client].end()){    
                // 按站点对应客户从多到少的顺序来,这样得分更高
                string site = *si;
                // 找到一个能用的site
                if(site_remaining[site] >= stream_bw){
                    remain_iter = client_remaining[client].erase(remain_iter);
                    site_remaining[site] -= stream_bw;
                    
                    if(site_t_client.find(site) != site_t_client.end() && site_t_client[site].find(client) != site_t_client[site].end()){
                        site_t_client[site][client].push_back(make_pair(stream_type, stream_bw));
                    }
                    else{
                        vector<pair<string, int>> tmp;
                        tmp.push_back(make_pair(stream_type, stream_bw));
                        site_t_client[site][client] = tmp;
                    }
                    if(remain_iter != client_remaining[client].end()){
                        stream_type = (*remain_iter).first;
                        stream_bw = (*remain_iter).second;
                    }
                }
                si++;
                if(si == actual_site.end()){
                    si = actual_site.begin();
                }
            }
        }
        for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
            string site = si->first;
            int assigned_bw = site_current_bw[site] - site_remaining[site];
            site_current_bw[site] = site_remaining[site];
            if(site_current_ts_usage.find(site) != site_current_ts_usage.end()){
                site_current_ts_usage[site] += assigned_bw;
            }
            else{
                site_current_ts_usage[site] = assigned_bw;
            }
        }
        for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
            string site = it->first;
            if(site_t_client.find(site) != site_t_client.end()){
                site_t[site].push_back(site_t_client[site]);
            }
            else{
                map<string,vector<pair<string,int>>> tmp;
                site_t[site].push_back(tmp);
            }
            site_t_usage[site].push_back(site_current_ts_usage[site]);
        }
    }


    int position_95 = int(ceil(timestamps * 0.95) - 1);
    for(int kk = 0; kk < 1; kk++){
    for(int i = 0; i < 1; i++){
        set<string> zero_site;
        set<string> bigger_than_V_site;
        for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
            string site = it->first;
            vector<int> index = argsort(site_t_usage[site]);
            int value_95 = site_t_usage[site][index[position_95]];
            if(value_95 > base_cost){
                bigger_than_V_site.insert(site);
            }
            else if(value_95 == 0 && site_t_usage[site][index[timestamps-1]] == 0){
                zero_site.insert(site);
            }
        }
        //对于每个95%位值大于V的站点，（这里不排序），给它找一个公共客户最多的0站点（注意，若0站点的95%位值为V了就将它从0站点移除）
        for(auto it = bigger_than_V_site.begin(); it != bigger_than_V_site.end(); it++){
            string big_V_site = (*it);
            //对当前站点，看看所有zero site跟它的共同客户个数，按照这个排个序
            vector<pair<string, int>> zero_site_common_client_num;
            for(auto zero_iter = zero_site.begin(); zero_iter != zero_site.end(); zero_iter++){
                pair<string,string> tmp_key = make_pair(big_V_site,*zero_iter);
                int com_cli_num = site_common_client[tmp_key].size();
                zero_site_common_client_num.push_back(make_pair(*zero_iter, com_cli_num));
            }
            //从共同客户数目多到少排序
            sort(zero_site_common_client_num.begin(), zero_site_common_client_num.end(), cmp);
            for(int k = 0; k < zero_site_common_client_num.size(); k++){
                vector<int> index = argsort(site_t_usage[big_V_site]);
                int value_95 = site_t_usage[big_V_site][index[position_95]];
                if(value_95 <= base_cost)
                    break;
                
                //要push过去的目标站点
                string target_site = zero_site_common_client_num[k].first;
                pair<string,string> tmp_key = make_pair(big_V_site,target_site);
                vector<string> same_client = site_common_client[tmp_key];
                //对若干个95%及之前的时刻
                for(int pos = 0; pos <= position_95; pos++){
                    int t = index[pos];
                    bool current_t_full = false; //当前时间步给它分担的站点满V没有
                    if(site_t_usage[big_V_site][t] > base_cost){ //大于V了就push到目标站点
                        for(auto cl = same_client.begin(); cl != same_client.end(); cl++){
                            if(current_t_full == true || site_t_usage[big_V_site][t] <= base_cost){
                                break;
                            }
                            vector<pair<string,int>>& same_client_stream_list = site_t[big_V_site][t][*cl];
                            for(auto st_iter = same_client_stream_list.begin(); st_iter != same_client_stream_list.end();){
                                if(site_t_usage[target_site][t] >= base_cost || site_t_usage[big_V_site][t] <= base_cost){
                                    current_t_full = true;
                                    break;
                                }
                                string stream_type = (*st_iter).first;
                                int stream_bw = (*st_iter).second;
                                if(stream_bw + site_t_usage[target_site][t] <= base_cost){
                                    pair<string, int> move_flow = make_pair(stream_type,stream_bw);
                                    site_t[target_site][t][*cl].push_back(move_flow);
                                    site_t_usage[target_site][t] += move_flow.second;
                                    st_iter = same_client_stream_list.erase(st_iter);
                                    site_t_usage[big_V_site][t] -= move_flow.second;
                                }
                                else{
                                    st_iter++;
                                }
                            }
                        }
                    }
                }

                vector<int> tmp_idx = argsort(site_t_usage[target_site]);
                if(site_t_usage[target_site][tmp_idx[position_95]] >= base_cost){
                    for(auto zz = zero_site.begin(); zz != zero_site.end(); zz++){
                        if(*zz == target_site){
                            zero_site.erase(zz);
                            break;
                        }
                    }
                }               
            }
        }
    }
    for(int i = 0; i < 1; i++){
        vector<pair<string,int>> still_big_site;
        map<string, int> site_95_value; //注意了这个值是要动态变化的。站点push之后注意更新
        for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
            string site = it->first; 
            int bw_limit = it->second;
            vector<int> index = argsort(site_t_usage[site]);
            int value_95 = site_t_usage[site][index[position_95]];
            int check_threshold = base_cost + int(sqrt((double)bw_limit));
            site_95_value[site] = value_95;
            if(value_95 > check_threshold){
                still_big_site.push_back(make_pair(site,value_95));
            }
        }
        sort(still_big_site.begin(), still_big_site.end(), cmp);
        for(auto it = still_big_site.rbegin(); it != still_big_site.rend(); it++){
            string b_site = it->first;
            //对于每个这样的站点，只要找到Wj比它小的有共同客户的站点，就能用来分担流量，因为现在是把站点拉到V以上的情况，不能光找V以下的站点
            vector<pair<string,int>> useful_site; //记录可用站点及其公共客户数
            //int accumulated_95_value = site_95_value[b_site];
            
            for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
                pair<string, string> tmp_key = make_pair(b_site, si->first);
                int com_cli_num = site_common_client[tmp_key].size();
                if(com_cli_num != 0 && site_95_value[si->first] <= site_95_value[b_site]){
                    useful_site.push_back(make_pair(si->first,com_cli_num));
                    //accumulated_95_value += site_95_value[si->first];
                }
            }
            
            //可用站点根据共同客户数目多少排序
            sort(useful_site.begin(),useful_site.end(), cmp);
            vector<int> index = argsort(site_t_usage[b_site]);
            for(auto us_iter = useful_site.begin(); us_iter != useful_site.end(); us_iter++){
                string us_site = (*us_iter).first;
                //cout<<"bsite="<<b_site<<" checking us_site="<<us_site<<endl;
                double divide_up = site_bandwidth[us_site]*(site_95_value[b_site]-base_cost) + site_bandwidth[b_site]*(base_cost-site_95_value[us_site]);
                int best_allocate = (int)((double) divide_up /(site_bandwidth[us_site] + site_bandwidth[b_site]));
                bool pushable = true;
                if(best_allocate <= 0){
                    continue; //说明当前这个ts us site不适合分配
                }
                for(int pos = position_95; pos >= 0; pos--){
                    int t = index[pos]; //从大站点95%位开始检查
                    if(site_t_usage[b_site][t] <= base_cost + int(sqrt((double) site_bandwidth[b_site]))){
                        break; //大站点当前时间步已经降下来了，就不用继续分担了
                    }
                    int already_allocate = 0;
                    //if(site_t_usage[us_site][t] >= expected_95_value){
                    //    continue; //当前选中的这个站点超出了期望值，就不需要分担了
                    //}
                    pair<string, string> tmp_key = make_pair(b_site, us_site);
                    vector<string> common_cli_list = site_common_client[tmp_key];

                    //使用while的时间复杂度太高了
                    //每次都取这个共同客户列表中的不同客户，即按照这个列表顺序客户一次取一个流
                    
                    auto circulate_client_iter = common_cli_list.begin();
                    set<string> cannot_pull_client; //这个客户的流量被拉完了没得再拉了
                    //退出循环条件为：没得拉取客户流量了，或者是站点值到达期望值了
                    while(already_allocate < best_allocate && cannot_pull_client.size() != common_cli_list.size() && site_t_usage[b_site][t] > base_cost + int(sqrt((double) site_bandwidth[b_site]))){
                        string operate_client = *circulate_client_iter;
                        vector<pair<string, int>>& stream_list = site_t[b_site][t][operate_client];
                        if(stream_list.size() == 0){
                            cannot_pull_client.insert(operate_client);
                            circulate_client_iter++;
                            if(circulate_client_iter == common_cli_list.end()){
                                circulate_client_iter = common_cli_list.begin();
                            }
                            continue;
                        }
                        
                        //从stream list的头开始遍历，找到一个能移动的就移动
                        bool movable = false;//看看这个client的stream_list有没有能移动的
                        for(auto st_iter = stream_list.begin(); st_iter != stream_list.end(); st_iter++){
                            //cout<<"area3-1"<<endl;
                            if(already_allocate >= best_allocate || site_t_usage[b_site][t] <= base_cost + int(sqrt((double) site_bandwidth[b_site]))){
                                break;
                            }
                            string stream_type = (*st_iter).first;
                            int stream_bw = (*st_iter).second;
                            
                            //满足条件则移动
                            if(stream_bw + already_allocate <= best_allocate + base_cost && stream_bw + site_t_usage[us_site][t] < site_bandwidth[us_site]){
                                pair<string, int> move_flow = make_pair(stream_type, stream_bw);
                                site_t[us_site][t][operate_client].push_back(move_flow);
                                site_t_usage[us_site][t] += move_flow.second;
                                st_iter = stream_list.erase(st_iter);
                                site_t_usage[b_site][t] -= move_flow.second;
                                movable = true;
                                already_allocate += move_flow.second;
                                break; //移动一个就可用跳出了，不用再遍历stream list
                            }
                            
                        }
                        if(movable == false){
                            cannot_pull_client.insert(operate_client);
                        }
                        circulate_client_iter++;
                        if(circulate_client_iter == common_cli_list.end()){
                            circulate_client_iter = common_cli_list.begin();
                        }                        
                    }                    

                    if(pos == position_95 && already_allocate == 0){
                        //说明最大的这个时间步推不动，那就换一个站点去推
                        pushable = false;
                        break;
                    }
                }
                if(pushable == false){
                    continue;
                }
                //更新95%位值
                vector<int> idx_tmp = argsort(site_t_usage[us_site]);
                site_95_value[us_site] = site_t_usage[us_site][idx_tmp[position_95]];
                idx_tmp = argsort(site_t_usage[b_site]);
                site_95_value[b_site] = site_t_usage[b_site][idx_tmp[position_95]];
                //cout<<"move from "<<b_site<<" to "<<us_site<<endl;
                break;
            }            
            //push完之后更新每个站点95%位置信息           
        }
    }
    cout<<"finish Vmore push"<<endl;
    

    //pull
    for(int i = 0; i < 1; i++){
        //pull
        set<string> site_processed;
        //95%小于V的全部给它拉到V
        //排序策略是先考虑95%位值,再考虑对应客户数目多少
        vector<pair<string,pair<int,int>>> site_order_95_and_client;
        for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
            string site = it->first;
            vector<int> index = argsort(site_t_usage[site]);
            int value_95 = site_t_usage[site][index[position_95]];
            if(client4site.find(site) != client4site.end()){
                site_order_95_and_client.push_back(make_pair(it->first, make_pair(value_95, client4site[site].size())));
            }
            else{
                site_order_95_and_client.push_back(make_pair(it->first, make_pair(value_95,0)));
            }
        }
        sort(site_order_95_and_client.begin(), site_order_95_and_client.end(), cmp2);

        for(auto si = site_order_95_and_client.rbegin(); si != site_order_95_and_client.rend(); si++){
            string site = si->first;
            site_processed.insert(site);
            vector<int> index = argsort(site_t_usage[site]);
            int value_95 = site_t_usage[site][index[position_95]];
            if (value_95 < base_cost){
                value_95 = base_cost;
            }
            for(int position = 0; position < int(index.size()); position++){
                int t = index[position];
                int left = 0;
                if(position <= position_95){
                    left = value_95 - site_t_usage[site][t];
                }
                else{
                    left = site_bandwidth[site] - site_t_usage[site][t];
                }
                if(left <= 0){
                    continue;
                }
                for(auto client = client4site[site].begin(); client != client4site[site].end(); client++){
                    for(auto site_client = site4client[*client].begin(); site_client != site4client[*client].end(); site_client++){
                        if(left <= 0){
                            break;
                        }
                        if(site_processed.find(*site_client) == site_processed.end() && site_t[*site_client][t].find(*client) != site_t[*site_client][t].end()){
                            //确定move flow
                            //这里用引用,即site_client_stream_list 指向的就是 site_t[*site_client][t][*client]所指的地方,改前者等于改后者
                            vector<pair<string, int>>& site_client_stream_list = site_t[*site_client][t][*client];
                            sort(site_client_stream_list.begin(), site_client_stream_list.end(), cmp); //对这个vector排序一下,流类型从大到小
                            //在left允许的情况下,移动尽可能多的流到这边
                            //使用iterator,便于删除,这里使用手动stream_iter++,而不是自动加
                            for(auto stream_iter = site_client_stream_list.begin(); stream_iter != site_client_stream_list.end();){
                                string stream_type = (*stream_iter).first;
                                int stream_bw = (*stream_iter).second;
                                //注意只能一整个一整个地移动,小于等于就移动它
                                if(stream_bw <= left){
                                    pair<string, int> move_flow = make_pair(stream_type,stream_bw);
                                    site_t[site][t][*client].push_back(move_flow);
                                    site_t_usage[site][t] += move_flow.second;
                                    stream_iter = site_client_stream_list.erase(stream_iter);
                                    site_t_usage[*site_client][t] -= move_flow.second;
                                    left -= move_flow.second;
                                }
                                else{
                                    stream_iter++;
                                }
                            }
                            
                        }
                    }
                }
            }

            
        }
        }
    }

    /****************** 输出结果 ******************/
    ofstream solution;
    solution.open(output_path);
    if(!solution.is_open()){
        std:cerr<< "failed to open "<< output_path;
    }
    int line_count = 0;
    for(int t = 0; t < timestamps; t++){
        for(auto cl = demand.begin(); cl != demand.end(); cl++){
            string client = cl->first;
            solution << client << ":";
            ++line_count;
            int check_all_zero = 1; //假设全0
            for(auto de = demand[client][t].begin(); de != demand[client][t].end(); de++){
                if((*de).second != 0){
                    check_all_zero = 0; //找到那么一个不为0的需求
                    break;
                }
            }
            if(check_all_zero == 1){ //真的需求全0
                solution << endl;
                continue;
            }
            int count = 0;
            int writed_site_count = 0;
            for(auto si = site4client[client].begin(); si != site4client[client].end(); si++){
                ++count;
                string site = *si;
                vector<pair<string,int>> allocate_situation;
                if(site_t[site][t].find(client) != site_t[site][t].end()){
                    allocate_situation = site_t[site][t][client];
                }
                if(allocate_situation.size() != 0){
                    if(writed_site_count == 0)
                        solution<<"<"<<site;
                    else
                        solution<<",<"<<site;
                    writed_site_count++;
                    for(int i = 0; i < allocate_situation.size(); i++){
                        solution<<","<<allocate_situation[i].first;
                    }
                    solution<<">";
                }
            }
            //判断是不是最后一个时间步的最后一行,若是,则不换行,否则换行
            if(count == site4client[client].size() && line_count != timestamps * client_order.size()){
                solution<<endl;
            }
        }
    }
    solution.close();

    return 0;
}