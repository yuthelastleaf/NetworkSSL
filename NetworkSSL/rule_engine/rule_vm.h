/*
 * rule_vm.h - 规则虚拟机定义
 * 将AST编译为字节码并执行
 */

#pragma once

#include "rule_engine.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    // ==================== 虚拟机指令集 ====================

    typedef enum {
        // 栈操作指令
        OP_NOP = 0x00,          // 无操作
        OP_PUSH_NUM,            // 压入数字常量
        OP_PUSH_STR,            // 压入字符串常量
        OP_PUSH_BOOL,           // 压入布尔常量
        OP_PUSH_NULL,           // 压入NULL
        OP_PUSH_FIELD,          // 压入字段值
        OP_POP,                 // 弹出栈顶
        OP_DUP,                 // 复制栈顶
        OP_SWAP,                // 交换栈顶两个元素

        // 算术运算指令
        OP_ADD = 0x10,          // 加法
        OP_SUB,                 // 减法
        OP_MUL,                 // 乘法
        OP_DIV,                 // 除法
        OP_MOD,                 // 取模
        OP_NEG,                 // 取负

        // 比较运算指令
        OP_EQ = 0x20,           // 等于
        OP_NE,                  // 不等于
        OP_LT,                  // 小于
        OP_LE,                  // 小于等于
        OP_GT,                  // 大于
        OP_GE,                  // 大于等于

        // 逻辑运算指令
        OP_AND = 0x30,          // 逻辑与
        OP_OR,                  // 逻辑或
        OP_NOT,                 // 逻辑非

        // 字符串运算指令
        OP_CONTAINS = 0x40,     // 包含
        OP_STARTSWITH,          // 开始于
        OP_ENDSWITH,            // 结束于
        OP_REGEX,               // 正则匹配
        OP_ICONTAINS,           // 忽略大小写包含

        // 数组运算指令
        OP_IN = 0x50,           // 在数组中
        OP_PUSH_ARRAY,          // 压入数组
        OP_ARRAY_BEGIN,         // 数组开始标记
        OP_ARRAY_END,           // 数组结束标记

        // 控制流指令
        OP_JMP = 0x60,          // 无条件跳转
        OP_JMP_TRUE,            // 条件为真时跳转
        OP_JMP_FALSE,           // 条件为假时跳转
        OP_CALL,                // 函数调用
        OP_RET,                 // 返回

        // 特殊指令
        OP_HALT = 0xFF,         // 停止执行
    } vm_opcode_t;

    // ==================== 字节码结构 ====================

    // 字节码指令
    typedef struct {
        uint8_t opcode;         // 操作码
        union {
            int64_t integer;    // 整数操作数
            double number;      // 浮点数操作数
            uint32_t offset;    // 跳转偏移
            field_id_t field_id;// 字段ID
            struct {
                uint32_t index; // 常量池索引
                uint32_t length;// 长度（用于字符串）
            } constant;
        } operand;
    } vm_instruction_t;

    // 常量池项
    typedef struct {
        rule_data_type_t type;
        union {
            double number;
            struct {
                char* data;
                size_t length;
            } string;
            bool boolean;
        } value;
    } vm_constant_t;

    // 字节码程序
    typedef struct {
        // 指令数组
        vm_instruction_t* instructions;
        size_t instruction_count;
        size_t instruction_capacity;

        // 常量池
        vm_constant_t* constants;
        size_t constant_count;
        size_t constant_capacity;

        // 元数据
        char* source;           // 原始规则（调试用）
        bool is_valid;
        char error_message[256];
    } vm_bytecode_t;

    // ==================== 虚拟机状态 ====================

    // VM栈帧
    typedef struct {
        rule_value_t* stack;    // 操作数栈
        size_t stack_size;      // 栈大小
        size_t stack_capacity;  // 栈容量
        size_t pc;              // 程序计数器
    } vm_frame_t;

    // 虚拟机实例
    typedef struct {
        vm_bytecode_t* bytecode;        // 字节码
        vm_frame_t* frame;              // 当前栈帧
        const event_context_t* context; // 事件上下文

        // 执行状态
        bool running;
        bool has_error;
        char error_message[256];

        // 性能统计
        size_t instruction_count;       // 执行的指令数
        size_t max_stack_depth;         // 最大栈深度
    } vm_instance_t;

    // ==================== 编译器API ====================

    // 将AST编译为字节码
    vm_bytecode_t* compile_to_bytecode(const ast_node_t* ast);

    // 从规则字符串直接编译为字节码
    vm_bytecode_t* compile_rule_to_bytecode(const char* rule_string);

    // 销毁字节码
    void destroy_bytecode(vm_bytecode_t* bytecode);

    // 反汇编字节码（调试用）
    void disassemble_bytecode(const vm_bytecode_t* bytecode);

    // ==================== 虚拟机API ====================

    // 创建虚拟机实例
    vm_instance_t* create_vm_instance(vm_bytecode_t* bytecode);

    // 销毁虚拟机实例
    void destroy_vm_instance(vm_instance_t* vm);

    // 执行字节码
    bool vm_execute(vm_instance_t* vm, const event_context_t* context);

    // 获取执行结果
    rule_value_t vm_get_result(vm_instance_t* vm);

    // ==================== 优化的规则评估API ====================

    // 编译并缓存的规则
    typedef struct {
        char* rule_string;          // 原始规则
        vm_bytecode_t* bytecode;    // 编译后的字节码
        compiled_rule_t* ast_rule;  // AST（备用）
    } cached_rule_t;

    // 创建缓存规则
    cached_rule_t* create_cached_rule(const char* rule_string);

    // 销毁缓存规则
    void destroy_cached_rule(cached_rule_t* rule);

    // 使用缓存规则评估事件
    bool evaluate_cached_rule(const cached_rule_t* rule, const event_context_t* context);

    // ==================== 字节码序列化 ====================

    // 将字节码序列化为二进制格式
    uint8_t* serialize_bytecode(const vm_bytecode_t* bytecode, size_t* size);

    // 从二进制格式反序列化字节码
    vm_bytecode_t* deserialize_bytecode(const uint8_t* data, size_t size);

    // 保存字节码到文件
    bool save_bytecode_to_file(const vm_bytecode_t* bytecode, const char* filename);

    // 从文件加载字节码
    vm_bytecode_t* load_bytecode_from_file(const char* filename);

#ifdef __cplusplus
}
#endif