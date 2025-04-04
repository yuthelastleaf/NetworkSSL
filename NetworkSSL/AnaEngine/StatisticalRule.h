// StatisticalRule.h - 统计规则实现
#pragma once

#include "Rule.h"
#include <functional>

namespace malware_analysis {

    // 统计规则 - 基于事件数量或频率
    class StatisticalRule : public Rule {
    public:
        // 构造函数
        StatisticalRule(std::string id, std::string name,
            std::function<bool(const Event&)> eventFilter,
            size_t threshold);

        // 默认构造函数
        StatisticalRule();

        // 析构函数
        ~StatisticalRule() override = default;

        // 设置事件过滤器
        void setEventFilter(std::function<bool(const Event&)> filter);

        // 设置阈值
        void setThreshold(size_t threshold);

        // 获取阈值
        size_t getThreshold() const;

        // 分析行为链
        std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const override;

        // 序列化为JSON
        std::string toJson() const override;

        // 序列化规则
        std::string serialize() const override;

    private:
        std::function<bool(const Event&)> eventFilter;
        size_t threshold;
    };

} // namespace malware_analysis