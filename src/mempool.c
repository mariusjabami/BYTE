#include "mempool.h"
#include "wallet.h"
#include "balance.h"
#include "chain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLR_RESET   "\033[0m"
#define CLR_GRAY    "\033[90m"
#define CLR_RED     "\033[31m"

extern void p2p_broadcast_tx(const Transaction *tx);

typedef struct {
    Transaction *txs;
    uint32_t count;
    uint32_t max;
} Mempool;

static Mempool pool;

/* Verifica se uma transação já foi incluída em algum bloco */
static int tx_already_mined(const char *tx_hash) {
    Block latest = get_latest_block();
    uint32_t start = (latest.height > 100) ? latest.height - 100 : 0;
    for (uint32_t h = latest.height; h > start; h--) {
        Block b = get_block(h);
        if (b.height == UINT32_MAX) continue;
        for (uint32_t i = 0; i < b.tx_count; i++) {
            if (strcmp(b.txs[i].hash, tx_hash) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void mempool_init(void) {
    pool.max = MEMPOOL_MAX;
    pool.count = 0;
    pool.txs = (Transaction*)malloc(sizeof(Transaction) * pool.max);
    if (pool.txs == NULL) {
        fprintf(stderr, "[mempool] Erro ao alocar memoria\n");
        pool.max = 0;
    }
}

uint32_t mempool_count(void) { return pool.count; }

int mempool_has(const char *tx_hash) {
    for (uint32_t i = 0; i < pool.count; i++) {
        if (strcmp(pool.txs[i].hash, tx_hash) == 0) return 1;
    }
    return 0;
}

int mempool_add(const Transaction *tx) {
    if (pool.txs == NULL) return 0;
    if (pool.count >= pool.max) return 0;
    
    /* 1. Verifica se já está na mempool */
    if (mempool_has(tx->hash)) return 0;
    
    /* 2. Verifica se já foi minerada */
    if (tx_already_mined(tx->hash)) {
        printf(CLR_GRAY "  ⊗ Tx ja minerada: %.10s...\n" CLR_RESET, tx->hash);
        return 0;
    }
    
    /* 3. Valida a transação (hash + assinatura) */
    if (!validate_transaction((Transaction*)tx)) {
        printf(CLR_RED "  ✗ Tx inválida: %.10s...\n" CLR_RESET, tx->hash);
        return 0;
    }
    
    /* 4. Verifica assinatura ECDSA */
    if (tx->signature_len > 0 && !wallet_verify(tx)) {
        printf(CLR_RED "  ✗ Assinatura inválida: %.10s...\n" CLR_RESET, tx->hash);
        return 0;
    }
    
    /* Exceção para endereços do sistema */
    if (strcmp(tx->to, "STAKE_LOCK") == 0 || strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0) {
        memcpy(&pool.txs[pool.count], tx, sizeof(Transaction));
        pool.count++;
        printf("  ✓ Tx sistema: %.10s...\n", tx->hash);
        return 1;
    }
    /* 5. Verifica saldo (considerando taxa mínima) */
    uint64_t min_fee = 1; /* Taxa mínima */
    if (get_balance(tx->from) < tx->amount + min_fee) {
        printf(CLR_RED "  ✗ Saldo insuficiente: %.10s...\n" CLR_RESET, tx->hash);
        return 0;
    }

    memcpy(&pool.txs[pool.count], tx, sizeof(Transaction));
    pool.count++;

    printf(CLR_GRAY "  ↓ Tx: %.10s... (%u na mempool)\n" CLR_RESET, tx->hash, pool.count);
    p2p_broadcast_tx(tx);
    return 1;
}

void mempool_remove(const char *tx_hash) {
    for (uint32_t i = 0; i < pool.count; i++) {
        if (strcmp(pool.txs[i].hash, tx_hash) == 0) {
            pool.count--;
            if (i < pool.count) memcpy(&pool.txs[i], &pool.txs[pool.count], sizeof(Transaction));
            return;
        }
    }
}

uint32_t mempool_select(Transaction *txs, uint32_t max) {
    if (pool.txs == NULL) return 0;
    uint32_t n = pool.count < max ? pool.count : max;
    for (uint32_t i = 0; i < n; i++) memcpy(&txs[i], &pool.txs[i], sizeof(Transaction));
    return n;
}

void mempool_free(void) {
    if (pool.txs != NULL) { free(pool.txs); pool.txs = NULL; }
    pool.count = 0;
    pool.max = 0;
}
