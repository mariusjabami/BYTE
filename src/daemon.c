#include "chain.h"
#include "wallet.h"
#include "balance.h"
#include "mempool.h"
#include "consensus.h"
#include "p2p.h"
#include "tokenomics.h"
#include "gossip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_CYAN    "\033[36m"
#define CLR_WHITE   "\033[37m"
#define CLR_GRAY    "\033[90m"

#define PEER_TIMEOUT 300

static volatile int running = 1;
static unsigned char g_priv[32], g_pub[65];
static char g_addr[65];
static int g_has_wallet = 0;

void sig_handler(int sig) { (void)sig; running = 0; }

/* Converte hex string para bytes */
static int hex_to_bytes(const char *hex, unsigned char *out, size_t max_len) {
    size_t len = strlen(hex) / 2;
    if (len > max_len) len = max_len;
    for (size_t i = 0; i < len; i++) {
        unsigned int byte;
        if (sscanf(hex + i * 2, "%2x", &byte) != 1) return -1;
        out[i] = (unsigned char)byte;
    }
    return (int)len;
}

static void setup_onion_address(void) {
    FILE *src = fopen("Hidden/hostname", "r");
    if (!src) {
        printf(CLR_YELLOW "  [onion] Hidden/hostname não encontrado\n O arquivo e as chaves precisa estar na pasta Hidden dentro desta pasta do projeto!\nN" CLR_RESET);
        return;
    }
    char onion[256];
    if (!fgets(onion, sizeof(onion), src)) {
        fclose(src);
        return;
    }
    fclose(src);
    onion[strcspn(onion, "\r\n")] = '\0';
    if (strlen(onion) < 10 || !strstr(onion, ".onion")) return;

    p2p_set_onion(onion);
    printf(CLR_GREEN "  [onion] %s\n" CLR_RESET, onion);
}
void *peer_cleanup_thread(void *arg){(void)arg;while(running){sleep(60);p2p_cleanup_inactive(PEER_TIMEOUT);}return NULL;}
void *gossip_thread(void *arg){(void)arg;while(running){sleep(60);p2p_broadcast_peers();}return NULL;}

