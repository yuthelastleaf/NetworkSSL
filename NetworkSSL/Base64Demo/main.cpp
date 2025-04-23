#include <Windows.h>
#include <iostream>
#include <string>
#include "../../include/base64/base64.h" // ȷ��ͷ�ļ�����Ŀ��


// ��ȡ�ļ����ڴ�ĺ���
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

    // ��ȡ�ļ���С
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        std::cerr << "GetFileSizeEx failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return nullptr;
    }

    // �����ڴ棨+1 ���ڿ�ѡ�Ŀ���ֹ����
    BYTE* buffer = (BYTE*)VirtualAlloc(
        NULL,
        fileSize.QuadPart + 1, // �����1�ֽ������ַ�����ֹ��
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!buffer) {
        std::cerr << "VirtualAlloc failed." << std::endl;
        CloseHandle(hFile);
        return nullptr;
    }

    // ��ȡ�ļ�����
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

    // ��ӿ���ֹ������ѡ�������ı��ļ���
    buffer[fileSize.QuadPart] = '\0';

    // �����ļ���С����ѡ��
    if (fileSizeOut) {
        *fileSizeOut = (DWORD)fileSize.QuadPart;
    }

    CloseHandle(hFile);
    return buffer;
}

// �ͷ��ڴ�ĺ���
void FreeFileMemory(BYTE* buffer) {
    VirtualFree(buffer, 0, MEM_RELEASE);
}

int main() {
    DWORD fileSize;
    BYTE* data = ReadFileToMemory(L"D:\\NewFrame\\Plugins\\XDRManager\\10.34.11.201_DESKTOP-IBF44OE_1745375113190931400.log", &fileSize);
    if (!data) return 1;

    std::string base64 = base64_encode(data, fileSize);
    std::cout << "Base64 length: " << base64.size() << std::endl;

    // ��ѡ������ض�����
    std::string decoded = base64_decode(base64);

    FreeFileMemory(data);
    return 0;
}
