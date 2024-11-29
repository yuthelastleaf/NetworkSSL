#include <windows.h>

#include <winnt.h>
#include <ntstatus.h>
#include <winternl.h>
#include <atlstr.h>

#include <iostream>

#include "../../include/ntalpcapi.h"
// 强制覆盖枚举定义
#ifdef _PROCESSINFOCLASS
#undef _PROCESSINFOCLASS
#endif

#include "../../include/wininfo/ntinfodef.h"


bool IsWindows8Point1OrGreater() {

    DEFAPI(RtlGetVersion);

    bool flag = false;

    // 调用 RtlGetVersion
    RTL_OSVERSIONINFOW versionInfo = { 0 };
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

    if (pfunc_RtlGetVersion(&versionInfo) == 0 /* STATUS_SUCCESS */) {
        if (versionInfo.dwMajorVersion > 6) {
            flag = true;
        }
        else if (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion >= 3) {
            flag = true;
        }
    }

   

    return flag;
}

/**
 * Gets a string stored in a process' parameters structure.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Offset The string to retrieve.
 * \param String A variable which receives a pointer to the requested string. You must free the
 * string using PhDereferenceObject() when you no longer need it.
 *
 * \retval STATUS_INVALID_PARAMETER_2 An invalid value was specified in the Offset parameter.
 */
NTSTATUS PhGetProcessPebString(
    _In_ HANDLE ProcessHandle,
    _In_ PH_PEB_OFFSET Offset,
    _Out_ CString& String
)
{
    DEFAPI(NtQueryInformationProcess);
    DEFAPI(NtReadVirtualMemory);
    DEFAPI(RtlAllocateHeap);
    DEFAPI(RtlFreeHeap);

    NTSTATUS status;

    ULONG bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH;
    

    ULONG offset;

#define PEB_OFFSET_CASE(Enum, Field) \
    case Enum: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS64, Field); break; \
    case Enum | PhpoWow64: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Field); break

    switch (Offset)
    {
        PEB_OFFSET_CASE(PhpoCurrentDirectory, CurrentDirectory);
        PEB_OFFSET_CASE(PhpoDllPath, DllPath);
        PEB_OFFSET_CASE(PhpoImagePathName, ImagePathName);
        PEB_OFFSET_CASE(PhpoCommandLine, CommandLine);
        PEB_OFFSET_CASE(PhpoWindowTitle, WindowTitle);
        PEB_OFFSET_CASE(PhpoDesktopInfo, DesktopInfo);
        PEB_OFFSET_CASE(PhpoShellInfo, ShellInfo);
        PEB_OFFSET_CASE(PhpoRuntimeData, RuntimeData);
    default:
        return STATUS_INVALID_PARAMETER_2;
    }

    if (!(Offset & PhpoWow64))
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID processParameters;
        UNICODE_STRING unicodeString;
        ULONG ReadBytes = 0;
        PEB peb;

        status = pfunc_NtQueryInformationProcess(
            ProcessHandle,
            wininfo::ProcessBasicInformation,
            &basicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            &ReadBytes
        );

        // Get the PEB address.
        if (!NT_SUCCESS(status))
            return status;

        int offset_para = FIELD_OFFSET(PEB, ProcessParameters);

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
        )))
            return status;

        ReadProcessMemory(ProcessHandle, basicInfo.PebBaseAddress, &peb, sizeof(PEB), NULL);
        RTL_USER_PROCESS_PARAMETERS ppara = { 0 };

        // Read the string structure.
        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, offset),
            &unicodeString,
            sizeof(UNICODE_STRING),
            NULL
        )))
            return status;

        ReadProcessMemory(ProcessHandle, peb.ProcessParameters, &ppara, sizeof(RTL_USER_PROCESS_PARAMETERS), NULL);

        if (unicodeString.Length == 0)
        {
            return status;
        }

        WCHAR* string = (PWCHAR)pfunc_RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * (unicodeString.Length + 1));

        // Read the string contents.
        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            unicodeString.Buffer,
            // string->Buffer,
            string,
            unicodeString.Length,
            NULL
        )))
        {
            pfunc_RtlFreeHeap(GetProcessHeap(), 0, string);
            return status;
        }
        wprintf_s(L"the alloc string not wow64 is : %s\n", string);
        String = string;
        pfunc_RtlFreeHeap(GetProcessHeap(), 0, string);
    }
    else
    {
        LONG_PTR wow64;
        PVOID peb32;
        ULONG processParameters32;
        UNICODE_STRING32 unicodeString32;

        status = pfunc_NtQueryInformationProcess(
            ProcessHandle,
            wininfo::ProcessWow64Information,
            &wow64,
            sizeof(ULONG_PTR),
            NULL
        );
        if (!NT_SUCCESS(status)) {
            return status;
        }
        else {
            peb32 = (PVOID)wow64;
        }

        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
        ))) {
            return status;
        }

        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, offset),
            &unicodeString32,
            sizeof(UNICODE_STRING32),
            NULL
        ))) {
            return status;
        }

        if (unicodeString32.Length == 0)
        {
            return status;
        }

        WCHAR* string = (PWCHAR)pfunc_RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * (unicodeString32.Length + 1));
        // RtlZeroMemory(string, unicodeString32.Length + 1);

        // Read the string contents.
        if (!NT_SUCCESS(status = pfunc_NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(unicodeString32.Buffer),
            string,
            unicodeString32.Length,
            NULL
        )))
        {
            pfunc_RtlFreeHeap(GetProcessHeap(), 0, string);
            return status;
        }
        wprintf_s(L"the alloc string is : %s\n", string);
        String = string;
        pfunc_RtlFreeHeap(GetProcessHeap(), 0, string);
    }

    return status;
}

