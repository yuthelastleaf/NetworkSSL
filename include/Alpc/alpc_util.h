#pragma once

#ifdef __cplusplus
#include <Windows.h>
#include <winternl.h>
#include <sddl.h>

#include <thread>
#include <map>
#include <string>

#include "../ntalpcapi.h"

#include "alpc_register.h"
#include "../../include/SingletonTools/AsyncTaskManager.h"

class CommUtil {
public:
    static std::wstring NtPathToDosPath(const std::wstring& NtPath)
    {
        WCHAR deviceName[MAX_PATH];
        WCHAR driveName[3] = L" :";
        DWORD dwDrives = GetLogicalDrives();

        // 遍历所有可能的驱动器盘符
        for (int i = 0; i < 26; i++)
        {
            if ((dwDrives & (1 << i)) == 0)
                continue;

            // 构建驱动器名称（如 "C:"）
            driveName[0] = 'A' + i;

            // 获取驱动器的设备路径
            if (QueryDosDeviceW(driveName, deviceName, MAX_PATH))
            {
                size_t deviceNameLen = wcslen(deviceName);

                if (_wcsnicmp(NtPath.c_str(), deviceName, deviceNameLen) == 0 &&
                    (NtPath[deviceNameLen] == L'\\' || NtPath[deviceNameLen] == L'\0'))
                {
                    // 构建DOS路径
                    std::wstring DosPath = driveName;
                    DosPath += L"\\";

                    // 添加剩余路径部分（如果有）
                    if (NtPath.length() > deviceNameLen)
                    {
                        DosPath += NtPath.substr(deviceNameLen + 1);
                    }

                    return DosPath;
                }
            }
        }

        // 处理网络路径和其他特殊情况
        if (NtPath.substr(0, 12) == L"\\Device\\Mup\\")
        {
            return L"\\\\" + NtPath.substr(12);
        }

        // 如果没有找到匹配，就返回原始NT路径
        return NtPath;
    }

    static std::wstring DosPathToNtPath(const std::wstring& DosPath)
    {
        WCHAR driveName[3] = L" :";
        WCHAR deviceName[MAX_PATH];

        // 检查是否是UNC路径（网络路径）
        if (DosPath.substr(0, 2) == L"\\\\")
        {
            return L"\\Device\\Mup\\" + DosPath.substr(2);
        }

        // 检查是否是有效的驱动器路径
        if (DosPath.length() >= 2 && iswalpha(DosPath[0]) && DosPath[1] == L':')
        {
            driveName[0] = DosPath[0];

            // 获取设备路径
            if (QueryDosDeviceW(driveName, deviceName, MAX_PATH))
            {
                std::wstring NtPath = deviceName;

                // 添加路径的其余部分
                if (DosPath.length() > 2 && DosPath[2] == L'\\')
                {
                    NtPath += DosPath.substr(2);
                }

                return NtPath;
            }
        }

        // 无法转换，返回空字符串
        return L"";
    }

};


class AlpcHandlerCtx {
public:
    AlpcHandlerCtx(ULONG msg_id, void* alpc, std::shared_ptr<CJSONHandler> json)
        : json_(json)
        , alpc_(alpc)
        , msg_id_(msg_id)
    {}

    // 可以在此添加任意的上下文数据
    ULONG msg_id_;
    HANDLE alpc_;
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
private:
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

private:
    PALPC_MESSAGE_ATTRIBUTES alloc_message_attribute(ULONG ulAttributeFlags) {
        DEFAPI(AlpcGetHeaderSize);
        DEFAPI(AlpcInitializeMessageAttribute);


        NTSTATUS lSuccess;
        PALPC_MESSAGE_ATTRIBUTES pAttributeBuffer;
        LPVOID lpBuffer;
        ULONG lpReqBufSize = 0;
        SIZE_T ulAllocBufSize;

        ulAllocBufSize = pfunc_AlpcGetHeaderSize(ulAttributeFlags); // this calculates: sizeof(ALPC_MESSAGE_ATTRIBUTES) + size of attribute structures
        lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulAllocBufSize);
        if (GetLastError() != 0) {
            return NULL;
        }
        pAttributeBuffer = (PALPC_MESSAGE_ATTRIBUTES)lpBuffer;
        //wprintf(L"[*] Initializing ReceiveMessage Attributes (0x%X)...", ulAttributeFlags);
        lSuccess = pfunc_AlpcInitializeMessageAttribute(
            ulAttributeFlags,	// attributes
            pAttributeBuffer,	// pointer to attributes structure
            ulAllocBufSize,	// buffer size
            &lpReqBufSize
        );
        if (!NT_SUCCESS(lSuccess)) {
            //wprintf(L"Error: 0x%X\n", lSuccess);
            //pAttributeBuffer->ValidAttributes = ulAttributeFlags;
            return NULL;
        }
        else {
            //wprintf(L"Success.\n");
            return pAttributeBuffer;
        }
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

