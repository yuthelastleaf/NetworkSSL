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

    enum MatchType {
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
        {"proccreate",     PROCESS_CREATE},
        {"procterminate", PROCESS_TERMINATE},
        {"filenew",           FILE_NEW},
        {"fileread",          FILE_READ},
        {"filerename",        FILE_RENAME},
        {"filewrite",        FILE_WRITE},
        {"filedelete",        FILE_DELETE},
        {"dllload",          DLL_LOAD},
        {"regcreate",    REGISTRY_CREATE},
        {"regread",     REGISTRY_READ},
        {"regwrite",    REGISTRY_WRITE},
        {"regdelete",    REGISTRY_DELETE},
        {"netconnect",   NETWORK_CONNECT},
        {"netlisten",     NETWORK_LISTEN},
        {"netsend",       NETWORK_SEND},
        {"netreceive",    NETWORK_RECEIVE},
        {"memalloc",    MEMORY_ALLOCATE},
        {"memprotect",     MEMORY_PROTECT},
        {"memwrite",      MEMORY_WRITE}
    };

    static const std::unordered_map<std::string_view, EventProp> String2EventProp = {
        {"Image",    PROCESS_IMAGE},
        {"ParentImage",    PROCESS_PARENT_IMAGE},
        {"CommandLine",      PROCESS_CMD},
        {"FileName",       FILE_SOURCE_NAME},
        {"SourceFilename",       FILE_SOURCE_NAME},
        {"TargetFilename",       FILE_TARGET_NAME},
        {"OriginalFileName",       FILE_ORIGINAL_NAME},
        {"filecreatetime", FILE_CREATETIME},
        {"filemodtime",    FILE_MODTIME},
        {"TargetObject",        REG_PATH},
        {"Details",       REG_VALUE}
    };

    static const std::unordered_map<std::string_view, MatchType> String2MatchType = {
        {"contains", MATCH_CONTAINS},
        {"startswith",    MATCH_START},
        {"endswith",      MATCH_END}
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