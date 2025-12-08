#include <iostream>
#include <string>
#include <map>
#include <set>
#include <sstream>

using namespace std;

map<string, map<char, string>> dfa;
set<string> final_states;
string start_state = "X";

int main() {
    string token;
    // 1. 读取字母表
    while (cin >> token) {
        if (token.back() == '#') break;
    }

    // 2. 读取状态集合
    while (cin >> token) {
        string s = token;
        if (s.back() == '#') s.pop_back();
        // 题目约定 Y 为终态
        if (s == "Y") final_states.insert(s);
        if (token.back() == '#') break;
    }

    // 消耗掉状态行末尾的换行符
    string line;
    getline(cin, line);

    while (getline(cin, line)) {
        if (line.empty()) break;

        stringstream ss(line);
        string part;
        while (ss >> part) {
            size_t arrow = part.find("->");
            size_t dash = part.find('-');


            if (arrow != string::npos && dash != string::npos) {
                string u = part.substr(0, dash);      // 源状态
                char c = part[dash + 1];              // 输入字符
                string v = part.substr(arrow + 2);    // 目标状态
                dfa[u][c] = v;
            }
        }
    }

    while (getline(cin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string input_str;
        ss >> input_str;

        if (input_str.empty()) continue;
        if (input_str.back() == '#') input_str.pop_back();

        string curr = start_state;
        bool error_occurred = false;


        for (char c : input_str) {

            if (dfa[curr].count(c)) {

                cout << c << endl;

                curr = dfa[curr][c];
            } else {

                cout << "error" << endl;
                error_occurred = true;
                break;
            }
        }


        if (!error_occurred) {
            if (final_states.count(curr)) {
                cout << "pass" << endl;
            } else {
                cout << "error" << endl;
            }
        }
    }

    return 0;
}