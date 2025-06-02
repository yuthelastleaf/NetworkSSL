#ifndef STRING_HANDLER_H_
#define STRING_HANDLER_H_

#include <stdlib.h>
#include <locale.h>
#include <vector>

#define MAX_POINTER_CNT 100

namespace CStringHandler {

	void InitChinese() {
		setlocale(LC_ALL, "chs");
	}

	// 判断字符串是不是utf8字符
	bool IsUtf8(const void* pBuffer, long size)
	{
		bool IsUTF8 = true;
		unsigned char* start = (unsigned char*)pBuffer;
		unsigned char* end = (unsigned char*)pBuffer + size;
		while (start < end)
		{
			if (*start < 0x80) // (10000000): 值小于0x80的为ASCII字符
			{
				start++;
			}
			else if (*start < (0xC0)) // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
			{
				IsUTF8 = false;
				break;
			}
			else if (*start < (0xE0)) // (11100000): 此范围内为2字节UTF-8字符
			{
				if (start >= end - 1)
				{
					break;
				}

				if ((start[1] & (0xC0)) != 0x80)
				{
					IsUTF8 = false;
					break;
				}

				start += 2;
			}
			else if (*start < (0xF0)) // (11110000): 此范围内为3字节UTF-8字符
			{
				if (start >= end - 2)
				{
					break;
				}

				if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
				{
					IsUTF8 = false;
					break;
				}

				start += 3;
			}
			else
			{
				IsUTF8 = false;
				break;
			}
		}

		return IsUTF8;
	}

	// 宽字符转换为窄字符
	bool WChar2Ansi(const wchar_t* pwszSrc, char* &pszDst)
	{
		bool flag = false; 
		size_t len = 0;
		size_t trans_len = (wcslen(pwszSrc) * 2) + 1;

		pszDst = new char[trans_len];
		errno_t err = wcstombs_s(&len, pszDst, trans_len, pwszSrc, trans_len);

		if (len > 0) {
			flag = true;
		}
		else {
			delete[] pszDst;
			pszDst = nullptr;
		}
		return flag;
	}

	// 窄字符转换为宽字符
	bool Ansi2WChar(const char* pszSrc, wchar_t* &pwszDst)
	{
		bool flag = false;
		size_t len = 0;
		size_t trans_len = strlen(pszSrc) + 1;

		pwszDst = new wchar_t[trans_len];
		errno_t err = mbstowcs_s(&len, pwszDst, trans_len, pszSrc, trans_len );

		if (len > 0) {
			flag = true;
		}
		else {
			delete[] pwszDst;
			pwszDst = nullptr;
		}
		return flag;
	}

	class StrSplitHandler
	{
	public:
		StrSplitHandler()
			: split_res_(nullptr)
			, elem_cnt_(0)
		{

		}
		~StrSplitHandler() {
			if (split_res_ != nullptr) {
				free(split_res_);
			}
		}


#ifdef UNICODE
		bool StrSplit(wchar_t* str_cont, unsigned long str_len, wchar_t split_elem = L' ')
#else
		bool StrSplit(char* str_cont, unsigned long str_len, char split_elem = ' ')
#endif
		{
			bool flag = false;
			unsigned int max_cnt = MAX_POINTER_CNT;
			unsigned int pre_pos = 0;

			do {
				if (split_res_) {
					free(split_res_);
					split_res_ = nullptr;
				}

#ifdef UNICODE
				split_res_ = (wchar_t**)malloc(max_cnt);
#else
				split_res_ = (char**)malloc(max_cnt);
#endif
				

				if (split_res_ == nullptr) {
					break;
				}
				if (str_cont == nullptr) {
					break;
				}

				for (int i = 0; i < str_len - 1; i++) {

					if (!(str_cont[i] ^ split_elem)) {
						split_res_[elem_cnt_] = &str_cont[pre_pos];
						elem_cnt_++;
						if (elem_cnt_ >= max_cnt) {
							max_cnt += MAX_POINTER_CNT;
#ifdef UNICODE
							split_res_ = (wchar_t**)realloc(split_res_, max_cnt);
#else
							split_res_ = (char**)realloc(split_res_, max_cnt);
#endif			
							if (!split_res_) {
								break;
							}
						}
						str_cont[i] = 0;
						pre_pos = i + 1;
					}

				}

				if (pre_pos < str_len - 1) {
					split_res_[elem_cnt_] = &str_cont[pre_pos];
					elem_cnt_++;
				}

				flag = true;
			} while (0);

			return flag;
		}

