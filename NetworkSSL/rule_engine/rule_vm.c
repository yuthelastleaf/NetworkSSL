/*
 * rule_vm.c - ���������ʵ��
 */

#include "rule_vm.h"
#include "rule_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

 // ==================== ǰ������ ====================

 // ��rule_engine.c�������ĺ���
extern rule_value_t evaluate_binary_op(token_type_t op, const rule_value_t* left, const rule_value_t* right);
extern rule_value_t evaluate_unary_op(token_type_t op, const rule_value_t* operand);
extern rule_value_t get_field_value(field_id_t field_id, const event_context_t* context);

// ==================== �ֽ��������ʵ�� ====================

// ������������
typedef struct {
    vm_bytecode_t* bytecode;
    bool has_error;
    char error_message[256];
} compiler_context_t;

// ����������������
static compiler_context_t* create_compiler_context() {
    compiler_context_t* ctx = (compiler_context_t*)malloc(sizeof(compiler_context_t));
    ctx->bytecode = (vm_bytecode_t*)malloc(sizeof(vm_bytecode_t));

    // ��ʼ���ֽ���ṹ
    ctx->bytecode->instructions = NULL;
    ctx->bytecode->instruction_count = 0;
    ctx->bytecode->instruction_capacity = 0;
    ctx->bytecode->constants = NULL;
    ctx->bytecode->constant_count = 0;
    ctx->bytecode->constant_capacity = 0;
    ctx->bytecode->source = NULL;
    ctx->bytecode->is_valid = true;
    ctx->bytecode->error_message[0] = '\0';

    ctx->has_error = false;
    ctx->error_message[0] = '\0';

    return ctx;
}

// ���ٱ�����������
static void destroy_compiler_context(compiler_context_t* ctx) {
    if (ctx) {
        // ע�⣺������bytecode�����ᱻ���ظ�������
        free(ctx);
    }
}

// ���ָ��
static void emit_instruction(compiler_context_t* ctx, vm_instruction_t inst) {
    if (ctx->has_error) return;

    // ����
    if (ctx->bytecode->instruction_count >= ctx->bytecode->instruction_capacity) {
        size_t new_capacity = ctx->bytecode->instruction_capacity == 0 ? 16 : ctx->bytecode->instruction_capacity * 2;
        ctx->bytecode->instructions = (vm_instruction_t*)realloc(
            ctx->bytecode->instructions,
            sizeof(vm_instruction_t) * new_capacity
        );
        ctx->bytecode->instruction_capacity = new_capacity;
    }

    ctx->bytecode->instructions[ctx->bytecode->instruction_count++] = inst;
}

// ��ӳ�����������
static uint32_t add_constant(compiler_context_t* ctx, vm_constant_t constant) {
    if (ctx->has_error) return 0;

    // ����Ƿ��Ѵ�����ͬ����
    for (size_t i = 0; i < ctx->bytecode->constant_count; i++) {
        vm_constant_t* existing = &ctx->bytecode->constants[i];
        if (existing->type == constant.type) {
            switch (constant.type) {
            case RULE_TYPE_NUMBER:
                if (existing->value.number == constant.value.number) {
                    return (uint32_t)i;
                }
                break;
            case RULE_TYPE_STRING:
                if (existing->value.string.length == constant.value.string.length &&
                    memcmp(existing->value.string.data, constant.value.string.data, constant.value.string.length) == 0) {
                    return (uint32_t)i;
                }
                break;
            case RULE_TYPE_BOOLEAN:
                if (existing->value.boolean == constant.value.boolean) {
                    return (uint32_t)i;
                }
                break;
            default:
                break;
            }
        }
    }

    // ����
    if (ctx->bytecode->constant_count >= ctx->bytecode->constant_capacity) {
        size_t new_capacity = ctx->bytecode->constant_capacity == 0 ? 8 : ctx->bytecode->constant_capacity * 2;
        ctx->bytecode->constants = (vm_constant_t*)realloc(
            ctx->bytecode->constants,
            sizeof(vm_constant_t) * new_capacity
        );
        ctx->bytecode->constant_capacity = new_capacity;
    }

    // ����³���
    uint32_t index = (uint32_t)ctx->bytecode->constant_count;
    ctx->bytecode->constants[ctx->bytecode->constant_count++] = constant;
    return index;
}

