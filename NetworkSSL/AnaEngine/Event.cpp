#include "Event.h"
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <ctime>

namespace malware_analysis {

    using json = nlohmann::json;

    std::string eventTypeToString(EventType type) {
        switch (type) {
        case EventType::PROCESS_CREATE: return "PROCESS_CREATE";
        case EventType::PROCESS_TERMINATE: return "PROCESS_TERMINATE";
        case EventType::FILE_READ: return "FILE_READ";
        case EventType::FILE_WRITE: return "FILE_WRITE";
        case EventType::FILE_DELETE: return "FILE_DELETE";
        case EventType::DLL_LOAD: return "DLL_LOAD";
        case EventType::REGISTRY_READ: return "REGISTRY_READ";
        case EventType::REGISTRY_WRITE: return "REGISTRY_WRITE";
        case EventType::NETWORK_CONNECT: return "NETWORK_CONNECT";
        case EventType::NETWORK_LISTEN: return "NETWORK_LISTEN";
        case EventType::NETWORK_SEND: return "NETWORK_SEND";
        case EventType::NETWORK_RECEIVE: return "NETWORK_RECEIVE";
        case EventType::MEMORY_ALLOCATE: return "MEMORY_ALLOCATE";
        case EventType::MEMORY_PROTECT: return "MEMORY_PROTECT";
        case EventType::MEMORY_WRITE: return "MEMORY_WRITE";
        default: return "UNKNOWN";
        }
    }

    std::string ProcessEventDetails::toString() const {
        return "ParentPID: " + std::to_string(parentProcessId) +
            ", CommandLine: " + commandLine +
            ", Path: " + executablePath +
            ", Elevated: " + (elevated ? "Yes" : "No");
    }

    std::string FileEventDetails::toString() const {
        return "Path: " + filePath +
            ", Size: " + std::to_string(fileSize) +
            ", Success: " + (success ? "Yes" : "No") +
            (fileHash.has_value() ? ", Hash: " + fileHash.value() : "");
    }

    std::string DllEventDetails::toString() const {
        return "Name: " + dllName +
            ", Path: " + dllPath +
            ", Success: " + (success ? "Yes" : "No") +
            (dllHash.has_value() ? ", Hash: " + dllHash.value() : "");
    }

    std::string NetworkEventDetails::toString() const {
        return "Remote: " + remoteAddress + ":" + std::to_string(remotePort) +
            ", Protocol: " + protocol +
            ", Success: " + (success ? "Yes" : "No") +
            ", DataSize: " + std::to_string(dataSize);
    }

    std::string MemoryEventDetails::toString() const {
        std::stringstream ss;
        ss << "Address: 0x" << std::hex << address
            << ", Size: " << std::dec << size
            << ", Protection: 0x" << std::hex << protection
            << ", Executable: " << (isExecutable ? "Yes" : "No");
        return ss.str();
    }

    Event::Event(uint32_t pid, std::string name, EventType eventType,
        std::shared_ptr<EventDetails> eventDetails)
        : processId(pid), processName(std::move(name)), type(eventType),
        timestamp(std::chrono::system_clock::now()), details(std::move(eventDetails)) {}

    std::string Event::toString() const {
        auto time_t_point = std::chrono::system_clock::to_time_t(timestamp);
        std::stringstream ss;
        tm now_time;
        localtime_s(&now_time, &time_t_point);
        
        ss << "[" << std::put_time(&now_time, "%Y-%m-%d %H:%M:%S") << "] "
            << "PID " << processId << " (" << processName << ") "
            << eventTypeToString(type) << ": ";

        if (details) {
            ss << details->toString();
        }

        return ss.str();
    }

    std::string Event::toJson() const {
        std::stringstream ss;
        ss << "{";
        ss << "\"timestamp\":\"";
        
        // 使用 localtime_s 替代 localtime
        auto time_t_point = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm;
        localtime_s(&tm, &time_t_point);
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        ss << "\",\"type\":\"" << static_cast<unsigned int>(type) << "\",\"details\":\"" << details << "\"";
        ss << "}";
        return ss.str();
    }

    Event Event::fromJson(const std::string& jsonStr) {
        // 解析 JSON 字符串
        size_t timestampStart = jsonStr.find("\"timestamp\":\"") + 12;
        size_t timestampEnd = jsonStr.find("\"", timestampStart);
        std::string timestampStr = jsonStr.substr(timestampStart, timestampEnd - timestampStart);

        size_t typeStart = jsonStr.find("\"type\":\"", timestampEnd) + 8;
        size_t typeEnd = jsonStr.find("\"", typeStart);
        std::string typeStr = jsonStr.substr(typeStart, typeEnd - typeStart);

        size_t detailsStart = jsonStr.find("\"details\":\"", typeEnd) + 10;
        size_t detailsEnd = jsonStr.find("\"", detailsStart);
        std::string detailsStr = jsonStr.substr(detailsStart, detailsEnd - detailsStart);

        // 解析时间戳
        std::tm tm = {};
        std::istringstream ss(timestampStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        time_t timestamp = std::mktime(&tm);

        return Event(timestamp, typeStr, detailsStr);
    }

    namespace filters {

        std::function<bool(const Event&)> filePathContains(const std::string& substring) {
            return [substring](const Event& event) {
                if (event.type != EventType::FILE_READ &&
                    event.type != EventType::FILE_WRITE &&
                    event.type != EventType::FILE_DELETE) return false;

                auto details = std::dynamic_pointer_cast<FileEventDetails>(event.details);
                if (!details) return false;

                return details->filePath.find(substring) != std::string::npos;
                };
        }

        std::function<bool(const Event&)> dllNameEquals(const std::string& dllName) {
            return [dllName](const Event& event) {
                if (event.type != EventType::DLL_LOAD) return false;

                auto details = std::dynamic_pointer_cast<DllEventDetails>(event.details);
                if (!details) return false;

                return details->dllName == dllName;
                };
        }

        std::function<bool(const Event&)> networkConnectTo(const std::string& address, std::optional<uint16_t> port) {
            return [address, port](const Event& event) {
                if (event.type != EventType::NETWORK_CONNECT) return false;

                auto details = std::dynamic_pointer_cast<NetworkEventDetails>(event.details);
                if (!details) return false;

                bool addressMatch = details->remoteAddress.find(address) != std::string::npos;
                if (!addressMatch) return false;

                if (port) {
                    return details->remotePort == *port;
                }

                return true;
                };
        }

        std::function<bool(const Event&)> commandLineContains(const std::string& substring) {
            return [substring](const Event& event) {
                if (event.type != EventType::PROCESS_CREATE) return false;

                auto details = std::dynamic_pointer_cast<ProcessEventDetails>(event.details);
                if (!details) return false;

                return details->commandLine.find(substring) != std::string::npos;
                };
        }

        std::function<bool(const Event&)> processNameEquals(const std::string& processName) {
            return [processName](const Event& event) {
                return event.processName == processName;
                };
        }
    }

} // namespace malware_analysis