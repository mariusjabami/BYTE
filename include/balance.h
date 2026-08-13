#ifndef BALANCE_H
#define  BALANCE_H

#include <stdint.h>

int init_balances(void);

uint64_t get_balance(const char *address);

int update_balance(const char *address, int64_t delta);

int transfer(const char *from, const char *to, uint64_t amount);

void close_balances(void);

#endif
