///*
// * example.c - 规则引擎使用示例
// * 演示如何编译规则并评估事件
// */
//
//#include "rule_engine.h"
//#include <stdio.h>
//#include <string.h>
//
// // 创建测试事件
//event_context_t* create_test_event(int event_type) {
//    event_context_t* event = new event_context_t;
//    memset(event, 0, sizeof(event_context_t));
//    switch (event_type) {
//    case 1: // 进程创建事件
//        event->process.name = "cmd.exe";
//        event->process.pid = 1234;
//        event->process.ppid = 567;
//        event->process.cmdline = "cmd.exe /c whoami";
//        event->process.path = "C:\\Windows\\System32\\cmd.exe";
//        event->process.user = "SYSTEM";
//        event->process.session_id = 0;
//        break;
//
//    case 2: // 文件操作事件
//        event->file.path = "C:\\Windows\\System32\\drivers\\etc\\hosts";
//        event->file.name = "hosts";
//        event->file.extension = "";
//        event->file.size = 824;
//        event->file.operation = "write";
//        event->file.attributes = 0x20;
//        break;
//
//    case 3: // 注册表操作事件
//        event->registry.key = "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
//        event->registry.value_name = "Malware";
//        event->registry.value_type = 1; // REG_SZ
//        event->registry.value_data = "C:\\malware.exe";
//        event->registry.operation = "create";
//        break;
//
//    case 4: // 网络连接事件
//        event->network.src_ip = "192.168.1.100";
//        event->network.dst_ip = "8.8.8.8";
//        event->network.src_port = 54321;
//        event->network.dst_port = 53;
//        event->network.protocol = "UDP";
//        break;
//
//    default:
//        break;
//    }
//
//    return event;
//}
//
//void test_rule(const char* rule_string, event_context_t* event, const char* description) {
//    printf("\n=== %s ===\n", description);
//    printf("规则: %s\n", rule_string);
//
//    // 编译规则
//    compiled_rule_t* rule = compile_rule(rule_string);
//
//    if (!rule->is_valid) {
//        printf("编译失败: %s\n", rule->error_message);
//        destroy_compiled_rule(rule);
//        return;
//    }
//
//    printf("编译成功!\n");
//
//    // 打印AST (调试用)
//    printf("\nAST结构:\n");
//    print_ast(rule->ast, 0);
//
//    // 评估规则
//    bool result = evaluate_rule(rule, event);
//    printf("\n评估结果: %s\n", result ? "匹配" : "不匹配");
//
//    // 使用扩展评估获取详细结果
//    rule_value_t detailed_result;
//    if (evaluate_rule_ex(rule, event, &detailed_result)) {
//        printf("详细结果类型: ");
//        switch (detailed_result.type) {
//        case RULE_TYPE_BOOLEAN:
//            printf("布尔值 = %s\n", detailed_result.value.boolean ? "true" : "false");
//            break;
//        case RULE_TYPE_NUMBER:
//            printf("数值 = %.2f\n", detailed_result.value.number);
//            break;
//        case RULE_TYPE_STRING:
//            printf("字符串 = \"%s\"\n", detailed_result.value.string.data);
//            break;
//        default:
//            printf("其他类型\n");
//            break;
//        }
//        free_rule_value(&detailed_result);
//    }
//
//    destroy_compiled_rule(rule);
//}
//
//int main() {
//    printf("规则引擎示例程序\n");
//    printf("================\n");
//
//    // 创建不同类型的测试事件
//    event_context_t* process_event = create_test_event(1);
//    event_context_t* file_event = create_test_event(2);
//    event_context_t* registry_event = create_test_event(3);
//    event_context_t* network_event = create_test_event(4);
//
//    // 测试1: 简单的进程名称匹配
//    test_rule(
//        "process.name == \"cmd.exe\"",
//        process_event,
//        "测试1: 进程名称匹配"
//    );
//
//    // 测试2: 复杂的进程规则
//    test_rule(
//        "process.name == \"cmd.exe\" && process.user == \"SYSTEM\" && process.pid > 1000",
//        process_event,
//        "测试2: 复杂进程规则"
//    );
//
//    // 测试3: 字符串包含检查
//    test_rule(
//        "process.cmdline contains \"whoami\"",
//        process_event,
//        "测试3: 命令行包含检查"
//    );
//
//    // 测试4: 文件路径检查
//    test_rule(
//        "file.path startswith \"C:\\\\Windows\\\\System32\" && file.operation == \"write\"",
//        file_event,
//        "测试4: 系统文件写入检查"
//    );
//
//    // 测试5: 注册表启动项检查
//    test_rule(
//        "registry.key endswith \"\\\\Run\" && registry.operation == \"create\"",
//        registry_event,
//        "测试5: 注册表启动项创建"
//    );
//
//    // 测试6: 网络连接检查
//    test_rule(
//        "network.dst_port == 53 || network.dst_port == 80 || network.dst_port == 443",
//        network_event,
//        "测试6: 常见端口连接"
//    );
//
//    // 测试7: 使用in运算符
//    test_rule(
//        "network.dst_port in [53, 80, 443, 8080]",
//        network_event,
//        "测试7: 端口列表检查"
//    );
//
//    // 测试8: 复杂的组合规则
//    test_rule(
//        "(process.name == \"powershell.exe\" || process.name == \"cmd.exe\") && "
//        "process.user != \"Administrator\" && "
//        "process.ppid > 0",
//        process_event,
//        "测试8: 可疑进程检测"
//    );
//
//    // 测试9: 数学运算
//    test_rule(
//        "file.size > 500 && file.size < 1024",
//        file_event,
//        "测试9: 文件大小范围检查"
//    );
//
//    // 测试10: 逻辑运算符优先级
//    test_rule(
//        "!process.name == \"explorer.exe\" && process.pid > 1000 || process.user == \"SYSTEM\"",
//        process_event,
//        "测试10: 复杂逻辑表达式"
//    );
//
//    // 测试错误情况
//    test_rule(
//        "process.invalid_field == \"test\"",
//        process_event,
//        "测试错误: 无效字段"
//    );
//
//    test_rule(
//        "process.name ==",
//        process_event,
//        "测试错误: 语法错误"
//    );
//
//    // 释放资源
//    delete(process_event);
//    delete(file_event);
//    delete(registry_event);
//    delete(network_event);
//
//    printf("\n示例程序结束\n");
//    return 0;
//}


