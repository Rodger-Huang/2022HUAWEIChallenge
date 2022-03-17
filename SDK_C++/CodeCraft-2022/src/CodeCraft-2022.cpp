#include <bits/stdc++.h>
using namespace std;

int main() {
    map<string, vector<int>> demand;
    ifstream inFile("/home/hadoop/2022HUAWEIChallenge/SDK/SDK_C++/data/demand.csv", ios::in);
    string line;
    while (getline(inFile, line))
    {
        istringstream readstr(line);
        string tmp;
        bool skip = true;
        while(getline(readstr, tmp, ',')){
            if (skip == true){
                continue;
            }
            demand[tmp] = 1;
        }
        cout << line << endl;
    }
    
	return 0;
}
