#include "chain.h"
#include "balance.h"
#include "p2p.h"
#include "mempool.h"
#include "gossip.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#define CLR_RESET   "\033[0m"
#define CLR_GRAY    "\033[90m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_CYAN    "\033[36m"
#define CLR_RED     "\033[31m"

#define TOR_SOCKS_HOST "127.0.0.1"
#define TOR_SOCKS_PORT 9050
#define KEEPALIVE_INTERVAL 120
#define RETRY_INTERVAL 300  /* 5 minutos entre tentativas de reconexão */

static const char *SEED_NODES[] = {
    "hmlv4coj5ui4nc6jvutpjsetsmyhqsb6adxwvbxttsnzwo6cw3gqn3ad.onion:9333",
    "vah4blmjgwonpittgdtuah4kqzapzg4inwk45ouliinbbw7c74wz2syd.onion:9333",
    "nc4pbdsvbotvdz6i2rtiwxfc5iytwaysc7yg6xlcdz3sk6i3twomwzid.onion:9333",
    NULL
};

extern sqlite3 *db;

static int verbose = 0;
void p2p_set_verbose(int v) { verbose = v; }

#define LOG_INFO(fmt, ...)  do { printf(fmt, ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(fmt, ...) do { if(verbose) printf(fmt, ##__VA_ARGS__); } while(0)

static Peer        peers[MAX_PEERS];
static uint32_t    peer_count = 0;
static int         server_socket = -1;
static int         my_port = 9333;
static pthread_mutex_t peers_mutex = PTHREAD_MUTEX_INITIALIZER;
static char        my_node_id[65];
static char        my_onion[256];

/* ========== I/O ========== */
static int raw_send_all(int sock, const void *data, size_t len) {
    const char *ptr = (const char*)data;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t sent = send(sock, ptr, remaining, 0);
        if (sent < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue; return -1; }
        if (sent == 0) return -1;
        ptr += sent; remaining -= sent;
    }
    return 0;
}

static int raw_recv_all(int sock, void *data, size_t len) {
    char *ptr = (char*)data;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t received = recv(sock, ptr, remaining, 0);
        if (received < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue; return -1; }
        if (received == 0) return -1;
        ptr += received; remaining -= received;
    }
    return 0;
}

/* ========== SOCKS5 ========== */
static int socks5_connect(const char *onion, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_in tor_addr = {0};
    tor_addr.sin_family = AF_INET; tor_addr.sin_port = htons(TOR_SOCKS_PORT);
    inet_pton(AF_INET, TOR_SOCKS_HOST, &tor_addr.sin_addr);
    struct timeval tv = {30, 0}; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (connect(sock, (struct sockaddr*)&tor_addr, sizeof(tor_addr)) < 0) { close(sock); return -1; }
    unsigned char greet[] = {0x05, 0x01, 0x00}; raw_send_all(sock, greet, 3);
    unsigned char resp[2]; raw_recv_all(sock, resp, 2);
    if (resp[0] != 0x05 || resp[1] != 0x00) { close(sock); return -1; }
    size_t onion_len = strlen(onion); if (onion_len > 255) { close(sock); return -1; }
    unsigned char req[7+255]; req[0]=0x05; req[1]=0x01; req[2]=0x00; req[3]=0x03; req[4]=(unsigned char)onion_len;
    memcpy(req+5, onion, onion_len); req[5+onion_len]=(port>>8)&0xFF; req[6+onion_len]=port&0xFF;
    raw_send_all(sock, req, 5+onion_len+2);
    unsigned char hdr[4]; raw_recv_all(sock, hdr, 4);
    if (hdr[0]!=0x05 || hdr[1]!=0x00) { close(sock); return -1; }
    size_t addr_len;
    switch(hdr[3]) {
        case 0x01: addr_len=4; break;
        case 0x04: addr_len=16; break;
        case 0x03: { unsigned char l; raw_recv_all(sock, &l, 1); addr_len=l; break; }
        default: close(sock); return -1;
    }
    unsigned char trailer[256]; if (addr_len>0) raw_recv_all(sock, trailer, addr_len);
    unsigned char port_bytes[2]; raw_recv_all(sock, port_bytes, 2);
    return sock;
}

