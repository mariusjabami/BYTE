# BYTE Whitepaper

**Uma blockchain Proof-of-Stake anônima e acessível para todos.**

**Ticker:** BYTE
**Supply:** 21.000.000 BYTE
**Criador:** 4bytes
**Slogan:** Somos 1/0.

---

## 1. Introdução

BYTE Chain é uma blockchain descentralizada baseada em Proof-of-Stake (PoS), projetada para ser acessível a qualquer pessoa, independentemente de idade, localização geográfica ou condição financeira. Diferente do sistema bancário tradicional, que exclui adolescentes, pessoas sem documentação e populações de baixa renda, a BYTE Chain não exige identidade, permissão ou intermediários.

A rede opera inteiramente sobre a rede Tor, garantindo anonimato e resistência à censura desde o primeiro bloco.

### 1.1 Por que BYTE?

- **Sem bancos:** Ninguém precisa de conta bancária para usar BYTE.
- **Sem idade mínima:** Adolescentes podem transacionar livremente.
- **Sem fronteiras:** Acesso global via rede Tor.
- **Sem taxas abusivas:** Taxa fixa de 1 BYTE por transação, com 50% queimado.
- **Proof-of-Stake justo:** Stake mínimo de apenas 0.00010000 BYTE.

---

## 2. Tokenomics

| Parâmetro | Valor |
|-----------|-------|
| Supply total | 21.000.000 BYTE |
| Decimais | 8 (1 BYTE = 100.000.000 satoshis) |
| Recompensa por bloco | 10 BYTE |
| Halving | A cada 210.000 blocos |
| Recompensa mínima | 1 BYTE |
| Taxa por transação | 0.1% (mínimo 1 BYTE) |
| Queima | 50% da taxa |
| Validador | 50% da taxa |
| Stake mínimo | 0.00010000 BYTE (10.000 satoshis) |
| Stake máximo | 100.000 BYTE |
| Unstake delay | 100 blocos |
| Bloco Genesis (fundador) | 5.000 BYTE |

### 2.1 Distribuição

Os fundadores recebem 5.000 BYTE no bloco genesis para distribuição inicial a early adopters e desenvolvimento da rede. Os outros 20.995.000 BYTE serão minerados ao longo de aproximadamente 131 anos via recompensas de bloco e halving.

### 2.2 Halving

A cada 210.000 blocos, a recompensa de bloco é reduzida pela metade:

| Período (blocos) | Recompensa |
|------------------|------------|
| 0 - 209.999 | 10 BYTE |
| 210.000 - 419.999 | 5 BYTE |
| 420.000 - 629.999 | 2.5 BYTE |
| ... | ... |
| Mínimo | 1 BYTE |

---

## 3. Proof-of-Stake (PoS)

BYTE Chain utiliza um mecanismo de consenso Proof-of-Stake puro, sem mineração (PoW). Validadores são selecionados com base na proporção de seu stake em relação ao stake total da rede.

### 3.1 Como funciona

1. Qualquer endereço com stake >= 0.00010000 BYTE é um validador.
2. A cada bloco (30 segundos), um validador é selecionado aleatoriamente, com probabilidade proporcional ao seu stake.
3. O validador selecionado monta o bloco com as transações da mempool.
4. O bloco é assinado e propagado pela rede.
5. O validador recebe a recompensa do bloco + 50% das taxas + bônus proporcional ao stake.

### 3.2 Bônus de Stake

Validadores com maior stake recebem um bônus adicional de até 50% da recompensa base:

```
bônus = (recompensa_base / 2) * (meu_stake / maior_stake_da_rede)
```

Isso incentiva validadores a manterem stakes altos, aumentando a segurança da rede.

### 3.3 Regra anti-self-mining

Um validador **não pode** minerar transações que o envolvem (como remetente ou destinatário) se houver outros validadores ativos na rede. Isso previne conflitos de interesse e garante imparcialidade.

Se o validador for o **único** na rede, ele pode minerar qualquer transação.

### 3.4 Stake e Unstake

- **Stake:** Envie BYTE para o endereço de stake. Mínimo: 0.00010000 BYTE.
- **Unstake:** Solicite a retirada. O stake fica bloqueado por 100 blocos e depois é liberado automaticamente.
- **Slashing:** Validadores maliciosos perdem de 5% a 50% do stake (queimado).

---

## 4. Transações

### 4.1 Estrutura

Cada transação contém:
- Remetente (from)
- Destinatário (to)
- Valor (amount, em satoshis)
- Taxa (fee, em satoshis)
- Timestamp
- Chave pública (65 bytes)
- Assinatura ECDSA (curva secp256k1)

### 4.2 Taxas e Queima

- Taxa: 0.1% do valor (mínimo 1 BYTE)
- 50% da taxa é queimado (enviado para endereço de queima)
- 50% da taxa vai para o validador que minerou o bloco

### 4.3 Assinaturas

Todas as transações são assinadas com ECDSA (curva prime256v1). A assinatura cobre: `from + to + amount + fee + timestamp`. Isso impede que terceiros modifiquem qualquer campo da transação.

---

## 5. Carteira e Seed Phrase

A BYTE Chain utiliza um sistema de seed phrase de 12 palavras (baseado no padrão BIP39) para geração e recuperação de carteiras.

### 5.1 Criar uma carteira

```bash
./byte-cli create
```

