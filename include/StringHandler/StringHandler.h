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

}




#endif // STRING_HANDLER_H_
