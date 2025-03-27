// Event.h - 定义事件类型和结构
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <functional>
#include <unordered_map>

namespace malware_analysis {

    // 事件类型定义
    enum class EventType {
        PROCESS_CREATE,
        PROCESS_TERMINATE,
        FILE_READ,
        FILE_WRITE,
        FILE_DELETE,
        DLL_LOAD,
        REGISTRY_READ,
        REGISTRY_WRITE,
        NETWORK_CONNECT,
        NETWORK_LISTEN,
        NETWORK_SEND,
        NETWORK_RECEIVE,
        MEMORY_ALLOCATE,
        MEMORY_PROTECT,
        MEMORY_WRITE
    };

    // 将EventType转换为字符串
    std::string eventTypeToString(EventType type);

    // 事件详细信息的基类
    struct EventDetails {
        virtual ~EventDetails() = default;
        virtual std::string toString() const = 0;
    };

    // 进程事件详情
    struct ProcessEventDetails : public EventDetails {
        uint32_t parentProcessId;
        std::string commandLine;
        std::string executablePath;
        bool elevated;

        std::string toString() const override;
    };

    // 文件事件详情
    struct FileEventDetails : public EventDetails {
        std::string filePath;
        uint64_t fileSize;
        bool success;
        std::optional<std::string> fileHash;

        std::string toString() const override;
    };

    // DLL加载事件详情
    struct DllEventDetails : public EventDetails {
        std::string dllName;
        std::string dllPath;
        bool success;
        std::optional<std::string> dllHash;

        std::string toString() const override;
    };

    // 网络事件详情
    struct NetworkEventDetails : public EventDetails {
        std::string remoteAddress;
        uint16_t remotePort;
        std::string protocol;
        bool success;
        uint64_t dataSize;

        std::string toString() const override;
    };

    // 内存事件详情
    struct MemoryEventDetails : public EventDetails {
        uint64_t address;
        uint64_t size;
        uint32_t protection; // 内存保护标志
        bool isExecutable;

        std::string toString() const override;
    };

    // 事件基本信息
    struct Event {
        uint32_t processId;
        std::string processName;
        EventType type;
        std::chrono::system_clock::time_point timestamp;
        std::shared_ptr<EventDetails> details;

        Event(uint32_t pid, std::string name, EventType eventType,
            std::shared_ptr<EventDetails> eventDetails);

        // 转换为字符串用于日志记录
        std::string toString() const;

        // 转换为JSON格式
        std::string toJson() const;

        // 从JSON创建事件
        static Event fromJson(const std::string& json);
    };

    // 辅助函数命名空间
    namespace filters {
        // 创建文件路径匹配过滤器
        std::function<bool(const Event&)> filePathContains(const std::string& substring);

        // 创建DLL名称匹配过滤器
        std::function<bool(const Event&)> dllNameEquals(const std::string& dllName);

        // 创建网络连接匹配过滤器
        std::function<bool(const Event&)> networkConnectTo(const std::string& address,
            std::optional<uint16_t> port = std::nullopt);

        // 创建命令行包含过滤器
        std::function<bool(const Event&)> commandLineContains(const std::string& substring);

        // 创建进程名匹配过滤器
        std::function<bool(const Event&)> processNameEquals(const std::string& processName);
    }

} // namespace malware_analysis