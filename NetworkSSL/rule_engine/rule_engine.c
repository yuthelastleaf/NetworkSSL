/*
 * rule_engine.c - �����������ʵ��
 */

#include "rule_engine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// ==================== �ڲ��ṹ���� ====================

// Token�ṹ
typedef struct token {
    token_type_t type;
    char* text;           // ԭʼ�ı�
    size_t length;        // �ı�����
    int line;             // �к�
    int column;           // �к�
    rule_value_t value;   // �������ֵ
} token_t;

// �ʷ�������״̬
typedef struct lexer_state {
    const char* source;        // Դ����
    const char* current;       // ��ǰλ��
    const char* start;         // ��ǰtoken��ʼλ��
    size_t length;             // Դ���볤��
    int line;                  // ��ǰ�к�
    int column;                // ��ǰ�к�

    // ������
    bool has_error;
    char error_message[256];

} lexer_state_t;

// �﷨������״̬
typedef struct parser_state {
    lexer_state_t* lexer;      // �ʷ�������
    token_t current_token;     // ��ǰtoken
    token_t previous_token;    // ǰһ��token

    // ������
    bool has_error;
    char error_message[256];
    bool panic_mode;           // �ֻ�ģʽ(����ָ�)

} parser_state_t;

// ==================== �ֶ�ӳ��� ====================

// �ֶ�ӳ���
typedef struct field_mapping {
    const char* object_name;
    const char* field_name;
    field_id_t field_id;
    rule_data_type_t field_type;
} field_mapping_t;

// ȫ���ֶ�ӳ���
static const field_mapping_t field_mappings[] = {
    {"process", "name",       FIELD_PROCESS_NAME,     RULE_TYPE_STRING},
    {"process", "pid",        FIELD_PROCESS_PID,      RULE_TYPE_NUMBER},
    {"process", "ppid",       FIELD_PROCESS_PPID,     RULE_TYPE_NUMBER},
    {"process", "cmdline",    FIELD_PROCESS_CMDLINE,  RULE_TYPE_STRING},
    {"process", "path",       FIELD_PROCESS_PATH,     RULE_TYPE_STRING},
    {"process", "user",       FIELD_PROCESS_USER,     RULE_TYPE_STRING},
    {"process", "session_id", FIELD_PROCESS_SESSION_ID, RULE_TYPE_NUMBER},

    {"file", "path",         FIELD_FILE_PATH,        RULE_TYPE_STRING},
    {"file", "name",         FIELD_FILE_NAME,        RULE_TYPE_STRING},
    {"file", "extension",    FIELD_FILE_EXTENSION,   RULE_TYPE_STRING},
    {"file", "size",         FIELD_FILE_SIZE,        RULE_TYPE_NUMBER},
    {"file", "operation",    FIELD_FILE_OPERATION,   RULE_TYPE_STRING},
    {"file", "attributes",   FIELD_FILE_ATTRIBUTES,  RULE_TYPE_NUMBER},

    {"registry", "key",          FIELD_REGISTRY_KEY,        RULE_TYPE_STRING},
    {"registry", "value_name",   FIELD_REGISTRY_VALUE_NAME, RULE_TYPE_STRING},
    {"registry", "value_type",   FIELD_REGISTRY_VALUE_TYPE, RULE_TYPE_NUMBER},
    {"registry", "value_data",   FIELD_REGISTRY_VALUE_DATA, RULE_TYPE_STRING},
    {"registry", "operation",    FIELD_REGISTRY_OPERATION,  RULE_TYPE_STRING},

    {"network", "src_ip",    FIELD_NETWORK_SRC_IP,   RULE_TYPE_STRING},
    {"network", "dst_ip",    FIELD_NETWORK_DST_IP,   RULE_TYPE_STRING},
    {"network", "src_port",  FIELD_NETWORK_SRC_PORT, RULE_TYPE_NUMBER},
    {"network", "dst_port",  FIELD_NETWORK_DST_PORT, RULE_TYPE_NUMBER},
    {"network", "protocol",  FIELD_NETWORK_PROTOCOL, RULE_TYPE_STRING},

    {NULL, NULL, FIELD_UNKNOWN, RULE_TYPE_UNKNOWN}  // �������
};

// ==================== ǰ������ ====================

// �ʷ�������
static lexer_state_t* create_lexer(const char* source);
static void destroy_lexer(lexer_state_t* lexer);
static token_t next_token(lexer_state_t* lexer);
static void skip_whitespace(lexer_state_t* lexer);

// �﷨������
static parser_state_t* create_parser(const char* source);
static void destroy_parser(parser_state_t* parser);
static ast_node_t* parse_expression(parser_state_t* parser);

// AST����
static ast_node_t* create_ast_node(ast_node_type_t type, int line, int column);
static void destroy_ast_node(ast_node_t* node);

// ��������
static rule_value_t evaluate_ast_node(const ast_node_t* node, const event_context_t* context);

// ==================== ���ߺ��� ====================

// �����ַ���
static char* ll_strndup(const char* str_src, size_t size) {
    char* str_dst = (char*)malloc((size + 1) * sizeof(char));
    if (str_dst) {
        memset(str_dst, 0, (size + 1) * sizeof(char));
        memcpy(str_dst, str_src, size);
    }
    return str_dst;
}

// ==================== �ʷ�������ʵ�� ====================

static lexer_state_t* create_lexer(const char* source) {
    lexer_state_t* lexer = (lexer_state_t*)malloc(sizeof(lexer_state_t));
    lexer->source = source;
    lexer->current = source;
    lexer->start = source;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_error = false;
    lexer->error_message[0] = '\0';
    return lexer;
}

static void destroy_lexer(lexer_state_t* lexer) {
    if (lexer) {
        free(lexer);
    }
}

// �����հ��ַ�
static void skip_whitespace(lexer_state_t* lexer) {
    while (*lexer->current) {
        char c = *lexer->current;
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->current++;
            lexer->column++;
        }
        else if (c == '\n') {
            lexer->current++;
            lexer->line++;
            lexer->column = 1;
        }
        else if (c == '/' && lexer->current[1] == '/') {
            // ��������ע��
            while (*lexer->current && *lexer->current != '\n') {
                lexer->current++;
            }
        }
        else {
            break;
        }
    }
}

// �ж��Ƿ񵽴�Դ��ĩβ
static bool is_at_end(lexer_state_t* lexer) {
    return *lexer->current == '\0';
}

