#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include "../../include/ntalpcapi.h"
#include "../../include/CJSON/CJSONHanler.h"

#include "../../include/Alpc/alpc_util.h"

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

LPVOID AllocMsgMem(SIZE_T Size)
{
    return (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size + sizeof(PORT_MESSAGE)));
}

void main_old()
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
    pfunc_RtlInitUnicodeString(&usPort, L"\\RPC Control\\YJN");
    RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
    pmSend.u1.s1.DataLength = MSG_LEN;
    pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(pmSend);
    lpMem = CreateMsgMem(&pmSend, MSG_LEN, (LPVOID)L"Hello World!");
    ntRet = pfunc_NtAlpcConnectPort(&hPort, &usPort, NULL, NULL, ALPC_MSGFLG_SYNC_REQUEST, NULL, NULL, NULL, NULL, NULL, NULL);

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
            json[L"newobj"][L"qiantao"][L"lipu"] = L"Ç¶Ì×¶ÔÏó";
            json[L"name"][L"Ìæ»»"] = "replace";
            std::shared_ptr<char> json_string = json.GetJsonString();
            
            // ²éÕÒ²¢ÒÆ³ý»»ÐÐ·û '\n'
            size_t len = strlen(szInput);
            if (len > 0 && szInput[len - 1] == '\n') {
                szInput[len - 1] = '\0';  // ÓÃ¿Õ×Ö·ûÌæ»» '\n'
            }

            if (json_string) {
                pmSend.u1.s1.DataLength = strlen(json_string.get());
                pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(PORT_MESSAGE);
                lpMem = CreateMsgMem(&pmSend, pmSend.u1.s1.DataLength, json_string.get());

                /*LPVOID recv_mem = NULL;
                SIZE_T msg_size = MAX_MSG_LEN + sizeof(PORT_MESSAGE);
                recv_mem = AllocMsgMem(MAX_MSG_LEN);*/
                PORT_MESSAGE receiveMessage;
                SIZE_T receiveMessageLength;


                ntRet = pfunc_NtAlpcSendWaitReceivePort(hPort, 0, (PPORT_MESSAGE)lpMem, NULL, (PPORT_MESSAGE)&receiveMessage, &receiveMessageLength, NULL, NULL);


                printf("[i] server Data: ");

                char* recv_data = ((char*)&receiveMessage + sizeof(PORT_MESSAGE));
                printf("%s\n", recv_data);
                
                HeapFree(GetProcessHeap(), 0, lpMem);
                // HeapFree(GetProcessHeap(), 0, recv_mem);
            }

            /*if (szInput) {
                RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
                pmSend.u1.s1.DataLength = strlen(szInput);
                pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(PORT_MESSAGE);
                lpMem = CreateMsgMem(&pmSend, pmSend.u1.s1.DataLength, szInput);

                ntRet = pfunc_NtAlpcSendWaitReceivePort(hPort, 0, (PPORT_MESSAGE)lpMem, NULL, NULL, NULL, NULL, NULL);

                HeapFree(GetProcessHeap(), 0, lpMem);
            }*/

            printf("[i] NtAlpcSendWaitReceivePort: 0x%X\n", ntRet);
            
        }
    }
    getchar();
    return;
}

int main() {
    char            szInput[POST_LEN];
    while (true) {

        /*printf("[.] Enter Message > ");
        RtlSecureZeroMemory(&szInput, sizeof(szInput));
        fgets(szInput, POST_LEN, stdin);*/
        char            szInput[POST_LEN] = "stest123,456";

        CJSONHandler json;
        json[L"reply"] = 1;
        json[L"type"] = L"test";
        json[L"name"] = "test";
        json[L"name"] = "bushiba";
        json[L"info"] = szInput;
        json[L"newobj"][L"qiantao"][L"lipu"] = L"Ç¶Ì×¶ÔÏó";
        json[L"name"][L"Ìæ»»"] = "replace";
        

        // ²éÕÒ²¢ÒÆ³ý»»ÐÐ·û '\n'
        size_t len = strlen(szInput);
        if (len > 0 && szInput[len - 1] == '\n') {
            szInput[len - 1] = '\0';  // ÓÃ¿Õ×Ö·ûÌæ»» '\n'
        }

        if (szInput[0] == 's') {
            AlpcMng::getInstance().notify_msg(L"testserver", json, false);

            printf("[i] server Data: ");
            printf("%s\n", json.GetJsonString().get());

        }
        else {
            AlpcMng::getInstance().notify_msg(L"testserver", json);
        }
        Sleep(5000);
    }

    return 0;
}