// ǰ������
static void compile_ast_node(compiler_context_t* ctx, const ast_node_t* node);

// ����������
static void compile_literal(compiler_context_t* ctx, const rule_value_t* value) {
    vm_instruction_t inst = { 0 };

    switch (value->type) {
    case RULE_TYPE_NUMBER:
    {
        vm_constant_t constant = { 0 };
        constant.type = RULE_TYPE_NUMBER;
        constant.value.number = value->value.number;
        uint32_t index = add_constant(ctx, constant);

        inst.opcode = OP_PUSH_NUM;
        inst.operand.constant.index = index;
        emit_instruction(ctx, inst);
    }
    break;

    case RULE_TYPE_STRING:
    {
        vm_constant_t constant = { 0 };
        constant.type = RULE_TYPE_STRING;
        constant.value.string.length = value->value.string.length;
        constant.value.string.data = (char*)malloc(constant.value.string.length + 1);
        memcpy(constant.value.string.data, value->value.string.data, constant.value.string.length + 1);
        uint32_t index = add_constant(ctx, constant);

        inst.opcode = OP_PUSH_STR;
        inst.operand.constant.index = index;
        emit_instruction(ctx, inst);
    }
    break;

    case RULE_TYPE_BOOLEAN:
        inst.opcode = OP_PUSH_BOOL;
        inst.operand.integer = value->value.boolean ? 1 : 0;
        emit_instruction(ctx, inst);
        break;

    case RULE_TYPE_FIELD_REF:
        inst.opcode = OP_PUSH_FIELD;
        inst.operand.field_id = value->value.field_ref.field_id;
        emit_instruction(ctx, inst);
        break;

    default:
        inst.opcode = OP_PUSH_NULL;
        emit_instruction(ctx, inst);
        break;
    }
}

// �����Ԫ����
static void compile_binary_op(compiler_context_t* ctx, const ast_node_t* node) {
    // �ȱ����������
    compile_ast_node(ctx, node->data.binary_op.left);

    // �ٱ����Ҳ�����
    compile_ast_node(ctx, node->data.binary_op.right);

    // ��������ָ��
    vm_instruction_t inst = { 0 };

    switch (node->data.binary_op.tok_opt) {
        // ��������
    case TOK_PLUS:     inst.opcode = OP_ADD; break;
    case TOK_MINUS:    inst.opcode = OP_SUB; break;
    case TOK_MULTIPLY: inst.opcode = OP_MUL; break;
    case TOK_DIVIDE:   inst.opcode = OP_DIV; break;
    case TOK_MODULO:   inst.opcode = OP_MOD; break;

        // �Ƚ�����
    case TOK_EQ: inst.opcode = OP_EQ; break;
    case TOK_NE: inst.opcode = OP_NE; break;
    case TOK_LT: inst.opcode = OP_LT; break;
    case TOK_LE: inst.opcode = OP_LE; break;
    case TOK_GT: inst.opcode = OP_GT; break;
    case TOK_GE: inst.opcode = OP_GE; break;

        // �߼�����
    case TOK_AND: inst.opcode = OP_AND; break;
    case TOK_OR:  inst.opcode = OP_OR; break;

        // �ַ�������
    case TOK_CONTAINS:   inst.opcode = OP_CONTAINS; break;
    case TOK_STARTSWITH: inst.opcode = OP_STARTSWITH; break;
    case TOK_ENDSWITH:   inst.opcode = OP_ENDSWITH; break;
    case TOK_REGEX:      inst.opcode = OP_REGEX; break;
    case TOK_ICONTAINS:  inst.opcode = OP_ICONTAINS; break;

        // ��������
    case TOK_IN: inst.opcode = OP_IN; break;

    default:
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message),
            "Unknown binary operator: %d", node->data.binary_op.tok_opt);
        return;
    }

    emit_instruction(ctx, inst);
}

// ����һԪ����
static void compile_unary_op(compiler_context_t* ctx, const ast_node_t* node) {
    // �ȱ��������
    compile_ast_node(ctx, node->data.unary_op.operand);

    // ��������ָ��
    vm_instruction_t inst = { 0 };

    switch (node->data.unary_op.tok_opt) {
    case TOK_NOT:
        inst.opcode = OP_NOT;
        break;
    case TOK_MINUS:
        inst.opcode = OP_NEG;
        break;
    default:
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message),
            "Unknown unary operator: %d", node->data.unary_op.tok_opt);
        return;
    }

    emit_instruction(ctx, inst);
}

