// ActionChain.h - ���������Ϊ���ṹ
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

    // ��Ϊ����ʾһ�����̵������¼�����
    class BehaviorChain {
    public:
        BehaviorChain(uint32_t pid, std::string name);
        ~BehaviorChain() = default;

        // ����¼�������
        void addEvent(const Event& event);

        // ����ӽ���
        void addChildProcess(std::shared_ptr<BehaviorChain> child);

        // ��ǽ�����ֹ
        void terminate();

        // ��ȡ����ID
        uint32_t getProcessId() const;

        // ��ȡ��������
        const std::string& getProcessName() const;

        // �������Ƿ�����ֹ
        bool isTerminated() const;

        // ��ȡ�����¼��������ӽ��̣�
        std::vector<Event> getAllEvents() const;

        // ��ȡֱ���¼����������ӽ��̣�
        std::vector<Event> getDirectEvents() const;

        // ��ȡ�ӽ�����
        std::map<uint32_t, std::shared_ptr<BehaviorChain>> getChildProcesses() const;

        // ��ȡ�¼����еĳ���ʱ��
        std::chrono::milliseconds getDuration() const;

        // ���л�ΪJSON��ʽ
        std::string toJson() const;

        // ��JSON�����л�
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