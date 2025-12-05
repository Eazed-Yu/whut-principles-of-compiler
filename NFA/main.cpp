#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include <algorithm>
#include <queue>
#include <set>

using namespace std;

// 定义转换结构：表示从当前状态接收输入 'val' 后到达状态 'to'
struct Transition {
    char val; // 输入字符，'~' 表示空串 (Epsilon)
    int to;   // 目标状态ID
};

// 全局邻接表存储NFA
// key: 状态ID, value: 该状态出发的转换列表
map<int, vector<Transition>> nfa;
int state_counter = 0; // 用于生成唯一的状态ID

// NFA片段结构体，记录该片段的起始和结束状态ID
struct NFAFragment {
    int start;
    int end;
};

// 分配一个新的状态ID
int newState() {
    return state_counter++;
}

// 向全局NFA中添加一条边
void addEdge(int u, int v, char val) {
    nfa[u].push_back({val, v});
}

// 1. 预处理：显式添加连接符 '.'
// 例如："ab" -> "a.b", "a(b)" -> "a.(b)", "a*b" -> "a*.b"
string addConcat(string s) {
    string res = "";
    for (int i = 0; i < s.length(); i++) {
        res += s[i];
        if (i + 1 < s.length()) {
            char c1 = s[i];
            char c2 = s[i+1];

            // 判断是否需要加点：
            // 如果 c1 是字符、')' 或 '*'，且 c2 是字符 或 '('，则中间意味着连接
            bool c1_is_operand = (isalnum(c1) || c1 == '*' || c1 == ')');
            bool c2_is_operand = (isalnum(c2) || c2 == '(');

            if (c1_is_operand && c2_is_operand) {
                res += '.';
            }
        }
    }
    return res;
}

// 获取运算符优先级
int getPrecedence(char c) {
    if (c == '*') return 3; // 闭包优先级最高
    if (c == '.') return 2; // 连接次之
    if (c == '|') return 1; // 选择最低
    return 0;
}

// 2. 中缀表达式转后缀表达式 (逆波兰式)
string toPostfix(string s) {
    string res = "";
    stack<char> ops;
    for (char c : s) {
        if (isalnum(c)) {
            res += c; // 操作数直接输出
        } else if (c == '(') {
            ops.push(c);
        } else if (c == ')') {
            // 弹出直到遇到左括号
            while (!ops.empty() && ops.top() != '(') {
                res += ops.top();
                ops.pop();
            }
            if (!ops.empty()) ops.pop(); // 弹出 '('
        } else {
            // 运算符处理：弹出优先级大于等于当前运算符的栈顶元素
            while (!ops.empty() && getPrecedence(ops.top()) >= getPrecedence(c)) {
                res += ops.top();
                ops.pop();
            }
            ops.push(c);
        }
    }
    // 弹出剩余运算符
    while (!ops.empty()) {
        res += ops.top();
        ops.pop();
    }
    return res;
}

// 3. 使用 Thomson 构造法根据后缀表达式构建 NFA
NFAFragment buildNFA(string postfix) {
    stack<NFAFragment> st;

    for (char c : postfix) {
        if (isalnum(c)) {
            // 基本情况：字符转换 a -> (s)-a->(e)
            int s = newState();
            int e = newState();
            addEdge(s, e, c);
            st.push({s, e});
        } else if (c == '*') {
            // Kleene Star (闭包)
            NFAFragment A = st.top(); st.pop();
            int s = newState();
            int e = newState();

            addEdge(s, A.start, '~'); // 新起点通过空串进入A
            addEdge(s, e, '~');       // 新起点直接跳过A到终点
            addEdge(A.end, A.start, '~'); // A的终点循环回A的起点
            addEdge(A.end, e, '~');       // A的终点离开到新终点

            st.push({s, e});
        } else if (c == '.') {
            // Concatenation (连接)
            NFAFragment B = st.top(); st.pop(); // 注意栈是后进先出，所以先弹出B，再弹出A
            NFAFragment A = st.top(); st.pop();

            // 连接 A 的终点到 B 的起点（通过空串）
            addEdge(A.end, B.start, '~');

            // 新片段起点是A的起点，终点是B的终点
            st.push({A.start, B.end});
        } else if (c == '|') {
            // Union (选择/并联)
            NFAFragment B = st.top(); st.pop();
            NFAFragment A = st.top(); st.pop();
            int s = newState();
            int e = newState();

            // 新起点分裂到 A 和 B
            addEdge(s, A.start, '~');
            addEdge(s, B.start, '~');

            // A 和 B 汇聚到新终点
            addEdge(A.end, e, '~');
            addEdge(B.end, e, '~');

            st.push({s, e});
        }
    }
    return st.top();
}

