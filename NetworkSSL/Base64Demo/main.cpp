#include <Windows.h>
#include <iostream>
#include <string>
#include "../../include/base64/base64.h" // 确保头文件在项目中


// 读取文件到内存的函数
BYTE* ReadFileToMemory(const wchar_t* filePath, DWORD* fileSizeOut) {
    HANDLE hFile = CreateFile(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed. Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    // 获取文件大小
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        std::cerr << "GetFileSizeEx failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return nullptr;
    }

    // 分配内存（+1 用于可选的空终止符）
    BYTE* buffer = (BYTE*)VirtualAlloc(
        NULL,
        fileSize.QuadPart + 1, // 多分配1字节用于字符串终止符
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!buffer) {
        std::cerr << "VirtualAlloc failed." << std::endl;
        CloseHandle(hFile);
        return nullptr;
    }

    // 读取文件内容
    DWORD bytesRead;
    BOOL success = ReadFile(
        hFile,
        buffer,
        (DWORD)fileSize.QuadPart,
        &bytesRead,
        NULL
    );

    if (!success || bytesRead != fileSize.QuadPart) {
        std::cerr << "ReadFile failed. Error: " << GetLastError() << std::endl;
        VirtualFree(buffer, 0, MEM_RELEASE);
        CloseHandle(hFile);
        return nullptr;
    }

    // 添加空终止符（可选，用于文本文件）
    buffer[fileSize.QuadPart] = '\0';

    // 返回文件大小（可选）
    if (fileSizeOut) {
        *fileSizeOut = (DWORD)fileSize.QuadPart;
    }

    CloseHandle(hFile);
    return buffer;
}

// 释放内存的函数
void FreeFileMemory(BYTE* buffer) {
    VirtualFree(buffer, 0, MEM_RELEASE);
}

int main() {
    DWORD fileSize;
    BYTE* data = ReadFileToMemory(L"D:\\NewFrame\\Plugins\\XDRManager\\10.34.11.201_DESKTOP-IBF44OE_1745375113190931400.log", &fileSize);
    if (!data) return 1;

    std::string base64 = base64_encode(data, fileSize);
    std::cout << "Base64 length: " << base64.size() << std::endl;

    // 可选：解码回二进制
    std::string decoded = base64_decode(base64);

    FreeFileMemory(data);
    return 0;
}
