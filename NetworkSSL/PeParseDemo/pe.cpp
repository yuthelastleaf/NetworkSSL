#include "../../include/pe/nt-headers.h"
#include "../../include/pe/parse.h"
#include <openssl/pkcs7.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/cms.h>




#include <iostream>
#include <fstream>

typedef struct _WIN_CERTIFICATE {
    unsigned long dwLength;
    unsigned short  wRevision;
    unsigned short  wCertificateType;
    unsigned char*  bCertificate;
} WIN_CERTIFICATE, * LPWIN_CERTIFICATE;

// 计算文件哈希（排除签名部分）
std::vector<unsigned char> CalculateFileHash(const char* filepath, size_t signatureSize) {
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t dataSize = fileSize - signatureSize;
    std::vector<char> buffer(dataSize);
    file.read(buffer.data(), dataSize);

    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<unsigned char*>(buffer.data()), dataSize, hash.data());
    return hash;
}

// 手动验证PKCS7签名
int ManualVerifyPKCS7(const unsigned char* signatureData, size_t signatureLen, const char* filePath, size_t signatureSize) {
    const unsigned char* p = signatureData;
    PKCS7* p7 = d2i_PKCS7(nullptr, &p, signatureLen);

    
    if (!p7) {
        std::cerr << "解析PKCS7失败: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
        return 0;
    }

    // 1. 检查PKCS7类型是否为SignedData
    if (!PKCS7_type_is_signed(p7)) {
        std::cerr << "非签名类型PKCS7" << std::endl;
        PKCS7_free(p7);
        return 0;
    }

    // 2. 提取签名中的哈希值
    STACK_OF(PKCS7_SIGNER_INFO)* signers = PKCS7_get_signer_info(p7);
    if (!signers || sk_PKCS7_SIGNER_INFO_num(signers) == 0) {
        std::cerr << "无签名者信息" << std::endl;
        PKCS7_free(p7);
        return 0;
    }

    PKCS7_SIGNER_INFO* si = sk_PKCS7_SIGNER_INFO_value(signers, 0);
    // const ASN1_OCTET_STRING* signedHash = PKCS7_get0_signature(p7);
    
    // 或直接访问结构体字段（不推荐，但有文档支持时可行）
    const ASN1_OCTET_STRING* signedHash = si->enc_digest;
    if (!signedHash) {
        std::cerr << "无法获取签名哈希" << std::endl;
        PKCS7_free(p7);
        return 0;
    }

    std::cout << "\n===== 共找到 " << sk_PKCS7_SIGNER_INFO_num(signers) << " 个签名者 =====" << std::endl;
    for (int i = 0; i < sk_PKCS7_SIGNER_INFO_num(signers); ++i) {
        PKCS7_SIGNER_INFO* si = sk_PKCS7_SIGNER_INFO_value(signers, i);
        std::cout << "\n[ 签名者 #" << i + 1 << " 的信息 ]" << std::endl;

        // 打印颁发者和序列号
        if (si->issuer_and_serial) {
            char* issuer = X509_NAME_oneline(si->issuer_and_serial->issuer, nullptr, 0);
            char* serial = i2s_ASN1_INTEGER(nullptr, si->issuer_and_serial->serial);
            std::cout << "颁发者    : " << issuer << std::endl;
            std::cout << "序列号    : " << serial << std::endl;
            OPENSSL_free(issuer);
            OPENSSL_free(serial);
        }

        // 打印摘要算法
        char alg_name[256];
        OBJ_obj2txt(alg_name, sizeof(alg_name), si->digest_alg->algorithm, 0);
        std::cout << "摘要算法  : " << alg_name << std::endl;

        // 打印签名算法
        OBJ_obj2txt(alg_name, sizeof(alg_name), si->digest_enc_alg->algorithm, 0);
        std::cout << "签名算法  : " << alg_name << std::endl;

        // 打印签名值信息
        std::cout << "签名长度  : " << si->enc_digest->length << " 字节" << std::endl;
    }

    // 3. 打印证书链信息
    STACK_OF(X509)* certs = p7->d.sign->cert;
    if (!certs || sk_X509_num(certs) == 0) {
        std::cerr << "\n无证书信息" << std::endl;
    }
    else {
        std::cout << "\n===== 共找到 " << sk_X509_num(certs) << " 张证书 =====" << std::endl;
        for (int i = 0; i < sk_X509_num(certs); ++i) {
            X509* cert = sk_X509_value(certs, i);
            std::cout << "\n[ 证书 #" << i + 1 << " 的信息 ]" << std::endl;

            // 打印主题信息
            char* subject = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
            std::cout << "主题      : " << subject << std::endl;
            OPENSSL_free(subject);

            // 打印颁发者信息
            char* issuer = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
            std::cout << "颁发者    : " << issuer << std::endl;
            OPENSSL_free(issuer);

            // 打印序列号
            ASN1_INTEGER* serial = X509_get_serialNumber(cert);
            char* serialStr = i2s_ASN1_INTEGER(nullptr, serial);
            std::cout << "序列号    : " << serialStr << std::endl;
            OPENSSL_free(serialStr);

            // 打印有效期
            /*ASN1_TIME* notBefore = X509_get_notBefore(cert);
            ASN1_TIME* notAfter = X509_get_notAfter(cert);
            std::cout << "有效期    : ";
            
            ASN1_TIME_print_fp(stdout, notBefore);

            std::cout << " 至 ";
            ASN1_TIME_print_fp(stdout, notAfter);
            std::cout << std::endl;*/
        }
    }


    // 3. 计算文件哈希
    std::vector<unsigned char> fileHash = CalculateFileHash(filePath, signatureSize);
    if (signedHash->length != fileHash.size() || memcmp(signedHash->data, fileHash.data(), fileHash.size()) != 0) {
        std::cerr << "哈希值不匹配" << std::endl;
        PKCS7_free(p7);
        return 0;
    }

    // 4. 提取签名者证书
    X509* cert = PKCS7_cert_from_signer_info(p7, si);
    if (!cert) {
        std::cerr << "无法获取签名证书" << std::endl;
        PKCS7_free(p7);
        return 0;
    }

    // 5. 验证证书链
    X509_STORE* store = X509_STORE_new();
    X509_STORE_set_default_paths(store); // 加载系统根证书

    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(ctx, store, cert, nullptr);
    X509_STORE_CTX_set_flags(ctx, X509_V_FLAG_CHECK_SS_SIGNATURE);

    int ret = X509_verify_cert(ctx);
    if (ret != 1) {
        std::cerr << "证书链验证失败: " << X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)) << std::endl;
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        PKCS7_free(p7);
        return 0;
    }

    // 6. 清理资源
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    PKCS7_free(p7);
    return 1;
}

