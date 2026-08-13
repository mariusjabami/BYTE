#include "tokenomics.h"
#include <stdio.h>
#include <string.h>
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_RED     "\033[31m"
#define CLR_CYAN    "\033[36m"
#define CLR_GRAY    "\033[90m"

uint64_t calculate_fee(uint64_t amount) { uint64_t fee = (amount * FEE_PERCENT) / 1000; return fee < FEE_MIN ? FEE_MIN : fee; }
uint64_t calculate_burn(uint64_t fee) { return (fee * FEE_BURN_PERCENT) / 100; }
uint64_t calculate_validator_reward(uint64_t fee) { return (fee * FEE_VALIDATOR_PERCENT) / 100; }
uint64_t calculate_block_reward(uint32_t height) { uint32_t halvings = height / HALVING_INTERVAL; uint64_t reward = INITIAL_REWARD; for (uint32_t i = 0; i < halvings && reward > 0; i++) reward /= 2; return reward > 0 ? reward : 1; }

/* Converte satoshis para string legível: 500000000000 → "5000.00000000 BYTE" */
void format_lbxc(uint64_t satoshis, char *out, size_t len) {
    uint64_t whole = satoshis / SATOSHI;
    uint64_t frac = satoshis % SATOSHI;
    snprintf(out, len, "%llu.%08llu BYTE", (unsigned long long)whole, (unsigned long long)frac);
}

void print_tokenomics(void) {
    char supply_str[64], reward_str[64], fee_str[64], max_stake_str[64];
    format_lbxc(TOTAL_SUPPLY, supply_str, sizeof(supply_str));
    format_lbxc(BLOCK_REWARD, reward_str, sizeof(reward_str));
    format_lbxc(FEE_MIN, fee_str, sizeof(fee_str));
    format_lbxc(MAX_STAKE, max_stake_str, sizeof(max_stake_str));
    
    printf("\n" CLR_CYAN "  ═══════════════════════════════════════════════\n" CLR_RESET);
    printf(CLR_BOLD "   Byte Tokenomics v1.0\n" CLR_RESET);
    printf(CLR_CYAN "  ═══════════════════════════════════════════════\n" CLR_RESET);
    printf(CLR_GRAY "   Supply total:    " CLR_GREEN "%s\n" CLR_RESET, supply_str);
    printf(CLR_GRAY "   Decimais:        " CLR_GREEN "%d\n" CLR_RESET, DECIMALS);
    printf(CLR_GRAY "   Recompensa base: " CLR_GREEN "%s\n" CLR_RESET, reward_str);
    printf(CLR_GRAY "   Halving a cada:  " CLR_GREEN "%d blocos\n" CLR_RESET, HALVING_INTERVAL);
    printf(CLR_GRAY "   Taxa:            " CLR_YELLOW "%.1f%%\n" CLR_RESET, FEE_PERCENT);
    printf(CLR_GRAY "   Taxa minima:     " CLR_YELLOW "%s\n" CLR_RESET, fee_str);
    printf(CLR_GRAY "   Queima:          " CLR_RED "%d%% da taxa\n" CLR_RESET, FEE_BURN_PERCENT);
    printf(CLR_GRAY "   Validador:       " CLR_GREEN "%d%% da taxa\n" CLR_RESET, FEE_VALIDATOR_PERCENT);
    printf(CLR_GRAY "   Stake minimo:    " CLR_GREEN "0.00010000 BYTE\n" CLR_RESET); printf(CLR_GRAY "   Stake maximo:    " CLR_YELLOW "%s\n" CLR_RESET, max_stake_str); printf(CLR_GRAY "   Slash:           " CLR_RED "%d%% - %d%%\n" CLR_RESET, SLASH_BASE_PERCENT, SLASH_MAX_PERCENT); printf(CLR_CYAN "  ═══════════════════════════════════════════════\n\n" CLR_RESET); }
