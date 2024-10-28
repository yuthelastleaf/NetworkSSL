#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#include "NetBIOS.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")

#define TIMEOUT 5000 // 5秒超时时间 (毫秒)
#define MAX_RETRIES 5 // 最大重试次数

//struct nbname_request {
//    uint16_t transaction_id;
//    uint16_t flags;
//    uint16_t question_count;
//    uint16_t answer_count;
//    uint16_t name_service_count;
//    uint16_t additional_record_count;
//    char question_name[34];
//    uint16_t question_type;
//    uint16_t question_class;
//};

/* Start of code from Samba */
int name_mangle(const char* In, char* Out, char name_type) {
    int   i;
    int   c;
    int   len;
    char  buf[20];
    char* p = Out;
    char scope[] = "";

    /* Safely copy the input string, In, into buf[]. */
    (void)memset(buf, 0, 20);
    if (strcmp(In, "*") == 0) buf[0] = '*';
    else
#ifdef HAVE_SNPRINTF
    (void)snprintf(buf, sizeof(buf) - 1, "%-15.15s%c", In, name_type);
#else
        (void)sprintf_s(buf, "%-15.15s%c", In, name_type);
#endif /* HAVE_SNPRINTF */

    /* Place the length of the first field into the output buffer. */
    p[0] = 32;
    p++;

    /* Now convert the name to the rfc1001/1002 format. */
    for (i = 0; i < 16; i++) {
        c = toupper(buf[i]);
        p[i * 2] = ((c >> 4) & 0x000F) + 'A';
        p[(i * 2) + 1] = (c & 0x000F) + 'A';
    }
    p += 32;
    p[0] = '\0';

    /* Add the scope string. */
    for (i = 0, len = 0; NULL != scope; i++, len++) {
        switch (scope[i]) {
        case '\0':
            p[0] = len;
            if (len > 0) p[len + 1] = 0;
            return(strlen(Out));
        case '.':
            p[0] = len;
            p += (len + 1);
            len = 0;
            break;
        default:
            p[len + 1] = scope[i];
            break;
        }
    }
    return(strlen(Out));
};

uint32_t get32(void* data) {
    union {
        char bytes[4];
        uint32_t all;
    } x;

    memcpy(x.bytes, data, 4);
    return(ntohl(x.all));
};

uint16_t get16(void* data) {
    union {
        char bytes[2];
        uint16_t all;
    } x;

    memcpy(x.bytes, data, 2);
    return(ntohs(x.all));
};

struct nb_host_info* parse_response(char* buff, int buffsize) {
    struct nb_host_info* hostinfo = NULL;
    nbname_response_footer_t* response_footer;
    nbname_response_header_t* response_header;
    int name_table_size;
    int offset = 0;

    if ((response_header = (nbname_response_header_t*)malloc(sizeof(nbname_response_header_t))) == NULL) return NULL;
    if ((response_footer = (nbname_response_footer_t*)malloc(sizeof(nbname_response_footer_t))) == NULL) return NULL;
    memset(response_header, 0, sizeof(nbname_response_header_t));
    memset(response_footer, 0, sizeof(nbname_response_footer_t));

    if ((hostinfo = (nb_host_info*)malloc(sizeof(struct nb_host_info))) == NULL) return NULL;
    hostinfo->header = NULL;
    hostinfo->names = NULL;
    hostinfo->footer = NULL;

    /* Parsing received packet */
    /* Start with header */
    if (offset + sizeof(response_header->transaction_id) >= buffsize) goto broken_packet;
    response_header->transaction_id = get16(buff + offset);
    //Move pointer to the next structure field
    offset += sizeof(response_header->transaction_id);
    hostinfo->header = response_header;

    // Check if there is room for next field in buffer
    if (offset + sizeof(response_header->flags) >= buffsize) goto broken_packet;
    response_header->flags = get16(buff + offset);
    offset += sizeof(response_header->flags);

    if (offset + sizeof(response_header->question_count) >= buffsize) goto broken_packet;
    response_header->question_count = get16(buff + offset);
    offset += sizeof(response_header->question_count);

    if (offset + sizeof(response_header->answer_count) >= buffsize) goto broken_packet;
    response_header->answer_count = get16(buff + offset);
    offset += sizeof(response_header->answer_count);

    if (offset + sizeof(response_header->name_service_count) >= buffsize) goto broken_packet;
    response_header->name_service_count = get16(buff + offset);
    offset += sizeof(response_header->name_service_count);

    if (offset + sizeof(response_header->additional_record_count) >= buffsize) goto broken_packet;
    response_header->additional_record_count = get16(buff + offset);
    offset += sizeof(response_header->additional_record_count);

    if (offset + sizeof(response_header->question_name) >= buffsize) goto broken_packet;
    strncpy_s(response_header->question_name, buff + offset, sizeof(response_header->question_name));
    offset += sizeof(response_header->question_name);

    if (offset + sizeof(response_header->question_type) >= buffsize) goto broken_packet;
    response_header->question_type = get16(buff + offset);
    offset += sizeof(response_header->question_type);