            SECURITY_QUALITY_OF_SERVICE SecurityQos;
            SecurityQos.ImpersonationLevel = SecurityIdentification; // SecurityImpersonation; // ; // ; // ;// ;
            SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
            SecurityQos.EffectiveOnly = 0;
            SecurityQos.Length = sizeof(SecurityQos);

            // ALPC Port Attributs
            serverPortAttr.Flags = ALPC_PORTFLG_ALLOW_DUP_OBJECT | ALPC_PORTFLG_ALLOWIMPERSONATION | ALPC_PORTFLG_LRPC_WAKE_POLICY1 | ALPC_PORTFLG_LRPC_WAKE_POLICY2 | ALPC_PORTFLG_LRPC_WAKE_POLICY3; //0xb84a3f0;// ALPC_PORTFLG_ALLOW_DUP_OBJECT | ALPC_PORTFLG_AllowImpersonation | ALPC_PORTFLG_LRPC_WAKE_POLICY1 | ALPC_PORTFLG_LRPC_WAKE_POLICY2 | ALPC_PORTFLG_LRPC_WAKE_POLICY3; // ; //0x8080000;// ALPC_PORFLG_ALLOW_LPC_REQUESTS;// ALPC_PORFLG_ALLOW_LPC_REQUESTS;// | ALPC_PORFLG_SYSTEM_PROCESS;//0x010000 | 0x020000;	// Found '0x3080000' in rpcrt4.dll
            serverPortAttr.MaxMessageLength = POST_LEN; // technically the hard limit for this is 65535, if no constrains you can use AlpcMaxAllowedMessageLength() to set this limit
            serverPortAttr.MemoryBandwidth = 512;
            serverPortAttr.MaxPoolUsage = 0xffffffff;
            serverPortAttr.MaxSectionSize = 0xffffffff; // 20000;  
            serverPortAttr.MaxViewSize = 0xffffffff; // 20000; // sizeof(PORT_VIEW); 
            serverPortAttr.MaxTotalSectionSize = 0xffffffff; // 20000;
            serverPortAttr.DupObjectTypes = 0xffffffff;
            RtlSecureZeroMemory(&SecurityQos, sizeof(SecurityQos));
            serverPortAttr.SecurityQos = SecurityQos;

            LPCWSTR szDACL = L"D:(A;OICI;GAGW;;;AU)";// Allow full control to authenticated users

            PSECURITY_DESCRIPTOR pSD;
            ULONG ulSDSize = 0;
            BOOL success = ConvertStringSecurityDescriptorToSecurityDescriptor(
                szDACL,
                SDDL_REVISION_1,
                &pSD,
                &ulSDSize
            );

            InitializeObjectAttributes(&objPort, &usPortName, 0, 0, 0);
            // RtlSecureZeroMemory(&serverPortAttr, sizeof(serverPortAttr));

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

                // if (recvmsg.port_msg_.u1.s1.TotalLength == sizeof(recvmsg.port_msg_)) {
                if (recvmsg.port_msg_.u2.s2.Type == ALPC_GET_CONNECT) {
                    PostMsg requestmsg(nullptr, recvmsg.port_msg_.MessageId, 0);
                    HANDLE hclient;
                    ntRet = pfunc_NtAlpcAcceptConnectPort(&hclient, alpc_port_, 0, NULL, NULL, NULL,
                        (PPORT_MESSAGE)requestmsg.GetMsgMem(), NULL, TRUE); // 0
                    // printf("[i] NtAlpcAcceptConnectPort: 0x%X，connected\n", ntRet);
                    // printf("connect client name: %s\n", recvmsg.GetDataMem());

                    
                }
                else if(recvmsg.port_msg_.u2.s2.Type == ALPC_GET_MESSAGE){
                    std::shared_ptr<AlpcHandlerCtx> aptr =
                        std::make_shared<AlpcHandlerCtx>(
                            recvmsg.port_msg_.MessageId, alpc_port_,
                            std::make_shared<CJSONHandler>((char*)recvmsg.GetDataMem()));

                    AsyncTaskManager::GetInstance().AddTask([](std::shared_ptr<AlpcHandlerCtx> aptr) {
                        if (!(*aptr->json_)[L"reply"].GetInt()) {
                            AlpcHandler::getInstance().submit((*aptr->json_)[L"type"].GetWString().get(),
                                std::static_pointer_cast<void>(aptr));
                        }
                        else {
                            AlpcHandler::getInstance().sync_run_task((*aptr->json_)[L"type"].GetWString().get(),
                                std::static_pointer_cast<void>(aptr));
                        }
                        }, aptr);
                }
            }
        }
    }

