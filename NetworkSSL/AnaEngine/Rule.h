// Rule.h - �������ӿں���ؽṹ
#pragma once

#include "ActionChain.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace malware_analysis {

    // ���������Լ���
    enum class SeverityLevel {
        INFORMATIONAL,
        LOW,
        MEDIUM,
        HIGH,
        CRITICAL
    };

    // �������Լ���ת��Ϊ�ַ���
    std::string severityToString(SeverityLevel level);

    // ���ַ���ת��Ϊ�����Լ���
    SeverityLevel stringToSeverity(const std::string& str);

    // ����ƥ����
    struct RuleMatchResult {
        std::string ruleId;
        std::string ruleName;
        std::string description;
        SeverityLevel severity;
        std::vector<Event> matchedEvents;
        std::map<std::string, std::string> metadata;

        // ��ѡ��MITRE ATT&CKӳ��
        std::optional<std::string> mitreTactic;
        std::optional<std::string> mitreTechnique;

        // ת��ΪJSON
        std::string toJson() const;

        // ��JSON�����������
        static RuleMatchResult fromJson(const std::string& json);
    };

    // �������ӿ�
    class Rule {
    public:
        virtual ~Rule() = default;

        // ����һ����Ϊ��������ƥ���������ƥ�䣩
        virtual std::optional<RuleMatchResult> analyze(const BehaviorChain& chain) const = 0;

        // ��ȡ����ID
        const std::string& getId() const;

        // ��ȡ��������
        const std::string& getName() const;

        // ��ȡ��������
        const std::string& getDescription() const;

        // ��ȡ����������
        SeverityLevel getSeverity() const;

        // ���ù���ID
        void setId(const std::string& id);

        // ���ù�������
        void setName(const std::string& name);

        // ���ù�������
        void setDescription(const std::string& description);

        // ���ù���������
        void setSeverity(SeverityLevel severity);

        // ����MITRE ATT&CKս��
        void setMitreTactic(const std::string& tactic);

        // ����MITRE ATT&CK����
        void setMitreTechnique(const std::string& technique);

        // ���Ԫ����
        void addMetadata(const std::string& key, const std::string& value);

        // ת��ΪJSON
        virtual std::string toJson() const;

        // ���л�����
        virtual std::string serialize() const = 0;

    protected:
        std::string id;
        std::string name;
        std::string description;
        SeverityLevel severity = SeverityLevel::MEDIUM;
        std::map<std::string, std::string> metadata;

        // MITRE ATT&CKӳ��
        std::optional<std::string> mitreTactic;
        std::optional<std::string> mitreTechnique;
    };

    // ���򹤳��ӿ�
    class RuleFactory {
    public:
        virtual ~RuleFactory() = default;

        // ��JSON��������
        virtual std::shared_ptr<Rule> createFromJson(const std::string& json) = 0;

        // ��JSON�ļ���������
        virtual std::vector<std::shared_ptr<Rule>> createFromJsonFile(const std::string& filePath) = 0;

        // �����л��ַ�����������
        virtual std::shared_ptr<Rule> deserialize(const std::string& serialized) = 0;
    };

} // namespace malware_analysis