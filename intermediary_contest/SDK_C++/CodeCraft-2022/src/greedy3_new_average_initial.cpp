#include "helpers2.hpp"
using namespace std;
// 这个贪心是讨论后的实现方案
// 先从对应客户站点数目少的站点开始，计算所有时间步可能服务的流量大小，挑出最大的5%
// 在考虑下一个站点的时候用的是更新后的流量情况

//-------------------全局变量------------------
int qos_constraint = getConstraint();
int base_cost = getBaseCost();
map<pair<string,string>, int> qos = getQoS();
map<string, int> site_bandwidth = getSiteBandwidth();
map<string, vector<map<string,int>>> demand = getDemand();

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
        //site4client里面是必定有client的key的
        client_site_number.insert(make_pair(it->first, it->second.size()));
        client_order.push_back(make_pair(it->first, it->second.size()));
    }
    sort(client_order.begin(), client_order.end(), cmp);
    // 对节点根据客户数量从多到少进行排序，少客户的节点有更大概率95%值比较小
    
    for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
        string site = it->first;
        // 因为有可能client4site里面没有site的key所以要判断
        if(client4site.find(site) != client4site.end()){
            site_order.push_back(make_pair(it->first, client4site[site].size()));
        }else{
            site_order.push_back(make_pair(it->first, 0));
        }
    }
    //cmp返回的是降序，从大到小
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