// ��������
static void compile_array(compiler_context_t* ctx, const ast_node_t* node) {
    vm_instruction_t inst = { 0 };

    // �������鿪ʼ���
    inst.opcode = OP_ARRAY_BEGIN;
    inst.operand.integer = node->data.array.element_count;
    emit_instruction(ctx, inst);

    // ����ÿ��Ԫ��
    for (size_t i = 0; i < node->data.array.element_count; i++) {
        compile_ast_node(ctx, node->data.array.elements[i]);
    }

    // ��������������
    inst.opcode = OP_ARRAY_END;
    inst.operand.integer = node->data.array.element_count;
    emit_instruction(ctx, inst);
}

// ����AST�ڵ�
static void compile_ast_node(compiler_context_t* ctx, const ast_node_t* node) {
    if (ctx->has_error || !node) return;

    switch (node->type) {
    case AST_NUMBER:
    case AST_STRING:
    case AST_BOOLEAN:
    case AST_FIELD_REF:
        compile_literal(ctx, &node->data.literal);
        break;

    case AST_BINARY_OP:
        compile_binary_op(ctx, node);
        break;

    case AST_UNARY_OP:
        compile_unary_op(ctx, node);
        break;

    case AST_ARRAY:
        compile_array(ctx, node);
        break;

    default:
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message),
            "Unknown AST node type: %d", node->type);
        break;
    }
}

// ��AST����Ϊ�ֽ���
vm_bytecode_t* compile_to_bytecode(const ast_node_t* ast) {
    compiler_context_t* ctx = create_compiler_context();

    // ����AST
    compile_ast_node(ctx, ast);

    // ���HALTָ��
    if (!ctx->has_error) {
        vm_instruction_t halt = { 0 };
        halt.opcode = OP_HALT;
        emit_instruction(ctx, halt);
    }

    // ���ô���״̬
    if (ctx->has_error) {
        ctx->bytecode->is_valid = false;
        strncpy(ctx->bytecode->error_message, ctx->error_message,
            sizeof(ctx->bytecode->error_message) - 1);
    }

    vm_bytecode_t* bytecode = ctx->bytecode;
    destroy_compiler_context(ctx);

    return bytecode;
}

// �ӹ����ַ���ֱ�ӱ���Ϊ�ֽ���
vm_bytecode_t* compile_rule_to_bytecode(const char* rule_string) {
    // �ȱ���ΪAST
    compiled_rule_t* rule = compile_rule(rule_string);
    if (!rule || !rule->is_valid) {
        vm_bytecode_t* bytecode = (vm_bytecode_t*)malloc(sizeof(vm_bytecode_t));
        memset(bytecode, 0, sizeof(vm_bytecode_t));
        bytecode->is_valid = false;
        if (rule) {
            strncpy(bytecode->error_message, rule->error_message,
                sizeof(bytecode->error_message) - 1);
            destroy_compiled_rule(rule);
        }
        else {
            strcpy(bytecode->error_message, "Failed to compile rule");
        }
        return bytecode;
    }

    // ����Ϊ�ֽ���
    vm_bytecode_t* bytecode = compile_to_bytecode(rule->ast);
    bytecode->source = _strdup(rule_string);

    destroy_compiled_rule(rule);
    return bytecode;
}

// �����ֽ���
void destroy_bytecode(vm_bytecode_t* bytecode) {
    if (!bytecode) return;

    if (bytecode->instructions) {
        free(bytecode->instructions);
    }

    if (bytecode->constants) {
        // �ͷ��ַ�������
        for (size_t i = 0; i < bytecode->constant_count; i++) {
            if (bytecode->constants[i].type == RULE_TYPE_STRING &&
                bytecode->constants[i].value.string.data) {
                free(bytecode->constants[i].value.string.data);
            }
        }
        free(bytecode->constants);
    }

    if (bytecode->source) {
        free(bytecode->source);
    }

    free(bytecode);
}

// ==================== �����ʵ�� ====================

