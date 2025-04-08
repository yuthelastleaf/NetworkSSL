#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <thread>
#include <chrono>

#include "Rule.h"
#include "Event.h"

const char* jsonString = R"({
    "Image": "file_1234.jpg",
    "eventtype": "proccreate",
    "ParentImage": "file_5678.exe",
    "CommandLine": "C:\\Program Files\\Example\\file_4321.txt --option 42",
    "FileName": "file_8765.png",
    "SourceFilename": "file_1357.docx",
    "TargetFilename": "file_2468.jpg",
    "OriginalFileName": "file_9876.txt",
    "filecreatetime": "2025-04-15T14:30:00Z",
    "filemodtime": "2025-04-22T09:15:00Z",
    "TargetObject": "HKEY_LOCAL_MACHINE\\Software\\Example\\",
    "Details": "23"
})";

// 主函数
int main(int argc, char** argv) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Malware Behavior Analysis Engine - Example      " << std::endl;
    std::cout << "==================================================" << std::endl;

    // APTEvent apt_event(jsonString);

    return 0;
}