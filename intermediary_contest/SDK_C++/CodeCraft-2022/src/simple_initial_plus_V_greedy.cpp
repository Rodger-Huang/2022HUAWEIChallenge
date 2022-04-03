#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<map>
#include<vector>
#include<algorithm>
#include<cmath>
#include<typeinfo>
#include<set>
#define CHECK_DATA 0
#define DEBUG 0

using namespace std;
// 分配方案使用最简单的，对于每个时间步，对于每个客户，顺序遍历它的流，找到一个能用的服务器就分上去。
// 后续使用的是初赛时的贪心策略改版,尽量拉满95%,没到V的95%拉到V,不仅仅是95%
// score = 1726487
// 测试用
const string input_path = "intermediary_contest/data/";
const string output_path = "intermediary_contest/output/solution.txt";

//测试用
//const string input_path = "../../../../data/";
//const string output_path = "../../../../output/solution.txt";

// 提交用
// const string input_path = "/data/";
// const string output_path = "/output/solution.txt";

int timestamps = 0;
int site_number = 0;
int client_number = 0;

map<string, int> getSiteBandwidth(){
    map<string, int> result;
    string target_file = input_path + "site_bandwidth.csv";
    ifstream fin(target_file);
    if(!fin.is_open()){
        std::cerr<<"failed to open "<<input_path<< "site_bandwidth.csv.";
    }
    string header;
    getline(fin, header);
    string site_bandwidth_info;
    int bandwidth;
    string site_name;
    string bandwidth_str;
    while(getline(fin, site_bandwidth_info)){
        site_number++;
        istringstream readline(site_bandwidth_info);
        getline(readline,site_name,',');
        getline(readline,bandwidth_str,',');
        bandwidth = atoi(bandwidth_str.c_str());
        if(CHECK_DATA){
            cout<<"site name:"<<site_name<<" bandwidth:"<<bandwidth<<endl;
        }
        result.insert(make_pair(site_name,bandwidth));
    }
    fin.close();
    return result;
}

