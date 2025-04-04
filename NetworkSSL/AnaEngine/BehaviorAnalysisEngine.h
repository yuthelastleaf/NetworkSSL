// BehaviorAnalysisEngine.h - ��Ϊ�����������Ҫͷ�ļ�
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

    // ǰ������
    class RuleEngine;

    // ��Ϊ�������� - ��Ҫ�������
    class BehaviorAnalysisEngine {
    public:
        // ���캯��
        BehaviorAnalysisEngine();

        // ��������
        ~BehaviorAnalysisEngine();

        // ע��һ��������Ϊ��
        std::shared_ptr<BehaviorChain> registerProcess(uint32_t pid, std::string name,
            std::optional<uint32_t> parentPid = std::nullopt);

        // ��¼һ���¼�
        void recordEvent(const Event& event);

        // ��ǽ�����ֹ
        void terminateProcess(uint32_t pid);

        // ��������������Ϊ��
        std::vector<RuleMatchResult> analyzeProcess(uint32_t pid) const;

        // �������н�����Ϊ��
        std::map<uint32_t, std::vector<RuleMatchResult>> analyzeAllProcesses() const;

        // �����Զ������
        void loadRules(const std::string& rulesDirectory);

        // ����Զ������
        void addRule(std::shared_ptr<Rule> rule);

        // ����������
        bool saveResults(const std::string& filename,
            const std::map<uint32_t, std::vector<RuleMatchResult>>& results) const;

        // ���ؽ�����Ϊ��
        bool loadBehaviorChains(const std::string& filename);

        // ���������Ϊ��
        bool saveBehaviorChains(const std::string& filename) const;

        // �����¼��ص�
        using EventCallback = std::function<void(const Event&)>;
        void setEventCallback(EventCallback callback);

        // ���ý���ص�
        using ResultCallback = std::function<void(uint32_t, const std::vector<RuleMatchResult>&)>;
        void setResultCallback(ResultCallback callback);

    private:
        // ��ʼ�����ù���
        void initializeBuiltinRules();

        mutable std::mutex processesMutex;
        std::unordered_map<uint32_t, std::shared_ptr<BehaviorChain>> activeProcesses;
        std::unordered_map<uint32_t, std::shared_ptr<BehaviorChain>> rootProcesses;
        std::unique_ptr<RuleEngine> ruleEngine;

        EventCallback eventCallback;
        ResultCallback resultCallback;
    };

    // �������� - �����Ӧ�ù���
    class RuleEngine {
    public:
        // ���캯��
        RuleEngine();

        // ��������
        ~RuleEngine();

        // ��ӹ���
        void addRule(std::shared_ptr<Rule> rule);

        // ��Ŀ¼���ع���
        void loadRulesFromDirectory(const std::string& directory);

        // ������Ϊ��
        std::vector<RuleMatchResult> analyzeChain(const BehaviorChain& chain) const;

        // ��ȡ���й���
        std::vector<std::shared_ptr<Rule>> getRules() const;

        // ������й���
        void clearRules();

        // ��������ļ�
        bool saveRulesToFile(const std::string& filePath) const;

        // ���ļ����ع���
        bool loadRulesFromFile(const std::string& filePath);

    private:
        std::vector<std::shared_ptr<Rule>> rules;
        mutable std::mutex rulesMutex;
        std::unique_ptr<RuleFactory> ruleFactory;
    };

} // namespace malware_analysis