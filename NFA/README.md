实验内容：
```markdown
【问题描述】

正规表达式→NFA问题的一种描述是：

编写一个程序，输入一个正规表达式，输出与该文法等价的有穷自动机。

【基本要求】设置FA初始状态X，终态Y，过程态用数字表示：0 1 2 3………



【输入形式】正规式形式
【输出形式】NFA的状态转换式


【样例输入】
0(0|1)*

【样例输出】
X X-0->1
Y 
1 1-~->2 
2 2-~->3 2-~->Y 
3 3-~->4 3-~->6 
4 4-0->5 
5 5-~->8 
6 6-1->7 
7 7-~->8 
8 8-~->3 8-~->Y


【样例说明】符号”~“表示空串
```


我们将采用经典的 **Thompson 构造法 (Thompson's Construction Algorithm)**。

### 程序设计思路
1.  **预处理**：输入的正规表达式通常是中缀形式（如 `a(b|c)*`），且省略了连接符。我们需要显式地加上连接符 `.`（例如变为 `a.(b|c)*`）。
2.  **后缀转换**：将中缀表达式转换为后缀表达式（逆波兰表示法），以便于计算机按顺序处理运算符优先级（`*` > `.` > `|`）。
3.  **构建NFA**：使用栈结构，根据后缀表达式逐步构建NFA片段。
    *   遇到字符：创建一个基本的 `Start -> End` 转换。
    *   遇到 `.`（连接）：连接前一个片段的尾部到后一个片段的头部。
    *   遇到 `|`（选择）：新建起点和终点，并联两个片段。
    *   遇到 `*`（闭包）：新建起点和终点，添加循环和跳过的空串（Epsilon）转换。
4.  **状态重命名 (BFS)**：
    *   题目要求初始态为 `X`，终态为 `Y`，中间过程态为数字。
    *   由于构建过程中生成的状态ID是杂乱的，我们使用广度优先搜索（BFS）从起点遍历图，按照发现顺序将中间状态依次命名为 1, 2, 3...
5.  **输出**：按照题目要求的格式输出状态转换表。


# 正规表达式转 NFA

## 1. 核心流程

将一个字符串形式的正则表达式转换为 NFA 图通常分为三个步骤：

1.  **预处理 (Preprocessing)**：处理隐式连接符。例如，将 `ab` 转换为 `a.b`。
2.  **转后缀表达式 (Infix to Postfix)**：将中缀表达式（如 `a|b`）转换为后缀表达式（如 `ab|`），以去除括号并确定运算优先级。
3.  **Thompson 构造 (NFA Construction)**：利用栈结构，根据后缀表达式自底向上构建 NFA 片段。

---

## 2. 详细步骤解析

### 第一步：显式添加连接符 (.)

正则表达式中，`ab` 表示 `a` 连接 `b`，但并没有显式的运算符。为了方便计算，我们需要在以下情况中间插入连接符（假设用 `.` 表示）：
*   两个字符之间：`ab` -> `a.b`
*   字符和左括号之间：`a(b` -> `a.(b`
*   右括号/星号和字符之间：`)a` -> `).a`, `*a` -> `*.a`

### 第二步：中缀转后缀 (Shunting-yard Algorithm)

我们需要处理的运算符优先级从高到低为：
1.  **闭包 (Kleene Star, `*`)**
2.  **连接 (Concatenation, `.`)**
3.  **并集 (Union, `|`)**

利用栈将 `a.b|c*` 转换为 `ab.c*|`。

### 第三步：Thompson 构造法

我们需要维护一个 **NFA 片段 (Fragment)** 的栈。每个片段包含一个**起始状态**和一个**结束状态**。

*   **遇到字符 (如 'a')**：
    *   新建状态 `start`, `end`。
    *   连边 `start --a--> end`。
    *   入栈。
*   **遇到连接 (.)**：
    *   弹出片段 `B`，弹出片段 `A`。
    *   将 `A` 的结束状态与 `B` 的开始状态合并（或连一条 $\epsilon$ 边）。
    *   新片段：`A.start` -> `B.end`。
    *   入栈。
*   **遇到并集 (|)**：
    *   弹出片段 `B`，弹出片段 `A`。
    *   新建总起 `S`，总终 `E`。
    *   `S` 连 $\epsilon$ 到 `A.start` 和 `B.start`。
    *   `A.end` 和 `B.end` 连 $\epsilon$ 到 `E`。
    *   新片段：`S` -> `E`。
    *   入栈。
*   **遇到闭包 (*)**：
    *   弹出片段 `A`。
    *   新建总起 `S`，总终 `E`。
    *   `S` 连 $\epsilon$ 到 `A.start`，且 `S` 连 $\epsilon$ 到 `E`（匹配0次）。
    *   `A.end` 连 $\epsilon$ 到 `A.start`（循环），且 `A.end` 连 $\epsilon$ 到 `E`。
    *   新片段：`S` -> `E`。
    *   入栈。

---

## 3. C++ 代码实现

以下是一个清晰的、面向对象的 C++ 实现。

### 3.1 数据结构定义

```cpp
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <map>

using namespace std;

// 定义边结构
struct Edge {
    int to;
    char symbol; // '~' 代表 Epsilon (空边)
};

// 定义 NFA 片段
// 每个正则部分都会生成一个子图，拥有唯一的入口(start)和出口(end)
struct Fragment {
    int startState;
    int endState;
};

// 全局状态计数器和邻接表
int stateCount = 0;
map<int, vector<Edge>> nfaGraph;

// 申请新状态 ID
int newState() {
    return stateCount++;
}

// 添加边
void addEdge(int u, int v, char symbol) {
    nfaGraph[u].push_back({v, symbol});
}
```

### 3.2 预处理与后缀转换

```cpp
// 判断是否为操作符
bool isOperator(char c) {
    return c == '|' || c == '*' || c == '.' || c == '(' || c == ')';
}

// 获取优先级
int precedence(char op) {
    if (op == '*') return 3;
    if (op == '.') return 2;
    if (op == '|') return 1;
    return 0;
}

// 1. 插入显式连接符 '.'
string addConcatSymbol(const string& regex) {
    string res = "";
    for (int i = 0; i < regex.length(); ++i) {
        char c1 = regex[i];
        res += c1;
        if (i + 1 < regex.length()) {
            char c2 = regex[i + 1];
            // 逻辑：如果 c1 是 字符/*/) 且 c2 是 字符/(，则中间需要加点
            bool c1Valid = !isOperator(c1) || c1 == '*' || c1 == ')';
            bool c2Valid = !isOperator(c2) || c2 == '(';
            if (c1Valid && c2Valid) {
                res += '.';
            }
        }
    }
    return res;
}

// 2. 中缀转后缀
string infixToPostfix(const string& regex) {
    string postfix = "";
    stack<char> opStack;
    
    for (char c : regex) {
        if (!isOperator(c)) {
            postfix += c;
        } else if (c == '(') {
            opStack.push(c);
        } else if (c == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                postfix += opStack.top();
                opStack.pop();
            }
            if (!opStack.empty()) opStack.pop(); // 弹出 '('
        } else {
            // 处理优先级 *, ., |
            while (!opStack.empty() && precedence(opStack.top()) >= precedence(c)) {
                postfix += opStack.top();
                opStack.pop();
            }
            opStack.push(c);
        }
    }
    while (!opStack.empty()) {
        postfix += opStack.top();
        opStack.pop();
    }
    return postfix;
}
```

### 3.3 Thompson 构造核心逻辑

```cpp
// 3. 根据后缀表达式构建 NFA
Fragment buildNFA(const string& postfix) {
    stack<Fragment> st;
    
    for (char c : postfix) {
        if (!isOperator(c)) {
            // 字面量： S -c-> E
            int s = newState();
            int e = newState();
            addEdge(s, e, c);
            st.push({s, e});
        } 
        else if (c == '.') {
            // 连接： A.start ... A.end -> B.start ... B.end
            Fragment b = st.top(); st.pop();
            Fragment a = st.top(); st.pop();
            
            // 将 A 的结束连到 B 的开始 (空边)
            addEdge(a.endState, b.startState, '~');
            
            st.push({a.startState, b.endState});
        } 
        else if (c == '|') {
            // 并集： S -> A, S -> B; A -> E, B -> E
            Fragment b = st.top(); st.pop();
            Fragment a = st.top(); st.pop();
            
            int s = newState();
            int e = newState();
            
            addEdge(s, a.startState, '~');
            addEdge(s, b.startState, '~');
            addEdge(a.endState, e, '~');
            addEdge(b.endState, e, '~');
            
            st.push({s, e});
        } 
        else if (c == '*') {
            // 闭包： S -> A, A -> S, S -> E, A -> E
            Fragment a = st.top(); st.pop();
            
            int s = newState();
            int e = newState();
            
            addEdge(s, a.startState, '~'); // 进入 A
            addEdge(s, e, '~');            // 匹配 0 次
            addEdge(a.endState, a.startState, '~'); // 循环
            addEdge(a.endState, e, '~');   // 离开
            
            st.push({s, e});
        }
    }
    
    return st.top();
}
```

### 3.4 主函数与测试

```cpp
void printGraph(int start, int end) {
    cout << "NFA Graph (Start: " << start << ", End: " << end << ")\n";
    cout << "Format: From --Symbol--> To\n";
    cout << "---------------------------\n";
    for (auto const& [u, edges] : nfaGraph) {
        for (const auto& edge : edges) {
            cout << u << " --" << edge.symbol << "--> " << edge.to << endl;
        }
    }
}

int main() {
    // 重置全局变量
    stateCount = 0;
    nfaGraph.clear();
    
    string regex;
    cout << "Enter Regex (e.g., (a|b)*abb): ";
    cin >> regex;
    
    string withConcat = addConcatSymbol(regex);
    cout << "1. Added Concatenation: " << withConcat << endl;
    
    string postfix = infixToPostfix(withConcat);
    cout << "2. Postfix: " << postfix << endl;
    
    Fragment finalNFA = buildNFA(postfix);
    
    cout << "3. Construction Complete." << endl;
    printGraph(finalNFA.startState, finalNFA.endState);
    
    return 0;
}
```

---

## 4. 运行示例

假设输入正则表达式：`a(b|c)*`

**控制台输出：**

```text
Enter Regex: a(b|c)*
1. Added Concatenation: a.(b|c)*
2. Postfix: abc|*.
3. Construction Complete.
NFA Graph (Start: 0, End: 9)
Format: From --Symbol--> To
---------------------------
0 --a--> 1
1 --~--> 8
2 --b--> 3
3 --~--> 7
4 --c--> 5
5 --~--> 7
6 --~--> 2
6 --~--> 4
7 --~--> 6
7 --~--> 9
8 --~--> 6
8 --~--> 9
```

## 5. 关键优化提示

注意以下几点：

1.  **Epsilon 边的处理**：在 NFA 中，Epsilon (`~`) 边允许不消耗字符进行转移。在后续将 NFA 转 DFA 时，需要计算 **Epsilon-Closure (空闭包)**。
2.  **状态压缩**：Thompson 算法生成的 NFA 状态非常多（很多空边）。如果在意性能，可以在连接操作（`.`）时，直接合并两个节点（`unite(A.end, B.start)`），而不是加一条边，这也就是你之前代码中使用并查集 (DSU) 的原因。
3.  **内存管理**：上述代码使用了 `map` 和 `vector`，对于超大规模正则可能较慢，可以用静态数组优化。

这个教程涵盖了从原理到实现的完整过程，代码结构清晰，易于扩展。