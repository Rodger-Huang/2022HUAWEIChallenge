#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<map>
#include<vector>
#include<algorithm>
#include<cmath>
#include<typeinfo>
#define CHECK_DATA 0

using namespace std;

const string input_path = "../../../data/";
const string output_path = "../../../output/solution.txt";

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
    getline(fin,header);
    string site_bandwidth_info;
    int bandwidth;
    string site_name;
    string bandwidth_str;
    while(getline(fin,site_bandwidth_info)){
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
//client->[site1,site2,...]
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
int main(){
    ofstream solution;
    solution.open(output_path);
    if(!solution.is_open()){
        std:cerr<<"failed to open "<<output_path;
    }

    int qos_constraint = getConstraint();
    map<string,int> site_bandwidth = getSiteBandwidth();
    map<string, vector<int>> demand = getDemand();
    
    map<string, vector<string>> site4client = getSiteForClient();
    map<string, vector<string>> client4site = getClientForSite();

    map<string, int> client_site_number;
    map<string, int> site_client_number;

    //sort client by site number
    vector<pair<string,int>> client_order;

    for(auto it = site4client.begin(); it != site4client.end(); it++){
        client_site_number.insert(make_pair(it->first, it->second.size()));
        client_order.push_back(make_pair(it->first, it->second.size()));
    }
    sort(client_order.begin(), client_order.end(), cmp);

    for(auto it = client4site.begin(); it != client4site.end(); it++){
        site_client_number.insert(make_pair(it->first, it->second.size()));
    }

    cout<<"timestamp="<<timestamps<<" client num="<<client_number<<" site_num="<<site_number<<endl;
    //注意：可以通过，demand[client][t]获取客户在t时刻的总需求
    //通过 site_bandwidth[site]获取站点的总带宽
    for(int t = 0; t < timestamps; t++){
        map<string, int> client_reamaining;
        map<string, int> site_remaining = site_bandwidth;
        map<string, int> site_current_bw = site_bandwidth;

        for(auto it = demand.begin(); it != demand.end(); it++){
            client_reamaining.insert(make_pair(it->first, it->second[t]));
        }

        for(int co = 0; co < client_order.size(); co++){
            string client = client_order[co].first;
            //cout<<"dealing with client"<<client;
            solution << client <<":";
            if(demand[client][t] == 0){
                solution<<"\n";
                continue;
            }
            int test = 0;
            while(client_reamaining[client] > 0 && test < 5){
                test++;
                //cout<<"client remaining demand=" << client_reamaining[client]<<endl;
                vector<string> actual_site = site4client[client];
                //cout<<"size actual site="<<actual_site.size();
                int average_bandwidth = ceil(client_reamaining[client] / actual_site.size());
                for(auto it = actual_site.begin(); it != actual_site.end();){
                    string site = (*it);
                    //cout<<"checking site"<<site;
                    if(site_remaining[site] >= average_bandwidth){
                        //cout<<"got bandwidth="<<average_bandwidth<<" from site="<<site;
                        client_reamaining[client] -= average_bandwidth;
                        site_remaining[site] -= average_bandwidth;
                        if (client_reamaining[client] < 0){
                            site_remaining[site] += (0 - client_reamaining[client]);
                            client_reamaining[client] = 0;
                            break;
                        }
                        it++;
                    }
                    else{
                        it = actual_site.erase(it);
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
                if(assigned_bw != 0){
                    if(assigned_bw < 0){
                        cout<<"error";
                    }
                    if(k == site4client[client].size()-1){
                        solution << "<" << site << "," << assigned_bw << ">";
                    }
                    else{
                        solution<< "<" << site << "," << assigned_bw << ">,";
                    }
                }
                else{
                    cout<<"zero assigned bw, ts="<<t<<"client="<<client<<endl;
                }
            }
            //solution<<endl<<t;
            if(t != timestamps - 1 || co != client_order.size()-1){
                solution << endl;
            }
        }
    }

    solution.close();
    
}