// ǰ��һ���ַ�
static char advance_char(lexer_state_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    char c = *lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    }
    else {
        lexer->column++;
    }
    return c;
}

// �鿴��ǰ�ַ�
static char peek_char(lexer_state_t* lexer) {
    return *lexer->current;
}

// �鿴��һ���ַ�
static char peek_next_char(lexer_state_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

// ƥ���ַ�
static bool match_char(lexer_state_t* lexer, char expected) {
    if (is_at_end(lexer) || *lexer->current != expected) {
        return false;
    }
    lexer->current++;
    lexer->column++;
    return true;
}

// ����token
static token_t make_token(lexer_state_t* lexer, token_type_t type) {
    token_t token;
    token.type = type;
    token.text = (char*)lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    token.column = lexer->column - (int)token.length;
    token.value.type = RULE_TYPE_UNKNOWN;
    return token;
}

// ��������token
static token_t make_error_token(lexer_state_t* lexer, const char* message) {
    lexer->has_error = true;
    strncpy(lexer->error_message, message, sizeof(lexer->error_message) - 1);

    token_t token;
    token.type = TOK_ERROR;
    token.text = (char*)message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    token.value.type = RULE_TYPE_UNKNOWN;
    return token;
}

// �����ַ���������
static token_t parse_string(lexer_state_t* lexer) {
    char quote = lexer->current[-1]; // ��ʼ������

    while (!is_at_end(lexer) && peek_char(lexer) != quote) {
        if (peek_char(lexer) == '\\') {
            advance_char(lexer); // ����ת���ַ�
            if (!is_at_end(lexer)) {
                advance_char(lexer); // ������ת����ַ�
            }
        }
        else {
            advance_char(lexer);
        }
    }

    if (is_at_end(lexer)) {
        return make_error_token(lexer, "Unterminated string");
    }

    // ���ѽ�������
    advance_char(lexer);

    // �����ַ���token
    token_t token = make_token(lexer, TOK_STRING);

    // �����ַ���ֵ (ȥ������)
    size_t string_length = token.length - 2;
    char* string_value = (char*)malloc(string_length + 1);
    strncpy(string_value, token.text + 1, string_length);
    string_value[string_length] = '\0';

    token.value.type = RULE_TYPE_STRING;
    token.value.value.string.data = string_value;
    token.value.value.string.length = string_length;

    return token;
}

// ��������������
static token_t parse_number(lexer_state_t* lexer) {
    while (isdigit(peek_char(lexer))) {
        advance_char(lexer);
    }

    // ����С����
    if (peek_char(lexer) == '.' && isdigit(peek_next_char(lexer))) {
        advance_char(lexer); // ����С����
        while (isdigit(peek_char(lexer))) {
            advance_char(lexer);
        }
    }

    token_t token = make_token(lexer, TOK_NUMBER);

    // ��������ֵ
    char* end_ptr;
    double number_value = strtod(token.text, &end_ptr);

    token.value.type = RULE_TYPE_NUMBER;
    token.value.value.number = number_value;

    return token;
}

// ������ʶ����ؼ���
static token_t parse_identifier(lexer_state_t* lexer) {
    while (isalnum(peek_char(lexer)) || peek_char(lexer) == '_') {
        advance_char(lexer);
    }

    // ����Ƿ����ֶ����� (identifier.identifier)
    if (peek_char(lexer) == '.') {
        const char* object_start = lexer->start;
        size_t object_length = lexer->current - lexer->start;

        advance_char(lexer); // ���� '.'

        if (!isalpha(peek_char(lexer))) {
            return make_error_token(lexer, "Expected field name after '.'");
        }

        const char* field_start = lexer->current;
        while (isalnum(peek_char(lexer)) || peek_char(lexer) == '_') {
            advance_char(lexer);
        }

        token_t token = make_token(lexer, TOK_FIELD_REF);

        // �����ֶ�����ֵ
        size_t field_length = lexer->current - field_start;

        char* object_name = (char*)malloc(object_length + 1);
        char* field_name = (char*)malloc(field_length + 1);

        strncpy(object_name, object_start, object_length);
        object_name[object_length] = '\0';

        strncpy(field_name, field_start, field_length);
        field_name[field_length] = '\0';

        token.value.type = RULE_TYPE_FIELD_REF;
        token.value.value.field_ref.object_name = object_name;
        token.value.value.field_ref.field_name = field_name;
        token.value.value.field_ref.field_id = resolve_field_id(object_name, field_name);

        return token;
    }

    // ���ؼ���
    size_t length = lexer->current - lexer->start;
    if (length == 4 && strncmp(lexer->start, "true", 4) == 0) {
        token_t token = make_token(lexer, TOK_BOOLEAN);
        token.value.type = RULE_TYPE_BOOLEAN;
        token.value.value.boolean = true;
        return token;
    }

    if (length == 5 && strncmp(lexer->start, "false", 5) == 0) {
        token_t token = make_token(lexer, TOK_BOOLEAN);
        token.value.type = RULE_TYPE_BOOLEAN;
        token.value.value.boolean = false;
        return token;
    }

    if (length == 8 && strncmp(lexer->start, "contains", 8) == 0) {
        return make_token(lexer, TOK_CONTAINS);
    }

    if (length == 10 && strncmp(lexer->start, "startswith", 10) == 0) {
        return make_token(lexer, TOK_STARTSWITH);
    }

    if (length == 8 && strncmp(lexer->start, "endswith", 8) == 0) {
        return make_token(lexer, TOK_ENDSWITH);
    }

    if (length == 5 && strncmp(lexer->start, "regex", 5) == 0) {
        return make_token(lexer, TOK_REGEX);
    }

    if (length == 2 && strncmp(lexer->start, "in", 2) == 0) {
        return make_token(lexer, TOK_IN);
    }

    // ��ͨ��ʶ��
    return make_token(lexer, TOK_IDENTIFIER);
}

// ��Ҫ�Ĵʷ���������
static token_t next_token(lexer_state_t* lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOK_EOF);
    }

    char c = advance_char(lexer);

    // ����
    if (isdigit(c)) {
        return parse_number(lexer);
    }

    // ��ʶ���͹ؼ���
    if (isalpha(c) || c == '_') {
        return parse_identifier(lexer);
    }

    // �ַ���
    if (c == '"' || c == '\'') {
        return parse_string(lexer);
    }

    // ˫�ַ������
    switch (c) {
    case '=':
        if (match_char(lexer, '=')) return make_token(lexer, TOK_EQ);
        break;
    case '!':
        if (match_char(lexer, '=')) return make_token(lexer, TOK_NE);
        return make_token(lexer, TOK_NOT);
    case '<':
        if (match_char(lexer, '=')) return make_token(lexer, TOK_LE);
        return make_token(lexer, TOK_LT);
    case '>':
        if (match_char(lexer, '=')) return make_token(lexer, TOK_GE);
        return make_token(lexer, TOK_GT);
    case '&':
        if (match_char(lexer, '&')) return make_token(lexer, TOK_AND);
        break;
    case '|':
        if (match_char(lexer, '|')) return make_token(lexer, TOK_OR);
        break;
    }

    // ���ַ�token
    switch (c) {
    case '+': return make_token(lexer, TOK_PLUS);
    case '-': return make_token(lexer, TOK_MINUS);
    case '*': return make_token(lexer, TOK_MULTIPLY);
    case '/': return make_token(lexer, TOK_DIVIDE);
    case '%': return make_token(lexer, TOK_MODULO);
    case '(': return make_token(lexer, TOK_LPAREN);
    case ')': return make_token(lexer, TOK_RPAREN);
    case '[': return make_token(lexer, TOK_LBRACKET);
    case ']': return make_token(lexer, TOK_RBRACKET);
    case ',': return make_token(lexer, TOK_COMMA);
    case '.': return make_token(lexer, TOK_DOT);
    case ';': return make_token(lexer, TOK_SEMICOLON);
    }

    // δ֪�ַ�
    char error_msg[64];
    snprintf(error_msg, sizeof(error_msg), "Unexpected character: '%c'", c);
    return make_error_token(lexer, error_msg);
}

