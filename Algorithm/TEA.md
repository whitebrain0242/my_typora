# TEA





## 滑动窗口与双指针

### 一、定长滑动窗口

**入-更新-出**



### 二、不定长滑动窗口

利用哈希表计数

判断有没有重复

  重复的话就除去左窗口，左指针加一 

判断大小，ans和表现在的长度

## 二分算法

### 二分查找

一般会给递增或者递减的数组----

**大概思路如下**

先定义一个 【返回最小的满足>=target的下标i】的函数，这个i就是符合条件的数字第一次出现的下标

再判定这个【下标有没有超过数组长度】 或者【这个下标和目标值一样不】为的就是判定数组里面有没有符合条件的数字

最后一步，有开头就有结尾，再次使用那个函数，寻找【返回最小的满足>=target+1的下标i】-1之后就是结尾的下标

返回两个下标的数组



**然后说一下这个函数**

这里说明的是开区间写法

定义

```c
 int left = -1;
 int right = nums.length; // 开区间 (left, right)
```

判定循环条件`left + 1 < right`【区间不为空】

定义中间值mid

mid和target比大小，更新left和right

循环结束后 【left+1 = right】
        此时 nums[left] < target 而 nums[right] >= target
        所以 right 就是第一个 >= target 的元素下标

### 二分答案

我们先要知道向上取整公式

x/m向上取整：

`result=(x + m - 1) / m`

**要找什么就对什么二分**

1. 首先初始化二分边界为0，右边界设置为数组最大元素本身（可以由题改变
2. 循环条件left+1<right ，定义mid，判断mid是否符合题目条件调用函数
3. 符合找更小的，right=mid，不符合找更大的，left=mid
4. 返回right
5. 另外一个判断函数：求和，向上取整布尔类型

## 数据结构

### 枚举

#### ⚡**枚举右，维护左**

⭐这是**解决「找两个元素最优组合（后 - 前最大差值）」的顶级贪心思路**，专门替代**暴力枚举所有左右组合**（太慢）

1. **枚举右**：固定**右边的元素 / 行**（后出现的那个），一个一个往后遍历，不回头；
2. **维护左**：只记录**左边所有元素 / 行**（先出现的那些）的**关键最优信息**（比如最小值、最大值），不保存所有左边数据，不重复遍历左边；
3. **算答案**：每拿到一个新的「右」，直接用「维护好的左」计算最优解，一步到位。

举例：在数组中找到两个数使其和为目标值，返回下标

暴力做法：双重循环遍历

思路：利用哈希表查找数据只用o(1)时间来看，我们可以让j从0开始遍历，寻找j左侧有没有`target-nums[j]`,并且每次遍历一个数字就记录它的键和下标也就是键值嗯对~~~

例如：

```c
class Solution {
    public int[] twoSum(int[] nums, int target) {
        Map<Integer, Integer>index=new HashMap<>();
        for(int i=0;;i++){
             //遍历就是枚举右
            int x=nums[i];
            if(index.containsKey(target-x)){
                 //用右+左计算
                return new int[]{index.get(target-x),i};
            }
            index.put(x,i);//hashmap记录就是维护左
        }
    }
}
```

```c
#define MIN(a, b) ((b) < (a) ? (b) : (a))
#define MAX(a, b) ((b) > (a) ? (b) : (a))

int maxDistance(int** arrays, int arraysSize, int* arraysColSize) {
    int ans = 0;
    int mn = INT_MAX / 2, mx = INT_MIN / 2; // 防止减法溢出
    for (int i = 0; i < arraysSize; i++) {
         //一个一个遍历就是枚举右
        int x = arrays[i][0], y = arrays[i][arraysColSize[i] - 1];
         //核心计算，用枚举的当前xy+维护的mnmx算最大值
        ans = MAX(ans, MAX(y - mn, mx - x));
         //维护左：把当前右并入左边，更新左边最小值最大值
        mn = MIN(mn, x);
        mx = MAX(mx, y);
    }
    return ans;
}
```

##### 做题的总结

首先就是枚举右，判断是否需要索引来选择两种for循环

其次就是记录用不用哈希表，有没有明确的范围？有范围大概率可以使用数组，或者说HashSet，需要自己判断

最后，维护左

#### ⚡枚举中间

### 前缀和

```c
class NumArray {
    private final int []s;
    public NumArray(int[] nums) {//大概就是创建一个前缀和数组
        //前缀和数组是这样的，s【1】是nums【0】的和，s【2】是nums【1】+s【1】的和，依此类推
        s=new int [nums.length+1];
        for(int i=0;i<nums.length;i++){
            s[i+1]=s[i]+nums[i];
        }
    }
    public int sumRange(int left, int right) {//求两个范围之间的和
        return s[right+1]-s[left];//right+1是所有和的和减去left和
    }
}
```

