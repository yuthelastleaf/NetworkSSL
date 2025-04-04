// StatisticalRule.h - ͳ�ƹ���ʵ��
#pragma once

#include "Rule.h"
#include <functional>

namespace malware_analysis {

    // ͳ�ƹ��� - �����¼�������Ƶ��
    class StatisticalRule : public Rule {
    public:
        // ���캯��
        StatisticalRule(std::string id, std::string name,
            std::function<bool(const Event&)> eventFilter,
            size_t threshold);

        // Ĭ�Ϲ��캯��
        StatisticalRule();

        // ��������
        ~StatisticalRule() override = default;

        // �����¼�������
        void setEventFilter(std::function<bool(const Event&)> filter);

        // ������ֵ
        void setThreshold(size_t threshold);

        // ��ȡ��ֵ
        size_t getThreshold() const;

        // ������Ϊ��
        std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const override;

        // ���л�ΪJSON
        std::string toJson() const override;

        // ���л�����
        std::string serialize() const override;

    private:
        std::function<bool(const Event&)> eventFilter;
        size_t threshold;
    };

} // namespace malware_analysis