// CompositeRule.h - ���Ϲ���ʵ��
#pragma once

#include "Rule.h"
#include <vector>
#include <functional>

namespace malware_analysis {

    // ���Ϲ��� - ��϶������
    class CompositeRule : public Rule {
    public:
        enum class LogicOperator {
            AND,     // ������������������
            OR,      // ����һ��������������
            SEQUENCE // �������밴˳������
        };

        // ���߼������ת��Ϊ�ַ���
        static std::string logicOpToString(LogicOperator op);

        // ���ַ���ת��Ϊ�߼������
        static LogicOperator stringToLogicOp(const std::string& str);

        // ���캯��
        CompositeRule(std::string id, std::string name, LogicOperator op = LogicOperator::AND);

        // Ĭ�Ϲ��캯��
        CompositeRule();

        // ��������
        ~CompositeRule() override = default;

        // �������
        void addCondition(std::function<bool(const Event&)> condition);

        // ������Ϊ��
        std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const override;

        // ��ȡ�߼������
        LogicOperator getLogicOp() const;

        // �����߼������
        void setLogicOp(LogicOperator op);

        // ���л�ΪJSON
        std::string toJson() const override;

        // ���л����� (����Ҫ���⴦����Ϊ��������ֱ�����л�)
        std::string serialize() const override;

        // ������������
        static std::function<bool(const Event&)> createCondition(const std::string& type, const std::string& param);

    private:
        LogicOperator logicOp;
        std::vector<std::function<bool(const Event&)>> conditions;
    };

} // namespace malware_analysis