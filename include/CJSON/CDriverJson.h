#ifndef CDRIVER_JSON_H_
#define CDRIVER_JSON_H_

#include "cJSON.h"

size_t ctm_strlen(const char* str);
cJSON_bool ctm_strcmp(const char* src, const char* dst);

typedef struct _cDriverJSON {
    cJSON* json_obj;
    cJSON* parent;

    // 方法指针
    cJSON_bool(*setstring)(struct _cDriverJSON*, const char*);
    cJSON_bool(*setwstring)(struct _cDriverJSON*, const wchar_t*);
    cJSON_bool(*setint)(struct _cDriverJSON*, const int);
    char* (*getstring)(struct _cDriverJSON*);
    int (*getint)(struct _cDriverJSON*);
    struct _cDriverJSON* (*get)(struct _cDriverJSON*, const char*);
    char* (*getjsonstring)(struct _cDriverJSON*);
} cDriverJSON, *PcDriverJSON;

cJSON_bool __stdcall CreateDJson(cDriverJSON** json, const char* str_json);
// 初始化 cJSONHandler
cJSON_bool __stdcall InitDriverJSON(cDriverJSON** json, cJSON* obj, cJSON* parent);
cJSON_bool __stdcall ReleaseDJson(cDriverJSON* json);

cJSON_bool __stdcall setstring(cDriverJSON* json, const char* value);
cJSON_bool __stdcall setwstring(cDriverJSON* json, const wchar_t* value);
cJSON_bool __stdcall setint(cDriverJSON* json, const int value);
char* __stdcall getstring(cDriverJSON* json);
char* __stdcall getjsonstring(cDriverJSON* json);
int __stdcall getint(cDriverJSON* json);
cDriverJSON* __stdcall get(cDriverJSON* json, const char* path);



#endif // CDRIVER_JSON_H_
