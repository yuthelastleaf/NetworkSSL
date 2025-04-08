
#include "Rule.h"
#include "Event.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

namespace malware_analysis {
    Rule::Rule(YAML::Node str_rule)
        : match_type_(MatchType::MATCH_COMPLETE)
        , match_event_(EventType::EVENT_NULL)
        , match_prop_(EventProp::PROCESS_IMAGE)
        , match_all_(false)
    {
        do
        {
            if (str_rule.IsMap()) {
                break;
            }

            for (YAML::const_iterator it = str_rule.begin(); it != str_rule.end(); ++it) {
                Rule match_rule(it->first, it->second);
                match_rule_.push_back(match_rule);
            }

        } while (0);
    }

    Rule::Rule(YAML::Node match_key, YAML::Node value)
        : match_type_(MatchType::MATCH_COMPLETE)
        , match_event_(EventType::EVENT_NULL)
        , match_prop_(EventProp::PROCESS_IMAGE)
        , match_all_(false)
    {
        do
        {

            if (match_key.IsScalar()) {
                ParseMatch(match_key.Scalar());
            }
            else {
                break;
            }

            if (!value.IsScalar() && !value.IsSequence()) {
                break;
            }

            

            if (value.IsScalar()) {
                match_value_.push_back(value.Scalar());
            }

            if (value.IsSequence()) {
                for (auto var : value)
                {
                    if (var.IsScalar()) {
                        match_value_.push_back(var.as<std::string>());
                    }
                }
            }
        } while (0);
    }

    Rule::~Rule()
    {
    }

    // 递归匹配
    bool Rule::MatchRule(APTEvent& apt_event)
    {
        bool flag = false;

        if (match_value_.size() > 0) {
            unsigned int cnt = 0;
            for (std::string& value : match_value_) {
                if (apt_event.Match(match_event_, match_type_, match_prop_, value)) {
                    cnt++;
                    if (!match_all_) {
                        break;
                    }
                }
                else if (match_all_) {
                    break;
                }
            }

            if (!match_all_) {
                if (cnt > 0) {
                    flag = true;
                }
            }
            else {
                if (cnt == match_value_.size()) {
                    flag = true;
                }
            }
        }
        else {

            for (Rule& rule : match_rule_) {
                if (rule.MatchRule(apt_event)) {
                    flag = true;
                    break;
                }
            }
        }

        return flag;
    }

    void Rule::ParseMatch(std::string match_detail)
    {

        auto parse = [&](std::string str_parse) mutable {
            do
            {
                if (str_parse.empty()) {
                    break;
                }

                auto type = ToEventType(str_parse);
                if (type != std::nullopt) {
                    match_event_ = *type;
                    break;
                }

                auto prop = ToEventProp(str_parse);
                if (prop != std::nullopt) {
                    match_prop_ = *prop;
                    break;
                }

                auto match = ToMatchType(str_parse);
                if (match != std::nullopt) {
                    match_type_ = *match;
                    break;
                }

                if (str_parse == "all") {
                    match_all_ = true;
                    break;
                }

            } while (0);
        };

        std::vector<std::string> tokens;
        size_t start = 0, end = match_detail.find("|");
        while (end != std::string::npos) {
            parse(match_detail.substr(start, end - start));
            start = end + 1;
            end = match_detail.find("|", start);
            
        }
        parse(match_detail.substr(start)); // 添加最后一个子串

        
    }

    RuleManager::RuleManager(std::string str_chain)
    {
        YAML::Node rule = YAML::Load(str_chain);
        do
        {
            if (!rule["detection"].IsMap()) {
                break;
            }

            if (!rule["condition"].IsScalar()) {
                break;
            }

            auto getscalar = [&](std::string& set_value, std::string set_scalar) mutable {
                if (rule[set_scalar].IsScalar()) {
                    set_value = rule[set_scalar].as<std::string>();
                }
            };

            getscalar(rule_title_, "title");
            getscalar(rule_id_, "id");
            getscalar(rule_desc_, "description");
            getscalar(rule_level_, "level");

            for (YAML::const_iterator it = rule["detection"].begin(); it != rule["detection"].end(); ++it) {

                if (it->second.Type() != YAML::NodeType::Map) {
                    continue;
                }

                Rule rule(it->second);
                rule_map_[it->first.as<std::string>()] = rule;
                symbol_table_.create_variable(it->first.as<std::string>());
            }

            std::string condition = rule["condition"].Scalar();
            expression_.register_symbol_table(symbol_table_);
            parser_.compile(condition, expression_);



        } while (0);
    }

    RuleManager::~RuleManager()
    {
    }

    bool RuleManager::Match(APTEvent& apt_event)
    {
        bool flag = false;
        for (auto& it : rule_map_) {
            symbol_table_.get_variable(it.first)->ref() = it.second.MatchRule(apt_event) ? 1 : 0;
        }

        if (static_cast<int>(expression_.value())) {
            flag = true;
        }
        return flag;
    }

} // namespace malware_analysis