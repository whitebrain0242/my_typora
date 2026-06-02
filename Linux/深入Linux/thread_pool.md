下面给你一版**适合阶段性任务提交**的 C++ 线程池实现。它包含：

1. 一个**手写线程池 ThreadPool**
2. 一个**任务完成回调队列 CallbackDispatcher**
3. 一个**Without Network：矩阵乘法任务调度程序**
4. 一个**With Network：并发下载器版本，需要 libcurl**
5. 每一部分都有详细讲解

你的题目中有一句写的是“矩阵相乘”，后面又出现“计算阶乘”。这两个任务本质上都可以放进线程池执行。我下面以**矩阵乘法**为主实现，因为题目多次强调“矩阵任务”；如果要改成阶乘任务，只需要把任务函数换成 factorial 即可。

------

# 一、完整代码

文件名建议叫：

```cpp
main.cpp
```

这份代码默认运行**矩阵乘法线程池示例**。

如果你想运行**并发下载器版本**，编译时加上：

```bash
-DENABLE_NETWORK
```

------

```cpp
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <chrono>
#include <string>
#include <fstream>
#include <atomic>
#include <algorithm>

#ifdef ENABLE_NETWORK
#include <curl/curl.h>
#include <cstdio>
#endif

// ============================================================
// 1. 通用线程池 ThreadPool
// ============================================================

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount)
        : stopping_(false) {
        if (threadCount == 0) {
            threadCount = 1;
        }

        for (std::size_t i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this, i]() {
                workerLoop(i);
            });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        shutdown();
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex_);

            if (stopping_) {
                throw std::runtime_error("ThreadPool has been stopped. Cannot submit new task.");
            }

            tasks_.emplace([task]() {
                (*task)();
            });
        }

        condition_.notify_one();

        return result;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);

            if (stopping_) {
                return;
            }

            stopping_ = true;
        }

        condition_.notify_all();

        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void workerLoop(std::size_t workerId) {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queueMutex_);

                condition_.wait(lock, [this]() {
                    return stopping_ || !tasks_.empty();
                });

                if (stopping_ && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "[Worker " << workerId << "] Task exception: "
                          << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Worker " << workerId << "] Unknown task exception."
                          << std::endl;
            }
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;

    bool stopping_;
};


// ============================================================
// 2. 回调调度器 CallbackDispatcher
// ============================================================

class CallbackDispatcher {
public:
    void post(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push(std::move(callback));
    }

    void processAll() {
        std::queue<std::function<void()>> localCallbacks;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(localCallbacks, callbacks_);
        }

        while (!localCallbacks.empty()) {
            localCallbacks.front()();
            localCallbacks.pop();
        }
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return callbacks_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::queue<std::function<void()>> callbacks_;
};


// ============================================================
// 3. Without Network：矩阵乘法任务
// ============================================================

using Matrix = std::vector<std::vector<long long>>;

Matrix generateMatrix(std::size_t rows, std::size_t cols, long long seed) {
    Matrix matrix(rows, std::vector<long long>(cols));

    long long value = seed;

    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            matrix[i][j] = value % 10 + 1;
            value += 3;
        }
    }

    return matrix;
}

Matrix multiplyMatrix(const Matrix& a, const Matrix& b) {
    if (a.empty() || b.empty()) {
        throw std::runtime_error("Matrix cannot be empty.");
    }

    std::size_t aRows = a.size();
    std::size_t aCols = a[0].size();
    std::size_t bRows = b.size();
    std::size_t bCols = b[0].size();

    if (aCols != bRows) {
        throw std::runtime_error("Matrix size mismatch: a.columns must equal b.rows.");
    }

    Matrix result(aRows, std::vector<long long>(bCols, 0));

    for (std::size_t i = 0; i < aRows; ++i) {
        for (std::size_t j = 0; j < bCols; ++j) {
            for (std::size_t k = 0; k < aCols; ++k) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return result;
}

void printMatrix(const Matrix& matrix, std::size_t maxRows = 4, std::size_t maxCols = 4) {
    std::size_t rows = std::min(maxRows, matrix.size());

    for (std::size_t i = 0; i < rows; ++i) {
        std::size_t cols = std::min(maxCols, matrix[i].size());

        for (std::size_t j = 0; j < cols; ++j) {
            std::cout << matrix[i][j] << "\t";
        }

        if (matrix[i].size() > maxCols) {
            std::cout << "...";
        }

        std::cout << "\n";
    }

    if (matrix.size() > maxRows) {
        std::cout << "...\n";
    }
}

void runMatrixDemo() {
    std::cout << "========== Matrix Thread Pool Demo ==========\n";

    const std::size_t threadCount = 10;
    ThreadPool pool(threadCount);
    CallbackDispatcher dispatcher;

    std::vector<std::future<void>> futures;

    const int taskCount = 12;

    for (int taskId = 1; taskId <= taskCount; ++taskId) {
        Matrix a = generateMatrix(3 + taskId % 3, 4, taskId);
        Matrix b = generateMatrix(4, 3 + taskId % 2, taskId * 10);

        auto future = pool.submit(
            [taskId, a = std::move(a), b = std::move(b), &dispatcher]() mutable {
                auto start = std::chrono::steady_clock::now();

                Matrix result = multiplyMatrix(a, b);

                auto end = std::chrono::steady_clock::now();

                auto costMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - start
                ).count();

                dispatcher.post(
                    [taskId, result = std::move(result), costMs]() mutable {
                        std::cout << "\n[Callback] Task " << taskId
                                  << " finished. Cost: " << costMs << " ms\n";
                        std::cout << "Result matrix preview:\n";
                        printMatrix(result);
                    }
                );
            }
        );

        futures.push_back(std::move(future));

        std::cout << "Submitted matrix task " << taskId << "\n";
    }

    for (auto& future : futures) {
        try {
            future.get();
        } catch (const std::exception& e) {
            std::cerr << "[Main] Task failed: " << e.what() << std::endl;
        }
    }

    dispatcher.processAll();

    pool.shutdown();

    std::cout << "\nAll matrix tasks finished. Thread pool closed safely.\n";
}


// ============================================================
// 4. With Network：并发下载器，需要 libcurl
// ============================================================

#ifdef ENABLE_NETWORK

struct DownloadTask {
    int id;
    std::string url;
    std::string savePath;
};

struct DownloadResult {
    int id;
    std::string url;
    std::string savePath;
    bool success;
    long httpCode;
    std::string message;
};

class CurlGlobalGuard {
public:
    CurlGlobalGuard() {
        CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (code != CURLE_OK) {
            throw std::runtime_error("curl_global_init failed.");
        }
    }

    ~CurlGlobalGuard() {
        curl_global_cleanup();
    }
};

std::size_t writeToFileCallback(
    void* contents,
    std::size_t size,
    std::size_t nmemb,
    void* userPointer
) {
    std::size_t totalSize = size * nmemb;

    std::ofstream* output = static_cast<std::ofstream*>(userPointer);
    output->write(static_cast<const char*>(contents), static_cast<std::streamsize>(totalSize));

    if (!output->good()) {
        return 0;
    }

    return totalSize;
}

DownloadResult downloadFile(const DownloadTask& task) {
    DownloadResult result;
    result.id = task.id;
    result.url = task.url;
    result.savePath = task.savePath;
    result.success = false;
    result.httpCode = 0;

    std::ofstream output(task.savePath, std::ios::binary);

    if (!output.is_open()) {
        result.message = "Failed to open local file: " + task.savePath;
        return result;
    }

    CURL* curl = curl_easy_init();

    if (!curl) {
        result.message = "curl_easy_init failed.";
        return result;
    }

    curl_easy_setopt(curl, CURLOPT_URL, task.url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SimpleThreadPoolDownloader/1.0");

    CURLcode code = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    result.httpCode = httpCode;

    curl_easy_cleanup(curl);
    output.close();

    if (code != CURLE_OK) {
        result.message = std::string("Download failed: ") + curl_easy_strerror(code);
        std::remove(task.savePath.c_str());
        return result;
    }

    if (httpCode >= 400) {
        result.message = "HTTP error code: " + std::to_string(httpCode);
        std::remove(task.savePath.c_str());
        return result;
    }

    result.success = true;
    result.message = "Download success.";

    return result;
}

void runDownloadDemo() {
    std::cout << "========== Concurrent Downloader Demo ==========\n";

    CurlGlobalGuard curlGuard;

    const std::size_t threadCount = 4;
    ThreadPool pool(threadCount);
    CallbackDispatcher dispatcher;

    std::vector<DownloadTask> tasks = {
        {1, "https://www.example.com/", "example_1.html"},
        {2, "https://www.example.com/", "example_2.html"},
        {3, "https://www.example.com/", "example_3.html"},
        {4, "https://www.example.com/", "example_4.html"}
    };

    std::vector<std::future<void>> futures;

    for (const auto& task : tasks) {
        auto future = pool.submit(
            [task, &dispatcher]() {
                auto start = std::chrono::steady_clock::now();

                DownloadResult result = downloadFile(task);

                auto end = std::chrono::steady_clock::now();

                auto costMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - start
                ).count();

                dispatcher.post(
                    [result, costMs]() {
                        std::cout << "\n[Download Callback] Task " << result.id << "\n";
                        std::cout << "URL: " << result.url << "\n";
                        std::cout << "Save Path: " << result.savePath << "\n";
                        std::cout << "HTTP Code: " << result.httpCode << "\n";
                        std::cout << "Cost: " << costMs << " ms\n";
                        std::cout << "Status: "
                                  << (result.success ? "SUCCESS" : "FAILED")
                                  << "\n";
                        std::cout << "Message: " << result.message << "\n";
                    }
                );
            }
        );

        futures.push_back(std::move(future));

        std::cout << "Submitted download task " << task.id << "\n";
    }

    for (auto& future : futures) {
        try {
            future.get();
        } catch (const std::exception& e) {
            std::cerr << "[Main] Download task failed: " << e.what() << std::endl;
        }
    }

    dispatcher.processAll();

    pool.shutdown();

    std::cout << "\nAll download tasks finished. Thread pool closed safely.\n";
}

#endif


// ============================================================
// 5. main 函数
// ============================================================

int main() {
#ifndef ENABLE_NETWORK
    runMatrixDemo();
#else
    runDownloadDemo();
#endif

    return 0;
}
```

