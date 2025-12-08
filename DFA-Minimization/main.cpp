#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

map<string, map<char, string>> adj;
set<string> allStates;
set<char> alphabet;
set<string> finalStates;
string startState = "X";

pair<char, string> parseTransition(const string& t)
{
    size_t dashPos = t.find('-');
    size_t arrowPos = t.find("->");

    // 获取输入字符 (位于第一个 '-' 和 "->" 之间)
    // 假设输入字符长度为1
    char inputChar = t[dashPos + 1];

    // 获取目标状态 (位于 "->" 之后)
    string target = t.substr(arrowPos + 2);

    return {inputChar, target};
}


int getGroupId(const string& state, const vector<vector<string>>& groups)
{
    for (int i = 0; i < groups.size(); ++i)
    {
        for (const auto& s : groups[i])
        {
            if (s == state) return i;
        }
    }
    return -1; // Should not happen
}

bool isFinal(const string& s)
{
    return s == "Y" || finalStates.count(s);
}


bool stateComparator(const string& a, const string& b)
{
    if (a == "X") return true;
    if (b == "X") return false;
    if (a == "Y") return true;
    if (b == "Y") return false;
    try
    {
        int na = stoi(a);
        int nb = stoi(b);
        return na < nb;
    }
    catch (...)
    {
        return a < b;
    }
}

int main()
{
    string line;
    while (getline(cin, line) && !line.empty())
    {
        stringstream ss(line);
        string srcState, token;
        ss >> srcState;

        allStates.insert(srcState);
        if (srcState == "Y") finalStates.insert(srcState);

        while (ss >> token)
        {
            pair<char, string> trans = parseTransition(token);
            adj[srcState][trans.first] = trans.second;
            alphabet.insert(trans.first);
            allStates.insert(trans.second);
            if (trans.second == "Y") finalStates.insert("Y");
        }
    }

    // 2. 初始化分组：终止状态组 和 非终止状态组
    vector<vector<string>> splitStates;
    vector<string> groupFinal, groupNonFinal;

    for (const auto& s : allStates)
    {
        if (isFinal(s))
        {
            groupFinal.push_back(s);
        }
        else
        {
            groupNonFinal.push_back(s);
        }
    }

    if (!groupFinal.empty()) splitStates.push_back(groupFinal);
    if (!groupNonFinal.empty()) splitStates.push_back(groupNonFinal);

    // 3. 循环遍历分割状态集合
    while (true)
    {
        vector<vector<string>> newSplitStates;

        map<string, int> stateToGroupId;
        for (int i = 0; i < splitStates.size(); ++i)
        {
            for (const auto& s : splitStates[i])
            {
                stateToGroupId[s] = i;
            }
        }

        // 遍历当前的每一个分组
        for (const auto& group : splitStates)
        {
            if (group.size() <= 1)
            {
                newSplitStates.push_back(group);
                continue;
            }

            // Map<特征, 状态集合>
            map<vector<int>, vector<string>> aimStateTypeList;

            for (const auto& state : group)
            {
                vector<int> signature;
                for (char c : alphabet)
                {
                    string target = adj[state][c];
                    signature.push_back(stateToGroupId[target]);
                }
                aimStateTypeList[signature].push_back(state);
            }

            for (auto const& dst : aimStateTypeList)
            {
                auto key = dst.first;
                auto subGroup = dst.second;
                newSplitStates.push_back(subGroup);
            }
        }

        if (newSplitStates.size() == splitStates.size())
        {
            break;
        }
        splitStates = newSplitStates;
    }

    // 4. 构建输出结果
    struct OutputLine
    {
        string src;
        vector<string> transitions;
    };
    vector<OutputLine> outputLines;

    for (const auto& group : splitStates)
    {
        string representative = group[0];
        for (const auto& s : group)
        {
            if (s == "X")
            {
                representative = s;
                break;
            }
            if (s == "Y" && representative != "X") { representative = s; }
        }

        OutputLine line;
        line.src = representative;
        for (char c : alphabet)
        {
            string rawTarget = adj[representative][c];
            string targetRep = "";

            // 找到 rawTarget 所在的组，并获取该组的代表
            for (const auto& g : splitStates)
            {
                bool found = false;
                for (const auto& s : g)
                {
                    if (s == rawTarget)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    string gRep = g[0];
                    for (const auto& s : g)
                    {
                        if (s == "X")
                        {
                            gRep = s;
                            break;
                        }
                        if (s == "Y" && gRep != "X") { gRep = s; }
                    }
                    targetRep = gRep;
                    break;
                }
            }

            // 格式: X-a->0
            string transStr = representative + "-" + c + "->" + targetRep;
            if (!targetRep.empty()) line.transitions.push_back(transStr);
        }
        outputLines.push_back(line);
    }

    // 对输出行进行排序，使其符合样例顺序 (X, Y, 0, 1...)
    sort(outputLines.begin(), outputLines.end(), [](const OutputLine& a, const OutputLine& b)
    {
        return stateComparator(a.src, b.src);
    });

    for (const auto& line : outputLines)
    {
        cout << line.src;
        for (const auto& t : line.transitions)
        {
            cout << " " << t;
        }
        cout << endl;
    }

    return 0;
}
