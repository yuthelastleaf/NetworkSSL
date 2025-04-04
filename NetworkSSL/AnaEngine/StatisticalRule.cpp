
#include "StatisticalRule.h"
#include "Event.h"
#include "ActionChain.h"

#include <nlohmann/json.hpp>

namespace malware_analysis {

    using json = nlohmann::json;

    StatisticalRule::StatisticalRule(std::string id, std::string name,
        std::function<bool(const Event&)> eventFilter,
        size_t threshold)
        : eventFilter(std::move(eventFilter)), threshold(threshold) {
        this->id = std::move(id);
        this->name = std::move(name);
    }

    StatisticalRule::StatisticalRule() : threshold(0) {
        // 默认过滤器总是返回false
        eventFilter = [](const Event&) { return false; };
    }

    void StatisticalRule::setEventFilter(std::function<bool(const Event&)> filter) {
        eventFilter = std::move(filter);
    }

    void StatisticalRule::setThreshold(size_t threshold) {
        this->threshold = threshold;
    }

    size_t StatisticalRule::getThreshold() const {
        return threshold;
    }

    std::optional<RuleMatchResult> StatisticalRule::analyze(const BehaviorChain& chain) const {
        auto events = chain.getAllEvents();
        std::vector<Event> matchedEvents;

        for (const auto& event : events) {
            if (eventFilter(event)) {
                matchedEvents.push_back(event);
            }
        }

        if (matchedEvents.size() >= threshold) {
            RuleMatchResult result;
            result.ruleId = id;
            result.ruleName = name;
            result.description = description;
            result.severity = severity;
            result.matchedEvents = std::move(matchedEvents);
            result.metadata = metadata;
            result.mitreTactic = mitreTactic;
            result.mitreTechnique = mitreTechnique;

            return result;
        }

        return std::nullopt;
    }

    std::string StatisticalRule::toJson() const {
        json j = json::parse(Rule::toJson());
        j["type"] = "statistical";
        j["threshold"] = threshold;

        // 注意：事件过滤器无法直接序列化到JSON

        return j.dump(2);
    }

    std::string StatisticalRule::serialize() const {
        // 序列化规则基本信息
        json j = json::parse(Rule::toJson());
        j["type"] = "statistical";
        j["threshold"] = threshold;

        // 事件过滤器的序列化与CompositeRule中的条件序列化类似
        // 需要开发一种方式来序列化函数

        return j.dump(2);
    }

} // namespace malware_analysis