------

# 二、编译方式

## 1. 运行 Without Network 矩阵任务版本

这个版本只依赖 C++ 标准库。

```bash
g++ -std=c++17 -O2 -pthread main.cpp -o matrix_pool
./matrix_pool
```

------

## 2. 运行 With Network 并发下载器版本

这个版本需要安装 `libcurl`。

Ubuntu / Debian 可以安装：

```bash
sudo apt install libcurl4-openssl-dev
```

然后编译：

```bash
g++ -std=c++17 -O2 -pthread -DENABLE_NETWORK main.cpp -lcurl -o downloader
./downloader
```

------

# 三、整体设计思路

你可以把整个程序想象成一个餐厅。

| 程序概念           | 类比                               |
| ------------------ | ---------------------------------- |
| 任务队列           | 后厨订单队列                       |
| 工作线程           | 厨师                               |
| submit 提交任务    | 顾客下单                           |
| workerLoop         | 厨师不断取订单做菜                 |
| mutex              | 防止多个厨师同时抢同一张订单       |
| condition_variable | 没订单时厨师休息，有订单时叫醒厨师 |
| shutdown           | 餐厅打烊，不接新单，但做完已有订单 |

线程池的核心思想是：

> 不要每来一个任务就创建一个线程，而是提前创建一批线程，让这些线程反复从任务队列里取任务执行。