// ���������ʵ��
vm_instance_t* create_vm_instance(vm_bytecode_t* bytecode) {
    vm_instance_t* vm = (vm_instance_t*)malloc(sizeof(vm_instance_t));

    vm->bytecode = bytecode;
    vm->context = NULL;
    vm->running = false;
    vm->has_error = false;
    vm->error_message[0] = '\0';
    vm->instruction_count = 0;
    vm->max_stack_depth = 0;

    // ����ջ֡
    vm->frame = (vm_frame_t*)malloc(sizeof(vm_frame_t));
    vm->frame->stack_capacity = 64;
    vm->frame->stack = (rule_value_t*)malloc(sizeof(rule_value_t) * vm->frame->stack_capacity);
    vm->frame->stack_size = 0;
    vm->frame->pc = 0;

    return vm;
}

// ���������ʵ��
void destroy_vm_instance(vm_instance_t* vm) {
    if (!vm) return;

    if (vm->frame) {
        if (vm->frame->stack) {
            // �ͷ�ջ�е�ֵ
            for (size_t i = 0; i < vm->frame->stack_size; i++) {
                free_rule_value(&vm->frame->stack[i]);
            }
            free(vm->frame->stack);
        }
        free(vm->frame);
    }

    free(vm);
}

// ѹջ
static void vm_push(vm_instance_t* vm, rule_value_t value) {
    if (vm->frame->stack_size >= vm->frame->stack_capacity) {
        // ����
        vm->frame->stack_capacity *= 2;
        vm->frame->stack = (rule_value_t*)realloc(
            vm->frame->stack,
            sizeof(rule_value_t) * vm->frame->stack_capacity
        );
    }

    vm->frame->stack[vm->frame->stack_size++] = value;

    if (vm->frame->stack_size > vm->max_stack_depth) {
        vm->max_stack_depth = vm->frame->stack_size;
    }
}

// ��ջ
static rule_value_t vm_pop(vm_instance_t* vm) {
    if (vm->frame->stack_size == 0) {
        vm->has_error = true;
        strcpy(vm->error_message, "Stack underflow");
        rule_value_t null_value = { 0 };
        null_value.type = RULE_TYPE_NULL;
        return null_value;
    }

    return vm->frame->stack[--vm->frame->stack_size];
}

// �鿴ջ��
static rule_value_t* vm_peek(vm_instance_t* vm) {
    if (vm->frame->stack_size == 0) {
        return NULL;
    }
    return &vm->frame->stack[vm->frame->stack_size - 1];
}