/*
 * vm_example.c - 规则虚拟机使用示例
 * 演示字节码编译和执行
 */

//#include "rule_engine.h"
//#include "rule_vm.h"
//#include <stdio.h>
//#include <string.h>
//#include <time.h>
//#include <memory>
//
// // 创建测试事件
//event_context_t* create_test_event() {
//    event_context_t* event = (event_context_t*)malloc(sizeof(event_context_t));
//    memset(event, 0, sizeof(event_context_t));
//
//    event->process.name = "powershell.exe";
//    event->process.pid = 1234;
//    event->process.ppid = 567;
//    event->process.cmdline = "powershell.exe -encodedCommand SGVsbG8gV29ybGQ=";
//    event->process.path = "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
//    event->process.user = "user123";
//    event->process.session_id = 1;
//
//    event->file.path = "C:\\Users\\user123\\Documents\\malware.exe";
//    event->file.name = "malware.exe";
//    event->file.extension = "exe";
//    event->file.size = 102400;
//    event->file.operation = "create";
//
//    event->network.dst_ip = "192.168.1.100";
//    event->network.dst_port = 443;
//    event->network.protocol = "TCP";
//
//    return event;
//}
//
//// 性能测试：比较AST和VM执行速度
//void performance_test(const char* rule_string, event_context_t* event, int iterations) {
//    printf("\n=== 性能测试: %d 次迭代 ===\n", iterations);
//    printf("规则: %s\n", rule_string);
//
//    // 编译规则
//    compiled_rule_t* ast_rule = compile_rule(rule_string);
//    vm_bytecode_t* bytecode = compile_rule_to_bytecode(rule_string);
//
//    if (!ast_rule->is_valid || !bytecode->is_valid) {
//        printf("规则编译失败\n");
//        destroy_compiled_rule(ast_rule);
//        destroy_bytecode(bytecode);
//        return;
//    }
//
//    // 测试AST执行
//    clock_t start = clock();
//    bool ast_result = false;
//    for (int i = 0; i < iterations; i++) {
//        ast_result = evaluate_rule(ast_rule, event);
//    }
//    clock_t ast_time = clock() - start;
//
//    // 测试VM执行
//    vm_instance_t* vm = create_vm_instance(bytecode);
//    start = clock();
//    bool vm_result = false;
//    for (int i = 0; i < iterations; i++) {
//        if (vm_execute(vm, event)) {
//            rule_value_t result = vm_get_result(vm);
//            vm_result = (result.type == RULE_TYPE_BOOLEAN && result.value.boolean);
//            free_rule_value(&result);
//        }
//    }
//    clock_t vm_time = clock() - start;
//
//    // 打印结果
//    printf("AST 结果: %s, 时间: %.3f ms\n",
//        ast_result ? "匹配" : "不匹配",
//        (double)ast_time * 1000.0 / CLOCKS_PER_SEC);
//
//    printf("VM  结果: %s, 时间: %.3f ms\n",
//        vm_result ? "匹配" : "不匹配",
//        (double)vm_time * 1000.0 / CLOCKS_PER_SEC);
//
//    double speedup = (double)ast_time / vm_time;
//    printf("VM 加速比: %.2fx %s\n", speedup, speedup > 1.0 ? "更快" : "更慢");
//
//    // 清理
//    destroy_compiled_rule(ast_rule);
//    destroy_bytecode(bytecode);
//    destroy_vm_instance(vm);
//}
//
//// 测试字节码编译和反汇编
//void test_bytecode_compilation(const char* rule_string) {
//    printf("\n=== 字节码编译测试 ===\n");
//    printf("规则: %s\n", rule_string);
//
//    // 编译为字节码
//    vm_bytecode_t* bytecode = compile_rule_to_bytecode(rule_string);
//
//    if (!bytecode->is_valid) {
//        printf("编译失败: %s\n", bytecode->error_message);
//        destroy_bytecode(bytecode);
//        return;
//    }
//
//    printf("编译成功!\n");
//    printf("指令数: %zu\n", bytecode->instruction_count);
//    printf("常量数: %zu\n", bytecode->constant_count);
//
//    // 反汇编
//    printf("\n");
//    disassemble_bytecode(bytecode);
//
//    destroy_bytecode(bytecode);
//}
//
//// 测试缓存规则
//void test_cached_rule(const char* rule_string, event_context_t* event) {
//    printf("\n=== 缓存规则测试 ===\n");
//    printf("规则: %s\n", rule_string);
//
//    // 创建缓存规则
//    cached_rule_t* rule = create_cached_rule(rule_string);
//
//    // 评估
//    bool result = evaluate_cached_rule(rule, event);
//    printf("结果: %s\n", result ? "匹配" : "不匹配");
//
//    // 多次评估（测试缓存效果）
//    clock_t start = clock();
//    for (int i = 0; i < 1000; i++) {
//        evaluate_cached_rule(rule, event);
//    }
//    clock_t elapsed = clock() - start;
//
//    printf("1000次评估时间: %.3f ms\n",
//        (double)elapsed * 1000.0 / CLOCKS_PER_SEC);
//
//    destroy_cached_rule(rule);
//}
//
//// 测试字节码序列化
//void test_bytecode_serialization(const char* rule_string) {
//    printf("\n=== 字节码序列化测试 ===\n");
//    printf("规则: %s\n", rule_string);
//
//    // 编译字节码
//    vm_bytecode_t* original = compile_rule_to_bytecode(rule_string);
//    if (!original->is_valid) {
//        printf("编译失败\n");
//        destroy_bytecode(original);
//        return;
//    }
//
//    // 序列化
//    size_t size;
//    uint8_t* data = serialize_bytecode(original, &size);
//    printf("序列化大小: %zu 字节\n", size);
//
//    // 反序列化
//    vm_bytecode_t* restored = deserialize_bytecode(data, size);
//    free(data);
//
//    if (!restored) {
//        printf("反序列化失败\n");
//        destroy_bytecode(original);
//        return;
//    }
//
//    printf("反序列化成功\n");
//
//    // 验证：使用两个字节码执行并比较结果
//    event_context_t* event = create_test_event();
//
//    vm_instance_t* vm1 = create_vm_instance(original);
//    vm_instance_t* vm2 = create_vm_instance(restored);
//
//    bool result1 = false, result2 = false;
//
//    if (vm_execute(vm1, event)) {
//        rule_value_t r = vm_get_result(vm1);
//        result1 = (r.type == RULE_TYPE_BOOLEAN && r.value.boolean);
//        free_rule_value(&r);
//    }
//
//    if (vm_execute(vm2, event)) {
//        rule_value_t r = vm_get_result(vm2);
//        result2 = (r.type == RULE_TYPE_BOOLEAN && r.value.boolean);
//        free_rule_value(&r);
//    }
//
//    printf("原始字节码结果: %s\n", result1 ? "匹配" : "不匹配");
//    printf("恢复字节码结果: %s\n", result2 ? "匹配" : "不匹配");
//    printf("结果一致: %s\n", (result1 == result2) ? "是" : "否");
//
//    // 清理
//    destroy_vm_instance(vm1);
//    destroy_vm_instance(vm2);
//    destroy_bytecode(original);
//    destroy_bytecode(restored);
//    free(event);
//}
//
//// 测试文件保存和加载
//void test_bytecode_file_io(const char* rule_string) {
//    printf("\n=== 字节码文件I/O测试 ===\n");
//    printf("规则: %s\n", rule_string);
//
//    const char* filename = "test_rule.rbc"; // Rule ByteCode
//
//    // 编译并保存
//    vm_bytecode_t* original = compile_rule_to_bytecode(rule_string);
//    if (!original->is_valid) {
//        printf("编译失败\n");
//        destroy_bytecode(original);
//        return;
//    }
//
//    if (!save_bytecode_to_file(original, filename)) {
//        printf("保存失败\n");
//        destroy_bytecode(original);
//        return;
//    }
//
//    printf("字节码已保存到: %s\n", filename);
//
//    // 加载
//    vm_bytecode_t* loaded = load_bytecode_from_file(filename);
//    if (!loaded) {
//        printf("加载失败\n");
//        destroy_bytecode(original);
//        return;
//    }
//
//    printf("字节码已从文件加载\n");
//
//    // 测试执行
//    event_context_t* event = create_test_event();
//    vm_instance_t* vm = create_vm_instance(loaded);
//
//    if (vm_execute(vm, event)) {
//        rule_value_t result = vm_get_result(vm);
//        printf("执行结果: %s\n",
//            (result.type == RULE_TYPE_BOOLEAN && result.value.boolean) ? "匹配" : "不匹配");
//        free_rule_value(&result);
//    }
//
//    // 清理
//    destroy_vm_instance(vm);
//    destroy_bytecode(original);
//    destroy_bytecode(loaded);
//    free(event);
//}
//
//// 复杂规则测试
//void test_complex_rules() {
//    printf("\n=== 复杂规则测试 ===\n");
//
//    event_context_t* event = create_test_event();
//
//    const char* complex_rules[] = {
//        // 测试各种运算符
//        "process.pid > 1000 && process.pid < 2000",
//        "process.name == \"powershell.exe\" || process.name == \"cmd.exe\"",
//        "process.cmdline contains \"encoded\" && process.user != \"SYSTEM\"",
//        "file.size > 100000 && file.extension == \"exe\"",
//        "network.dst_port in [80, 443, 8080, 8443]",
//
//        // 复杂嵌套
//        "(process.name == \"powershell.exe\" && process.cmdline contains \"encoded\") || "
//        "(process.name == \"cmd.exe\" && process.ppid < 100)",
//
//        // 字符串操作
//        "process.path startswith \"C:\\\\Windows\" && process.path endswith \".exe\"",
//
//        // 数学运算
//        "file.size / 1024 > 100 && file.size % 1024 == 0",
//
//        // 逻辑运算
//        "!(process.user == \"SYSTEM\") && (process.pid > 0 || process.ppid > 0)",
//
//        NULL
//    };
//
//    for (int i = 0; complex_rules[i] != NULL; i++) {
//        printf("\n测试规则 %d: %s\n", i + 1, complex_rules[i]);
//
//        // 编译为字节码
//        vm_bytecode_t* bytecode = compile_rule_to_bytecode(complex_rules[i]);
//
//        if (!bytecode->is_valid) {
//            printf("编译失败: %s\n", bytecode->error_message);
//            destroy_bytecode(bytecode);
//            continue;
//        }
//
//        // 创建VM并执行
//        vm_instance_t* vm = create_vm_instance(bytecode);
//
//        if (vm_execute(vm, event)) {
//            rule_value_t result = vm_get_result(vm);
//            printf("执行成功，结果: ");
//
//            switch (result.type) {
//            case RULE_TYPE_BOOLEAN:
//                printf("%s\n", result.value.boolean ? "true" : "false");
//                break;
//            case RULE_TYPE_NUMBER:
//                printf("%.2f\n", result.value.number);
//                break;
//            case RULE_TYPE_STRING:
//                printf("\"%s\"\n", result.value.string.data);
//                break;
//            default:
//                printf("未知类型\n");
//                break;
//            }
//
//            free_rule_value(&result);
//        }
//        else {
//            printf("执行失败: %s\n", vm->error_message);
//        }
//
//        printf("执行统计:\n");
//        printf("  - 执行指令数: %zu\n", vm->instruction_count);
//        printf("  - 最大栈深度: %zu\n", vm->max_stack_depth);
//
//        destroy_vm_instance(vm);
//        destroy_bytecode(bytecode);
//    }
//
//    free(event);
//}
//
//// 错误处理测试
//void test_error_handling() {
//    printf("\n=== 错误处理测试 ===\n");
//
//    const char* error_rules[] = {
//        "process.invalid_field == \"test\"",  // 无效字段
//        "process.name ==",                     // 语法错误
//        "1 / 0",                              // 除零错误
//        "process.name + 123",                 // 类型错误
//        NULL
//    };
//
//    for (int i = 0; error_rules[i] != NULL; i++) {
//        printf("\n测试错误规则 %d: %s\n", i + 1, error_rules[i]);
//
//        vm_bytecode_t* bytecode = compile_rule_to_bytecode(error_rules[i]);
//
//        if (!bytecode->is_valid) {
//            printf("编译错误（预期）: %s\n", bytecode->error_message);
//        }
//        else {
//            printf("编译意外成功\n");
//
//            // 尝试执行
//            event_context_t* event = create_test_event();
//            vm_instance_t* vm = create_vm_instance(bytecode);
//
//            if (!vm_execute(vm, event)) {
//                printf("执行错误（预期）: %s\n", vm->error_message);
//            }
//            else {
//                printf("执行意外成功\n");
//            }
//
//            destroy_vm_instance(vm);
//            free(event);
//        }
//
//        destroy_bytecode(bytecode);
//    }
//}
//
//// 主函数
//int main() {
//    printf("规则引擎虚拟机示例\n");
//    printf("==================\n");
//
//    // 创建测试事件
//    event_context_t* event = create_test_event();
//
//    // 基本测试
//    test_bytecode_compilation("process.name == \"powershell.exe\"");
//
//    // 性能测试
//    performance_test("process.pid > 1000 && process.name == \"powershell.exe\"", event, 10000);
//    performance_test("process.cmdline contains \"encoded\"", event, 10000);
//    performance_test("network.dst_port in [80, 443, 8080]", event, 10000);
//
//    // 缓存规则测试
//    test_cached_rule("file.size > 100000 && file.extension == \"exe\"", event);
//
//    // 序列化测试
//    test_bytecode_serialization("process.user != \"SYSTEM\" && process.pid > 0");
//
//    // 文件I/O测试
//    test_bytecode_file_io("(process.name == \"cmd.exe\" || process.name == \"powershell.exe\") && process.ppid > 0");
//
//    // 复杂规则测试
//    test_complex_rules();
//
//    // 错误处理测试
//    test_error_handling();
//
//    // 清理
//    free(event);
//
//    printf("\n示例程序结束\n");
//    return 0;
//}
//