这样做的好处是：

1. 避免频繁创建和销毁线程；
2. 控制并发数量；
3. 防止任务太多导致系统线程爆炸；
4. 让任务调度更加稳定。

------

# 四、ThreadPool 线程池讲解

核心成员变量如下：

```cpp
std::vector<std::thread> workers_;
std::queue<std::function<void()>> tasks_;

std::mutex queueMutex_;
std::condition_variable condition_;

bool stopping_;
```

它们分别表示：

```cpp
workers_
```

保存所有工作线程。比如题目要求 10 个线程，那么这里就会保存 10 个 `std::thread`。

```cpp
tasks_
```

任务队列。所有提交进来的任务都会先进入这个队列。

```cpp
queueMutex_
```

保护任务队列。因为多个线程会同时访问队列，如果不加锁，可能出现数据竞争。

```cpp
condition_
```

条件变量。没有任务的时候，工作线程不能一直死循环空转，否则会浪费 CPU。所以我们让它们睡眠；当新任务到来时，再唤醒其中一个线程。

```cpp
stopping_
```

表示线程池是否正在关闭。

------

# 五、线程池是如何启动的？

构造函数里：

```cpp
for (std::size_t i = 0; i < threadCount; ++i) {
    workers_.emplace_back([this, i]() {
        workerLoop(i);
    });
}
```

