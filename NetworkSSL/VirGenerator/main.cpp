#include <windows.h>
#include <iostream>
#include <fstream>

#include "resource.h"

#include "PEGenerator.h"

//int wmain(int argc, wchar_t* argv[]) {
//
//    CPEGenerator pe_gen;
//    pe_gen.ParseParams(argc, argv);
//
//    return 0;
//}

#ifdef _WINCMD

int wmain(int argc, wchar_t* argv[]) {

    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    return 0;
}

#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ��ȡ���ַ������в���
    LPWSTR cmdLine = GetCommandLineW();

    // ���������ַ�������Ϊ `argc` �� `argv` ���
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

    if (argv == NULL) {
        return 1;
    }
    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    // �ͷŽ�����Ĳ����ڴ�
    LocalFree(argv);

    return 0;
}
#endif
