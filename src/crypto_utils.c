#include "crypto_utils.h"
#include <openssl/sha.h>

void sha256(const unsigned char *data, size_t len, unsigned char *out) {
    SHA256(data, len, out);
}