//增加了找到就break，另外数据中的config.ini中的qos_constraint是有下划线的但复赛任务书中没有,也许是任务书漏掉了,这里找的还是有下划线的qos
int getConstraint(){
    ifstream fconfig;
    string target_file = input_path + "config.ini";
    fconfig.open(target_file);
    if(!fconfig.is_open()){
        std::cerr<<"failed to open "<<input_path<< "config.ini";
    }
    string find_qos_line;
    string qos_constraint_str;
    while(getline(fconfig,find_qos_line)){
        if(find_qos_line.find("qos_constraint=") != std::string::npos){
            qos_constraint_str = find_qos_line.substr(find_qos_line.find("=")+1);
            if(CHECK_DATA)
                cout<<"check qos constraint string:"<<qos_constraint_str<<endl;
            break;
        }
    }
    int qos_constraint = 0;
    qos_constraint = atoi(qos_constraint_str.c_str());
    fconfig.close();
    if(CHECK_DATA)
        cout<<"qos constraint(int):"<<qos_constraint<<endl;
    return qos_constraint;
}
//复赛任务书增加了base_cost
int getBaseCost(){
    ifstream fconfig;
    string target_file = input_path + "config.ini";
    fconfig.open(target_file);
    if(!fconfig.is_open()){
        std:cerr << "failed to open "<< input_path << "config.ini";
    }
    string find_base_cost_line;
    string base_cost_str;
    while(getline(fconfig, find_base_cost_line)){
        if(find_base_cost_line.find("base_cost=") != std::string::npos){
            base_cost_str = find_base_cost_line.substr(find_base_cost_line.find("=")+1);
            if(CHECK_DATA)
                cout<<"check base cost string:"<<base_cost_str<<endl;
            break;
        }
    }
    int base_cost = 0;
    base_cost = atoi(base_cost_str.c_str());
    fconfig.close();
    if(CHECK_DATA)
        cout<<"base costs(int):"<<base_cost<<endl;
    return base_cost;

}
//修改了demand存储方式,本来一个客户一个时刻只有一个总的需求,现在变为一个客户一个时刻有多个拆分的需求(不同种类的流)
//map为客户名字到其需求的映射,第一个vector索引的是不同时刻,第二个vector索引的是客户某个时刻的若干种流的需求,pair表示流的种类与其大小的对应关系
//示例:要寻找客户C在第2个时刻的所有流的需求组成的vector:  vector<pair<string,int>> client_current_t_demand_list = result['C'][2]
//再去遍历这个vector里面的每一个pair寻找这个客户当前时刻的每种流的需求
map<string, vector<vector<pair<string,int>>>> getDemand(){
    map<string, vector<vector<pair<string,int>>>> result;
    string target_file = input_path + "demand.csv";
    ifstream fdemand;
    fdemand.open(target_file);
    if(!fdemand.is_open()){
        std:cerr<<"failed to open "<<target_file;
    }
    vector<string> client_name_list;
    string header;
    getline(fdemand,header);

    //下列三行照抄原来的,应该是linux系统的,不知道这里还要不要改
    size_t n = header.find_last_not_of("\r\n\t");
    if(n != string::npos){
        header.erase(n+1, header.size() -n);
    }
    istringstream readheader(header);
    string client_name;
    getline(readheader,client_name,','); //读掉mtime
    getline(readheader,client_name,','); //读掉stream id
    while(getline(readheader, client_name, ',')){
        client_number++;
        client_name_list.push_back(client_name);
        vector<vector<pair<string,int>>> ts_demand;
        result.insert(make_pair(client_name,ts_demand));
    }
    string time_str = "";
    //注意了这里需要处理标识时间的字符串,进行判断是否同一个时刻
    string demand_line;
    while(getline(fdemand,demand_line)){
        istringstream demand_data(demand_line);
        string stream_type;
        string detail_demand;
        string current_time_str;
        getline(demand_data, current_time_str, ','); //读掉时间
        getline(demand_data,stream_type,','); //读流类型
        if(current_time_str != time_str){ //时间步跳转
            timestamps++;
            time_str = current_time_str;
            int i = 0;
            while(getline(demand_data, detail_demand, ',')){
                vector<pair<string,int>> current_ts_demand_list;
                current_ts_demand_list.push_back(make_pair(stream_type, atoi(detail_demand.c_str())));
                result[client_name_list[i++]].push_back(current_ts_demand_list);
            }
        }
        else{
            int i = 0;
            while(getline(demand_data, detail_demand, ',')){
                //由于timestamp记录的是真实的时间步数,而vector是索引从0开始的,注意索引减一
                result[client_name_list[i++]][timestamps-1].push_back(make_pair(stream_type, atoi(detail_demand.c_str())));
            }
        }
    }
    fdemand.close();
    if(CHECK_DATA){
        for(auto it = result.begin(); it != result.end(); it++){
            cout<<"client:"<<it->first;
            for(int i = 0; i < it->second.size(); i++){
                if(i < 5)
                    cout<<"timestamp = "<<i<<" (count from 0)"<<endl;
                vector<pair<string, int>> tmp = it->second[i];
                for(int j = 0; j < tmp.size(); j++){
                    if(i < 5)
                        cout<<"stream type = "<<tmp[j].first<<" demand = "<<tmp[j].second<< " ";
                }
            }
            cout<<endl;
        }
    }
    return result;
}

map<pair<string,string>, int> getQoS(){
    map<pair<string,string>, int> result;
    ifstream fQoS;
    string target_file = input_path + "qos.csv";
    fQoS.open(target_file);
    if(!fQoS.is_open()){
        std:cerr<<"failed to open "<<input_path<<" qos.csv";
    }
    vector<string> client_name_list;
    string header;
    getline(fQoS, header);
    size_t n = header.find_last_not_of("\r\n\t");
    if(n != string::npos){
        header.erase(n+1, header.size() - n);
    }
    istringstream readHeader(header);
    string client_name;
    getline(readHeader,client_name,',');
    while(getline(readHeader,client_name,',')){
        client_name_list.push_back(client_name);
    }
    string qos_line;
    while(getline(fQoS,qos_line)){
        istringstream qos_data(qos_line);
        string site_name;
        getline(qos_data,site_name,',');
        string qos_detail;
        int i = 0;
        while(getline(qos_data,qos_detail,',')){
            result.insert(make_pair(make_pair(site_name,client_name_list[i++]),atoi(qos_detail.c_str())));
        }
    }
    fQoS.close();
    if(0){
        cout<<"check qos data"<<endl;
        for(auto it = result.begin(); it != result.end(); it++){
            cout<<"site "<< (it->first).first <<" client "<<(it->first).second <<" qos"<<it->second<<endl;
        }
    }
    return result;
}