/*
 * performance_comparison.c - AST vs VM 性能对比测试
 * 测试不同复杂度规则下的性能差异
 */

#include "rule_engine.h"
#include "rule_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
double get_time_ms() {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * 1000.0 / freq.QuadPart;
}
#else
#include <sys/time.h>
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
#endif

// 创建复杂的测试事件
event_context_t* create_complex_event() {
    event_context_t* event = (event_context_t*)malloc(sizeof(event_context_t));
    memset(event, 0, sizeof(event_context_t));

    // 设置各种字段以测试不同条件
    event->process.name = "powershell1.exe";
    event->process.pid = 1856;
    event->process.ppid = 1024;
    event->process.cmdline = "powershell.exe -ExecutionPolicy Bypass -EncodedCommand U3RhcnQtUHJvY2VzcyAtRmlsZVBhdGggImNtZC5leGUiIC1Bcmd1bWVudExpc3QgIi9jIiwgIndob2FtaSI=";
    event->process.path = "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
    event->process.user = "admin_user";
    event->process.session_id = 2;

    event->file.path = "C:\\Windows\\Temp\\suspicious_file.exe";
    event->file.name = "suspicious_file.exe";
    event->file.extension = "exe";
    event->file.size = 524288; // 512KB
    event->file.operation = "create";
    event->file.attributes = 0x20;

    event->registry.key = "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    event->registry.value_name = "UpdateCheck";
    event->registry.value_type = 1;
    event->registry.value_data = "C:\\Users\\Public\\update.exe";
    event->registry.operation = "set";

    event->network.src_ip = "192.168.1.100";
    event->network.dst_ip = "185.220.101.45";
    event->network.src_port = 49152;
    event->network.dst_port = 443;
    event->network.protocol = "TCP";

    return event;
}

