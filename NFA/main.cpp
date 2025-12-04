#include <iostream>
#include <stack>
#include <string>



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
std::string addConcatSymbol(const std::string& regex) {
    std::string res = "";
    for (int i = 0; i < regex.length(); ++i) {
        char c1 = regex[i];
        res += c1;
        if (i + 1 < regex.length()) {
            char c2 = regex[i + 1];
            // 逻辑：如果 c1 是 [字符, *, )] 且 c2 是[字符, (]，则中间需要加点
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
std::string infixToPostfix(const std::string& regex) {
    std::string postfix;
    std::stack<char> opStack;

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

int main()
{
    std::string s;
    std::cin >> s;
    auto res = addConcatSymbol(s);
    std::cout << res << std::endl;
    std::cout << infixToPostfix(res);
    return 0;
}