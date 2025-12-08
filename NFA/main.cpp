#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include <algorithm>
#include <queue>
#include <set>

using namespace std;

class NFANode
{
public:
    int stateNum;
    char pathChar;
    vector<NFANode> nextNodes;
};


class NFA
{
public:
    NFANode headNode;
    NFANode tailNode;
};
