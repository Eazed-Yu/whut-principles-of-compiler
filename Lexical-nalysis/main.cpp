#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <map>

using namespace std;
// 关键字映射表
map<string, string> keywords = {
    {"const", "CONSTTK"}, {"int", "INTTK"}, {"char", "CHARTK"},
    {"void", "VOIDTK"}, {"main", "MAINTK"}, {"if", "IFTK"},
    {"else", "ELSETK"}, {"do", "DOTK"}, {"while", "WHILETK"},
    {"for", "FORTK"}, {"scanf", "SCANFTK"}, {"printf", "PRINTFTK"},
    {"return", "RETURNTK"}
};

// 判断是否为单纯的单字符符号
bool isSingleCharSymbol(char c) {
    return string("+-*/;,()[]{}").find(c) != string::npos;
}

int main() {
    ifstream inFile("testfile.txt");
    ofstream outFile("output.txt");

    if (!inFile.is_open()) {
        cerr << "无法打开输入文件 testfile.txt" << endl;
        return 1;
    }

    char ch;
    while (inFile.get(ch)) {
        // 1. 跳过空白字符
        if (isspace(ch)) {
            continue;
        }

        // 2. 标识符或保留字
        if (isalpha(ch) || ch == '_') {
            string token = "";
            token += ch;
            // 继续读取直到不是字母、数字或下划线
            while (inFile.get(ch)) {
                if (isalnum(ch) || ch == '_') {
                    token += ch;
                } else {
                    inFile.unget();
                    break;
                }
            }

            // 查表判断是保留字还是标识符
            if (keywords.count(token)) {
                outFile << keywords[token] << " " << token << endl;
            } else {
                outFile << "IDENFR " << token << endl;
            }
        }
        // 3. 整型常量
        else if (isdigit(ch)) {
            string token;
            token += ch;
            while (inFile.get(ch)) {
                if (isdigit(ch)) {
                    token += ch;
                } else {
                    inFile.unget();
                    break;
                }
            }
            outFile << "INTCON " << token << endl;
        }
        // 4. 字符串常量
        else if (ch == '"') {
            string token;
            // 读取直到遇到下一个双引号
            while (inFile.get(ch)) {
                if (ch == '"') break;
                token += ch;
            }
            outFile << "STRCON " << token << endl;
        }
        // 5. 字符常量
        else if (ch == '\'') {
            string token = "";
            // 读取内容（题目样例似乎不包含转义字符处理，直接取单引号内的内容）
            while (inFile.get(ch)) {
                if (ch == '\'') break;
                token += ch;
            }
            outFile << "CHARCON " << token << endl;
        }
        // 6. 运算符和界符
        else {
            if (ch == '<') {
                if (inFile.get(ch)) {
                    if (ch == '=') outFile << "LEQ <=" << endl;
                    else {
                        inFile.unget();
                        outFile << "LSS <" << endl;
                    }
                } else outFile << "LSS <" << endl;
            }
            else if (ch == '>') {
                if (inFile.get(ch)) {
                    if (ch == '=') outFile << "GEQ >=" << endl;
                    else {
                        inFile.unget();
                        outFile << "GRE >" << endl;
                    }
                } else outFile << "GRE >" << endl;
            }
            else if (ch == '=') {
                if (inFile.get(ch)) {
                    if (ch == '=') outFile << "EQL ==" << endl;
                    else {
                        inFile.unget();
                        outFile << "ASSIGN =" << endl;
                    }
                } else outFile << "ASSIGN =" << endl;
            }
            else if (ch == '!') {
                if (inFile.get(ch)) {
                    if (ch == '=') outFile << "NEQ !=" << endl;
                    else {
                        inFile.unget();
                    }
                }
            }
            else {
                // 单字符符号处理
                switch (ch) {
                    case '+': outFile << "PLUS +" << endl; break;
                    case '-': outFile << "MINU -" << endl; break;
                    case '*': outFile << "MULT *" << endl; break;
                    case '/': outFile << "DIV /" << endl; break;
                    case ';': outFile << "SEMICN ;" << endl; break;
                    case ',': outFile << "COMMA ," << endl; break;
                    case '(': outFile << "LPARENT (" << endl; break;
                    case ')': outFile << "RPARENT )" << endl; break;
                    case '[': outFile << "LBRACK [" << endl; break;
                    case ']': outFile << "RBRACK ]" << endl; break;
                    case '{': outFile << "LBRACE {" << endl; break;
                    case '}': outFile << "RBRACE }" << endl; break;
                    default:
                        break;
                }
            }
        }
    }

    inFile.close();
    outFile.close();
    return 0;
}

