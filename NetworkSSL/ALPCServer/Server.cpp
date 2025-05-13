#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <future>
#include "../../include/ntalpcapi.h"

#include "../../include/CJSON/CJSONHanler.h"

#include "../../include/Alpc/alpc_util.h"
#include "../../include/SingletonTools/AsyncTaskManager.h"

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
    /*
        It's important to understand that after the PORT_MESSAGE struct is the message data		
    */
    return(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size + sizeof(PORT_MESSAGE)));
}

DWORD CreatePortAndListen(LPVOID PortName)
{
    ALPC_PORT_ATTRIBUTES    serverPortAttr;
    OBJECT_ATTRIBUTES       objPort;
    UNICODE_STRING          usPortName;
    PORT_MESSAGE            pmRequest;
    PORT_MESSAGE            pmReceive;
    NTSTATUS                ntRet;
    BOOLEAN                 bBreak;
    HANDLE                  hConnectedPort;
    HANDLE                  hPort;
    SIZE_T                  nLen;
    LPVOID                  lpMem;
    BYTE                    bTemp;


    DEFAPI(RtlInitUnicodeString);
    DEFAPI(NtAlpcCreatePort);
    DEFAPI(NtAlpcConnectPort);
    DEFAPI(NtAlpcSendWaitReceivePort);
    DEFAPI(NtAlpcAcceptConnectPort);
    DEFAPI(NtAlpcDisconnectPort);

	
    pfunc_RtlInitUnicodeString(&usPortName, (LPCWSTR)PortName);
    InitializeObjectAttributes(&objPort, &usPortName, 0, 0, 0);
    RtlSecureZeroMemory(&serverPortAttr, sizeof(serverPortAttr));
    serverPortAttr.MaxMessageLength = MAX_MSG_LEN; // For ALPC this can be max of 64KB

    ntRet = pfunc_NtAlpcCreatePort(&hPort, &objPort, &serverPortAttr);
    printf("[i] NtAlpcCreatePort: 0x%X\n", ntRet);
    if (!ntRet)
    {
        // nLen = sizeof(pmReceive);
        nLen = MAX_MSG_LEN;
        ntRet = pfunc_NtAlpcSendWaitReceivePort(hPort, 0, NULL, NULL, &pmReceive, &nLen, NULL, NULL);
        if (!ntRet)
        {
            RtlSecureZeroMemory(&pmRequest, sizeof(pmRequest));
            pmRequest.MessageId = pmReceive.MessageId;
            pmRequest.u1.s1.DataLength = 0x0;
            pmRequest.u1.s1.TotalLength = pmRequest.u1.s1.DataLength + sizeof(PORT_MESSAGE);
            ntRet = pfunc_NtAlpcAcceptConnectPort(&hConnectedPort, hPort, 0, NULL, NULL, NULL, &pmRequest, NULL, TRUE); // 0
            printf("[i] NtAlpcAcceptConnectPort: 0x%X\n", ntRet);

            LARGE_INTEGER timeout;
            timeout.QuadPart = -50000000LL;  // 500毫秒，以100纳秒为单位
            if (!ntRet)
            {
                bBreak = TRUE;
                while (bBreak)
                {	
                    nLen = MAX_MSG_LEN;
                    lpMem = AllocMsgMem(nLen);
                    pfunc_NtAlpcSendWaitReceivePort(hPort, 0, NULL, NULL, (PPORT_MESSAGE)lpMem, &nLen, NULL, &timeout);


                    if (ntRet == STATUS_TIMEOUT) {
                        // 超时后可以执行其他检测连接状态的逻辑，例如心跳检查
                        // 如果需要，也可以继续循环等待
                        printf("[i] Connection timed out, checking connection status...\n");
                        continue;
                    }
                    else if (!NT_SUCCESS(ntRet)) {
                        // 出现错误时，处理断开连接逻辑
                        printf("[!] Connection lost or receive error: 0x%X\n", ntRet);
                        HeapFree(GetProcessHeap(), 0, lpMem);
                        pfunc_NtAlpcDisconnectPort(hPort, 0);
                        CloseHandle(hConnectedPort);

                        ntRet = pfunc_NtAlpcAcceptConnectPort(&hConnectedPort, hPort, 0, NULL, NULL, NULL, &pmRequest, NULL, TRUE); // 0
                        printf("[i] NtAlpcAcceptConnectPort: 0x%X\n", ntRet);
                        if (!ntRet) {
                            ExitThread(0);
                            break;
                        }
                        continue;
                    }

                    pmReceive = *(PORT_MESSAGE*)lpMem;
                    if (!strcmp((char*)lpMem + sizeof(PORT_MESSAGE), "exit\n"))
                    {
                        printf("[i] Received 'exit' command\n");
                        HeapFree(GetProcessHeap(), 0, lpMem);
                        ntRet = pfunc_NtAlpcDisconnectPort(hPort, 0);
                        printf("[i] NtAlpcDisconnectPort: %X\n", ntRet);
                        CloseHandle(hConnectedPort);
                        CloseHandle(hPort);
                        ExitThread(0);
                    }
                    else
                    {
                        printf("[i] Received Data: ");
                        /*for (int i = 0; i <= pmReceive.u1.s1.DataLength; i++)
                        {
                            bTemp = *(BYTE*)((BYTE*)lpMem + i + sizeof(PORT_MESSAGE));
                            printf("0x%X ", bTemp);
                        }*/

                        char* data = ((char*)lpMem + sizeof(PORT_MESSAGE));
                        printf("%s\n", data);

                        if (strlen(data) <= 0) {
                            HeapFree(GetProcessHeap(), 0, lpMem);
                            continue;
                        }

                        
                        CJSONHandler json(cJSON_Parse(data), nullptr);
                        json[L"name"][L"result"] = "mytest";
                        std::shared_ptr<char> pdata = json.GetJsonString();

                        PORT_MESSAGE    pmSend;
                        RtlSecureZeroMemory(&pmSend, sizeof(pmSend));
                        pmSend.u1.s1.DataLength = strlen(pdata.get());
                        pmSend.u1.s1.TotalLength = pmSend.u1.s1.DataLength + sizeof(PORT_MESSAGE);
                        LPVOID precv = CreateMsgMem(&pmSend, pmSend.u1.s1.DataLength, pdata.get());

                        ntRet = pfunc_NtAlpcSendWaitReceivePort(hConnectedPort, 0, (PPORT_MESSAGE)precv, NULL, NULL, NULL, NULL, &timeout);
                        

                        HeapFree(GetProcessHeap(), 0, lpMem);
                        HeapFree(GetProcessHeap(), 0, precv);
                    }
                }
            }
        }
    }
    ExitThread(0);
    return 0;
}

