#pragma once

#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <map>
#include <memory>
#include <string>

#include "alpc_util.h"

class AlpcHandlerCtx {
public:
    AlpcHandlerCtx(CJSONHandler& json, AlpcConn* alpc, ULONG msg_id)
        : json_(json)
        , alpc_(alpc)
        , msg_id_(msg_id)
    {}

    // 可以在此添加任意的上下文数据
    CJSONHandler& json_;
    AlpcConn* alpc_;
    ULONG msg_id_;
};

class AlpcHandler {
public:
    // 获取线程池单例实例
    static AlpcHandler& getInstance() {
        static AlpcHandler instance;
        return instance;
    }

    // 启动线程池，启动线程
    void start(size_t numThreads) {
        stopFlag = false;
        for (size_t i = 0; i < numThreads; ++i) {
            workers.push_back(std::thread(&AlpcHandler::workerThread, this));
        }
    }

    // 停止线程池，等待线程结束
    void stop() {
        stopFlag = true;
        cv.notify_all();  // 唤醒所有线程结束工作
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // 提交任务到线程池
    void submit(const std::wstring& taskName, std::shared_ptr<Context> context) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskMap.find(taskName) != taskMap.end()) {
            // 找到任务并推送到任务队列
            tasks.push({ taskName, context });
            cv.notify_one();  // 唤醒一个等待的线程来处理任务
        }
        else {
            std::cout << "Task not found: " << taskName << std::endl;
        }
    }

    // 注册任务到线程池
    void registerTask(const std::wstring& taskName, std::function<void(std::shared_ptr<Context>)> taskFunc) {
        std::lock_guard<std::mutex> lock(taskMapMutex);
        taskMap[taskName] = taskFunc;
    }

private:
    AlpcHandler() : stopFlag(false) {}

    // 工作线程处理任务的函数
    void workerThread() {
        while (true) {
            std::pair<std::wstring, std::shared_ptr<Context>> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);

                // 等待任务队列中有任务，或者线程池被停止
                cv.wait(lock, [this] { return !tasks.empty() || stopFlag; });

                // 如果线程池停止且队列为空，则退出
                if (stopFlag && tasks.empty()) {
                    break;
                }

                // 从队列中获取任务
                task = tasks.front();
                tasks.pop();
            }

            // 获取对应的任务函数并执行
            {
                std::lock_guard<std::mutex> lock(taskMapMutex);
                auto it = taskMap.find(task.first);
                if (it != taskMap.end()) {
                    it->second(task.second);  // 执行任务
                }
            }
        }
    }

    // 禁止拷贝和赋值
    AlpcHandler(const AlpcHandler&) = delete;
    AlpcHandler& operator=(const AlpcHandler&) = delete;

    std::vector<std::thread> workers;  // 工作线程
    std::queue<std::pair<std::wstring, std::shared_ptr<Context>>> tasks; // 任务队列
    std::mutex queueMutex;             // 保护任务队列的锁
    std::condition_variable cv;        // 条件变量，用于线程同步
    std::atomic<bool> stopFlag;        // 停止标志
    std::map<std::wstring, std::function<void(std::shared_ptr<Context>)>> taskMap;  // 任务名称映射
    std::mutex taskMapMutex;           // 保护任务映射的锁
};

