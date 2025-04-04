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
        return LogicOperator::AND; // Ĭ��
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
            // ��������������ƥ������һ���¼�
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
            // ����һ����������ƥ������һ���¼�
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
            // �������밴˳��ƥ��
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

        // ע�⣺���������޷�ֱ�����л���JSON
        // �������¼����������ʵ�ʵ����л���Ҫ�Զ���
        j["conditionsCount"] = conditions.size();

        return j.dump(2);
    }

    std::string CompositeRule::serialize() const {
        // ���л����������Ϣ
        json j = json::parse(Rule::toJson());
        j["type"] = "composite";
        j["logicOp"] = logicOpToString(logicOp);

        // ����Ӧ����һ�ַ�ʽ�����л�����
        // ʵ��ʵ���У����ǿ��ܻᶨ��һ��Ԥ������������ͺͲ���
        // Ȼ��ͨ�����ͺͲ��������л�����
        j["conditions"] = json::array();

        // ע����ֻ��ʾ����ʵ������Ҫ����һ�ָ����ӵķ��������л�����
        // ���磬����Զ���һ���������ͺͲ�����ӳ���ϵ
        // �������Ǽ������ڻ��޷����л�����

        return j.dump(2);
    }

    std::function<bool(const Event&)> CompositeRule::createCondition(const std::string& type, const std::string& param) {
        // ����һ���������������ڴ����ͺͲ���������������
        // ʵ��ʵ���У�����Ҫ��չ���������֧�ָ������͵�����

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
            // ����򻯴���ʵ���п�����Ҫ������ַ�Ͷ˿�
            return filters::networkConnectTo(param);
        }

        // Ĭ�Ϸ���һ��ʼ�շ���false������
        return [](const Event&) { return false; };
    }

} // namespace malware_analysis