/* ========== NODE ID / ONION ========== */
void p2p_set_node_id(const char *id) { strncpy(my_node_id, id, 64); my_node_id[64]='\0'; }
const char* p2p_get_node_id(void) { return my_node_id; }
void p2p_set_onion(const char *onion) { if(onion&&onion[0]){strncpy(my_onion,onion,255);my_onion[255]='\0';}else my_onion[0]='\0'; }
const char* p2p_get_onion(void) { return my_onion; }

static int is_local_ip(const char *host) {
    struct ifaddrs *ifaddr,*ifa; if(getifaddrs(&ifaddr)==-1) return 0; int r=0;
    for(ifa=ifaddr;ifa;ifa=ifa->ifa_next) {
        if(ifa->ifa_addr&&ifa->ifa_addr->sa_family==AF_INET) {
            struct sockaddr_in *sa=(struct sockaddr_in*)ifa->ifa_addr; char lip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&sa->sin_addr,lip,sizeof(lip)); if(strcmp(host,lip)==0){r=1;break;}
        }
    }
    freeifaddrs(ifaddr); return r;
}
static int is_my_onion(const char *host) { return (my_onion[0]&&strcmp(host,my_onion)==0); }

/* ========== MENSAGENS ========== */
static int send_msg(int sock, uint32_t type, const void *data, size_t len) {
    uint32_t nt=htonl(type), nl=htonl((uint32_t)len);
    if(raw_send_all(sock,&nt,4)<0) return -1;
    if(raw_send_all(sock,&nl,4)<0) return -1;
    if(len>0&&raw_send_all(sock,data,len)<0) return -1;
    return 0;
}

static int recv_msg_timeout(int sock, uint32_t *type, void *data, size_t *len, int sec) {
    struct timeval tv={sec,0}; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    uint32_t nt,nl;
    if(raw_recv_all(sock,&nt,4)<0) return -1;
    if(raw_recv_all(sock,&nl,4)<0) return -1;
    *type=ntohl(nt); *len=ntohl(nl);
    if(*len>MESSAGE_SIZE) return -1;
    if(*len>0&&raw_recv_all(sock,data,*len)<0) return -1;
    return 0;
}

/* ========== BROADCAST EXCEPT ========== */
static void broadcast_block_except(const Block *b, int except_sock) {
    pthread_mutex_lock(&peers_mutex);
    int socks[MAX_PEERS]; uint32_t n = 0;
    for (uint32_t i = 0; i < peer_count && n < MAX_PEERS; i++)
        if (peers[i].socket > 0 && peers[i].socket != except_sock) socks[n++] = peers[i].socket;
    pthread_mutex_unlock(&peers_mutex);
    for (uint32_t i = 0; i < n; i++) send_msg(socks[i], MSG_BLOCK, b, sizeof(Block));
}

static void broadcast_tx_except(const Transaction *tx, int except_sock) {
    pthread_mutex_lock(&peers_mutex);
    int socks[MAX_PEERS]; uint32_t n = 0;
    for (uint32_t i = 0; i < peer_count && n < MAX_PEERS; i++)
        if (peers[i].socket > 0 && peers[i].socket != except_sock) socks[n++] = peers[i].socket;
    pthread_mutex_unlock(&peers_mutex);
    for (uint32_t i = 0; i < n; i++) send_msg(socks[i], MSG_TX, tx, sizeof(Transaction));
}

