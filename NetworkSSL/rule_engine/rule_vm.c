/*
 * rule_vm.c - ���������ʵ��
 */

#include "rule_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    bytecode->source = strdup(rule_string);

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
        value.value.string.data = strdup(constant->value.string.data);
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
        // rule_value_t value = get_field_value(inst->operand.field_id, vm->context);
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
        }
    }
    }
}
