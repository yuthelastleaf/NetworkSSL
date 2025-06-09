/*
 * rule_vm.h - �������������
 * ��AST����Ϊ�ֽ��벢ִ��
 */

#pragma once

#include "rule_engine.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    // ==================== �����ָ� ====================

    typedef enum {
        // ջ����ָ��
        OP_NOP = 0x00,          // �޲���
        OP_PUSH_NUM,            // ѹ�����ֳ���
        OP_PUSH_STR,            // ѹ���ַ�������
        OP_PUSH_BOOL,           // ѹ�벼������
        OP_PUSH_NULL,           // ѹ��NULL
        OP_PUSH_FIELD,          // ѹ���ֶ�ֵ
        OP_POP,                 // ����ջ��
        OP_DUP,                 // ����ջ��
        OP_SWAP,                // ����ջ������Ԫ��

        // ��������ָ��
        OP_ADD = 0x10,          // �ӷ�
        OP_SUB,                 // ����
        OP_MUL,                 // �˷�
        OP_DIV,                 // ����
        OP_MOD,                 // ȡģ
        OP_NEG,                 // ȡ��

        // �Ƚ�����ָ��
        OP_EQ = 0x20,           // ����
        OP_NE,                  // ������
        OP_LT,                  // С��
        OP_LE,                  // С�ڵ���
        OP_GT,                  // ����
        OP_GE,                  // ���ڵ���

        // �߼�����ָ��
        OP_AND = 0x30,          // �߼���
        OP_OR,                  // �߼���
        OP_NOT,                 // �߼���

        // �ַ�������ָ��
        OP_CONTAINS = 0x40,     // ����
        OP_STARTSWITH,          // ��ʼ��
        OP_ENDSWITH,            // ������
        OP_REGEX,               // ����ƥ��
        OP_ICONTAINS,           // ���Դ�Сд����

        // ��������ָ��
        OP_IN = 0x50,           // ��������
        OP_PUSH_ARRAY,          // ѹ������
        OP_ARRAY_BEGIN,         // ���鿪ʼ���
        OP_ARRAY_END,           // ����������

        // ������ָ��
        OP_JMP = 0x60,          // ��������ת
        OP_JMP_TRUE,            // ����Ϊ��ʱ��ת
        OP_JMP_FALSE,           // ����Ϊ��ʱ��ת
        OP_CALL,                // ��������
        OP_RET,                 // ����

        // ����ָ��
        OP_HALT = 0xFF,         // ִֹͣ��
    } vm_opcode_t;

    // ==================== �ֽ���ṹ ====================

    // �ֽ���ָ��
    typedef struct {
        uint8_t opcode;         // ������
        union {
            int64_t integer;    // ����������
            double number;      // ������������
            uint32_t offset;    // ��תƫ��
            field_id_t field_id;// �ֶ�ID
            struct {
                uint32_t index; // ����������
                uint32_t length;// ���ȣ������ַ�����
            } constant;
        } operand;
    } vm_instruction_t;

    // ��������
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

    // �ֽ������
    typedef struct {
        // ָ������
        vm_instruction_t* instructions;
        size_t instruction_count;
        size_t instruction_capacity;

        // ������
        vm_constant_t* constants;
        size_t constant_count;
        size_t constant_capacity;

        // Ԫ����
        char* source;           // ԭʼ���򣨵����ã�
        bool is_valid;
        char error_message[256];
    } vm_bytecode_t;

    // ==================== �����״̬ ====================

    // VMջ֡
    typedef struct {
        rule_value_t* stack;    // ������ջ
        size_t stack_size;      // ջ��С
        size_t stack_capacity;  // ջ����
        size_t pc;              // ���������
    } vm_frame_t;

    // �����ʵ��
    typedef struct {
        vm_bytecode_t* bytecode;        // �ֽ���
        vm_frame_t* frame;              // ��ǰջ֡
        const event_context_t* context; // �¼�������

        // ִ��״̬
        bool running;
        bool has_error;
        char error_message[256];

        // ����ͳ��
        size_t instruction_count;       // ִ�е�ָ����
        size_t max_stack_depth;         // ���ջ���
    } vm_instance_t;

    // ==================== ������API ====================

    // ��AST����Ϊ�ֽ���
    vm_bytecode_t* compile_to_bytecode(const ast_node_t* ast);

    // �ӹ����ַ���ֱ�ӱ���Ϊ�ֽ���
    vm_bytecode_t* compile_rule_to_bytecode(const char* rule_string);

    // �����ֽ���
    void destroy_bytecode(vm_bytecode_t* bytecode);

    // ������ֽ��루�����ã�
    void disassemble_bytecode(const vm_bytecode_t* bytecode);

    // ==================== �����API ====================

    // ���������ʵ��
    vm_instance_t* create_vm_instance(vm_bytecode_t* bytecode);

    // ���������ʵ��
    void destroy_vm_instance(vm_instance_t* vm);

    // ִ���ֽ���
    bool vm_execute(vm_instance_t* vm, const event_context_t* context);

    // ��ȡִ�н��
    rule_value_t vm_get_result(vm_instance_t* vm);

    // ==================== �Ż��Ĺ�������API ====================

    // ���벢����Ĺ���
    typedef struct {
        char* rule_string;          // ԭʼ����
        vm_bytecode_t* bytecode;    // �������ֽ���
        compiled_rule_t* ast_rule;  // AST�����ã�
    } cached_rule_t;

    // �����������
    cached_rule_t* create_cached_rule(const char* rule_string);

    // ���ٻ������
    void destroy_cached_rule(cached_rule_t* rule);

    // ʹ�û�����������¼�
    bool evaluate_cached_rule(const cached_rule_t* rule, const event_context_t* context);

    // ==================== �ֽ������л� ====================

    // ���ֽ������л�Ϊ�����Ƹ�ʽ
    uint8_t* serialize_bytecode(const vm_bytecode_t* bytecode, size_t* size);

    // �Ӷ����Ƹ�ʽ�����л��ֽ���
    vm_bytecode_t* deserialize_bytecode(const uint8_t* data, size_t size);

    // �����ֽ��뵽�ļ�
    bool save_bytecode_to_file(const vm_bytecode_t* bytecode, const char* filename);

    // ���ļ������ֽ���
    vm_bytecode_t* load_bytecode_from_file(const char* filename);

#ifdef __cplusplus
}
#endif