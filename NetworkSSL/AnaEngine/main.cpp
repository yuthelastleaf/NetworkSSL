#define _CRT_SECURE_NO_WARNINGS
#include "BehaviorAnalysisEngine.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace malware_analysis;

// 打印事件详情
void printEvent(const Event& event) {
    std::cout << event.toString() << std::endl;
}

// 打印匹配结果
void printMatchResults(uint32_t pid, const std::vector<RuleMatchResult>& results) {
    std::cout << "\n🚨 Alert: Process " << pid << " triggered " << results.size() << " rules:" << std::endl;

    for (const auto& result : results) {
        std::cout << "  - Rule: " << result.ruleName << " (ID: " << result.ruleId << ")" << std::endl;
        std::cout << "    Severity: " << severityToString(result.severity) << std::endl;
        std::cout << "    Description: " << result.description << std::endl;

        if (result.mitreTactic) {
            std::cout << "    MITRE Tactic: " << *result.mitreTactic << std::endl;
        }

        if (result.mitreTechnique) {
            std::cout << "    MITRE Technique: " << *result.mitreTechnique << std::endl;
        }

        std::cout << "    Matched " << result.matchedEvents.size() << " events." << std::endl;

        // 打印前3个匹配事件作为示例
        if (!result.matchedEvents.empty()) {
            std::cout << "    Sample matched events:" << std::endl;
            for (size_t i = 0; i < std::min<size_t>(3, result.matchedEvents.size()); ++i) {
                std::cout << "      " << result.matchedEvents[i].toString() << std::endl;
            }
        }

        std::cout << std::endl;
    }
}

