#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stddef.h>

void sha256(const unsigned char *data, size_t len, unsigned char *out);

#endif
