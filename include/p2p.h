#ifndef P2P_H
#define P2P_H
#include <stddef.h>
#include <stdint.h>
#include "block.h"
#include "transaction.h"

#define MAX_PEERS      125
#define MESSAGE_SIZE   16384
#define PEER_TIMEOUT   300

typedef enum {
    MSG_VERSION   = 1,
    MSG_VERACK    = 2,
    MSG_GETBLOCKS = 3,
    MSG_INV       = 4,
    MSG_GETDATA   = 5,
    MSG_BLOCK     = 6,
    MSG_TX        = 7,
    MSG_PING      = 8,
    MSG_PONG      = 9,
    MSG_PEERS     = 10
} MessageType;

typedef struct {
    char     ip[256];
    int      port;
    int      socket;
    uint32_t last_seen;
    uint32_t best_height;
} Peer;

int          p2p_init(int port);
int          p2p_connect(const char *host, int port);
void         p2p_broadcast_tx(const Transaction *tx);
void         p2p_broadcast_block(const Block *b);
void         p2p_broadcast_peers(void);
void        *p2p_server_thread(void *arg);
uint32_t     p2p_peer_count(void);
void         p2p_set_node_id(const char *id);
const char  *p2p_get_node_id(void);
void         p2p_set_onion(const char *onion);
void         p2p_set_verbose(int v);
const char  *p2p_get_onion(void);
void         p2p_load_peers(void);
void         p2p_list_peers(char *buf, size_t len);
int          p2p_disconnect_peer(const char *host);
void         p2p_cleanup_inactive(uint32_t timeout_sec);
void         p2p_shutdown(void);
void         gossip_process_peers(const char *peer_list);

#endif