// ==================== �﷨������ʵ�� ====================

static parser_state_t* create_parser(const char* source) {
    parser_state_t* parser = (parser_state_t*)malloc(sizeof(parser_state_t));
    parser->lexer = create_lexer(source);
    parser->has_error = false;
    parser->panic_mode = false;
    parser->error_message[0] = '\0';

    // ��ȡ��һ��token
    parser->current_token = next_token(parser->lexer);
    parser->previous_token.type = TOK_EOF;

    return parser;
}

static void destroy_parser(parser_state_t* parser) {
    if (parser) {
        destroy_lexer(parser->lexer);
        free(parser);
    }
}

static void advance_token(parser_state_t* parser) {
    parser->previous_token = parser->current_token;
    parser->current_token = next_token(parser->lexer);

    if (parser->current_token.type == TOK_ERROR) {
        parser->has_error = true;
        strncpy(parser->error_message, parser->current_token.text,
            sizeof(parser->error_message) - 1);
    }
}

static bool check_token(parser_state_t* parser, token_type_t type) {
    return parser->current_token.type == type;
}

static bool match_token(parser_state_t* parser, token_type_t type) {
    if (!check_token(parser, type)) return false;
    advance_token(parser);
    return true;
}

// ǰ������
static ast_node_t* parse_primary(parser_state_t* parser);
static ast_node_t* parse_unary(parser_state_t* parser);
static ast_node_t* parse_factor(parser_state_t* parser);
static ast_node_t* parse_term(parser_state_t* parser);
static ast_node_t* parse_comparison(parser_state_t* parser);
static ast_node_t* parse_equality(parser_state_t* parser);
static ast_node_t* parse_logical_and(parser_state_t* parser);
static ast_node_t* parse_logical_or(parser_state_t* parser);

// ����AST�ڵ�
static ast_node_t* create_ast_node(ast_node_type_t type, int line, int column) {
    ast_node_t* node = (ast_node_t*)malloc(sizeof(ast_node_t));
    node->type = type;
    node->line = line;
    node->column = column;
    node->value_type = RULE_TYPE_UNKNOWN;
    return node;
}

// ����AST�ڵ�
static void destroy_ast_node(ast_node_t* node) {
    if (!node) return;

    switch (node->type) {
    case AST_NUMBER:
    case AST_BOOLEAN:
        // ��Щ���Ͳ���Ҫ�ͷŶ����ڴ�
        break;

    case AST_STRING:
        if (node->data.literal.value.string.data) {
            free(node->data.literal.value.string.data);
        }
        break;

    case AST_FIELD_REF:
        if (node->data.literal.value.field_ref.object_name) {
            free(node->data.literal.value.field_ref.object_name);
        }
        if (node->data.literal.value.field_ref.field_name) {
            free(node->data.literal.value.field_ref.field_name);
        }
        break;

    case AST_BINARY_OP:
        destroy_ast_node(node->data.binary_op.left);
        destroy_ast_node(node->data.binary_op.right);
        break;

    case AST_UNARY_OP:
        destroy_ast_node(node->data.unary_op.operand);
        break;

    case AST_FUNCTION_CALL:
        if (node->data.function_call.function_name) {
            free(node->data.function_call.function_name);
        }
        for (size_t i = 0; i < node->data.function_call.argument_count; i++) {
            destroy_ast_node(node->data.function_call.arguments[i]);
        }
        if (node->data.function_call.arguments) {
            free(node->data.function_call.arguments);
        }
        break;

    case AST_ARRAY:
        for (size_t i = 0; i < node->data.array.element_count; i++) {
            destroy_ast_node(node->data.array.elements[i]);
        }
        if (node->data.array.elements) {
            free(node->data.array.elements);
        }
        break;

    default:
        break;
    }

    free(node);
}

// ��������������
static ast_node_t* parse_array(parser_state_t* parser) {
    ast_node_t* node = create_ast_node(AST_ARRAY, parser->current_token.line, parser->current_token.column);

    advance_token(parser); // ���� '['

    if (check_token(parser, TOK_RBRACKET)) {
        // ������
        node->data.array.elements = NULL;
        node->data.array.element_count = 0;
        advance_token(parser);
        return node;
    }

    // ��̬��������Ԫ��
    size_t capacity = 4;
    ast_node_t** elements = (ast_node_t**)malloc(sizeof(ast_node_t*) * capacity);
    size_t count = 0;

    do {
        if (count >= capacity) {
            capacity *= 2;
            elements = (ast_node_t**)realloc(elements, sizeof(ast_node_t*) * capacity);
        }

        elements[count++] = parse_expression(parser);

    } while (match_token(parser, TOK_COMMA));

    if (!match_token(parser, TOK_RBRACKET)) {
        parser->has_error = true;
        strcpy(parser->error_message, "Expected ']' after array elements");
        for (size_t i = 0; i < count; i++) {
            destroy_ast_node(elements[i]);
        }
        free(elements);
        free(node);
        return NULL;
    }

    node->data.array.elements = elements;
    node->data.array.element_count = count;
    node->value_type = RULE_TYPE_ARRAY;

    return node;
}

