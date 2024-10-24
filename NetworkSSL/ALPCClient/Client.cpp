#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include "../../include/ntalpcapi.h"
#include "../../include/CJSON/CJSONHanler.h"

#define MSG_LEN 0x100

LPVOID CreateMsgMem(PPORT_MESSAGE PortMessage, SIZE_T MessageSize, LPVOID Message)
{
    /*
        It's important to understand that after the PORT_MESSAGE struct is the message data
    */
    LPVOID lpMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MessageSize + sizeof(PORT_MESSAGE));
    memmove(lpMem, PortMessage, sizeof(PORT_MESSAGE));
    memmove((BYTE*)lpMem + sizeof(PORT_MESSAGE), Message, MessageSize);
    return(lpMem);
}

void main()
{
    MessageBox(NULL, L"test", L"test", MB_OK);

    CStringHandler::InitChinese();

    UNICODE_STRING  usPort = { 0 };
    PORT_MESSAGE    pmSend;
    PORT_MESSAGE    pmReceive;
    NTSTATUS        ntRet;
    BOOLEAN         bBreak;
    SIZE_T          nLen;
    HANDLE          hPort = NULL;
    LPVOID          lpMem; 
    char            szInput[MSG_LEN];

    DEFAPI(RtlInitUnicodeString);
    DEFAPI(NtAlpcConnectPort);
    DEFAPI(NtAlpcSendWaitReceivePort);


    printf("ALPC-Example Client\n");
    pfunc_RtlInitUnicodeString(&usPort, L"\\RPC Control\\NameOfPort");
    RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
    pmSend.u1.s1.DataLength = MSG_LEN;
    pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(pmSend);
    lpMem = CreateMsgMem(&pmSend, MSG_LEN, (LPVOID)L"Hello World!");
    ntRet = pfunc_NtAlpcConnectPort(&hPort, &usPort, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    printf("[i] NtAlpcConnectPort: 0x%X\n", ntRet);
    if (!ntRet)
    {
        printf("[i] type 'exit' to disconnect from the server\n");
        bBreak = TRUE;
        while (bBreak)
        {
            RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
            RtlSecureZeroMemory(&szInput, sizeof(szInput));
            printf("[.] Enter Message > ");
            fgets(szInput, MSG_LEN, stdin);

            CJSONHandler json;
            json[L"name"] = "test";
            json[L"name"] = "bushiba";
            json[L"info"] = "hhh";
            json[L"newobj"][L"qiantao"][L"lipu"] = L"Ƕ�׶���";
            json[L"name"][L"�滻"] = "replace";
            std::shared_ptr<char> json_string = json.GetJsonString();
            
            // ���Ҳ��Ƴ����з� '\n'
            size_t len = strlen(szInput);
            if (len > 0 && szInput[len - 1] == '\n') {
                szInput[len - 1] = '\0';  // �ÿ��ַ��滻 '\n'
            }

            if (json_string) {
                pmSend.u1.s1.DataLength = strlen(json_string.get());
                pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(PORT_MESSAGE);
                lpMem = CreateMsgMem(&pmSend, pmSend.u1.s1.DataLength, json_string.get());

                ntRet = pfunc_NtAlpcSendWaitReceivePort(hPort, 0, (PPORT_MESSAGE)lpMem, NULL, NULL, NULL, NULL, NULL);

                HeapFree(GetProcessHeap(), 0, lpMem);
            }

            if (szInput) {
                RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
                pmSend.u1.s1.DataLength = strlen(szInput);
                pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(PORT_MESSAGE);
                lpMem = CreateMsgMem(&pmSend, pmSend.u1.s1.DataLength, szInput);

                ntRet = pfunc_NtAlpcSendWaitReceivePort(hPort, 0, (PPORT_MESSAGE)lpMem, NULL, NULL, NULL, NULL, NULL);

                HeapFree(GetProcessHeap(), 0, lpMem);
            }

            printf("[i] NtAlpcSendWaitReceivePort: 0x%X\n", ntRet);
            
        }
    }
    getchar();
    return;
}
