//modified from sorted initial puls V greedy
#include "helpers.hpp"

using namespace std;
// 初始解，对于每个时间步，对于每个客户(客户是有序的)，顺序遍历它的流，(增加对服务器的排序,从对应客户数目少的开始)找到一个能用的服务器就分上去。
// 后续使用的是push+pull
// 对于push，找出那些大于V的站点，找出一个或多个站点分担其流量，用尽可能少的站点。（和它有较多公共客户且全0的站点，且每个站点要在多个时间步分担其流量）
// 对于pull，尽量拉满95%,没到V的95%拉到V,不仅仅是95%
// 在score=13xxxxx的基础上加了push
// score = ?



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
    /*
    if(CHECK_DATA){
        ofstream check_file;
        check_file.open("../output/check.txt");
        for(auto it = result.begin(); it != result.end(); it++){
            check_file<<"site "<<(it->first).first<<" site "<<(it->first).second<<" common client list:";
            for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
                check_file<<(*iter)<<" ";
            }
            check_file<<endl;
        }
        check_file.close();
    }
    */
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

        for(auto co = client_order.begin(); co != client_order.end(); co++){
            string client = co->first;
            //这里actual_site使用排序,正序,倒序都可以试一下,练习赛中目前是si=actual_site.begin()开始比较好
            vector<string> actual_site = site_order_for_client[client];
            //这里暂且使用擦除vector中元素的做法,如果出错,可替换成加一个flag进行判断,去计算vector中每个元素是否为0
            while(client_remaining[client].size() > 0){
                auto remain_iter = client_remaining[client].begin();
                string stream_type = (*remain_iter).first;
                int stream_bw = (*remain_iter).second;
                int assigned_success = 0; //debug用,看看有没有不成功的assign
                //按站点对应客户从多到少的顺序来,这样得分更高
                for(auto si = actual_site.begin(); si != actual_site.end(); si++){
                    string site = *si;
                    if(client_remaining[client].size() == 0){
                        break;
                    }
                    //找到一个能用的site
                    if(site_remaining[site] >= stream_bw){
                        assigned_success = 1;
                        client_remaining[client].erase(remain_iter);
                        site_remaining[site] -= stream_bw;
                        
                        if(site_t_client.find(site) != site_t_client.end() && site_t_client[site].find(client) != site_t_client[site].end()){
                            site_t_client[site][client].push_back(make_pair(stream_type,stream_bw));
                        }
                        else{
                            vector<pair<string,int>> tmp;
                            tmp.push_back(make_pair(stream_type,stream_bw));
                            site_t_client[site][client] = tmp;
                        }
                        break;
                    }
                }
                if(assigned_success == 0){
                    cout<<"ERROR! CAN NOT ASSIGN STREAM!"<<endl;
                }
            }
            for(auto si = site4client[client].begin(); si != site4client[client].end(); si++){
                string site = *si;
                int assigned_bw = site_current_bw[site] - site_remaining[site];
                site_current_bw[site] = site_remaining[site];
                if(site_current_ts_usage.find(site) != site_current_ts_usage.end()){
                    site_current_ts_usage[site] += assigned_bw;
                }
                else{
                    site_current_ts_usage[site] = assigned_bw;
                }
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

    //检查site_t的长度够不够,不确定是否有漏掉的情况,检查通过,应该ok
    if(DEBUG){
        for(auto si = site_bandwidth.begin(); si != site_bandwidth.end(); si++){
            if(site_t[si->first].size() != timestamps){
                cout<<"LENGTH ERROR!"<<endl;
            }
            if(site_t_usage[si->first].size() != timestamps){
                cout<<"LENGTH ERROR! USAGE."<<endl;
            }
        }
    }

    
    //贪心
    if(CHECK_CLOCK)
        start = clock();
    int position_95 = int(ceil(timestamps * 0.95) -1);
    //push
    //首先找到那些超过V比较多的站点，
    //这个for循环里面声明的都是这个for中的局部变量
    //遍历所有站点的95位值，找出那些95位值大和95位之后全0的站点
    
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
    for(int i = 0; i < 2; i++){
        

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
    if(CHECK_CLOCK){
        endtime = clock();
        cout<<"greedy time:"<<(double)(endtime-start)/CLOCKS_PER_SEC<<endl;
    }



    //输出
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
            line_count++;
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
                count++;
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