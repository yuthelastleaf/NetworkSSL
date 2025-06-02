#pragma once

#include "cJSON.h"

typedef struct _cJSONHanlerObj {
    cJSON* json_obj;
    cJSON* parent;
} cJSONHanlerObj;

#ifdef __cplusplus
// C++ ����
#include "../StringHandler/StringHandler.h"

#include <memory>


class CJSONHandler
{
public:
	CJSONHandler() 
        : m_object({ cJSON_CreateObject(), nullptr })
    {
		/*m_object = cJSON_CreateObject();*/
	}

	CJSONHandler(cJSON* obj, cJSON* parent)
        : m_object({ obj, parent })
    {
	}

    CJSONHandler(char* json_str)
        : m_object({ cJSON_Parse(json_str), nullptr })
    {
    }

    CJSONHandler(const char* json_str)
        : m_object({ cJSON_Parse(json_str), nullptr })
    {
    }

	~CJSONHandler() {
        if (!m_object.parent) {
            cJSON_Delete(m_object.json_obj);
        }
	}

public:
    bool SetString(const wchar_t* value) {
        bool flag = false;
        char* pvalue = nullptr;
        if (m_object.json_obj && CStringHandler::WChar2UTF8(value, pvalue)) {
            // ����ǰ��������Ϊ�ַ������ͣ���������ֵ
            m_object.json_obj->type = cJSON_String;
            flag = cJSON_SetValuestring(m_object.json_obj, pvalue);
        }
        return flag;
    }

    bool SetString(const char* value) {
        bool flag = false;
        if (m_object.json_obj) {
            m_object.json_obj->type = cJSON_String;
            flag = cJSON_SetValuestring(m_object.json_obj, value);
        }
        return flag;
    }

    std::shared_ptr<char> GetJsonString() {
        if (m_object.json_obj) {
            char* jsonString = cJSON_Print(m_object.json_obj);
            return std::shared_ptr<char>(jsonString, [](char* ptr) {
                cJSON_free(ptr);
                });
        }
        return nullptr; // ���ؿ�ָ�����������Ч
    }

    // ���µ�ǰ�ַ���
    bool UpdateJson(const char* json) {
        bool flag = false;
        
        if (!m_object.parent) {
            cJSON_Delete(m_object.json_obj);

            m_object.json_obj = cJSON_Parse(json);
            flag = true;
        }

        return flag;
    }

public:
    // ���������[]�����ڶ�ȡԪ��
    CJSONHandler operator[](const wchar_t* key) const {
        if (!m_object.json_obj) {
            return CJSONHandler();
        }
        char* pkey = nullptr;
        if (CStringHandler::WChar2UTF8(key, pkey)) {
            cJSON* child = cJSON_GetObjectItemCaseSensitive(m_object.json_obj, pkey);
            if (!child) {
                child = cJSON_CreateObject();
                cJSON_AddItemToObject(m_object.json_obj, pkey, child);
            }
            delete[] pkey;
            return CJSONHandler(child, m_object.json_obj);
        }
        return CJSONHandler();
    }

    CJSONHandler operator[](const char* key) const {
        if (!m_object.json_obj) {
            return CJSONHandler();
        }
        cJSON* child = cJSON_GetObjectItemCaseSensitive(m_object.json_obj, key);
        if (!child) {
            child = cJSON_CreateObject();
            cJSON_AddItemToObject(m_object.json_obj, key, child);
        }
        return CJSONHandler(child, m_object.json_obj);
    }

    // ��ֵ���������
    CJSONHandler& operator=(const wchar_t* value) {
        if (cJSON_UpdateType(m_object.json_obj, cJSON_String)) {
            if (cJSON_UpdateType(m_object.parent, cJSON_Object)) {
                char* pvalue = nullptr;
                if (CStringHandler::WChar2UTF8(value, pvalue)) {
                    cJSON_SetValuestring(m_object.json_obj, pvalue);
                    delete[] pvalue;
                }
            }
        }
        return *this;
    }

    // ��ֵ���������
    CJSONHandler& operator=(const char* value) {
        if (cJSON_UpdateType(m_object.json_obj, cJSON_String)) {
            if (cJSON_UpdateType(m_object.parent, cJSON_Object)) {
                cJSON_SetValuestring(m_object.json_obj, value);
            }
        }
        return *this;
    }

    // ��ֵ���������
    CJSONHandler& operator=(int value) {
        if (cJSON_UpdateType(m_object.json_obj, cJSON_Number)) {
            if (cJSON_UpdateType(m_object.parent, cJSON_Object)) {
                cJSON_SetNumberValue(m_object.json_obj, value);
            }
        }
        return *this;
    }

    const char* GetString() const {
        char* res = nullptr;
        do {
            if (!m_object.json_obj) {
                break;
            }

            if (cJSON_IsString(m_object.json_obj)) {
                res = m_object.json_obj->valuestring;
            }
        } while (0);

        return res;
    }

    std::unique_ptr<wchar_t> GetWString() const {
        wchar_t* obj_str = new wchar_t[1];
        memset(obj_str, 0, sizeof(wchar_t));
        std::unique_ptr<wchar_t> res(obj_str);
        do {
            if (!m_object.json_obj) {
                break;
            }

            if (!cJSON_IsString(m_object.json_obj)) {
                break;
            }

            wchar_t* new_obj_str = nullptr;
            if (!CStringHandler::Ansi2WChar(m_object.json_obj->valuestring, new_obj_str)) {
                break;
            }

            res.reset(new_obj_str);
        } while (0);

        return std::move(res);  // ���û���ҵ��ַ��������Ͳ�ƥ��
    }

    const int GetInt() const {
        int res = 0;
        do {
            if (!m_object.json_obj) {
                break;
            }

            if (cJSON_IsNumber(m_object.json_obj)) {
                res = m_object.json_obj->valueint;
            }
        } while (0);

        return res;
    }


// ����һЩ������Ҫ�õ��ķ���
private:


// ����һЩ�ⲿ�ܹ�ʹ�õľ�̬����
public:
    // ����һ���Զ���ɾ��������
    static void cJSONDeleter(cJSON* json) {
        cJSON_Delete(json);
    }

private:
    cJSONHanlerObj m_object;
};

#endif