/* ========== PERSISTÊNCIA ========== */
static void save_peers_to_db(void) {
    if(!db) return;
    sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS known_peers (address TEXT PRIMARY KEY, last_seen INTEGER);",NULL,NULL,NULL);
    pthread_mutex_lock(&peers_mutex);
    for(uint32_t i=0;i<peer_count;i++) {
        if(peers[i].port==0) continue;
        char addr[512]; snprintf(addr,sizeof(addr),"%s:%d",peers[i].ip,peers[i].port);
        sqlite3_stmt *s;
        if(sqlite3_prepare_v2(db,"INSERT OR REPLACE INTO known_peers VALUES (?,?);",-1,&s,NULL)==SQLITE_OK){
            sqlite3_bind_text(s,1,addr,-1,SQLITE_STATIC); sqlite3_bind_int(s,2,(int)time(NULL)); sqlite3_step(s); sqlite3_finalize(s);
        }
    }
    pthread_mutex_unlock(&peers_mutex);
}

/* ========== RECONEXÃO PERIÓDICA AOS SEEDS ========== */
void *p2p_retry_thread(void *arg) {
    (void)arg;
    while (1) {
        sleep(RETRY_INTERVAL);
        
        pthread_mutex_lock(&peers_mutex);
        uint32_t current_peers = peer_count;
        pthread_mutex_unlock(&peers_mutex);
        
        /* Se tem menos de 3 peers, tenta reconectar aos seeds */
        if (current_peers < 3) {
            LOG_DEBUG(CLR_CYAN "  [p2p] Reconectando aos seeds (%u peers atuais)...\n" CLR_RESET, current_peers);
            for (int i = 0; SEED_NODES[i] != NULL; i++) {
                char host[256]; int port = 9333;
                sscanf(SEED_NODES[i], "%255[^:]:%d", host, &port);
                if (!is_my_onion(host)) {
                    p2p_connect(host, port);
                }
            }
        }
    }
    return NULL;
}

void p2p_load_peers(void) {
    if(!db) return;
    sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS known_peers (address TEXT PRIMARY KEY, last_seen INTEGER);",NULL,NULL,NULL);
    
    /* Conecta aos seeds primeiro */
    for(int i=0;SEED_NODES[i];i++){
        char host[256];int port=9333;
        sscanf(SEED_NODES[i],"%255[^:]:%d",host,&port);
        if(!is_my_onion(host)&&peer_count<MAX_PEERS)p2p_connect(host,port);
    }
    
    /* Depois carrega peers conhecidos */
    sqlite3_stmt *s;
    if(sqlite3_prepare_v2(db,"SELECT address FROM known_peers ORDER BY last_seen DESC LIMIT 50;",-1,&s,NULL)==SQLITE_OK){
        while(sqlite3_step(s)==SQLITE_ROW){
            const char *addr=(const char*)sqlite3_column_text(s,0);
            char host[256];int port=9333;
            sscanf(addr,"%255[^:]:%d",host,&port);
            if(peer_count<MAX_PEERS)p2p_connect(host,port);
        }
        sqlite3_finalize(s);
    }
    
    /* Inicia thread de retry */
    pthread_t retry_thread;
    pthread_create(&retry_thread, NULL, p2p_retry_thread, NULL);
    pthread_detach(retry_thread);
}

void p2p_list_peers(char *buf, size_t len) {
    pthread_mutex_lock(&peers_mutex);
    int pos=snprintf(buf,len,"Peers (%u):\n",peer_count);
    for(uint32_t i=0;i<peer_count&&(size_t)pos<len;i++) pos+=snprintf(buf+pos,len-pos,"  %s:%d  h=%u\n",peers[i].ip,peers[i].port,peers[i].best_height);
    pthread_mutex_unlock(&peers_mutex);
}

int p2p_disconnect_peer(const char *host) {
    pthread_mutex_lock(&peers_mutex);
    for(uint32_t i=0;i<peer_count;i++){if(strcmp(peers[i].ip,host)==0){if(peers[i].socket>0)close(peers[i].socket);peers[i]=peers[--peer_count];pthread_mutex_unlock(&peers_mutex);return 1;}}
    pthread_mutex_unlock(&peers_mutex); return 0;
}

