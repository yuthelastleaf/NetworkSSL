// Rule.h - �������ӿں���ؽṹ
#pragma once

#include "ActionChain.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "yaml-cpp/yaml.h"

namespace malware_analysis {

    // ���������Լ���
    enum class SeverityLevel {
        INFORMATIONAL,
        LOW,
        MEDIUM,
        HIGH,
        CRITICAL
    };

    // ���ǻ������
    class Rule
    {
    public:
        explicit Rule(YAML::Node str_rule);
        explicit Rule(YAML::Node match_key, YAML::Node value);
        ~Rule();

    public:
        bool MatchRule(APTEvent& apt_event);

    private:
        void ParseMatch(std::string match_detail);

    private:
        MatchType match_type_;
        EventType match_event_;
        EventProp match_prop_;
        bool match_all_;
        unsigned short match_cnt_;
        std::vector<unsigned short> match_status_;
        std::vector<std::string> match_value_; // ��

        std::vector<Rule> match_rule_; // ��
    };

    // ���������ж���
    class RuleChain
    {
    public:
        explicit RuleChain(std::string str_chain);
        ~RuleChain();

    public:
        bool Match(APTEvent& apt_event, unsigned int& offset);

    private:
        std::map<std::string, Rule> rule_map_;
        std::vector<Rule> rule_chain_;  

        std::string rule_title_;
        std::string rule_id_;
        std::string rule_desc_;
        std::string rule_level_;
    };
} // namespace malware_analysis