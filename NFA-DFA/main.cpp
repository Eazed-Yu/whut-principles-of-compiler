#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <sstream>

using namespace std;

// 结构体：存储边的信息
struct Transition {
    char val;      // 输入字符
    string to;     // 目标状态ID (改为string以支持两位数状态)
};

// NFA存储：状态名 -> 边列表
map<string, vector<Transition>> nfa;
// 收集所有的输入符号 (a, b, c...)，不包含 '~'
set<char> alphabet;

// 字符串分割辅助函数
vector<string> split(const string& str, const string& delimiter) {
    vector<string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != string::npos) {
        if (end > start) {
            tokens.push_back(str.substr(start, end - start));
        }
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    if (start < str.length()) {
        tokens.push_back(str.substr(start));
    }
    return tokens;
}

// 获取单个状态的epsilon闭包 (包含自身)
void getEpsilonClosureSingle(string state, set<string> &closure) {
    // 避免死循环：如果已经处理过该状态，直接返回
    if (closure.count(state)) return;

    closure.insert(state);

    if (nfa.count(state)) {
        for (auto &edge : nfa[state]) {
            if (edge.val == '~') {
                getEpsilonClosureSingle(edge.to, closure);
            }
        }
    }
}

// 获取一个集合的epsilon闭包
set<string> getSetEpsilonClosure(const set<string>& states) {
    set<string> result;
    for (const auto& s : states) {
        getEpsilonClosureSingle(s, result); // 此时 result 充当 visited 集合
    }
    return result;
}

// Move操作：从状态集合states经过字符val能到达的NFA状态集合
set<string> moveSet(const set<string>& states, char val) {
    set<string> result;
    for (const auto& s : states) {
        if (nfa.count(s)) {
            for (auto &edge : nfa[s]) {
                if (edge.val == val) {
                    result.insert(edge.to);
                }
            }
        }
    }
    return result;
}

// 检查集合中是否包含NFA的终态 (这里假设NFA中包含'Y'的即为终态)
bool isFinalSet(const set<string>& states) {
    for (const auto& s : states) {
        // 如果状态名包含 'Y'，认为是终态
        if (s.find('Y') != string::npos) return true;
    }
    return false;
}

// 定义DFA的边
struct DfaEdge {
    string from;
    char input;
    string to;
};

int main() {
    string line;
    while (getline(cin, line) && !line.empty()) {
        vector<string> parts = split(line, " ");
        if (parts.empty()) continue;

        string u = parts[0];
        // 确保该状态在NFA中有记录（即使没有出边）
        if (nfa.find(u) == nfa.end()) {
            nfa[u] = {};
        }

        for (size_t i = 1; i < parts.size(); i++) {
            string transStr = parts[i];
            // 解析格式: u-a->v
            // 找到第一个 '-' 和 "->" 的位置
            size_t firstDash = transStr.find('-');
            size_t arrow = transStr.find("->");

            if (firstDash != string::npos && arrow != string::npos) {
                // 提取转换字符 (可能在 - 和 -> 之间)
                // 假设转换字符只有一个字符
                char val = transStr[firstDash + 1];
                string v = transStr.substr(arrow + 2);

                nfa[u].push_back({val, v});
                if (val != '~') {
                    alphabet.insert(val);
                }
            }
        }
    }

    // 2. 子集构造法构建DFA

    // 状态映射：NFA状态集合 -> DFA状态名
    map<set<string>, string> subsetToDfaName;
    // 队列：待处理的DFA状态(即NFA子集)
    queue<set<string>> processingQueue;
    // 存储DFA的转换边
    vector<DfaEdge> dfaEdges;

    int processIdCnt = 0; // 0, 1, 2...
    int finalIdCnt = 0;   // Y, Y1, Y2...

    // 初始状态 X 的闭包
    set<string> startSet = getSetEpsilonClosure({"X"});
    subsetToDfaName[startSet] = "X";
    processingQueue.push(startSet);

    // 记录已经生成的DFA状态名，用于后续排序输出
    vector<string> dfaStatesList;
    dfaStatesList.push_back("X");

    while (!processingQueue.empty()) {
        set<string> currentSet = processingQueue.front();
        processingQueue.pop();

        string currentDfaName = subsetToDfaName[currentSet];

        // 对字母表中的每个符号进行转移
        for (char inputChar : alphabet) {
            // move(T, a)
            set<string> temp = moveSet(currentSet, inputChar);
            // epsilon-closure(move(T, a))
            set<string> nextSet = getSetEpsilonClosure(temp);

            if (nextSet.empty()) continue;


            if (subsetToDfaName.find(nextSet) == subsetToDfaName.end()) {
                string newName;
                // 命名逻辑
                if (isFinalSet(nextSet)) {
                    if (finalIdCnt == 0) newName = "Y";
                    else newName = "Y" + to_string(finalIdCnt);
                    finalIdCnt++;
                } else {
                    newName = to_string(processIdCnt++);
                }

                subsetToDfaName[nextSet] = newName;
                dfaStatesList.push_back(newName);
                processingQueue.push(nextSet);
            }

            // 记录边
            dfaEdges.push_back({currentDfaName, inputChar, subsetToDfaName[nextSet]});
        }
    }

    // 3. 输出格式化
    // 题目要求输出形式归组： X X-a->0 X-b->1


    vector<string> sortedStates = dfaStatesList;
    sort(sortedStates.begin(), sortedStates.end(), [](const string& a, const string& b) {
        // 自定义优先级: X最前, Y其次, 数字最后
        int prioA = (a == "X") ? 0 : (a[0] == 'Y' ? 1 : 2);
        int prioB = (b == "X") ? 0 : (b[0] == 'Y' ? 1 : 2);
        if (prioA != prioB) return prioA < prioB;
        // 同类比较
        if (a[0] == 'Y' && b[0] == 'Y') {
            if (a == "Y") return true;
            if (b == "Y") return false;
            return a.length() < b.length() || (a.length() == b.length() && a < b);
        }
        if (isdigit(a[0]) && isdigit(b[0])) {
             return stoi(a) < stoi(b);
        }
        return a < b;
    });

    for (const string& u : sortedStates) {
        cout << u;
        // 找所有从 u 出发的边
        vector<pair<char, string>> edges;
        for (const auto& edge : dfaEdges) {
            if (edge.from == u) {
                edges.push_back({edge.input, edge.to});
            }
        }
        // 按字符排序 a, b...
        sort(edges.begin(), edges.end());

        for (const auto& e : edges) {
            cout << " " << u << "-" << e.first << "->" << e.second;
        }
        cout << endl;
    }

    return 0;
}