void *cmd_server_thread(void *arg){
    int cp=*(int*)arg;int cs=socket(AF_INET,SOCK_STREAM,0);if(cs<0)return NULL;
    int opt=1;setsockopt(cs,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    struct sockaddr_in a;memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=inet_addr("127.0.0.1");a.sin_port=htons(cp);
    if(bind(cs,(struct sockaddr*)&a,sizeof(a))<0){close(cs);return NULL;}
    if(listen(cs,5)<0){close(cs);return NULL;}
    while(running){
        int cl=accept(cs,NULL,NULL);if(cl<0)continue;
        char b[4096];int n=recv(cl,b,sizeof(b)-1,0);
        if(n>0){b[n]=0;char r[4096]={0},cmd[32],a1[128],a2[128];unsigned long long amt=0;
            int na=sscanf(b,"%31s %127s %127s %llu",cmd,a1,a2,&amt);

            if(strcmp(cmd,"balance")==0||strcmp(cmd,"getinfo")==0){
                if(g_has_wallet){
                    Block l=get_latest_block();const char *on=p2p_get_onion();
                    uint64_t bal=get_balance(g_addr), stk=get_stake(g_addr);
                    char bstr[64], sstr[64];
                    format_lbxc(bal,bstr,sizeof(bstr));
                    format_lbxc(stk,sstr,sizeof(sstr));
                    char *bp=strstr(bstr," BYTE"); if(bp)*bp=0;
                    char *sp=strstr(sstr," BYTE"); if(sp)*sp=0;
                    snprintf(r,sizeof(r),"{\"address\":\"%s\",\"balance\":\"%s\",\"stake\":\"%s\",\"height\":%u,\"peers\":%u,\"mempool\":%u,\"validators\":%u,\"node_id\":\"%.64s\",\"onion\":\"%s\",\"last_block\":\"%.64s\"}\n",
                        g_addr,bstr,sstr,l.height,p2p_peer_count(),mempool_count(),get_validator_count(),p2p_get_node_id(),on?on:"",l.hash);
                } else snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem carteira\"}\n");
            }else if(strcmp(cmd,"send")==0){
                if(!g_has_wallet)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem carteira\"}\n");
                else{
                    char *d=a1; unsigned long long sa=(na>=3)?strtoull(a2,NULL,10):amt;
                    if(d&&sa>0){Transaction tx;uint64_t fee=calculate_fee(sa);if(fee<FEE_MIN)fee=FEE_MIN;uint64_t ta=sa+fee;
                        if(get_balance(g_addr)<ta)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"saldo insuficiente\"}\n");
                        else{create_transaction(&tx,g_addr,d,sa,(uint64_t)time(NULL));tx.fee=fee;hash_transaction(&tx,tx.hash);wallet_sign(&tx,g_priv,g_pub);
                            char fstr[64];format_lbxc(fee,fstr,sizeof(fstr));char *fp=strstr(fstr," BYTE");if(fp)*fp=0;
                            if(mempool_add(&tx))snprintf(r,sizeof(r),"{\"status\":\"ok\",\"txid\":\"%.64s\",\"fee\":\"%s\"}\n",tx.hash,fstr);
                            else snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"falha mempool\"}\n");}}
                    else snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"uso: send <dest> <valor>\"}\n");}
            }else if(strcmp(cmd,"broadcast")==0){
                /* broadcast <from> <to> <amount_sat> <fee_sat> <sig_hex> <pubkey_hex> <timestamp> */
                char sig_hex[256]={0}, pub_hex[256]={0};
                unsigned long long fee_amt = 0, tx_ts = 0;
                int bn = sscanf(b,"%31s %127s %127s %llu %llu %255s %255s %llu",cmd,a1,a2,&amt,&fee_amt,sig_hex,pub_hex,&tx_ts);

                if(bn >= 5 && strlen(sig_hex) > 0 && strlen(pub_hex) > 0){
                    Transaction tx;
                    memset(&tx, 0, sizeof(tx));
                    strncpy(tx.from, a1, 64);
                    strncpy(tx.to, a2, 64);
                    tx.amount = (uint64_t)amt;
                    tx.fee = (uint64_t)fee_amt;
                    tx.timestamp = tx_ts > 0 ? tx_ts : (uint64_t)time(NULL);

                    int sig_len = hex_to_bytes(sig_hex, tx.signature, 72);
                    int pub_len = hex_to_bytes(pub_hex, tx.public_key, 65);

                    if(sig_len > 0 && pub_len > 0){
                        tx.signature_len = (unsigned int)sig_len;
                        hash_transaction(&tx, tx.hash);

                        if(wallet_verify(&tx)){
                            if(mempool_add(&tx)){
                                p2p_broadcast_tx(&tx);
                                snprintf(r,sizeof(r),"{\"status\":\"ok\",\"txid\":\"%.64s\"}\n",tx.hash);
                            }else{
                                snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"falha mempool (saldo insuficiente ou ja minerada)\"}\n");
                            }
                        }else{
                            snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"assinatura invalida\"}\n");
                        }
                    }else{
                        snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"erro ao decodificar assinatura ou chave publica\"}\n");
                    }
                }else{
                    snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"uso: broadcast <from> <to> <amount_sat> <fee_sat> <sig_hex> <pubkey_hex> <timestamp>\"}\n");
                }
            }else if(strcmp(cmd,"stake")==0){
                if(!g_has_wallet)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem carteira\"}\n");
                else{unsigned long long sa=(na>=2)?strtoull(a1,NULL,10):0;
                    if(sa<MIN_STAKE)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"stake minimo: 0.00010000 BYTE\"}\n");
                    else if(stake(g_addr,(uint64_t)sa))snprintf(r,sizeof(r),"{\"status\":\"ok\"}\n");
                    else snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"saldo insuficiente ou maximo excedido\"}\n");}
            }else if(strcmp(cmd,"unstake")==0){
                if(!g_has_wallet)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem carteira\"}\n");
                else{uint64_t cs=get_stake(g_addr);if(cs==0)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem stake\"}\n");
                    else if(unstake(g_addr))snprintf(r,sizeof(r),"{\"status\":\"ok\",\"msg\":\"stake liberado em %u blocos\"}\n",UNSTAKE_DELAY);
                    else snprintf(r,sizeof(r),"{\"status\":\"error\"}\n");}
            }else if(strcmp(cmd,"peers")==0)p2p_list_peers(r,sizeof(r));
            else if(strcmp(cmd,"connect")==0&&na>=2){int p=9333;if(na>=3)p=atoi(a2);snprintf(r,sizeof(r),p2p_connect(a1,p)?"{\"status\":\"ok\"}\n":"{\"status\":\"error\"}\n");}
            else if(strcmp(cmd,"disconnect")==0&&na>=2){p2p_disconnect_peer(a1);snprintf(r,sizeof(r),"{\"status\":\"ok\"}\n");}
            else if(strcmp(cmd,"mempool")==0)snprintf(r,sizeof(r),"{\"mempool\":%u}\n",mempool_count());
            else if(strcmp(cmd,"mine")==0){if(!g_has_wallet)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem carteira\"}\n");else if(!is_validator(g_addr))snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"sem stake\"}\n");else if(mempool_count()==0)snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"mempool vazia\"}\n");else snprintf(r,sizeof(r),create_next_block(g_addr)?"{\"status\":\"ok\"}\n":"{\"status\":\"error\"}\n");}
            else if(strcmp(cmd,"getblock")==0){uint32_t h=get_latest_block().height;if(na>=2)sscanf(a1,"%u",&h);Block bl=get_block(h);if(bl.height!=UINT32_MAX)snprintf(r,sizeof(r),"{\"height\":%u,\"hash\":\"%.64s\"}\n",bl.height,bl.hash);else snprintf(r,sizeof(r),"{\"status\":\"error\"}\n");}
            else if(strcmp(cmd,"status")==0){Block l=get_latest_block();snprintf(r,sizeof(r),"{\"height\":%u,\"peers\":%u,\"mempool\":%u}\n",l.height,p2p_peer_count(),mempool_count());}
            else snprintf(r,sizeof(r),"{\"status\":\"error\",\"msg\":\"comando desconhecido\"}\n");
            send(cl,r,strlen(r),0);}
        close(cl);}
    close(cs);return NULL;
}

