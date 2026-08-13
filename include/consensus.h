#ifndef CONSENSUS_H
#define CONSENSUS_H
#include <stdint.h>
#include "block.h"

#define BLOCK_TIME         30
#define UNSTAKE_DELAY      100
#define MAX_VALIDATORS     100

typedef struct {
    char address[65];
    uint64_t stake;
    uint32_t since;
    uint32_t last_block;
    uint32_t blocks_mined;
} Validator;

int consensus_init(void);
int stake(const char *address, uint64_t amount);
int unstake(const char *address);
uint64_t get_stake(const char *address);
int is_validator(const char *address);
uint32_t get_validator_count(void);
int get_validators(Validator *validators, uint32_t max);
const char* select_validator(uint32_t block_height, const char *prev_hash);
int create_next_block(const char *validator_address);
int slash(const char *address, const char *reason, uint32_t severity);
void detect_double_sign(uint32_t height, const char *validator, const char *hash1, const char *hash2);
void consensus_loop(void);
#endif