/**
 * Gets a process' command line.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION. Before Windows 8.1, the handle must also have PROCESS_VM_READ
 * access.
 * \param CommandLine A variable which receives a pointer to a string containing the command line. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ CString& CommandLine
)
{

    DEFAPI(NtQueryInformationProcess);
    DEFAPI(RtlCopyUnicodeString);
    DEFAPI(RtlAllocateHeap);
    DEFAPI(RtlFreeHeap);
    if (!pfunc_NtQueryInformationProcess) {
        return -1;
    }

    NTSTATUS status;
    PUNICODE_STRING buffer;
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH;
    buffer = (PUNICODE_STRING)pfunc_RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferLength);

    status = pfunc_NtQueryInformationProcess(
        ProcessHandle,
        wininfo::ProcessCommandLineInformation,
        buffer,
        bufferLength,
        &returnLength
    );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        pfunc_RtlFreeHeap(GetProcessHeap(), 0, buffer);
        bufferLength = returnLength;
        buffer = (PUNICODE_STRING)pfunc_RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferLength);

        status = pfunc_NtQueryInformationProcess(
            ProcessHandle,
            wininfo::ProcessCommandLineInformation,
            buffer,
            bufferLength,
            &returnLength
        );
    }

    if (NT_SUCCESS(status))
    {
        CommandLine = buffer->Buffer;
    }

    pfunc_RtlFreeHeap(GetProcessHeap(), 0, buffer);

    return status;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <PID>" << std::endl;
        return 1;
    }

    setlocale(LC_ALL, "chs");
    _wsetlocale(LC_ALL, L"chs");

    // MessageBox(NULL, L"test", L"test", MB_OK);

    // 将命令行参数转换为 DWORD
    DWORD pid = std::strtoul(argv[1], nullptr, 10);
    if (pid == 0) {
        std::cerr << "Invalid PID: " << argv[1] << std::endl;
        return 1;
    }
    CString str_cmd;
    // cmd = (PUNICODE_STRING)pfunc_RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UNICODE_STRING));

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    // HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        return 0;
    }
    // PhGetProcessPebString(hProcess, PhpoCommandLine, str_cmd);

    if (IsWindows8Point1OrGreater()) {
        PhGetProcessCommandLine(hProcess, str_cmd);
    }
    else {
        PhGetProcessPebString(hProcess, PhpoCommandLine, str_cmd);
    }
    wprintf_s(L"the commandline is : %s\n", str_cmd.GetBuffer());
    str_cmd.ReleaseBuffer();

    // system("pause");

    return 0;
}