Isso gera 12 palavras aleatórias, deriva a chave privada via SHA256, e salva tudo em `wallet.dat`.

### 5.2 Restaurar uma carteira

```bash
./byte-cli restore
```

Digite as 12 palavras. A chave privada é regenerada e a carteira é restaurada.

### 5.3 Comandos da CLI

| Comando | Descrição |
|---------|-----------|
| `create` | Criar nova carteira |
| `restore` | Restaurar da seed phrase |
| `balance` | Ver saldo e stake |
| `send <dest> <valor>` | Enviar BYTE (ex: `send BYTE... 0.5`) |
| `stake <valor>` | Fazer stake (ex: `stake 100`) |
| `unstake` | Retirar stake |
| `peers` | Listar peers conectados |
| `status` | Status do nó |
| `getinfo` | Informações completas (JSON) |

---

## 6. Rede P2P (Tor)

Toda a comunicação entre nós é feita através da rede Tor, usando Hidden Services (.onion). Isso garante:

- **Anonimato:** IPs reais nunca são expostos.
- **Resistência à censura:** Governos não podem bloquear conexões Tor facilmente.
- **NAT traversal:** Não precisa abrir portas no roteador.

### 6.1 Seed Nodes

A rede possui 3 seed nodes oficiais hardcoded:

```
hmlv4coj5ui4nc6jvutpjsetsmyhqsb6adxwvbxttsnzwo6cw3gqn3ad.onion:9333
vah4blmjgwonpittgdtuah4kqzapzg4inwk45ouliinbbw7c74wz2syd.onion:9333
nc4pbdsvbotvdz6i2rtiwxfc5iytwaysc7yg6xlcdz3sk6i3twomwzid.onion:9333
```

Ao iniciar, cada nó tenta conectar a estes seeds para descobrir outros peers.

### 6.2 Gossip

Peers compartilham automaticamente suas listas de peers conhecidos. A rede se auto-organiza.

### 6.3 Protocolo

O protocolo P2P utiliza mensagens binárias com header de 8 bytes:

```
[type: 4 bytes] [length: 4 bytes] [payload: length bytes]
```

Tipos de mensagem:

| Tipo | Valor | Descrição |
|------|-------|-----------|
| MSG_VERSION | 1 | Handshake inicial |
| MSG_BLOCK | 6 | Bloco minerado |
| MSG_TX | 7 | Transação |
| MSG_PING | 8 | Keepalive |
| MSG_PONG | 9 | Resposta keepalive |
| MSG_PEERS | 10 | Lista de peers |

---

## 7. Sincronização e Fork Resolution

### 7.1 Sincronização

Quando um nó conecta a outro com altura maior, ele solicita os blocos faltantes via `MSG_GETBLOCKS`. O peer envia todos os blocos desde a altura solicitada.

### 7.2 Fork Resolution

Se dois validadores mineram blocos na mesma altura, ocorre um fork. A regra de resolução é:

**O bloco com menor hash vence.**

Isso é determinístico e todos os nós concordam automaticamente. Em caso de double-sign (mesmo validador assinando dois blocos diferentes na mesma altura), o validador sofre slashing.

---

## 8. Slashing

Validadores mal-intencionados são punidos com perda de stake:

| Infração | Severidade | Stake queimado |
|----------|------------|----------------|
| Double-sign | 5 | 25% |
| Outras | 1-10 | 5% - 50% |

O stake queimado é enviado para o endereço de queima (`BYTE000...000`), reduzindo o supply circulante.

---

## 9. Como rodar um nó

### 9.1 Requisitos

- Linux (x86_64) ou Termux (Android)
- Tor instalado e rodando
- GCC, Make, OpenSSL, SQLite3

### 9.2 Compilar

```bash
git clone https://github.com/seu-user/bytechain.git
cd bytechain
make
```

### 9.3 Configurar Tor Hidden Service

```bash
# Instalar Tor
sudo apt install tor

# Configurar Hidden Service
sudo nano /etc/tor/torrc
# Adicionar:
# HiddenServiceDir /caminho-do-projeto/Hidden
# HiddenServicePort 9333 127.0.0.1:9333

sudo systemctl restart tor
sudo cat /caminho-do-projeto/Hidden/hostname > hostname
```

### 9.4 Iniciar o daemon

```bash
./byted &
```

### 9.5 Criar carteira

```bash
./byte-cli create
# ANOTE AS 12 PALAVRAS!
```

### 9.6 Fazer stake e validar

```bash
./byte-cli stake 100
```

Seu nó agora é um validador e pode minerar blocos!

---

## 10. Segurança

### 10.1 Assinaturas ECDSA

Todas as transações são assinadas com ECDSA (curva prime256v1). Nós rejeitam transações com assinaturas inválidas.

### 10.2 Anti-Double-Spend

A mempool verifica se a transação já foi minerada nos últimos 100 blocos antes de aceitar.

### 10.3 Anti-Self-Connect

Nós nunca conectam a si mesmos (verificação por node_id e onion).

### 10.4 Broadcast Except

Blocos e transações nunca são reenviados ao peer de origem, prevenindo loops.

---

## 11. Conclusão

Byte é uma blockchain PoS anônima, acessível e justa. Com supply limitado, taxas baixas, e operação 100% sobre Tor, ela foi projetada para ser uma alternativa real ao sistema financeiro tradicional.

**Sem bancos. Sem fronteiras. Sem permissão.**

---

