这份教程旨在带你从零理解并手写一个 **B+树（B+ Tree）** 的核心实现。

我们将避开晦涩的数学证明，专注于**工程实现**和**数据结构的行为**。

---

# 教程：手写数据库引擎核心 —— C++ B+树

## 第一部分：为什么要发明 B+树？

在看代码之前，你必须明白它解决了什么问题。

### 1. 磁盘 IO 是昂贵的
普通的 **二叉搜索树 (BST)** 或 **红黑树 (RB-Tree)**，节点是“瘦高”的。
*   每个节点只存 1 个数据。
*   查找 100万个数据，树高约为 20 层。
*   **致命伤**：如果节点存储在磁盘上，访问 20 层意味着 20 次磁盘读取（I/O）。数据库会慢得像蜗牛。

### 2. B+树的哲学：矮胖
B+树的核心思想是：**把树压扁**。
*   我们让一个节点变得很大（比如 4KB，刚好一页磁盘），里面能塞几百个 Key。
*   这样，树的高度通常只有 3 到 4 层。
*   查找只需 3-4 次 I/O。

### 3. B+树 vs B树（重要区别）
*   **B树**：数据散落在树的各个节点里。
*   **B+树**：
    *   **所有数据都在叶子节点**（最底层）。
    *   **上面几层全是索引**（路标），只负责指路。
    *   **叶子节点连成链表**：这点对数据库至关重要，因为它可以极其快速地进行**范围查询**（比如 `SELECT * FROM table WHERE id > 100`）。

---

## 第二部分：定义规则 (The Blueprint)

为了方便教学，我们定义一个 **3阶 B+树 (M=3)**。
这意味着树非常“容易满”，方便我们观察**分裂**过程。

**规则如下：**
1.  **节点容量**：每个节点最多有 `M` 个孩子（即最多 `M-1` 个 Key）。
    *   在本教程中，节点最多存 **2个 Key**。存第 **3** 个时，它会爆炸（分裂）。
2.  **叶子分裂 (Leaf Split)**：
    *   中间的 Key **复制 (Copy)** 一份送给父亲。
    *   原数据保留在右边叶子中（因为叶子必须包含所有数据）。
3.  **内部节点分裂 (Internal Split)**：
    *   中间的 Key **上交 (Push)** 给父亲。
    *   原数据从当前层消失（因为它们只是路标，一旦交给了上级，下级就不需要留存了）。

---

## 第三部分：代码拆解实现

我们将代码拆分为三个核心模块：**结构定义**、**查找/遍历**、**插入/分裂**。

### 1. 结构定义 (The Struct)

我们不需要复杂的模板，直接用 `int` 做 Key。

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
using namespace std;

// 阶数 M=3 (最多2个key，插第3个分裂)
const int M = 3;

struct Node {
    bool isLeaf;             // 身份标记：是叶子还是内部节点？
    vector<int> keys;        // 存 Key 的数组
    vector<Node*> children;  // 存子节点指针的数组
    Node* next;              // 【B+树特有】叶子节点的链表指针

    Node(bool leaf) : isLeaf(leaf), next(nullptr) {}
};
```

### 2. 核心难点：分裂逻辑 (The Splitting Logic)

这是整个 B+树最难写的部分。当一个节点满了（有了 3 个 Key），我们需要把它切成两半，并处理中间那个 Key。

**请仔细阅读注释中的 `Copy Up` 和 `Push Up` 的区别。**

```cpp
// parent: 父节点
// index: 满掉的那个子节点在父节点中的位置
// fullChild: 满掉的那个子节点
void splitChild(Node* parent, int index, Node* fullChild) {
    // 1. 创建新节点（用来存分裂后的右半部分）
    // 新节点的类型（叶子/内部）必须和满节点一致
    Node* newChild = new Node(fullChild->isLeaf);

    // 2. 找到切分点 (M=3, keys=[A, B, C], mid=1 即 B)
    int midIndex = M / 2; 

    // === 分支 A：内部节点分裂 (路标升级) ===
    if (!fullChild->isLeaf) {
        // 【关键】中间的 Key (mid) 要被“提拔”上去，不留在下面
        
        // 把 mid 右边的 key 搬给新节点
        newChild->keys.assign(fullChild->keys.begin() + midIndex + 1, fullChild->keys.end());
        // 把 mid 右边的孩子 搬给新节点 (注意孩子比Key多1个)
        newChild->children.assign(fullChild->children.begin() + midIndex + 1, fullChild->children.end());

        // 记录要提拔的 Key
        int upKey = fullChild->keys[midIndex];
        
        // 将原节点截断（丢弃 mid 及其右边）
        fullChild->keys.resize(midIndex);
        fullChild->children.resize(midIndex + 1);

        // 将 upKey 插到父节点里
        parent->keys.insert(parent->keys.begin() + index, upKey);
    } 
    // === 分支 B：叶子节点分裂 (数据分家) ===
    else {
        // 【关键】中间的 Key (mid) 要“复制”一份上去，自己还要留在右边
        
        // 把 mid 开始的所有 key 搬给新节点 (包括 mid 自己!)
        newChild->keys.assign(fullChild->keys.begin() + midIndex, fullChild->keys.end());
        
        // 记录要复制上去的 Key
        int upKey = fullChild->keys[midIndex];

        // 原节点只保留 mid 左边的
        fullChild->keys.resize(midIndex);

        // 【链表连接】 fullChild -> newChild -> oldNext
        newChild->next = fullChild->next;
        fullChild->next = newChild;

        // 将 upKey 插到父节点里
        parent->keys.insert(parent->keys.begin() + index, upKey);
    }

    // 3. 无论哪种分裂，新产生的节点(newChild)都要挂到父节点名下
    parent->children.insert(parent->children.begin() + index + 1, newChild);
}
```

### 3. 递归插入 (Recursive Insert)

B+树的插入是从根向下找，找到叶子插入。如果叶子满了，再“炸”开，把问题向上传递。

```cpp
void insertRecursive(Node* node, int key) {
    // === 这里的逻辑是：先向下找，直到找到叶子 ===

    // 情况1: 当前是叶子，直接插
    if (node->isLeaf) {
        // 找个合适的位置插进去（保持有序）
        auto it = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        node->keys.insert(it, key);
        return;
    }

    // 情况2: 当前是内部节点，找路
    // 比如 keys=[10, 20]，插入 15。
    // 15 > 10 但 < 20，所以走 index=1 的那个孩子
    int i = 0;
    while (i < node->keys.size() && key > node->keys[i]) {
        i++;
    }
    
    // 递归向下
    insertRecursive(node->children[i], key);

    // === 回溯阶段 (递归函数返回后) ===
    // 检查刚才那个孩子有没有吃撑 (Key数量达到 M)
    if (node->children[i]->keys.size() == M) {
        // 孩子满了，帮孩子分裂
        splitChild(node, i, node->children[i]);
    }
}
```

### 4. 树的入口 (The Wrapper)

我们需要处理一种特殊情况：**根节点自己满了**。根节点没有父节点，所以如果根满了，树的高度就会 +1。

```cpp
class BPlusTree {
    Node* root;
public:
    BPlusTree() { root = new Node(true); } // 初始根是空的叶子

