#include <stdio.h>
#include <stdlib.h>
#include "../../include/CJSON/cJSON.h"

int main() {
    // 1. ����һ��JSON����
    cJSON* root = cJSON_CreateObject();

    // 2. ��ӻ����������ͣ��ַ��������֡�����ֵ
    cJSON_AddStringToObject(root, "name", "ChatGPT");
    cJSON_AddNumberToObject(root, "age", 4);
    cJSON_AddBoolToObject(root, "isAI", 1);

    // 3. ���������һ������
    cJSON* hobbies = cJSON_CreateArray();
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Coding"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Learning"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Chatting"));
    cJSON_AddItemToObject(root, "hobbies", hobbies);

    // 4. ���������һ��Ƕ�׵�JSON����
    cJSON* address = cJSON_CreateObject();
    cJSON_AddStringToObject(address, "city", "San Francisco");
    cJSON_AddStringToObject(address, "state", "California");
    cJSON_AddNumberToObject(address, "zipcode", 94103);
    cJSON_AddItemToObject(root, "address", address);

    // 5. ת��JSON����Ϊ�ַ�������ӡ
    char* json_string = cJSON_Print(root);
    printf("Created JSON:\n%s\n", json_string);

    // 6. ����JSON�ַ���
    const char* json_data = "{\"name\":\"OpenAI\",\"type\":\"Research\",\"employees\":1000}";
    cJSON* parsed_json = cJSON_Parse(json_data);

    if (parsed_json == NULL) {
        printf("Error parsing JSON.\n");
    }
    else {
        // 7. ��ȡ���ݲ���ӡ
        const cJSON* name = cJSON_GetObjectItemCaseSensitive(parsed_json, "name");
        const cJSON* type = cJSON_GetObjectItemCaseSensitive(parsed_json, "type");
        const cJSON* employees = cJSON_GetObjectItemCaseSensitive(parsed_json, "employees");

        if (cJSON_IsString(name) && (name->valuestring != NULL)) {
            printf("Name: %s\n", name->valuestring);
        }

        if (cJSON_IsString(type) && (type->valuestring != NULL)) {
            printf("Type: %s\n", type->valuestring);
        }

        if (cJSON_IsNumber(employees)) {
            printf("Employees: %d\n", employees->valueint);
        }

        // 8. �ͷŽ����� JSON ����
        cJSON_Delete(parsed_json);
    }

    // 9. �ͷ��ѷ�����ַ����� JSON ����
    free(json_string);
    cJSON_Delete(root);

    return 0;
}