/* ========== HANDLE PEER ========== */
static void handle_peer(int sock, const char *their_ip) {
    uint32_t type, my_height=get_latest_block().height; size_t len;
    char buf[MESSAGE_SIZE], their_id[65]={0}, their_onion[256]={0};
    uint32_t their_height=0; int their_port=0, recv_version=0, invalid_blocks=0, mempool_sent=0;
    time_t last_keepalive=time(NULL);

    char v[512]; snprintf(v,sizeof(v),"%u|%s|%s|%d",my_height,my_node_id,my_onion[0]?my_onion:"direct",my_port);
    send_msg(sock,MSG_VERSION,v,strlen(v)+1);

    while(1) {
        int timeout=recv_version?(KEEPALIVE_INTERVAL/2):10;
        int ret=recv_msg_timeout(sock,&type,buf,&len,timeout);
        if(ret<0){if(!recv_version)LOG_DEBUG(CLR_YELLOW "  [p2p] timeout\n" CLR_RESET); goto cleanup;}
        if(ret==0){pthread_mutex_lock(&peers_mutex);for(uint32_t i=0;i<peer_count;i++)if(peers[i].socket==sock){peers[i].last_seen=(uint32_t)time(NULL);break;}pthread_mutex_unlock(&peers_mutex);}

        switch(type) {
        case MSG_VERSION:
            if(recv_version) continue;
            sscanf(buf,"%u|%64[^|]|%255[^|]|%d",&their_height,their_id,their_onion,&their_port);
            recv_version=1;
            if(strcmp(their_id,my_node_id)==0||(my_onion[0]&&strcmp(their_onion,my_onion)==0)){LOG_DEBUG(CLR_YELLOW "  [p2p] self\n" CLR_RESET);goto cleanup;}
            LOG_INFO(CLR_GREEN "  + Peer: %s:%d (h=%u)\n" CLR_RESET,their_onion[0]?their_onion:their_ip,their_port,their_height);
            pthread_mutex_lock(&peers_mutex);
            for(uint32_t i=0;i<peer_count;i++)if(peers[i].socket==sock){peers[i].best_height=their_height;if(their_port>0)peers[i].port=their_port;if(their_onion[0]&&strcmp(their_onion,"direct")!=0){strncpy(peers[i].ip,their_onion,255);peers[i].ip[255]='\0';}peers[i].last_seen=(uint32_t)time(NULL);break;}
            pthread_mutex_unlock(&peers_mutex);
            if(my_height>their_height)for(uint32_t h=their_height+1;h<=my_height;h++){Block b=get_block(h);if(b.height!=UINT32_MAX)send_msg(sock,MSG_BLOCK,&b,sizeof(Block));}
            else if(their_height>my_height)send_msg(sock,MSG_GETBLOCKS,&my_height,sizeof(uint32_t));
                        /* Enviar transações da mempool após sincronização (apenas uma vez) */
            if (!mempool_sent && their_height == my_height) {
                mempool_sent = 1;
                uint32_t mc = mempool_count();
                if (mc > 0) {
                    Transaction txs[20];
                    uint32_t n = mempool_select(txs, 20);
                    for (uint32_t i = 0; i < n; i++) {
                        send_msg(sock, MSG_TX, &txs[i], sizeof(Transaction));
                    }
                    LOG_INFO("  [p2p] Mempool: %u tx enviadas\n", n);
                }
            }
            break;
        case MSG_GETBLOCKS:
            if(recv_version&&len>=sizeof(uint32_t)){uint32_t from;memcpy(&from,buf,sizeof(uint32_t));from=ntohl(from);for(uint32_t h=from+1;h<=get_latest_block().height;h++){Block b=get_block(h);if(b.height!=UINT32_MAX)send_msg(sock,MSG_BLOCK,&b,sizeof(Block));}}
            break;
        case MSG_BLOCK:
            if(recv_version&&len==sizeof(Block)){
                Block *b=(Block*)buf;
                int result=add_block(b);
                if(result==1){invalid_blocks=0;for(uint32_t i=0;i<b->tx_count;i++)mempool_remove(b->txs[i].hash);broadcast_block_except(b,sock);}
                else if(result==-1){invalid_blocks++;LOG_INFO(CLR_YELLOW "  [p2p] Bloco invalido (%d/3)\n" CLR_RESET,invalid_blocks);if(invalid_blocks>=3){LOG_INFO(CLR_RED "  [p2p] Desconectando %s\n" CLR_RESET,their_ip);goto cleanup;}}
            }
            break;
        case MSG_TX:
            if(recv_version&&len==sizeof(Transaction)){Transaction *tx=(Transaction*)buf;if(mempool_add(tx))broadcast_tx_except(tx,sock);}
            break;
        case MSG_PING: if(recv_version)send_msg(sock,MSG_PONG,NULL,0); break;
        case MSG_PONG: break;
        case MSG_PEERS: if(recv_version)gossip_process_peers(buf); break;
        default: break;
        }
        if(recv_version&&time(NULL)-last_keepalive>=KEEPALIVE_INTERVAL){send_msg(sock,MSG_PING,NULL,0);last_keepalive=time(NULL);}
    }
cleanup: close(sock);
}

