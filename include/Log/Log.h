#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <direct.h>

#include "../StringHandler/StringHandler.h"

#define LogToCmd		CLog::CmdLog
#define LogToFile		CLog::FileLog
#define LogToOutput

namespace CLog {
	char glog_path[256] = { 0 };

	// 设置日志目录
	void SetLogPath(const char* log_path) {
		memcpy(glog_path, log_path, sizeof(log_path));
	}

	void SetLogPath(const wchar_t* log_path) {
		char* ch_log_path = nullptr;
		CStringHandler::WChar2Ansi(log_path, ch_log_path);
		SetLogPath(ch_log_path);
		delete[] ch_log_path;
	}

	// 获取时间字符串
	void GetTime(char*& str_time) {
		time_t now = time(0);
		tm ltm;
		localtime_s(&ltm, &now);

		str_time = new char[64];
		memset(str_time, 0, sizeof(char) * 64);
		sprintf_s(str_time, 64, "[%d年%d月%d日-%d时%d分%d秒] :", 1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday,
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
	}


	void GetTime(wchar_t*& str_time) {
		time_t now = time(0);
		tm ltm;

		localtime_s(&ltm, &now);

		str_time = new wchar_t[64];
		memset(str_time, 0, sizeof(wchar_t) * 64);
		swprintf_s(str_time, 64, L"[%d年%d月%d日-%d时%d分%d秒] :", 1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday,
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
	}

	// 控制台打印日志
	void CmdLog(const char* log) {
		char* str_time = nullptr;
		GetTime(str_time);
		printf_s("%s%s\n", str_time, log);
		delete[] str_time;
	}

	void CmdLog(const wchar_t* log) {
		wchar_t* str_time = nullptr;
		GetTime(str_time);
		wprintf_s(L"%s%s\n", str_time, log);
		delete[] str_time;
	}

	// 写入文件日志
	void FileLog(const char* log, const char* log_path = nullptr) {
		if (strlen(glog_path) == 0 && !log_path) {
			_getcwd(glog_path, 256);
			strcat_s(glog_path, 256, "\\CLog.log");
		}

		FILE* stream;
		if (!log_path) {
			errno_t err = fopen_s(&stream, glog_path, "a");
		}
		else {
			errno_t err = fopen_s(&stream, log_path, "a");
		}
		if (stream != NULL) {
			char* str_time = nullptr;
			GetTime(str_time);
			fwrite(str_time, sizeof(char), strlen(str_time), stream);
			fwrite(log, sizeof(char), strlen(log), stream);
			fwrite("\r\n", 1, 2, stream);
			fclose(stream);
			delete[] str_time;
		}
	}

	void FileLog(const wchar_t* log, const wchar_t* log_path = nullptr) {
		char* ch_log = nullptr;
		char* ch_log_path = nullptr;

		CStringHandler::WChar2Ansi(log, ch_log);

		if (log_path) {
			CStringHandler::WChar2Ansi(log_path, ch_log_path);
		}

		FileLog(ch_log, ch_log_path);
		delete[] ch_log;
		delete[] ch_log_path;
	}

};


#endif // LOG_H_
