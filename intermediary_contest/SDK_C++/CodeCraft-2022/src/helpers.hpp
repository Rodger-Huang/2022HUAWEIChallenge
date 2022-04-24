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
#include<cstdlib>
#include<ctime>
#define CHECK_DATA 0
#define CHECK_CLOCK 1
#define DEBUG 0

using namespace std;
int timestamps = 0;
int site_number = 0;
int client_number = 0;
clock_t start, endtime;

// 测试用
const string input_path = "intermediary_contest/data/";
const string output_path = "intermediary_contest/output/solution.txt";

//测试用
// const string input_path = "../data/";
// const string output_path = "../output/solution.txt";

// 提交用
//const string input_path = "/data/";
//const string output_path = "/output/solution.txt";

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
    if(CHECK_DATA == 2){
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
map<string, vector<string>> getSiteForClient(map<pair<string,string>, int>& qos, int qos_constraint){
    map<string, vector<string>> result;
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

map<string, vector<string>> getClientForSite(map<pair<string,string>, int>& qos, int qos_constraint){
    map<string, vector<string>> result;
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
map<string, set<string>> getSetSiteForClient(map<pair<string,string>, int>& qos, int& qos_constraint){
    map<string, set<string>> result;
    for(auto it = qos.begin(); it != qos.end(); it ++){
        if(it->second >= qos_constraint)
            continue;
        auto got = result.find((it->first).second);
        if(got != result.end()){
            got->second.insert((it->first).first);
        }
        else{
            set<string> tmp;
            tmp.insert((it->first).first);
            result.insert(make_pair((it->first).second,tmp));
        }
    }
    if(CHECK_DATA){
        cout<<"check set site for client "<<endl;
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
map<string, set<string>> getSetClientForSite(map<pair<string,string>, int>& qos, int& qos_constraint){
    map<string, set<string>> result;
    for(auto it = qos.begin(); it != qos.end(); it ++){
        if(it->second >= qos_constraint)
            continue;
        auto got = result.find((it->first).first);
        if(got != result.end()){
            got->second.insert((it->first).second);
        }
        else{
            set<string> tmp;
            tmp.insert((it->first).second);
            result.insert(make_pair((it->first).first,tmp));
        }
    }
    if(CHECK_DATA){
        cout<<"check set site for client "<<endl;
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
//先不管这个，后续看看是不是有用。
map<string, int> getSiteThreshold(map<string,int> &site_bandwidth, int base_cost){
    map<string, int> result;
    double threshold;
    for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
        string site = it->first;
        int site_bw = it->second;
        double delta = pow((double)site_bw,2) + 4 * base_cost * site_bw;
        double sqrt_delta = sqrt(delta);
        threshold = 1/2 * (2 * base_cost - site_bw + sqrt_delta);
        int thred_int = int(threshold);
        result.insert(make_pair(site,thred_int));
        if(1){
            cout<<"site = "<<site<<" threshold = "<<thred_int;
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