// ִ�е���ָ��
static void vm_execute_instruction(vm_instance_t* vm, vm_instruction_t* inst) {
    switch (inst->opcode) {
    case OP_NOP:
        break;

    case OP_PUSH_NUM:
    {
        rule_value_t value = { 0 };
        value.type = RULE_TYPE_NUMBER;
        value.value.number = vm->bytecode->constants[inst->operand.constant.index].value.number;
        vm_push(vm, value);
    }
    break;

    case OP_PUSH_STR:
    {
        rule_value_t value = { 0 };
        value.type = RULE_TYPE_STRING;
        vm_constant_t* constant = &vm->bytecode->constants[inst->operand.constant.index];
        value.value.string.length = constant->value.string.length;
        value.value.string.data = _strdup(constant->value.string.data);
        vm_push(vm, value);
    }
    break;

    case OP_PUSH_BOOL:
    {
        rule_value_t value = { 0 };
        value.type = RULE_TYPE_BOOLEAN;
        value.value.boolean = inst->operand.integer != 0;
        vm_push(vm, value);
    }
    break;

    case OP_PUSH_FIELD:
    {
        rule_value_t value = get_field_value(inst->operand.field_id, vm->context);
        vm_push(vm, value);
    }
    break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
    case OP_EQ:
    case OP_NE:
    case OP_LT:
    case OP_LE:
    case OP_GT:
    case OP_GE:
    case OP_AND:
    case OP_OR:
    case OP_CONTAINS:
    case OP_STARTSWITH:
    case OP_ENDSWITH:
    case OP_IN:
    {
        rule_value_t right = vm_pop(vm);
        rule_value_t left = vm_pop(vm);

        // ӳ��VM�����뵽token����
        token_type_t tok_type;
        switch (inst->opcode) {
        case OP_ADD: tok_type = TOK_PLUS; break;
        case OP_SUB: tok_type = TOK_MINUS; break;
        case OP_MUL: tok_type = TOK_MULTIPLY; break;
        case OP_DIV: tok_type = TOK_DIVIDE; break;
        case OP_MOD: tok_type = TOK_MODULO; break;
        case OP_EQ: tok_type = TOK_EQ; break;
        case OP_NE: tok_type = TOK_NE; break;
        case OP_LT: tok_type = TOK_LT; break;
        case OP_LE: tok_type = TOK_LE; break;
        case OP_GT: tok_type = TOK_GT; break;
        case OP_GE: tok_type = TOK_GE; break;
        case OP_AND: tok_type = TOK_AND; break;
        case OP_OR: tok_type = TOK_OR; break;
        case OP_CONTAINS: tok_type = TOK_CONTAINS; break;
        case OP_STARTSWITH: tok_type = TOK_STARTSWITH; break;
        case OP_ENDSWITH: tok_type = TOK_ENDSWITH; break;
        case OP_IN: tok_type = TOK_IN; break;
        default: tok_type = TOK_EOF; break;
        }

        rule_value_t result = evaluate_binary_op(tok_type, &left, &right);

        free_rule_value(&left);
        free_rule_value(&right);

        vm_push(vm, result);
    }
    break;

    case OP_NOT:
    case OP_NEG:
    {
        rule_value_t operand = vm_pop(vm);

        token_type_t tok_type = (inst->opcode == OP_NOT) ? TOK_NOT : TOK_MINUS;
        rule_value_t result = evaluate_unary_op(tok_type, &operand);

        free_rule_value(&operand);

        vm_push(vm, result);
    }
    break;

    case OP_ARRAY_BEGIN:
        // ���鿪ʼ��ǣ�Ŀǰ����Ҫ���⴦��
        break;

    case OP_ARRAY_END:
    {
        // ��ջ���ռ�����Ԫ��
        size_t count = inst->operand.integer;
        rule_value_t array_value = { 0 };
        array_value.type = RULE_TYPE_ARRAY;
        array_value.value.array.count = count;

        if (count > 0) {
            array_value.value.array.items = (rule_value_t*)malloc(sizeof(rule_value_t) * count);

            // ע�⣺Ԫ����ջ���������
            for (size_t i = count; i > 0; i--) {
                array_value.value.array.items[i - 1] = vm_pop(vm);
            }
        }

        vm_push(vm, array_value);
    }
    break;

    case OP_HALT:
        vm->running = false;
        break;

    default:
        vm->has_error = true;
        snprintf(vm->error_message, sizeof(vm->error_message),
            "Unknown opcode: 0x%02X", inst->opcode);
        vm->running = false;
        break;
    }
}

// ִ���ֽ���
bool vm_execute(vm_instance_t* vm, const event_context_t* context) {
    if (!vm || !vm->bytecode || !vm->bytecode->is_valid || !context) {
        return false;
    }

    // ����VM״̬
    vm->context = context;
    vm->running = true;
    vm->has_error = false;
    vm->error_message[0] = '\0';
    vm->frame->pc = 0;
    vm->frame->stack_size = 0;
    vm->instruction_count = 0;

    // ִ��ѭ��
    while (vm->running && vm->frame->pc < vm->bytecode->instruction_count) {
        vm_instruction_t* inst = &vm->bytecode->instructions[vm->frame->pc];
        vm->frame->pc++;
        vm->instruction_count++;

        vm_execute_instruction(vm, inst);

        if (vm->has_error) {
            return false;
        }
    }

    // ������ս��
    if (vm->frame->stack_size != 1) {
        vm->has_error = true;
        snprintf(vm->error_message, sizeof(vm->error_message),
            "Invalid stack size at end: %zu", vm->frame->stack_size);
        return false;
    }

    return true;
}

// ��ȡִ�н��
rule_value_t vm_get_result(vm_instance_t* vm) {
    if (!vm || vm->frame->stack_size == 0) {
        rule_value_t null_value = { 0 };
        null_value.type = RULE_TYPE_NULL;
        return null_value;
    }

    return copy_rule_value(&vm->frame->stack[0]);
}

