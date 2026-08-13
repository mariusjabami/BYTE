#include "chain.h"
#include "transaction.h"
#include "balance.h"
#include "consensus.h"
#include "wallet.h"
#include "crypto_utils.h"
#include "tokenomics.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#define CLR_RESET   "\033[0m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_CYAN    "\033[36m"
#define CLR_RED     "\033[31m"
#define CLR_GRAY    "\033[90m"

sqlite3 *db = NULL;
#define FUNDADOR_ADDRESS "BYTE0000b667d3d589965536e9d9db10fc73b400c052ec3125396a91e9cdd159"

static int get_block_validator(uint32_t height, char *validator_out) {
    Block b = get_block(height);
    if (b.height == UINT32_MAX || b.tx_count == 0) return 0;
    Transaction *coinbase = &b.txs[b.tx_count - 1];
    if (strcmp(coinbase->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0) {
        strncpy(validator_out, coinbase->to, 64);
        validator_out[64] = '\0';
        return 1;
    }
    return 0;
}

static void load_transactions(Block *block) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT tx_hash, from_addr, to_addr, amount, fee, timestamp, public_key, signature, sig_len FROM transactions WHERE block_height = ? ORDER BY rowid;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, block->height); block->tx_count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW && block->tx_count < MAX_TX_PER_BLOCK) {
            Transaction *tx = &block->txs[block->tx_count]; memset(tx, 0, sizeof(Transaction));
            const char *hash = (const char*)sqlite3_column_text(stmt, 0); if (hash) strncpy(tx->hash, hash, 64);
            const char *from = (const char*)sqlite3_column_text(stmt, 1); if (from) strncpy(tx->from, from, 64);
            const char *to   = (const char*)sqlite3_column_text(stmt, 2); if (to)   strncpy(tx->to, to, 64);
            tx->amount=(uint64_t)sqlite3_column_int64(stmt,3); tx->fee=(uint64_t)sqlite3_column_int64(stmt,4); tx->timestamp=(uint64_t)sqlite3_column_int64(stmt,5);
            const void *pub=sqlite3_column_blob(stmt,6); int pub_len=sqlite3_column_bytes(stmt,6); if(pub&&pub_len<=65)memcpy(tx->public_key,pub,pub_len);
            const void *sig=sqlite3_column_blob(stmt,7); int sig_len=sqlite3_column_bytes(stmt,7); if(sig&&sig_len<=72){memcpy(tx->signature,sig,sig_len);tx->signature_len=sig_len;}
            block->tx_count++;
        }
    }
    sqlite3_finalize(stmt);
}

static void revert_block(uint32_t height) {
    Block block = get_block(height); if (block.height == UINT32_MAX) return;
    sqlite3_stmt *stmt = NULL;
    for (uint32_t i = 0; i < block.tx_count; i++) {
        Transaction *tx = &block.txs[i];
        int is_coinbase = (strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0 || strcmp(tx->from, "STAKE_LOCK") == 0);
        if (!is_coinbase) {
            sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance + ? WHERE address = ?;", -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, (sqlite3_int64)(tx->amount + tx->fee)); sqlite3_bind_text(stmt, 2, tx->from, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
            sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance - ? WHERE address = ?;", -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, (sqlite3_int64)tx->amount); sqlite3_bind_text(stmt, 2, tx->to, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
            if (tx->fee > 0) {
                uint64_t burn_amount = (tx->fee * 50) / 100;
                sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance - ? WHERE address = ?;", -1, &stmt, NULL);
                sqlite3_bind_int64(stmt, 1, (sqlite3_int64)burn_amount); sqlite3_bind_text(stmt, 2, "BYTE000000000000000000000000000000000000000000000000000000000000", -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
            }
        } else {
            sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance - ? WHERE address = ?;", -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, (sqlite3_int64)tx->amount); sqlite3_bind_text(stmt, 2, tx->to, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
        }
    }
    sqlite3_prepare_v2(db, "DELETE FROM transactions WHERE block_height = ?;", -1, &stmt, NULL); sqlite3_bind_int(stmt, 1, height); sqlite3_step(stmt); sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, "DELETE FROM blocks WHERE height = ?;", -1, &stmt, NULL); sqlite3_bind_int(stmt, 1, height); sqlite3_step(stmt); sqlite3_finalize(stmt);
}

