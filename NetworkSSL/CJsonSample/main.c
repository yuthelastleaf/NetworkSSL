#include <stdio.h>
#include <stdlib.h>
#include "../../include/CJSON/cJSON.h"

int main() {
    // 1. 创建一个JSON对象
    cJSON* root = cJSON_CreateObject();

    // 2. 添加基本数据类型：字符串、数字、布尔值
    cJSON_AddStringToObject(root, "name", "ChatGPT");
    cJSON_AddNumberToObject(root, "age", 4);
    cJSON_AddBoolToObject(root, "isAI", 1);

    // 3. 创建并添加一个数组
    cJSON* hobbies = cJSON_CreateArray();
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Coding"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Learning"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("Chatting"));
    cJSON_AddItemToObject(root, "hobbies", hobbies);

    // 4. 创建并添加一个嵌套的JSON对象
    cJSON* address = cJSON_CreateObject();
    cJSON_AddStringToObject(address, "city", "San Francisco");
    cJSON_AddStringToObject(address, "state", "California");
    cJSON_AddNumberToObject(address, "zipcode", 94103);
    cJSON_AddItemToObject(root, "address", address);

    // 5. 转换JSON对象为字符串并打印
    char* json_string = cJSON_Print(root);
    printf("Created JSON:\n%s\n", json_string);

    // 6. 解析JSON字符串
    const char* json_data = "{\"name\":\"OpenAI\",\"type\":\"Research\",\"employees\":1000}";
    cJSON* parsed_json = cJSON_Parse(json_data);

    if (parsed_json == NULL) {
        printf("Error parsing JSON.\n");
    }
    else {
        // 7. 获取数据并打印
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

        // 8. 释放解析的 JSON 对象
        cJSON_Delete(parsed_json);
    }

    // 9. 释放已分配的字符串和 JSON 对象
    free(json_string);
    cJSON_Delete(root);

    return 0;
}