// ������������
static ast_node_t* parse_function_call(parser_state_t* parser, const char* function_name) {
    ast_node_t* node = create_ast_node(AST_FUNCTION_CALL, parser->current_token.line, parser->current_token.column);

    // ���ƺ�����
    size_t name_len = strlen(function_name);
    node->data.function_call.function_name = (char*)malloc(name_len + 1);
    strcpy(node->data.function_call.function_name, function_name);

    advance_token(parser); // ���� '('

    if (check_token(parser, TOK_RPAREN)) {
        // �޲�������
        node->data.function_call.arguments = NULL;
        node->data.function_call.argument_count = 0;
        advance_token(parser);
        return node;
    }

    // ��������
    size_t capacity = 4;
    ast_node_t** arguments = (ast_node_t**)malloc(sizeof(ast_node_t*) * capacity);
    size_t count = 0;

    do {
        if (count >= capacity) {
            capacity *= 2;
            arguments = (ast_node_t**)realloc(arguments, sizeof(ast_node_t*) * capacity);
        }

        arguments[count++] = parse_expression(parser);

    } while (match_token(parser, TOK_COMMA));

    if (!match_token(parser, TOK_RPAREN)) {
        parser->has_error = true;
        strcpy(parser->error_message, "Expected ')' after function arguments");
        for (size_t i = 0; i < count; i++) {
            destroy_ast_node(arguments[i]);
        }
        free(arguments);
        free(node->data.function_call.function_name);
        free(node);
        return NULL;
    }

    node->data.function_call.arguments = arguments;
    node->data.function_call.argument_count = count;

    return node;
}

// ���������ʽ
static ast_node_t* parse_primary(parser_state_t* parser) {
    // ����������
    if (check_token(parser, TOK_NUMBER)) {
        ast_node_t* node = create_ast_node(AST_NUMBER, parser->current_token.line, parser->current_token.column);
        node->data.literal = parser->current_token.value;
        node->value_type = RULE_TYPE_NUMBER;
        advance_token(parser);
        return node;
    }

    // �ַ���������
    if (check_token(parser, TOK_STRING)) {
        ast_node_t* node = create_ast_node(AST_STRING, parser->current_token.line, parser->current_token.column);
        node->data.literal = parser->current_token.value;
        node->value_type = RULE_TYPE_STRING;
        advance_token(parser);
        return node;
    }

    // ����������
    if (check_token(parser, TOK_BOOLEAN)) {
        ast_node_t* node = create_ast_node(AST_BOOLEAN, parser->current_token.line, parser->current_token.column);
        node->data.literal = parser->current_token.value;
        node->value_type = RULE_TYPE_BOOLEAN;
        advance_token(parser);
        return node;
    }

    // �ֶ�����
    if (check_token(parser, TOK_FIELD_REF)) {
        ast_node_t* node = create_ast_node(AST_FIELD_REF, parser->current_token.line, parser->current_token.column);
        node->data.literal = parser->current_token.value;
        // ���ֶ�ӳ����ȡ�ֶ�����
        field_id_t field_id = parser->current_token.value.value.field_ref.field_id;
        node->value_type = get_field_type(field_id);
        advance_token(parser);
        return node;
    }

    // ����������
    if (check_token(parser, TOK_LBRACKET)) {
        return parse_array(parser);
    }

    // ��ʶ�� (�����Ǻ�������)
    if (check_token(parser, TOK_IDENTIFIER)) {
        char* identifier = ll_strndup(parser->current_token.text, parser->current_token.length);
        advance_token(parser);

        // ����Ƿ��Ǻ�������
        if (check_token(parser, TOK_LPAREN)) {
            ast_node_t* node = parse_function_call(parser, identifier);
            free(identifier);
            return node;
        }

        // ��ͨ��ʶ�� (��ʱ��Ϊ������)
        parser->has_error = true;
        snprintf(parser->error_message, sizeof(parser->error_message),
            "Unexpected identifier: %s", identifier);
        free(identifier);
        return NULL;
    }

    // ���ű��ʽ
    if (match_token(parser, TOK_LPAREN)) {
        ast_node_t* expr = parse_expression(parser);
        if (!match_token(parser, TOK_RPAREN)) {
            parser->has_error = true;
            strcpy(parser->error_message, "Expected ')' after expression");
            destroy_ast_node(expr);
            return NULL;
        }
        return expr;
    }

    parser->has_error = true;
    strcpy(parser->error_message, "Expected expression");
    return NULL;
}