// ==================== �������ʵ�� ====================

static const char* opcode_to_string(vm_opcode_t opcode) {
    switch (opcode) {
    case OP_NOP: return "NOP";
    case OP_PUSH_NUM: return "PUSH_NUM";
    case OP_PUSH_STR: return "PUSH_STR";
    case OP_PUSH_BOOL: return "PUSH_BOOL";
    case OP_PUSH_NULL: return "PUSH_NULL";
    case OP_PUSH_FIELD: return "PUSH_FIELD";
    case OP_POP: return "POP";
    case OP_DUP: return "DUP";
    case OP_SWAP: return "SWAP";
    case OP_ADD: return "ADD";
    case OP_SUB: return "SUB";
    case OP_MUL: return "MUL";
    case OP_DIV: return "DIV";
    case OP_MOD: return "MOD";
    case OP_NEG: return "NEG";
    case OP_EQ: return "EQ";
    case OP_NE: return "NE";
    case OP_LT: return "LT";
    case OP_LE: return "LE";
    case OP_GT: return "GT";
    case OP_GE: return "GE";
    case OP_AND: return "AND";
    case OP_OR: return "OR";
    case OP_NOT: return "NOT";
    case OP_CONTAINS: return "CONTAINS";
    case OP_STARTSWITH: return "STARTSWITH";
    case OP_ENDSWITH: return "ENDSWITH";
    case OP_REGEX: return "REGEX";
    case OP_ICONTAINS: return "ICONTAINS";
    case OP_IN: return "IN";
    case OP_PUSH_ARRAY: return "PUSH_ARRAY";
    case OP_ARRAY_BEGIN: return "ARRAY_BEGIN";
    case OP_ARRAY_END: return "ARRAY_END";
    case OP_JMP: return "JMP";
    case OP_JMP_TRUE: return "JMP_TRUE";
    case OP_JMP_FALSE: return "JMP_FALSE";
    case OP_CALL: return "CALL";
    case OP_RET: return "RET";
    case OP_HALT: return "HALT";
    default: return "UNKNOWN";
    }
}

// ������ֽ���
void disassemble_bytecode(const vm_bytecode_t* bytecode) {
    if (!bytecode) {
        printf("Bytecode is NULL\n");
        return;
    }

    printf("=== Bytecode Disassembly ===\n");

    if (bytecode->source) {
        printf("Source: %s\n", bytecode->source);
    }

    printf("Valid: %s\n", bytecode->is_valid ? "Yes" : "No");

    if (!bytecode->is_valid) {
        printf("Error: %s\n", bytecode->error_message);
        return;
    }

    // ��ӡ������
    printf("\n--- Constant Pool ---\n");
    for (size_t i = 0; i < bytecode->constant_count; i++) {
        printf("[%zu] ", i);
        switch (bytecode->constants[i].type) {
        case RULE_TYPE_NUMBER:
            printf("NUMBER: %.2f\n", bytecode->constants[i].value.number);
            break;
        case RULE_TYPE_STRING:
            printf("STRING: \"%s\"\n", bytecode->constants[i].value.string.data);
            break;
        case RULE_TYPE_BOOLEAN:
            printf("BOOLEAN: %s\n", bytecode->constants[i].value.boolean ? "true" : "false");
            break;
        default:
            printf("UNKNOWN TYPE\n");
            break;
        }
    }

    // ��ӡָ��
    printf("\n--- Instructions ---\n");
    for (size_t i = 0; i < bytecode->instruction_count; i++) {
        vm_instruction_t* inst = &bytecode->instructions[i];
        printf("%04zu: %-15s", i, opcode_to_string(inst->opcode));

        // ��ӡ������
        switch (inst->opcode) {
        case OP_PUSH_NUM:
        case OP_PUSH_STR:
            printf(" #%u", inst->operand.constant.index);
            break;

        case OP_PUSH_BOOL:
            printf(" %s", inst->operand.integer ? "true" : "false");
            break;

        case OP_PUSH_FIELD:
            printf(" field_id=%d", inst->operand.field_id);
            break;

        case OP_ARRAY_BEGIN:
        case OP_ARRAY_END:
            printf(" count=%lld", (long long)inst->operand.integer);
            break;

        case OP_JMP:
        case OP_JMP_TRUE:
        case OP_JMP_FALSE:
            printf(" offset=%u", inst->operand.offset);
            break;

        default:
            break;
        }

        printf("\n");
    }

    printf("=== End of Disassembly ===\n\n");
}

