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

using namespace std;

// 测试用
// const string input_path = "/home/hadoop/2022HUAWEIChallenge/SDK/data/";
// const string output_path = "/home/hadoop/2022HUAWEIChallenge/SDK/output/solution.txt";

const string input_path = "/home/hadoop/2022HUAWEIChallenge/SDK/simulated_data/";
const string output_path = "/home/hadoop/2022HUAWEIChallenge/SDK/output/solution.txt";

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
        }
    }
    int qos_constraint = 0;
    qos_constraint = atoi(qos_constraint_str.c_str());
    fconfig.close();
    if(CHECK_DATA)
        cout<<"qos constraint(int):"<<qos_constraint<<endl;
    return qos_constraint;
}

map<string, vector<int>> getDemand(){
    map<string, vector<int>> result;
    ifstream fdemand;
    string target_file = input_path + "demand.csv";
    fdemand.open(target_file);
    if(!fdemand.is_open()){
        std:cerr<<"failed to open "<<input_path<<"demand.csv.";
    }
    vector<string> client_name_list;
    string header;
    getline(fdemand, header);
    header.pop_back();
    istringstream readheader(header);
    string client_name;
    getline(readheader, client_name, ',');
    while(getline(readheader, client_name,',')){
        client_number++;
        client_name_list.push_back(client_name);
        vector<int> ts_demand;
        result.insert(make_pair(client_name, ts_demand));
    }
    string demand_line;
    while(getline(fdemand, demand_line)){
        timestamps++; //这里修改全局变量timestamp
        istringstream demand_data(demand_line);
        string detail_demand;
        //读掉时间
        getline(demand_data,detail_demand, ',');
        int i = 0;
        while(getline(demand_data, detail_demand, ',')){
            result[client_name_list[i++]].push_back(atoi(detail_demand.c_str()));
        }
    }
    fdemand.close();
    if(CHECK_DATA){
        for (auto it = result.begin(); it != result.end(); it++){
            cout<<"client:"<<it->first;
            for(auto iter = it->second.begin(); iter != it->second.end(); iter++){
                cout<<" "<<(*iter)<<" ";
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
    header.pop_back();
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

int main(){
    int qos_constraint = getConstraint();
    map<string, int> site_bandwidth = getSiteBandwidth();
    map<string, vector<int>> demand = getDemand();
    
    map<string, vector<string>> site4client = getSiteForClient();
    map<string, vector<string>> client4site = getClientForSite();

    map<string, int> client_site_number;
    map<string, int> site_client_number;

    // sort client by site number
    vector<pair<string, int>> client_order;
    vector<pair<string, int>> site_order;

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

    //注意：可以通过，demand[client][t]获取客户在t时刻的总需求
    //通过 site_bandwidth[site]获取站点的总带宽
    map<string, vector<map<string, int>>> site_t;
    map<string, vector<int>> site_t_usage;
    for(int t = 0; t < timestamps; t++){
        map<string, int> client_reamaining;
        map<string, int> site_remaining = site_bandwidth;
        map<string, int> site_current_bw = site_bandwidth;
        map<string, map<string, int>> site_t_client;
        for(auto it = demand.begin(); it != demand.end(); it++){
            client_reamaining.insert(make_pair(it->first, it->second[t]));
        }

        for(int co = 0; co < client_order.size(); co++){
            string client = client_order[co].first;
            while(client_reamaining[client] > 0){
                vector<string> actual_site = site4client[client];
                int average_bandwidth = int(ceil(float(client_reamaining[client]) / actual_site.size()));
                for(auto it = actual_site.begin(); it != actual_site.end(); it++){
                    string site = (*it);
                    if(client_reamaining[client] == 0)
                        break;
                    else if(client_reamaining[client] < 0)
                        cout<<"ERROR!"<<endl;

                    if(site_remaining[site] >= average_bandwidth){
                        client_reamaining[client] -= average_bandwidth;
                        site_remaining[site] -= average_bandwidth;
                        if (client_reamaining[client] <= 0){
                            site_remaining[site] += (0-client_reamaining[client]);
                            break;
                        }

                    }else{
                        actual_site.erase(it);
                    }
                }
                if(actual_site.size() == 0){
                    cout<<" No feasible solution"<<endl;
                }
            }
            for(int k = 0; k < site4client[client].size(); k++){
                string site = site4client[client][k];
                int assigned_bw = site_current_bw[site] - site_remaining[site];
                site_current_bw[site] = site_remaining[site];
                site_t_client[site][client] = assigned_bw;
                site_t_client[site]["usage"] += assigned_bw;
            }
        }

        for(auto it = site_bandwidth.begin(); it != site_bandwidth.end(); it++){
            site_t[it->first].push_back(site_t_client[it->first]);
            site_t_usage[it->first].push_back(site_t_client[it->first]["usage"]);
        }
    }

    // 尽可能塞满每一个边缘节点：超过95%的节点就尽量塞到上限，低于95%的节点就尽量塞到95%
    // 记录处理过的节点，避免移入节点的流量在后续操作其他节点时又流出
    set<string> site_processed;
    int position_95 = int(ceil(timestamps * 0.95) - 1);
    // for (auto site = site_bandwidth.begin(); site != site_bandwidth.end(); site++){
    for(auto si = site_order.rbegin(); si != site_order.rend(); si++){
        string site = si->first;
        site_processed.insert(site);
        vector<int> index = argsort(site_t_usage[site]);
        int value_95 = site_t_usage[site][index[position_95]];
        for(int position = 0; position < index.size(); position++){
            int t = index[position];
            int left = 0;
            if(position <= position_95){
                left = value_95 - site_t_usage[site][t];

                // left = site_t_usage[site->first][t];
                // if(left <= 0){
                //     continue;
                // }
                // for(auto client = client4site[site->first].begin(); client != client4site[site->first].end(); client++){
                //     for(auto site_client = site4client[*client].begin(); site_client != site4client[*client].end(); site_client++){
                //         if(left <= 0){
                //             break;
                //         }
                //         if(site_processed.find(*site_client) != site_processed.end() && site_t[site->first][t].find(*client) != site_t[site->first][t].end()){
                //             int move_flow = min(site_t[site->first][t][*client], site_bandwidth[*site_client] - site_t_usage[*site_client][t]);
                //             // if(site_t[site][t].find(*client) != site_t[site][t].end()){
                //             site_t[*site_client][t][*client] += move_flow;
                //             // }
                //             site_t_usage[*site_client][t] += move_flow;
                //             site_t[site->first][t][*client] -= move_flow;
                //             site_t_usage[site->first][t] -= move_flow;
                //             left -= move_flow;
                //         }
                //     }
                // }
            }else{
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
                        int move_flow = min(left, site_t[*site_client][t][*client]);
                        // if(site_t[site][t].find(*client) != site_t[site][t].end()){
                        site_t[site][t][*client] += move_flow;
                        // }
                        site_t_usage[site][t] += move_flow;
                        site_t[*site_client][t][*client] -= move_flow;
                        site_t_usage[*site_client][t] -= move_flow;
                        left -= move_flow;
                    }
                }
            }
            // }
        }
    }

    // 输出结果
    ofstream solution;
    solution.open(output_path);
    if(!solution.is_open()){
        std:cerr<<"failed to open "<<output_path;
    }
    int line_count = 0;
    for(int t = 0; t < timestamps; t++){
        for(auto client = demand.begin(); client != demand.end(); client++){
            solution << client->first <<":";
            line_count++;
            if(demand[client->first][t] == 0){
                solution << "\n";
                continue;
            }
            int count = 0;
            int writed_site_count = 0;
            for(int k = 0; k < site4client[client->first].size(); k++){
                count++;
                string site = site4client[client->first][k];
                // int assigned_bw = site_current_bw[site] - site_remaining[site];
                // site_current_bw[site] = site_remaining[site];
                int assigned_bw = 0;
                if(site_t[site][t].find(client->first) != site_t[site][t].end()){
                    assigned_bw = site_t[site][t][client->first];
                }
                if(assigned_bw != 0){
                    if(assigned_bw < 0){
                        cout << "error";
                    }
                    if (count < int(site4client[client->first].size())){
                        if (writed_site_count == 0){
                            solution << "<" << site << "," << assigned_bw << ">";
                            writed_site_count++;
                        }else if (writed_site_count > 0){
                            solution << ",<" << site << "," << assigned_bw << ">";
                            writed_site_count++;
                        }
                    }else{
                        if (writed_site_count == 0){
                            if (line_count != timestamps*int(client_order.size())){
                                solution << "<" << site << "," << assigned_bw << ">\n";
                            }else{
                                solution << "<" << site << "," << assigned_bw << ">";
                            }
                            
                        }else{
                            if (line_count != timestamps*int(client_order.size())){
                                solution << ",<" << site << "," << assigned_bw << ">" << endl;
                            }else{
                                solution << ",<" << site << "," << assigned_bw << ">";
                            }
                        }
                    }
                    solution << "<" << site << "," << assigned_bw << ">";
                    
                }
                else{
                    if((count == int(site4client[client->first].size())) && (line_count != timestamps * int(client_order.size()))){
                        solution << endl;
                    }
                }
            }
        }
    }
    solution.close();
    return 0;
}