// Event.h - �����¼����ͺͽṹ
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <functional>
#include <unordered_map>

namespace malware_analysis {

    // �¼����Ͷ���
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

    // ��EventTypeת��Ϊ�ַ���
    std::string eventTypeToString(EventType type);

    // �¼���ϸ��Ϣ�Ļ���
    struct EventDetails {
        virtual ~EventDetails() = default;
        virtual std::string toString() const = 0;
    };

    // �����¼�����
    struct ProcessEventDetails : public EventDetails {
        uint32_t parentProcessId;
        std::string commandLine;
        std::string executablePath;
        bool elevated;

        std::string toString() const override;
    };

    // �ļ��¼�����
    struct FileEventDetails : public EventDetails {
        std::string filePath;
        uint64_t fileSize;
        bool success;
        std::optional<std::string> fileHash;

        std::string toString() const override;
    };

    // DLL�����¼�����
    struct DllEventDetails : public EventDetails {
        std::string dllName;
        std::string dllPath;
        bool success;
        std::optional<std::string> dllHash;

        std::string toString() const override;
    };

    // �����¼�����
    struct NetworkEventDetails : public EventDetails {
        std::string remoteAddress;
        uint16_t remotePort;
        std::string protocol;
        bool success;
        uint64_t dataSize;

        std::string toString() const override;
    };

    // �ڴ��¼�����
    struct MemoryEventDetails : public EventDetails {
        uint64_t address;
        uint64_t size;
        uint32_t protection; // �ڴ汣����־
        bool isExecutable;

        std::string toString() const override;
    };

    // �¼�������Ϣ
    struct Event {
        uint32_t processId;
        std::string processName;
        EventType type;
        std::chrono::system_clock::time_point timestamp;
        std::shared_ptr<EventDetails> details;

        Event(uint32_t pid, std::string name, EventType eventType,
            std::shared_ptr<EventDetails> eventDetails);

        // ת��Ϊ�ַ���������־��¼
        std::string toString() const;

        // ת��ΪJSON��ʽ
        std::string toJson() const;

        // ��JSON�����¼�
        static Event fromJson(const std::string& json);
    };

    // �������������ռ�
    namespace filters {
        // �����ļ�·��ƥ�������
        std::function<bool(const Event&)> filePathContains(const std::string& substring);

        // ����DLL����ƥ�������
        std::function<bool(const Event&)> dllNameEquals(const std::string& dllName);

        // ������������ƥ�������
        std::function<bool(const Event&)> networkConnectTo(const std::string& address,
            std::optional<uint16_t> port = std::nullopt);

        // ���������а���������
        std::function<bool(const Event&)> commandLineContains(const std::string& substring);

        // ����������ƥ�������
        std::function<bool(const Event&)> processNameEquals(const std::string& processName);
    }

} // namespace malware_analysis