// 模拟恶意软件行为场景
void simulateMalwareScenario(BehaviorAnalysisEngine& engine) {
    std::cout << "Simulating malware scenario...\n" << std::endl;

    // 根进程
    uint32_t explorerPid = 1000;
    engine.registerProcess(explorerPid, "explorer.exe");

    // 创建PowerShell进程 (使用可疑的编码命令)
    uint32_t powershellPid = 1001;
    auto powershellDetails = std::make_shared<ProcessEventDetails>();
    powershellDetails->parentProcessId = explorerPid;
    powershellDetails->commandLine = "powershell.exe -nop -windowstyle hidden -enc ZQBjAGgAbwAgACIASABlAGwAbABvACIACgA=";
    powershellDetails->executablePath = "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
    powershellDetails->elevated = false;

    Event powershellEvent(
        powershellPid,
        "powershell.exe",
        EventType::PROCESS_CREATE,
        powershellDetails
    );

    engine.registerProcess(powershellPid, "powershell.exe", explorerPid);
    engine.recordEvent(powershellEvent);

    // PowerShell创建临时文件
    auto tempFileDetails = std::make_shared<FileEventDetails>();
    tempFileDetails->filePath = "C:\\Users\\User\\AppData\\Local\\Temp\\malware.exe";
    tempFileDetails->fileSize = 250000;
    tempFileDetails->success = true;

    Event tempFileEvent(
        powershellPid,
        "powershell.exe",
        EventType::FILE_WRITE,
        tempFileDetails
    );

    engine.recordEvent(tempFileEvent);

    // PowerShell加载可疑DLL
    auto dllDetails = std::make_shared<DllEventDetails>();
    dllDetails->dllName = "wininet.dll";
    dllDetails->dllPath = "C:\\Windows\\System32\\wininet.dll";
    dllDetails->success = true;

    Event dllEvent(
        powershellPid,
        "powershell.exe",
        EventType::DLL_LOAD,
        dllDetails
    );

    engine.recordEvent(dllEvent);

    // 创建恶意软件进程
    uint32_t malwarePid = 1002;
    auto malwareDetails = std::make_shared<ProcessEventDetails>();
    malwareDetails->parentProcessId = powershellPid;
    malwareDetails->commandLine = "C:\\Users\\User\\AppData\\Local\\Temp\\malware.exe";
    malwareDetails->executablePath = "C:\\Users\\User\\AppData\\Local\\Temp\\malware.exe";
    malwareDetails->elevated = false;

    Event malwareEvent(
        malwarePid,
        "malware.exe",
        EventType::PROCESS_CREATE,
        malwareDetails
    );

    engine.registerProcess(malwarePid, "malware.exe", powershellPid);
    engine.recordEvent(malwareEvent);

    // 恶意软件多次网络连接 (C2通信)
    for (int i = 0; i < 15; i++) {
        auto networkDetails = std::make_shared<NetworkEventDetails>();
        networkDetails->remoteAddress = "192.168.1." + std::to_string(i % 5 + 100);
        networkDetails->remotePort = 8080 + (i % 10);
        networkDetails->protocol = "tcp";
        networkDetails->success = true;
        networkDetails->dataSize = 1024 + (i * 512);

        Event networkEvent(
            malwarePid,
            "malware.exe",
            EventType::NETWORK_CONNECT,
            networkDetails
        );

        engine.recordEvent(networkEvent);

        // 添加一些间隔以模拟真实场景
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 终止进程
    engine.terminateProcess(malwarePid);
    engine.terminateProcess(powershellPid);
    engine.terminateProcess(explorerPid);
}

// 添加自定义规则示例
void addCustomRule(BehaviorAnalysisEngine& engine) {
    // 创建一个检测多次文件删除的规则
    auto fileDeleteRule = std::make_shared<StatisticalRule>(
        "CUSTOM-001",
        "Multiple File Deletions",
        [](const Event& event) {
            return event.type == EventType::FILE_DELETE;
        },
        5  // 阈值：5个文件删除操作
    );

    fileDeleteRule->setDescription("Detects processes deleting multiple files, potentially indicating data destruction");
    fileDeleteRule->setSeverity(SeverityLevel::HIGH);
    fileDeleteRule->setMitreTactic("Impact");
    fileDeleteRule->setMitreTechnique("T1485");

    engine.addRule(fileDeleteRule);

    std::cout << "Added custom rule: " << fileDeleteRule->getName() << std::endl;
}

// 模拟事件捕获系统集成
void simulateEventCapture() {
    std::cout << "\nSimulating event capture system integration...\n" << std::endl;

    // 创建分析引擎
    BehaviorAnalysisEngine engine;

    // 设置事件回调
    engine.setEventCallback(printEvent);

    // 设置结果回调
    engine.setResultCallback(printMatchResults);

    // 添加自定义规则
    addCustomRule(engine);

    // 模拟恶意软件行为
    simulateMalwareScenario(engine);

    // 分析所有进程
    std::cout << "\nPerforming final analysis of all processes...\n" << std::endl;

    auto results = engine.analyzeAllProcesses();

    // 显示分析结果摘要
    std::cout << "\nAnalysis Summary:" << std::endl;
    std::cout << "----------------" << std::endl;
    std::cout << "Total processes analyzed: " << results.size() << std::endl;

    size_t totalAlerts = 0;
    for (const auto& [pid, processResults] : results) {
        totalAlerts += processResults.size();
    }

    std::cout << "Total alerts generated: " << totalAlerts << std::endl;

    // 保存结果和行为链
    std::cout << "\nSaving results and behavior chains..." << std::endl;

    bool saveResultsSuccess = engine.saveResults("analysis_results.json", results);
    bool saveChainSuccess = engine.saveBehaviorChains("behavior_chains.json");

    if (saveResultsSuccess) {
        std::cout << "Results saved to analysis_results.json" << std::endl;
    }

    if (saveChainSuccess) {
        std::cout << "Behavior chains saved to behavior_chains.json" << std::endl;
    }
}

// 规则加载示例
void testRuleLoading() {
    std::cout << "\nTesting rule loading from directory...\n" << std::endl;

    BehaviorAnalysisEngine engine;

    // 加载规则目录 (注意：这会尝试从rules目录加载规则)
    engine.loadRules("rules");

    // 模拟一个简单的进程和行为
    uint32_t testPid = 2000;
    engine.registerProcess(testPid, "test.exe");

    // 添加一些测试事件
    auto dllDetails = std::make_shared<DllEventDetails>();
    dllDetails->dllName = "samlib.dll";  // 这将触发内置规则
    dllDetails->dllPath = "C:\\Windows\\System32\\samlib.dll";
    dllDetails->success = true;

    Event dllEvent(
        testPid,
        "test.exe",
        EventType::DLL_LOAD,
        dllDetails
    );

    engine.recordEvent(dllEvent);

    // 分析并显示结果
    auto results = engine.analyzeProcess(testPid);

    std::cout << "Test process triggered " << results.size() << " rules" << std::endl;
    for (const auto& result : results) {
        std::cout << " - " << result.ruleName << " (" << result.ruleId << ")" << std::endl;
    }

    // 终止测试进程
    engine.terminateProcess(testPid);
}

// 主函数
int main(int argc, char** argv) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Malware Behavior Analysis Engine - Example      " << std::endl;
    std::cout << "==================================================" << std::endl;

    try {
        // 模拟事件捕获系统集成
        simulateEventCapture();

        // 测试规则加载
        testRuleLoading();

        std::cout << "\nExample completed successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}