// 测试规则结构
typedef struct {
    const char* name;
    const char* rule;
    int complexity; // 1-5, 5最复杂
} test_rule_t;

// 不同复杂度的测试规则
test_rule_t test_rules[] = {
    // 简单规则 (复杂度 1)
    {
        "简单字段比较",
        "process.name == \"powershell.exe\"",
        1
    },
    {
        "简单数值比较",
        "process.pid > 1000",
        1
    },

    // 中等规则 (复杂度 2-3)
    {
        "基础组合条件",
        "process.name == \"powershell.exe\" && process.pid > 1000 && process.user != \"SYSTEM\"",
        2
    },
    {
        "字符串操作组合",
        "process.cmdline contains \"Bypass\" && process.cmdline contains \"Encoded\"",
        2
    },
    {
        "混合比较和计算",
        "file.size > 100000 && file.size < 1048576 && file.size % 4096 == 0",
        3
    },
    {
        "路径检查组合",
        "process.path startswith \"C:\\\\Windows\" && process.path endswith \".exe\" && process.path contains \"System32\"",
        3
    },

    // 复杂规则 (复杂度 4)
    {
        "多层嵌套条件",
        "(process.name == \"powershell.exe\" || process.name == \"cmd.exe\") && "
        "(process.user != \"SYSTEM\" && process.user != \"LOCAL SERVICE\") && "
        "(process.cmdline contains \"Bypass\" || process.cmdline contains \"Hidden\")",
        4
    },
    {
        "复杂数组和逻辑",
        "network.dst_port in [80, 443, 8080, 8443] && "
        "!network.dst_ip startswith \"192.168.\" && "
        "!network.dst_ip startswith \"10.\" && "
        "network.protocol == \"TCP\"",
        4
    },
    {
        "多字段关联检查",
        "(file.operation == \"create\" || file.operation == \"write\") && "
        "file.path contains \"\\\\Temp\\\\\" && "
        "file.extension in [\"exe\", \"dll\", \"scr\", \"bat\", \"ps1\"] && "
        "file.size > 1024 && file.size < 10485760",
        4
    },

    // 极复杂规则 (复杂度 5)
    {
        "综合恶意行为检测",
        "((process.name == \"powershell.exe\" && process.cmdline contains \"Bypass\" && "
        "  (process.cmdline contains \"Encoded\" || process.cmdline contains \"Hidden\")) || "
        " (process.name == \"cmd.exe\" && process.ppid < 1000 && process.user != \"SYSTEM\")) && "
        "((file.operation in [\"create\", \"write\"] && file.path contains \"\\\\Temp\\\\\" && "
        "  file.extension in [\"exe\", \"dll\", \"bat\"]) || "
        " (registry.operation == \"set\" && registry.key contains \"\\\\Run\" && "
        "  registry.value_data contains \".exe\")) && "
        "(network.dst_port in [80, 443, 8080, 8443] && !network.dst_ip startswith \"192.168.\")",
        5
    },
    {
        "深度嵌套逻辑",
        "(((process.pid > 1000 && process.pid < 2000) || "
        "  (process.ppid > 500 && process.ppid < 1500)) && "
        " ((process.name endswith \".exe\" && process.path startswith \"C:\\\\\") || "
        "  (process.name endswith \".com\" && process.path contains \"System32\"))) && "
        "(((file.size / 1024 > 100 && file.size / 1024 < 1024) || "
        "  (file.size % 512 == 0 && file.attributes & 0x20 != 0)) && "
        " ((network.dst_port == 443 || network.dst_port == 8443) || "
        "  (network.protocol == \"TCP\" && network.src_port > 1024)))",
        5
    },
    {
        "超长条件链",
        "process.name == \"powershell.exe\" && "
        "process.pid > 1000 && process.pid < 10000 && "
        "process.ppid > 0 && process.ppid != process.pid && "
        "process.user != \"SYSTEM\" && process.user != \"\" && "
        "process.cmdline contains \"ps\" && !process.cmdline contains \"help\" && "
        "process.path startswith \"C:\\\\\" && process.path contains \"Windows\" && "
        "file.size > 0 && file.size < 104857600 && "
        "network.dst_port > 0 && network.dst_port < 65536 && "
        "registry.value_type >= 0 && registry.value_type <= 11",
        5
    }
};

