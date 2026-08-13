## 12. Guia de Instalação Completo (Windows, Linux, Termux)

### 12.1 Windows

#### 12.1.1 Instalar Tor

1. Baixe o Tor Browser: https://www.torproject.org/download/
2. Instale e execute
3. O proxy SOCKS5 estará em `127.0.0.1:9050`

#### 12.1.2 Instalar Git e compilar

```powershell
# Instalar Git
winget install Git.Git

# Instalar GCC (via MSYS2)
winget install MSYS2.MSYS2
# Abra MSYS2 e execute:
# pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl mingw-w64-x86_64-sqlite3 make

# Clonar repositório
cd %USERPROFILE%
git clone https://gitnary.dpdns.org/marius/blockchain.git
cd blockchain

# Compilar
mingw32-make
```

#### 12.1.3 Executar

```powershell
byted.exe
```

---

### 12.2 Linux (Ubuntu/Debian)

#### 12.2.1 Instalar dependências

```bash
sudo apt update
sudo apt install -y tor gcc make libssl-dev libsqlite3-dev git netcat-openbsd jq
```

#### 12.2.2 Clonar e compilar

```bash
cd ~
git clone https://gitnary.dpdns.org/marius/blockchain.git
cd blockchain
make
```

#### 12.2.3 Configurar Tor Hidden Service

```bash
# Criar diretório do Hidden Service
sudo mkdir -p /caminho-do-projeto/Hidden
sudo chmod 700 /caminho-do-projeto/Hidden

# Editar configuração do Tor
sudo nano /etc/tor/torrc
```

**Adicionar no final do arquivo `/etc/tor/torrc`:**
```
# byte Chain Hidden Service
HiddenServiceDir /caminho-do-projeto/Hidden
HiddenServicePort 9333 127.0.0.1:9333
```

```bash
# Reiniciar Tor
sudo systemctl restart tor
sudo systemctl enable tor

# Aguardar 10 segundos e verificar o .onion
sleep 10
sudo cat /caminho-do-projeto/Hidden/hostname
# Exemplo: xyz123.onion

# Copiar o hostname para a pasta do projeto
sudo cat /caminho-do-projeto/Hidden/hostname > hostname
chmod 644 hostname
cat hostname
```

#### 12.2.4 Iniciar o nó

```bash
./byted &
sleep 3
./byte-cli balance
```

---

### 12.3 Termux (Android)

#### 12.3.1 Instalar dependências

```bash
pkg update && pkg upgrade
pkg install -y tor git make clang openssl sqlite netcat-openbsd jq
```

#### 12.3.2 Configurar Tor Hidden Service

```bash
# Criar diretório do Hidden Service
mkdir -p $PREFIX/caminho-do-projeto/Hidden
chmod 700 $PREFIX/caminho-do-projeto/Hidden

# Editar configuração do Tor
nano $PREFIX/etc/tor/torrc
```

**Adicionar no final do arquivo:**
```
# byte Chain Hidden Service
HiddenServiceDir /caminho-do-projeto/Hidden
HiddenServicePort 9333 127.0.0.1:9333
SocksPort 9050
```

```bash
# Iniciar o Tor
tor &
sleep 10

# Pegar o endereço .onion
cat $PREFIX/caminho-do-projeto/Hidden/hostname > hostname
cat hostname
```

#### 12.3.3 Clonar e compilar

```bash
cd ~
git clone https://gitnary.dpdns.org/marius/blockchain.git
cd blockchain
make
```

#### 12.3.4 Iniciar o nó

```bash
./byted &
sleep 3
./byte-cli balance
```

---

### 12.4 Verificar se está tudo funcionando

```bash
# Ver saldo
./byte-cli balance | jq .

# Ver peers
./byte-cli peers

# Ver status
./byte-cli status

# Criar carteira
./byte-cli create

# Fazer stake (ex: 100 LBXC)
./byte-cli stake 100

# Enviar LBXC
./byte-cli send <ENDERECO_DESTINO> <VALOR>
```

---

### 12.5 Comandos Rápidos

| Comando | Descrição |
|---------|-----------|
| `./byted` | Iniciar daemon |
| `./byted --verbose` | Iniciar com logs detalhados |
| `./byted -p2p 9333` | Especificar porta P2P |
| `./byted -cmd 9334` | Especificar porta CLI |
| `pkill byted` | Parar daemon |
| `./byte-cli balance` | Ver saldo formatado |
| `./byte-cli balance \| jq .` | Ver saldo JSON |
| `./byte-cli send <dest> <valor>` | Enviar BYTE |
| `./byte-cli stake <valor>` | Fazer stake |
| `./byte-cli unstake` | Retirar stake |
| `./byte-cli create` | Criar carteira |
| `./byte-cli restore` | Restaurar da seed phrase |
| `./byte-cli peers` | Listar peers |
| `./byte-cli status` | Status rápido |
| `./byte-cli getinfo` | Info completa |

---

### 12.6 Seed Nodes (Bootstrap)

A rede possui 3 seed nodes hardcoded. Ao iniciar, teu nó conecta automaticamente:

```
hmlv4coj5ui4nc6jvutpjsetsmyhqsb6adxwvbxttsnzwo6cw3gqn3ad.onion:9333
vah4blmjgwonpittgdtuah4kqzapzg4inwk45ouliinbbw7c74wz2syd.onion:9333
nc4pbdsvbotvdz6i2rtiwxfc5iytwaysc7yg6xlcdz3sk6i3twomwzid.onion:9333
```

---

### 12.7 Estrutura de Diretórios

```
~/byte/
├── byted                # Executável do daemon
├── vanity               # Criar endereços personalizados de carteiras  
├── byte-cli             # Executável do CLI
├── hostname             # Teu endereço .onion
├── wallet.dat           # Carteira (chave privada + pública)
├── bytechain.db         # Blockchain (SQLite3)
├── README.md            # Informações do Projeto
├── src/                 # Código fonte C
├── include/             # Arquivos header
├── Makefile             # Script de compilação
└── CONFIG.md            # Este Documento


---

### 12.8 Arquivos para Backup

| Arquivo | Importância | Descrição |
|---------|-------------|-----------|
| `wallet.dat` | **CRÍTICO** | Contém chave privada e endereço |
| `hostname` | Médio | Pode ser regenerado pelo Tor |
| `bytechain.db` | Baixo | Pode ser re-sincronizado da rede |

**⚠ NUNCA compartilhe `wallet.dat`! || Perder ela significa perder os seus ativos!**

---
