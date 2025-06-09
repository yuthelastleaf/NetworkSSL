/*
 * rule_engine.h - 规则解析引擎头文件
 * 基于TinyExpr扩展，支持字符串、字段引用、逻辑运算符
 */

#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4267) // size_t to int conversion
#endif

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    // ==================== 基础数据类型定义 ====================

    // 扩展的数据类型
    typedef enum {
        RULE_TYPE_UNKNOWN = 0,
        RULE_TYPE_NUMBER,      // 数值: 42, 3.14
        RULE_TYPE_STRING,      // 字符串: "cmd.exe"
        RULE_TYPE_BOOLEAN,     // 布尔值: true, false
        RULE_TYPE_FIELD_REF,   // 字段引用: process.name
        RULE_TYPE_ARRAY,       // 数组: [1, 2, 3]
        RULE_TYPE_NULL         // 空值: null
    } rule_data_type_t;

    // 值容器结构
    typedef struct rule_value {
        rule_data_type_t type;
        union {
            double number;
            struct {
                char* data;
                size_t length;
            } string;
            bool boolean;
            struct {
                char* object_name;    // "process"
                char* field_name;     // "name"
                int field_id;         // 编译时确定的字段ID
            } field_ref;
            struct {
                struct rule_value* items;
                size_t count;
            } array;
        } value;
    } rule_value_t;

    // ==================== 字段ID映射表 ====================

    // 预定义的字段ID (编译时优化)
    typedef enum {
        FIELD_UNKNOWN = 0,

        // 进程相关字段
        FIELD_PROCESS_NAME = 1,         // process.name
        FIELD_PROCESS_PID,              // process.pid
        FIELD_PROCESS_PPID,             // process.ppid
        FIELD_PROCESS_CMDLINE,          // process.cmdline
        FIELD_PROCESS_PATH,             // process.path
        FIELD_PROCESS_USER,             // process.user
        FIELD_PROCESS_SESSION_ID,       // process.session_id

        // 文件相关字段
        FIELD_FILE_PATH = 100,          // file.path
        FIELD_FILE_NAME,                // file.name
        FIELD_FILE_EXTENSION,           // file.extension
        FIELD_FILE_SIZE,                // file.size
        FIELD_FILE_OPERATION,           // file.operation
        FIELD_FILE_ATTRIBUTES,          // file.attributes

        // 注册表相关字段
        FIELD_REGISTRY_KEY = 200,       // registry.key
        FIELD_REGISTRY_VALUE_NAME,      // registry.value_name
        FIELD_REGISTRY_VALUE_TYPE,      // registry.value_type
        FIELD_REGISTRY_VALUE_DATA,      // registry.value_data
        FIELD_REGISTRY_OPERATION,       // registry.operation

        // 网络相关字段
        FIELD_NETWORK_SRC_IP = 300,     // network.src_ip
        FIELD_NETWORK_DST_IP,           // network.dst_ip
        FIELD_NETWORK_SRC_PORT,         // network.src_port
        FIELD_NETWORK_DST_PORT,         // network.dst_port
        FIELD_NETWORK_PROTOCOL,         // network.protocol

    } field_id_t;

    // ==================== AST节点定义 ====================

    // Token类型
    typedef enum {
        // 基础类型
        TOK_EOF = 0,
        TOK_ERROR,
        TOK_NULL,

        // 字面量
        TOK_NUMBER,           // 42, 3.14
        TOK_STRING,           // "cmd.exe", 'notepad.exe'
        TOK_BOOLEAN,          // true, false
        TOK_IDENTIFIER,       // process, name
        TOK_FIELD_REF,        // process.name
        TOK_ARRAY,            // [1, 2, 3]

        // 算术运算符
        TOK_PLUS,             // +
        TOK_MINUS,            // -
        TOK_MULTIPLY,         // *
        TOK_DIVIDE,           // /
        TOK_MODULO,           // %

        // 比较运算符
        TOK_EQ,               // ==
        TOK_NE,               // !=
        TOK_LT,               // <
        TOK_LE,               // <=
        TOK_GT,               // >
        TOK_GE,               // >=

        // 逻辑运算符
        TOK_AND,              // &&
        TOK_OR,               // ||
        TOK_NOT,              // !

        // 字符串运算符
        TOK_CONTAINS,         // contains
        TOK_STARTSWITH,       // startswith
        TOK_ENDSWITH,         // endswith
        TOK_REGEX,            // regex
        TOK_ICONTAINS,        // icontains (忽略大小写)

        // 数组运算符
        TOK_IN,               // in
        TOK_NOT_IN,           // not in

        // 分隔符
        TOK_LPAREN,           // (
        TOK_RPAREN,           // )
        TOK_LBRACKET,         // [
        TOK_RBRACKET,         // ]
        TOK_COMMA,            // ,
        TOK_DOT,              // .
        TOK_SEMICOLON,        // ;

        // 关键字
        TOK_LET,              // let (变量定义)
        TOK_IF,               // if
        TOK_ELSE,             // else
        TOK_WITHIN,           // within (时间窗口)
        TOK_COUNT,            // count (聚合函数)

    } token_type_t;

    // AST节点类型
    typedef enum {
        AST_UNKNOWN = 0,

        // 字面量节点
        AST_NUMBER,           // 数字常量
        AST_STRING,           // 字符串常量
        AST_BOOLEAN,          // 布尔常量
        AST_FIELD_REF,        // 字段引用
        AST_ARRAY,            // 数组字面量

        // 运算符节点
        AST_BINARY_OP,        // 二元运算符
        AST_UNARY_OP,         // 一元运算符
        AST_FUNCTION_CALL,    // 函数调用

        // 控制流节点
        AST_CONDITIONAL,      // 条件表达式 (? :)
        AST_SEQUENCE,         // 序列表达式

    } ast_node_type_t;

    // 前向声明
    struct ast_node;

    // AST节点结构
    typedef struct ast_node {
        ast_node_type_t type;
        int line;
        int column;
        rule_data_type_t value_type;  // 节点计算结果类型

        union {
            // 字面量节点
            rule_value_t literal;

            // 二元运算符节点
            struct {
                token_type_t tok_opt;
                struct ast_node* left;
                struct ast_node* right;
            } binary_op;

            // 一元运算符节点
            struct {
                token_type_t tok_opt;
                struct ast_node* operand;
            } unary_op;

            // 函数调用节点
            struct {
                char* function_name;
                struct ast_node** arguments;
                size_t argument_count;
            } function_call;

            // 条件表达式节点
            struct {
                struct ast_node* condition;
                struct ast_node* true_expr;
                struct ast_node* false_expr;
            } conditional;

            // 数组节点
            struct {
                struct ast_node** elements;
                size_t element_count;
            } array;

        } data;
    } ast_node_t;

    // ==================== 编译后的规则结构 ====================

    // 编译后的规则
    typedef struct compiled_rule {
        ast_node_t* ast;           // 抽象语法树
        char* source;              // 原始规则字符串(用于调试)
        bool is_valid;             // 编译是否成功
        char error_message[256];   // 错误信息
    } compiled_rule_t;

    // ==================== 事件上下文结构 ====================

    // 事件上下文(用于规则评估)
    typedef struct event_context {
        // 进程信息
        struct {
            const char* name;
            int pid;
            int ppid;
            const char* cmdline;
            const char* path;
            const char* user;
            int session_id;
        } process;

        // 文件信息
        struct {
            const char* path;
            const char* name;
            const char* extension;
            size_t size;
            const char* operation;
            int attributes;
        } file;

        // 注册表信息
        struct {
            const char* key;
            const char* value_name;
            int value_type;
            const char* value_data;
            const char* operation;
        } registry;

        // 网络信息
        struct {
            const char* src_ip;
            const char* dst_ip;
            int src_port;
            int dst_port;
            const char* protocol;
        } network;

        // 扩展字段(用户自定义)
        void* user_data;
    } event_context_t;

    // ==================== 公共API ====================

    // 编译规则
    // 输入: rule_string - 规则字符串
    // 返回: 编译后的规则结构，调用者需要使用destroy_compiled_rule释放
    compiled_rule_t* compile_rule(const char* rule_string);

    // 销毁编译后的规则
    void destroy_compiled_rule(compiled_rule_t* rule);

    // 评估规则
    // 输入: rule - 编译后的规则, context - 事件上下文
    // 返回: 评估结果(true/false)
    bool evaluate_rule(const compiled_rule_t* rule, const event_context_t* context);

    // 评估规则并返回详细结果
    // 输入: rule - 编译后的规则, context - 事件上下文, result - 输出详细结果
    // 返回: 评估是否成功
    bool evaluate_rule_ex(const compiled_rule_t* rule, const event_context_t* context, rule_value_t* result);

    // 释放规则值
    void free_rule_value(rule_value_t* value);

    // 复制规则值
    rule_value_t copy_rule_value(const rule_value_t* value);

    // 辅助函数
    field_id_t resolve_field_id(const char* object_name, const char* field_name);
    rule_data_type_t get_field_type(field_id_t field_id);

    // 调试函数
    void print_ast(const ast_node_t* node, int indent);
    const char* token_type_to_string(token_type_t type);
    const char* ast_node_type_to_string(ast_node_type_t type);

#ifdef __cplusplus
}
#endif