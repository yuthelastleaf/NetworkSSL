#pragma once

#include "cJSON.h"

#ifdef __cplusplus
// C++ 编译
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
    // 重载运算符[]，用于读取元素
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

// 定义一些外部能够使用的静态函数
public:
    // 定义一个自定义删除器函数
    static void cJSONDeleter(cJSON* json) {
        cJSON_Delete(json);
    }

private:
	std::shared_ptr<cJSON> m_object;
};

#endif


