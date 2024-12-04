#pragma once

#ifdef __cplusplus
#include <Windows.h>
#include <winternl.h>

#include <thread>

#include "../ntalpcapi.h"
#include "../StringHandler/StringHandler.h"

#define POST_LEN 0x400

class PostMsg
{
public:

    PostMsg(const char* strmsg, ULONG msgid = 0, unsigned int data_len = POST_LEN) {
        init_portmsg(data_len, msgid);
        msg_ = CreateMsgMem(&port_msg_, POST_LEN, (LPVOID)strmsg);
    }

    ~PostMsg() {

        if (msg_) {
            ReleaseMsgMem(msg_);
        }

    }

public:
    LPVOID GetMsgMem() {
        return msg_;
    }

    LPVOID GetDataMem() {
        return ((char*)msg_ + sizeof(PORT_MESSAGE));
    }

    bool update_msg() {
        bool flag = false;
        if (msg_) {
            memcpy(&port_msg_, msg_, sizeof(PORT_MESSAGE));

            flag = true;
        }
        return flag;
    }

private:
    inline void init_portmsg(unsigned int data_len, ULONG msgid) {
        RtlSecureZeroMemory(&port_msg_, sizeof(port_msg_));
        port_msg_.MessageId = msgid;
        port_msg_.u1.s1.DataLength = data_len;
        port_msg_.u1.s1.TotalLength = port_msg_.u1.s1.DataLength + sizeof(port_msg_);
    }

    LPVOID CreateMsgMem(PPORT_MESSAGE PortMessage, SIZE_T MessageSize, LPVOID Message)
    {
        LPVOID lpMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MessageSize + sizeof(PORT_MESSAGE));
        memmove(lpMem, PortMessage, sizeof(PORT_MESSAGE));
        if (Message) {
            memmove((BYTE*)lpMem + sizeof(PORT_MESSAGE), Message, MessageSize);
        }
        return (lpMem);
    }


    BOOL ReleaseMsgMem(LPVOID msg_mem)
    {
        return  HeapFree(GetProcessHeap(), 0, msg_mem);
    }

private:
    LPVOID          msg_;

public:
    PORT_MESSAGE    port_msg_;
};

class AlpcConn
{
public:
    AlpcConn()
        : alpc_port_(0)
        , sendrecv(0)
    {
        CStringHandler::InitChinese();
    }
    ~AlpcConn() {

    }

// server
public:

    bool create_server(const wchar_t* port_name) {
        DEFAPI(RtlInitUnicodeString);
        DEFAPI(NtAlpcCreatePort);

        ALPC_PORT_ATTRIBUTES    serverPortAttr;
        UNICODE_STRING          usPortName;
        OBJECT_ATTRIBUTES       objPort;
        bool flag = false;
        alpc_port_ = 0;

        pfunc_RtlInitUnicodeString(&usPortName, port_name);
        InitializeObjectAttributes(&objPort, &usPortName, 0, 0, 0);
        RtlSecureZeroMemory(&serverPortAttr, sizeof(serverPortAttr));
        serverPortAttr.MaxMessageLength = POST_LEN; // For ALPC this can be max of 64KB
        if (!pfunc_NtAlpcCreatePort(&alpc_port_, &objPort, &serverPortAttr)) {
            std::thread runserver(&AlpcConn::run_server, this);
            runserver.detach();
            flag = true;
        }
        return flag;
    }

