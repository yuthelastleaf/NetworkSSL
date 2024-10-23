#ifndef CDRIVER_JSON_H_
#define CDRIVER_JSON_H_

#include "CJSONHanler.h"

typedef struct _cDriverJSON {
    cJSON* json_obj;
    cJSON* parent;

    // 方法指针
    cJSON_bool(*addobj)(struct cDriverJSON*, const char*, struct cDriverJSON*);
    cJSON_bool(*setstring)(struct cDriverJSON*, const char*);
    cJSON_bool(*setwstring)(struct cDriverJSON*, const wchar_t*);
    cJSON_bool(*setint)(struct cDriverJSON*, const int);
    char* (*getstring)(struct cDriverJSON*);
    int (*getint)(struct cDriverJSON*);
    struct cDriverJSON*(*get)(struct cDriverJSON*, const char*);
    char* (*getjsonstring)(struct cDriverJSON*);
} cDriverJSON;

// 初始化 cJSONHandler
cJSON_bool InitDriverJSON(cDriverJSON** json, cJSON* obj, cJSON* parent);

cJSON_bool addobj(cDriverJSON* par_json, const char* key, cDriverJSON* chd_json);
cJSON_bool setstring(cDriverJSON* json, const char* value);
cJSON_bool setwstring(cDriverJSON* json, const wchar_t* value);
cJSON_bool setint(cDriverJSON* json, const int value);
char* getstring(cDriverJSON* json);
char* getjsonstring(cDriverJSON* json);
int getint(cDriverJSON* json);
cDriverJSON* get(cDriverJSON* json, const char* path);


#endif // CDRIVER_JSON_H_
