// CompositeRule.h - 复合规则实现
#pragma once

#include "Rule.h"
#include <vector>
#include <functional>

namespace malware_analysis {

    // 复合规则 - 组合多个条件
    class CompositeRule : public Rule {
    public:
        enum class LogicOperator {
            AND,     // 所有条件都必须满足
            OR,      // 至少一个条件必须满足
            SEQUENCE // 条件必须按顺序满足
        };

        // 将逻辑运算符转换为字符串
        static std::string logicOpToString(LogicOperator op);

        // 将字符串转换为逻辑运算符
        static LogicOperator stringToLogicOp(const std::string& str);

        // 构造函数
        CompositeRule(std::string id, std::string name, LogicOperator op = LogicOperator::AND);

        // 默认构造函数
        CompositeRule();

        // 析构函数
        ~CompositeRule() override = default;

        // 添加条件
        void addCondition(std::function<bool(const Event&)> condition);

        // 分析行为链
        std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const override;

        // 获取逻辑运算符
        LogicOperator getLogicOp() const;

        // 设置逻辑运算符
        void setLogicOp(LogicOperator op);

        // 序列化为JSON
        std::string toJson() const override;

        // 序列化规则 (这需要特殊处理，因为函数不能直接序列化)
        std::string serialize() const override;

        // 创建条件工厂
        static std::function<bool(const Event&)> createCondition(const std::string& type, const std::string& param);

    private:
        LogicOperator logicOp;
        std::vector<std::function<bool(const Event&)>> conditions;
    };

} // namespace malware_analysis