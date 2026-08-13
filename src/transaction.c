#include "transaction.h"
#include "wallet.h"
#include <string.h>
#include <stdio.h>
#include <openssl/sha.h>

void hash_transaction(Transaction *tx, char *output_hash){
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    /* Inclui fee no hash para evitar adulteração */
    snprintf(buffer, sizeof(buffer), "%s%s%llu%llu%llu", 
             tx->from, tx->to, 
             (unsigned long long)tx->amount,
             (unsigned long long)tx->fee,
             (unsigned long long)tx->timestamp);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)buffer, strlen(buffer), hash);

    for(int i = 0; i < 32; i++){
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }
    output_hash[64] = '\0';
}

void create_transaction(Transaction *tx, const char *from, const char *to, 
                        uint64_t amount, uint64_t timestamp) {
    memset(tx, 0, sizeof(*tx));

    strncpy(tx->from, from, ADDRESS_SIZE - 1);
    tx->from[ADDRESS_SIZE - 1] = '\0';

    strncpy(tx->to, to, ADDRESS_SIZE - 1);
    tx->to[ADDRESS_SIZE - 1] = '\0';

    tx->amount = amount;
    tx->fee = 0;  /* Será preenchido pelo minerador */
    tx->timestamp = timestamp;

    hash_transaction(tx, tx->hash);
}

int validate_transaction(Transaction *tx){
    /* 1. Campos obrigatórios */
    if(strlen(tx->from) == 0) return 0;
    if(strlen(tx->to) == 0) return 0;
    if(tx->amount == 0) return 0;
    
    /* 2. Hash integrity (recalcula e compara) */
    char recalculated_hash[65];
    hash_transaction(tx, recalculated_hash);
    if(strcmp(tx->hash, recalculated_hash) != 0) return 0;
    
    /* 3. Verifica assinatura ECDSA (se tiver assinatura) */
    if(tx->signature_len > 0) {
        if(!wallet_verify(tx)) {
            printf("  ✗ Assinatura inválida: %.10s...\n", tx->hash);
            return 0;
        }
    }
    
    /* 4. Não permite transações do endereço de queima como remetente */
    if(strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0) {
        return 0; /* Coinbase só pode ser criada por minerador */
    }
    
    return 1;
}

void print_transaction(Transaction *tx){
    printf("FROM: %s\n", tx->from);
    printf("TO: %s\n", tx->to);
    printf("AMOUNT: %llu\n", (unsigned long long)tx->amount);
    printf("FEE: %llu\n", (unsigned long long)tx->fee);
    printf("TIME: %llu\n", (unsigned long long)tx->timestamp);
}
