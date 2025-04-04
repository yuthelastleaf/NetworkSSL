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

    enum EventType {
        EVENT_NULL,
        PROCESS_CREATE,
        PROCESS_TERMINATE,
        FILE_NEW,
        FILE_READ,
        FILE_RENAME,
        FILE_WRITE,
        FILE_DELETE,
        DLL_LOAD,
        REGISTRY_CREATE,
        REGISTRY_READ,
        REGISTRY_WRITE,
        REGISTRY_DELETE,
        NETWORK_CONNECT,
        NETWORK_LISTEN,
        NETWORK_SEND,
        NETWORK_RECEIVE,
        MEMORY_ALLOCATE,
        MEMORY_PROTECT,
        MEMORY_WRITE,
        COUNT
    };

    enum EventProp {
        PROCESS_IMAGE,
        PROCESS_CMD,
        FILE_PATH,
        FILE_NAME,
        FILE_CREATETIME,
        FILE_MODTIME,
        REG_PATH,
        REG_VALUE,
        COUNT
    };

    enum MatchType {
        MATCH_CONTAINS,
        MATCH_START,
        MATCH_END,
        MATCH_ALL,
        MATCH_PART,
        COUNT
    };

    class APTEvent
    {
    public:
        explicit APTEvent(std::string json);
        ~APTEvent();

        bool Match(EventType type, MatchType match, EventProp epindex, std::string match_value);

    private:
        unsigned int event_type_;
        std::vector<std::string> event_prop_;
    };

} // namespace malware_analysis