**懂你意思：**

1.创建前缀和数组，前缀和数组是这样的，s【1】是nums【0】的和，s【2】是nums【1】+s【1】的和，依此类推，所以

`s[i+1]=s[i]+nums[i];`

2.s【i】数组表示的是前i个数的总和

3.要求指定范围的和，例如left-right，那么就是前right的和-前left-1的和，正好就是`s[right+1]-s[left];`

### 差分

差分数组是处理 **「区间加 / 减」+「单点查询」** 问题的神器

#### 了解差分数组是什么

定义：用于将**区间修改**转化为**单点修改**，从而将时间复杂度从 O(n) 降为 O(1)。

差分数组可以记录每一个点累计**改变次数*改变量**，然后求原数组就像前缀和一样加起来，求每一个值就和改变值做运算

#### ⭐性质

1. 差分数组的前缀和数组就是原数组

2. 原数组a的【L，R】内数据加上x:

   **a[L]+x,a[L+1]+x······a[R]+x**==**d[L]+x,d[R+1]-x**

   

#### 懂你意思

第一步，创建差分数组，记录每一个点的值

>  ```
>  //那么坐标范围是【0，max】，假设最【1，max】操作，那么就要用到【max+1】-x
>  //那么数组下标是max【0，max+1】，所以数组大小是max+2
>  ```

第二部，赋值，一个范围内，【start】+x，【end+1】-x

第三步，还原原数组，求差分数组的前缀和，和题目连接起来，

### 栈

ArrayList

StringBuilder

 **`ArrayDeque`**

Stack

### 队列

LinkedList

### 堆

面试等用原地堆化！时间复杂度更低

![image-20260425160319699](TEA.assets/image-20260425160319699.png)

下沉sink：

每次选择左右儿子中更大的交换位置，直到左右儿子都小于自己





算法题的话就用优先队列

优先队列

```c
PriorityQueue<Integer> pq=new PriorityQueue<>(Collections.reverseOrder());
```

### 字典树Tire

字典树很简单：创建Tire节点，孩子可以用HashMap或者Tire【】数组来定义，经常用于找前缀之类的，通用的其实是字典树的构建

1.遍历每一个单词，外循环，从根开始-cur

2.遍历单词的每一个字母，判断cur的孩子是null吗是null表示没有此节点，可以创建一个，然后再让cur移动到此孩子节点上找下一个就好

P648



```c
public class Solution {
    public String replaceWords(List<String> dictionary, String sentence) {
        Tire tire=new Tire();
        for(String word:dictionary){
            Tire cur=tire;
            for(int i=0;i<word.length();i++){
                char c=word.charAt(i);
                cur.children.putIfAbsent(c,new Tire());
                cur=cur.children.get(c);
            }
            cur.children.put('#',new Tire());
        }
        String[]words=sentence.split(" ");
        for(int i=0;i<words.length;i++){
            words[i]=findRoot(words[i],tire);
        }
        return String.join(" ",words);
    }
    //查最短词根：参数：单词，字典树
    public String findRoot(String word,Tire tire){
        StringBuilder root=new StringBuilder();
        Tire cur=tire;
        for(int i=0;i<word.length();i++){
            char c=word.charAt(i);
            if(cur.children.containsKey('#'))return root.toString();
            if(!cur.children.containsKey(c))return word;
            root.append(c);
            cur=cur.children.get(c);
        }
        return root.toString();
    }
}
class Tire {
    Map<Character,Tire>children ;

    public Tire() {
        children =new HashMap<>();
    }
}
```

### 并查集

