#include "ActionChain.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace malware_analysis {

    using json = nlohmann::json;

    BehaviorChain::BehaviorChain(uint32_t pid, std::string name)
        : processId(pid), processName(std::move(name)),
        startTime(std::chrono::system_clock::now()), isFinished(false) {}

    void BehaviorChain::addEvent(const Event& event) {
        std::lock_guard<std::mutex> lock(eventsMutex);
        events.push_back(event);
    }

    void BehaviorChain::addChildProcess(std::shared_ptr<BehaviorChain> child) {
        std::lock_guard<std::mutex> lock(childrenMutex);
        childProcesses[child->getProcessId()] = child;
    }

    void BehaviorChain::terminate() {
        endTime = std::chrono::system_clock::now();
        isFinished = true;
    }

    uint32_t BehaviorChain::getProcessId() const {
        return processId;
    }

    const std::string& BehaviorChain::getProcessName() const {
        return processName;
    }

    bool BehaviorChain::isTerminated() const {
        return isFinished;
    }

    std::vector<Event> BehaviorChain::getAllEvents() const {
        std::vector<Event> allEvents;

        {
            std::lock_guard<std::mutex> lock(eventsMutex);
            allEvents.insert(allEvents.end(), events.begin(), events.end());
        }

        {
            std::lock_guard<std::mutex> lock(childrenMutex);
            for (const auto& [childPid, childChain] : childProcesses) {
                auto childEvents = childChain->getAllEvents();
                allEvents.insert(allEvents.end(), childEvents.begin(), childEvents.end());
            }
        }

        return allEvents;
    }

    std::vector<Event> BehaviorChain::getDirectEvents() const {
        std::lock_guard<std::mutex> lock(eventsMutex);
        return events;
    }

    std::map<uint32_t, std::shared_ptr<BehaviorChain>> BehaviorChain::getChildProcesses() const {
        std::lock_guard<std::mutex> lock(childrenMutex);
        return childProcesses;
    }

    std::chrono::milliseconds BehaviorChain::getDuration() const {
        auto end = isFinished ? endTime : std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime);
    }

    std::string BehaviorChain::toJson() const {
        json j;
        j["processId"] = processId;
        j["processName"] = processName;

        // 转换时间戳
        auto startTimet = std::chrono::system_clock::to_time_t(startTime);
        std::stringstream ssStart;
        std::tm tmStart;
        gmtime_s(&tmStart, &startTimet);
        ssStart << std::put_time(&tmStart, "%FT%TZ");
        j["startTime"] = ssStart.str();

        if (isFinished) {
            auto endTimet = std::chrono::system_clock::to_time_t(endTime);
            std::stringstream ssEnd;
            std::tm tmEnd;
            gmtime_s(&tmEnd, &endTimet);
            ssEnd << std::put_time(&tmEnd, "%FT%TZ");
            j["endTime"] = ssEnd.str();
        }
        else {
            j["endTime"] = nullptr;
        }

        j["terminated"] = static_cast<bool>(isFinished);

        // 添加事件
        j["events"] = json::array();
        {
            std::lock_guard<std::mutex> lock(eventsMutex);
            for (const auto& event : events) {
                j["events"].push_back(json::parse(event.toJson()));
            }
        }

        // 添加子进程
        j["childProcesses"] = json::object();
        {
            std::lock_guard<std::mutex> lock(childrenMutex);
            for (const auto& [childPid, childChain] : childProcesses) {
                j["childProcesses"][std::to_string(childPid)] = json::parse(childChain->toJson());
            }
        }

        return j.dump(2);
    }

    std::shared_ptr<BehaviorChain> BehaviorChain::fromJson(const std::string& jsonStr) {
        json j = json::parse(jsonStr);

        uint32_t pid = j["processId"];
        std::string name = j["processName"];

        auto chain = std::make_shared<BehaviorChain>(pid, name);

        // 解析时间戳
        if (j.contains("startTime") && !j["startTime"].is_null()) {
            std::string startTimeStr = j["startTime"];
            std::tm tm = {};
            std::istringstream ss(startTimeStr);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            chain->startTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }

        if (j.contains("endTime") && !j["endTime"].is_null()) {
            std::string endTimeStr = j["endTime"];
            std::tm tm = {};
            std::istringstream ss(endTimeStr);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            chain->endTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            chain->isFinished = true;
        }

        // 解析事件
        if (j.contains("events") && j["events"].is_array()) {
            for (const auto& eventJson : j["events"]) {
                chain->events.push_back(Event::fromJson(eventJson.dump()));
            }
        }

        // 解析子进程
        if (j.contains("childProcesses") && j["childProcesses"].is_object()) {
            for (auto it = j["childProcesses"].begin(); it != j["childProcesses"].end(); ++it) {
                uint32_t childPid = std::stoi(it.key());
                auto childChain = BehaviorChain::fromJson(it.value().dump());
                if (childChain) {
                    chain->childProcesses[childPid] = childChain;
                }
            }
        }

        return chain;
    }

} // namespace malware_analysis