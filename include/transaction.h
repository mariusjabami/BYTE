#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdint.h>

#define ADDRESS_SIZE 65

typedef struct {

	char from[ADDRESS_SIZE];
	char to[ADDRESS_SIZE];
	uint64_t timestamp;
	uint64_t amount;
    uint64_t fee;
	char hash[65];

	unsigned char public_key[65];
	unsigned char signature[72];
	unsigned int signature_len;
} Transaction;


void create_transaction(
		Transaction *tx,
		const char *from,
		const char *to,
		uint64_t amount,
		uint64_t timestamp
		);

void hash_transaction(Transaction *tx, char *output_hash);

int validate_transaction(Transaction *tx);

void print_transaction(Transaction *tx);

#endif