static void create_genesis_block(void) {
    Block genesis; memset(&genesis, 0, sizeof(Block)); genesis.height=0; genesis.timestamp=0;
    strcpy(genesis.prev_hash, "0000000000000000000000000000000000000000000000000000000000000000");
    genesis.tx_count = 1;
    
    Transaction *coinbase = &genesis.txs[0];
    memset(coinbase, 0, sizeof(Transaction));
    strcpy(coinbase->from, "BYTE000000000000000000000000000000000000000000000000000000000000");
    strcpy(coinbase->to, FUNDADOR_ADDRESS);
    coinbase->amount = FOUNDER_REWARD;  /* 5000 BYTE para o fundador */
    coinbase->timestamp = 0;
    coinbase->signature_len = 0;
    
    char tx_data[512];
    snprintf(tx_data, sizeof(tx_data), "%s%s%llu%llu", coinbase->from, coinbase->to, (unsigned long long)coinbase->amount, (unsigned long long)coinbase->timestamp);
    unsigned char tx_hash[32]; sha256((unsigned char*)tx_data, strlen(tx_data), tx_hash);
    for(int i=0;i<32;i++) sprintf(coinbase->hash + i*2, "%02x", tx_hash[i]);
    coinbase->hash[64] = '\0';
    
    char block_data[1024];
    snprintf(block_data, sizeof(block_data), "%s%u%llu%s", genesis.prev_hash, genesis.height, (unsigned long long)genesis.timestamp, coinbase->hash);
    unsigned char block_hash[32]; sha256((unsigned char*)block_data, strlen(block_data), block_hash);
    for(int i=0;i<32;i++) sprintf(genesis.hash + i*2, "%02x", block_hash[i]);
    genesis.hash[64] = '\0';

    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "INSERT INTO blocks (height, hash, prev_hash, timestamp, tx_count) VALUES (?, ?, ?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, genesis.height);
    sqlite3_bind_text(stmt, 2, genesis.hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, genesis.prev_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)genesis.timestamp);
    sqlite3_bind_int(stmt, 5, genesis.tx_count);
    sqlite3_step(stmt); sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "INSERT INTO transactions (tx_hash, from_addr, to_addr, amount, fee, timestamp, block_height, public_key, signature, sig_len) VALUES (?, ?, ?, ?, 0, ?, ?, ?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, coinbase->hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, coinbase->from, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, coinbase->to, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)coinbase->amount);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)coinbase->timestamp);
    sqlite3_bind_int(stmt, 6, genesis.height);
    sqlite3_bind_blob(stmt, 7, coinbase->public_key, 65, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 8, coinbase->signature, coinbase->signature_len, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, coinbase->signature_len);
    sqlite3_step(stmt); sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO balances (address, balance) VALUES (?, ?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, FUNDADOR_ADDRESS, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)FOUNDER_REWARD);
    sqlite3_step(stmt); sqlite3_finalize(stmt);

    printf(CLR_GREEN "  ⛏  Bloco Genesis criado!\n" CLR_RESET);
    printf(CLR_CYAN "     Supply: 21.000.000 BYTE (fundador: 5.000) -> %s\n" CLR_RESET, FUNDADOR_ADDRESS);
}

int init_chain(void) {
    int rc = sqlite3_open("bytechain.db", &db);
    if (rc != SQLITE_OK) { fprintf(stderr, "[ERRO] SQLite: %s\n", sqlite3_errmsg(db)); return 0; }
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS blocks (height INTEGER PRIMARY KEY, hash TEXT UNIQUE, prev_hash TEXT, timestamp INTEGER, tx_count INTEGER);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS transactions (tx_hash TEXT PRIMARY KEY, from_addr TEXT, to_addr TEXT, amount INTEGER, fee INTEGER DEFAULT 0, timestamp INTEGER, block_height INTEGER, public_key BLOB, signature BLOB, sig_len INTEGER);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS balances (address TEXT PRIMARY KEY, balance INTEGER);", NULL, NULL, NULL);
    sqlite3_stmt *stmt = NULL; int exists = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM blocks WHERE height = 0;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = sqlite3_column_int(stmt, 0) > 0;
        sqlite3_finalize(stmt);
    }
    if (!exists) create_genesis_block();
    else printf("Chain carregada.\n");
    consensus_init();
    return 1;
}

