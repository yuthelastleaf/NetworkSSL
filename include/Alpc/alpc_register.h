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

    // �����ڴ�������������������
    CJSONHandler& json_;
    AlpcConn* alpc_;
    ULONG msg_id_;
};

class AlpcHandler {
public:
    // ��ȡ�̳߳ص���ʵ��
    static AlpcHandler& getInstance() {
        static AlpcHandler instance;
        return instance;
    }

    // �����̳߳أ������߳�
    void start(size_t numThreads) {
        stopFlag = false;
        for (size_t i = 0; i < numThreads; ++i) {
            workers.push_back(std::thread(&AlpcHandler::workerThread, this));
        }
    }

    // ֹͣ�̳߳أ��ȴ��߳̽���
    void stop() {
        stopFlag = true;
        cv.notify_all();  // ���������߳̽�������
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // �ύ�����̳߳�
    void submit(const std::wstring& taskName, std::shared_ptr<Context> context) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskMap.find(taskName) != taskMap.end()) {
            // �ҵ��������͵��������
            tasks.push({ taskName, context });
            cv.notify_one();  // ����һ���ȴ����߳�����������
        }
        else {
            std::cout << "Task not found: " << taskName << std::endl;
        }
    }

    // ע�������̳߳�
    void registerTask(const std::wstring& taskName, std::function<void(std::shared_ptr<Context>)> taskFunc) {
        std::lock_guard<std::mutex> lock(taskMapMutex);
        taskMap[taskName] = taskFunc;
    }

private:
    AlpcHandler() : stopFlag(false) {}

    // �����̴߳�������ĺ���
    void workerThread() {
        while (true) {
            std::pair<std::wstring, std::shared_ptr<Context>> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);

                // �ȴ���������������񣬻����̳߳ر�ֹͣ
                cv.wait(lock, [this] { return !tasks.empty() || stopFlag; });

                // ����̳߳�ֹͣ�Ҷ���Ϊ�գ����˳�
                if (stopFlag && tasks.empty()) {
                    break;
                }

                // �Ӷ����л�ȡ����
                task = tasks.front();
                tasks.pop();
            }

            // ��ȡ��Ӧ����������ִ��
            {
                std::lock_guard<std::mutex> lock(taskMapMutex);
                auto it = taskMap.find(task.first);
                if (it != taskMap.end()) {
                    it->second(task.second);  // ִ������
                }
            }
        }
    }

    // ��ֹ�����͸�ֵ
    AlpcHandler(const AlpcHandler&) = delete;
    AlpcHandler& operator=(const AlpcHandler&) = delete;

    std::vector<std::thread> workers;  // �����߳�
    std::queue<std::pair<std::wstring, std::shared_ptr<Context>>> tasks; // �������
    std::mutex queueMutex;             // ����������е���
    std::condition_variable cv;        // ���������������߳�ͬ��
    std::atomic<bool> stopFlag;        // ֹͣ��־
    std::map<std::wstring, std::function<void(std::shared_ptr<Context>)>> taskMap;  // ��������ӳ��
    std::mutex taskMapMutex;           // ��������ӳ�����
};