static void *peer_thread(void *arg) {
    int sock=*(int*)arg; free(arg);
    struct sockaddr_in addr; socklen_t len=sizeof(addr); char ip[64]="unknown";
    if(getpeername(sock,(struct sockaddr*)&addr,&len)==0)inet_ntop(AF_INET,&addr.sin_addr,ip,sizeof(ip));
    handle_peer(sock,ip);
    pthread_mutex_lock(&peers_mutex);for(uint32_t i=0;i<peer_count;i++)if(peers[i].socket==sock){peers[i]=peers[--peer_count];break;}pthread_mutex_unlock(&peers_mutex);
    return NULL;
}

/* ========== API ========== */
int p2p_init(int port) {
    my_port=port; server_socket=socket(AF_INET,SOCK_STREAM,0); if(server_socket<0)return 0;
    int opt=1; setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    struct sockaddr_in a={0}; a.sin_family=AF_INET; a.sin_addr.s_addr=INADDR_ANY; a.sin_port=htons(port);
    if(bind(server_socket,(struct sockaddr*)&a,sizeof(a))<0){close(server_socket);return 0;}
    if(listen(server_socket,25)<0){close(server_socket);return 0;}
    LOG_INFO(CLR_GREEN "  [p2p] TCP :%d\n" CLR_RESET,port);
    return 1;
}

int p2p_connect(const char *host, int port) {
    if((is_local_ip(host)&&port==my_port)||(is_my_onion(host)&&port==my_port))return 0;
    pthread_mutex_lock(&peers_mutex);
    for(uint32_t i=0;i<peer_count;i++)if(strcmp(peers[i].ip,host)==0&&peers[i].port==port){pthread_mutex_unlock(&peers_mutex);return 1;}
    if(peer_count>=MAX_PEERS){pthread_mutex_unlock(&peers_mutex);return 0;}
    uint32_t idx=peer_count; pthread_mutex_unlock(&peers_mutex);
    int sock=-1;
    if(strstr(host,".onion")){sock=socks5_connect(host,port);}
    else{sock=socket(AF_INET,SOCK_STREAM,0);if(sock>=0){struct timeval tv={10,0};setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(port);struct addrinfo hints={0},*res;hints.ai_family=AF_INET;if(getaddrinfo(host,NULL,&hints,&res)==0){memcpy(&a,res->ai_addr,sizeof(a));freeaddrinfo(res);if(connect(sock,(struct sockaddr*)&a,sizeof(a))<0){close(sock);sock=-1;}}else{close(sock);sock=-1;}}}
    if(sock<0)return 0;
    pthread_mutex_lock(&peers_mutex);
    strncpy(peers[idx].ip,host,255);peers[idx].ip[255]='\0';peers[idx].socket=sock;peers[idx].port=port;peers[idx].last_seen=time(0);peers[idx].best_height=0;
    if(idx==peer_count)peer_count++; pthread_mutex_unlock(&peers_mutex);
    save_peers_to_db();
    int *sp=malloc(sizeof(int));*sp=sock;pthread_t t;pthread_create(&t,NULL,peer_thread,sp);pthread_detach(t);
    return 1;
}

