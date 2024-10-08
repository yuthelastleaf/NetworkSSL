#include <iostream>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <Winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")


int main() {
    // ��ʼ�� Windows �׽��ֿ�
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    // ��ʼ�� OpenSSL ��
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // ���� SSL �����Ķ���
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        WSACleanup();
        return -1;
    }

    // ����֤���˽Կ�ļ�
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load certificate" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load private key" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // ���� socket ���󶨶˿ںţ������ͻ�����������
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(8888);

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Binding failed" << std::endl;
        closesocket(sockfd);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    if (listen(sockfd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listening failed" << std::endl;
        closesocket(sockfd);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // ���ܿͻ�����������
    struct sockaddr_in client;
    int client_len = sizeof(client);
    SOCKET client_sockfd = accept(sockfd, (struct sockaddr*)&client, &client_len);
    if (client_sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to accept connection" << std::endl;
        closesocket(sockfd);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // ���� SSL ���󲢽� socket �󶨵� SSL ����
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_sockfd);

    // ���� TLS ����
    if (SSL_accept(ssl) <= 0) {
        std::cerr << "TLS handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        closesocket(client_sockfd);
        closesocket(sockfd);
        SSL_CTX_free(ctx);
        WSACleanup();
        return -1;
    }

    // ���պͷ�������
    char buffer[1024] = { 0 };
    int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "Received: " << buffer << std::endl;

        const char* response = "Hello from server!";
        SSL_write(ssl, response, strlen(response));
    }

    // �ر����Ӳ��ͷ���Դ
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(client_sockfd);
    closesocket(sockfd);
    SSL_CTX_free(ctx);
    WSACleanup();

    return 0;
}
