#pragma once

#ifdef _WIN32
#define ALPC_API __declspec(dllexport)
#define ALPC_CALL __stdcall
#else
#define ALPC_API __attribute__((visibility("default")))
#define ALPC_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /// <summary>
    /// 根据名称启动alpc服务
    /// </summary>
    /// <param name="port_name">服务端名称</param>
    /// <returns>是否启动成功</returns>
    ALPC_API bool ALPC_CALL start_server(const char* port_name);

    /// <summary>
    /// 消息通知
    /// </summary>
    /// <param name="port_name">连接的管理端名称</param>
    /// <param name="json">json字符串</param>
    /// <param name="post">是否post发送消息</param>
    /// <returns>是否发送成功</returns>
    ALPC_API bool ALPC_CALL notify_msg(const char* port_name, const char* json, bool post = true);

#ifdef __cplusplus
}
#endif