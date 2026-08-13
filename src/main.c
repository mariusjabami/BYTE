#include "chain.h"
#include "wallet.h"
#include "balance.h"
#include "mempool.h"
#include "consensus.h"
#include "p2p.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

static volatile int running = 1;
void sig_handler(int sig) { (void)sig; running = 0; }

void* cmd_server_thread(void *arg) {
    int cmd_port = *(int*)arg;
    int cmd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(cmd_socket < 0) return NULL;
    int opt = 1; setsockopt(cmd_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = inet_addr("127.0.0.1"); addr.sin_port = htons(cmd_port);
    if(bind(cmd_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){ close(cmd_socket); return NULL; }
    if(listen(cmd_socket, 5) < 0){ close(cmd_socket); return NULL; }
    while(running){
        int client = accept(cmd_socket, NULL, NULL); if(client < 0) continue;
        char buf[1024]; int n = recv(client, buf, sizeof(buf)-1, 0);
        if(n > 0){ buf[n] = '\0';
            unsigned char priv[32], pub[65]; char addr[65];
            if(!wallet_load("wallet.dat", priv, pub, addr)){ send(client, "ERRO: sem carteira\n", 20, 0); close(client); continue; }
            char cmd[16], dest[128]; unsigned long long amount = 0;
            if(sscanf(buf, "%s %s %llu", cmd, dest, &amount) >= 2 && strcmp(cmd, "send") == 0){
                Transaction tx; create_transaction(&tx, addr, dest, (uint64_t)amount, (uint64_t)time(NULL));
                wallet_sign(&tx, priv, pub);
                char resp[256];
                if(mempool_add(&tx)) snprintf(resp, sizeof(resp), "OK: %llu moedas para %s\n", amount, dest);
                else snprintf(resp, sizeof(resp), "ERRO: tx invalida\n");
                send(client, resp, strlen(resp), 0);
            } else if(strncmp(buf, "balance", 7) == 0){
                char resp[512];
                snprintf(resp, sizeof(resp), "\n  Endereco: %s\n  Saldo:    %llu\n  Stake:    %llu\n  Altura:   %u\n  Peers:    %u\n",
                         addr, (unsigned long long)get_balance(addr), (unsigned long long)get_stake(addr),
                         get_latest_block().height, p2p_peer_count());
                send(client, resp, strlen(resp), 0);
            } else { send(client, "ERRO\n", 5, 0); }
        }
        close(client);
    }
    close(cmd_socket); return NULL;
}

void cmd_full(const char *peer, int p2p_port, int cmd_port, int listen_only) {
    unsigned char priv[32], pub[65]; char addr[65];
    if(!wallet_load("wallet.dat", priv, pub, addr)){ printf("Carteira nao encontrada.\n"); return; }
    signal(SIGINT, sig_handler);
    p2p_init(p2p_port);
    char node_id[65];
    unsigned char rand_id[32];
    for(int _i=0;_i<32;_i++) rand_id[_i] = rand() % 256;
    for(int _i=0;_i<32;_i++) sprintf(node_id+_i*2, "%02x", rand_id[_i]);
    node_id[64] = '0';
    p2p_set_node_id(node_id);
    printf("[node] ID: %.64s\n", node_id);
    pthread_t tid_p2p, tid_cmd;
    pthread_create(&tid_p2p, NULL, p2p_server_thread, NULL); pthread_detach(tid_p2p);
    int *cp = malloc(sizeof(int)); *cp = cmd_port;
    pthread_create(&tid_cmd, NULL, cmd_server_thread, cp); pthread_detach(tid_cmd);
    if(get_stake(addr) < 1000 && get_balance(addr) >= 1000) stake(addr, 1000);
    if(!listen_only && peer) {
        // Suporta peer como: IP, IP:PORTA, http://IP:PORTA, https://DOMINIO
        char peer_buf[512], host[256], path[16] = "/";
        int port = p2p_port;
        strncpy(peer_buf, peer, sizeof(peer_buf)-1); peer_buf[sizeof(peer_buf)-1] = '\0';
        
        // Remove protocolo
        char *p = peer_buf;
        if(strncmp(p, "https://", 8) == 0) { port = 443; p += 8; }
        else if(strncmp(p, "http://", 7) == 0) { port = 80; p += 7; }
        
        // Extrai host e porta
        char *colon = strrchr(p, ':');
        char *slash = strchr(p, '/');
        if(colon && (!slash || colon > slash)) { *colon = '\0'; port = atoi(colon+1); }
        if(slash) { strncpy(path, slash, 15); path[15] = '\0'; *slash = '\0'; }
        strncpy(host, p, 255); host[255] = '\0';
        
        // Reconstrói URL se era HTTP/HTTPS
        char url[512];
        if(strncmp(peer, "https://", 8) == 0) snprintf(url, sizeof(url), "https://%s:%d%s", host, port, path);
        else if(strncmp(peer, "http://", 7) == 0) snprintf(url, sizeof(url), "http://%s:%d%s", host, port, path);
        else snprintf(url, sizeof(url), "%s:%d", host, port);
        
        printf("[node] Conectando a %s...\n", url);
        p2p_connect(peer, port);
    }
    p2p_load_peers();
    Block latest = get_latest_block();
    printf("\n  ╔══════════════════════════════════════╗\n");
    printf("  ║        LIBREX CHAIN NODE            ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║  Endereco: %s\n", addr);
    printf("  ║  Altura:   %u\n", latest.height);
    printf("  ║  Saldo:    %llu\n", (unsigned long long)get_balance(addr));
    printf("  ║  Stake:    %llu\n", (unsigned long long)get_stake(addr));
    printf("  ║  P2P: %d  |  CMD: %d\n", p2p_port, cmd_port);
    printf("  ║  Peers:    %u\n", p2p_peer_count());
    printf("  ╚══════════════════════════════════════╝\n\n");
    while(running){
        if(mempool_count() > 0 && is_validator(addr)){
            printf("[node] Minerando bloco...\n");
            create_next_block(addr);
        }
        sleep(5);
    }
    p2p_shutdown();
}

void print_help(const char *prog) {
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║        LIBREX CHAIN v1.0                 ║\n");
    printf("  ╠══════════════════════════════════════════╣\n");
    printf("  ║  Comandos:                               ║\n");
    printf("  ║    create       Criar nova carteira      ║\n");
    printf("  ║    balance      Ver saldo e stake        ║\n");
    printf("  ║    send <dst> <qtd>  Enviar moedas       ║\n");
    printf("  ║    stake <qtd>  Fazer stake              ║\n");
    printf("  ║    full [peer] [p2p] [cmd] [--listen]    ║\n");
    printf("  ║    cmd <cmd> [port]  Comando remoto      ║\n");
    printf("  ╠══════════════════════════════════════════╣\n");
    printf("  ║  Exemplos:                               ║\n");
    printf("  ║    %s create                             ║\n", prog);
    printf("  ║    %s full --listen 9333 9334            ║\n", prog);
    printf("  ║    %s full 192.168.1.5:9333              ║\n", prog);
    printf("  ║    %s full https://xxx.serveo.net        ║\n", prog);
    printf("  ╚══════════════════════════════════════════╝\n\n");
}

int main(int argc, char **argv){
    if(argc < 2){ print_help(argv[0]); return 1; }
    if(strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0){ print_help(argv[0]); return 0; }
    init_chain(); mempool_init();
    if(strcmp(argv[1], "create") == 0){
        unsigned char priv[32], pub[65]; char addr[65];
        wallet_generate(priv, pub, addr); wallet_save("wallet.dat", priv, pub, addr);
        printf("  Endereco: %s\n", addr);
    } else if(strcmp(argv[1], "balance") == 0){
        unsigned char priv[32], pub[65]; char addr[65];
        if(wallet_load("wallet.dat", priv, pub, addr))
            printf("  %s\n  Saldo: %llu\n  Stake: %llu\n", addr,
                   (unsigned long long)get_balance(addr), (unsigned long long)get_stake(addr));
    } else if(strcmp(argv[1], "send") == 0 && argc >= 4){
        unsigned char priv[32], pub[65]; char addr[65];
        if(!wallet_load("wallet.dat", priv, pub, addr)) return 1;
        Transaction tx; create_transaction(&tx, addr, argv[2], atoll(argv[3]), (uint64_t)time(NULL));
        wallet_sign(&tx, priv, pub); mempool_add(&tx);
        printf("Tx enviada.\n");
    } else if(strcmp(argv[1], "stake") == 0 && argc >= 3){
        unsigned char priv[32], pub[65]; char addr[65];
        if(!wallet_load("wallet.dat", priv, pub, addr)) return 1;
        stake(addr, atoll(argv[2]));
    } else if(strcmp(argv[1], "full") == 0){
        int p2p_port = 9333, cmd_port = 9334, listen_only = 0; char *peer = NULL;
        for(int i = 2; i < argc; i++){
            if(strcmp(argv[i], "--listen") == 0) listen_only = 1;
            else if(strspn(argv[i], "0123456789") == strlen(argv[i]) && atoi(argv[i]) > 0){
                if(p2p_port == 9333) p2p_port = atoi(argv[i]); else cmd_port = atoi(argv[i]);
            } else peer = argv[i];
        }
        cmd_full(peer, p2p_port, cmd_port, listen_only);
    } else if(strcmp(argv[1], "cmd") == 0 && argc >= 3){
        int cp = 9334;
        for(int i = argc-1; i >= 3; i--){ if(atoi(argv[i]) > 0){ cp = atoi(argv[i]); break; } }
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET; addr.sin_addr.s_addr = inet_addr("127.0.0.1"); addr.sin_port = htons(cp);
        if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){ printf("Node offline na porta %d.\n", cp); return 1; }
        char buf[512] = {0};
        for(int i = 2; i < argc; i++){
            if(atoi(argv[i]) == 0 || i < argc-1){ strcat(buf, argv[i]); strcat(buf, " "); }
        }
        send(sock, buf, strlen(buf), 0); char resp[512]; int n = recv(sock, resp, sizeof(resp)-1, 0);
        if(n > 0){ resp[n] = '\0'; printf("%s", resp); } close(sock);
    } else {
        print_help(argv[0]);
    }
    mempool_free(); close_chain(); return 0;
}
