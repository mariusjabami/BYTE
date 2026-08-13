#include "block.h"
#include "transaction.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>


void init_block(Block *block){
	// clear all buffer
	memset(block, 0, sizeof(Block));

	// fill the block w empity layers
	block->timestamp = time(NULL);
	block->tx_count = 0;
}

void calculate_block_hash(Block *block, char *output_hash){

	// create a buffer and alocate a resume of all block to a string
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%llu%u%u", block->prev_hash, (unsigned long long)block->timestamp, block->height, block->tx_count);

	for(uint32_t i = 0; i < block->tx_count; i++){
		strncat(buffer, block->txs[i].hash, sizeof(buffer) - strlen(buffer) - 1);
	}

	// create and hash w 32 bytes w the resume storated in buffer
	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256((unsigned char*)buffer, strlen(buffer), hash);

	// walk into hash array and convert bytes to hexa
	for(int i = 0; i < 32; i++){
		sprintf(output_hash + (i * 2), "%02x", hash[i]);
	}

	// using a null terminator for finalize the hash output array
	output_hash[64] = '\0';
}

int validate_block(Block *block, Block *prev_block, const char *prev_hash){ 

	// so, to validate a block we can use some rules to verify if a block is valide
	// the first one is verify if the prev_hash in the block is the same between the hash in the prev_block

	if(strcmp(block->prev_hash, prev_hash) != 0){
		return 0;
	}

	// the second one is verify the height or the block position in the entire book
	if(block->height != prev_block->height + 1){
		return 0;
	}
	
	// we can also verify the time, if the atual time block is > than the time on the prev_block
	if(block->timestamp < prev_block->timestamp){
		return 0;
	}

	// this is important, bcz we need verify if the block has the correct and accepted numbers of transactions stored on MAX_TX_PER_BLOCK
	if(block->tx_count > MAX_TX_PER_BLOCK){
		return 0;
	}


	// here, we dont trust in the hash stored in block, so we recalculated that block hash and verify if is the same
	char recalculated_hash[65];

	calculate_block_hash(block, recalculated_hash);

	if(strcmp(block->hash, recalculated_hash) != 0){
		return 0;
	}

	// here we just verify the lenght of the hash
	if(strlen(block->hash) != 64){
		return 1;
	}

	// if all is correct, the function return 1 that means that the block is fuckin valid
	return 1;
}
