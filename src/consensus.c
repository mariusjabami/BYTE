#include "consensus.h"
#include "balance.h"
#include "mempool.h"
#include "chain.h"
#include "wallet.h"
#include "p2p.h"
#include "tokenomics.h"
#include "crypto_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sqlite3.h>
#define CLR_RESET   "\033[0m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_RED     "\033[31m"
#define CLR_GRAY    "\033[90m"
#define SHA256_DIGEST_LENGTH 32

extern sqlite3 *db;
static char selected_validator[65];

static void release_matured_stakes(uint32_t current_height) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT address, amount FROM stakes WHERE unlock_height <= ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, current_height);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *address = (const char*)sqlite3_column_text(stmt, 0);
            uint64_t amount = (uint64_t)sqlite3_column_int64(stmt, 1);
            sqlite3_stmt *u = NULL;
            sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance + ? WHERE address = ?;", -1, &u, NULL);
            sqlite3_bind_int64(u, 1, (sqlite3_int64)amount);
            sqlite3_bind_text(u, 2, address, -1, SQLITE_STATIC);
            sqlite3_step(u); sqlite3_finalize(u);
            char amt_str[64]; format_lbxc(amount, amt_str, sizeof(amt_str));
            printf(CLR_GREEN "  ✓ Stake liberado: %s +%s\n" CLR_RESET, address, amt_str);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "DELETE FROM stakes WHERE unlock_height <= (SELECT MAX(height) FROM blocks);", NULL, NULL, NULL);
}

int consensus_init(void) {
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS validators (address TEXT PRIMARY KEY, stake INTEGER NOT NULL, since INTEGER NOT NULL, last_block INTEGER DEFAULT 0, blocks_mined INTEGER DEFAULT 0);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS stakes (address TEXT, amount INTEGER NOT NULL, unlock_height INTEGER NOT NULL);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS slashed (address TEXT PRIMARY KEY, reason TEXT, block_height INTEGER, severity INTEGER DEFAULT 1);", NULL, NULL, NULL);
    return 1;
}

int stake(const char *address, uint64_t amount) {
    if (amount < MIN_STAKE) { printf(CLR_YELLOW "  Stake minimo: 0.00010000 BYTE\n" CLR_RESET); return 0; }
    if (amount > MAX_STAKE) { printf(CLR_YELLOW "  Stake maximo: 100000 BYTE\n" CLR_RESET); return 0; }
    if (get_balance(address) < amount) return 0;
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "SELECT address FROM slashed WHERE address = ?;", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) { sqlite3_finalize(stmt); return 0; }
    sqlite3_finalize(stmt);
    
    /* Criar transação interna para STAKE_LOCK (sem assinatura, como coinbase) */
    Transaction tx; memset(&tx, 0, sizeof(tx));
    strncpy(tx.from, address, 64);
    strncpy(tx.to, "STAKE_LOCK", 64);
    tx.amount = amount;
    tx.fee = 0;
    tx.timestamp = (uint64_t)time(NULL);
    hash_transaction(&tx, tx.hash);
    tx.signature_len = 0;
    
    if (!mempool_add(&tx)) return 0;
    
    /* Atualizar validador local */
    uint64_t current_stake = get_stake(address);
    uint64_t new_stake = current_stake + amount;
    Block latest = get_latest_block();
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO validators (address, stake, since) VALUES (?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)new_stake);
    sqlite3_bind_int(stmt, 3, latest.height);
    sqlite3_step(stmt); sqlite3_finalize(stmt);
    
    printf(CLR_GREEN "  ✓ Stake: +%llu BYTE (total: %llu BYTE) - na mempool\n" CLR_RESET,
           (unsigned long long)amount, (unsigned long long)new_stake);
    return 1;
}

int unstake(const char *address) {
    uint64_t current_stake = get_stake(address);
    if (current_stake == 0) return 0;
    Block latest = get_latest_block();
    uint32_t unlock_height = latest.height + UNSTAKE_DELAY;
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "INSERT INTO stakes (address, amount, unlock_height) VALUES (?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)current_stake);
    sqlite3_bind_int(stmt, 3, unlock_height);
    sqlite3_step(stmt); sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, "DELETE FROM validators WHERE address = ?;", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
    sqlite3_step(stmt); sqlite3_finalize(stmt);
    printf(CLR_YELLOW "  ⊘ Unstake: liberado no bloco %u\n" CLR_RESET, unlock_height);
    return 1;
}

uint64_t get_stake(const char *address) {
    sqlite3_stmt *stmt = NULL; uint64_t stake_amount = 0;
    if (sqlite3_prepare_v2(db, "SELECT stake FROM validators WHERE address = ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) stake_amount = (uint64_t)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt); return stake_amount;
}

int is_validator(const char *address) { return get_stake(address) >= MIN_STAKE; }

uint32_t get_validator_count(void) {
    sqlite3_stmt *stmt = NULL; uint32_t count = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM validators WHERE stake >= ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, MIN_STAKE);
        if (sqlite3_step(stmt) == SQLITE_ROW) count = (uint32_t)sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt); return count;
}