// 辅助函数：根据映射获取状态的显示名称 (X, Y, 或数字)
string getStateName(int id, int originalStart, int originalEnd, const map<int, int>& mapping) {
    if (id == originalStart) return "X";
    if (id == originalEnd) return "Y";
    // 如果不是起点终点，返回映射后的数字ID
    if (mapping.find(id) != mapping.end()) {
        return to_string(mapping.at(id));
    }
    return "?"; // 异常情况
}

int main() {
    string input;
    if (!(cin >> input)) return 0;

    // 步骤1 & 2：预处理和转后缀
    string withConcat = addConcat(input);
    string postfix = toPostfix(withConcat);

    // 初始化全局变量
    nfa.clear();
    state_counter = 0;

    // 步骤3：构建 NFA
    NFAFragment result = buildNFA(postfix);

    // 步骤4：状态重命名 (BFS)
    // 题目要求：初始态 X，终态 Y，过程态 1, 2, 3... (根据样例是1开始)
    // 我们使用 BFS 来按照遍历顺序给状态分配 1, 2, 3... 这样的编号

    map<int, int> id_map; // 原始ID -> 新的数字ID
    queue<int> q;
    vector<bool> visited(state_counter, false);

    q.push(result.start);
    visited[result.start] = true;

    int current_label = 1; // 过程态计数器

    while(!q.empty()) {
        int u = q.front();
        q.pop();

        // 遍历当前状态的所有出边
        if (nfa.find(u) != nfa.end()) {
            for (auto &edge : nfa[u]) {
                if (!visited[edge.to]) {
                    visited[edge.to] = true;
                    // 如果不是最终状态，分配一个新的数字编号
                    if (edge.to != result.end) {
                        id_map[edge.to] = current_label++;
                    }
                    q.push(edge.to);
                }
            }
        }
    }

    // 步骤5：整理并输出
    // 收集所有需要输出的状态：起点 + 终点 + 过程态
    vector<int> all_states;
    all_states.push_back(result.start);
    if (result.start != result.end) {
        all_states.push_back(result.end);
    }
    for(auto const& [old_id, new_id] : id_map) {
        all_states.push_back(old_id);
    }

    // 排序逻辑：
    // 我们希望输出顺序为：X 先，然后是 Y (如果有的话通常放第二或者最后，但样例中 Y 在第二行)，
    // 接着是数字状态按顺序排列 (1, 2, 3...)。
    // 为了匹配样例输出：X, Y, 1, 2, 3...
    sort(all_states.begin(), all_states.end(), [&](int a, int b) {
        int pA, pB;

        // 设定优先级：Start=0, End=1, 其他=new_id+2
        if (a == result.start) pA = 0;
        else if (a == result.end) pA = 1;
        else pA = id_map[a] + 2;

        if (b == result.start) pB = 0;
        else if (b == result.end) pB = 1;
        else pB = id_map[b] + 2;

        return pA < pB;
    });

    // 执行打印
    for(int u : all_states) {
        string uName = getStateName(u, result.start, result.end, id_map);
        cout << uName;

        // 如果该状态有出边
        if (nfa.find(u) != nfa.end() && !nfa[u].empty()) {
            vector<Transition> edges = nfa[u];

            // 为了保证输出确定性，可以对边进行简单排序（可选）
            // 样例输出似乎倾向于先输出非空串，或者按目标状态名排序
            // 这里我们不强求边的顺序，通常 Thomson 构造的顺序就是正确的逻辑顺序

            for (auto &edge : edges) {
                string vName = getStateName(edge.to, result.start, result.end, id_map);
                cout << " " << uName << "-" << edge.val << "->" << vName;
            }
        }
        cout << endl;
    }

    return 0;
}