		unsigned int GetSplitCnt() {

			return elem_cnt_;
		}

		template<typename T>
		void _TransToNumList(std::vector<T>&vt) {

			if (!split_res_) {
				return;
			}

			for (int i = 0; i < elem_cnt_; i++) {

#ifdef UNICODE
				vt.push_back(_wtoi(split_res_[i]));
#else
				vt.push_back(atoi(split_res[i]));
#endif
				
			}

		}

		template<typename T>
		void _TransToStrList(std::vector<T>& vt) {

			if (!split_res_) {
				return;
		}

			for (int i = 0; i < elem_cnt_; i++) {
				vt.push_back(split_res_[i]);
			}

		}

	private:
#ifdef UNICODE
		wchar_t** split_res_;
#else
		char** split_res_;
#endif
		
		unsigned int elem_cnt_;
	};

    // 修改或添加这个函数
    bool WChar2UTF8(const wchar_t* wstr, char*& utf8str) {
        if (!wstr) return false;
#ifdef _WIN32
        // 计算需要的缓冲区大小
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len == 0) return false;
        // 分配缓冲区
        utf8str = new char[utf8Len];
        // 转换为 UTF-8
        int result = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8str, utf8Len, nullptr, nullptr);
        return result != 0;
#else
        // Linux/macOS 上 wchar_t 通常是 UTF-32，需要转换为 UTF-8
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string utf8 = converter.to_bytes(wstr);
        utf8str = new char[utf8.length() + 1];
        strcpy(utf8str, utf8.c_str());
        return true;
#endif
    }
    // 同样，如果需要从 UTF-8 转回宽字符
    bool UTF82WChar(const char* utf8str, wchar_t*& wstr) {
        if (!utf8str) return false;
#ifdef _WIN32
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, nullptr, 0);
        if (wideLen == 0) return false;
        wstr = new wchar_t[wideLen];
        int result = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wstr, wideLen);
        return result != 0;
#else
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide = converter.from_bytes(utf8str);
        wstr = new wchar_t[wide.length() + 1];
        wcscpy(wstr, wide.c_str());
        return true;
#endif
    }

    // ANSI/GBK 转 UTF-8
    bool ANSI2UTF8(const char* ansiStr, char*& utf8Str) {
        if (!ansiStr) return false;

#ifdef _WIN32
        // 先转为宽字符
        int wideLen = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, nullptr, 0);
        if (wideLen == 0) return false;

        wchar_t* wideStr = new wchar_t[wideLen];
        int result1 = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, wideLen);
        if (result1 == 0) {
            delete[] wideStr;
            return false;
        }

        // 再转为 UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len == 0) {
            delete[] wideStr;
            return false;
        }

        utf8Str = new char[utf8Len];
        int result2 = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, utf8Len, nullptr, nullptr);

        delete[] wideStr;
        return result2 != 0;
#else
        // Linux/macOS 通常已经是 UTF-8，直接复制
        size_t len = strlen(ansiStr) + 1;
        utf8Str = new char[len];
        strcpy(utf8Str, ansiStr);
        return true;
#endif
    }

    // UTF-8 转 ANSI/GBK
    bool UTF82ANSI(const char* utf8Str, char*& ansiStr) {
        if (!utf8Str) return false;

#ifdef _WIN32
        // 先转为宽字符
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
        if (wideLen == 0) return false;

        wchar_t* wideStr = new wchar_t[wideLen];
        int result1 = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wideStr, wideLen);
        if (result1 == 0) {
            delete[] wideStr;
            return false;
        }

        // 再转为 ANSI
        int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        if (ansiLen == 0) {
            delete[] wideStr;
            return false;
        }

        ansiStr = new char[ansiLen];
        int result2 = WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, ansiLen, nullptr, nullptr);

        delete[] wideStr;
        return result2 != 0;
#else
        // Linux/macOS 通常已经是 UTF-8，直接复制
        size_t len = strlen(utf8Str) + 1;
        ansiStr = new char[len];
        strcpy(ansiStr, utf8Str);
        return true;
