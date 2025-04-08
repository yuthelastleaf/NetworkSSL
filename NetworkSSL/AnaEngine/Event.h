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

    enum class EventType {
        EVENT_NULL,
        PROCESS_CREATE,
        PROCESS_STOP,
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

    enum class EventProp {
        PROCESS_IMAGE,
        PROCESS_PARENT_IMAGE,
        PROCESS_CMD,
        FILE_SOURCE_NAME,
        FILE_TARGET_NAME,
        FILE_ORIGINAL_NAME,
        FILE_CREATETIME,
        FILE_MODTIME,
        REG_PATH,
        REG_VALUE,
        COUNT
    };

    enum class MatchType {
        MATCH_COMPLETE,
        MATCH_CONTAINS,
        MATCH_START,
        MATCH_END,
        MATCH_ALL,
        MATCH_PART,
        MATCH_RULE,
        COUNT
    };

    static const std::unordered_map<std::string_view, EventType> String2EventType = {
        {"proccreate",     EventType::PROCESS_CREATE},
        {"procterminate", EventType::PROCESS_STOP},
        {"filenew",           EventType::FILE_NEW},
        {"fileread",          EventType::FILE_READ},
        {"filerename",        EventType::FILE_RENAME},
        {"filewrite",       EventType::FILE_WRITE},
        {"filedelete",        EventType::FILE_DELETE},
        {"dllload",          EventType::DLL_LOAD},
        {"regcreate",    EventType::REGISTRY_CREATE},
        {"regread",     EventType::REGISTRY_READ},
        {"regwrite",    EventType::REGISTRY_WRITE},
        {"regdelete",    EventType::REGISTRY_DELETE},
        {"netconnect",   EventType::NETWORK_CONNECT},
        {"netlisten",     EventType::NETWORK_LISTEN},
        {"netsend",       EventType::NETWORK_SEND},
        {"netreceive",    EventType::NETWORK_RECEIVE},
        {"memalloc",    EventType::MEMORY_ALLOCATE},
        {"memprotect",     EventType::MEMORY_PROTECT},
        {"memwrite",      EventType::MEMORY_WRITE}
    };

    static const std::unordered_map<std::string_view, EventProp> String2EventProp = {
        {"Image",    EventProp::PROCESS_IMAGE},
        {"ParentImage",    EventProp::PROCESS_PARENT_IMAGE},
        {"CommandLine",      EventProp::PROCESS_CMD},
        {"FileName",       EventProp::FILE_SOURCE_NAME},
        {"SourceFilename",       EventProp::FILE_SOURCE_NAME},
        {"TargetFilename",       EventProp::FILE_TARGET_NAME},
        {"OriginalFileName",       EventProp::FILE_ORIGINAL_NAME},
        {"filecreatetime", EventProp::FILE_CREATETIME},
        {"filemodtime",    EventProp::FILE_MODTIME},
        {"TargetObject",        EventProp::REG_PATH},
        {"Details",       EventProp::REG_VALUE}
    };

    static const std::unordered_map<std::string_view, MatchType> String2MatchType = {
        {"contains", MatchType::MATCH_CONTAINS},
        {"startswith",    MatchType::MATCH_START},
        {"endswith",     MatchType::MATCH_END}
    };

    // 字符串 → 枚举值（安全版，返回 optional）
    std::optional<EventType> ToEventType(std::string_view str) {
        if (auto it = String2EventType.find(str); it != String2EventType.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<EventProp> ToEventProp(std::string_view str) {
        if (auto it = String2EventProp.find(str); it != String2EventProp.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<MatchType> ToMatchType(std::string_view str) {
        if (auto it = String2MatchType.find(str); it != String2MatchType.end()) {
            return it->second;
        }
        return std::nullopt;
    }


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