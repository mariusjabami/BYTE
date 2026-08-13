#include "gossip.h"
#include "p2p.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLR_GRAY    "\033[90m"
#define CLR_RESET   "\033[0m"

void gossip_process_peers(const char *peer_list) {
    if (!peer_list || strlen(peer_list) == 0) return;
    
    char *list = strdup(peer_list);
    if (!list) return;
    
    char *token = strtok(list, ",");
    int count = 0;
    
    while (token != NULL) {
        char host[256];
        int port = 9333;
        
        if (sscanf(token, "%255[^:]:%d", host, &port) >= 1) {
            p2p_connect(host, port);
            count++;
        }
        token = strtok(NULL, ",");
    }
    
    free(list);
    
    if (count > 0)
        printf(CLR_GRAY "  [gossip] %d novos peers descobertos\n" CLR_RESET, count);
}

void gossip_share_peers(void) {
    p2p_broadcast_peers();
}
