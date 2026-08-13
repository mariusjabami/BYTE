#ifndef TOKENOMICS_H
#define TOKENOMICS_H
#include <stdint.h>
#include <stddef.h>

#define SATOSHI               100000000ULL
#define TOTAL_SUPPLY          (21000000ULL * SATOSHI)
#define DECIMALS              8
#define FOUNDER_REWARD        (5000ULL * SATOSHI)
#define FEE_PERCENT           0.1
#define FEE_MIN               (1ULL * SATOSHI)
#define FEE_BURN_PERCENT      50
#define FEE_VALIDATOR_PERCENT 50
#define BLOCK_REWARD          (10ULL * SATOSHI)
#define HALVING_INTERVAL      210000
#define INITIAL_REWARD        (10ULL * SATOSHI)
#define MIN_STAKE             (10000ULL)
#define MAX_STAKE             (100000ULL * SATOSHI)
#define UNSTAKE_DELAY         100
#define SLASH_BASE_PERCENT    5
#define SLASH_MAX_PERCENT     50
#define MIN_SLASH_AMOUNT      (100ULL * SATOSHI)
#define MAX_SLASH_AMOUNT      (10000ULL * SATOSHI)

uint64_t calculate_fee(uint64_t amount);
uint64_t calculate_burn(uint64_t fee);
uint64_t calculate_validator_reward(uint64_t fee);
uint64_t calculate_block_reward(uint32_t height);
void print_tokenomics(void);
void format_lbxc(uint64_t satoshis, char *out, size_t len);
#endif
