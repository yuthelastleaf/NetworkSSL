#include "CompositeRule.h"
#include "Event.h"
#include "ActionChain.h"
#include <nlohmann/json.hpp>

namespace malware_analysis {

    using json = nlohmann::json;

    std::string CompositeRule::logicOpToString(LogicOperator op) {
        switch (op) {
        case LogicOperator::AND: return "and";
        case LogicOperator::OR: return "or";
        case LogicOperator::SEQUENCE: return "sequence";
        default: return "unknown";
        }
    }

    CompositeRule::LogicOperator CompositeRule::stringToLogicOp(const std::string& str) {
        if (str == "and") return LogicOperator::AND;
        if (str == "or") return LogicOperator::OR;
        if (str == "sequence") return LogicOperator::SEQUENCE;
        return LogicOperator::AND; // 默认
    }

    CompositeRule::CompositeRule(std::string id, std::string name, LogicOperator op)
        : logicOp(op) {
        this->id = std::move(id);
        this->name = std::move(name);
    }

    CompositeRule::CompositeRule() : logicOp(LogicOperator::AND) {}

    void CompositeRule::addCondition(std::function<bool(const Event&)> condition) {
        conditions.push_back(std::move(condition));
    }

    std::optional<RuleMatchResult> CompositeRule::analyze(const BehaviorChain& chain) const {
        auto events = chain.getAllEvents();
        std::vector<Event> matchedEvents;

        if (events.empty() || conditions.empty()) {
            return std::nullopt;
        }

        if (logicOp == LogicOperator::AND) {
            // 所有条件都必须匹配至少一个事件
            for (const auto& condition : conditions) {
                bool anyMatch = false;
                for (const auto& event : events) {
                    if (condition(event)) {
                        matchedEvents.push_back(event);
                        anyMatch = true;
                        break;
                    }
                }
                if (!anyMatch) return std::nullopt;
            }
        }
        else if (logicOp == LogicOperator::OR) {
            // 至少一个条件必须匹配至少一个事件
            bool anyMatch = false;
            for (const auto& condition : conditions) {
                for (const auto& event : events) {
                    if (condition(event)) {
                        matchedEvents.push_back(event);
                        anyMatch = true;
                        break;
                    }
                }
                if (anyMatch) break;
            }
            if (!anyMatch) return std::nullopt;
        }
        else if (logicOp == LogicOperator::SEQUENCE) {
            // 条件必须按顺序匹配
            size_t currentCondition = 0;

            for (const auto& event : events) {
                if (currentCondition >= conditions.size()) break;

                if (conditions[currentCondition](event)) {
                    matchedEvents.push_back(event);
                    currentCondition++;
                }
            }

            if (currentCondition < conditions.size()) return std::nullopt;
        }

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

    CompositeRule::LogicOperator CompositeRule::getLogicOp() const {
        return logicOp;
    }

    void CompositeRule::setLogicOp(LogicOperator op) {
        logicOp = op;
    }

    std::string CompositeRule::toJson() const {
        json j = json::parse(Rule::toJson());
        j["type"] = "composite";
        j["logicOp"] = logicOpToString(logicOp);

        // 注意：条件函数无法直接序列化到JSON
        // 这里仅记录条件数量，实际的序列化需要自定义
        j["conditionsCount"] = conditions.size();

        return j.dump(2);
    }

    std::string CompositeRule::serialize() const {
        // 序列化规则基本信息
        json j = json::parse(Rule::toJson());
        j["type"] = "composite";
        j["logicOp"] = logicOpToString(logicOp);

        // 这里应该有一种方式来序列化条件
        // 实际实现中，我们可能会定义一组预定义的条件类型和参数
        // 然后通过类型和参数来序列化条件
        j["conditions"] = json::array();

        // 注：这只是示例，实际中需要开发一种更复杂的方法来序列化函数
        // 例如，你可以定义一组条件类型和参数的映射关系
        // 这里我们假设现在还无法序列化条件

        return j.dump(2);
    }

    std::function<bool(const Event&)> CompositeRule::createCondition(const std::string& type, const std::string& param) {
        // 这是一个工厂方法，用于从类型和参数创建条件函数
        // 实际实现中，你需要扩展这个方法来支持更多类型的条件

        if (type == "filePathContains") {
            return filters::filePathContains(param);
        }
        else if (type == "dllNameEquals") {
            return filters::dllNameEquals(param);
        }
        else if (type == "processNameEquals") {
            return filters::processNameEquals(param);
        }
        else if (type == "commandLineContains") {
            return filters::commandLineContains(param);
        }
        else if (type == "networkConnectTo") {
            // 这里简化处理，实际中可能需要解析地址和端口
            return filters::networkConnectTo(param);
        }

        // 默认返回一个始终返回false的条件
        return [](const Event&) { return false; };
    }

} // namespace malware_analysis