void main()
{
    HANDLE hThread;

    printf("[i] ALPC-Example Server\n");
    LPCWSTR port_name = L"\\RPC Control\\NameOfPort";
    /*hThread = CreateThread(NULL, 0, &CreatePortAndListen, (LPVOID)port_name, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);*/


    AlpcHandler::getInstance().registerTask(L"event", [](std::shared_ptr<void> ctx) {
        auto alpcContext = std::static_pointer_cast<AlpcHandlerCtx>(ctx);
        AsyncTaskManager::GetInstance().AddTask([](std::shared_ptr<void> ctx) {
            auto alpcContext = std::static_pointer_cast<AlpcHandlerCtx>(ctx);
            printf("event data: 0x%X\n%s\n", &alpcContext->json_, alpcContext->json_->GetJsonString().get());
            // DualString full_path((*alpcContext->json_)["FullImage"].GetString());
            //DualString parent_full_path((*alpcContext->json_)["ParentFullImage"].GetString());
            /*printf("full path data \n full:%s \n parent : %s\n", full_path.GetAnsi(), parent_full_path.GetAnsi());
            wprintf(L"transform full path data \n full:%s \n parent : %s\n", CommUtil::NtPathToDosPath(full_path.GetWide()).c_str(),
                CommUtil::NtPathToDosPath(parent_full_path.GetWide()).c_str());*/

            }, ctx);
        if ((*alpcContext->json_)[L"reply"].GetInt() == 1) {
            if(strcmp((*alpcContext->json_)[L"Image"].GetString(), "notepad.exe") == 0) {
                (*alpcContext->json_)[L"result"] = -1;
            }
            else {
                (*alpcContext->json_)[L"result"] = 1;
            }
            AlpcConn::getInstance().post_msg(*alpcContext->json_, alpcContext->alpc_, alpcContext->msg_id_);
        }
        });

    AlpcHandler::getInstance().registerTask(L"connect", [](std::shared_ptr<void> ctx) {
        // 尝试将 void 指针转回 AlpcHandlerCtx
        // auto alpcContext = std::static_pointer_cast<PostMsg>(ctx);
        // printf("event data: \n%s\n", alpcContext->json_->GetJsonString().get());
        AsyncTaskManager::GetInstance().AddTask([](std::shared_ptr<void> ctx) {
            auto alpcContext = std::static_pointer_cast<PostMsg>(ctx);
            printf("connect client name: %s\n", alpcContext->GetDataMem());
            }, ctx);
        });

    /*while (true) {

        
        AlpcMng::getInstance().stop_server();

        Sleep(1000);
    }*/
    AlpcConn::getInstance().start_server(L"yjnclient");
    // printf("[!] Shuting down server\n");

    char            szInput[POST_LEN];
    while (true) {

        printf("[.] Enter Message > ");
        RtlSecureZeroMemory(&szInput, sizeof(szInput));
        fgets(szInput, POST_LEN, stdin);
        // char            szInput[POST_LEN] = "stest123,456";

        size_t len = strlen(szInput);
        if (len > 0 && szInput[len - 1] == '\n') {
            szInput[len - 1] = '\0';  // 用空字符替换 '\n'
        }

        CJSONHandler json;
        json[L"reply"] = 1;
        json[L"type"] = "event";
        json[L"name"] = "test";
        json[L"name"] = "bushiba";
        json[L"info"]["notify"] = szInput;
        json[L"newobj"][L"qiantao"][L"lipu"] = L"嵌套对象";
        json[L"name"][L"替换"] = "replace";

        if (szInput[0] == 's') {
            AlpcConn::getInstance().notify_msg("yjnalpc", json, false);

            printf("[i] server Data: ");
            printf("%s\n", json.GetJsonString().get());

        }
        else {
            AlpcConn::getInstance().notify_msg("yjnalpc", json);
        }
    }

    getchar();
    return;
}