// client->[site1,site2,...]
map<string, vector<string>> getSiteForClient(){
    map<string, vector<string>> result;
    map<pair<string,string>, int> qos = getQoS();
    int qos_constraint = getConstraint();
    for(auto it = qos.begin(); it != qos.end(); it ++){
        if(it->second >= qos_constraint)
            continue;
        auto got = result.find((it->first).second);
        if(got != result.end()){
            got->second.push_back((it->first).first);
        }
        else{
            vector<string> tmp;
            tmp.push_back((it->first).first);
            result.insert(make_pair((it->first).second,tmp));
        }
    }
    if(CHECK_DATA){
        cout<<"check site for client "<<endl;
        for(auto it = result.begin(); it != result.end(); it++){
            cout<<"client "<<it->first<<" qos sites: ";
            for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
                cout<<*(iter)<<" ";
            }
            cout<<endl;
        }
    }
    return result;
}

map<string, vector<string>> getClientForSite(){
    map<string, vector<string>> result;
    map<pair<string,string>, int> qos = getQoS();
    int qos_constraint = getConstraint();
        for(auto it = qos.begin(); it != qos.end(); it ++){
        if(it->second >= qos_constraint)
            continue;
        auto got = result.find((it->first).first);
        if(got != result.end()){
            got->second.push_back((it->first).second);
        }
        else{
            vector<string> tmp;
            tmp.push_back((it->first).second);
            result.insert(make_pair((it->first).first,tmp));
        }
    }
    if(CHECK_DATA){
        cout<<"check site for client "<<endl;
        for(auto it = result.begin(); it != result.end(); it++){
            cout<<"site "<<it->first<<" qos client: ";
            for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
                cout<<*(iter)<<" ";
            }
            cout<<endl;
        }
    }
    return result;
}

bool cmp(pair<string, int> a, pair<string, int> b){
    return a.second > b.second;
}

bool cmp2(pair<string, pair<int, int>> a, pair<string, pair<int, int>> b){
    //优先比较后面pair中的第一个元素
    if (a.second.first > b.second.first){
        return true;
    }
    else if(a.second.first < b.second.first){
        return false;
    }
    else{
        //后面pair中第一个元素相等的情况下，比较pair的第二个元素
        return a.second.second > b.second.second;
    }

}
// 实现argsort功能
template<typename T> vector<int> argsort(const vector<T>& array)
{
	const int array_len(array.size());
	vector<int> array_index(array_len, 0);
	for (int i = 0; i < array_len; ++i)
		array_index[i] = i;

	sort(array_index.begin(), array_index.end(),
		[&array](int pos1, int pos2) {return (array[pos1] < array[pos2]);});

	return array_index;
}
//-------------------全局变量------------------
int qos_constraint = getConstraint();
int base_cost = getBaseCost();
map<string, int> site_bandwidth = getSiteBandwidth();
map<string, vector<vector<pair<string,int>>>> demand = getDemand();

map<string, vector<string>> site4client = getSiteForClient();
map<string, vector<string>> client4site = getClientForSite();

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
//注意这个函数要在调用了operateOnGloabal之后调用,才能得到正确的变量值
map<string, vector<string>> getSiteOrderForClient(){
    map<string, vector<string>> result;
    for(auto it = site4client.begin(); it != site4client.end(); it++){
        vector<string> this_client_site_order;
        for(int i = 0; i < site_order.size(); i++){
        }
    }
    return result;
}
int main(){
    operateOnGlobalVariable();

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
            //这里actual_site应当排序一下,对应客户数目少的优先, 待修改!!!!排序
            vector<string> actual_site = site4client[client];
            //这里暂且使用擦除vector中元素的做法,如果出错,可替换成加一个flag进行判断,去计算vector中每个元素是否为0
            while(client_remaining[client].size() > 0){
                auto remain_iter = client_remaining[client].begin();
                string stream_type = (*remain_iter).first;
                int stream_bw = (*remain_iter).second;
                int assigned_success = 0; //debug用,看看有没有不成功的assign
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
    int position_95 = int(ceil(timestamps * 0.95) -1);
    for(int i = 0; i < 2; i++){
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