```c
// 模板来源 https://leetcode.cn/circle/discuss/mOr1u6/
class UnionFind {
    private final int[] fa; // 代表根节点
    private final int[] size; // 集合大小
    public int cc; // 连通块个数

    UnionFind(int n) {
        // 一开始有 n 个集合 {0}, {1}, ..., {n-1}
        // 集合 i 的代表元是自己，大小为 1
        fa = new int[n];
        for (int i = 0; i < n; i++) {
            fa[i] = i;
        }
        size = new int[n];
        Arrays.fill(size, 1);
        cc = n;
    }

    // 返回 x 所在集合的代表元
    // 同时做路径压缩，也就是把 x 所在集合中的所有元素的 fa 都改成代表元
    public int find(int x) {
        // 如果 fa[x] == x，则表示 x 是代表根
        if (fa[x] != x) {
            fa[x] = find(fa[x]); // fa 改成根节点
        }
        return fa[x];
    }

    // 判断 x 和 y 是否在同一个集合
    public boolean isSame(int x, int y) {
        // 如果 x 的代表元和 y 的代表元相同，那么 x 和 y 就在同一个集合
        // 这就是代表元的作用：用来快速判断两个元素是否在同一个集合
        return find(x) == find(y);
    }

    // 把 from 所在集合合并到 to 所在集合中
    // 返回是否合并成功
    public boolean merge(int from, int to) {
        int x = find(from);
        int y = find(to);
        if (x == y) { // from 和 to 在同一个集合，不做合并
            return false;
        }
        fa[x] = y; // 合并集合。修改后就可以认为 from 和 to 在同一个集合了
        size[y] += size[x]; // 更新集合大小（注意集合大小保存在代表元上）
        // 无需更新 size[x]，因为我们不用 size[x] 而是用 size[find(x)] 获取集合大小，但 find(x) == y，我们不会再访问 size[x]
        cc--; // 成功合并，连通块个数减一
        return true;
    }

    // 返回 x 所在集合的大小
    public int getSize(int x) {
        return size[find(x)]; // 集合大小保存在代表元上
    }
}

```

## 链表 二叉树与回溯

### 链表

-   **递归**像**“俄罗斯套娃”**：你必须先打开最外面的大娃（当前节点），但因为你不知道里面还剩几个，你只能一直拆到最里面（递归边界），然后再从最里面一层层把结果组装出来套回去。

递归一般就是

1.   先判断边界条件，什么时候会到底层
2.   再判断每一层都要进行的核心操作
3.   最后把结果返回给上一层

-   **迭代**像**“贪吃蛇”**：你一口一口吃掉 l1 和 l2 的节点，每吃一口就吐出一个新节点接在尾巴上，目标明确，一路走到黑。

#### 遍历链表

#### 删除节点

##### 一般

​     大致思路就是说找到目标节点的前一个节点然后让前一个节点指向当前节点的下一个节点（c++再删除这个节点就好了）

##### 给出要删除的节点P237

​     这时候直接把下一个节点的val值复制到现在这个节点的val里面，再删除下一个节点hhh

##### 删除链表的倒数第 N 个结点P19

​     1.先遍历一边链表，求出链表长度，然后就知道倒数第n个是正数第几个了，然后在遍历一边链表，找到要删除节点的前一个，操作如上

