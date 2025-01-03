#include "CDriverJson.h"

size_t ctm_strlen(const char* str)
{
    const char* s = str;
    while (*s)
        ++s;
    return (s - str);
}

cJSON_bool ctm_strcmp(const char* src, const char* dst) {
    cJSON_bool flag = cJSON_False;
    if (src && dst) {
        size_t slen = ctm_strlen(src);
        size_t dlen = ctm_strlen(dst);
        if (slen == dlen) {
            int cmp_len = 0;
            for (cmp_len = 0; cmp_len < slen; cmp_len++) {
                if (src[cmp_len] != dst[cmp_len]) {
                    break;
                }
            }
            if (cmp_len == slen) {
                flag = cJSON_True;
            }
        }
    }
    return flag;
}

cJSON_bool __stdcall CreateDJson(cDriverJSON** json, const char* str_json)
{
    cJSON_bool flag = cJSON_False;

    cJSON* obj = cJSON_Parse(str_json);

    flag = InitDriverJSON(json, obj, NULL);

    if (!flag) {
        cJSON_Delete(obj);
    }

    return flag;
}

cJSON_bool __stdcall ReleaseDJson(cDriverJSON* json)
{
    cJSON_bool flag = cJSON_False;
    if (!json->parent && json->json_obj) {
        cJSON_Delete(json->json_obj);
    }
    return flag;
}

cJSON_bool __stdcall InitDriverJSON(cDriverJSON** json, cJSON* obj, cJSON* parent)
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
        
        (*json)->getpathint = getpathint;
        (*json)->getpathstring = getpathstring;
        
        (*json)->getjsonstring = getjsonstring;
        (*json)->setint = setint;
        (*json)->setstring = setstring;
        (*json)->setwstring = setwstring;
        
        (*json)->setpathint = setpathint;
        (*json)->setpathstring = setpathstring;
        (*json)->setpathwstring = setpathwstring;

        flag = cJSON_True;
    }

	return flag;
}

cJSON_bool __stdcall setstring(cDriverJSON* json, const char* value)
{
    cJSON_bool flag = cJSON_False;

    if (json->parent) {
        if (json->json_obj) {
            if (cJSON_UpdateType(json->json_obj, cJSON_String) == cJSON_True) {
                if (cJSON_UpdateType(json->parent, cJSON_Object) == cJSON_True) {
                    cJSON_SetValuestring(json->json_obj, value);
                    flag = cJSON_True;
                }
            }
        }
    }

    return flag;
}

cJSON_bool __stdcall setwstring(cDriverJSON* json, const wchar_t* value)
{
    _Unreferenced_parameter_(json);
    _Unreferenced_parameter_(value);


    return cJSON_False;
}

cJSON_bool __stdcall setint(cDriverJSON* json, const int value)
{
    cJSON_bool flag = cJSON_False;

    if (json->parent) {
        if (json->json_obj) {
            if (cJSON_UpdateType(json->json_obj, cJSON_Number) == cJSON_True) {
                if (cJSON_UpdateType(json->parent, cJSON_Object) == cJSON_True) {
                    cJSON_SetIntValue(json->json_obj, value);
                    flag = cJSON_True;
                }
            }
        }
    }

    return flag;
}

cJSON_bool __stdcall setpathstring(cDriverJSON* json, const char* path, const char* value)
{
    cDriverJSON* obj = json->get(json, path);
    return obj->setstring(obj, value);
}

cJSON_bool __stdcall setpathwstring(cDriverJSON* json, const char* path, const wchar_t* value)
{
    cDriverJSON* obj = json->get(json, path);
    return obj->setwstring(obj, value);
}

cJSON_bool __stdcall setpathint(cDriverJSON* json, const char* path, const int value)
{
    cDriverJSON* obj = json->get(json, path);
    return obj->setint(obj, value);
}

char* __stdcall getstring(cDriverJSON* json)
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

char* __stdcall getjsonstring(cDriverJSON* json)
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

int __stdcall getint(cDriverJSON* json)
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

char* __stdcall getpathstring(cDriverJSON* json, const char* path)
{
    cDriverJSON* obj = json->get(json, path);
    return obj->getstring(obj);
}

int __stdcall getpathint(cDriverJSON* json, const char* path)
{
    cDriverJSON* obj = json->get(json, path);
    return obj->getint(obj);
}

cDriverJSON* __stdcall get(cDriverJSON* json, const char* path)
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
                    if (!(cDriverJSON*)child->reference) {
                        cDriverJSON* new_child = NULL;
                        InitDriverJSON(&new_child, child, dst_json->json_obj);
                        dst_json = new_child;
                    }
                    else {
                        dst_json = (cDriverJSON*)child->reference;
                    }
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

        cJSON_free(mypath);

    } while (0);

    return dst_json;
}