// ==================== �������ʵ�� ====================

// �����������
cached_rule_t* create_cached_rule(const char* rule_string) {
    cached_rule_t* rule = (cached_rule_t*)malloc(sizeof(cached_rule_t));

    rule->rule_string = _strdup(rule_string);
    rule->bytecode = compile_rule_to_bytecode(rule_string);
    rule->ast_rule = NULL; // �ӳٴ���

    return rule;
}

// ���ٻ������
void destroy_cached_rule(cached_rule_t* rule) {
    if (!rule) return;

    if (rule->rule_string) {
        free(rule->rule_string);
    }

    if (rule->bytecode) {
        destroy_bytecode(rule->bytecode);
    }

    if (rule->ast_rule) {
        destroy_compiled_rule(rule->ast_rule);
    }

    free(rule);
}

// ʹ�û�����������¼�
bool evaluate_cached_rule(const cached_rule_t* rule, const event_context_t* context) {
    if (!rule || !context) return false;

    // ����ʹ���ֽ���
    if (rule->bytecode && rule->bytecode->is_valid) {
        vm_instance_t* vm = create_vm_instance(rule->bytecode);
        bool result = false;

        if (vm_execute(vm, context)) {
            rule_value_t vm_result = vm_get_result(vm);

            if (vm_result.type == RULE_TYPE_BOOLEAN) {
                result = vm_result.value.boolean;
            }
            else if (vm_result.type == RULE_TYPE_NUMBER) {
                result = (vm_result.value.number != 0);
            }

            free_rule_value(&vm_result);
        }

        destroy_vm_instance(vm);
        return result;
    }

    // ���˵�AST����
    if (!rule->ast_rule) {
        ((cached_rule_t*)rule)->ast_rule = compile_rule(rule->rule_string);
    }

    if (rule->ast_rule && rule->ast_rule->is_valid) {
        return evaluate_rule(rule->ast_rule, context);
    }

    return false;
}

// ==================== ���л�ʵ�� ====================

// ħ���Ͱ汾
#define BYTECODE_MAGIC 0x52554C45 // "RULE"
#define BYTECODE_VERSION 1

// ���л��ֽ��뵽�����Ƹ�ʽ
uint8_t* serialize_bytecode(const vm_bytecode_t* bytecode, size_t* size) {
    if (!bytecode || !size) return NULL;

    // ������Ҫ�Ĵ�С
    *size = sizeof(uint32_t) * 2; // magic + version
    *size += sizeof(uint32_t) * 2; // instruction_count + constant_count
    *size += bytecode->instruction_count * sizeof(vm_instruction_t);

    // ���㳣���ش�С
    for (size_t i = 0; i < bytecode->constant_count; i++) {
        *size += sizeof(uint8_t); // type
        switch (bytecode->constants[i].type) {
        case RULE_TYPE_NUMBER:
            *size += sizeof(double);
            break;
        case RULE_TYPE_STRING:
            *size += sizeof(uint32_t); // length
            *size += bytecode->constants[i].value.string.length;
            break;
        case RULE_TYPE_BOOLEAN:
            *size += sizeof(uint8_t);
            break;
        default:
            break;
        }
    }

    // �����ڴ�
    uint8_t* buffer = (uint8_t*)malloc(*size);
    uint8_t* ptr = buffer;

    // д��ħ���Ͱ汾
    *(uint32_t*)ptr = BYTECODE_MAGIC;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = BYTECODE_VERSION;
    ptr += sizeof(uint32_t);

    // д�����
    *(uint32_t*)ptr = (uint32_t)bytecode->instruction_count;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = (uint32_t)bytecode->constant_count;
    ptr += sizeof(uint32_t);

    // д��ָ��
    memcpy(ptr, bytecode->instructions, bytecode->instruction_count * sizeof(vm_instruction_t));
    ptr += bytecode->instruction_count * sizeof(vm_instruction_t);

    // д�볣����
    for (size_t i = 0; i < bytecode->constant_count; i++) {
        *ptr++ = (uint8_t)bytecode->constants[i].type;

        switch (bytecode->constants[i].type) {
        case RULE_TYPE_NUMBER:
            *(double*)ptr = bytecode->constants[i].value.number;
            ptr += sizeof(double);
            break;

        case RULE_TYPE_STRING:
            *(uint32_t*)ptr = (uint32_t)bytecode->constants[i].value.string.length;
            ptr += sizeof(uint32_t);
            memcpy(ptr, bytecode->constants[i].value.string.data,
                bytecode->constants[i].value.string.length);
            ptr += bytecode->constants[i].value.string.length;
            break;

        case RULE_TYPE_BOOLEAN:
            *ptr++ = bytecode->constants[i].value.boolean ? 1 : 0;
            break;

        default:
            break;
        }
    }

    return buffer;
}

