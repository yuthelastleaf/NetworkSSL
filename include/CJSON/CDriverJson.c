#include "CDriverJson.h"

size_t ctm_strlen(const char* str)
{
    const char* s = str;
    while (*s)
        ++s;
    return (s - str);
}

cJSON_bool InitDriverJSON(cDriverJSON** json, cJSON* obj, cJSON* parent)
{
    cJSON_bool flag = cJSON_False;

    if (*json) {
        cJSON_free(*json);
    }
    *json = cJSON_malloc(sizeof(cDriverJSON));
    if (!obj) {
        (*json)->json_obj = cJSON_CreateObject();
    }
    else {
        (*json)->json_obj = obj;
    }
    if ((*json)->json_obj) {
        cJSON_SetRef((*json)->json_obj, (*json));
        (*json)->parent = parent;

        (*json)->get = get;
        (*json)->getint = getint;
        (*json)->getstring = getstring;
        (*json)->getjsonstring = getjsonstring;
        (*json)->setint = setint;
        (*json)->setstring = setstring;
        (*json)->setwstring = setwstring;

        flag = cJSON_True;
    }

	return flag;
}

cJSON_bool setstring(cDriverJSON* json, const char* value)
{
    cJSON_bool flag = cJSON_False;

    if (json->parent) {
        if (json->json_obj) {
            if (cJSON_UpdateType(json->json_obj, cJSON_String)) {
                if (cJSON_UpdateType(json->parent, cJSON_Object)) {
                    cJSON_SetValuestring(json->json_obj, value);
                    flag = cJSON_True;
                }
            }
        }
    }

    return flag;
}

cJSON_bool setwstring(cDriverJSON* json, const wchar_t* value)
{
    return cJSON_False;
}

cJSON_bool setint(cDriverJSON* json, const int value)
{
    cJSON_bool flag = cJSON_False;

    if (json->parent) {
        if (json->json_obj) {
            if (cJSON_UpdateType(json->json_obj, cJSON_Number)) {
                if (cJSON_UpdateType(json->parent, cJSON_Object)) {
                    cJSON_SetIntValue(json->json_obj, value);
                    flag = cJSON_True;
                }
            }
        }
    }

    return flag;
}

char* getstring(cDriverJSON* json)
{
    char* res = NULL;
    do {
        if (!json->json_obj) {
            break;
        }

        if (cJSON_IsString(json->json_obj)) {
            res = json->json_obj->valuestring;
        }
    } while (0);

    return res;
}

char* getjsonstring(cDriverJSON* json)
{
    char* res = NULL;
    do
    {
        if (!json->json_obj) {
            break;
        }
        res = cJSON_Print(json->json_obj);
    } while (0);

    return res;
}

int getint(cDriverJSON* json)
{
    int res = 0;
    do {
        if (!json->json_obj) {
            break;
        }

        if (cJSON_IsNumber(json->json_obj)) {
            res = json->json_obj->valueint;
        }
    } while (0);

    return res;
}

cDriverJSON* get(cDriverJSON* json, const char* path)
{
    cDriverJSON* dst_json = json;
    do
    {
        if (!json) {
            break;
        }
        if (!json->json_obj) {
            break;
        }
        char* mypath = NULL;
        char* parse_path = NULL;
        int i = 0;
        size_t len = ctm_strlen(path) + 1;

        mypath = cJSON_malloc(len);

        memset(mypath, 0, len);
        memcpy(mypath, path, len - 1);

        parse_path = mypath;
        for (i = 0; i <= len; i++) {
            if (!(mypath[i] ^ '/') || !(mypath[i] ^ '\0')) {
                mypath[i] = '\0';

                if (!ctm_strlen(parse_path)) {
                    parse_path = mypath + i + 1;
                    continue;
                }

                cJSON* child = cJSON_GetObjectItemCaseSensitive(dst_json->json_obj, parse_path);

                if (child) {
                    dst_json = (cDriverJSON*)child->reference;
                }
                else {
                    cDriverJSON* new_child = NULL;
                    InitDriverJSON(&new_child, NULL, dst_json->json_obj);
                    cJSON_AddItemToObject(dst_json->json_obj, parse_path, new_child->json_obj);
                    dst_json = new_child;
                }
                parse_path = mypath + i + 1;
            }
        }
    } while (0);

    return dst_json;
}
