#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include "../../include/ntalpcapi.h"

#include "../../include/CJSON/CJSONHanler.h"

#include "../../include/Alpc/alpc_util.h"

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


    AlpcHandler::getInstance().registerTask(L"test", [](std::shared_ptr<void> ctx) {
        // 尝试将 void 指针转回 AlpcHandlerCtx
        auto alpcContext = std::static_pointer_cast<AlpcHandlerCtx>(ctx);
        printf("client data: \n%s\n", alpcContext->json_->GetJsonString().get());
        if ((*alpcContext->json_)[L"reply"].GetInt() == 1) {
            (*alpcContext->json_)[L"replymsg"] = L"server_reply_msg";
            AlpcConn* alpc_con = (AlpcConn*)alpcContext->alpc_;
            alpc_con->post_msg(*alpcContext->json_, alpcContext->msg_id_);
        }
        });

    AlpcMng::getInstance().run_server(L"testserver");

    // printf("[!] Shuting down server\n");
    getchar();
    return;
}