    if (offset + sizeof(response_header->question_class) >= buffsize) goto broken_packet;
    response_header->question_class = get16(buff + offset);
    offset += sizeof(response_header->question_class);

    if (offset + sizeof(response_header->ttl) >= buffsize) goto broken_packet;
    response_header->ttl = get32(buff + offset);
    offset += sizeof(response_header->ttl);

    if (offset + sizeof(response_header->rdata_length) >= buffsize) goto broken_packet;
    response_header->rdata_length = get16(buff + offset);
    offset += sizeof(response_header->rdata_length);

    if (offset + sizeof(response_header->number_of_names) >= buffsize) goto broken_packet;
    response_header->number_of_names = *(uint8_t*)(buff + offset);
    offset += sizeof(response_header->number_of_names);

    /* Done with packet header - it is okay */

    name_table_size = (response_header->number_of_names) * (sizeof(struct nbname));
    if (offset + name_table_size >= buffsize) goto broken_packet;

    if ((hostinfo->names = (nbname*)malloc(name_table_size)) == NULL) return NULL;
    memcpy(hostinfo->names, buff + offset, name_table_size);

    offset += name_table_size;

    /* Done with name table - it is okay */

        /* Now parse response footer */

    if (offset + sizeof(response_footer->adapter_address) >= buffsize) goto broken_packet;
    memcpy(response_footer->adapter_address,
        (buff + offset),
        sizeof(response_footer->adapter_address));
    offset += sizeof(response_footer->adapter_address);

    hostinfo->footer = response_footer;

    if (offset + sizeof(response_footer->version_major) >= buffsize) goto broken_packet;
    response_footer->version_major = *(uint8_t*)(buff + offset);
    offset += sizeof(response_footer->version_major);

    if (offset + sizeof(response_footer->version_minor) >= buffsize) goto broken_packet;
    response_footer->version_minor = *(uint8_t*)(buff + offset);
    offset += sizeof(response_footer->version_minor);

    if (offset + sizeof(response_footer->duration) >= buffsize) goto broken_packet;
    response_footer->duration = get16(buff + offset);
    offset += sizeof(response_footer->duration);

    if (offset + sizeof(response_footer->frmps_received) >= buffsize) goto broken_packet;
    response_footer->frmps_received = get16(buff + offset);
    offset += sizeof(response_footer->frmps_received);

    if (offset + sizeof(response_footer->frmps_transmitted) >= buffsize) goto broken_packet;
    response_footer->frmps_transmitted = get16(buff + offset);
    offset += sizeof(response_footer->frmps_transmitted);

    if (offset + sizeof(response_footer->iframe_receive_errors) >= buffsize) goto broken_packet;
    response_footer->iframe_receive_errors = get16(buff + offset);
    offset += sizeof(response_footer->iframe_receive_errors);

    if (offset + sizeof(response_footer->transmit_aborts) >= buffsize) goto broken_packet;
    response_footer->transmit_aborts = get16(buff + offset);
    offset += sizeof(response_footer->transmit_aborts);

    if (offset + sizeof(response_footer->transmitted) >= buffsize) goto broken_packet;
    response_footer->transmitted = get32(buff + offset);
    offset += sizeof(response_footer->transmitted);

    if (offset + sizeof(response_footer->received) >= buffsize) goto broken_packet;
    response_footer->received = get32(buff + offset);
    offset += sizeof(response_footer->received);

    if (offset + sizeof(response_footer->iframe_transmit_errors) >= buffsize) goto broken_packet;
    response_footer->iframe_transmit_errors = get16(buff + offset);
    offset += sizeof(response_footer->iframe_transmit_errors);

    if (offset + sizeof(response_footer->no_receive_buffer) >= buffsize) goto broken_packet;
    response_footer->no_receive_buffer = get16(buff + offset);
    offset += sizeof(response_footer->no_receive_buffer);

    if (offset + sizeof(response_footer->tl_timeouts) >= buffsize) goto broken_packet;
    response_footer->tl_timeouts = get16(buff + offset);
    offset += sizeof(response_footer->tl_timeouts);

    if (offset + sizeof(response_footer->ti_timeouts) >= buffsize) goto broken_packet;
    response_footer->ti_timeouts = get16(buff + offset);
    offset += sizeof(response_footer->ti_timeouts);

    if (offset + sizeof(response_footer->free_ncbs) >= buffsize) goto broken_packet;
    response_footer->free_ncbs = get16(buff + offset);
    offset += sizeof(response_footer->free_ncbs);

    if (offset + sizeof(response_footer->ncbs) >= buffsize) goto broken_packet;
    response_footer->ncbs = get16(buff + offset);
    offset += sizeof(response_footer->ncbs);

    if (offset + sizeof(response_footer->max_ncbs) >= buffsize) goto broken_packet;
    response_footer->max_ncbs = get16(buff + offset);
    offset += sizeof(response_footer->max_ncbs);

    if (offset + sizeof(response_footer->no_transmit_buffers) >= buffsize) goto broken_packet;
    response_footer->no_transmit_buffers = get16(buff + offset);
    offset += sizeof(response_footer->no_transmit_buffers);

