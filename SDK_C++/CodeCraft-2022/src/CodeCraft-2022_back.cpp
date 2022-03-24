#include <bits/stdc++.h>
#define DEBUG 1
using namespace std;

map<string, int> getSiteBandwidth(string path){
    map<string, int> result;
    ifstream fin(path);
    if(!fin.is_open()){
        std::cerr<<"failed to open data/site_bandwidth.csv.";
    }
    string header;
    getline(fin, header);
    string site_bandwidth_info;
    int bandwidth;
    string site_name;
    string bandwidth_str;
    while(getline(fin, site_bandwidth_info)){
        istringstream readline(site_bandwidth_info);
        getline(readline, site_name, ',');
        getline(readline, bandwidth_str, ',');
        bandwidth = atoi(bandwidth_str.c_str());
        if(DEBUG){
            cout<<"site name:"<<site_name<<" bandwidth:"<<bandwidth<<endl;
        }
        result.insert(make_pair(site_name, bandwidth));
    }
    fin.close();
    return result;
}

int getConstraint(string path){
    ifstream fconfig;
    fconfig.open(path);
    if(!fconfig.is_open()){
        std::cerr<<"failed to open data/config.ini";
    }
    string find_qos_line;
    string qos_constraint_str;
    while(getline(fconfig, find_qos_line)){
        if(find_qos_line.find("qos_constraint=") != std::string::npos){
            qos_constraint_str = find_qos_line.substr(find_qos_line.find("=")+1);
            if(DEBUG)
                cout<<"check qos constraint string:"<<qos_constraint_str<<endl;
        }
    }
    int qos_constraint = 0;
    qos_constraint = atoi(qos_constraint_str.c_str());
    fconfig.close();
    if(DEBUG)
        cout<<"qos constraint(int):"<<qos_constraint<<endl;
    return qos_constraint;
}

int main() {
    string site_bandwidth_path = "/home/hadoop/2022HUAWEIChallenge/SDK/data/site_bandwidth.csv";
    string config_path = "/home/hadoop/2022HUAWEIChallenge/SDK/data/config.ini";
    string demand_path = "/home/hadoop/2022HUAWEIChallenge/SDK/data/demand.csv";
    string qos_path = "/home/hadoop/2022HUAWEIChallenge/SDK/data/qos.csv";

    int qos_constraint = getConstraint(config_path);
    map<string, int> site_bandwidth = getSiteBandwidth(site_bandwidth_path);
    
    auto it = site_bandwidth.find("A");
    if(DEBUG)
        cout<<"check:"<<it->second<<endl;

    // map<string, vector<int>> demand;
    // ifstream inFile("/home/hadoop/2022HUAWEIChallenge/SDK/SDK_C++/data/demand.csv", ios::in);
    // string line;
    // while (getline(inFile, line))
    // {
    //     istringstream readstr(line);
    //     string tmp;
    //     bool skip = true;
    //     while(getline(readstr, tmp, ',')){
    //         if (skip == true){
    //             continue;
    //         }
    //         demand[tmp] = 1;
    //     }
    //     cout << line << endl;
    // }
    cout << "hello" << endl;
	return 0;
}
