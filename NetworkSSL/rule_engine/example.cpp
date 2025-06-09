/*
 * example.c - ��������ʹ��ʾ��
 * ��ʾ��α�����������¼�
 */

#include "rule_engine.h"
#include <stdio.h>
#include <string.h>

 // ���������¼�
event_context_t* create_test_event(int event_type) {
    event_context_t* event = new event_context_t;
    memset(event, 0, sizeof(event_context_t));
    switch (event_type) {
    case 1: // ���̴����¼�
        event->process.name = "cmd.exe";
        event->process.pid = 1234;
        event->process.ppid = 567;
        event->process.cmdline = "cmd.exe /c whoami";
        event->process.path = "C:\\Windows\\System32\\cmd.exe";
        event->process.user = "SYSTEM";
        event->process.session_id = 0;
        break;

    case 2: // �ļ������¼�
        event->file.path = "C:\\Windows\\System32\\drivers\\etc\\hosts";
        event->file.name = "hosts";
        event->file.extension = "";
        event->file.size = 824;
        event->file.operation = "write";
        event->file.attributes = 0x20;
        break;

    case 3: // ע�������¼�
        event->registry.key = "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        event->registry.value_name = "Malware";
        event->registry.value_type = 1; // REG_SZ
        event->registry.value_data = "C:\\malware.exe";
        event->registry.operation = "create";
        break;

    case 4: // ���������¼�
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
    printf("����: %s\n", rule_string);

    // �������
    compiled_rule_t* rule = compile_rule(rule_string);

    if (!rule->is_valid) {
        printf("����ʧ��: %s\n", rule->error_message);
        destroy_compiled_rule(rule);
        return;
    }

    printf("����ɹ�!\n");

    // ��ӡAST (������)
    printf("\nAST�ṹ:\n");
    print_ast(rule->ast, 0);

    // ��������
    bool result = evaluate_rule(rule, event);
    printf("\n�������: %s\n", result ? "ƥ��" : "��ƥ��");

    // ʹ����չ������ȡ��ϸ���
    rule_value_t detailed_result;
    if (evaluate_rule_ex(rule, event, &detailed_result)) {
        printf("��ϸ�������: ");
        switch (detailed_result.type) {
        case RULE_TYPE_BOOLEAN:
            printf("����ֵ = %s\n", detailed_result.value.boolean ? "true" : "false");
            break;
        case RULE_TYPE_NUMBER:
            printf("��ֵ = %.2f\n", detailed_result.value.number);
            break;
        case RULE_TYPE_STRING:
            printf("�ַ��� = \"%s\"\n", detailed_result.value.string.data);
            break;
        default:
            printf("��������\n");
            break;
        }
        free_rule_value(&detailed_result);
    }

    destroy_compiled_rule(rule);
}

int main() {
    printf("��������ʾ������\n");
    printf("================\n");

    // ������ͬ���͵Ĳ����¼�
    event_context_t* process_event = create_test_event(1);
    event_context_t* file_event = create_test_event(2);
    event_context_t* registry_event = create_test_event(3);
    event_context_t* network_event = create_test_event(4);

    // ����1: �򵥵Ľ�������ƥ��
    test_rule(
        "process.name == \"cmd.exe\"",
        process_event,
        "����1: ��������ƥ��"
    );

    // ����2: ���ӵĽ��̹���
    test_rule(
        "process.name == \"cmd.exe\" && process.user == \"SYSTEM\" && process.pid > 1000",
        process_event,
        "����2: ���ӽ��̹���"
    );

    // ����3: �ַ����������
    test_rule(
        "process.cmdline contains \"whoami\"",
        process_event,
        "����3: �����а������"
    );

    // ����4: �ļ�·�����
    test_rule(
        "file.path startswith \"C:\\\\Windows\\\\System32\" && file.operation == \"write\"",
        file_event,
        "����4: ϵͳ�ļ�д����"
    );

    // ����5: ע�����������
    test_rule(
        "registry.key endswith \"\\\\Run\" && registry.operation == \"create\"",
        registry_event,
        "����5: ע����������"
    );

    // ����6: �������Ӽ��
    test_rule(
        "network.dst_port == 53 || network.dst_port == 80 || network.dst_port == 443",
        network_event,
        "����6: �����˿�����"
    );

    // ����7: ʹ��in�����
    test_rule(
        "network.dst_port in [53, 80, 443, 8080]",
        network_event,
        "����7: �˿��б���"
    );

    // ����8: ���ӵ���Ϲ���
    test_rule(
        "(process.name == \"powershell.exe\" || process.name == \"cmd.exe\") && "
        "process.user != \"Administrator\" && "
        "process.ppid > 0",
        process_event,
        "����8: ���ɽ��̼��"
    );

    // ����9: ��ѧ����
    test_rule(
        "file.size > 500 && file.size < 1024",
        file_event,
        "����9: �ļ���С��Χ���"
    );

    // ����10: �߼���������ȼ�
    test_rule(
        "!process.name == \"explorer.exe\" && process.pid > 1000 || process.user == \"SYSTEM\"",
        process_event,
        "����10: �����߼����ʽ"
    );

    // ���Դ������
    test_rule(
        "process.invalid_field == \"test\"",
        process_event,
        "���Դ���: ��Ч�ֶ�"
    );

    test_rule(
        "process.name ==",
        process_event,
        "���Դ���: �﷨����"
    );

    // �ͷ���Դ
    delete(process_event);
    delete(file_event);
    delete(registry_event);
    delete(network_event);

    printf("\nʾ���������\n");
    return 0;
}