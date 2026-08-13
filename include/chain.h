#ifndef CHAIN_H
#define CHAIN_H

#include "block.h"
#include "transaction.h"
#include <stdint.h>

int init_chain(void);
void init_block(Block *block);
int add_block(const Block *block);
Block get_latest_block(void);
Block get_block(uint32_t height);
void close_chain(void);

#endif