如果 `threadCount = 10`，这里就会创建 10 个线程。

每个线程都会执行：

```cpp
workerLoop(i);
```

也就是说，每个线程都会进入自己的工作循环。

------

# 六、workerLoop 是线程池的核心

代码：

```cpp
void workerLoop(std::size_t workerId) {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            condition_.wait(lock, [this]() {
                return stopping_ || !tasks_.empty();
            });

            if (stopping_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}
```

这段逻辑非常重要。

它的意思是：

1. 工作线程进入死循环；
2. 先尝试从任务队列取任务；
3. 如果任务队列为空，就睡眠；
4. 如果有新任务进来，就被唤醒；
5. 取出任务；
6. 释放锁；
7. 执行任务；
8. 执行完之后继续回到循环开头，继续取下一个任务。

这里有一个关键点：

```cpp
if (stopping_ && tasks_.empty()) {
    return;
}
```

这句话的作用是：

> 如果线程池已经准备关闭，并且任务队列也空了，那么线程就退出。

也就是说，线程池关闭时不是马上杀死线程，而是：

> 不再接收新任务，但会把已有任务执行完。

这正好符合你的任务要求：

> 程序应提供一个方法来关闭任务调度程序和线程池，以确保所有任务都被执行完毕。

------

# 七、为什么需要 mutex？

因为任务队列是共享资源。

多个线程可能同时做这些事情：

1. 主线程正在往队列里添加任务；
2. 工作线程正在从队列里取任务；
3. 另一个工作线程也在取任务。

如果不加锁，可能出现这种情况：

```text
线程 A 判断队列不为空
线程 B 也判断队列不为空
线程 A 取走任务
线程 B 又去取同一个任务
程序崩溃
```

所以每次访问任务队列时，都要加锁：

```cpp
std::lock_guard<std::mutex> lock(queueMutex_);
```

或者：

```cpp
std::unique_lock<std::mutex> lock(queueMutex_);
```

------

# 八、为什么需要 condition_variable？

如果没有条件变量，工作线程可能会这样写：

```cpp
while (true) {
    if (!tasks.empty()) {
        取任务;
    }
}
```

这叫忙等。

忙等的问题是：

> 即使没有任务，线程也一直在循环，占用 CPU。

使用条件变量后，线程没有任务时会睡眠：

```cpp
condition_.wait(lock, [this]() {
    return stopping_ || !tasks_.empty();
});
```

这句话的意思是：

> 如果没有任务，也没有关闭信号，线程就睡觉。
> 如果有任务，或者线程池要关闭了，就醒来。

这样 CPU 不会被浪费。

------

# 九、submit 是如何提交任务的？

代码：

```cpp
template <typename F, typename... Args>
auto submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
```

这个函数支持提交任意类型的任务。

比如你可以提交：

```cpp
pool.submit([]() {
    std::cout << "Hello\n";
});
```

也可以提交：

```cpp
pool.submit([](int a, int b) {
    return a + b;
}, 10, 20);
```

它会返回一个 `std::future`。

`future` 可以用来获取任务结果。

