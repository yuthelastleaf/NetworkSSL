// BehaviorAnalysisEngine.h - 行为分析引擎的主要头文件
#pragma once

#include "ActionChain.h"
#include "Rule.h"
#include "CompositeRule.h"
#include "StatisticalRule.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace malware_analysis {

    // 前向声明
    class RuleEngine;

    // 行为分析引擎 - 主要分析组件
    class BehaviorAnalysisEngine {
    public:
        // 构造函数
        BehaviorAnalysisEngine();

        // 析构函数
        ~BehaviorAnalysisEngine();

        // 注册一个进程行为链
        std::shared_ptr<BehaviorChain> registerProcess(uint32_t pid, std::string name,
            std::optional<uint32_t> parentPid = std::nullopt);

        // 记录一个事件
        void recordEvent(const Event& event);

        // 标记进程终止
        void terminateProcess(uint32_t pid);

        // 分析单个进程行为链
        std::vector<RuleMatchResult> analyzeProcess(uint32_t pid) const;

        // 分析所有进程行为链
        std::map<uint32_t, std::vector<RuleMatchResult>> analyzeAllProcesses() const;

        // 加载自定义规则
        void loadRules(const std::string& rulesDirectory);

        // 添加自定义规则
        void addRule(std::shared_ptr<Rule> rule);

        // 保存分析结果
        bool saveResults(const std::string& filename,
            const std::map<uint32_t, std::vector<RuleMatchResult>>& results) const;

        // 加载进程行为链
        bool loadBehaviorChains(const std::string& filename);

        // 保存进程行为链
        bool saveBehaviorChains(const std::string& filename) const;

        // 设置事件回调
        using EventCallback = std::function<void(const Event&)>;
        void setEventCallback(EventCallback callback);

        // 设置结果回调
        using ResultCallback = std::function<void(uint32_t, const std::vector<RuleMatchResult>&)>;
        void setResultCallback(ResultCallback callback);

    private:
        // 初始化内置规则
        void initializeBuiltinRules();

        mutable std::mutex processesMutex;
        std::unordered_map<uint32_t, std::shared_ptr<BehaviorChain>> activeProcesses;
        std::unordered_map<uint32_t, std::shared_ptr<BehaviorChain>> rootProcesses;
        std::unique_ptr<RuleEngine> ruleEngine;

        EventCallback eventCallback;
        ResultCallback resultCallback;
    };

    // 规则引擎 - 管理和应用规则
    class RuleEngine {
    public:
        // 构造函数
        RuleEngine();

        // 析构函数
        ~RuleEngine();

        // 添加规则
        void addRule(std::shared_ptr<Rule> rule);

        // 从目录加载规则
        void loadRulesFromDirectory(const std::string& directory);

        // 分析行为链
        std::vector<RuleMatchResult> analyzeChain(const BehaviorChain& chain) const;

        // 获取所有规则
        std::vector<std::shared_ptr<Rule>> getRules() const;

        // 清除所有规则
        void clearRules();

        // 保存规则到文件
        bool saveRulesToFile(const std::string& filePath) const;

        // 从文件加载规则
        bool loadRulesFromFile(const std::string& filePath);

    private:
        std::vector<std::shared_ptr<Rule>> rules;
        mutable std::mutex rulesMutex;
        std::unique_ptr<RuleFactory> ruleFactory;
    };

} // namespace malware_analysis