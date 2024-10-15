#pragma once

#include "cJSON.h"

#ifdef __cplusplus
// C++ ����
#include "../StringHandler/StringHandler.h"

#include <memory>

class CJSONHandler
{
public:
	CJSONHandler() 
        : m_object(cJSON_CreateObject(), CJSONHandler::cJSONDeleter)
    {
		/*m_object = cJSON_CreateObject();*/
	}

	CJSONHandler(cJSON* obj)
        : m_object(obj, CJSONHandler::cJSONDeleter)
    {
	}

    CJSONHandler(std::shared_ptr<cJSON> obj)
        : m_object(obj)
    {
    }

	~CJSONHandler() {
	}

public:
    // ���������[]�����ڶ�ȡԪ��
    CJSONHandler operator[](wchar_t* key) const {
        if (!m_object) {
            return CJSONHandler();
        }

        char* pkey = nullptr;
        if (CStringHandler::WChar2Ansi(key, pkey)) {
            std::shared_ptr<cJSON> key_mem(cJSON_GetObjectItemCaseSensitive(m_object.get(), pkey),
                CJSONHandler::cJSONDeleter);
            return CJSONHandler(key_mem);
        }

        return CJSONHandler();
    }

// ����һЩ�ⲿ�ܹ�ʹ�õľ�̬����
public:
    // ����һ���Զ���ɾ��������
    static void cJSONDeleter(cJSON* json) {
        cJSON_Delete(json);
    }

private:
	std::shared_ptr<cJSON> m_object;
};

#endif


