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

#include "../CJSON/CJSONHanler.h"

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
    void submit(std::wstring task_name, std::shared_ptr<void> context) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskMap.find(task_name) != taskMap.end() ||
            taskMap.find(L"*") != taskMap.end()) {
            // 找到任务并推送到任务队列
            tasks.push({ task_name, context });
            cv.notify_one();  // 唤醒一个等待的线程来处理任务
        }
    }

    // 注册任务到线程池
    void registerTask(const std::wstring& taskName, std::function<void(std::shared_ptr<void>)> taskFunc) {
        std::lock_guard<std::mutex> lock(taskMapMutex);
        taskMap[taskName] = taskFunc;
    }

    // sync handle task 
    void sync_run_task(std::wstring task_name, std::shared_ptr<void> context) {
        std::function<void(std::shared_ptr<void>)> task = nullptr;
        auto def_it = taskMap.find(L"*");
        if (!task) {
            std::lock_guard<std::mutex> lock(taskMapMutex);
            auto it = taskMap.find(task_name);
            if (it != taskMap.end()) {
                task = it->second;  // 执行任务
            }
        }
        if (task) {
            task(context);
        } 
        else if (def_it != taskMap.end()) {
            def_it->second(context);
        }
    }

private:
    AlpcHandler() : stopFlag(false) {}

    // 工作线程处理任务的函数
    void workerThread() {
        while (true) {
            std::pair<std::wstring, std::shared_ptr<void>> task;
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
                auto def_it = taskMap.find(L"*");
                if (it != taskMap.end()) {
                    it->second(task.second);  // 执行任务
                }
                else if (def_it != taskMap.end()) {
                    def_it->second(task.second);
                }
            }
        }
    }

    // 禁止拷贝和赋值
    AlpcHandler(const AlpcHandler&) = delete;
    AlpcHandler& operator=(const AlpcHandler&) = delete;

    std::vector<std::thread> workers;  // 工作线程
    std::queue<std::pair<std::wstring, std::shared_ptr<void>>> tasks; // 任务队列
    std::mutex queueMutex;             // 保护任务队列的锁
    std::condition_variable cv;        // 条件变量，用于线程同步
    std::atomic<bool> stopFlag;        // 停止标志
    std::map<std::wstring, std::function<void(std::shared_ptr<void>)>> taskMap;  // 任务名称映射
    std::mutex taskMapMutex;           // 保护任务映射的锁
};

