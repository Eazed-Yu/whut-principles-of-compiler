#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>

using namespace std;

// === 配置区 ===
// 为了演示分裂，我们设为3阶树。
// 意味着：一个节点最多2个Key，最多3个孩子。插第3个Key时爆炸。
const int M = 3;

// === 节点定义 ===
struct Node {
    bool isLeaf;
    vector<int> keys;       // 存储 Key
    vector<Node*> children; // 内部节点存储子节点指针
    Node* next;             // 叶子节点的链表指针

    explicit Node(const bool leaf) : isLeaf(leaf), next(nullptr) {}
};

// === B+ 树类 ===
class BPlusTree {
    Node* root;

public:
    BPlusTree() {
        root = new Node(true); // 初始根是叶子
    }

    // 对外接口：插入
    void insert(const int key) {
        // 递归插入，如果有分裂，会返回一个新的兄弟节点和提升上来的key
        // 这里的逻辑稍微做了一点变通：我们先找到叶子插进去，如果满了再自底向上处理
        insertRecursive(root, key);

        // 检查根节点是否因为分裂变得太大了（这是一种简化的检查方式）
        // 标准写法应该是在递归返回时处理，但为了代码可读性，
        // 我们在递归内部处理了分裂，除了根节点的特殊情况。
        if (root->keys.size() == M) {
            Node* newRoot = new Node(false);
            newRoot->children.push_back(root);
            splitChild(newRoot, 0, root);
            root = newRoot;
        }
    }

    // 对外接口：打印树（层级遍历）
    void print() {
        if (!root) return;
        cout << "\n=== Current B+ Tree Structure ===\n";
        queue<Node*> q;
        q.push(root);
        int level = 0;

        while (!q.empty()) {
            int size = q.size();
            cout << "Level " << level++ << ": ";
            while (size--) {
                Node* curr = q.front(); q.pop();
                cout << "[";
                for (int i = 0; i < curr->keys.size(); i++) {
                    cout << curr->keys[i] << (i < curr->keys.size()-1 ? "|" : "");
                }
                cout << "] ";

                if (!curr->isLeaf) {
                    for (auto child : curr->children) q.push(child);
                }
            }
            cout << endl;
        }

        // 打印叶子链表
        cout << "Leaf List: ";
        Node* curr = root;
        while (!curr->isLeaf) curr = curr->children[0]; // 找最左叶子
        while (curr) {
            cout << "[";
            for(int k : curr->keys) cout << k << " ";
            cout << "] -> ";
            curr = curr->next;
        }
        cout << "NULL\n";
    }

private:
    // 递归查找并插入
    void insertRecursive(Node* node, int key) {
        // 1. 如果是叶子节点，直接找位置插入
        if (node->isLeaf) {
            // 找到第一个大于 key 的位置
            auto it = std::upper_bound(node->keys.begin(), node->keys.end(), key);
            // 实际上 B+ 树不应该有重复 Key，这里简化，假设不重复
            node->keys.insert(it, key);
            return;
        }

        // 2. 如果是内部节点，找到子节点递归下去
        // 找到第一个大于 key 的 key 的索引，对应的就是子节点索引
        int i = 0;
        while (i < node->keys.size() && key > node->keys[i]) {
            i++;
        }

        // 递归进入子节点
        insertRecursive(node->children[i], key);

        // 3. 回溯阶段：检查子节点是否满了
        if (node->children[i]->keys.size() == M) {
            splitChild(node, i, node->children[i]);
        }
    }

    // 核心逻辑：分裂节点
    // parent: 父节点
    // index: fullChild 在 parent 的 children 中的下标
    // fullChild: 满出来的那个节点
    void splitChild(Node* parent, int index, Node* fullChild) {
        // 创建新节点（分裂出的右半部分）
        Node* newChild = new Node(fullChild->isLeaf);

        // 计算分裂点：中间位置
        // M=3, size=3, mid=1. Keys: [0, 1, 2] -> mid key is [1]
        int midIndex = M / 2;

        // --- 情况 A: 内部节点分裂 (Push Up) ---
        // 中间的 key 上移到父节点，不会保留在左右孩子中
        if (!fullChild->isLeaf) {
            // 把 mid 之后的部分给新节点
            // fullChild: [A, B, C] -> mid=B. 左:[A], 右:[C]. B上移

            // 搬运 Key
            newChild->keys.assign(fullChild->keys.begin() + midIndex + 1, fullChild->keys.end());
            // 搬运 Children (注意：内部节点孩子数 = Key数 + 1，所以要搬运对应数量的孩子)
            newChild->children.assign(fullChild->children.begin() + midIndex + 1, fullChild->children.end());

            // 提升的 Key
            int upKey = fullChild->keys[midIndex];

            // 调整原节点大小
            fullChild->keys.resize(midIndex);
            fullChild->children.resize(midIndex + 1);

            // 将 upKey 插入父节点
            parent->keys.insert(parent->keys.begin() + index, upKey);
            // 将 newChild 链接到父节点
            parent->children.insert(parent->children.begin() + index + 1, newChild);
        }

        // --- 情况 B: 叶子节点分裂 (Copy Up) ---
        // 中间的 key 也要上移（作为索引），但必须保留在右边的叶子里（作为数据）
        else {
            // fullChild: [1, 5, 8] -> mid=5. 左:[1], 右:[5, 8]. 5 复制一份上移

            // 搬运 Key (从 mid 开始全部搬走，包括 mid 自己)
            newChild->keys.assign(fullChild->keys.begin() + midIndex, fullChild->keys.end());

            // 提升的 Key (Copy)
            int upKey = fullChild->keys[midIndex];

            // 调整原节点大小
            fullChild->keys.resize(midIndex);

            // 维护叶子链表: fullChild -> newChild -> oldNext
            newChild->next = fullChild->next;
            fullChild->next = newChild;

            // 将 upKey 插入父节点
            parent->keys.insert(parent->keys.begin() + index, upKey);
            // 将 newChild 链接到父节点
            parent->children.insert(parent->children.begin() + index + 1, newChild);
        }
    }
};

int main() {
    BPlusTree bt;

    // 演示序列：精心设计的顺序以触发不同类型的分裂
    // M = 3 (每个节点最多存 2 个 Key，插第 3 个时分裂)

    cout << "Insert 5, 10, 15 (Leaf full -> Split)\n";
    bt.insert(5);
    bt.insert(10);
    bt.insert(15);
    // 此时叶子 [5, 10, 15] 满，分裂。10 被 Copy Up。
    // 结果: Root:[10], Left:[5], Right:[10, 15]
    bt.print();

    cout << "\nInsert 20 (Simple insert into right leaf)\n";
    bt.insert(20);
    // 右叶子 [10, 15, 20] 满，但还没处理，等下一次分裂
    bt.print();

    cout << "\nInsert 25 (Cascading Split)\n";
    bt.insert(25);
    // 1. 右叶子 [10, 15, 20, 25] -> 炸裂。
    // 2. 中位数 20 Copy Up 到根。
    // 3. 根节点原本是 [10]，现在变成 [10, 20]。
    // 4. 根节点变成满状态 [10, 20] (size=2, OK, max is 2 keys)
    bt.print();

    cout << "\nInsert 30 (Root Split -> Tree grows height)\n";
    bt.insert(30);
    // 1. 最右叶子 [20, 25, 30] 炸裂 -> 25 Copy Up。
    // 2. 根节点 [10, 20] 收到 25 -> 变成 [10, 20, 25]。
    // 3. 根节点炸裂 (Internal Split) -> 中位数 20 Push Up。
    // 4. 新根产生 [20]。
    bt.print();

    return 0;
}