void p2p_broadcast_tx(const Transaction *tx) {
    pthread_mutex_lock(&peers_mutex);int socks[MAX_PEERS];uint32_t n=0;
    for(uint32_t i=0;i<peer_count&&n<MAX_PEERS;i++)if(peers[i].socket>0)socks[n++]=peers[i].socket;
    pthread_mutex_unlock(&peers_mutex);
    for(uint32_t i=0;i<n;i++)send_msg(socks[i],MSG_TX,tx,sizeof(Transaction));
}

void p2p_broadcast_block(const Block *b) {
    pthread_mutex_lock(&peers_mutex);int socks[MAX_PEERS];uint32_t n=0;
    for(uint32_t i=0;i<peer_count&&n<MAX_PEERS;i++)if(peers[i].socket>0)socks[n++]=peers[i].socket;
    pthread_mutex_unlock(&peers_mutex);
    for(uint32_t i=0;i<n;i++)send_msg(socks[i],MSG_BLOCK,b,sizeof(Block));
}

void p2p_broadcast_peers(void) {
    pthread_mutex_lock(&peers_mutex);char pl[MESSAGE_SIZE]={0};
    for(uint32_t i=0;i<peer_count;i++){if(!peers[i].port)continue;char e[512];snprintf(e,sizeof(e),"%s:%d",peers[i].ip,peers[i].port);if(pl[0])strcat(pl,",");strncat(pl,e,MESSAGE_SIZE-strlen(pl)-1);}
    int socks[MAX_PEERS];uint32_t n=0;
    for(uint32_t i=0;i<peer_count&&n<MAX_PEERS;i++)if(peers[i].socket>0)socks[n++]=peers[i].socket;
    pthread_mutex_unlock(&peers_mutex);
    if(pl[0])for(uint32_t i=0;i<n;i++)send_msg(socks[i],MSG_PEERS,pl,strlen(pl)+1);
}

void p2p_cleanup_inactive(uint32_t to) {
    uint32_t n=time(0);pthread_mutex_lock(&peers_mutex);
    for(uint32_t i=0;i<peer_count;i++)if(n-peers[i].last_seen>to){if(peers[i].socket>0)close(peers[i].socket);peers[i]=peers[--peer_count];i--;}
    pthread_mutex_unlock(&peers_mutex);
}

void *p2p_server_thread(void *arg) {
    (void)arg;
    while(1){struct sockaddr_in ca;socklen_t al=sizeof(ca);int cs=accept(server_socket,(struct sockaddr*)&ca,&al);if(cs<0){usleep(100000);continue;}pthread_mutex_lock(&peers_mutex);if(peer_count<MAX_PEERS){peers[peer_count].socket=cs;strncpy(peers[peer_count].ip,inet_ntoa(ca.sin_addr),255);peers[peer_count].port=0;peers[peer_count].last_seen=time(0);peers[peer_count].best_height=0;peer_count++;pthread_mutex_unlock(&peers_mutex);int *sp=malloc(sizeof(int));*sp=cs;pthread_t t;pthread_create(&t,NULL,peer_thread,sp);pthread_detach(t);}else{pthread_mutex_unlock(&peers_mutex);close(cs);}}
    return NULL;
}

uint32_t p2p_peer_count(void) { uint32_t c; pthread_mutex_lock(&peers_mutex); c=peer_count; pthread_mutex_unlock(&peers_mutex); return c; }

void p2p_shutdown(void) {
    save_peers_to_db();pthread_mutex_lock(&peers_mutex);
    for(uint32_t i=0;i<peer_count;i++)if(peers[i].socket>0)close(peers[i].socket);
    peer_count=0;pthread_mutex_unlock(&peers_mutex);
    if(server_socket>0){close(server_socket);server_socket=-1;}
}
