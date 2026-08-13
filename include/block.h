#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "transaction.h"

// the max transaction per block
#define MAX_TX_PER_BLOCK 20

// main struct of a block on libre x chain
typedef struct Block {

	// hash of current block
	char hash[65];

	// hash of previous block
	char prev_hash[65];

	// time of creation of block
	uint64_t timestamp;

	// position on chain (genesis = 0)
	uint32_t height;	

	// real number of transactions on a unique block
	uint32_t tx_count;

	// list of transaction
	Transaction txs[MAX_TX_PER_BLOCK];
} Block;

// initialize a block w standard values
void init_block(Block *block);

// calcule the block hash based in his content
void calculate_block_hash(Block *block, char *output_hash);

// validate if the block if valid between the previuos block
int validate_block(Block *block, Block *prev_block, const char *prev_hash);

#endif 