    void insert(int key) {
        insertRecursive(root, key);

        // 特殊检查：根节点是不是爆了？
        if (root->keys.size() == M) {
            Node* newRoot = new Node(false); // 新根肯定是内部节点
            newRoot->children.push_back(root); // 旧根变成新根的大儿子
            splitChild(newRoot, 0, root);      // 分裂旧根
            root = newRoot;                    // 更新指针
        }
    }
    
    // ... 这里可以加上 print() 函数，参考上一份回答 ...
};
```

---

## 第四部分：推演剧本 (Visualization)

现在，我们在脑海中（或纸上）运行这个程序。
**设定：Order M = 3**（每个节点最多存 2 个数，存第 3 个时分裂）。

### 场景一：根的初始生长
1.  **Insert 10**: `[10]`
2.  **Insert 20**: `[10, 20]` (根满了，但还没炸，等待下一次触发)

### 场景二：第一次裂变 (叶子分裂)
3.  **Insert 30**:
    *   叶子变成 `[10, 20, 30]` -> **BOOM!**
    *   **动作**：Leaf Split (Copy Up)。
    *   中位数 `20` 复制一份去上面。
    *   **结果**：
        ```text
          [20]       <-- 新根
         /    \
    [10] -> [20, 30] <-- 叶子链表
    ```

### 场景三：右侧填满
4.  **Insert 40**:
    *   找到右边叶子 `[20, 30]`，变成 `[20, 30, 40]` -> **BOOM!**
    *   **动作**：Leaf Split (Copy Up)。
    *   中位数 `30` 复制一份去上面。
    *   父节点 `[20]` 收到 `30`，变成 `[20, 30]` (满了，待命)。
    *   **结果**：
        ```text
            [20, 30]
           /   |    \
        [10]->[20]->[30, 40]
        ```

### 场景四：终极裂变 (树高增高)
5.  **Insert 50**:
    *   最右叶子 `[30, 40]` 变成 `[30, 40, 50]` -> **BOOM!**
    *   中位数 `40` Copy Up。
    *   父节点 `[20, 30]` 收到 `40`，变成 `[20, 30, 40]` -> **BOOM!** (内部节点满了)。
    *   **动作**：Internal Split (Push Up)。
    *   中位数 `30` **上移** (注意：不是复制，是提走)。
    *   **结果**：
        ```text
             [30]         <-- 新的根 (Level 2)
            /    \
         [20]    [40]     <-- 内部节点 (Level 1)
         /  \    /  \
    [10]->[20]->[30]->[40, 50] <-- 叶子 (Level 0)
    ```
    *   **注意看**：Level 1 的节点里没有 `30` 了，因为 `30` 已经去 Level 2 当大领导了。而 Level 0 必须保留 `30`，因为那里是数据仓库。

---

## 第五部分：学习建议

1.  **不要背代码**：背诵 `splitChild` 的索引操作是没有意义的。
2.  **画图**：拿纸和笔，画出 M=3 时插入 1 到 10 的过程。如果不画图，你永远会被 `mid` 和 `mid+1` 搞晕。
3.  **调试**：将上面的代码复制到 IDE 中，在 `splitChild` 函数里打断点。观察 `fullChild` 的 keys 是怎么变少，`newChild` 是怎么把 keys 接过去的。
4.  **扩展思考**：
    *   现在的 `insert` 是单纯的插，如果我想支持 `Range Query` (例如找 15 到 35 之间的数)，利用 `next` 指针该怎么写？(提示：先找到 >=15 的第一个叶子，然后顺着链表往后跑，直到 >35)。

这才是 B+树的精髓：**稳定的索引结构** + **高速的链表扫描**。