// client
private:
    bool connect_server(const wchar_t* port_name, std::string alpc_name, HANDLE& hsrv) {
        DEFAPI(RtlInitUnicodeString);
        DEFAPI(NtAlpcConnectPort);

        UNICODE_STRING  usPort = { 0 };
        NTSTATUS        ntRet;
        bool flag = false;

        std::wstring str_port = L"\\RPC Control\\";
        str_port += port_name;
        pfunc_RtlInitUnicodeString(&usPort, str_port.c_str());

        ALPC_PORT_ATTRIBUTES    serverPortAttr;
        SECURITY_QUALITY_OF_SERVICE SecurityQos;
        SecurityQos.ImpersonationLevel = SecurityIdentification; // SecurityImpersonation; // ; // ; // ;// ;
        SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        SecurityQos.EffectiveOnly = 0;
        SecurityQos.Length = sizeof(SecurityQos);

        // ALPC Port Attributs
        serverPortAttr.Flags = ALPC_PORTFLG_ALLOW_DUP_OBJECT | ALPC_PORTFLG_ALLOWIMPERSONATION | ALPC_PORTFLG_LRPC_WAKE_POLICY1 | ALPC_PORTFLG_LRPC_WAKE_POLICY2 | ALPC_PORTFLG_LRPC_WAKE_POLICY3; //0xb84a3f0;// ALPC_PORTFLG_ALLOW_DUP_OBJECT | ALPC_PORTFLG_AllowImpersonation | ALPC_PORTFLG_LRPC_WAKE_POLICY1 | ALPC_PORTFLG_LRPC_WAKE_POLICY2 | ALPC_PORTFLG_LRPC_WAKE_POLICY3; // ; //0x8080000;// ALPC_PORFLG_ALLOW_LPC_REQUESTS;// ALPC_PORFLG_ALLOW_LPC_REQUESTS;// | ALPC_PORFLG_SYSTEM_PROCESS;//0x010000 | 0x020000;	// Found '0x3080000' in rpcrt4.dll
        serverPortAttr.MaxMessageLength = POST_LEN; // technically the hard limit for this is 65535, if no constrains you can use AlpcMaxAllowedMessageLength() to set this limit
        serverPortAttr.MemoryBandwidth = 512;
        serverPortAttr.MaxPoolUsage = 0xffffffff;
        serverPortAttr.MaxSectionSize = 0xffffffff; // 20000;  
        serverPortAttr.MaxViewSize = 0xffffffff; // 20000; // sizeof(PORT_VIEW); 
        serverPortAttr.MaxTotalSectionSize = 0xffffffff; // 20000;
        serverPortAttr.DupObjectTypes = 0xffffffff;
        RtlSecureZeroMemory(&SecurityQos, sizeof(SecurityQos));
        serverPortAttr.SecurityQos = SecurityQos;

        LPCWSTR szDACL = L"D:(A;OICI;GAGW;;;AU)";// Allow full control to authenticated users

        PSECURITY_DESCRIPTOR pSD;
        ULONG ulSDSize = 0;
        BOOL success = ConvertStringSecurityDescriptorToSecurityDescriptor(
            szDACL,
            SDDL_REVISION_1,
            &pSD,
            &ulSDSize
        );

        // InitializeObjectAttributes(&objPort, &usPortName, 0, 0, 0);
        // RtlSecureZeroMemory(&serverPortAttr, sizeof(serverPortAttr));

        PostMsg connect(alpc_name.c_str(), 0, alpc_name.length());
        /*ntRet = pfunc_NtAlpcConnectPort(&hsrv, &usPort, NULL, &serverPortAttr,
            ALPC_SYNC_CONNECTION, NULL, (PPORT_MESSAGE)connect.GetMsgMem(), NULL, NULL, NULL, NULL);*/
        ntRet = pfunc_NtAlpcConnectPort(&hsrv, &usPort, NULL, &serverPortAttr,
            ALPC_SYNC_CONNECTION, NULL, NULL, NULL, NULL, NULL, NULL);

        if (!ntRet) {
            flag = true;
        }
        else {
            hsrv = NULL;
        }
        return flag;
    }


