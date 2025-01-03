#pragma once

#ifdef __cplusplus
#include <Windows.h>
#include <winternl.h>

#include <thread>
#include <map>
#include <string>

#include "../ntalpcapi.h"

#include "alpc_register.h"

class AlpcHandlerCtx {
public:
    AlpcHandlerCtx(ULONG msg_id, void* alpc, std::shared_ptr<CJSONHandler> json)
        : json_(json)
        , alpc_(alpc)
        , msg_id_(msg_id)
    {}

    // 可以在此添加任意的上下文数据
    ULONG msg_id_;
    void* alpc_;
    std::shared_ptr<CJSONHandler> json_;
};

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

        DEFAPI(NtAlpcSendWaitReceivePort);
        sendrecv = pfunc_NtAlpcSendWaitReceivePort;
    }
    ~AlpcConn() {

    }

// server
public:

    bool create_server(const wchar_t* port_name) {

        bool flag = false;

        if (!alpc_port_) {
            DEFAPI(RtlInitUnicodeString);
            DEFAPI(NtAlpcCreatePort);

            ALPC_PORT_ATTRIBUTES    serverPortAttr;
            UNICODE_STRING          usPortName;
            OBJECT_ATTRIBUTES       objPort;

            std::wstring str_port = L"\\RPC Control\\";
            str_port += port_name;

            pfunc_RtlInitUnicodeString(&usPortName, str_port.c_str());
            InitializeObjectAttributes(&objPort, &usPortName, 0, 0, 0);
            RtlSecureZeroMemory(&serverPortAttr, sizeof(serverPortAttr));
            serverPortAttr.MaxMessageLength = POST_LEN; // For ALPC this can be max of 64KB

            NTSTATUS nret = pfunc_NtAlpcCreatePort(&alpc_port_, &objPort, &serverPortAttr);
            if (!nret) {
                std::thread runserver(&AlpcConn::run_server, this);
                runserver.detach();
                flag = true;
            }
            else {
                alpc_port_ = 0;
            }
        }
        return flag;
    }

    bool disconnect_server() {
        NTSTATUS ntRet = 0;
        if (alpc_port_) {
            DEFAPI(NtAlpcDisconnectPort);
            
            ntRet = pfunc_NtAlpcDisconnectPort(alpc_port_, 0);
            CloseHandle(alpc_port_);
            alpc_port_ = 0;
        }
        return NT_SUCCESS(ntRet);
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
                    std::shared_ptr<AlpcHandlerCtx> aptr =
                        std::make_shared<AlpcHandlerCtx>(
                            recvmsg.port_msg_.MessageId, this,
                            std::make_shared<CJSONHandler>((char*)recvmsg.GetDataMem()));

                    AlpcHandler::getInstance().submit((*aptr->json_)[L"type"].GetWString().get(),
                        std::static_pointer_cast<void>(aptr));
                    /*if (json[L"reply"].GetInt() == 1) {
                        json[L"replymsg"] = L"server_reply_msg";
                        std::shared_ptr<char> repstr = json.GetJsonString();
                        PostMsg replymsg(repstr.get(), recvmsg.port_msg_.MessageId, strlen(repstr.get()));
                        ntRet = pfunc_NtAlpcSendWaitReceivePort(alpc_port_, 0, (PPORT_MESSAGE)replymsg.GetMsgMem(),
                            NULL, NULL, NULL, NULL, NULL);
                    }*/
                }
            }
        }
    }

// client
public:
    bool connect_server(const wchar_t* port_name) {
        DEFAPI(RtlInitUnicodeString);
        DEFAPI(NtAlpcConnectPort);

        UNICODE_STRING  usPort = { 0 };
        NTSTATUS        ntRet;
        bool flag = false;

        std::wstring str_port = L"\\RPC Control\\";
        str_port += port_name;
        pfunc_RtlInitUnicodeString(&usPort, str_port.c_str());
        ntRet = pfunc_NtAlpcConnectPort(&alpc_port_, &usPort, NULL, NULL,
            ALPC_MSGFLG_SYNC_REQUEST, NULL, NULL, NULL, NULL, NULL, NULL);

        if (!ntRet) {
            flag = true;
        }
        return flag;
    }

    bool post_msg(CJSONHandler& json, ULONG msgid = 0) {
        bool flag = false;
        std::shared_ptr<char> msg = json.GetJsonString();
        PostMsg sendmsg(msg.get(), msgid, strlen(msg.get()));
        NTSTATUS ntRet = sendrecv(alpc_port_, 0, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            NULL, NULL, NULL, NULL);
        if (NT_SUCCESS(ntRet)) {
            flag = true;
        }
        return flag;
    }

    bool send_msg(CJSONHandler& json, ULONG msgid = 0) {
        bool flag = false;
        std::shared_ptr<char> msg = json.GetJsonString();
        PostMsg sendmsg(msg.get(), msgid, strlen(msg.get()));
        PostMsg recvmsg(nullptr);
        SIZE_T recv_len = POST_LEN;
        NTSTATUS ntRet = sendrecv(alpc_port_, ALPC_MSGFLG_SYNC_REQUEST, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            (PPORT_MESSAGE)recvmsg.GetMsgMem(), &recv_len, NULL, NULL);
        if (NT_SUCCESS(ntRet)) {
            recvmsg.update_msg();
            json.UpdateJson((char*)recvmsg.GetDataMem());
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
class AlpcMng {
public:
    // 禁止拷贝和赋值
    AlpcMng(const AlpcMng&) = delete;
    AlpcMng& operator=(const AlpcMng&) = delete;

    // 获取单例实例
    static AlpcMng& getInstance() {
        static AlpcMng instance;
        return instance;
    }

    // 示例方法
    bool run_server(const wchar_t* port_name) {
        return alpc_server_.create_server(port_name);
    }

    bool stop_server() {
        return alpc_server_.disconnect_server();
    }

    bool notify_msg(const wchar_t* port_name, CJSONHandler& json, bool post = true) {
        bool flag = false;
        if (alpc_client_.find(port_name) == alpc_client_.end()) {
            AlpcConn new_client;
            new_client.connect_server(port_name);
            alpc_client_[port_name] = new_client;
        }

        if (!post) {
            json[L"reply"] = 1;
            flag = alpc_client_[port_name].send_msg(json);
        }
        else {
            flag = alpc_client_[port_name].post_msg(json);
        }

        if (!flag) {
            alpc_client_.erase(port_name);
        }

        return flag;
    }

private:
    // 私有构造函数
    AlpcMng() {
        CStringHandler::InitChinese();
        AlpcHandler::getInstance().start(4);
    }

    // 私有析构函数（通常不需要，但可以加以防止意外销毁）
    ~AlpcMng() {
        AlpcHandler::getInstance().stop();
    }

private:
    std::map<std::wstring, AlpcConn> alpc_client_;
    AlpcConn alpc_server_;
};

#endif
