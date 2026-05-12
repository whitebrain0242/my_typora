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

