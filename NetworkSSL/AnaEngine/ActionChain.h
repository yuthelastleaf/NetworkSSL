// ActionChain.h - 定义进程行为链结构
#pragma once

#include "Event.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <chrono>
#include <atomic>

namespace malware_analysis {

    // 行为链表示一个进程的所有事件序列
    class BehaviorChain {
    public:
        BehaviorChain(uint32_t pid, std::string name);
        ~BehaviorChain() = default;

        // 添加事件到链中
        void addEvent(const Event& event);

        // 添加子进程
        void addChildProcess(std::shared_ptr<BehaviorChain> child);

        // 标记进程终止
        void terminate();

        // 获取进程ID
        uint32_t getProcessId() const;

        // 获取进程名称
        const std::string& getProcessName() const;

        // 检查进程是否已终止
        bool isTerminated() const;

        // 获取所有事件（包括子进程）
        std::vector<Event> getAllEvents() const;

        // 获取直接事件（不包括子进程）
        std::vector<Event> getDirectEvents() const;

        // 获取子进程链
        std::map<uint32_t, std::shared_ptr<BehaviorChain>> getChildProcesses() const;

        // 获取事件序列的持续时间
        std::chrono::milliseconds getDuration() const;

        // 序列化为JSON格式
        std::string toJson() const;

        // 从JSON反序列化
        static std::shared_ptr<BehaviorChain> fromJson(const std::string& json);

    private:
        uint32_t processId;
        std::string processName;
        std::vector<Event> events;
        std::map<uint32_t, std::shared_ptr<BehaviorChain>> childProcesses;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        std::atomic<bool> isFinished;

        mutable std::mutex eventsMutex;
        mutable std::mutex childrenMutex;
    };

} // namespace malware_analysis