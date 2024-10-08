#include <iostream>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

int main() {
    // 初始化 Winsock 库
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    // 初始化 OpenSSL 库
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        WSACleanup();
        return -1;
    }

    // 创建 SSL 对象
    SSL* ssl = SSL_new(ctx);

    // 创建 socket 并与服务器建立连接
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // 设置服务器地址和端口号
    const char* server_addr = "127.0.0.1";
    int server_port = 8888;
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_addr, &(server.sin_addr)) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    // 将 socket 绑定到 SSL 对象
    SSL_set_fd(ssl, sockfd);

    // 进行 TLS 握手
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "TLS handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    // 发送和接收数据
    const char* message = "Hello from client!";
    char buffer[1024];
    SSL_write(ssl, message, strlen(message));
    int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "Received: " << buffer << std::endl;
    }
    else {
        std::cerr << "SSL_read failed" << std::endl;
    }

    // 关闭连接并释放资源
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(sockfd);
    SSL_CTX_free(ctx);
    WSACleanup();

    return 0;
}