int get_validators(Validator *validators, uint32_t max) {
    sqlite3_stmt *stmt = NULL; uint32_t count = 0;
    if (sqlite3_prepare_v2(db, "SELECT address, stake, since, last_block, blocks_mined FROM validators WHERE stake >= ? ORDER BY stake DESC LIMIT ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, MIN_STAKE); sqlite3_bind_int(stmt, 2, max);
        while (sqlite3_step(stmt) == SQLITE_ROW && count < max) {
            strncpy(validators[count].address, (const char*)sqlite3_column_text(stmt, 0), 64); validators[count].address[64] = '\0';
            validators[count].stake = (uint64_t)sqlite3_column_int64(stmt, 1); validators[count].since = (uint32_t)sqlite3_column_int(stmt, 2);
            validators[count].last_block = (uint32_t)sqlite3_column_int(stmt, 3); validators[count].blocks_mined = (uint32_t)sqlite3_column_int(stmt, 4);
            count++;
        }
    }
    sqlite3_finalize(stmt); return count;
}

const char* select_validator(uint32_t block_height, const char *prev_hash) {
    Validator validators[MAX_VALIDATORS]; uint32_t count = get_validators(validators, MAX_VALIDATORS);
    if (count == 0) return NULL;
    uint32_t seed = 0;
    for (int i = 0; i < 8 && prev_hash[i*2]; i++) { char hex[3] = {prev_hash[i*2], prev_hash[i*2+1], 0}; seed = (seed << 4) | (strtol(hex, NULL, 16) & 0xF); }
    seed ^= block_height; srand(seed);
    uint64_t total_stake = 0; for (uint32_t i = 0; i < count; i++) total_stake += validators[i].stake;
    for (int attempt = 0; attempt < 10; attempt++) {
        uint64_t random = ((uint64_t)rand() << 32 | rand()) % total_stake; uint64_t accumulator = 0;
        for (uint32_t i = 0; i < count; i++) { accumulator += validators[i].stake;
            if (random < accumulator) { if (validators[i].last_block != block_height - 1 || count == 1) { strncpy(selected_validator, validators[i].address, 64); selected_validator[64] = '\0'; return selected_validator; } break; }
        }
    }
    strncpy(selected_validator, validators[0].address, 64); selected_validator[64] = '\0'; return selected_validator;
}

int create_next_block(const char *validator_address) {
    Transaction all_txs[MAX_TX_PER_BLOCK], valid_txs[MAX_TX_PER_BLOCK];
    char mined_hashes[MAX_TX_PER_BLOCK][65];
    uint32_t all_count = mempool_select(all_txs, MAX_TX_PER_BLOCK), valid_count = 0;
    uint64_t total_fees = 0; uint32_t active_validators = get_validator_count();
    for (uint32_t i = 0; i < all_count && valid_count < MAX_TX_PER_BLOCK - 1; i++) {
        int tx_involves_validator = (strcmp(all_txs[i].from, validator_address) == 0 || strcmp(all_txs[i].to, validator_address) == 0);
        if (tx_involves_validator && active_validators > 1) continue;
        memcpy(&valid_txs[valid_count], &all_txs[i], sizeof(Transaction));
        strncpy(mined_hashes[valid_count], all_txs[i].hash, 64); mined_hashes[valid_count][64] = '\0';
        total_fees += valid_txs[valid_count].fee; valid_count++;
    }
    Block latest = get_latest_block();
    if (valid_count == 0 && latest.height > 0) return 0;
    release_matured_stakes(latest.height + 1);
    uint64_t block_reward = calculate_block_reward(latest.height + 1);
    uint64_t my_stake = get_stake(validator_address), stake_bonus = 0;
    if (my_stake > 0) { sqlite3_stmt *stmt = NULL; uint64_t max_stake = 0;
        if (sqlite3_prepare_v2(db, "SELECT MAX(stake) FROM validators;", -1, &stmt, NULL) == SQLITE_OK) { if (sqlite3_step(stmt) == SQLITE_ROW) max_stake = (uint64_t)sqlite3_column_int64(stmt, 0); }
        sqlite3_finalize(stmt); if (max_stake > 0) stake_bonus = (block_reward / 2) * my_stake / max_stake; }
    uint64_t total_reward = block_reward + stake_bonus + (total_fees / 2);
    Transaction reward; memset(&reward, 0, sizeof(Transaction));
    strcpy(reward.from, "BYTE000000000000000000000000000000000000000000000000000000000000"); strcpy(reward.to, validator_address);
    reward.amount = total_reward; reward.fee = 0; reward.timestamp = (uint64_t)time(NULL); reward.signature_len = 0;
    char tx_data[512]; snprintf(tx_data, sizeof(tx_data), "%s%s%llu%llu%llu", reward.from, reward.to, (unsigned long long)reward.amount, (unsigned long long)reward.fee, (unsigned long long)reward.timestamp);
    unsigned char tx_hash[32]; sha256((unsigned char*)tx_data, strlen(tx_data), tx_hash);
    for (int j = 0; j < 32; j++) sprintf(reward.hash + j*2, "%02x", tx_hash[j]); reward.hash[64] = '\0';
    valid_txs[valid_count] = reward; valid_count++;
    Block block; init_block(&block); for (uint32_t i = 0; i < valid_count; i++) block.txs[i] = valid_txs[i]; block.tx_count = valid_count;
    strncpy(block.prev_hash, latest.hash, 64); block.prev_hash[64] = '\0'; block.height = latest.height + 1; block.timestamp = (uint64_t)time(NULL);
    char block_data[2048]; int offset = snprintf(block_data, sizeof(block_data), "%s%u%llu", block.prev_hash, block.height, (unsigned long long)block.timestamp);
    for (uint32_t i = 0; i < block.tx_count && offset < (int)sizeof(block_data) - 70; i++) offset += snprintf(block_data + offset, sizeof(block_data) - offset, "%s", block.txs[i].hash);
    unsigned char block_hash[32]; sha256((unsigned char*)block_data, strlen(block_data), block_hash);
    for (int j = 0; j < 32; j++) sprintf(block.hash + j*2, "%02x", block_hash[j]); block.hash[64] = '\0';
    if (add_block(&block)) {
        for (uint32_t i = 0; i < valid_count - 1; i++) mempool_remove(mined_hashes[i]);
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "UPDATE validators SET last_block = ?, blocks_mined = blocks_mined + 1 WHERE address = ?;", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, block.height); sqlite3_bind_text(stmt, 2, validator_address, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
        p2p_broadcast_block(&block);
        char rw_str[64], bn_str[64], tf_str[64];
        format_lbxc(block_reward, rw_str, sizeof(rw_str)); format_lbxc(stake_bonus, bn_str, sizeof(bn_str)); format_lbxc(total_fees, tf_str, sizeof(tf_str));
        printf(CLR_GREEN "  ⛏  Bloco #%u | Recompensa: %s (+%s) | Taxas: %s | Tx: %u\n" CLR_RESET, block.height, rw_str, bn_str, tf_str, block.tx_count);
        return 1;
    }
    return 0;
}

int slash(const char *address, const char *reason, uint32_t severity) {
    uint64_t staked = get_stake(address); if (staked == 0) return 0;
    uint64_t slash_percent = SLASH_BASE_PERCENT * severity; if (slash_percent > SLASH_MAX_PERCENT) slash_percent = SLASH_MAX_PERCENT;
    uint64_t burn = (staked * slash_percent) / 100; if (burn < MIN_SLASH_AMOUNT) burn = MIN_SLASH_AMOUNT; if (burn > MAX_SLASH_AMOUNT) burn = MAX_SLASH_AMOUNT; if (burn > staked) burn = staked;
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "DELETE FROM validators WHERE address = ?;", -1, &stmt, NULL); sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, "INSERT INTO slashed (address, reason, block_height, severity) VALUES (?, ?, ?, ?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC); sqlite3_bind_text(stmt, 2, reason, -1, SQLITE_STATIC); sqlite3_bind_int(stmt, 3, get_latest_block().height); sqlite3_bind_int(stmt, 4, severity);
    sqlite3_step(stmt); sqlite3_finalize(stmt);
    uint64_t remaining = staked - burn;
    if (remaining > 0) { sqlite3_prepare_v2(db, "UPDATE balances SET balance = balance + ? WHERE address = ?;", -1, &stmt, NULL); sqlite3_bind_int64(stmt, 1, (sqlite3_int64)remaining); sqlite3_bind_text(stmt, 2, address, -1, SQLITE_STATIC); sqlite3_step(stmt); sqlite3_finalize(stmt); }
    if (burn > 0) { sqlite3_prepare_v2(db, "INSERT INTO balances (address, balance) VALUES (?, ?) ON CONFLICT(address) DO UPDATE SET balance = balance + ?;", -1, &stmt, NULL); sqlite3_bind_text(stmt, 1, "BYTE000000000000000000000000000000000000000000000000000000000000", -1, SQLITE_STATIC); sqlite3_bind_int64(stmt, 2, (sqlite3_int64)burn); sqlite3_bind_int64(stmt, 3, (sqlite3_int64)burn); sqlite3_step(stmt); sqlite3_finalize(stmt); }
    char b_str[64]; format_lbxc(burn, b_str, sizeof(b_str));
    printf(CLR_RED "  ⚠ SLASH: %s -%s (%llu%%) - %s\n" CLR_RESET, address, b_str, (unsigned long long)slash_percent, reason);
    return 1;
}

void detect_double_sign(uint32_t height, const char *validator, const char *hash1, const char *hash2) {
    if (strcmp(hash1, hash2) != 0) { printf(CLR_RED "  ⚠ DOUBLE-SIGN: %s na altura %u!\n" CLR_RESET, validator, height); slash(validator, "double-sign", 5); }
}

void consensus_loop(void) { while (1) { Block latest = get_latest_block(); const char *validator = select_validator(latest.height + 1, latest.hash); if (validator != NULL) create_next_block(validator); sleep(BLOCK_TIME); } }
