#include "wallet.h"
#include "crypto_utils.h"
#include <stdio.h>
#include <string.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

int wallet_generate(unsigned char *private_key, unsigned char *public_key, char *address) {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) return 0;
    EC_KEY_generate_key(key);
    
    const BIGNUM *priv_bn = EC_KEY_get0_private_key(key);
    BN_bn2bin(priv_bn, private_key);
    
    const EC_POINT *pub_pt = EC_KEY_get0_public_key(key);
    const EC_GROUP *group = EC_KEY_get0_group(key);
    EC_POINT_point2oct(group, pub_pt, POINT_CONVERSION_UNCOMPRESSED, public_key, 65, NULL);
    
    unsigned char sha[32];
    sha256(public_key, 65, sha);
    
    strcpy(address, "BYTE");
    for(int i = 0; i < 30; i++) sprintf(address + 4 + i*2, "%02x", sha[i]);
    address[64] = '\0';
    
    EC_KEY_free(key);
    return 1;
}

int wallet_save(const char *filename, const unsigned char *private_key, 
                const unsigned char *public_key, const char *address) {
    FILE *f = fopen(filename, "wb");
    if(!f) return 0;
    fwrite(private_key, 1, 32, f);
    fwrite(public_key, 1, 65, f);
    fwrite(address, 1, 65, f);
    fclose(f);
    return 1;
}

int wallet_load(const char *filename, unsigned char *private_key, 
                unsigned char *public_key, char *address) {
    FILE *f = fopen(filename, "rb");
    if(!f) return 0;
    
    if(fread(private_key, 1, 32, f) != 32 ||
       fread(public_key, 1, 65, f) != 65 ||
       fread(address, 1, 65, f) != 65) {
        fclose(f);
        return 0;
    }
    
    fclose(f);
    return 1;
}

int wallet_sign(Transaction *tx, const unsigned char *private_key, 
                const unsigned char *public_key) {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) return 0;
    
    BIGNUM *priv_bn = BN_bin2bn(private_key, 32, NULL);
    EC_KEY_set_private_key(key, priv_bn);
    
    EC_POINT *pub_pt = EC_POINT_new(EC_KEY_get0_group(key));
    EC_POINT_oct2point(EC_KEY_get0_group(key), pub_pt, public_key, 65, NULL);
    EC_KEY_set_public_key(key, pub_pt);
    
    unsigned char hash[32];
    char data[512];
    /* Inclui fee no hash da assinatura */
    snprintf(data, sizeof(data), "%s%s%llu%llu%llu", 
             tx->from, tx->to, 
             (unsigned long long)tx->amount,
             (unsigned long long)tx->fee,
             (unsigned long long)tx->timestamp);
    sha256((unsigned char*)data, strlen(data), hash);
    
    tx->signature_len = ECDSA_size(key);
    if (tx->signature_len > 72) tx->signature_len = 72;
    ECDSA_sign(0, hash, 32, tx->signature, &tx->signature_len, key);
    
    memcpy(tx->public_key, public_key, 65);
    
    BN_free(priv_bn);
    EC_POINT_free(pub_pt);
    EC_KEY_free(key);
    return 1;
}

int wallet_verify(const Transaction *tx) {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) return 0;
    
    EC_POINT *pub_pt = EC_POINT_new(EC_KEY_get0_group(key));
    EC_POINT_oct2point(EC_KEY_get0_group(key), pub_pt, tx->public_key, 65, NULL);
    EC_KEY_set_public_key(key, pub_pt);
    
    unsigned char hash[32];
    char data[512];
    /* Inclui fee no hash da verificação */
    snprintf(data, sizeof(data), "%s%s%llu%llu%llu",
             tx->from, tx->to,
             (unsigned long long)tx->amount,
             (unsigned long long)tx->fee,
             (unsigned long long)tx->timestamp);
    sha256((unsigned char*)data, strlen(data), hash);
    
    int ret = ECDSA_verify(0, hash, 32, tx->signature, tx->signature_len, key);
    
    EC_POINT_free(pub_pt);
    EC_KEY_free(key);
    
    return ret == 1;
}
