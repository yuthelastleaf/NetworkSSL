// Rule.h - 定义规则接口和相关结构
#pragma once

#include "ActionChain.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace malware_analysis {

    // 规则严重性级别
    enum class SeverityLevel {
        INFORMATIONAL,
        LOW,
        MEDIUM,
        HIGH,
        CRITICAL
    };

    // 将严重性级别转换为字符串
    std::string severityToString(SeverityLevel level);

    // 将字符串转换为严重性级别
    SeverityLevel stringToSeverity(const std::string& str);

    // 规则匹配结果
    struct RuleMatchResult {
        std::string ruleId;
        std::string ruleName;
        std::string description;
        SeverityLevel severity;
        std::vector<Event> matchedEvents;
        std::map<std::string, std::string> metadata;

        // 可选的MITRE ATT&CK映射
        std::optional<std::string> mitreTactic;
        std::optional<std::string> mitreTechnique;

        // 转换为JSON
        std::string toJson() const;

        // 从JSON创建结果对象
        static RuleMatchResult fromJson(const std::string& json);
    };

    // 抽象规则接口
    class Rule {
    public:
        virtual ~Rule() = default;

        // 分析一个行为链并返回匹配结果（如果匹配）
        virtual std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const = 0;

        // 获取规则ID
        const std::string& getId() const;

        // 获取规则名称
        const std::string& getName() const;

        // 获取规则描述
        const std::string& getDescription() const;

        // 获取规则严重性
        SeverityLevel getSeverity() const;

        // 设置规则ID
        void setId(const std::string& id);

        // 设置规则名称
        void setName(const std::string& name);

        // 设置规则描述
        void setDescription(const std::string& description);

        // 设置规则严重性
        void setSeverity(SeverityLevel severity);

        // 设置MITRE ATT&CK战术
        void setMitreTactic(const std::string& tactic);

        // 设置MITRE ATT&CK技术
        void setMitreTechnique(const std::string& technique);

        // 添加元数据
        void addMetadata(const std::string& key, const std::string& value);

        // 转换为JSON
        virtual std::string toJson() const;

        // 序列化规则
        virtual std::string serialize() const = 0;

    protected:
        std::string id;
        std::string name;
        std::string description;
        SeverityLevel severity = SeverityLevel::MEDIUM;
        std::map<std::string, std::string> metadata;

        // MITRE ATT&CK映射
        std::optional<std::string> mitreTactic;
        std::optional<std::string> mitreTechnique;
    };

    // 规则工厂接口
    class RuleFactory {
    public:
        virtual ~RuleFactory() = default;

        // 从JSON创建规则
        virtual std::shared_ptr<Rule> createFromJson(const std::string& json) = 0;

        // 从JSON文件创建规则
        virtual std::vector<std::shared_ptr<Rule>> createFromJsonFile(const std::string& filePath) = 0;

        // 从序列化字符串创建规则
        virtual std::shared_ptr<Rule> deserialize(const std::string& serialized) = 0;
    };

} // namespace malware_analysis