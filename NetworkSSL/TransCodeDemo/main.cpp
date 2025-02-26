#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <atlstr.h>

#include "../../include/Alpc/alpc_util.h"


int wmain(int argc, wchar_t* argv[]) {
    bool flag = false;
    if (argc >= 3) {
        
        CString str_param = argv[1];
        if (str_param == L"-e") {
            CJSONHandler json;
            json[L"type"] = "encode";
            json[L"path"] = argv[2];

            flag = AlpcMng::getInstance().notify_msg(L"yjnmini", json);
        }
        else if (str_param == L"-d") {
            CJSONHandler json;
            json[L"type"] = "decode";
            json[L"path"] = argv[2];

            flag = AlpcMng::getInstance().notify_msg(L"yjnmini", json);
        }
    }
    printf("post result is : %d\n", flag);

    return 0;
}
