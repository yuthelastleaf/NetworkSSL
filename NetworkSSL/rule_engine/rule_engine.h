/*
 * rule_engine.h - �����������ͷ�ļ�
 * ����TinyExpr��չ��֧���ַ������ֶ����á��߼������
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

    // ==================== �����������Ͷ��� ====================

    // ��չ����������
    typedef enum {
        RULE_TYPE_UNKNOWN = 0,
        RULE_TYPE_NUMBER,      // ��ֵ: 42, 3.14
        RULE_TYPE_STRING,      // �ַ���: "cmd.exe"
        RULE_TYPE_BOOLEAN,     // ����ֵ: true, false
        RULE_TYPE_FIELD_REF,   // �ֶ�����: process.name
        RULE_TYPE_ARRAY,       // ����: [1, 2, 3]
        RULE_TYPE_NULL         // ��ֵ: null
    } rule_data_type_t;

    // ֵ�����ṹ
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
                int field_id;         // ����ʱȷ�����ֶ�ID
            } field_ref;
            struct {
                struct rule_value* items;
                size_t count;
            } array;
        } value;
    } rule_value_t;

    // ==================== �ֶ�IDӳ��� ====================

    // Ԥ������ֶ�ID (����ʱ�Ż�)
    typedef enum {
        FIELD_UNKNOWN = 0,

        // ��������ֶ�
        FIELD_PROCESS_NAME = 1,         // process.name
        FIELD_PROCESS_PID,              // process.pid
        FIELD_PROCESS_PPID,             // process.ppid
        FIELD_PROCESS_CMDLINE,          // process.cmdline
        FIELD_PROCESS_PATH,             // process.path
        FIELD_PROCESS_USER,             // process.user
        FIELD_PROCESS_SESSION_ID,       // process.session_id

        // �ļ�����ֶ�
        FIELD_FILE_PATH = 100,          // file.path
        FIELD_FILE_NAME,                // file.name
        FIELD_FILE_EXTENSION,           // file.extension
        FIELD_FILE_SIZE,                // file.size
        FIELD_FILE_OPERATION,           // file.operation
        FIELD_FILE_ATTRIBUTES,          // file.attributes

        // ע�������ֶ�
        FIELD_REGISTRY_KEY = 200,       // registry.key
        FIELD_REGISTRY_VALUE_NAME,      // registry.value_name
        FIELD_REGISTRY_VALUE_TYPE,      // registry.value_type
        FIELD_REGISTRY_VALUE_DATA,      // registry.value_data
        FIELD_REGISTRY_OPERATION,       // registry.operation

        // ��������ֶ�
        FIELD_NETWORK_SRC_IP = 300,     // network.src_ip
        FIELD_NETWORK_DST_IP,           // network.dst_ip
        FIELD_NETWORK_SRC_PORT,         // network.src_port
        FIELD_NETWORK_DST_PORT,         // network.dst_port
        FIELD_NETWORK_PROTOCOL,         // network.protocol

    } field_id_t;

    // ==================== AST�ڵ㶨�� ====================

    // Token����
    typedef enum {
        // ��������
        TOK_EOF = 0,
        TOK_ERROR,
        TOK_NULL,

        // ������
        TOK_NUMBER,           // 42, 3.14
        TOK_STRING,           // "cmd.exe", 'notepad.exe'
        TOK_BOOLEAN,          // true, false
        TOK_IDENTIFIER,       // process, name
        TOK_FIELD_REF,        // process.name
        TOK_ARRAY,            // [1, 2, 3]

        // ���������
        TOK_PLUS,             // +
        TOK_MINUS,            // -
        TOK_MULTIPLY,         // *
        TOK_DIVIDE,           // /
        TOK_MODULO,           // %

        // �Ƚ������
        TOK_EQ,               // ==
        TOK_NE,               // !=
        TOK_LT,               // <
        TOK_LE,               // <=
        TOK_GT,               // >
        TOK_GE,               // >=

        // �߼������
        TOK_AND,              // &&
        TOK_OR,               // ||
        TOK_NOT,              // !

        // �ַ��������
        TOK_CONTAINS,         // contains
        TOK_STARTSWITH,       // startswith
        TOK_ENDSWITH,         // endswith
        TOK_REGEX,            // regex
        TOK_ICONTAINS,        // icontains (���Դ�Сд)

        // ���������
        TOK_IN,               // in
        TOK_NOT_IN,           // not in

        // �ָ���
        TOK_LPAREN,           // (
        TOK_RPAREN,           // )
        TOK_LBRACKET,         // [
        TOK_RBRACKET,         // ]
        TOK_COMMA,            // ,
        TOK_DOT,              // .
        TOK_SEMICOLON,        // ;

        // �ؼ���
        TOK_LET,              // let (��������)
        TOK_IF,               // if
        TOK_ELSE,             // else
        TOK_WITHIN,           // within (ʱ�䴰��)
        TOK_COUNT,            // count (�ۺϺ���)

    } token_type_t;

    // AST�ڵ�����
    typedef enum {
        AST_UNKNOWN = 0,

        // �������ڵ�
        AST_NUMBER,           // ���ֳ���
        AST_STRING,           // �ַ�������
        AST_BOOLEAN,          // ��������
        AST_FIELD_REF,        // �ֶ�����
        AST_ARRAY,            // ����������

        // ������ڵ�
        AST_BINARY_OP,        // ��Ԫ�����
        AST_UNARY_OP,         // һԪ�����
        AST_FUNCTION_CALL,    // ��������

        // �������ڵ�
        AST_CONDITIONAL,      // �������ʽ (? :)
        AST_SEQUENCE,         // ���б��ʽ

    } ast_node_type_t;

    // ǰ������
    struct ast_node;

    // AST�ڵ�ṹ
    typedef struct ast_node {
        ast_node_type_t type;
        int line;
        int column;
        rule_data_type_t value_type;  // �ڵ����������

        union {
            // �������ڵ�
            rule_value_t literal;

            // ��Ԫ������ڵ�
            struct {
                token_type_t tok_opt;
                struct ast_node* left;
                struct ast_node* right;
            } binary_op;

            // һԪ������ڵ�
            struct {
                token_type_t tok_opt;
                struct ast_node* operand;
            } unary_op;

            // �������ýڵ�
            struct {
                char* function_name;
                struct ast_node** arguments;
                size_t argument_count;
            } function_call;

            // �������ʽ�ڵ�
            struct {
                struct ast_node* condition;
                struct ast_node* true_expr;
                struct ast_node* false_expr;
            } conditional;

            // ����ڵ�
            struct {
                struct ast_node** elements;
                size_t element_count;
            } array;

        } data;
    } ast_node_t;

    // ==================== �����Ĺ���ṹ ====================

    // �����Ĺ���
    typedef struct compiled_rule {
        ast_node_t* ast;           // �����﷨��
        char* source;              // ԭʼ�����ַ���(���ڵ���)
        bool is_valid;             // �����Ƿ�ɹ�
        char error_message[256];   // ������Ϣ
    } compiled_rule_t;

    // ==================== �¼������Ľṹ ====================

    // �¼�������(���ڹ�������)
    typedef struct event_context {
        // ������Ϣ
        struct {
            const char* name;
            int pid;
            int ppid;
            const char* cmdline;
            const char* path;
            const char* user;
            int session_id;
        } process;

        // �ļ���Ϣ
        struct {
            const char* path;
            const char* name;
            const char* extension;
            size_t size;
            const char* operation;
            int attributes;
        } file;

        // ע�����Ϣ
        struct {
            const char* key;
            const char* value_name;
            int value_type;
            const char* value_data;
            const char* operation;
        } registry;

        // ������Ϣ
        struct {
            const char* src_ip;
            const char* dst_ip;
            int src_port;
            int dst_port;
            const char* protocol;
        } network;

        // ��չ�ֶ�(�û��Զ���)
        void* user_data;
    } event_context_t;

    // ==================== ����API ====================

    // �������
    // ����: rule_string - �����ַ���
    // ����: �����Ĺ���ṹ����������Ҫʹ��destroy_compiled_rule�ͷ�
    compiled_rule_t* compile_rule(const char* rule_string);

    // ���ٱ����Ĺ���
    void destroy_compiled_rule(compiled_rule_t* rule);

    // ��������
    // ����: rule - �����Ĺ���, context - �¼�������
    // ����: �������(true/false)
    bool evaluate_rule(const compiled_rule_t* rule, const event_context_t* context);

    // �������򲢷�����ϸ���
    // ����: rule - �����Ĺ���, context - �¼�������, result - �����ϸ���
    // ����: �����Ƿ�ɹ�
    bool evaluate_rule_ex(const compiled_rule_t* rule, const event_context_t* context, rule_value_t* result);

    // �ͷŹ���ֵ
    void free_rule_value(rule_value_t* value);

    // ���ƹ���ֵ
    rule_value_t copy_rule_value(const rule_value_t* value);

    // ��������
    field_id_t resolve_field_id(const char* object_name, const char* field_name);
    rule_data_type_t get_field_type(field_id_t field_id);

    // ���Ժ���
    void print_ast(const ast_node_t* node, int indent);
    const char* token_type_to_string(token_type_t type);
    const char* ast_node_type_to_string(ast_node_type_t type);

#ifdef __cplusplus
}
#endif