// 性能测试函数
void run_performance_test(test_rule_t* rule, event_context_t* event, int iterations) {
    printf("\n=== 测试规则: %s ===\n", rule->name);
    printf("复杂度: %d/5\n", rule->complexity);
    printf("规则: %.60s%s\n", rule->rule, strlen(rule->rule) > 60 ? "..." : "");
    printf("迭代次数: %d\n", iterations);

    // 编译规则
    double compile_start = get_time_ms();
    compiled_rule_t* ast_rule = compile_rule(rule->rule);
    double ast_compile_time = get_time_ms() - compile_start;

    compile_start = get_time_ms();
    vm_bytecode_t* bytecode = compile_rule_to_bytecode(rule->rule);
    double vm_compile_time = get_time_ms() - compile_start;

    if (!ast_rule->is_valid || !bytecode->is_valid) {
        printf("规则编译失败!\n");
        if (ast_rule) destroy_compiled_rule(ast_rule);
        if (bytecode) destroy_bytecode(bytecode);
        return;
    }

    printf("\n编译时间:\n");
    printf("  AST编译: %.3f ms\n", ast_compile_time);
    printf("  VM编译:  %.3f ms (包含AST编译)\n", vm_compile_time);

    // 打印字节码信息
    printf("\n字节码统计:\n");
    printf("  指令数: %zu\n", bytecode->instruction_count);
    printf("  常量数: %zu\n", bytecode->constant_count);

    // 预热
    for (int i = 0; i < 10; i++) {
        evaluate_rule(ast_rule, event);
        vm_instance_t* vm = create_vm_instance(bytecode);
        vm_execute(vm, event);
        destroy_vm_instance(vm);
    }

    // AST执行测试
    bool ast_result = false;
    double ast_start = get_time_ms();

    for (int i = 0; i < iterations; i++) {
        ast_result = evaluate_rule(ast_rule, event);
    }

    double ast_time = get_time_ms() - ast_start;
    double ast_avg = ast_time / iterations;

    // VM执行测试（包括VM创建/销毁）
    bool vm_result = false;
    double vm_start = get_time_ms();

    for (int i = 0; i < iterations; i++) {
        vm_instance_t* vm = create_vm_instance(bytecode);
        if (vm_execute(vm, event)) {
            rule_value_t result = vm_get_result(vm);
            vm_result = (result.type == RULE_TYPE_BOOLEAN && result.value.boolean);
            free_rule_value(&result);
        }
        destroy_vm_instance(vm);
    }

    double vm_time = get_time_ms() - vm_start;
    double vm_avg = vm_time / iterations;

    // VM执行测试（重用VM实例）
    double vm_reuse_start = get_time_ms();
    vm_instance_t* reused_vm = create_vm_instance(bytecode);

    for (int i = 0; i < iterations; i++) {
        if (vm_execute(reused_vm, event)) {
            rule_value_t result = vm_get_result(reused_vm);
            vm_result = (result.type == RULE_TYPE_BOOLEAN && result.value.boolean);
            free_rule_value(&result);
        }
    }

    destroy_vm_instance(reused_vm);
    double vm_reuse_time = get_time_ms() - vm_reuse_start;
    double vm_reuse_avg = vm_reuse_time / iterations;

    // 结果分析
    printf("\n执行结果:\n");
    printf("  AST结果: %s\n", ast_result ? "匹配" : "不匹配");
    printf("  VM结果:  %s\n", vm_result ? "匹配" : "不匹配");
    printf("  结果一致: %s\n", (ast_result == vm_result) ? "✓" : "✗");

    printf("\n性能统计:\n");
    printf("  AST总时间:        %8.3f ms (平均: %.4f ms/次)\n", ast_time, ast_avg);
    printf("  VM总时间(含创建): %8.3f ms (平均: %.4f ms/次)\n", vm_time, vm_avg);
    printf("  VM总时间(重用):   %8.3f ms (平均: %.4f ms/次)\n", vm_reuse_time, vm_reuse_avg);

    printf("\n性能对比:\n");
    double speedup1 = ast_time / vm_time;
    double speedup2 = ast_time / vm_reuse_time;
    printf("  VM vs AST (含创建): %.2fx %s\n",
        speedup1 > 1 ? speedup1 : 1 / speedup1,
        speedup1 > 1 ? "更快 ✓" : "更慢 ✗");
    printf("  VM vs AST (重用):   %.2fx %s\n",
        speedup2 > 1 ? speedup2 : 1 / speedup2,
        speedup2 > 1 ? "更快 ✓" : "更慢 ✗");

    // 计算盈亏平衡点
    if (vm_compile_time > ast_compile_time && vm_reuse_avg < ast_avg) {
        double compile_diff = vm_compile_time - ast_compile_time;
        double exec_diff = ast_avg - vm_reuse_avg;
        int breakeven = (int)(compile_diff / exec_diff) + 1;
        printf("\n盈亏平衡点: 执行 %d 次后VM更快\n", breakeven);
    }

    // 清理
    destroy_compiled_rule(ast_rule);
    destroy_bytecode(bytecode);
}