    if (offset + sizeof(response_footer->max_datagram) >= buffsize) goto broken_packet;
    response_footer->max_datagram = get16(buff + offset);
    offset += sizeof(response_footer->max_datagram);

    if (offset + sizeof(response_footer->pending_sessions) >= buffsize) goto broken_packet;
    response_footer->pending_sessions = get16(buff + offset);
    offset += sizeof(response_footer->pending_sessions);

    if (offset + sizeof(response_footer->max_sessions) >= buffsize) goto broken_packet;
    response_footer->max_sessions = get16(buff + offset);
    offset += sizeof(response_footer->max_sessions);

    if (offset + sizeof(response_footer->packet_sessions) >= buffsize) goto broken_packet;
    response_footer->packet_sessions = get16(buff + offset);
    offset += sizeof(response_footer->packet_sessions);

    /* Done with packet footer and the whole packet */

    return hostinfo;

broken_packet:
    hostinfo->is_broken = offset;
    return hostinfo;
};

int gettimeofday(struct timeval* tp, struct timezone* tzp) {
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static const unsigned __int64 EPOCH = ((unsigned __int64)116444736000000000ULL);

    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres -= EPOCH;
    tmpres /= 10;  // convert into microseconds

    tp->tv_sec = (long)(tmpres / 1000000UL);
    tp->tv_usec = (long)(tmpres % 1000000UL);

    return 0;
}

int send_query(int sock, struct in_addr dest_addr, uint32_t rtt_base) {
    struct nbname_request request;
    struct sockaddr_in dest_sockaddr;
    int status;
    struct timeval tv;
    char errmsg[80];

    memset((void*)&dest_sockaddr, 0, sizeof(dest_sockaddr));
    dest_sockaddr.sin_family = AF_INET;
    dest_sockaddr.sin_port = htons(137);
    dest_sockaddr.sin_addr = dest_addr;

    request.flags = htons(0x0010);
    request.question_count = htons(1);
    request.answer_count = 0;
    request.name_service_count = 0;
    request.additional_record_count = 0;
    name_mangle("*", request.question_name, 0);
    request.question_type = htons(0x21);
    request.question_class = htons(0x01);

    gettimeofday(&tv, NULL);
    /* Use transaction ID as a timestamp */
    request.transaction_id = htons((tv.tv_sec - rtt_base) * 1000 + tv.tv_usec / 1000);
    /* printf("%s: timestamp: %d\n", inet_ntoa(dest_addr), request.transaction_id); */

    status = sendto(sock, (char*)&request, sizeof(request), 0,
        (struct sockaddr*)&dest_sockaddr, sizeof(dest_sockaddr));
    if (status == -1) {
        // snprintf(errmsg, 80, "%s\tSendto failed", inet_ntoa(dest_addr));
        char ipStr[INET_ADDRSTRLEN]; // 用于存储转换后的IP字符串
        inet_ntop(AF_INET, &(dest_sockaddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        std::cout << "IP Address: " << ipStr << std::endl;
    };
    return status;
};

bool sendAndReceiveUDP(const std::string& ipAddress, int port, const std::string& message) {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in destAddr;
    char recvBuf[512];
    int retries = 0;

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }

    // 创建 UDP 套接字
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return false;
    }

    // 设置接收超时时间
    DWORD timeout = TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // 配置目标地址
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &destAddr.sin_addr);

    // 重试发送和接收
    while (retries < MAX_RETRIES) {
        // 发送数据
        // sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
        int sendResult = send_query(sock, destAddr.sin_addr, 0);
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "Failed to send data. Error: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            WSACleanup();
            return false;
        }
        std::cout << "Data sent to " << ipAddress << " on port " << port << ". Waiting for response..." << std::endl;

        // 接收数据
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        int recvResult = recvfrom(sock, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&fromAddr, &fromLen);

        if (recvResult == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAETIMEDOUT) {
                std::cerr << "Timeout reached. Retrying (" << retries + 1 << "/" << MAX_RETRIES << ")..." << std::endl;
                retries++;
                continue;
            }
            else {
                std::cerr << "Failed to receive data. Error: " << WSAGetLastError() << std::endl;
                closesocket(sock);
                WSACleanup();
                return false;
            }
        }

        nb_host_info*  res = parse_response(recvBuf, recvResult);

        // 成功接收到数据
        // recvBuf[recvResult] = '\0'; // 将接收缓冲区末尾设为字符串结束符
        std::cout << "Received response: " << res->names->ascii_name << std::endl;

        // 清理并退出
        closesocket(sock);
        WSACleanup();
        return true;
    }

    // 达到最大重试次数，视为失败
    std::cerr << "Max retries reached. Sending failed." << std::endl;
    closesocket(sock);
    WSACleanup();
    return false;
}

int main() {
    std::string targetIp = "10.34.11.201"; // 替换为目标 IP 地址
    int targetPort = 137;                  // 替换为目标端口
    std::string message = "Hello, NetBIOS"; // 发送的消息内容

    if (sendAndReceiveUDP(targetIp, targetPort, message)) {
        std::cout << "Data sent and response received successfully." << std::endl;
    }
    else {
        std::cout << "Failed to send data and receive response." << std::endl;
    }

    return 0;
}