例如：

```cpp
auto future = pool.submit([]() {
    return 100;
});

int result = future.get();
```

`future.get()` 会等待任务执行完成，然后拿到返回值。

------

# 十、为什么使用 packaged_task？

这行代码：

```cpp
auto task = std::make_shared<std::packaged_task<ReturnType()>>(...);
```

`packaged_task` 的作用是：

> 把一个普通函数包装成一个可以异步执行的任务，并且可以通过 future 拿到结果。

简单理解：

```text
普通函数
   ↓
packaged_task 包装
   ↓
放进任务队列
   ↓
工作线程执行
   ↓
future 拿到结果
```

这样线程池不仅能执行任务，还能让主线程知道任务是否完成、结果是什么、有没有异常。

------

# 十一、矩阵乘法任务是怎么放进线程池的？

在 `runMatrixDemo()` 里：

```cpp
auto future = pool.submit(
    [taskId, a = std::move(a), b = std::move(b), &dispatcher]() mutable {
        Matrix result = multiplyMatrix(a, b);

        dispatcher.post(
            [taskId, result = std::move(result)]() mutable {
                std::cout << "Task finished\n";
                printMatrix(result);
            }
        );
    }
);
```

这里提交的是一个 lambda 任务。

它做了几件事：

1. 在线程池中执行矩阵乘法；
2. 得到结果矩阵；
3. 把“打印结果”这个操作放进回调队列；
4. 主线程最后统一处理回调。

------

# 十二、为什么回调不直接在工作线程里打印？

其实可以直接打印，但我这里设计得更规范。

因为在真实项目中，工作线程通常只负责干活，比如：

1. 计算矩阵；
2. 下载文件；
3. 压缩文件；
4. 处理图片；
5. 解析数据。

而主线程或者某个专门模块负责：

1. 更新界面；
2. 打印日志；
3. 通知其他系统；
4. 统计任务结果。

所以我加了一个：

```cpp
CallbackDispatcher
```

它的作用是：

> 工作线程完成任务后，把回调函数放进回调队列；主线程之后再统一执行这些回调。

这样结构更清晰，也更接近真实项目。

------

# 十三、CallbackDispatcher 的作用

核心代码：

```cpp
void post(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push(std::move(callback));
}
```

工作线程调用 `post()`，把回调任务放进去。

然后主线程调用：

```cpp
dispatcher.processAll();
```

执行所有回调。

例如矩阵任务完成后会输出：

```text
[Callback] Task 1 finished. Cost: 0 ms
Result matrix preview:
...
```

这就是题目要求的：

> 添加任务完成的回调机制，以便在任务执行完成后进行一些操作，如打印结果或记录日志。

------

# 十四、shutdown 是如何保证安全关闭的？

代码：

```cpp
void shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        if (stopping_) {
            return;
        }

        stopping_ = true;
    }

    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
```

关闭分三步：

第一步，设置关闭标志：

```cpp
stopping_ = true;
```

这表示线程池不再接收新任务。

第二步，唤醒所有线程：

```cpp
condition_.notify_all();
```

有些线程可能正在睡眠。现在要关闭了，需要把它们叫醒。

第三步，等待所有线程结束：

```cpp
worker.join();
```

`join()` 的意思是：

> 主线程等待这个工作线程执行完毕。

只有所有工作线程都结束后，线程池才真正关闭。

------

# 十五、并发下载器版本讲解

网络版本使用的是：

```cpp
#ifdef ENABLE_NETWORK
```

这表示：

> 只有编译时加了 `-DENABLE_NETWORK`，下载器代码才会启用。

例如：

```bash
g++ -std=c++17 -O2 -pthread -DENABLE_NETWORK main.cpp -lcurl -o downloader
```

------

下载任务结构体：

```cpp
struct DownloadTask {
    int id;
    std::string url;
    std::string savePath;
};
```

每个下载任务包含：

1. 任务编号；
2. 文件 URL；
3. 本地保存路径。

下载结果结构体：

```cpp
struct DownloadResult {
    int id;
    std::string url;
    std::string savePath;
    bool success;
    long httpCode;
    std::string message;
};
```