static int supply_exceeded(uint64_t new_coins) {
    sqlite3_stmt *stmt = NULL;
    uint64_t total = 0;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(SUM(amount), 0) FROM transactions WHERE from_addr = ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "BYTE000000000000000000000000000000000000000000000000000000000000", -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) total = (uint64_t)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return (total + new_coins > TOTAL_SUPPLY);
}

static int apply_block(const Block *block) {
    sqlite3_stmt *stmt = NULL;
    for (uint32_t i = 0; i < block->tx_count; i++) {
        const Transaction *tx = &block->txs[i];
        if (strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0 || strcmp(tx->from, "STAKE_LOCK") == 0) {
            if (supply_exceeded(tx->amount)) {
                printf(CLR_RED "  [chain] Supply maximo excedido!\n" CLR_RESET);
                return -1;
            }
        }
    }
    for (uint32_t i = 0; i < block->tx_count; i++) {
        const Transaction *tx = &block->txs[i];
        int is_coinbase = (strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0 || strcmp(tx->from, "STAKE_LOCK") == 0);
        if (!is_coinbase) {
            if (tx->signature_len > 0 && !wallet_verify(tx)) { printf(CLR_RED "  [chain] Assinatura invalida\n" CLR_RESET); return -1; }
            if (get_balance(tx->from) < tx->amount + tx->fee) { 
    printf(CLR_RED "  [chain] Saldo insuficiente: from=%.10s... balance=%llu need=%llu\n" CLR_RESET, 
           tx->from, (unsigned long long)get_balance(tx->from), (unsigned long long)(tx->amount + tx->fee)); 
    return -1; 
}
        }
    }
    sqlite3_prepare_v2(db, "INSERT INTO blocks (height, hash, prev_hash, timestamp, tx_count) VALUES (?, ?, ?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, block->height); sqlite3_bind_text(stmt, 2, block->hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, block->prev_hash, -1, SQLITE_STATIC); sqlite3_bind_int64(stmt, 4, (sqlite3_int64)block->timestamp);
    sqlite3_bind_int(stmt, 5, block->tx_count);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); return -1; }
    sqlite3_finalize(stmt);
    for (uint32_t i = 0; i < block->tx_count; i++) {
        const Transaction *tx = &block->txs[i];
        sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO transactions (tx_hash, from_addr, to_addr, amount, fee, timestamp, block_height, public_key, signature, sig_len) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, tx->hash, -1, SQLITE_STATIC); sqlite3_bind_text(stmt, 2, tx->from, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, tx->to, -1, SQLITE_STATIC); sqlite3_bind_int64(stmt, 4, (sqlite3_int64)tx->amount);
        sqlite3_bind_int64(stmt, 5, (sqlite3_int64)tx->fee); sqlite3_bind_int64(stmt, 6, (sqlite3_int64)tx->timestamp);
        sqlite3_bind_int(stmt, 7, block->height); sqlite3_bind_blob(stmt, 8, tx->public_key, 65, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 9, tx->signature, tx->signature_len, SQLITE_STATIC); sqlite3_bind_int(stmt, 10, tx->signature_len);
        sqlite3_step(stmt); sqlite3_finalize(stmt);
        int is_coinbase = (strcmp(tx->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0 || strcmp(tx->from, "STAKE_LOCK") == 0);
        if (!is_coinbase) {
            sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance - ? WHERE address = ?;", -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, (sqlite3_int64)(tx->amount + tx->fee)); sqlite3_bind_text(stmt, 2, tx->from, -1, SQLITE_STATIC);
            sqlite3_step(stmt); sqlite3_finalize(stmt);
            sqlite3_prepare_v2(db, "INSERT INTO balances (address, balance) VALUES (?, ?) ON CONFLICT(address) DO UPDATE SET balance = balance + ?;", -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, tx->to, -1, SQLITE_STATIC); sqlite3_bind_int64(stmt, 2, (sqlite3_int64)tx->amount);
            sqlite3_bind_int64(stmt, 3, (sqlite3_int64)tx->amount); sqlite3_step(stmt); sqlite3_finalize(stmt);
            if (tx->fee > 0) {
                uint64_t burn_amount = (tx->fee * 50) / 100;
                if (burn_amount > 0) {
                    sqlite3_prepare_v2(db, "INSERT INTO balances (address, balance) VALUES (?, ?) ON CONFLICT(address) DO UPDATE SET balance = balance + ?;", -1, &stmt, NULL);
                    sqlite3_bind_text(stmt, 1, "BYTE000000000000000000000000000000000000000000000000000000000000", -1, SQLITE_STATIC);
                    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)burn_amount); sqlite3_bind_int64(stmt, 3, (sqlite3_int64)burn_amount);
                    sqlite3_step(stmt); sqlite3_finalize(stmt);
                }
            }
        } else {
            sqlite3_prepare_v2(db, "INSERT INTO balances (address, balance) VALUES (?, ?) ON CONFLICT(address) DO UPDATE SET balance = balance + ?;", -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, tx->to, -1, SQLITE_STATIC); sqlite3_bind_int64(stmt, 2, (sqlite3_int64)tx->amount);
            sqlite3_bind_int64(stmt, 3, (sqlite3_int64)tx->amount); sqlite3_step(stmt); sqlite3_finalize(stmt);
        }
    }
    return 1;
}

