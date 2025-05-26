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
    /// ������������alpc����
    /// </summary>
    /// <param name="port_name">���������</param>
    /// <returns>�Ƿ������ɹ�</returns>
    ALPC_API bool ALPC_CALL start_server(const char* port_name);

    /// <summary>
    /// ��Ϣ֪ͨ
    /// </summary>
    /// <param name="port_name">���ӵĹ��������</param>
    /// <param name="json">json�ַ���</param>
    /// <param name="post">�Ƿ�post������Ϣ</param>
    /// <returns>�Ƿ��ͳɹ�</returns>
    ALPC_API bool ALPC_CALL notify_msg(const char* port_name, const char* json, bool post = true);

#ifdef __cplusplus
}
#endif