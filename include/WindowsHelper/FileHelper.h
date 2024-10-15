#ifndef FILE_HELPER_H_
#define FILE_HELPER_H_

#include <Windows.h>
#include <atlstr.h>

#include "../StringHandler/StringHandler.h"

class CAnsiFileRWNoShare
{
public:
	CAnsiFileRWNoShare()
		: stream(nullptr)
	{

	}

	CAnsiFileRWNoShare(const char path[]) {
		stream = nullptr;
		OpenFile(path);
	}

	~CAnsiFileRWNoShare() {
		if (stream) {
			fclose(stream);
		}
	}

	bool OpenFile(const char path[]) {
		errno_t err = fopen_s(&stream, path, "a+");
		if (err) {
			stream = nullptr;
		}
		return !err;
	}

	size_t Write(const char Cont[]) {
		fseek(stream, 0L, SEEK_END);
		return fwrite(Cont, sizeof(char), sizeof(char) * strlen(Cont), stream);
	}

private:
	FILE* stream;
};


class CFileHelper : public CStringHandler::StrSplitHandler
{
public:
	CFileHelper() 
		: file_data_(nullptr)
		, file_len_(0)
		, bin_data_(nullptr)
		, bin_len_(0)
	{}

	CFileHelper(char path[])
		: file_data_(nullptr)
		, file_len_(0)
	{
		bin_data_ = nullptr;
		bin_len_ = 0;
		ReadBinFile(path);
	}

	~CFileHelper() {
		if (file_data_) {
			delete file_data_;
			file_data_ = nullptr;
		}
		if (bin_data_) {
			free(bin_data_);
			bin_data_ = nullptr;
		}
	}

public:
	bool ReadTestData(CString test_file_name) {
		BOOL flag = false;

		do {
#ifdef UNICODE
			wchar_t buf_file_path[MAX_PATH] = {};
#else
			char buf_file_path[MAX_PATH] = {};
#endif

			CString str_file_path;
			if (!GetCurrentDirectory(MAX_PATH, buf_file_path)) {
				break;
			}
			str_file_path = buf_file_path;
#ifdef UNICODE
			HANDLE test_file = CreateFile(str_file_path + L"\\" + test_file_name,
				GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
#else
			HANDLE test_file = CreateFile(str_file_path + "\\" + test_file_name,
				GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
#endif
			if (test_file == INVALID_HANDLE_VALUE) {
				break;
			}
			file_len_ = GetFileSize(test_file, NULL);
			if (!(file_len_ ^ INVALID_FILE_SIZE)) {
				CloseHandle(test_file);
				break;
			}

			if (file_len_ > 65535) {
				break;
			}

			if (file_data_) {
				delete[] file_data_;
			}



			char* tmp_file_data = new char[file_len_ + 1];
			memset(tmp_file_data, 0, sizeof(char) * (file_len_ + 1));

			if (!ReadFile(test_file, tmp_file_data, file_len_, NULL, NULL)) {
				CloseHandle(test_file);
				break;
			}

#ifdef UNICODE
			CStringHandler::Ansi2WChar(tmp_file_data, file_data_);
			delete[] tmp_file_data;
			if (!file_data_) {
				break;
			}
#else
			file_data = tmp_file_data;
#endif

			CloseHandle(test_file);

			flag = true;

		} while (0);

		return flag;
	}

	// 读取二进制文件数据
	bool ReadBinFile(const char path[]) {
		FILE* stream;
		errno_t err = fopen_s(&stream, path, "r");
		if (stream != NULL) {
			fseek(stream, 0L, SEEK_END);
			bin_len_ = ftell(stream);
			fseek(stream, 0L, SEEK_SET);
			if (bin_data_ != nullptr) {
				free(bin_data_);
			}
			bin_data_ = static_cast<char*>(malloc(sizeof(char) * bin_len_));
			if (bin_data_) {
				err = fread(bin_data_, sizeof(char), bin_len_, stream);
			}
			fclose(stream);
		}

		return err;
	}

	template<typename T>
	bool TransToNumList(std::vector<T>& vt, CString test_file_name) {
		bool flag = false;
		do {
			if (!ReadTestData(test_file_name)) {
				break;
			}

			if (!StrSplit(file_data_, file_len_)) {
				break;
			}

			_TransToNumList(vt);
			flag = true;
		} while (0);

		return flag;
	}

	DWORD GetBinLen() {
		return bin_len_;
	}

	char* GetBinData() {
		return bin_data_;
	}

	char GetBinDataAt(int i) {
		char res = '\0';
		if (i < bin_len_) {
			res = bin_data_[i];
		}
		return res;
	}

protected:
#ifdef UNICODE
	wchar_t* file_data_;
#else
	char* file_data_;
#endif
	char* bin_data_;
	DWORD bin_len_;
	DWORD file_len_;
};


#endif // FILE_HELPER_H_

