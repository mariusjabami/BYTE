#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "transaction.h"
#include <stdint.h>

#define MEMPOOL_MAX 10000  /* máximo de transações na mempool */

void mempool_init(void);
int mempool_add(const Transaction *tx);
void mempool_remove(const char *tx_hash);
int mempool_has(const char *tx_hash);
uint32_t mempool_count(void);
uint32_t mempool_select(Transaction *txs, uint32_t max);
void mempool_free(void);

#endif