int Openssl_Verify(unsigned char* signature_msg, unsigned int length) {
    const unsigned char* p_signature_msg = signature_msg;
    PKCS7* p7 = d2i_PKCS7(NULL, &p_signature_msg, length);
    if (!p7) {
        fprintf(stderr, "解析 PKCS7 失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return 0;
    }

    STACK_OF(X509)* certs = p7->d.sign->cert;
    STACK_OF(PKCS7_SIGNER_INFO)* sk = PKCS7_get_signer_info(p7);
    if (!sk) {
        fprintf(stderr, "没有签名者信息: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return 0;
    }

    // 检查 PKCS7 类型是否为 SignedData
    if (!PKCS7_type_is_signed(p7)) {
        fprintf(stderr, "非 SignedData 类型的 PKCS7 结构\n");
        PKCS7_free(p7);
        return 0;
    }

    // 初始化证书存储（加载系统根证书）
    X509_STORE* store = X509_STORE_new();
    X509_STORE_set_default_paths(store);

    // 直接验证签名（无需解码数据）
    int ret = PKCS7_verify(p7, NULL, store, NULL, NULL, PKCS7_NOVERIFY);
    if (ret != 1) {
        fprintf(stderr, "签名验证失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
        X509_STORE_free(store);
        PKCS7_free(p7);
        return 0;
    }

    // 检查证书链有效性（可选）
    ret = PKCS7_verify(p7, NULL, store, NULL, NULL, 0);
    if (ret != 1) {
        fprintf(stderr, "证书链验证失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
        X509_STORE_free(store);
        PKCS7_free(p7);
        return 0;
    }

    X509_STORE_free(store);
    PKCS7_free(p7);
    return 1;
}

PKCS7* ParseSignature(const std::vector<unsigned char>& signature) {
    BIO* bio = BIO_new_mem_buf(signature.data(), signature.size());
    PKCS7* pkcs7 = d2i_PKCS7_bio(bio, nullptr);
    BIO_free(bio);
    if (!pkcs7) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    return pkcs7;
}

//std::vector<unsigned char> CalculateFileHash(const char* filepath, size_t signatureSize) {
//    std::ifstream file(filepath, std::ios::binary);
//    file.seekg(0, std::ios::end);
//    size_t fileSize = file.tellg();
//    file.seekg(0, std::ios::beg);
//
//    // 排除签名部分
//    size_t dataSize = fileSize - signatureSize;
//    std::vector<char> buffer(dataSize);
//    file.read(buffer.data(), dataSize);
//
//    // 计算 SHA256 哈希
//    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
//    SHA256(reinterpret_cast<unsigned char*>(buffer.data()), dataSize, hash.data());
//    return hash;
//}

//bool VerifySignature(PKCS7* pkcs7, const std::vector<unsigned char>& fileHash) {
//    // 1. 初始化证书存储（加载系统根证书）
//    X509_STORE* store = X509_STORE_new();
//    X509_STORE_set_default_paths(store);
//
//    // 2. 创建用于验证的上下文
//    PKCS7_VERIFY_CTX* ctx = PKCS7_VERIFY_CTX_new();
//    PKCS7_VERIFY_CTX_init(ctx);
//    PKCS7_VERIFY_CTX_set_flags(ctx, PKCS7_NOVERIFY);  // 跳过证书链验证（仅验证哈希）
//
//    // 3. 验证哈希是否匹配
//    int result = PKCS7_verify(pkcs7, nullptr, store, nullptr, nullptr, PKCS7_NOVERIFY);
//    if (result != 1) {
//        ERR_print_errors_fp(stderr);
//        PKCS7_VERIFY_CTX_free(ctx);
//        X509_STORE_free(store);
//        return false;
//    }
//
//    // 4. 比较哈希值
//    const ASN1_OCTET_STRING* signedHash = PKCS7_get0_signature(pkcs7);
//    if (!signedHash || signedHash->length != fileHash.size() ||
//        memcmp(signedHash->data, fileHash.data(), fileHash.size()) != 0) {
//        std::cerr << "哈希值不匹配" << std::endl;
//        return false;
//    }
//
//    // 5. 清理资源
//    PKCS7_VERIFY_CTX_free(ctx);
//    X509_STORE_free(store);
//    return true;
//}

int main(int argc, char* argv[]) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    unsigned char* raw_entry = nullptr;
    unsigned int sign_size = 0;
    WIN_CERTIFICATE cert = { 0 };
    WIN_CERTIFICATE cert2 = { 0 };
    if (argc > 1) {

        peparse::parsed_pe* pe = peparse::ParsePEFromFile(argv[1]);
        sign_size = peparse::GetDataDirectoryEntry(pe, peparse::data_directory_kind::DIR_SECURITY, raw_entry);
        // 解析第一个签名块的头
        unsigned char* p = raw_entry;
        cert.dwLength = *reinterpret_cast<const unsigned long*>(p); p += 4;
        cert.wRevision = *reinterpret_cast<const unsigned short*>(p); p += 2;
        cert.wCertificateType = *reinterpret_cast<const unsigned short*>(p); p += 2;
        cert.bCertificate = p; // 数据部分起始地址

        //// 移动到第二个签名块
        //unsigned char* next_block = raw_entry + cert.dwLength;
        //if (next_block - raw_entry > sign_size) {
        //    std::cerr << "第一个签名块超出安全目录范围" << std::endl;
        //    return 0;
        //}

        //// 解析第二个签名块的头
        //cert2.dwLength = *reinterpret_cast<const unsigned long*>(next_block); next_block += 4;
        //cert2.wRevision = *reinterpret_cast<const unsigned short*>(next_block); next_block += 2;
        //cert2.wCertificateType = *reinterpret_cast<const unsigned short*>(next_block); next_block += 2;
        //cert2.bCertificate = next_block;

        //// 验证第二个块是否在安全目录范围内
        //if ((next_block + cert2.dwLength - 8) > (raw_entry + sign_size)) {
        //    std::cerr << "第二个签名块超出安全目录范围" << std::endl;
        //    return 0;
        //}

        
    }
    if (sign_size > 0) {
        // Openssl_Verify(cert.bCertificate, cert.dwLength);
        ManualVerifyPKCS7(cert.bCertificate, cert.dwLength, argv[1], cert.dwLength);
        ManualVerifyPKCS7(cert2.bCertificate, cert2.dwLength, argv[1], cert2.dwLength);
    }
    

    const char* peFilePath = "YourApp.exe";

    // 2. 解析签名
    //PKCS7* pkcs7 = ParseSignature(raw_entry);
    //if (!pkcs7) return 1;

    //// 3. 计算文件哈希（排除签名部分）
    //std::vector<unsigned char> fileHash = CalculateFileHash(peFilePath, raw_entry.size());

    
    //// 4. 验证签名
    //if (VerifySignature(pkcs7, fileHash)) {
    //    std::cout << "✅ 签名验证成功" << std::endl;
    //}
    //else {
    //    std::cout << "❌ 签名验证失败" << std::endl;
    //}

    // 5. 清理 OpenSSL 对象
    // PKCS7_free(pkcs7);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    return 0;
}
