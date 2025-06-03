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
    void submit(std::wstring task_name, std::shared_ptr<void> context) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskMap.find(task_name) != taskMap.end() ||
            taskMap.find(L"*") != taskMap.end()) {
            // �ҵ��������͵��������
            tasks.push({ task_name, context });
            cv.notify_one();  // ����һ���ȴ����߳�����������
        }
    }

    // ע�������̳߳�
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
                task = it->second;  // ִ������
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

    // �����̴߳�������ĺ���
    void workerThread() {
        while (true) {
            std::pair<std::wstring, std::shared_ptr<void>> task;
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
                auto def_it = taskMap.find(L"*");
                if (it != taskMap.end()) {
                    it->second(task.second);  // ִ������
                }
                else if (def_it != taskMap.end()) {
                    def_it->second(task.second);
                }
            }
        }
    }

    // ��ֹ�����͸�ֵ
    AlpcHandler(const AlpcHandler&) = delete;
    AlpcHandler& operator=(const AlpcHandler&) = delete;

    std::vector<std::thread> workers;  // �����߳�
    std::queue<std::pair<std::wstring, std::shared_ptr<void>>> tasks; // �������
    std::mutex queueMutex;             // ����������е���
    std::condition_variable cv;        // ���������������߳�ͬ��
    std::atomic<bool> stopFlag;        // ֹͣ��־
    std::map<std::wstring, std::function<void(std::shared_ptr<void>)>> taskMap;  // ��������ӳ��
    std::mutex taskMapMutex;           // ��������ӳ�����
};

