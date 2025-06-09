/*
 * example.c - 规则引擎使用示例
 * 演示如何编译规则并评估事件
 */

#include "rule_engine.h"
#include <stdio.h>
#include <string.h>

 // 创建测试事件
event_context_t* create_test_event(int event_type) {
    event_context_t* event = new event_context_t;
    memset(event, 0, sizeof(event_context_t));
    switch (event_type) {
    case 1: // 进程创建事件
        event->process.name = "cmd.exe";
        event->process.pid = 1234;
        event->process.ppid = 567;
        event->process.cmdline = "cmd.exe /c whoami";
        event->process.path = "C:\\Windows\\System32\\cmd.exe";
        event->process.user = "SYSTEM";
        event->process.session_id = 0;
        break;

    case 2: // 文件操作事件
        event->file.path = "C:\\Windows\\System32\\drivers\\etc\\hosts";
        event->file.name = "hosts";
        event->file.extension = "";
        event->file.size = 824;
        event->file.operation = "write";
        event->file.attributes = 0x20;
        break;

    case 3: // 注册表操作事件
        event->registry.key = "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        event->registry.value_name = "Malware";
        event->registry.value_type = 1; // REG_SZ
        event->registry.value_data = "C:\\malware.exe";
        event->registry.operation = "create";
        break;

    case 4: // 网络连接事件
        event->network.src_ip = "192.168.1.100";
        event->network.dst_ip = "8.8.8.8";
        event->network.src_port = 54321;
        event->network.dst_port = 53;
        event->network.protocol = "UDP";
        break;

    default:
        break;
    }

    return event;
}

void test_rule(const char* rule_string, event_context_t* event, const char* description) {
    printf("\n=== %s ===\n", description);
    printf("规则: %s\n", rule_string);

    // 编译规则
    compiled_rule_t* rule = compile_rule(rule_string);

    if (!rule->is_valid) {
        printf("编译失败: %s\n", rule->error_message);
        destroy_compiled_rule(rule);
        return;
    }

    printf("编译成功!\n");

    // 打印AST (调试用)
    printf("\nAST结构:\n");
    print_ast(rule->ast, 0);

    // 评估规则
    bool result = evaluate_rule(rule, event);
    printf("\n评估结果: %s\n", result ? "匹配" : "不匹配");

    // 使用扩展评估获取详细结果
    rule_value_t detailed_result;
    if (evaluate_rule_ex(rule, event, &detailed_result)) {
        printf("详细结果类型: ");
        switch (detailed_result.type) {
        case RULE_TYPE_BOOLEAN:
            printf("布尔值 = %s\n", detailed_result.value.boolean ? "true" : "false");
            break;
        case RULE_TYPE_NUMBER:
            printf("数值 = %.2f\n", detailed_result.value.number);
            break;
        case RULE_TYPE_STRING:
            printf("字符串 = \"%s\"\n", detailed_result.value.string.data);
            break;
        default:
            printf("其他类型\n");
            break;
        }
        free_rule_value(&detailed_result);
    }

    destroy_compiled_rule(rule);
}

int main() {
    printf("规则引擎示例程序\n");
    printf("================\n");

    // 创建不同类型的测试事件
    event_context_t* process_event = create_test_event(1);
    event_context_t* file_event = create_test_event(2);
    event_context_t* registry_event = create_test_event(3);
    event_context_t* network_event = create_test_event(4);

    // 测试1: 简单的进程名称匹配
    test_rule(
        "process.name == \"cmd.exe\"",
        process_event,
        "测试1: 进程名称匹配"
    );

    // 测试2: 复杂的进程规则
    test_rule(
        "process.name == \"cmd.exe\" && process.user == \"SYSTEM\" && process.pid > 1000",
        process_event,
        "测试2: 复杂进程规则"
    );

    // 测试3: 字符串包含检查
    test_rule(
        "process.cmdline contains \"whoami\"",
        process_event,
        "测试3: 命令行包含检查"
    );

    // 测试4: 文件路径检查
    test_rule(
        "file.path startswith \"C:\\\\Windows\\\\System32\" && file.operation == \"write\"",
        file_event,
        "测试4: 系统文件写入检查"
    );

    // 测试5: 注册表启动项检查
    test_rule(
        "registry.key endswith \"\\\\Run\" && registry.operation == \"create\"",
        registry_event,
        "测试5: 注册表启动项创建"
    );

    // 测试6: 网络连接检查
    test_rule(
        "network.dst_port == 53 || network.dst_port == 80 || network.dst_port == 443",
        network_event,
        "测试6: 常见端口连接"
    );

    // 测试7: 使用in运算符
    test_rule(
        "network.dst_port in [53, 80, 443, 8080]",
        network_event,
        "测试7: 端口列表检查"
    );

    // 测试8: 复杂的组合规则
    test_rule(
        "(process.name == \"powershell.exe\" || process.name == \"cmd.exe\") && "
        "process.user != \"Administrator\" && "
        "process.ppid > 0",
        process_event,
        "测试8: 可疑进程检测"
    );

    // 测试9: 数学运算
    test_rule(
        "file.size > 500 && file.size < 1024",
        file_event,
        "测试9: 文件大小范围检查"
    );

    // 测试10: 逻辑运算符优先级
    test_rule(
        "!process.name == \"explorer.exe\" && process.pid > 1000 || process.user == \"SYSTEM\"",
        process_event,
        "测试10: 复杂逻辑表达式"
    );

    // 测试错误情况
    test_rule(
        "process.invalid_field == \"test\"",
        process_event,
        "测试错误: 无效字段"
    );

    test_rule(
        "process.name ==",
        process_event,
        "测试错误: 语法错误"
    );

    // 释放资源
    delete(process_event);
    delete(file_event);
    delete(registry_event);
    delete(network_event);

    printf("\n示例程序结束\n");
    return 0;
}