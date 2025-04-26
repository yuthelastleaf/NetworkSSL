#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <thread>
#include <chrono>

#include "Rule.h"
#include "Event.h"

const char* jsonString = R"({
    "Image": "C:\\Windows\\explorer.exe",
    "eventtype": "proccreate",
    "ParentImage": "file_5678.exe",
    "CommandLine": "C:\\Program Files\\Example\\file_4321.txt --option 42",
    "FileName": "\\\\C:\\mytest\\sysvol\\temp\\Policies\\audit.csv",
    "SourceFilename": "\\\\C:\\mytest\\sysvol\\temp\\Policies\\audit.csv",
    "TargetFilename": "file_2468.jpg",
    "OriginalFileName": "file_9876.txt",
    "filecreatetime": "2025-04-15T14:30:00Z",
    "filemodtime": "2025-04-22T09:15:00Z",
    "TargetObject": "HKEY_LOCAL_MACHINE\\Software\\Example\\",
    "Details": "23"
})";

const char* jsonString1 = R"({
    "Image": "C:\\Windows222\\explorer.exe",
    "eventtype": "proccreate",
    "ParentImage": "file_5678.exe",
    "CommandLine": "C:\\Program Files\\Example\\file_4321.txt --option 42",
    "FileName": "\\\\C:\\mytest\\sysvol\\temp\\Policies\\audit.csv",
    "SourceFilename": "\\\\C:\\mytest\\sysvol\\temp\\Policies\\audit.csv",
    "TargetFilename": "file_2468.jpg",
    "OriginalFileName": "file_9876.txt",
    "filecreatetime": "2025-04-15T14:30:00Z",
    "filemodtime": "2025-04-22T09:15:00Z",
    "TargetObject": "HKEY_LOCAL_MACHINE\\Software\\Example\\",
    "Details": "23"
})";

const char yaml_content[] =
"title: Access To Potentially Sensitive Sysvol Files By Uncommon Applications\n"
"id: d51694fe-484a-46ac-92d6-969e76d60d10\n"
"related:\n"
"    - id: 8344c19f-a023-45ff-ad63-a01c5396aea0\n"
"      type: derived\n"
"status: experimental\n"
"description: Detects file access requests to potentially sensitive files hosted on the Windows Sysvol share.\n"
"references:\n"
"    - https://github.com/vletoux/pingcastle\n"
"author: frack113\n"
"date: 2023-12-21\n"
"modified: 2024-07-29\n"
"tags:\n"
"    - attack.credential-access\n"
"    - attack.t1552.006\n"
"logsource:\n"
"    category: file_access\n"
"    product: windows\n"
"    definition: 'Requirements: Microsoft-Windows-Kernel-File ETW provider'\n"
"detection:\n"
"    selection:\n"
"        FileName|startswith: '\\\\'\n"
"        FileName|contains|all:\n"
"            - '\\sysvol\\'\n"
"            - '\\Policies\\'\n"
"        FileName|endswith:\n"
"            - 'audit.csv'\n"
"            - 'Files.xml'\n"
"            - 'GptTmpl.inf'\n"
"            - 'groups.xml'\n"
"            - 'Registry.pol'\n"
"            - 'Registry.xml'\n"
"            - 'scheduledtasks.xml'\n"
"            - 'scripts.ini'\n"
"            - 'services.xml'\n"
"    filter_main_generic:\n"
"        Image|startswith:\n"
"            - 'C:\\Program Files (x86)\\'\n"
"            - 'C:\\Program Files\\'\n"
"            - 'C:\\Windows\\system32\\'\n"
"            - 'C:\\Windows\\SysWOW64\\'\n"
"    filter_main_explorer:\n"
"        Image: 'C:\\Windows\\explorer.exe'\n"
"    condition: selection and not (filter_main_generic or filter_main_explorer)\n"
"falsepositives:\n"
"    - Unknown\n"
"level: medium\n";


// 主函数
int main(int argc, char** argv) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Malware Behavior Analysis Engine - Example      " << std::endl;
    std::cout << "==================================================" << std::endl;

    
    malware_analysis::RuleManager rulema(yaml_content);

    auto printres = [&](std::string str) {
        malware_analysis::APTEvent apt_event(str);
        rulema.Match(apt_event);
    };

    for (int i = 0; i < 100000; i++) {
        if (i % 2 == 0) {
            printres(jsonString);
        }
        else {
            printres(jsonString1);
        }
    }

    return 0;
}