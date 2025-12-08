实验内容：
```markdown
【问题描述】
基于DFA的单词识别问题的一种描述是：编写一个程序，输入一个确定的有穷自动机（DFA），使用该DFA识别单词。

【基本要求】设置DFA初始状态X，终态Y，过程态用数字表示：0 1 2 3………
【样例输入】
a b#
X Y 0 2#
X X-a->0 X-b->X
Y Y-a->0 Y-b->X
0 0-a->0 0-b->2
2 2-a->0 2-b->Y

abb#
ba#
aca#

【样例输出】
a
b
b
pass
b
a
error
a
error


【样例说明】符号”~“表示空串
```
