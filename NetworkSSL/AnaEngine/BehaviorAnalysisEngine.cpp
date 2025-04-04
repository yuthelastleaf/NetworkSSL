#include "BehaviorAnalysisEngine.h"

#include "CompositeRule.h"
#include "ActionChain.h"
#include "StatisticalRule.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace malware_analysis {

    using json = nlohmann::json;
    namespace fs = std::filesystem;

    // ���򹤳�ʵ��
    class DefaultRuleFactory : public RuleFactory {
    public:
        std::shared_ptr<Rule> createFromJson(const std::string& jsonStr) override {
            json j = json::parse(jsonStr);

            std::string type = j.value("type", "");

            if (type == "composite") {
                auto rule = std::make_shared<CompositeRule>();
                rule->setId(j.value("id", ""));
                rule->setName(j.value("name", ""));
                rule->setDescription(j.value("description", ""));
                rule->setSeverity(stringToSeverity(j.value("severity", "medium")));

                if (j.contains("logicOp")) {
                    rule->setLogicOp(CompositeRule::stringToLogicOp(j["logicOp"]));
                }

                // ע�⣺�����޷���JSON�ָ���������
                // ��ʵ��Ӧ���У�����Ҫһ�ַ�ʽ�����л����������ؽ�����

                if (j.contains("mitreTactic")) {
                    rule->setMitreTactic(j["mitreTactic"]);
                }

                if (j.contains("mitreTechnique")) {
                    rule->setMitreTechnique(j["mitreTechnique"]);
                }

                if (j.contains("metadata") && j["metadata"].is_object()) {
                    for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
                        rule->addMetadata(it.key(), it.value());
                    }
                }

                return rule;
            }
            else if (type == "statistical") {
                auto rule = std::make_shared<StatisticalRule>();
                rule->setId(j.value("id", ""));
                rule->setName(j.value("name", ""));
                rule->setDescription(j.value("description", ""));
                rule->setSeverity(stringToSeverity(j.value("severity", "medium")));

                if (j.contains("threshold")) {
                    rule->setThreshold(j["threshold"]);
                }

                // ͬ���޷���JSON�ָ��¼�����������

                if (j.contains("mitreTactic")) {
                    rule->setMitreTactic(j["mitreTactic"]);
                }

                if (j.contains("mitreTechnique")) {
                    rule->setMitreTechnique(j["mitreTechnique"]);
                }

                if (j.contains("metadata") && j["metadata"].is_object()) {
                    for (auto it = j["metadata"].begin(); it != j["metadata"].end(); ++it) {
                        rule->addMetadata(it.key(), it.value());
                    }
                }

                return rule;
            }

            // δ֪����
            return nullptr;
        }

        std::vector<std::shared_ptr<Rule>> createFromJsonFile(const std::string& filePath) override {
            std::vector<std::shared_ptr<Rule>> rules;
            std::ifstream file(filePath);

            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                return rules;
            }

            try {
                json j;
                file >> j;

                if (j.is_array()) {
                    // ��������
                    for (const auto& ruleJson : j) {
                        auto rule = createFromJson(ruleJson.dump());
                        if (rule) {
                            rules.push_back(rule);
                        }
                    }
                }
                else {
                    // ��������
                    auto rule = createFromJson(j.dump());
                    if (rule) {
                        rules.push_back(rule);
                    }
                }
            }
            catch (const json::exception& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }

            return rules;
        }

        std::shared_ptr<Rule> deserialize(const std::string& serialized) override {
            try {
                json j = json::parse(serialized);
                return createFromJson(serialized);
            }
            catch (const json::exception& e) {
                std::cerr << "Deserialization error: " << e.what() << std::endl;
                return nullptr;
            }
        }
    };

    // RuleEngineʵ��
    RuleEngine::RuleEngine() : ruleFactory(std::make_unique<DefaultRuleFactory>()) {}

    RuleEngine::~RuleEngine() = default;

    void RuleEngine::addRule(std::shared_ptr<Rule> rule) {
        std::lock_guard<std::mutex> lock(rulesMutex);
        rules.push_back(std::move(rule));
    }

    void RuleEngine::loadRulesFromDirectory(const std::string& directory) {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            std::cerr << "Invalid rules directory: " << directory << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".json" || entry.path().extension() == ".rule")) {
                auto newRules = ruleFactory->createFromJsonFile(entry.path().string());

                std::lock_guard<std::mutex> lock(rulesMutex);
                rules.insert(rules.end(), newRules.begin(), newRules.end());
            }
        }
    }

    std::vector<RuleMatchResult> RuleEngine::analyzeChain(const BehaviorChain& chain) const {
        std::vector<RuleMatchResult> results;

        std::lock_guard<std::mutex> lock(rulesMutex);
        for (const auto& rule : rules) {
            auto result = rule->analyze(chain);
            if (result) {
                results.push_back(*result);
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<Rule>> RuleEngine::getRules() const {
        std::lock_guard<std::mutex> lock(rulesMutex);
        return rules;
    }

    void RuleEngine::clearRules() {
        std::lock_guard<std::mutex> lock(rulesMutex);
        rules.clear();
    }

    bool RuleEngine::saveRulesToFile(const std::string& filePath) const {
        json rulesArray = json::array();

        {
            std::lock_guard<std::mutex> lock(rulesMutex);
            for (const auto& rule : rules) {
                // ���л�����
                rulesArray.push_back(json::parse(rule->serialize()));
            }
        }

        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filePath << std::endl;
                return false;
            }

            file << rulesArray.dump(2);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving rules: " << e.what() << std::endl;
            return false;
        }
    }

    bool RuleEngine::loadRulesFromFile(const std::string& filePath) {
        auto newRules = ruleFactory->createFromJsonFile(filePath);

        if (newRules.empty()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(rulesMutex);
        rules.insert(rules.end(), newRules.begin(), newRules.end());
        return true;
    }

    // BehaviorAnalysisEngineʵ��
    BehaviorAnalysisEngine::BehaviorAnalysisEngine()
        : ruleEngine(std::make_unique<RuleEngine>()) {
        initializeBuiltinRules();
    }

    BehaviorAnalysisEngine::~BehaviorAnalysisEngine() = default;

    std::shared_ptr<BehaviorChain> BehaviorAnalysisEngine::registerProcess(
        uint32_t pid, std::string name, std::optional<uint32_t> parentPid) {
        std::lock_guard<std::mutex> lock(processesMutex);

        auto process = std::make_shared<BehaviorChain>(pid, std::move(name));
        activeProcesses[pid] = process;

        if (parentPid && activeProcesses.count(*parentPid) > 0) {
            // ����һ���ӽ���
            activeProcesses[*parentPid]->addChildProcess(process);
        }
        else {
            // ����һ��������
            rootProcesses[pid] = process;
        }

        return process;
    }

    void BehaviorAnalysisEngine::recordEvent(const Event& event) {
        // ���ҹ����Ľ���
        std::shared_ptr<BehaviorChain> processChain;

        {
            std::lock_guard<std::mutex> lock(processesMutex);
            auto it = activeProcesses.find(event.processId);
            if (it != activeProcesses.end()) {
                processChain = it->second;
            }
        }

        // ��¼�¼�
        if (processChain) {
            processChain->addEvent(event);

            // �����¼��ص�
            if (eventCallback) {
                eventCallback(event);
            }
        }
    }

    void BehaviorAnalysisEngine::terminateProcess(uint32_t pid) {
        std::lock_guard<std::mutex> lock(processesMutex);

        auto it = activeProcesses.find(pid);
        if (it != activeProcesses.end()) {
            it->second->terminate();
            activeProcesses.erase(it);
        }
    }

    std::vector<RuleMatchResult> BehaviorAnalysisEngine::analyzeProcess(uint32_t pid) const {
        std::lock_guard<std::mutex> lock(processesMutex);

        auto it = rootProcesses.find(pid);
        if (it == rootProcesses.end()) {
            return {};
        }

        auto results = ruleEngine->analyzeChain(*(it->second));

        // ��������ص�
        if (resultCallback && !results.empty()) {
            resultCallback(pid, results);
        }

        return results;
    }

    std::map<uint32_t, std::vector<RuleMatchResult>> BehaviorAnalysisEngine::analyzeAllProcesses() const {
        std::map<uint32_t, std::vector<RuleMatchResult>> results;

        std::lock_guard<std::mutex> lock(processesMutex);
        for (const auto& [pid, chain] : rootProcesses) {
            auto processResults = ruleEngine->analyzeChain(*chain);

            if (!processResults.empty()) {
                results[pid] = processResults;

                // ��������ص�
                if (resultCallback) {
                    resultCallback(pid, processResults);
                }
            }
        }

        return results;
    }

    void BehaviorAnalysisEngine::loadRules(const std::string& rulesDirectory) {
        ruleEngine->loadRulesFromDirectory(rulesDirectory);
    }

    void BehaviorAnalysisEngine::addRule(std::shared_ptr<Rule> rule) {
        ruleEngine->addRule(std::move(rule));
    }

    bool BehaviorAnalysisEngine::saveResults(
        const std::string& filename,
        const std::map<uint32_t, std::vector<RuleMatchResult>>& results) const {
        try {
            json j;

            for (const auto& [pid, processResults] : results) {
                j[std::to_string(pid)] = json::array();

                for (const auto& result : processResults) {
                    j[std::to_string(pid)].push_back(json::parse(result.toJson()));
                }
            }

            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                return false;
            }

            file << j.dump(2);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving results: " << e.what() << std::endl;
            return false;
        }
    }

    bool BehaviorAnalysisEngine::loadBehaviorChains(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filename << std::endl;
                return false;
            }

            json j;
            file >> j;

            std::lock_guard<std::mutex> lock(processesMutex);
            rootProcesses.clear();
            activeProcesses.clear();

            for (auto it = j.begin(); it != j.end(); ++it) {
                uint32_t pid = std::stoi(it.key());
                auto chain = BehaviorChain::fromJson(it.value().dump());

                if (chain) {
                    rootProcesses[pid] = chain;

                    // �������δ��ֹ��Ҳ���������б�
                    if (!chain->isTerminated()) {
                        activeProcesses[pid] = chain;
                    }
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading behavior chains: " << e.what() << std::endl;
            return false;
        }
    }

    bool BehaviorAnalysisEngine::saveBehaviorChains(const std::string& filename) const {
        try {
            json j;

            std::lock_guard<std::mutex> lock(processesMutex);
            for (const auto& [pid, chain] : rootProcesses) {
                j[std::to_string(pid)] = json::parse(chain->toJson());
            }

            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                return false;
            }

            file << j.dump(2);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving behavior chains: " << e.what() << std::endl;
            return false;
        }
    }

    void BehaviorAnalysisEngine::setEventCallback(EventCallback callback) {
        eventCallback = std::move(callback);
    }

    void BehaviorAnalysisEngine::setResultCallback(ResultCallback callback) {
        resultCallback = std::move(callback);
    }

    void BehaviorAnalysisEngine::initializeBuiltinRules() {
        // �������ù���

        // 1. PowerShellִ�п��������й���
        auto powershellRule = std::make_shared<CompositeRule>(
            "BUILTIN-001",
            "Suspicious PowerShell Command Line",
            CompositeRule::LogicOperator::AND
        );

        powershellRule->setDescription("Detects suspicious PowerShell command line options often used by malware");
        powershellRule->setSeverity(SeverityLevel::HIGH);
        powershellRule->setMitreTactic("Execution");
        powershellRule->setMitreTechnique("T1059.001");

        powershellRule->addCondition(filters::processNameEquals("powershell.exe"));
        powershellRule->addCondition([](const Event& event) {
            if (event.type != EventType::PROCESS_CREATE) return false;

            auto details = std::dynamic_pointer_cast<ProcessEventDetails>(event.details);
            if (!details) return false;

            const std::string& cmd = details->commandLine;
            return (cmd.find("-enc") != std::string::npos ||
                cmd.find("-EncodedCommand") != std::string::npos ||
                cmd.find("-exec bypass") != std::string::npos ||
                cmd.find("-nop") != std::string::npos ||
                cmd.find("-windowstyle hidden") != std::string::npos);
            });

        addRule(powershellRule);

        // 2. ���ɵ���ʱ�ļ���������
        auto tempFileRule = std::make_shared<CompositeRule>(
            "BUILTIN-002",
            "Suspicious Temp File Creation",
            CompositeRule::LogicOperator::AND
        );

        tempFileRule->setDescription("Detects suspicious file creation in temporary directories");
        tempFileRule->setSeverity(SeverityLevel::MEDIUM);
        tempFileRule->setMitreTactic("Defense Evasion");
        tempFileRule->setMitreTechnique("T1564");

        tempFileRule->addCondition([](const Event& event) {
            if (event.type != EventType::FILE_WRITE) return false;

            auto details = std::dynamic_pointer_cast<FileEventDetails>(event.details);
            if (!details) return false;

            const std::string& path = details->filePath;

            // ����Ƿ�����ʱĿ¼
            return (path.find("\\Temp\\") != std::string::npos ||
                path.find("/tmp/") != std::string::npos);
            });

        tempFileRule->addCondition([](const Event& event) {
            if (event.type != EventType::FILE_WRITE) return false;

            auto details = std::dynamic_pointer_cast<FileEventDetails>(event.details);
            if (!details) return false;

            const std::string& path = details->filePath;

            // �����ɵ��ļ���չ��
            return (path.find(".exe") != std::string::npos ||
                path.find(".dll") != std::string::npos ||
                path.find(".bat") != std::string::npos ||
                path.find(".vbs") != std::string::npos ||
                path.find(".ps1") != std::string::npos);
            });

        addRule(tempFileRule);

        // 3. Ƶ�����������ӹ���
        auto networkRule = std::make_shared<StatisticalRule>(
            "BUILTIN-003",
            "Multiple Network Connections",
            [](const Event& event) {
                return event.type == EventType::NETWORK_CONNECT;
            },
            10  // ��ֵ��10����������
        );

        networkRule->setDescription("Detects processes making multiple network connections, potentially indicating C2 activity");
        networkRule->setSeverity(SeverityLevel::MEDIUM);
        networkRule->setMitreTactic("Command and Control");
        networkRule->setMitreTechnique("T1071");

        addRule(networkRule);

        // 4. ����DLL���ع���
        auto dllRule = std::make_shared<CompositeRule>(
            "BUILTIN-004",
            "Suspicious DLL Loading",
            CompositeRule::LogicOperator::OR
        );

        dllRule->setDescription("Detects loading of DLLs commonly used for malicious purposes");
        dllRule->setSeverity(SeverityLevel::MEDIUM);
        dllRule->setMitreTactic("Defense Evasion");
        dllRule->setMitreTechnique("T1055");

        // ��ӿ���DLL�б�
        std::vector<std::string> suspiciousDlls = {
            "samlib.dll",       // ����ƾ�ݷ���
            "vaultcli.dll",     // ���ڷ���Windowsƾ�ݱ��տ�
            "amsi.dll",         // ���������ɨ��ӿڣ������ƹ�
            "wininet.dll"       // ����ͨ��
        };

        for (const auto& dll : suspiciousDlls) {
            dllRule->addCondition(filters::dllNameEquals(dll));
        }

        addRule(dllRule);

        // ���Լ�����Ӹ������ù���...
    }

} // namespace malware_analysis