int main(int argc,char **argv){
    int pp=9333,cp=9334;
    for(int i=1;i<argc;i++){if(strcmp(argv[i],"-p2p")==0&&i+1<argc)pp=atoi(argv[++i]);else if(strcmp(argv[i],"-cmd")==0&&i+1<argc)cp=atoi(argv[++i]);else if(strcmp(argv[i],"--verbose")==0)p2p_set_verbose(1);}
    signal(SIGINT,sig_handler);signal(SIGTERM,sig_handler);signal(SIGPIPE,SIG_IGN);srand((unsigned)time(NULL));
    if(!init_chain()){fprintf(stderr,CLR_RED "[ERRO] Blockchain\n" CLR_RESET);return 1;}
    mempool_init();
    char nid[65];unsigned char rid[32];FILE *f=fopen("/dev/urandom","rb");
    if(f){if(fread(rid,1,32,f)!=32)for(int i=0;i<32;i++)rid[i]=rand()%256;fclose(f);}else for(int i=0;i<32;i++)rid[i]=rand()%256;
    for(int i=0;i<32;i++)sprintf(nid+i*2,"%02x",rid[i]);nid[64]=0;p2p_set_node_id(nid);
    if(!p2p_init(pp)){fprintf(stderr,CLR_RED "[ERRO] Porta %d em uso\n" CLR_RESET,pp);close_chain();return 1;}
    setup_onion_address();
    pthread_t t1,t2,t3,t4;pthread_create(&t1,NULL,p2p_server_thread,NULL);pthread_detach(t1);
    int *c=malloc(sizeof(int));*c=cp;pthread_create(&t2,NULL,cmd_server_thread,c);pthread_detach(t2);
    pthread_create(&t3,NULL,peer_cleanup_thread,NULL);pthread_detach(t3);
    pthread_create(&t4,NULL,gossip_thread,NULL);pthread_detach(t4);
    p2p_load_peers();print_tokenomics();
    g_has_wallet=wallet_load("wallet.dat",g_priv,g_pub,g_addr);
    Block l=get_latest_block();
    printf("\n" CLR_CYAN "  ═══════════════════════════════════════════════\n" CLR_RESET);
    printf(CLR_BOLD CLR_WHITE "   Byte Chain Daemon v1.0\n" CLR_RESET);
    printf(CLR_CYAN "  ═══════════════════════════════════════════════\n" CLR_RESET);
    printf(CLR_GRAY "   Node ID: " CLR_YELLOW "%.24s...\n" CLR_RESET,nid);
    printf(CLR_GRAY "   P2P: " CLR_GREEN "%d" CLR_GRAY "  |  CMD: " CLR_GREEN "%d" CLR_RESET "\n",pp,cp);
    const char *on=p2p_get_onion();if(on&&on[0])printf(CLR_GRAY "   Onion:  " CLR_CYAN "%s\n" CLR_RESET,on);
    if(g_has_wallet){uint64_t bal=get_balance(g_addr),stk=get_stake(g_addr);char bstr[64],sstr[64];format_lbxc(bal,bstr,sizeof(bstr));format_lbxc(stk,sstr,sizeof(sstr));
        printf(CLR_GRAY "   Carteira: " CLR_GREEN "%s\n" CLR_RESET,g_addr);
        printf(CLR_GRAY "   Saldo:    " CLR_GREEN "%s\n" CLR_RESET,bstr);
        printf(CLR_GRAY "   Stake:    " CLR_YELLOW "%s\n" CLR_RESET,sstr);
        printf(CLR_GRAY "   Status:   %s\n" CLR_RESET,stk>=MIN_STAKE?CLR_GREEN "● VALIDADOR" CLR_RESET:CLR_RED "○ nao-validador" CLR_RESET);}
    else printf(CLR_GRAY "   Carteira: " CLR_RED "nao encontrada\n" CLR_RESET);
    printf(CLR_CYAN "  ═══════════════════════════════════════════════\n" CLR_RESET);
    printf(CLR_GRAY "   Altura: " CLR_BLUE "%-6u" CLR_GRAY "  Peers: " CLR_GREEN "%-4u" CLR_GRAY "  Mempool: " CLR_YELLOW "%-4u\n" CLR_RESET,l.height,p2p_peer_count(),mempool_count());
    printf(CLR_CYAN "  ═══════════════════════════════════════════════\n\n" CLR_RESET);
    time_t lbt=time(NULL),lst=0;
    while(running){time_t now=time(NULL);
        if(g_has_wallet&&is_validator(g_addr)&&now-lbt>=BLOCK_TIME){const char *ex=select_validator(l.height+1,l.hash);uint32_t tv=get_validator_count();int sm=0;if(ex&&strcmp(ex,g_addr)==0)sm=1;else if(tv<=1)sm=1;
            if(sm&&create_next_block(g_addr)){lbt=now;l=get_latest_block();char bstr[64];format_lbxc(get_balance(g_addr),bstr,sizeof(bstr));printf(CLR_GREEN "  ⛏  Bloco #%u | Saldo: %s\n" CLR_RESET,l.height,bstr);}}
        if(now-lst>=60){lst=now;printf(CLR_GRAY "  [status] altura=%u peers=%u mempool=%u\n" CLR_RESET,get_latest_block().height,p2p_peer_count(),mempool_count());}
        sleep(1);}
    printf("\n" CLR_YELLOW "  [daemon] Encerrando...\n" CLR_RESET);p2p_shutdown();mempool_free();close_chain();
    printf(CLR_GREEN "  [daemon] Ate logo!\n" CLR_RESET);return 0;
}

