#pragma once

class AsyncTaskManager {
public:
    // 获取单例实例
    static AsyncTaskManager& GetInstance() {
        static AsyncTaskManager instance; // C++11保证静态局部变量线程安全
        return instance;
    }

    // 添加异步任务（支持任意函数和参数）
    template<typename Func, typename... Args>
    void AddTask(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx_);
        // 启动异步任务并保存 future
        futures_.emplace_back(
            std::async(std::launch::async,
                [func = std::forward<Func>(func)](auto&&... params) {
                    try {
                        func(std::forward<decltype(params)>(params)...);
                    }
                    catch (...) {
                        // 统一异常处理（可扩展为日志记录）
                    }
                },
                std::forward<Args>(args)...
            )
        );
    }

    // 等待所有任务完成（可选）
    void WaitAll() {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto& fut : futures_) {
            if (fut.valid()) fut.wait();
        }
    }

    // 禁止复制和移动
    AsyncTaskManager(const AsyncTaskManager&) = delete;
    AsyncTaskManager& operator=(const AsyncTaskManager&) = delete;

private:
    AsyncTaskManager() = default;
    ~AsyncTaskManager() {
        WaitAll(); // 析构时确保所有任务完成
    }

    std::mutex mtx_;
    std::vector<std::future<void>> futures_;
};
