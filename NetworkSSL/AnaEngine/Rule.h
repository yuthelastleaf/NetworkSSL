// Rule.h - 定义规则接口和相关结构
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

    // 规则严重性级别
    enum class SeverityLevel {
        INFORMATIONAL,
        LOW,
        MEDIUM,
        HIGH,
        CRITICAL
    };

    // 这是或的条件
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
        std::vector<std::string> match_value_; // 并

        std::vector<Rule> match_rule_; // 或
    };

    // 这是整条判定链,修改规则链，不应该这样做，存在问题
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
        std::map<std::string, Rule> rule_map_; // 保存内置现有规则
        std::string rule_title_;
        std::string rule_id_;
        std::string rule_desc_;
        std::string rule_level_;

        // 规则解析
        symbol_table_t symbol_table_;
        expression_t expression_;
        parser_t parser_;
    };
} // namespace malware_analysis