#endif
    }

}

class DualString {
private:
    char* m_ansiStr;
    wchar_t* m_wideStr;
    bool m_ownAnsi;    // 标记是否拥有ansi字符串的内存
    bool m_ownWide;    // 标记是否拥有宽字符串的内存
    bool m_converted;  // 标记是否已经进行了转换

    // 释放资源
    void FreeMemory() {
        if (m_ansiStr && m_ownAnsi) {
            delete[] m_ansiStr;
            m_ansiStr = nullptr;
        }

        if (m_wideStr && m_ownWide) {
            delete[] m_wideStr;
            m_wideStr = nullptr;
        }

        m_ownAnsi = false;
        m_ownWide = false;
        m_converted = false;
    }

    // 宽字符转换为窄字符 (懒转换)
    bool ConvertWideToAnsi() {
        if (m_ansiStr && m_converted) {
            return true;  // 已经转换过了
        }

        if (!m_wideStr) {
            return false;  // 没有源字符串
        }

        // 如果已有ansi字符串但不拥有它，先释放引用
        if (m_ansiStr && !m_ownAnsi) {
            m_ansiStr = nullptr;
        }

        char* newAnsi = nullptr;
        bool result = CStringHandler::WChar2Ansi(m_wideStr, newAnsi);

        if (result) {
            // 如果已经拥有ansi字符串，先释放它
            if (m_ansiStr && m_ownAnsi) {
                delete[] m_ansiStr;
            }

            m_ansiStr = newAnsi;
            m_ownAnsi = true;
            m_converted = true;
        }

        return result;
    }

    // 窄字符转换为宽字符 (懒转换)
    bool ConvertAnsiToWide() {
        if (m_wideStr && m_converted) {
            return true;  // 已经转换过了
        }

        if (!m_ansiStr) {
            return false;  // 没有源字符串
        }

        // 如果已有宽字符串但不拥有它，先释放引用
        if (m_wideStr && !m_ownWide) {
            m_wideStr = nullptr;
        }

        wchar_t* newWide = nullptr;
        bool result = CStringHandler::Ansi2WChar(m_ansiStr, newWide);

        if (result) {
            // 如果已经拥有宽字符串，先释放它
            if (m_wideStr && m_ownWide) {
                delete[] m_wideStr;
            }

            m_wideStr = newWide;
            m_ownWide = true;
            m_converted = true;
        }

        return result;
    }

public:

    // 从窄字符构造
    explicit DualString(const char* ansiStr, bool makeCopy = true)
        : m_wideStr(nullptr), m_ownWide(false), m_converted(false) {
        if (ansiStr) {
            if (makeCopy) {
                size_t len = strlen(ansiStr) + 1;
                m_ansiStr = new char[len];
                strcpy_s(m_ansiStr, len, ansiStr);
                m_ownAnsi = true;
            }
            else {
                m_ansiStr = const_cast<char*>(ansiStr);
                m_ownAnsi = false;
            }
        }
        else {
            m_ansiStr = nullptr;
            m_ownAnsi = false;
        }
    }

    // 从宽字符构造
    explicit DualString(const wchar_t* wideStr, bool makeCopy = true)
        : m_ansiStr(nullptr), m_ownAnsi(false), m_converted(false) {
        if (wideStr) {
            if (makeCopy) {
                size_t len = wcslen(wideStr) + 1;
                m_wideStr = new wchar_t[len];
                wcscpy_s(m_wideStr, len, wideStr);
                m_ownWide = true;
            }
            else {
                m_wideStr = const_cast<wchar_t*>(wideStr);
                m_ownWide = false;
            }
        }
        else {
            m_wideStr = nullptr;
            m_ownWide = false;
        }
    }

    // 析构函数
    ~DualString() {
        FreeMemory();
    }

    // 复制构造函数
    DualString(const DualString& other) : m_ansiStr(nullptr), m_wideStr(nullptr),
        m_ownAnsi(false), m_ownWide(false), m_converted(false) {
        if (other.m_ansiStr) {
            size_t len = strlen(other.m_ansiStr) + 1;
            m_ansiStr = new char[len];
            strcpy_s(m_ansiStr, len, other.m_ansiStr);
            m_ownAnsi = true;
        }

        if (other.m_wideStr) {
            size_t len = wcslen(other.m_wideStr) + 1;
            m_wideStr = new wchar_t[len];
            wcscpy_s(m_wideStr, len, other.m_wideStr);
            m_ownWide = true;
        }

        m_converted = other.m_converted;
    }

