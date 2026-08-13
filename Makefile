CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O0 -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto -lpthread -ldl -lm

OBJ = src/block.o src/chain.o src/transaction.o src/wallet.o src/balance.o \
      src/mempool.o src/p2p.o src/consensus.o src/sqlite3.o \
      src/crypto_utils.o src/tokenomics.o src/gossip.o

all: byted byte-cli

byted: src/daemon.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

byte-cli: src/cli.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o byted byte-cli bytechain.db