// ����һԪ���ʽ
static ast_node_t* parse_unary(parser_state_t* parser) {
    if (match_token(parser, TOK_NOT) || match_token(parser, TOK_MINUS)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* node = create_ast_node(AST_UNARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.unary_op.tok_opt = tok_opt;
        node->data.unary_op.operand = parse_unary(parser);

        // �Ƶ�����
        if (tok_opt == TOK_NOT) {
            node->value_type = RULE_TYPE_BOOLEAN;
        }
        else if (tok_opt == TOK_MINUS) {
            node->value_type = RULE_TYPE_NUMBER;
        }

        return node;
    }

    return parse_primary(parser);
}

// �����˳������ʽ
static ast_node_t* parse_factor(parser_state_t* parser) {
    ast_node_t* expr = parse_unary(parser);

    while (match_token(parser, TOK_MULTIPLY) || match_token(parser, TOK_DIVIDE) || match_token(parser, TOK_MODULO)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_unary(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_NUMBER;

        expr = node;
    }

    return expr;
}

// �����Ӽ������ʽ
static ast_node_t* parse_term(parser_state_t* parser) {
    ast_node_t* expr = parse_factor(parser);

    while (match_token(parser, TOK_PLUS) || match_token(parser, TOK_MINUS)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_factor(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_NUMBER;

        expr = node;
    }

    return expr;
}

// �����Ƚϱ��ʽ
static ast_node_t* parse_comparison(parser_state_t* parser) {
    ast_node_t* expr = parse_term(parser);

    while (match_token(parser, TOK_GT) || match_token(parser, TOK_GE) ||
        match_token(parser, TOK_LT) || match_token(parser, TOK_LE)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_term(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_BOOLEAN;

        expr = node;
    }

    // �����ַ���������in�����
    if (match_token(parser, TOK_CONTAINS) || match_token(parser, TOK_STARTSWITH) ||
        match_token(parser, TOK_ENDSWITH) || match_token(parser, TOK_REGEX) ||
        match_token(parser, TOK_IN)) {

        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_term(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_BOOLEAN;

        expr = node;
    }

    return expr;
}

// ��������Ա��ʽ
static ast_node_t* parse_equality(parser_state_t* parser) {
    ast_node_t* expr = parse_comparison(parser);

    while (match_token(parser, TOK_EQ) || match_token(parser, TOK_NE)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_comparison(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_BOOLEAN;

        expr = node;
    }

    return expr;
}

// �����߼�����ʽ
static ast_node_t* parse_logical_and(parser_state_t* parser) {
    ast_node_t* expr = parse_equality(parser);

    while (match_token(parser, TOK_AND)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_equality(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_BOOLEAN;

        expr = node;
    }

    return expr;
}

// �����߼�����ʽ
static ast_node_t* parse_logical_or(parser_state_t* parser) {
    ast_node_t* expr = parse_logical_and(parser);

    while (match_token(parser, TOK_OR)) {
        token_type_t tok_opt = parser->previous_token.type;
        ast_node_t* right = parse_logical_and(parser);

        ast_node_t* node = create_ast_node(AST_BINARY_OP, parser->previous_token.line, parser->previous_token.column);
        node->data.binary_op.tok_opt = tok_opt;
        node->data.binary_op.left = expr;
        node->data.binary_op.right = right;
        node->value_type = RULE_TYPE_BOOLEAN;

        expr = node;
    }

    return expr;
}

// �������ʽ (�������)
static ast_node_t* parse_expression(parser_state_t* parser) {
    return parse_logical_or(parser);
}

// ==================== �ֶ�ID����ʵ�� ====================

field_id_t resolve_field_id(const char* object_name, const char* field_name) {
    for (int i = 0; field_mappings[i].object_name != NULL; i++) {
        if (strcmp(field_mappings[i].object_name, object_name) == 0 &&
            strcmp(field_mappings[i].field_name, field_name) == 0) {
            return field_mappings[i].field_id;
        }
    }
    return FIELD_UNKNOWN;
}

rule_data_type_t get_field_type(field_id_t field_id) {
    for (int i = 0; field_mappings[i].object_name != NULL; i++) {
        if (field_mappings[i].field_id == field_id) {
            return field_mappings[i].field_type;
        }
    }
    return RULE_TYPE_UNKNOWN;
}

// ==================== ����ֵ���� ====================

void free_rule_value(rule_value_t* value) {
    if (!value) return;

    switch (value->type) {
    case RULE_TYPE_STRING:
        if (value->value.string.data) {
            free(value->value.string.data);
            value->value.string.data = NULL;
        }
        break;

    case RULE_TYPE_FIELD_REF:
        if (value->value.field_ref.object_name) {
            free(value->value.field_ref.object_name);
            value->value.field_ref.object_name = NULL;
        }
        if (value->value.field_ref.field_name) {
            free(value->value.field_ref.field_name);
            value->value.field_ref.field_name = NULL;
        }
        break;

    case RULE_TYPE_ARRAY:
        if (value->value.array.items) {
            for (size_t i = 0; i < value->value.array.count; i++) {
                free_rule_value(&value->value.array.items[i]);
            }
            free(value->value.array.items);
            value->value.array.items = NULL;
        }
        break;

    default:
        break;
    }

    value->type = RULE_TYPE_UNKNOWN;
}

rule_value_t copy_rule_value(const rule_value_t* src) {
    rule_value_t dest = { 0 };
    dest.type = src->type;

    switch (src->type) {
    case RULE_TYPE_NUMBER:
        dest.value.number = src->value.number;
        break;

    case RULE_TYPE_STRING:
        if (src->value.string.data) {
            dest.value.string.length = src->value.string.length;
            dest.value.string.data = (char*)malloc(dest.value.string.length + 1);
            memcpy(dest.value.string.data, src->value.string.data, dest.value.string.length + 1);
        }
        break;

    case RULE_TYPE_BOOLEAN:
        dest.value.boolean = src->value.boolean;
        break;

    case RULE_TYPE_FIELD_REF:
        if (src->value.field_ref.object_name) {
            dest.value.field_ref.object_name = _strdup(src->value.field_ref.object_name);
        }
        if (src->value.field_ref.field_name) {
            dest.value.field_ref.field_name = _strdup(src->value.field_ref.field_name);
        }
        dest.value.field_ref.field_id = src->value.field_ref.field_id;
        break;

    case RULE_TYPE_ARRAY:
        if (src->value.array.count > 0 && src->value.array.items) {
            dest.value.array.count = src->value.array.count;
            dest.value.array.items = (rule_value_t*)malloc(sizeof(rule_value_t) * dest.value.array.count);
            for (size_t i = 0; i < dest.value.array.count; i++) {
                dest.value.array.items[i] = copy_rule_value(&src->value.array.items[i]);
            }
        }
        break;

    default:
        break;
    }

    return dest;
}

// ==================== ��������ʵ�� ====================

// ���¼������Ļ�ȡ�ֶ�ֵ
rule_value_t get_field_value(field_id_t field_id, const event_context_t* context) {
    rule_value_t value = { 0 };

    switch (field_id) {
        // �����ֶ�
    case FIELD_PROCESS_NAME:
        value.type = RULE_TYPE_STRING;
        if (context->process.name) {
            value.value.string.data = _strdup(context->process.name);
            value.value.string.length = strlen(context->process.name);
        }
        break;

    case FIELD_PROCESS_PID:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->process.pid;
        break;

    case FIELD_PROCESS_PPID:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->process.ppid;
        break;

    case FIELD_PROCESS_CMDLINE:
        value.type = RULE_TYPE_STRING;
        if (context->process.cmdline) {
            value.value.string.data = _strdup(context->process.cmdline);
            value.value.string.length = strlen(context->process.cmdline);
        }
        break;

    case FIELD_PROCESS_PATH:
        value.type = RULE_TYPE_STRING;
        if (context->process.path) {
            value.value.string.data = _strdup(context->process.path);
            value.value.string.length = strlen(context->process.path);
        }
        break;

    case FIELD_PROCESS_USER:
        value.type = RULE_TYPE_STRING;
        if (context->process.user) {
            value.value.string.data = _strdup(context->process.user);
            value.value.string.length = strlen(context->process.user);
        }
        break;

    case FIELD_PROCESS_SESSION_ID:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->process.session_id;
        break;

        // �ļ��ֶ�
    case FIELD_FILE_PATH:
        value.type = RULE_TYPE_STRING;
        if (context->file.path) {
            value.value.string.data = _strdup(context->file.path);
            value.value.string.length = strlen(context->file.path);
        }
        break;

    case FIELD_FILE_NAME:
        value.type = RULE_TYPE_STRING;
        if (context->file.name) {
            value.value.string.data = _strdup(context->file.name);
            value.value.string.length = strlen(context->file.name);
        }
        break;

    case FIELD_FILE_EXTENSION:
        value.type = RULE_TYPE_STRING;
        if (context->file.extension) {
            value.value.string.data = _strdup(context->file.extension);
            value.value.string.length = strlen(context->file.extension);
        }
        break;

    case FIELD_FILE_SIZE:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = (double)context->file.size;
        break;

    case FIELD_FILE_OPERATION:
        value.type = RULE_TYPE_STRING;
        if (context->file.operation) {
            value.value.string.data = _strdup(context->file.operation);
            value.value.string.length = strlen(context->file.operation);
        }
        break;

    case FIELD_FILE_ATTRIBUTES:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->file.attributes;
        break;

        // ע����ֶ�
    case FIELD_REGISTRY_KEY:
        value.type = RULE_TYPE_STRING;
        if (context->registry.key) {
            value.value.string.data = _strdup(context->registry.key);
            value.value.string.length = strlen(context->registry.key);
        }
        break;

    case FIELD_REGISTRY_VALUE_NAME:
        value.type = RULE_TYPE_STRING;
        if (context->registry.value_name) {
            value.value.string.data = _strdup(context->registry.value_name);
            value.value.string.length = strlen(context->registry.value_name);
        }
        break;

    case FIELD_REGISTRY_VALUE_TYPE:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->registry.value_type;
        break;

    case FIELD_REGISTRY_VALUE_DATA:
        value.type = RULE_TYPE_STRING;
        if (context->registry.value_data) {
            value.value.string.data = _strdup(context->registry.value_data);
            value.value.string.length = strlen(context->registry.value_data);
        }
        break;

    case FIELD_REGISTRY_OPERATION:
        value.type = RULE_TYPE_STRING;
        if (context->registry.operation) {
            value.value.string.data = _strdup(context->registry.operation);
            value.value.string.length = strlen(context->registry.operation);
        }
        break;

        // �����ֶ�
    case FIELD_NETWORK_SRC_IP:
        value.type = RULE_TYPE_STRING;
        if (context->network.src_ip) {
            value.value.string.data = _strdup(context->network.src_ip);
            value.value.string.length = strlen(context->network.src_ip);
        }
        break;

    case FIELD_NETWORK_DST_IP:
        value.type = RULE_TYPE_STRING;
        if (context->network.dst_ip) {
            value.value.string.data = _strdup(context->network.dst_ip);
            value.value.string.length = strlen(context->network.dst_ip);
        }
        break;

    case FIELD_NETWORK_SRC_PORT:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->network.src_port;
        break;

    case FIELD_NETWORK_DST_PORT:
        value.type = RULE_TYPE_NUMBER;
        value.value.number = context->network.dst_port;
        break;

    case FIELD_NETWORK_PROTOCOL:
        value.type = RULE_TYPE_STRING;
        if (context->network.protocol) {
            value.value.string.data = _strdup(context->network.protocol);
            value.value.string.length = strlen(context->network.protocol);
        }
        break;

    default:
        value.type = RULE_TYPE_NULL;
        break;
    }

    return value;
}

// �ַ����������
static bool string_contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    return strstr(haystack, needle) != NULL;
}

// �ַ�����ʼ���
static bool string_startswith(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

// �ַ����������
static bool string_endswith(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// ������Ԫ�����
// ͬ���������ȥ��static����
rule_value_t evaluate_binary_op(token_type_t op, const rule_value_t* left, const rule_value_t* right) {
    rule_value_t result = { 0 };

    // ���������
    if (op >= TOK_PLUS && op <= TOK_MODULO) {
        if (left->type != RULE_TYPE_NUMBER || right->type != RULE_TYPE_NUMBER) {
            result.type = RULE_TYPE_NULL;
            return result;
        }

        result.type = RULE_TYPE_NUMBER;
        switch (op) {
        case TOK_PLUS:
            result.value.number = left->value.number + right->value.number;
            break;
        case TOK_MINUS:
            result.value.number = left->value.number - right->value.number;
            break;
        case TOK_MULTIPLY:
            result.value.number = left->value.number * right->value.number;
            break;
        case TOK_DIVIDE:
            if (right->value.number != 0) {
                result.value.number = left->value.number / right->value.number;
            }
            else {
                result.type = RULE_TYPE_NULL;
            }
            break;
        case TOK_MODULO:
            if (right->value.number != 0) {
                result.value.number = (double)((int)left->value.number % (int)right->value.number);
            }
            else {
                result.type = RULE_TYPE_NULL;
            }
            break;
        default:
            break;
        }
        return result;
    }

    // �Ƚ������
    if (op >= TOK_EQ && op <= TOK_GE) {
        result.type = RULE_TYPE_BOOLEAN;
        result.value.boolean = false;

        // ����ԱȽ�
        if (op == TOK_EQ || op == TOK_NE) {
            bool equal = false;

            if (left->type == RULE_TYPE_NUMBER && right->type == RULE_TYPE_NUMBER) {
                equal = (left->value.number == right->value.number);
            }
            else if (left->type == RULE_TYPE_STRING && right->type == RULE_TYPE_STRING) {
                equal = (strcmp(left->value.string.data, right->value.string.data) == 0);
            }
            else if (left->type == RULE_TYPE_BOOLEAN && right->type == RULE_TYPE_BOOLEAN) {
                equal = (left->value.boolean == right->value.boolean);
            }

            result.value.boolean = (op == TOK_EQ) ? equal : !equal;
            return result;
        }

        // ��С�Ƚ�
        if (left->type == RULE_TYPE_NUMBER && right->type == RULE_TYPE_NUMBER) {
            switch (op) {
            case TOK_LT:
                result.value.boolean = left->value.number < right->value.number;
                break;
            case TOK_LE:
                result.value.boolean = left->value.number <= right->value.number;
                break;
            case TOK_GT:
                result.value.boolean = left->value.number > right->value.number;
                break;
            case TOK_GE:
                result.value.boolean = left->value.number >= right->value.number;
                break;
            default:
                break;
            }
        }
        else if (left->type == RULE_TYPE_STRING && right->type == RULE_TYPE_STRING) {
            int cmp = strcmp(left->value.string.data, right->value.string.data);
            switch (op) {
            case TOK_LT:
                result.value.boolean = cmp < 0;
                break;
            case TOK_LE:
                result.value.boolean = cmp <= 0;
                break;
            case TOK_GT:
                result.value.boolean = cmp > 0;
                break;
            case TOK_GE:
                result.value.boolean = cmp >= 0;
                break;
            default:
                break;
            }
        }

        return result;
    }

    // �߼������
    if (op == TOK_AND || op == TOK_OR) {
        result.type = RULE_TYPE_BOOLEAN;

        // ��ֵת��Ϊ����ֵ
        bool left_bool = false;
        bool right_bool = false;

        if (left->type == RULE_TYPE_BOOLEAN) {
            left_bool = left->value.boolean;
        }
        else if (left->type == RULE_TYPE_NUMBER) {
            left_bool = (left->value.number != 0);
        }

        if (right->type == RULE_TYPE_BOOLEAN) {
            right_bool = right->value.boolean;
        }
        else if (right->type == RULE_TYPE_NUMBER) {
            right_bool = (right->value.number != 0);
        }

        if (op == TOK_AND) {
            result.value.boolean = left_bool && right_bool;
        }
        else {
            result.value.boolean = left_bool || right_bool;
        }

        return result;
    }

    // �ַ��������
    if (op >= TOK_CONTAINS && op <= TOK_ICONTAINS) {
        result.type = RULE_TYPE_BOOLEAN;
        result.value.boolean = false;

        if (left->type == RULE_TYPE_STRING && right->type == RULE_TYPE_STRING) {
            switch (op) {
            case TOK_CONTAINS:
                result.value.boolean = string_contains(left->value.string.data, right->value.string.data);
                break;
            case TOK_STARTSWITH:
                result.value.boolean = string_startswith(left->value.string.data, right->value.string.data);
                break;
            case TOK_ENDSWITH:
                result.value.boolean = string_endswith(left->value.string.data, right->value.string.data);
                break;
            case TOK_REGEX:
                // TODO: ʵ��������ʽƥ��
                result.value.boolean = false;
                break;
            case TOK_ICONTAINS:
                // TODO: ʵ�ֺ��Դ�Сд�İ������
                result.value.boolean = false;
                break;
            default:
                break;
            }
        }

        return result;
    }

    // in �����
    if (op == TOK_IN) {
        result.type = RULE_TYPE_BOOLEAN;
        result.value.boolean = false;

        if (right->type == RULE_TYPE_ARRAY) {
            for (size_t i = 0; i < right->value.array.count; i++) {
                rule_value_t eq_result = evaluate_binary_op(TOK_EQ, left, &right->value.array.items[i]);
                if (eq_result.type == RULE_TYPE_BOOLEAN && eq_result.value.boolean) {
                    result.value.boolean = true;
                    break;
                }
            }
        }

        return result;
    }

    // δ֪�����
    result.type = RULE_TYPE_NULL;
    return result;
}

// ����һԪ�����
// ԭ���Ǿ�̬���������������ȥ��static
rule_value_t evaluate_unary_op(token_type_t op, const rule_value_t* operand) {
    rule_value_t result = { 0 };

    if (op == TOK_NOT) {
        result.type = RULE_TYPE_BOOLEAN;

        if (operand->type == RULE_TYPE_BOOLEAN) {
            result.value.boolean = !operand->value.boolean;
        }
        else if (operand->type == RULE_TYPE_NUMBER) {
            result.value.boolean = (operand->value.number == 0);
        }
        else {
            result.value.boolean = false;
        }
    }
    else if (op == TOK_MINUS) {
        if (operand->type == RULE_TYPE_NUMBER) {
            result.type = RULE_TYPE_NUMBER;
            result.value.number = -operand->value.number;
        }
        else {
            result.type = RULE_TYPE_NULL;
        }
    }

    return result;
}

// ����AST�ڵ�
static rule_value_t evaluate_ast_node(const ast_node_t* node, const event_context_t* context) {
    rule_value_t result = { 0 };

    if (!node) {
        result.type = RULE_TYPE_NULL;
        return result;
    }

    switch (node->type) {
    case AST_NUMBER:
    case AST_STRING:
    case AST_BOOLEAN:
        result = copy_rule_value(&node->data.literal);
        break;

    case AST_FIELD_REF:
    {
        field_id_t field_id = node->data.literal.value.field_ref.field_id;
        result = get_field_value(field_id, context);
    }
    break;

    case AST_ARRAY:
    {
        result.type = RULE_TYPE_ARRAY;
        result.value.array.count = node->data.array.element_count;
        if (result.value.array.count > 0) {
            result.value.array.items = (rule_value_t*)malloc(sizeof(rule_value_t) * result.value.array.count);
            for (size_t i = 0; i < result.value.array.count; i++) {
                result.value.array.items[i] = evaluate_ast_node(node->data.array.elements[i], context);
            }
        }
    }
    break;

    case AST_BINARY_OP:
    {
        rule_value_t left = evaluate_ast_node(node->data.binary_op.left, context);
        rule_value_t right = evaluate_ast_node(node->data.binary_op.right, context);
        result = evaluate_binary_op(node->data.binary_op.tok_opt, &left, &right);
        free_rule_value(&left);
        free_rule_value(&right);
    }
    break;

    case AST_UNARY_OP:
    {
        rule_value_t operand = evaluate_ast_node(node->data.unary_op.operand, context);
        result = evaluate_unary_op(node->data.unary_op.tok_opt, &operand);
        free_rule_value(&operand);
    }
    break;

    case AST_FUNCTION_CALL:
        // TODO: ʵ�ֺ�������
        result.type = RULE_TYPE_NULL;
        break;

    default:
        result.type = RULE_TYPE_NULL;
        break;
    }

    return result;
}

// ==================== ����APIʵ�� ====================

// �������
compiled_rule_t* compile_rule(const char* rule_string) {
    compiled_rule_t* rule = (compiled_rule_t*)malloc(sizeof(compiled_rule_t));
    memset(rule, 0, sizeof(compiled_rule_t));

    // ����Դ��
    rule->source = _strdup(rule_string);

    // ����������
    parser_state_t* parser = create_parser(rule_string);

    // �������ʽ
    rule->ast = parse_expression(parser);

    // ������
    if (parser->has_error || !rule->ast) {
        rule->is_valid = false;
        if (parser->has_error) {
            strncpy(rule->error_message, parser->error_message, sizeof(rule->error_message) - 1);
        }
        else {
            strcpy(rule->error_message, "Failed to parse expression");
        }
    }
    else if (!check_token(parser, TOK_EOF)) {
        rule->is_valid = false;
        strcpy(rule->error_message, "Unexpected token after expression");
        destroy_ast_node(rule->ast);
        rule->ast = NULL;
    }
    else {
        rule->is_valid = true;
    }

    destroy_parser(parser);
    return rule;
}

// ���ٱ����Ĺ���
void destroy_compiled_rule(compiled_rule_t* rule) {
    if (!rule) return;

    if (rule->source) {
        free(rule->source);
    }

    if (rule->ast) {
        destroy_ast_node(rule->ast);
    }

    free(rule);
}

// ��������
bool evaluate_rule(const compiled_rule_t* rule, const event_context_t* context) {
    if (!rule || !rule->is_valid || !rule->ast || !context) {
        return false;
    }

    rule_value_t result = evaluate_ast_node(rule->ast, context);
    bool ret = false;

    if (result.type == RULE_TYPE_BOOLEAN) {
        ret = result.value.boolean;
    }
    else if (result.type == RULE_TYPE_NUMBER) {
        ret = (result.value.number != 0);
    }

    free_rule_value(&result);
    return ret;
}

// �������򲢷�����ϸ���
bool evaluate_rule_ex(const compiled_rule_t* rule, const event_context_t* context, rule_value_t* result) {
    if (!rule || !rule->is_valid || !rule->ast || !context || !result) {
        return false;
    }

    *result = evaluate_ast_node(rule->ast, context);
    return true;
}

// ==================== ���Ժ���ʵ�� ====================

const char* token_type_to_string(token_type_t type) {
    switch (type) {
    case TOK_EOF: return "EOF";
    case TOK_ERROR: return "ERROR";
    case TOK_NUMBER: return "NUMBER";
    case TOK_STRING: return "STRING";
    case TOK_BOOLEAN: return "BOOLEAN";
    case TOK_IDENTIFIER: return "IDENTIFIER";
    case TOK_FIELD_REF: return "FIELD_REF";
    case TOK_PLUS: return "+";
    case TOK_MINUS: return "-";
    case TOK_MULTIPLY: return "*";
    case TOK_DIVIDE: return "/";
    case TOK_MODULO: return "%";
    case TOK_EQ: return "==";
    case TOK_NE: return "!=";
    case TOK_LT: return "<";
    case TOK_LE: return "<=";
    case TOK_GT: return ">";
    case TOK_GE: return ">=";
    case TOK_AND: return "&&";
    case TOK_OR: return "||";
    case TOK_NOT: return "!";
    case TOK_CONTAINS: return "contains";
    case TOK_STARTSWITH: return "startswith";
    case TOK_ENDSWITH: return "endswith";
    case TOK_IN: return "in";
    default: return "UNKNOWN";
    }
}

const char* ast_node_type_to_string(ast_node_type_t type) {
    switch (type) {
    case AST_NUMBER: return "NUMBER";
    case AST_STRING: return "STRING";
    case AST_BOOLEAN: return "BOOLEAN";
    case AST_FIELD_REF: return "FIELD_REF";
    case AST_ARRAY: return "ARRAY";
    case AST_BINARY_OP: return "BINARY_OP";
    case AST_UNARY_OP: return "UNARY_OP";
    case AST_FUNCTION_CALL: return "FUNCTION_CALL";
    default: return "UNKNOWN";
    }
}

void print_ast(const ast_node_t* node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    printf("%s", ast_node_type_to_string(node->type));

    switch (node->type) {
    case AST_NUMBER:
        printf(" %.2f", node->data.literal.value.number);
        break;

    case AST_STRING:
        printf(" \"%s\"", node->data.literal.value.string.data);
        break;

    case AST_BOOLEAN:
        printf(" %s", node->data.literal.value.boolean ? "true" : "false");
        break;

    case AST_FIELD_REF:
        printf(" %s.%s",
            node->data.literal.value.field_ref.object_name,
            node->data.literal.value.field_ref.field_name);
        break;

    case AST_BINARY_OP:
        printf(" %s", token_type_to_string(node->data.binary_op.tok_opt));
        break;

    case AST_UNARY_OP:
        printf(" %s", token_type_to_string(node->data.unary_op.tok_opt));
        break;

    case AST_FUNCTION_CALL:
        printf(" %s", node->data.function_call.function_name);
        break;

    default:
        break;
    }

    printf("\n");

    // �ݹ��ӡ�ӽڵ�
    switch (node->type) {
    case AST_BINARY_OP:
        print_ast(node->data.binary_op.left, indent + 1);
        print_ast(node->data.binary_op.right, indent + 1);
        break;

    case AST_UNARY_OP:
        print_ast(node->data.unary_op.operand, indent + 1);
        break;

    case AST_ARRAY:
        for (size_t i = 0; i < node->data.array.element_count; i++) {
            print_ast(node->data.array.elements[i], indent + 1);
        }
        break;

    case AST_FUNCTION_CALL:
        for (size_t i = 0; i < node->data.function_call.argument_count; i++) {
            print_ast(node->data.function_call.arguments[i], indent + 1);
        }
        break;

    default:
        break;
    }
}