// 复杂度分组测试
void test_by_complexity() {
    event_context_t* event = create_complex_event();
    int rule_count = sizeof(test_rules) / sizeof(test_rules[0]);

    printf("\n========== 性能对比测试报告 ==========\n");

    for (int complexity = 1; complexity <= 5; complexity++) {
        printf("\n\n########## 复杂度 %d 规则测试 ##########\n", complexity);

        double total_ast_time = 0;
        double total_vm_time = 0;
        double total_vm_reuse_time = 0;
        int count = 0;

        for (int i = 0; i < rule_count; i++) {
            if (test_rules[i].complexity == complexity) {
                // 根据复杂度调整迭代次数
                int iterations = 10000 / complexity;

                run_performance_test(&test_rules[i], event, iterations);
                count++;
            }
        }
    }

    free(event);
}

// 单规则深度测试
void deep_test_single_rule(const char* rule_str, int max_iterations) {
    printf("\n========== 单规则深度测试 ==========\n");
    printf("规则: %s\n", rule_str);

    event_context_t* event = create_complex_event();
    compiled_rule_t* ast_rule = compile_rule(rule_str);
    vm_bytecode_t* bytecode = compile_rule_to_bytecode(rule_str);

    if (!ast_rule->is_valid || !bytecode->is_valid) {
        printf("规则编译失败!\n");
        destroy_compiled_rule(ast_rule);
        destroy_bytecode(bytecode);
        free(event);
        return;
    }

    printf("\n迭代次数\tAST(ms)\t\tVM(ms)\t\tVM重用(ms)\t加速比\n");
    printf("--------\t-------\t\t------\t\t----------\t------\n");

    int iterations[] = { 1, 10, 100, 1000, 10000, 100000 };
    int test_count = sizeof(iterations) / sizeof(iterations[0]);

    if (max_iterations > 0) {
        test_count = 0;
        for (int i = 0; i < sizeof(iterations) / sizeof(iterations[0]); i++) {
            if (iterations[i] <= max_iterations) test_count++;
        }
    }

    for (int i = 0; i < test_count; i++) {
        int iter = iterations[i];

        // AST测试
        double ast_start = get_time_ms();
        for (int j = 0; j < iter; j++) {
            evaluate_rule(ast_rule, event);
        }
        double ast_time = get_time_ms() - ast_start;

        // VM测试（含创建）
        double vm_start = get_time_ms();
        for (int j = 0; j < iter; j++) {
            vm_instance_t* vm = create_vm_instance(bytecode);
            vm_execute(vm, event);
            destroy_vm_instance(vm);
        }
        double vm_time = get_time_ms() - vm_start;

        // VM测试（重用）
        double vm_reuse_start = get_time_ms();
        vm_instance_t* vm = create_vm_instance(bytecode);
        for (int j = 0; j < iter; j++) {
            vm_execute(vm, event);
        }
        destroy_vm_instance(vm);
        double vm_reuse_time = get_time_ms() - vm_reuse_start;

        double speedup = ast_time / vm_reuse_time;

        printf("%d\t\t%.3f\t\t%.3f\t\t%.3f\t\t%.2fx\n",
            iter, ast_time, vm_time, vm_reuse_time, speedup);
    }

    destroy_compiled_rule(ast_rule);
    destroy_bytecode(bytecode);
    free(event);
}

// 主函数
int main(int argc, char* argv[]) {
    printf("AST vs VM 性能对比测试\n");
    printf("====================\n");

    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        printf("\n快速测试模式\n");

        // 只测试一个复杂规则
        event_context_t* event = create_complex_event();
        test_rule_t complex_rule = {
            "快速测试规则",
            test_rules[10].rule, // 使用一个复杂度5的规则
            5
        };
        run_performance_test(&complex_rule, event, 1000);
        free(event);
    }
    else {
        // 完整测试
        test_by_complexity();

        // 单规则深度测试
        printf("\n\n");
        deep_test_single_rule(
            "((process.name == \"powershell.exe\" && process.cmdline contains \"Bypass\") || "
            "(process.name == \"cmd.exe\" && process.user != \"SYSTEM\")) && "
            "network.dst_port in [80, 443, 8080, 8443]",
            10000
        );
    }

    printf("\n\n测试完成!\n");
    return 0;
}

