
#include "Rule.h"
#include "Event.h"

#include <nlohmann/json.hpp>

namespace malware_analysis {

    using json = nlohmann::json;

    std::string severityToString(SeverityLevel level) {
        switch (level) {
        case SeverityLevel::INFORMATIONAL: return "informational";
        case SeverityLevel::LOW: return "low";
        case SeverityLevel::MEDIUM: return "medium";
        case SeverityLevel::HIGH: return "high";
        case SeverityLevel::CRITICAL: return "critical";
        default: return "unknown";
        }
    }

    SeverityLevel stringToSeverity(const std::string& str) {
        if (str == "informational") return SeverityLevel::INFORMATIONAL;
        if (str == "low") return SeverityLevel::LOW;
        if (str == "medium") return SeverityLevel::MEDIUM;
        if (str == "high") return SeverityLevel::HIGH;
        if (str == "critical") return SeverityLevel::CRITICAL;
        return SeverityLevel::MEDIUM; // д╛хо
    }

    std::string RuleMatchResult::toJson() const {
        json j;
        j["ruleId"] = ruleId;
        j["ruleName"] = ruleName;
        j["description"] = description;
        j["severity"] = severityToString(severity);

        j["matchedEvents"] = json::array();
        for (const auto& event : matchedEvents) {
            j["matchedEvents"].push_back(json::parse(event.toJson()));
        }

        j["metadata"] = metadata;

        if (mitreTactic) {
            j["mitreTactic"] = *mitreTactic;
        }

        if (mitreTechnique) {
            j["mitreTechnique"] = *mitreTechnique;
        }

        return j.dump(2);
    }

    RuleMatchResult RuleMatchResult::fromJson(const std::string& jsonStr) {
        json j = json::parse(jsonStr);

        RuleMatchResult result;
        result.ruleId = j["ruleId"];
        result.ruleName = j["ruleName"];
        result.description = j["description"];
        result.severity = stringToSeverity(j["severity"]);

        if (j.contains("matchedEvents") && j["matchedEvents"].is_array()) {
            for (const auto& eventJson : j["matchedEvents"]) {
                result.matchedEvents.push_back(Event::fromJson(eventJson.dump()));
            }
        }

        if (j.contains("metadata") && j["metadata"].is_object()) {
            for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
                result.metadata[it.key()] = it.value();
            }
        }

        if (j.contains("mitreTactic") && !j["mitreTactic"].is_null()) {
            result.mitreTactic = j["mitreTactic"];
        }

        if (j.contains("mitreTechnique") && !j["mitreTechnique"].is_null()) {
            result.mitreTechnique = j["mitreTechnique"];
        }

        return result;
    }

    const std::string& Rule::getId() const {
        return id;
    }

    const std::string& Rule::getName() const {
        return name;
    }

    const std::string& Rule::getDescription() const {
        return description;
    }

    SeverityLevel Rule::getSeverity() const {
        return severity;
    }

    void Rule::setId(const std::string& id) {
        this->id = id;
    }

    void Rule::setName(const std::string& name) {
        this->name = name;
    }

    void Rule::setDescription(const std::string& description) {
        this->description = description;
    }

    void Rule::setSeverity(SeverityLevel severity) {
        this->severity = severity;
    }

    void Rule::setMitreTactic(const std::string& tactic) {
        this->mitreTactic = tactic;
    }

    void Rule::setMitreTechnique(const std::string& technique) {
        this->mitreTechnique = technique;
    }

    void Rule::addMetadata(const std::string& key, const std::string& value) {
        metadata[key] = value;
    }

    std::string Rule::toJson() const {
        json j;
        j["id"] = id;
        j["name"] = name;
        j["description"] = description;
        j["severity"] = severityToString(severity);
        j["metadata"] = metadata;

        if (mitreTactic) {
            j["mitreTactic"] = *mitreTactic;
        }

        if (mitreTechnique) {
            j["mitreTechnique"] = *mitreTechnique;
        }

        return j.dump(2);
    }

} // namespace malware_analysis