// �Ӷ����Ƹ�ʽ�����л��ֽ���
vm_bytecode_t* deserialize_bytecode(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(uint32_t) * 4) return NULL;

    const uint8_t* ptr = data;

    // ���ħ��
    if (*(uint32_t*)ptr != BYTECODE_MAGIC) {
        return NULL;
    }
    ptr += sizeof(uint32_t);

    // ���汾
    if (*(uint32_t*)ptr != BYTECODE_VERSION) {
        return NULL;
    }
    ptr += sizeof(uint32_t);

    // �����ֽ���ṹ
    vm_bytecode_t* bytecode = (vm_bytecode_t*)malloc(sizeof(vm_bytecode_t));
    memset(bytecode, 0, sizeof(vm_bytecode_t));
    bytecode->is_valid = true;

    // ��ȡ����
    bytecode->instruction_count = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    bytecode->constant_count = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);

    // ���䲢��ȡָ��
    bytecode->instruction_capacity = bytecode->instruction_count;
    bytecode->instructions = (vm_instruction_t*)malloc(
        sizeof(vm_instruction_t) * bytecode->instruction_capacity);
    memcpy(bytecode->instructions, ptr, bytecode->instruction_count * sizeof(vm_instruction_t));
    ptr += bytecode->instruction_count * sizeof(vm_instruction_t);

    // ���䲢��ȡ������
    bytecode->constant_capacity = bytecode->constant_count;
    bytecode->constants = (vm_constant_t*)malloc(
        sizeof(vm_constant_t) * bytecode->constant_capacity);

    for (size_t i = 0; i < bytecode->constant_count; i++) {
        bytecode->constants[i].type = (rule_data_type_t)*ptr++;

        switch (bytecode->constants[i].type) {
        case RULE_TYPE_NUMBER:
            bytecode->constants[i].value.number = *(double*)ptr;
            ptr += sizeof(double);
            break;

        case RULE_TYPE_STRING:
        {
            uint32_t length = *(uint32_t*)ptr;
            ptr += sizeof(uint32_t);
            bytecode->constants[i].value.string.length = length;
            bytecode->constants[i].value.string.data = (char*)malloc(length + 1);
            memcpy(bytecode->constants[i].value.string.data, ptr, length);
            bytecode->constants[i].value.string.data[length] = '\0';
            ptr += length;
        }
        break;

        case RULE_TYPE_BOOLEAN:
            bytecode->constants[i].value.boolean = (*ptr++ != 0);
            break;

        default:
            break;
        }
    }

    return bytecode;
}

// �����ֽ��뵽�ļ�
bool save_bytecode_to_file(const vm_bytecode_t* bytecode, const char* filename) {
    size_t size;
    uint8_t* data = serialize_bytecode(bytecode, &size);
    if (!data) return false;

    FILE* file = fopen(filename, "wb");
    if (!file) {
        free(data);
        return false;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    free(data);

    return written == size;
}

// ���ļ������ֽ���
vm_bytecode_t* load_bytecode_from_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    // ��ȡ�ļ���С
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ��ȡ�ļ�����
    uint8_t* data = (uint8_t*)malloc(size);
    size_t read = fread(data, 1, size, file);
    fclose(file);

    if (read != size) {
        free(data);
        return NULL;
    }

    // �����л�
    vm_bytecode_t* bytecode = deserialize_bytecode(data, size);
    free(data);

    return bytecode;
}