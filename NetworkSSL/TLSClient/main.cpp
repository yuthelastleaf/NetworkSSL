#include <iostream>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

int main() {
    // ��ʼ�� Winsock ��
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    // ��ʼ�� OpenSSL ��
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

    // ���� SSL ����
    SSL* ssl = SSL_new(ctx);

    // ���� socket �����������������
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // ���÷�������ַ�Ͷ˿ں�
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

    // �� socket �󶨵� SSL ����
    SSL_set_fd(ssl, sockfd);

    // ���� TLS ����
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "TLS handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    // ���ͺͽ�������
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

    // �ر����Ӳ��ͷ���Դ
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(sockfd);
    SSL_CTX_free(ctx);
    WSACleanup();

    return 0;
}
