#ifndef LOG_HELPER_H_
#define LOG_HELPER_H_

#include <windows.h>
#include <Lmcons.h> // UNLEN + GetUserName
#include <tlhelp32.h> // CreateToolhelp32Snapshot()
#include <strsafe.h>

#include "Log.h"

#define ProcessLog LogHelper::ProcessInfoLog

namespace LogHelper {

    void ProcessInfoLog(LPCWSTR pwszCallingFrom)
    {
        LPWSTR pwszBuffer, pwszCommandLine;
        WCHAR wszUsername[UNLEN + 1] = { 0 };
        SYSTEMTIME st = { 0 };
        HANDLE hToolhelpSnapshot;
        PROCESSENTRY32 stProcessEntry = { 0 };
        DWORD dwPcbBuffer = UNLEN, dwBytesWritten = 0, dwProcessId = 0, dwParentProcessId = 0, dwBufSize = 0;
        BOOL bResult = FALSE;

        // Get the command line of the current process
        pwszCommandLine = GetCommandLine();

        // Get the name of the process owner
        GetUserName(wszUsername, &dwPcbBuffer);

        // Get the PID of the current process
        dwProcessId = GetCurrentProcessId();

        // Get the PID of the parent process
        hToolhelpSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        stProcessEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hToolhelpSnapshot, &stProcessEntry)) {
            do {
                if (stProcessEntry.th32ProcessID == dwProcessId) {
                    dwParentProcessId = stProcessEntry.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(hToolhelpSnapshot, &stProcessEntry));
        }
        CloseHandle(hToolhelpSnapshot);

        // Get the current date and time
        GetLocalTime(&st);

        // Prepare the output string and log the result
        dwBufSize = 4096 * sizeof(WCHAR);
        pwszBuffer = (LPWSTR)malloc(dwBufSize);
        if (pwszBuffer)
        {
            StringCchPrintf(pwszBuffer, dwBufSize, L"[%.2u:%.2u:%.2u] - PID=%d - PPID=%d - USER='%s' - CMD='%s' - METHOD='%s'\r\n",
                st.wHour,
                st.wMinute,
                st.wSecond,
                dwProcessId,
                dwParentProcessId,
                wszUsername,
                pwszCommandLine,
                pwszCallingFrom
            );

            LogToFile(L"D:\\LOGS\\RpcEptMapperPoc.log", pwszBuffer);

            free(pwszBuffer);
        }
    }


}


#endif // LOG_HELPER_H_