//----------------------全局变量---------------------
int main(){
    unsigned seed = 1;
    operateOnGlobalVariable();
    map<string, vector<string>> site_order_for_client = getSiteOrderForClient();
    map<pair<string,string>, vector<string>> site_common_client = getSiteCommonClient();
    //map为site到一个长度为timestamp的vector的映射,记录site每个时间步使用情况
    //对于上述vector中的每个元素,是一个客户名字到其分配到这个站点的种类的流量的情况
    //将原来能用int表示的单个客户需求,用vector<pair<string,int>>表示
    map<string, vector<map<string,vector<pair<string,int>>>>> site_t;
    map<string, vector<int>> site_t_usage;    

    //由于贪心不是顺序按照时间步分配的，使用这两个记录不同时间步的情况，贪心和平均结束后再转换到site_t和site_t_usage
    //时间步映射到站点服务的客户流量情况
    map<int, map<string, map<string, vector<pair<string, int>>>>> ts_site_t;
    //时间步映射到站点的带宽使用情况
    map<int, map<string, int>> ts_site_t_usage;
    

    //由于先贪心，后再平均分，这些变量用于记录所有时间步的实际站点剩余情况，站点当前带宽情况，客户剩余请求情况
    //记为状态变量
    map<int, map<string, int>> ts_site_remaining; //当前剩余多少，分配一个流修改一次
    //当前剩余多少，完成整个时间步分配之后才修改，实际上暂时没有用到这个，因为在每次流分配的时候顺手改了site_usage,就不借助这个改了
    //map<int, map<string, int>> ts_site_current_bw; 
    map<int, map<string, map<string, int>>> ts_client_remaining;

    start = clock();
    st = clock();
    //状态变量初始化
    for(int t = 0; t < timestamps; t++){
        ts_site_remaining[t] = site_bandwidth;
        //ts_site_current_bw[t] = site_bandwidth;
        for(auto it = demand.begin(); it != demand.end(); it++){
            ts_client_remaining[t][it->first] = it->second[t];
        }
        for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
            ts_site_t_usage[t][si->first] = 0;
        }
    }

    //-----------------贪心-------------------------
    // 按照站点对应客户数目从小到大的顺序来
    for(auto so = site_order.rbegin(); so != site_order.rend(); so++){
        string site = so->first;
        vector<int> site_possible_serve; //这个站点每个时间步可能服务的流量大小
        //计算当前站点所有时间步最大可能服务流量大小
        for(int t = 0; t < timestamps; t++){
            int current_ts_possible_serve = 0;
            //对于它每个能服务的客户计算客户剩余多少流的需求
            for(auto cl = client4site[site].begin(); cl != client4site[site].end(); cl++){
                for(auto st_iter = ts_client_remaining[t][*cl].begin(); st_iter != ts_client_remaining[t][*cl].end(); st_iter++){
                    current_ts_possible_serve += st_iter->second;
                }
            }
            site_possible_serve.push_back(current_ts_possible_serve);
        }
        //给这个站点所有时间步可能服务流量大小排序
        vector<int> index = argsort(site_possible_serve);
        int position_95 = int(ceil(timestamps * 0.95) - 1);
        // 分配最大的5%所在的时间步
        for(int pos = timestamps - 1; pos > position_95; pos--){
            // 索引找出时间步
            int process_ts = index[pos];
            //分配，这里按照所有客户的流大小顺序，从大到小
            vector<UnrollClientDemand> all_client_demand;

            for(auto cl = client4site[site].begin(); cl != client4site[site].end(); cl++){
                string client_name = *cl;
                for(auto st_iter = ts_client_remaining[process_ts][*cl].begin(); st_iter != ts_client_remaining[process_ts][*cl].end(); st_iter++){
                    string stream_type = st_iter->first;
                    int stream_bw = st_iter->second;
                    UnrollClientDemand current_stream_demand = {client_name, stream_type, stream_bw, process_ts};
                    all_client_demand.push_back(current_stream_demand);
                }
            }

            sort(all_client_demand.begin(), all_client_demand.end(), compareUnrollClientDemand);
           
            for(auto d_iter = all_client_demand.begin(); d_iter != all_client_demand.end(); d_iter++){
                if(ts_site_remaining[process_ts][site] == 0)
                    break;
                string stream_type = d_iter->stream_type;
                int stream_bw = d_iter->stream_bw;
                string client_name = d_iter->client_name;
                if(stream_bw <= ts_site_remaining[process_ts][site]){
                    ts_client_remaining[process_ts][client_name].erase(stream_type);
                    ts_site_remaining[process_ts][site] -= stream_bw;
                    ts_site_t[process_ts][site][client_name].push_back(make_pair(stream_type,stream_bw));
                    ts_site_t_usage[process_ts][site] += stream_bw;
                }
            }
        }
    }
    ed = clock();
    cout<<"greedy ok, time = "<<(double)(ed-st)/CLOCKS_PER_SEC<<endl;

    //---------------贪心分配结束--------------------

    //--------------平均分-----------------
    for(int t = 0; t < timestamps; t++){
        for(auto co = client_order.begin(); co != client_order.end(); co++){
            string client = co->first;
            vector<string> actual_site = site_order_for_client[client];
            //shuffle(actual_site.begin(), actual_site.end(), std::default_random_engine(seed));
            auto si = actual_site.begin();
            auto remain_iter = ts_client_remaining[t][client].begin();
            string stream_type = remain_iter->first;
            int stream_bw = remain_iter->second;
            for(auto remain_iter = ts_client_remaining[t][client].begin(); remain_iter != ts_client_remaining[t][client].end();){
                //对于每个流，都遍历一遍服务器，找一个最“合适”的服务器
                stream_type = remain_iter->first;
                stream_bw = remain_iter->second;
                
                string suitable_site = "";
                int site_left = 1410065407; //找剩余流量最少的服务器
                //int site_left = -1; //找剩余流量最多的服务器
                //int site_already_in = 1410065407;  //现在找的是一个site当前已使用流量最少的服务器
                for(auto sit = actual_site.begin(); sit != actual_site.end(); sit++){
                    if(ts_site_remaining[t][*sit] >= stream_bw && ts_site_remaining[t][*sit] < site_left){ //剩余流量最少的
                    //if(ts_site_remaining[t][*sit] >= stream_bw && ts_site_remaining[t][*sit] > site_left){ //剩余流量最多的
                    //if(ts_site_remaining[t][*sit] >= stream_bw && ts_site_t_usage[t][*sit] < site_already_in){ //已用流量最少的
                        suitable_site = *sit;
                        //site_already_in = ts_site_t_usage[t][*sit];
                        site_left = ts_site_remaining[t][*sit];
                    }
                }
                if(suitable_site == ""){
                    cout<<"ERROR"<<endl;
                }
                else{
                    remain_iter = ts_client_remaining[t][client].erase(remain_iter);
                    ts_site_remaining[t][suitable_site] -= stream_bw;
                    ts_site_t[t][suitable_site][client].push_back(make_pair(stream_type,stream_bw));
                    ts_site_t_usage[t][suitable_site] += stream_bw;
                }
            }
            /*
            while(remain_iter != ts_client_remaining[t][client].end()){
                string site = *si;
                if(ts_site_remaining[t][site] >= stream_bw){
                    remain_iter = ts_client_remaining[t][client].erase(remain_iter);
                    ts_site_remaining[t][site] -= stream_bw;
                    ts_site_t[t][site][client].push_back(make_pair(stream_type,stream_bw));
                    ts_site_t_usage[t][site] += stream_bw;
                    
                    if(remain_iter != ts_client_remaining[t][client].end()){
                        stream_type = remain_iter->first;
                        stream_bw = remain_iter->second;
                    }
                }
                si++;
                if(si == actual_site.end()){
                    si = actual_site.begin();
                }
            }
            */
        }

    }
    //------------平均分结束---------------
    

    // 转换变量
    for(int t = 0; t < timestamps; t++){
        map<string, map<string, vector<pair<string, int>>>> current_ts_site_t = ts_site_t[t];
        map<string, int> current_ts_site_usage = ts_site_t_usage[t];
        for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
            string site = si->first;
            if(current_ts_site_t.find(site) == current_ts_site_t.end()){
                map<string,vector<pair<string, int>>> empty;
                site_t[site].push_back(empty);
            }
            else{
                site_t[site].push_back(current_ts_site_t[site]);
            }
            if(current_ts_site_usage.find(site) == current_ts_site_usage.end()){
                site_t_usage[site].push_back(0);
            }
            else{
                site_t_usage[site].push_back(current_ts_site_usage[site]);
            }
        }
    }

    endtime = clock();
    cout<<"initial use time = "<<(double)(endtime - start)/CLOCKS_PER_SEC<<endl;
    
    

    //-------------------贪心push&pull------------------
    int position_95 = int(ceil(timestamps * 0.95) - 1);
    for(int kk = 0; kk < 1; kk++){
        //push要改，对每个95%位置大于V的站点，（从大到小），给他push出去
        // push的时候找站点原则：看看95%位值比它小的站点，如果说，别人没到V就给它push到V，
        // push顺序：从95%位值较小的开始push，注意push的话不能把别人push到比自己还高
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