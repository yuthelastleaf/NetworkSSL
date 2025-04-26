// Rule.h - �������ӿں���ؽṹ
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "Event.h"
#include "yaml-cpp/yaml.h"
#include "../../include/exprtk/exprtk.hpp"

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
        Rule();
        Rule(YAML::Node str_rule);
        Rule(YAML::Node match_key, YAML::Node value);
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

    // ���������ж���,�޸Ĺ���������Ӧ������������������
    class RuleManager
    {
        typedef exprtk::symbol_table<double> symbol_table_t;
        typedef exprtk::expression<double>   expression_t;
        typedef exprtk::parser<double>       parser_t;

    public:
        explicit RuleManager(std::string str_chain);
        ~RuleManager();

    public:
        bool Match(APTEvent& apt_event);

    private:
        std::map<std::string, Rule> rule_map_; // �����������й���
        std::string rule_title_;
        std::string rule_id_;
        std::string rule_desc_;
        std::string rule_level_;

        // �������
        symbol_table_t symbol_table_;
        expression_t expression_;
        parser_t parser_;
    };
} // namespace malware_analysis