    // 移动构造函数
    DualString(DualString&& other) noexcept : m_ansiStr(other.m_ansiStr), m_wideStr(other.m_wideStr),
        m_ownAnsi(other.m_ownAnsi), m_ownWide(other.m_ownWide),
        m_converted(other.m_converted) {
        other.m_ansiStr = nullptr;
        other.m_wideStr = nullptr;
        other.m_ownAnsi = false;
        other.m_ownWide = false;
        other.m_converted = false;
    }

    // 赋值运算符
    DualString& operator=(const DualString& other) {
        if (this != &other) {
            FreeMemory();

            if (other.m_ansiStr) {
                size_t len = strlen(other.m_ansiStr) + 1;
                m_ansiStr = new char[len];
                strcpy_s(m_ansiStr, len, other.m_ansiStr);
                m_ownAnsi = true;
            }

            if (other.m_wideStr) {
                size_t len = wcslen(other.m_wideStr) + 1;
                m_wideStr = new wchar_t[len];
                wcscpy_s(m_wideStr, len, other.m_wideStr);
                m_ownWide = true;
            }

            m_converted = other.m_converted;
        }
        return *this;
    }

    // 移动赋值运算符
    DualString& operator=(DualString&& other) noexcept {
        if (this != &other) {
            FreeMemory();

            m_ansiStr = other.m_ansiStr;
            m_wideStr = other.m_wideStr;
            m_ownAnsi = other.m_ownAnsi;
            m_ownWide = other.m_ownWide;
            m_converted = other.m_converted;

            other.m_ansiStr = nullptr;
            other.m_wideStr = nullptr;
            other.m_ownAnsi = false;
            other.m_ownWide = false;
            other.m_converted = false;
        }
        return *this;
    }

    // 获取窄字符串，如果没有则尝试转换
    const char* GetAnsi() {
        if (!m_ansiStr && m_wideStr) {
            ConvertWideToAnsi();
        }
        return m_ansiStr;
    }

    // 获取宽字符串，如果没有则尝试转换
    const wchar_t* GetWide() {
        if (!m_wideStr && m_ansiStr) {
            ConvertAnsiToWide();
        }
        return m_wideStr;
    }

    // 重置为新的窄字符串
    void SetAnsi(const char* ansiStr, bool makeCopy = true) {
        FreeMemory();

        if (ansiStr) {
            if (makeCopy) {
                size_t len = strlen(ansiStr) + 1;
                m_ansiStr = new char[len];
                strcpy_s(m_ansiStr, len, ansiStr);
                m_ownAnsi = true;
            }
            else {
                m_ansiStr = const_cast<char*>(ansiStr);
                m_ownAnsi = false;
            }
        }
    }

    // 重置为新的宽字符串
    void SetWide(const wchar_t* wideStr, bool makeCopy = true) {
        FreeMemory();

        if (wideStr) {
            if (makeCopy) {
                size_t len = wcslen(wideStr) + 1;
                m_wideStr = new wchar_t[len];
                wcscpy_s(m_wideStr, len, wideStr);
                m_ownWide = true;
            }
            else {
                m_wideStr = const_cast<wchar_t*>(wideStr);
                m_ownWide = false;
            }
        }
    }

    // 检查是否为空
    bool IsEmpty() const {
        return (m_ansiStr == nullptr && m_wideStr == nullptr);
    }

    // 获取窄字符串长度
    size_t GetAnsiLength() {
        if (!m_ansiStr && m_wideStr) {
            ConvertWideToAnsi();
        }
        return m_ansiStr ? strlen(m_ansiStr) : 0;
    }

    // 获取宽字符串长度
    size_t GetWideLength() {
        if (!m_wideStr && m_ansiStr) {
            ConvertAnsiToWide();
        }
        return m_wideStr ? wcslen(m_wideStr) : 0;
    }

    // 清空字符串
    void Clear() {
        FreeMemory();
    }
};




#endif // STRING_HANDLER_H_