​     2.[快慢指针解法](#删除链表的倒数第n个节点P19)

#####  删除排序链表中的重复元素P83

​     首先，这道题不用创造哨兵节点，因为就算是头节点和下一个节点的数值一样，只用删掉下一个节点就好了

​     从头节点开始遍历cur,如果cur的下一个节点的值和cur相同的话，那么就删除下一个节点，反之，cur移动到下一个节点的地方

##### 删除排序链表中的重复元素 IIP82

​    因为这个重复节点都要删除，所以说需要哨兵节点

​    我们从烧饼节点开始判断，设定next1是下一个节点，next2是下下一个节点，这个时候比较next1和next2,如果相同的话就while删除节点（删除节点），如果不一样的话就后移一个节点



#### 插入节点

#### 反转链表

两种方法

##### 递归(尾插法)

![image-20260527165716804](/home/white/.config/Typora/typora-user-images/image-20260527165716804.png)

思路：递归到末尾节点，作为新链表的头节点，归的时候再把节点插在新链表的末尾

```
if(head==nullptr||head->next==nullptr)return head;
auto rev_head=reverseList(head->next);


//head指针不变，而是把下一个节点传进函数里面，递归反转后面的链表，把最后的反转链表的头节点存在rev_head
//下面应该就是写反转链表是怎么实现的
//从前往后改变


ListNode*tail=head->next;
tail->next=head;
head->next=nullptr;
return rev_head;
```



##### 迭代（头插法

![image-20260527170256733](/home/white/.config/Typora/typora-user-images/image-20260527170256733.png)

对于链表 1→2→3，结合代码来说，顺序为：

第一轮循环结束后，得到链表 1。
第二轮循环结束后，得到链表 2→1。
第三轮循环结束后，得到链表 3→2→1。

```
ListNode*pre=nullptr;
ListNode*cur=head;
while(cur){
    ListNode*nxt=cur->next;//保存一下，防止丢失
    //反转
    cur->next=pre;
    pre=cur;
    cur=nxt;
}
return pre;
```

##### 某一段反转

left-right

创建一个哨兵节点方便一点dummy

找到left前一个节点

然后这一段内反转

最后看图

![image-20260530151351025](/home/white/.config/Typora/typora-user-images/image-20260530151351025.png)





<img src="https://pic.leetcode.cn/1769394801-rMZOrC-%E6%B5%81%E7%A8%8B%E5%9B%BE.png" alt="流程图.png" style="zoom: 25%;" />

```
ListNode dummy(0, head);
ListNode* p0 = &dummy;

for (int i = 0; i < left - 1; i++) {
     p0 = p0->next;
}
ListNode* pre = nullptr;
ListNode* cur = p0->next;
for (int i = 0; i < right - left + 1; i++) {
    ListNode* nxt = cur->next;
    cur->next = pre; // 每次循环只修改一个 next，方便大家理解
    pre = cur;
    cur = nxt;
}
p0->next->next = cur;
p0->next = pre;
return dummy.next;

```

#### 快慢指针

##### 删除链表的倒数第n个节点P19

​      首先判断需不需要创建哨兵节点：如果n=链表长度，也就是删除头节点的话，就需要哨兵节点

​      同理，此题需要哨兵节点，先初始化左右指针都指向`dummynode`，让right先走n步，然后左右指针一起走，等到right指针遍历到最后一个的时候，此时left指针指向的就是要删除节点的前一个节点，此时正常进行删除操作就好了

​      话说怎么理解呢？大概就是说，现在的目标是寻找倒数第n个节点，想象有一把长度刚好为n+1的尺子，当尺子右端在最后一个节点的时候，那么左端就是前一个节点

​     时间复杂度是O(m),m是链表长度，两个循环加起来的长度就是m

​     空间复杂读是O(1),只用到一些额外变量

##### 链表的中间结点P876

​     寻找林链表的中间节点，可以使用快慢指针

​     设定一个快指针一个为慢指针，快指针一次走两步，慢指针一次走1步，这样当快指针为空或者快指针的下一个为空的时候，slow就是中间节点

​     怎么理解呢？快指针的速度是慢指针的两倍，所以路程也就是两倍，如果是奇数个节点的话，fast走到null的时候slow就在中间了，f偶数个节点的话，fast的下一个为空的时候slow就在中间了![image-20260530153443443](/home/white/.config/Typora/typora-user-images/image-20260530153443443.png)

![image-20260530153605452](/home/white/.config/Typora/typora-user-images/image-20260530153605452.png)

​      所以while循环需要判断fast和fats的next是否存在

​      时间复杂读是O(n),空间复杂度是O(1)

##### 环形链表P141

​      如果链表里面有环的话，那么我们可以想象成追击问题，快指针总会追上慢指针，所以再加一个判断，如果slow=fast,那么就是环形链表

##### 环形链表P142

**Floyd 判圈算法**

<img src="file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-05/Ori/a931a1ddc5c8f3723329fd7a3853b64b.jpg" alt="img" style="zoom: 33%;" />

​      结论：当快慢指针相遇的时候，慢指针还没有走完一整圈

<img src="file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-05/Ori/895ec61437687a97d2832b6f98e92ba2.jpg" alt="img" style="zoom:33%;" />

​       相遇之后，fast和slow都在相遇点，继续走，slow走c步到达入口，head从头节点出发，走了c步之后，还有整数个环长到达入口，此时以slow和head一定会在入口相遇

<img src="file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-05/Ori/31d218800b10528ea2c64dabd71f2929.jpg" alt="img" style="zoom:33%;" />

快慢指针相遇的时候，为什么慢指针没有走完内一整圈，按照图上这种极端情况来看，fast需要走(环长-1)步才能追上slow，1.连快指针都走不了一整圈，（以相对速度来看），慢指针更不可能走完一整圈 2. slow走了（环长-1）/2步，那么（环长-1）/2<环长成立，即证

​          所以，这道题只需要额外判断，当快慢指针相遇之后，一直等到慢指针和头节点相遇时，返回slow即是入口!

​          时间复杂度O(n) 空间复杂度O(1)

​          在快慢指针相遇的阶段，慢指针最多走n步进入循环，快指针花费小于n的步数与慢指针相遇，总时间复杂度是On

​          在head和slow相遇阶段：

`head` 从起点到入环点，最多走 `a` 步，`slow` 从相遇点到入环点，最多走 `a` 步（因为其实是c+环长=a），而 `a ≤ n`（因为 `n = a + b`，b≥1），所以这个阶段的步数 ≤ n，时间复杂度为 **O(n)**

​         所以总的时间复杂度是On

##### 重排链表P143

<img src="file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-05/Ori/b8f59727d8918cd0cbcdfa546007ec88.jpg" alt="img" style="zoom:33%;" />

​          这道题的思路就是先寻找到中间节点，然后**反转**后半部分的链表，然后重新排序就好了

<img src="file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-05/Ori/0e75ebfb5b56ec638f2fcf2cf909f5dc.jpg" alt="img" style="zoom:33%;" />

记得保存head的nxt和head2的nxt做循环，记得反转链表只反转后半部分，然后前面一个后面一个连接起来，最后判断head2的下一个是不是空的就好了



##### 环形数组是否存在循环P457

这道题我觉得重在理解，给出了nums数组其实是移动的步数

大致思路就是借用了P142判圈算法

先借用lamada表达式创建一个寻找下一个索引位置的匿名函数next,

```
((cur + nums[cur]) % n + n) % n
```

当前索引加上移动步数再取于避免超出长度，最后又加了一次n，又取于是因为要把负数转换称正数

然后开始遍历每一个索引，先看有没有被标记

创建快慢指针，快指针先走一步，避免初始时二者相同而错误判定为环

开始内层循环，要求三个元素方向相同即在一个环上面，当slow和fast相遇的时候说明有环，如果slow的索引和next（slow）的索引一样说明有环但是长度为1,舍去，否则找到

循环条件是慢指针走一次，快指针走两次

这个索引没有找到环的话就开始标记环上每一个数字都是0,对应了每一次遍历的时候检查有没有被标记，依旧是判定两个元素是不是一个方向的，变成0！完

#### 双指针

其实也就是一个链表一个指针，可以用来整理数据构造两个指针，判断两链表结合点

#### 合并链表

##### 两数相加P2

这道题很有意思，因为他的数字是倒着来的，也就是个位对齐

###### 递归---创建新节点

因为是递归，所以先想边界条件：也就是两个链表都是空并且进位carry也是空的时候停止

递归中：判断l1 l2是不是空的，不是空的就加到sum里面，然后链表后移，这一步很妙，既后移了链表，为后续递归创造条件，而且还做到了把和相加，最重要的是他处理了为空的时候，比如说第一个链表十位上没有数字，但是第二个链表十位上有数字，那么就会直接跳过第一个链表，也就是默认把nullptr当作是0了，很妙

递归创建新节点：<img src="/home/white/.config/Typora/typora-user-images/image-20260617215918676.png" alt="image-20260617215918676" style="zoom: 67%;" />



表面看起来是从头节点开始相加的，实际上是从高位到低位创建，从后往前创建，很妙

```c++
return new ListNode(s%10,addTwoNumbers(l1,l2,s/10));
```



###### 递归---原地修改

简化了创建新节点的做法，其根源就是直接改变l1链表，只在l1l2都为空的时候判断carry值来确定用不用建立一个新节点（所以说从头到尾最多建造一个新节点，但是弊端就是改变了原链表）

这种做法把**两个链表不确定的状态**变成了**一个绝对非空另外一个需要判定**的状态，减少了逻辑分支也就是if嵌套，代码的认知复杂度降低了

具体是怎么实现的呢？需要提前判断l1链表是不是空的，空的就交换l1l2,这样就是一个极大的简化，l1不空，然后之后的判断只需要判断l2就好了

###### 递归

时间复杂度就是O(n)，n是l1l2长度的最大值

空间复杂度O(n)，递归需要的栈空间

###### 迭代

-   **递归**：把“创建节点”和“处理下一位”绑定在 `return` 语句里，利用**系统栈**帮你记录当前走到哪了。

-   **迭代**：把“创建节点”放在循环体里，利用**自己定义的 `cur` 指针**手动记录当前走到哪了。

    所以说迭代做法其实和递归---创建新节点的思路是一样的，只不过一个是系统调用栈记录，但是迭代是自己用cur记录，求和部分完全一眼

    -   **递归**：`return new ListNode(s % 10, addTwoNumbers(...));`
        ——先`new`出当前节点，它的`next`指向**未来**（递归返回的结果）。
    -   **迭代**：`cur = cur->next = new ListNode(sum % 10);`
        ——先`new`出当前节点，把它挂在`cur`的**过去**（上一个节点的后面）。

    这就像是在搭积木：递归是从**前往后**创建，但**从后往前**连接（先创建头，再回头接尾）；迭代是从**前往后**创建，且**从前往后**连接（边创建边接上）。

所以递归依赖系统调用栈，每递归一层CPU就会压一次栈，但是迭代依赖局部变量



###### 迭代

时间复杂度O(n)，n是链表长度最大值

空间复杂度O(1)，只有一些变量

##### 合并两个有序链表P21

###### 合并两个有序数组P88

倒着来，因为后面都是0

思路：

1.   创建nums1,nums2最后一个有序元素的下标p1p2,nums1（新数组）最后一个元素的下标

2.   然后开始循环遍历，条件判断是nums2也就是p2（因为是把nums2加入到nums1里面），判断p1是否合理并且比较p1p2的大小

3.   如果p1大，那就把这个元素移动到p,否则把p2移动到p,记得最后要移动p1（或p2）和p

     ```c++
     nums1[p--] = nums1[p1--];
     ```

###### 迭代（尾插法

创建一个哨兵节点，为什么这么做？

因为这样子就不用单独去判断链表会不会为空，或者说处理头节点的事情

然后开始遍历两个链表，条件是两个链表都存在

谁更小就让cur的next指向他，**注意这里是直接指向链表而不是赋值**

```c++
cur->next=list1;
```

指向的链表后移

遍历完之后判断有没有哪一个链表没有遍历完全，即最后一个链表没有遍历完全，追加判断

```c
cur->next=list1?list1:list2;
```

时间复杂度O(n+m)两个链表的长度

空间复杂度O(1)若干变量

###### 递归（头插法

递归切忌不要往深处去想

首先判断边界条件：如果一个链表是空的，那么就直接返回另一个链表就好了

然后比较list1list2的大小，如果是list1更小，那么应该递归list1的next和list2

​                                          如果是list2更小，那么应该递归list2的next和list1

也就是想一下当前位置的next是什么？其实就是调用递归函数的时候了，这个时候要相信递归函数它可以返回你要的下一个节点

最后返回，倒着连接在了一起



###### 双指针查找两个数字之和等于某值

```c++
vector<int>res;
int left=0;
int right=res.size()-1;
while(left<right){
    int sum=res[left]+res[right];
    if(sum==k){
        return true;
    }else if(sum<k){
        left++;
    }else{
        right++;
    }
}
```











### 二叉树

二叉树有三种遍历方式：先序遍历,后序遍历和中序遍历

##### 话说，这三种遍历有什么区别吗？

核心差异是在与**根节点的访问时间**

| 遍历方式 | 根访问时机 | 信息流向     | 核心能力         | 典型题型                     |
| -------- | ---------- | ------------ | ---------------- | ---------------------------- |
| **前序** | 最先       | 根 → 子树    | 构建/复制/路径   | 路径和、序列化、克隆         |
| **中序** | 中间       | 左 → 根 → 右 | **BST 有序性**   | 验证BST、第K小、恢复BST      |
| **后序** | 最后       | 子树 → 根    | **子树信息汇总** | 高度、直径、删除树、子树统计 |

-   **要路径、要复制、要序列化** → 想**前序**
-   **BST、有序、第K个** → 想**中序**
-   **要高度、要直径、要删树、要子树信息** → 想**后序**





#### 先序遍历DFS 

---



##### 二叉树的最大深度P104

###### 自顶向下和自底向上有什么区别？

| 维度         | 自顶向下 (Top-down)                | 自底向上 (Bottom-up)                               |
| ------------ | ---------------------------------- | -------------------------------------------------- |
| **场景**     | 只需要路径信息                     | 需要知道左右子树的结果才能计算当前节点             |
| **应用**     | 求路径和，路径列表，特定值是否存在 | 求树的**高度，深度，直径，节点属，最大值和最小值** |
| **返回值**   | 通常无返回值，用外部变量收集结果   | 通常有返回值，子树结果合并给父节点                 |
| **典型应用** | 路径、遍历、条件判断               | 高度、直径、平衡性、子树统计                       |

那么以这一道题为例

###### 自底向上

```c
class Solution {
public:
    int maxDepth(TreeNode* root) {
        if(!root)return 0;
        left_depth=maxDepth(root->left);
        right_depth=maxDepth(root->right);
        return max(left_depth,right_depth)+1;
    }
};
```

先递归到底，在逐层返回，每一个节点都依赖子树的返回值

###### 自顶向下

```c#
class Solution {
public:
    int maxDepth(TreeNode* root) {
        int ans=0;
        auto dfs=[&](this auto&& dfs,TreeNode*node,int depth)->void{
            if(!node)return;
            depth++;
            ans=max(ans,depth);
            dfs(node->left,depth);
            dfs(node->right,depth);
        };
        dfs(root,0);
        return ans;
    }
};
```

先处理当前节点，然后带着结果递归下去，用参数传递路径信息

###### 模板

自底向上模板

```c
def bottom_up(node):
    if not node:
        return 基础值  # 空树的返回值（0, None, []等）
    
    left = bottom_up(node.left)    # 先递归左
    right = bottom_up(node.right)  # 再递归右
    
    # 用 left、right 的结果计算当前节点的结果
    result = 合并(left, right, node.val)
    return result
```

自顶向下

```c
def top_down(node, 路径状态):
    if not node:
        return
    
    # 1. 先处理当前节点（更新全局答案）
    更新答案(node, 路径状态)
    
    # 2. 更新状态，传给子节点
    top_down(node.left, 新状态)
    top_down(node.right, 新状态)
```







##### 二叉树的最小深度P111

###### 自底向上（普通递归

对于最小深度来说，增加了很多判断逻辑和步骤

首先对于最大深度使用了max(0,x)自动会把0排除掉，但是对于最小深度使用min(0,x)0就会影响结果，比如说是只有右子树的情况下，左子树根本不能参与运算，但是最大深度可以，它可以排除0的影响，最小深度不可以

第一种：

分类讨论：

如果 node 是空节点，由于没有节点，返回 0。
如果 node 没有右儿子，那么深度就是左子树的深度加一，即 dfs(node)=dfs(node.left)+1。
如果 node 没有左儿子，那么深度就是右子树的深度加一，即 dfs(node)=dfs(node.right)+1。
如果 node 左右儿子都有，那么分别递归计算左子树的深度，以及右子树的深度，二者取最小值再加一，即
dfs(node)=min(dfs(node.left),dfs(node.right))+1
注意：并不需要特判 node 是叶子的情况，因为在没有右儿子的情况下，我们会递归 node.left，如果它是空节点，递归的返回值是 0，加一后得到 1，这正是叶子节点要返回的值

第二种：

如果 node 是空节点，返回 0。
如果 node 是叶子节点，返回 1。
否则，计算 leftDepth，如果左儿子不是空节点，那么 leftDepth=minDepth(node.left)，否则 leftDepth=∞，这样后面计算 min 不会取到 ∞。对于右儿子也同理，计算出 rightDepth。最后返回 min(leftDepth,rightDepth)+1。

###### 自顶向下

空的就返回

每次调用都会使深度增加

叶子结点就更新最小值

最后递归调用

优化：如果cnt>=ans就可以停止了，增加一个判断

```c
++cnt>=ans
```

















































##### 二叉树的右视图P199

因为要从右边看，所以先遍历右子树再遍历左子树，用一个ans代表答案的长度，然后判断答案长度和遍历深度是否一样，一样的话就加入答案队列

因为先遍历的是右子树，所以先回把最右边的记录进去，再一次往左记录，这样就实现了右视图



##### 



---



#### 后序遍历DFS

---

##### 相同的树P100

判断两颗树是否相同，只需要判断左子树和右子树是不是分享相同就好了

递归思路就是

1.想考虑边界条件，什么时候会停下，递归到底层的时候怎么判别，当所有子树有一个是null时，判断左右子树是否相等

2.考虑调用自身函数实现递归：判断当前左右子树的值是否相同，递归调用两个左子树，递归调用两个右子树

##### 对称二叉树P101

判断是不是对称子树，只需要判断，左子树和右子树是否相同，利用上一题判断相同的树来递归调用

##### 平衡二叉树P110

那就是判断左子树深度和右子树深度的差是不是小于1，

1.判断边界，如果是空的就返回0

2.计算左子树高度和右子树高度，如果高度是-1就层层向上传递-1,表示出错，或者是相差的值大于1,也返回-1,最后返回左子树和右子树的更大值加一，给上级传递使用

3.最后传入跟节点看是不是等于-1\



#### 二叉搜索树

什么是二叉搜索树？

就是左子树比节点值小，右子树比节点值大的二叉树

二叉搜索树的中序遍历

```c#
    void searchBST(TreeNode* cur) {
        if (cur == NULL) return ;
        searchBST(cur->left);       // 左
        （处理节点）                // 中
        searchBST(cur->right);      // 右
        return ;
    }
```



##### 验证二叉搜索树P98

![img](file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-08/Ori/0e728f82bc624f23b56bacae3081071e.jpg)

###### 前序遍历

对于左子树的递归，将右边界修改为节点值

对于右子树的递归，将左边界修改为节点值

```c++
class Solution {
public:
    bool isValidBST(TreeNode* root,long long left=LLONG_MIN,long long right=LLONG_MAX) {
        if(!root)return true;
        long long x=root->val;
        return left<x&&right>x&&isValidBST(root->left,left,x)&&isValidBST(root->right,x,right);
    }
};
```



###### 中序遍历

我们按照中序遍历出来的数组一定是严格递增的，那么如果我们要判断一个二叉树是不是二叉搜索树，只需要证明他的当前节点值是不是比前一个值小，我们维护一个上一个的节点值就好了

```c++
class Solution {
public:
    long long pre=LLONG_MIN;
    bool isValidBST(TreeNode* root) {
        if(!root)return true;
        if(!isValidBST(root->left))return false;
        if(root->val<=pre)return false;
        pre=root->val;
        return isValidBST(root->right);
    }
};
```

###### 后序遍历

![img](file:////home/white/.config/QQ/nt_qq_fa6f8ae2d2dd591655d689996a896030/nt_data/Pic/2026-08/Ori/b0d957eec7d1af853e35c44bcd1bd338.jpg)



前序遍历是从上往下传，那么后序遍历就是从下往上传

先获取左右子树的范围，然后判断当前节点的值是否小于左子树的最大值或者是大于右子树的最小值，这些是错误情况

```c
class Solution {
public:
    pair<long long ,long long>dfs(TreeNode*node){
        if(!node)return {LLONG_MAX, LLONG_MIN};
        auto [l_min,l_max]=dfs(node->left);
        auto [r_min,r_max]=dfs(node->right);
        long long x=node->val;
        if(x<=l_max||x>=r_min){
            return {LLONG_MIN, LLONG_MAX};
        }
        return {min(l_min,x),max(r_max,x)};
    }
    bool isValidBST(TreeNode* root) {
        return dfs(root).second!=LLONG_MAX;
    }
};
```

  

#### 最近公共祖先

##### 二叉树的最近公共祖先P236

我们首先要判断两个节点的位置

边界:如果节点是空节点或者是任意一个节点,那么直接返回,意味着找到了位置

我们分类讨论:

1.   如果节点在左右两个子树上都存在,那么最近的公共祖先是当前节点
2.   如果只有左子树存在,那么我们返回左子树
3.   如果只有右子树存在,那么我们返回右子树

```c++
class Solution {
public:
    TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
        if(!root||root==p||root==q){
            return root;
        }
        TreeNode*left=lowestCommonAncestor(root->left,p,q);
        TreeNode*right=lowestCommonAncestor(root->right,p,q);
        if(left&&right)return root;
        if(left&&!right)return left;
        return right;
    }
};
```

##### 二叉搜索树的最近公共祖先

因为是二叉搜索树,我们就可以直接判断节点值的大小从而判断位置,而不用看递归是否存在

分类讨论:

1.   如果两个节点值都小于当前节点值,那么两者都在左子树上,祖先也在左子树上,递归左子树
2.   如果两个节点值都大于当前节点值,那么二者都在右子树上,祖先也在右子树上,递归右子树
3.   如果不是以上两种情况,代表两个节点在左右子树上,那么返回当前节点

```c++
class Solution {
public:
    TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
        if(p->val<root->val&&q->val<root->val)
        return lowestCommonAncestor(root->left,p,q);
        if(p->val>root->val&&q->val>root->val)
        return lowestCommonAncestor(root->right,p,q);
        return root;       
    }
};
```



#### 创建二叉树

递归构造二叉树，找到左边界和右边界，通过根节点递归找出左右部分

相关题目有：升序数组，二叉搜索树

##### 将有序数组转换为二叉搜索树P108

1. 升序数组，那么我们要找到中间节点作为根节点
2. 递归左右部分

```c++
TreeNode* helper(vector<int>& nums, int left, int right) {
        if (left > right) {
            return nullptr;
        }

       // 找到中间节点作为根节点
        int mid = (left + right) / 2;

        TreeNode* root = new TreeNode(nums[mid]);
    //递归处理左右子树
        root->left = helper(nums, left, mid - 1);
        root->right = helper(nums, mid + 1, right);
        return root;
    }

```



##### 前序遍历构造二叉搜索树P1008

1. 前序遍历的话，那么第一个元素是根节点
2. 剩余元素中，小于根节点的是左子树，大于根节点的是右子树
3. 我们只需要找到第一个比根节点大的位置mid，划分为左右子树两个区间，递归构建

```c++
TreeNode*dfs(const vector<int>& preorder, int left, int right) {
        if(left>right)return  nullptr;
    //先找到根节点，然后构建根节点
        int rootVal=preorder[left];
        TreeNode* root=new TreeNode(rootVal);
    //区分左右子树空间
        int mid=left+1;
        while(mid<=right&&preorder[mid]<rootVal){
            mid++;
        }
    //递归构建左右子树
        root->left=dfs(preorder, left+1, mid-1);
        root->right=dfs(preorder, mid, right);
    //最后返回
        return root;
    }
```