    void run_server() {

        DEFAPI(NtAlpcSendWaitReceivePort);
        DEFAPI(NtAlpcAcceptConnectPort);
        DEFAPI(NtAlpcDisconnectPort);

        while (true) {

            NTSTATUS                ntRet;
            SIZE_T                  nLen;
            PostMsg recvmsg(nullptr);
            nLen = POST_LEN;
            ntRet = pfunc_NtAlpcSendWaitReceivePort(alpc_port_, 0, NULL, NULL, (PPORT_MESSAGE)recvmsg.GetMsgMem(), &nLen, NULL, NULL);

            if (NT_SUCCESS(ntRet)) {
                recvmsg.update_msg();
                if (recvmsg.port_msg_.u1.s1.TotalLength == sizeof(recvmsg.port_msg_)) {
                    PostMsg requestmsg(nullptr, recvmsg.port_msg_.MessageId, 0);
                    HANDLE hclient;
                    ntRet = pfunc_NtAlpcAcceptConnectPort(&hclient, alpc_port_, 0, NULL, NULL, NULL,
                        (PPORT_MESSAGE)requestmsg.GetMsgMem(), NULL, TRUE); // 0
                    printf("[i] NtAlpcAcceptConnectPort: 0x%X\n", ntRet);
                }
                else {
                    if (!strcmp((char*)recvmsg.GetDataMem(), "exit\n"))
                    {
                        printf("[i] Received 'exit' command\n");
                        ntRet = pfunc_NtAlpcDisconnectPort(alpc_port_, 0);
                        printf("[i] NtAlpcDisconnectPort: %X\n", ntRet);
                        CloseHandle(alpc_port_);
                        ExitThread(0);
                    }
                    else
                    {
                        printf("[i] Received Data: ");
                        printf("%s\n", (char*)recvmsg.GetDataMem());
                        CJSONHandler json((char*)recvmsg.GetDataMem());
                        if (json[L"reply"].GetInt() == 1) {
                            json[L"replymsg"] = L"servermsg";
                            std::shared_ptr<char> repstr = json.GetJsonString();
                            PostMsg replymsg(repstr.get(), recvmsg.port_msg_.MessageId, strlen(repstr.get()));
                            ntRet = pfunc_NtAlpcSendWaitReceivePort(alpc_port_, 0, (PPORT_MESSAGE)replymsg.GetMsgMem(),
                                NULL, NULL, NULL, NULL, NULL);

                            printf("[i] NtAlpcSendWaitReceivePort - reply: %X\n", ntRet);
                        }
                    }
                }
            }
        }
    }

// client
public:
    bool connect_server(const wchar_t* port_name) {
        DEFAPI(RtlInitUnicodeString);
        DEFAPI(NtAlpcConnectPort);
        DEFAPI(NtAlpcSendWaitReceivePort);

        sendrecv = pfunc_NtAlpcSendWaitReceivePort;

        UNICODE_STRING  usPort = { 0 };
        NTSTATUS        ntRet;
        bool flag = false;

        pfunc_RtlInitUnicodeString(&usPort, port_name);
        ntRet = pfunc_NtAlpcConnectPort(&alpc_port_, &usPort, NULL, NULL,
            ALPC_MSGFLG_SYNC_REQUEST, NULL, NULL, NULL, NULL, NULL, NULL);

        if (!ntRet) {
            flag = true;
        }
        return flag;
    }

    bool post_msg(const char* msg) {
        bool flag = false;

        PostMsg sendmsg(msg, 0, strlen(msg));
        PostMsg recvmsg(nullptr);
        SIZE_T recv_len = POST_LEN;

        NTSTATUS ntRet = sendrecv(alpc_port_, 0, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            NULL, NULL, NULL, NULL);

        if (NT_SUCCESS(ntRet)) {
            printf("[i] server Data: ");
            printf("%s\n", (char*)recvmsg.GetDataMem());
            flag = true;
        }

        return flag;
    }

    bool send_msg(const char* msg) {
        bool flag = false;

        PostMsg sendmsg(msg, 0, strlen(msg));
        PostMsg recvmsg(nullptr);
        SIZE_T recv_len = POST_LEN;

        NTSTATUS ntRet = sendrecv(alpc_port_, ALPC_MSGFLG_SYNC_REQUEST, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            (PPORT_MESSAGE)recvmsg.GetMsgMem(), &recv_len, NULL, NULL);

        if (NT_SUCCESS(ntRet)) {
            recvmsg.update_msg();
            printf("[i] server Data: ");
            printf("%s\n", (char*)recvmsg.GetDataMem());
            flag = true;
        }

        return flag;
    }

    bool post_msg(const wchar_t* msg) {
        bool flag = false;
        char* amsg = nullptr;
        if (CStringHandler::WChar2Ansi(msg, amsg)) {
            post_msg(amsg);
            delete[] amsg;
            flag = true;
        }
        return flag;
    }


    bool send_msg(const wchar_t* msg) {
        bool flag = false;
        char* amsg = nullptr;
        if (CStringHandler::WChar2Ansi(msg, amsg)) {
            send_msg(amsg);
            delete[] amsg;
            flag = true;
        }
        return flag;
    }


// 默认的一些类方法
private:
    HANDLE alpc_port_;
    NtAlpcSendWaitReceivePort_FuncType sendrecv;
};


// 来个单例主要维护发送消息的连接
class Singleton {
public:
    // 禁止拷贝和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    // 获取单例实例
    static Singleton& getInstance() {
        static Singleton instance;
        return instance;
    }

    // 示例方法
    void doSomething() {
        std::cout << "Singleton instance is working!" << std::endl;
    }

private:
    // 私有构造函数
    Singleton() {
        std::cout << "Singleton constructor called!" << std::endl;
    }

    // 私有析构函数（通常不需要，但可以加以防止意外销毁）
    ~Singleton() {
        std::cout << "Singleton destructor called!" << std::endl;
    }
};

#endif