int add_block(const Block *block) {
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM blocks WHERE hash = ?;", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, block->hash, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) { sqlite3_finalize(stmt); return 0; }
    sqlite3_finalize(stmt);
    Block latest = get_latest_block();
    if (block->height == latest.height + 1) {
        if (strcmp(block->prev_hash, latest.hash) != 0) return 0;
        return apply_block(block);
    }
    if (block->height <= latest.height) {
        Block existing = get_block(block->height);
        if (existing.height != UINT32_MAX && strcmp(existing.hash, block->hash) == 0) return 0;
        char val1[65] = {0}, val2[65] = {0};
        int has_val1 = get_block_validator(block->height, val1);
        int has_val2 = 0;
        if (block->tx_count > 0) {
            const Transaction *coinbase = &block->txs[block->tx_count - 1];
            if (strcmp(coinbase->from, "BYTE000000000000000000000000000000000000000000000000000000000000") == 0) {
                strncpy(val2, coinbase->to, 64); val2[64] = '\0'; has_val2 = 1;
            }
        }
        if (has_val1 && has_val2 && strcmp(val1, val2) == 0) {
            printf(CLR_RED "  ⚠ DOUBLE-SIGN: %s na altura %u!\n" CLR_RESET, val1, block->height);
            slash(val1, "double-sign", 5);
            return -1;
        }
        if (strcmp(block->hash, existing.hash) < 0) {
            printf(CLR_YELLOW "  [chain] Reorganizando altura %u (hash menor)\n" CLR_RESET, block->height);
            revert_block(block->height);
            return apply_block(block);
        }
        return 0;
    }
    return 0;
}

Block get_latest_block(void) {
    Block block; init_block(&block);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT height, hash, prev_hash, timestamp, tx_count FROM blocks ORDER BY height DESC LIMIT 1;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            block.height = (uint32_t)sqlite3_column_int(stmt, 0);
            const char *h = (const char*)sqlite3_column_text(stmt, 1); if (h) strncpy(block.hash, h, 64);
            const char *p = (const char*)sqlite3_column_text(stmt, 2); if (p) strncpy(block.prev_hash, p, 64);
            block.timestamp = (uint64_t)sqlite3_column_int64(stmt, 3);
            block.tx_count = (uint32_t)sqlite3_column_int(stmt, 4);
        }
    }
    sqlite3_finalize(stmt);
    if (block.height != UINT32_MAX) load_transactions(&block);
    return block;
}

Block get_block(uint32_t height) {
    Block block; init_block(&block);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT height, hash, prev_hash, timestamp, tx_count FROM blocks WHERE height = ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, height);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            block.height = (uint32_t)sqlite3_column_int(stmt, 0);
            const char *h = (const char*)sqlite3_column_text(stmt, 1); if (h) strncpy(block.hash, h, 64);
            const char *p = (const char*)sqlite3_column_text(stmt, 2); if (p) strncpy(block.prev_hash, p, 64);
            block.timestamp = (uint64_t)sqlite3_column_int64(stmt, 3);
            block.tx_count = (uint32_t)sqlite3_column_int(stmt, 4);
        }
    }
    sqlite3_finalize(stmt);
    if (block.height != UINT32_MAX) load_transactions(&block);
    return block;
}

void close_chain(void) { if (db) { sqlite3_close(db); db = NULL; } }