它记录：

1. 下载是否成功；
2. HTTP 状态码；
3. 保存路径；
4. 错误信息。

------

# 十六、并发下载器是怎么工作的？

核心代码：

```cpp
auto future = pool.submit(
    [task, &dispatcher]() {
        DownloadResult result = downloadFile(task);

        dispatcher.post(
            [result]() {
                std::cout << "Download finished\n";
            }
        );
    }
);
```

流程如下：

```text
主线程提交多个下载任务
        ↓
任务进入线程池队列
        ↓
多个工作线程同时取任务
        ↓
每个线程下载一个文件
        ↓
下载完成后把结果放进回调队列
        ↓
主线程处理回调，打印成功或失败
```

这样就实现了并发下载。

如果线程池有 4 个线程，提交了 10 个下载任务，那么大致执行方式是：

```text
第一批：4 个任务同时下载
第二批：前面某个下载完，再取下一个
第三批：继续取下一个
直到所有任务完成
```

------

# 十七、这份代码满足了哪些要求？

## Without Network 部分

| 要求                       | 是否满足                    |
| -------------------------- | --------------------------- |
| 提供任务队列               | 满足，`tasks_`              |
| 提供线程池                 | 满足，`ThreadPool`          |
| 固定数量线程，例如 10 个   | 满足，`ThreadPool pool(10)` |
| 新任务加入任务队列         | 满足，`submit()`            |
| 线程执行完继续取下一个任务 | 满足，`workerLoop()`        |
| 队列为空时等待             | 满足，`condition_variable`  |
| 回调机制                   | 满足，`CallbackDispatcher`  |
| 安全关闭                   | 满足，`shutdown()`          |
| 矩阵乘法任务               | 满足，`multiplyMatrix()`    |

------

## With Network 部分

| 要求                        | 是否满足                   |
| --------------------------- | -------------------------- |
| 创建线程池执行下载任务      | 满足                       |
| 固定数量线程                | 满足，`ThreadPool pool(4)` |
| 下载任务包含 URL 和保存路径 | 满足，`DownloadTask`       |
| 任务队列存储下载任务        | 满足，`tasks_`             |
| 工作线程执行下载            | 满足，`downloadFile()`     |
| 同时处理多个下载任务        | 满足                       |
| 下载完成后通知主线程        | 满足，`CallbackDispatcher` |
| 关闭线程池                  | 满足，`shutdown()`         |

------

# 十八、如果要改成阶乘任务

题目里提到了阶乘，如果老师更想看到阶乘任务，可以加这个函数：

```cpp
unsigned long long factorial(unsigned int n) {
    unsigned long long result = 1;

    for (unsigned int i = 2; i <= n; ++i) {
        result *= i;
    }

    return result;
}
```

然后这样提交任务：

```cpp
auto future = pool.submit([]() {
    return factorial(10);
});

std::cout << future.get() << std::endl;
```

也就是说，线程池本身不用改。

你只需要换任务函数：

```text
矩阵乘法任务
阶乘任务
下载任务
图片处理任务
文件压缩任务
```

这些都可以扔进同一个线程池。

------

# 十九、可以写进报告里的总结

你可以在阶段性报告里这样描述：

> 本项目手动实现了一个固定大小的 C++ 线程池。线程池内部维护一个任务队列，主线程通过 submit 方法向任务队列提交任务，工作线程通过 condition_variable 等待任务到来。每个工作线程在完成当前任务后会继续从任务队列中获取下一个任务执行，直到线程池关闭且任务队列为空。
>
> 为了支持任务完成后的后续处理，程序额外实现了 CallbackDispatcher 回调调度器。工作线程完成计算或下载任务后，会将回调函数投递到回调队列中，由主线程统一处理结果打印、日志记录等操作。
>
> 在线程池关闭时，程序会先停止接受新任务，然后唤醒所有工作线程，并等待所有线程执行完已有任务后退出，从而保证任务不会丢失，线程资源能够被正确释放。

这段总结可以直接放到作业说明里。