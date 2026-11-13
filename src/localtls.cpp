#include "localtls.h"
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>
#include <memory>
#include <utility>

bool LocalTls::generate(QByteArray &certificate, QByteArray &privateKey) {
    certificate.clear(); privateKey.clear();
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA,nullptr),EVP_PKEY_CTX_free);
    if (!context || EVP_PKEY_keygen_init(context.get())<=0 || EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(),2048)<=0) return false;
    EVP_PKEY *generated=nullptr;
    const int result=EVP_PKEY_keygen(context.get(),&generated);
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(generated,EVP_PKEY_free);
    if (result<=0 || !key) return false;
    std::unique_ptr<X509, decltype(&X509_free)> cert(X509_new(),X509_free);
    unsigned char bytes[16];
    if (!cert || RAND_bytes(bytes,sizeof(bytes))!=1) return false;
    bytes[0]|=0x01; // Nonzero, random positive 128-bit serial.
    std::unique_ptr<BIGNUM, decltype(&BN_free)> serial(BN_bin2bn(bytes,sizeof(bytes),nullptr),BN_free);
    if (!serial || !BN_to_ASN1_INTEGER(serial.get(),X509_get_serialNumber(cert.get()))
        || X509_set_version(cert.get(),2)!=1
        || !X509_gmtime_adj(X509_getm_notBefore(cert.get()),-300)
        || !X509_gmtime_adj(X509_getm_notAfter(cert.get()),3650L*24*60*60)
        || X509_set_pubkey(cert.get(),key.get())!=1) return false;
    auto *subject=X509_get_subject_name(cert.get());
    if (!subject || X509_NAME_add_entry_by_txt(subject,"CN",MBSTRING_ASC,
            reinterpret_cast<const unsigned char *>("ABM Local Companion"),-1,-1,0)!=1
        || X509_set_issuer_name(cert.get(),subject)!=1) return false;
    X509V3_CTX extensions;
    X509V3_set_ctx(&extensions,cert.get(),cert.get(),nullptr,nullptr,0);
    for (const auto &entry : {std::pair<int,const char *>{NID_basic_constraints,"critical,CA:FALSE"},
            {NID_key_usage,"critical,digitalSignature,keyEncipherment"}, {NID_ext_key_usage,"serverAuth"}}) {
        std::unique_ptr<X509_EXTENSION, decltype(&X509_EXTENSION_free)> extension(
            X509V3_EXT_conf_nid(nullptr,&extensions,entry.first,entry.second),X509_EXTENSION_free);
        if (!extension || X509_add_ext(cert.get(),extension.get(),-1)!=1) return false;
    }
    if (X509_sign(cert.get(),key.get(),EVP_sha256())<=0) return false;
    std::unique_ptr<BIO, decltype(&BIO_free)> certPem(BIO_new(BIO_s_mem()),BIO_free), keyPem(BIO_new(BIO_s_mem()),BIO_free);
    if (!certPem || !keyPem || PEM_write_bio_X509(certPem.get(),cert.get())!=1
        || PEM_write_bio_PrivateKey(keyPem.get(),key.get(),nullptr,nullptr,0,nullptr,nullptr)!=1) return false;
    char *certData=nullptr, *keyData=nullptr;
    const auto certSize=BIO_get_mem_data(certPem.get(),&certData), keySize=BIO_get_mem_data(keyPem.get(),&keyData);
    if (certSize<=0 || keySize<=0) return false;
    certificate=QByteArray(certData,certSize); privateKey=QByteArray(keyData,keySize);
    return true;
}