// 提供的外部方法类
public:
    bool post_msg(CJSONHandler& json, HANDLE port = NULL, ULONG msgid = 0) {
        bool flag = false;
        std::shared_ptr<char> msg = json.GetJsonString();
        PostMsg sendmsg(msg.get(), msgid, strlen(msg.get()));

        if (!port) {
            port = alpc_port_;
        }

        NTSTATUS ntRet = sendrecv(port, 0, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            NULL, NULL, NULL, NULL);
        if (NT_SUCCESS(ntRet)) {
            flag = true;
        }
        return flag;
    }

    bool send_msg(CJSONHandler& json, HANDLE port = NULL, ULONG msgid = 0) {
        bool flag = false;
        std::shared_ptr<char> msg = json.GetJsonString();
        PostMsg sendmsg(msg.get(), msgid, strlen(msg.get()));
        PostMsg recvmsg(nullptr);
        SIZE_T recv_len = POST_LEN;

        if (!port) {
            port = alpc_port_;
        }

        NTSTATUS ntRet = sendrecv(port, NULL, (PPORT_MESSAGE)sendmsg.GetMsgMem(), NULL,
            (PPORT_MESSAGE)recvmsg.GetMsgMem(), &recv_len, NULL, NULL);
        if (NT_SUCCESS(ntRet)) {
            recvmsg.update_msg();
            json.UpdateJson((char*)recvmsg.GetDataMem());
            flag = true;
        }
        return flag;
    }


    // 禁止拷贝和赋值
    AlpcConn(const AlpcConn&) = delete;
    AlpcConn& operator=(const AlpcConn&) = delete;

    // 获取单例实例
    static AlpcConn& getInstance() {
        static AlpcConn instance;
        return instance;
    }

    // 示例方法
    bool start_server(const wchar_t* port_name) {

        if (alpc_svr_name_.empty()) {
            char* str_name;
            CStringHandler::WChar2Ansi(port_name, str_name);
            alpc_svr_name_ = str_name;
        }
        return create_server(port_name);
    }

    bool start_server(const char* port_name) {

        if (alpc_svr_name_.empty()) {
            alpc_svr_name_ = port_name;
        }
        wchar_t* wstr_name = NULL;
        CStringHandler::Ansi2WChar(port_name, wstr_name);
        std::unique_ptr<wchar_t> pportname(wstr_name);
        return create_server(pportname.get());
    }

    bool set_alpc_name(const char* aname) {
        bool flag = false;
        if (alpc_svr_name_.empty()) {
            alpc_svr_name_ = aname;
            flag = true;
        }
        return flag;
    }

    bool stop_server() {
        return disconnect_server();
    }

    bool notify_msg(const char* port_name, CJSONHandler& json, bool post = true) {
        bool flag = false;
        HANDLE client = NULL;
        {
            std::lock_guard<std::mutex> lock(alpc_mtx_);
            if (alpc_connect_.find(port_name) == alpc_connect_.end()) {
                wchar_t* wport_name = nullptr;
                CStringHandler::Ansi2WChar(port_name, wport_name);
                if (wport_name) {
                    connect_server(wport_name, alpc_svr_name_, client);
                    alpc_connect_[port_name] = client;
                }
            }
            else {
                client = alpc_connect_[port_name];
            }
        }
        if (!post) {
            json[L"reply"] = 1;
            flag = send_msg(json, client);
        }
        else {
            flag = post_msg(json, client);
        }

        if (!flag) {
            std::lock_guard<std::mutex> lock(alpc_mtx_);
            alpc_connect_.erase(port_name);
        }

        return flag;
    }

// 默认的一些类方法
private:
    HANDLE alpc_port_;
    std::mutex alpc_mtx_;
    std::string alpc_svr_name_;
    std::map<std::string, HANDLE> alpc_connect_;
    NtAlpcSendWaitReceivePort_FuncType sendrecv;
};

#endif
