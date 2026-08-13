#ifndef WALLET_H
#define WALLET_H

#include "transaction.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

int wallet_generate(unsigned char *private_key,
                    unsigned char *public_key,
                    char *address);

void wallet_public_to_address(const unsigned char *public_key, char *address);

int wallet_sign(Transaction *tx, const unsigned char *private_key,
                const unsigned char *public_key);

int wallet_verify(const Transaction *tx);

int wallet_get_public(const unsigned char *private_key,
                      unsigned char *public_key);

int wallet_save(const char *filename, const unsigned char *priv,
                const unsigned char *pub, const char *address);

int wallet_load(const char *filename, unsigned char *priv,
                unsigned char *pub, char *address);

#endif
