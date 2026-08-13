# BYTE — Uma Blockchain Anônima Para Todos

**4bytes** | Somos 1/0

---

## Resumo

A Byte é uma criptomoeda descentralizada que opera inteiramente sobre a
rede Tor, permitindo pagamentos anónimos, sem fronteiras e sem permissão.
Utiliza Proof-of-Stake, tem um supply limitado a 21 milhões de BYTE, e
foi projetada para funcionar em qualquer dispositivo — desde um servidor
a um telemóvel Android com Termux.

Não foi criada por uma empresa ou um governo. Foi criada pela **4bytes** —
uma legião anónima que existe no digital.

**Sem bancos. Sem fronteiras. Sem idade mínima. Sem permissão.**

---

## 1. O Problema

### 1.1 Dinheiro Controlado

O sistema financeiro atual decide quem pode participar. Precisas de:

- Documentos de identidade
- Conta bancária
- Morada fixa
- Ser maior de idade
- Estar num país "aceitável"

Milhões de pessoas são excluídas. Adolescentes, refugiados, trabalhadores
informais, pessoas sem documentos — todos estão fora.

### 1.2 Vigilância Financeira

Cada compra que fazes com cartão, cada transferência bancária, cada
salário que recebes — tudo é registado, analisado e potencialmente
bloqueado por terceiros. O teu dinheiro não é realmente teu — está sob
custódia de um banco que pode decidir congelá-lo a qualquer momento.

### 1.3 Taxas e Inflação

Enviar dinheiro para outro país pode custar 20% do valor. E o dinheiro
que guardas perde valor todos os anos porque os bancos centrais imprimem
mais. A inflação é um imposto silencioso que rouba o poder de compra de
quem guarda dinheiro.

### 1.4 Exclusão Digital

Muitas criptomoedas exigem hardware especializado, conhecimentos técnicos
avançados, ou internet de alta velocidade. Isto exclui a maioria da
população mundial.

---

## 2. A Solução

A Byte resolve todos estes problemas com um sistema simples:

### 2.1 Sem Permissão

Não precisas de autorização para usar Byte. Não há formulários, não há
verificação de identidade, não há idade mínima. Só precisas de um
telemóvel e a vontade de participar.

### 2.2 Anónima por Defeito

Toda a comunicação da rede Byte passa pela rede Tor. O teu endereço IP
nunca é exposto. As transações são pseudónimas — ligadas a endereços
BYTE, não a nomes reais.

### 2.3 Taxas Justas

- Taxa fixa de 1 BYTE por transação
- 50% da taxa é queimado (reduz o supply)
- 50% vai para o validador que processou a transação

### 2.4 Supply Limitado

Apenas 21 milhões de BYTE existirão. A recompensa por bloco diminui
para metade a cada 210.000 blocos, tal como o Bitcoin. Isto protege
contra a inflação.

### 2.5 Acessível

A Byte foi desenhada para correr em telemóveis Android com Termux, em
computadores antigos, em Raspberry Pi. Não precisas de hardware caro
para participar.

---

## 3. Como Funciona uma Blockchain

Antes de entrarmos nos detalhes da Byte, é importante perceber o que é
uma blockchain e porque é que ela é segura.

### 3.1 O Problema do Gasto Duplo (Double-Spend)

Imagina que tens um ficheiro digital que representa 1 BYTE. O que te
impede de copiar esse ficheiro e enviá-lo para duas pessoas diferentes?
Num sistema digital normal, nada. Isto é o problema do **gasto duplo**.

Os bancos resolvem isto mantendo um livro centralizado — eles sabem
exatamente quanto cada conta tem. Mas isto requer confiança no banco.

### 3.2 O Livro Partilhado (Ledger)

Uma blockchain resolve o gasto duplo sem precisar de um banco. Em vez
de um livro centralizado, existe um **livro partilhado** — milhares de
cópias idênticas espalhadas pelo mundo.

Cada página deste livro é um **bloco**. Cada bloco contém:

- Um número de página (altura)
- A data e hora (timestamp)
- Uma lista de transações
- Um código que liga esta página à página anterior (hash)
- A resposta a um puzzle matemático (Proof-of-Stake, no caso da Byte)

### 3.3 O Encadeamento (Chain)

A magia está na ligação entre blocos. Cada bloco contém o **hash** do
bloco anterior. Um hash é uma impressão digital única — se mudares um
único bit no bloco, o hash muda completamente.

```
Bloco #0 (Génese):
  Hash: 0000abc123...

Bloco #1:
  Hash anterior: 0000abc123...
  Transações: Maria → João: 100 BYTE
  Hash: 0000def456...

Bloco #2:
  Hash anterior: 0000def456...
  Transações: João → Ana: 50 BYTE
  Hash: 0000ghi789...
```

Se alguém tentar alterar uma transação no Bloco #1:

1. O hash do Bloco #1 muda
2. O Bloco #2 aponta para o hash antigo do Bloco #1
3. A ligação parte-se
4. Todos os nós da rede rejeitam a alteração

Para falsificar uma transação, terias de refazer todos os blocos
seguintes — e fazer isto mais rápido do que todos os outros nós
da rede juntos. Isto é computacionalmente impossível.

### 3.4 O Consenso

A rede precisa de concordar sobre qual é a versão correta do livro.
O mecanismo que decide isto chama-se **consenso**.

Na Byte, o consenso é **Proof-of-Stake (PoS)** — prova de participação.
Em vez de gastar eletricidade a resolver puzzles (como o Bitcoin), os
validadores bloqueiam BYTE como garantia e são escolhidos aleatoriamente
para criar blocos.

Isto torna a Byte energeticamente eficiente e acessível a qualquer pessoa.

### 3.5 Imutabilidade

Uma vez que uma transação é incluída num bloco e blocos adicionais são
construídos por cima, ela torna-se praticamente imutável. Cada bloco
adicional é uma confirmação. Após 6 confirmações (cerca de 3 minutos na
Byte), a transação é considerada final.

---

## 4. Carteiras e Chaves

### 4.1 O Que é uma Carteira?

Uma carteira Byte não "guarda" moedas. As moedas existem na blockchain.
A carteira guarda as **chaves criptográficas** que te permitem gastar
as moedas associadas ao teu endereço.

### 4.2 Chave Privada

A chave privada é um número secreto de 256 bits (32 bytes). É gerada
aleatoriamente quando crias uma carteira. Com ela, podes assinar
transações — provar que és o dono do endereço.

**A chave privada nunca deve ser partilhada.** Quem a tem, pode gastar
todas as moedas do endereço.

Exemplo de chave privada:
```
62 79 6b 5c f2 c5 4c 96 05 3d d7 fb f1 a6 26 03
b5 a3 ba a7 74 5d 07 2e 50 54 55 3c 46 d5 5a c3
```

### 4.3 Chave Pública

A chave pública é derivada matematicamente da chave privada usando
criptografia de curva elíptica (ECDSA, curva prime256v1). É um ponto
na curva, representado por 65 bytes.

É **impossível** descobrir a chave privada a partir da chave pública.
Esta é a base da segurança da Byte.

Exemplo de chave pública:
```
04 ed 62 93 2a 6d 96 a0 bc e2 93 40 f5 d6 b7 02
bb 23 8b 30 34 1e c6 76 c7 13 0f 27 f7 0d 5b 71
c6 50 42 ac 6e 5a 41 52 d1 b0 4b 25 65 c3 84 75
4c c4 a7 8b 31 45 73 9f 41 14 be 39 ff d4 6b 43 45
```

### 4.4 Endereço

O endereço é derivado da chave pública. Primeiro, aplica-se SHA256 à
chave pública. Depois, adiciona-se o prefixo `BYTE` e trunca-se para
64 caracteres.

```
Chave pública → SHA256 → BYTE + 60 caracteres hex = Endereço
```

Exemplo de endereço:
```
BYTEc6d00a8d0670b4b87f014891e208495866b4c5cab89ba7a699888476e5bd
```

### 4.5 Seed Phrase (12 Palavras)

Para simplificar a gestão de chaves, a Byte usa uma **seed phrase** —
12 palavras aleatórias do standard BIP39. Estas palavras são passadas
pelo SHA256 para gerar a chave privada.

Exemplo de seed phrase:
```
abandon ability able about above absent absorb abstract absurd abuse access accident
```

**Guarda estas palavras!** Com elas, podes recuperar toda a tua carteira
se perderes o dispositivo. Sem elas, o teu dinheiro está perdido para
sempre.

### 4.6 Criar e Restaurar

```bash
# Criar
./byte-cli create
# Guarda as 12 palavras

# Restaurar
./byte-cli restore
# Introduz as 12 palavras
```

---

## 5. Transações em Detalhe

### 5.1 Anatomia de uma Transação

Cada transação na Byte contém:

```
┌─────────────────────────────────────────┐
│ from:         BYTEc6d00a...             │  Quem envia
│ to:           BYTE7abc...               │  Quem recebe
│ amount:       100.00000000 BYTE         │  Quantidade
│ fee:          1.00000000 BYTE           │  Taxa
│ timestamp:    1717200000                │  Data/hora
│ public_key:   04ed6293...               │  Chave pública do remetente
│ signature:    30450221...               │  Assinatura digital
│ hash:         a3f2b8c9...               │  Hash da transação
└─────────────────────────────────────────┘
```

### 5.2 Como se Cria uma Transação

1. O remetente decide enviar BYTE para um endereço
2. A carteira monta a transação com: `from`, `to`, `amount`, `fee`, `timestamp`
3. A carteira calcula o **hash da transação** (SHA256 de todos os campos)
4. A carteira **assina** este hash com a chave privada (ECDSA)
5. A transação assinada é transmitida para a rede

### 5.3 Como se Verifica uma Transação

Cada nó da rede, ao receber a transação:

1. Recalcula o hash da transação
2. Usa a chave pública (da transação) + o hash + a assinatura
3. A função `ECDSA_verify` retorna "válido" ou "inválido"
4. Se válido, verifica o saldo do remetente
5. Se tiver saldo suficiente, a transação entra na mempool

**A chave privada nunca sai do dispositivo do remetente.** Só a
assinatura e a chave pública são transmitidas.

### 5.4 O que é uma Assinatura Digital?

Uma assinatura digital é a prova matemática de que o remetente possui
a chave privada correspondente à chave pública. É gerada com ECDSA
(Elliptic Curve Digital Signature Algorithm).

- **Gerar assinatura**: requer a chave privada + o hash da transação
- **Verificar assinatura**: requer a chave pública + o hash + a assinatura
- **Forjar assinatura**: matematicamente impossível sem a chave privada

### 5.5 Taxas e Queima

Cada transação paga uma taxa mínima de 1 BYTE. Esta taxa é dividida:

- **50% queimado**: enviado para `BYTE000...000` (endereço de queima).
  Estes BYTE saem permanentemente de circulação.
- **50% para o validador**: recompensa adicional para quem minerou o bloco.

A queima torna a Byte **deflacionária** — a oferta total diminui ao longo
do tempo, aumentando o valor de cada BYTE restante.

---

## 6. Proof-of-Stake em Profundidade

### 6.1 O Problema do Proof-of-Work

O Bitcoin usa Proof-of-Work (PoW) — os mineradores gastam eletricidade
a resolver puzzles matemáticos. Isto tem problemas:

- **Desperdício de energia**: O Bitcoin consome mais eletricidade que
  muitos países.
- **Centralização**: Só quem tem hardware especializado (ASICs) pode
  minerar com lucro.
- **Inacessível**: Um adolescente com um telemóvel não pode participar.

### 6.2 Como o Proof-of-Stake Resolve

Na Byte, os validadores não gastam eletricidade. Em vez disso, bloqueiam
BYTE como **stake** (garantia). O protocolo escolhe aleatoriamente um
validador para criar cada bloco, com probabilidade proporcional ao seu
stake.

```
Probabilidade = teu_stake / stake_total_da_rede
```

Se tens 1.000 BYTE em stake e a rede tem 100.000 BYTE em stake total,
tens 1% de probabilidade de seres escolhido para cada bloco.

### 6.3 Tornar-se Validador

Qualquer pessoa pode tornar-se validadora. O stake mínimo é de apenas
**0.00010000 BYTE** (10.000 satoshis) — menos de um cêntimo.

```bash
./byte-cli stake 10
```

Isto bloqueia 10 BYTE como stake. A partir de agora, o teu nó participa
na seleção de validadores.

### 6.4 Stake Máximo

Para evitar que um único validador domine a rede, existe um limite
máximo de **100.000 BYTE** por validador. Isto promove a descentralização.

### 6.5 Bónus de Stake

Validadores com maior stake recebem um bónus adicional:

```
bónus = (recompensa_base / 2) × (meu_stake / maior_stake_da_rede)
```

Se o maior stake da rede for 100.000 BYTE e o teu for 50.000 BYTE,
recebes 25% extra sobre a recompensa base.

### 6.6 Unstake

Quando queres recuperar o teu stake, inicias o processo de **unstake**.
O stake fica bloqueado por **100 blocos** (cerca de 50 minutos) e depois
é automaticamente liberado para a tua carteira.

Este período de espera impede que validadores maliciosos retirem o stake
antes de serem punidos.

### 6.7 Slashing (Penalização)

Se um validador tentar trapacear, perde parte do seu stake:

| Infração | Stake Queimado |
|----------|----------------|
| Double-sign (assinar dois blocos na mesma altura) | 25% |
| Bloco inválido | 10% |
| Outras violações | 5% a 50% |

O stake queimado é enviado para o endereço de queima, reduzindo o supply
total. Isto alinha os incentivos — trapacear custa dinheiro real.

### 6.8 Regra Anti-Self-Mining

Um validador **não pode** incluir transações que o envolvam (como
remetente ou destinatário) se houver outros validadores ativos na rede.
Isto previne conflitos de interesse.

Se o validador for o **único** na rede, esta regra é suspensa — a rede
precisa de funcionar mesmo com um só validador.

---

## 7. Tokenomics

### 7.1 Números

| Parâmetro | Valor |
|-----------|-------|
| Supply total máximo | 21.000.000 BYTE |
| Decimais | 8 (1 BYTE = 100.000.000 unidades) |
| Bloco génesis (fundadores) | 5.000 BYTE |
| Recompensa inicial | 10 BYTE |
| Halving | A cada 210.000 blocos (~73 dias) |
| Recompensa mínima | 1 BYTE |
| Stake mínimo | 0.00010000 BYTE |
| Stake máximo | 100.000 BYTE |
| Tempo entre blocos | ~30 segundos |
| Transações por bloco | Até 20 |
| Taxa mínima | 1 BYTE |
| Queima por taxa | 50% |

### 7.2 Halving em Detalhe

A cada 210.000 blocos (aproximadamente 73 dias), a recompensa por bloco
é reduzida para metade:

| Época | Blocos | Recompensa | BYTE Criados por Época |
|-------|--------|------------|------------------------|
| 1 | 0 – 209.999 | 10 | 2.100.000 |
| 2 | 210.000 – 419.999 | 5 | 1.050.000 |
| 3 | 420.000 – 629.999 | 2.5 | 525.000 |
| 4 | 630.000 – 839.999 | 1.25 | 262.500 |
| 5 | 840.000 – 1.049.999 | 1 | 210.000 |
| 6+ | ... | 1 | 210.000 por época |

O último BYTE será minerado aproximadamente no ano 2157.

### 7.3 Curva de Emissão

```
Recompensa
   10 │●
      │ ●
    5 │  ●
      │   ●
  2.5 │    ●
      │     ●●●●●●●●●●●●●●●●●●●●●●●●●●
    1 │________________________________
      0    210k   420k   630k   840k   ∞
                    Blocos
```

### 7.4 Distribuição Inicial Justa

Ao contrário de muitas criptomoedas onde os fundadores ficam com uma
percentagem enorme do supply, a Byte reserva apenas **5.000 BYTE**
(0.024% do supply total) para os fundadores, exclusivamente para
distribuição inicial e desenvolvimento da rede.

Os outros 99.976% são distribuídos ao longo de décadas para os
validadores que mantêm a rede segura.

---

## 8. A Rede Tor

### 8.1 Porquê Tor?

A Byte opera inteiramente sobre a rede Tor para garantir:

- **Anonimato**: O teu endereço IP real nunca é exposto a outros nós
- **Resistência à censura**: Não é possível bloquear a Byte bloqueando
  IPs ou portas específicas
- **NAT traversal**: Funciona atrás de qualquer roteador, sem precisar
  de abrir portas
- **Privacidade por defeito**: Todos os utilizadores são protegidos,
  não apenas os "avançados"

### 8.2 Como Funciona o Tor na Byte

Cada nó da Byte é um **Hidden Service** (.onion). Quando o teu nó
inicia, o Tor gera um endereço `.onion` único e guarda-o em
`Hidden/hostname`.

Outros nós podem conectar-se a ti usando este endereço `.onion`, sem
nunca saber o teu IP real. Da mesma forma, quando te conectas a outros
nós, o tráfego passa pela rede Tor, protegendo a tua identidade.

### 8.3 Configurar o Tor

```bash
# Instalar Tor
sudo apt install tor

# Configurar Hidden Service
sudo nano /etc/tor/torrc
# Adicionar:
# HiddenServiceDir /caminho-do-projeto/Hidden
# HiddenServicePort 9333 127.0.0.1:9333

sudo systemctl restart tor
sudo cat /caminho-do-projeto/Hidden/hostname
# Mostra o teu endereço .onion
```

### 8.4 Seed Nodes

Três nós semente oficiais ajudam novos participantes a entrar na rede.
Estes endereços estão hardcoded no código para garantir que todos
conseguem encontrar a rede:

```
hmlv4coj5ui4nc6jvutpjsetsmyhqsb6adxwvbxttsnzwo6cw3gqn3ad.onion:9333
vah4blmjgwonpittgdtuah4kqzapzg4inwk45ouliinbbw7c74wz2syd.onion:9333
nc4pbdsvbotvdz6i2rtiwxfc5iytwaysc7yg6xlcdz3sk6i3twomwzid.onion:9333
```

Ao conectar a um seed, o teu nó descobre outros peers automaticamente
através do protocolo de gossip.

### 8.5 Propagação (Gossip)

Quando um nó recebe uma nova transação ou bloco:

1. Valida-a
2. Adiciona à sua mempool ou blockchain
3. Reencaminha para **todos** os seus peers conhecidos
4. Cada peer faz o mesmo

Em segundos, toda a rede tem a transação. Isto é o **gossip protocol**.

---

## 9. O Daemon e o CLI

### 9.1 Arquitetura

A Byte é composta por dois binários:

- **`byted`**: O daemon — corre em background, gere a blockchain, a rede
  P2P, a mempool, e o consenso PoS. É o "servidor".
- **`byte-cli`**: O cliente de linha de comandos — envia comandos ao
  daemon via socket local (porta 9334).

```
┌──────────────┐     socket TCP      ┌──────────────────┐
│  byte-cli    │ ←───────────────→  │     byted        │
│  (comandos)  │     127.0.0.1:9334 │   (daemon)       │
└──────────────┘                     │                   │
                                     │  ┌─────────────┐ │
                                     │  │ Blockchain   │ │
                                     │  │ Mempool      │ │
                                     │  │ P2P (Tor)    │ │
                                     │  │ PoS          │ │
                                     │  └─────────────┘ │
                                     └──────────────────┘
```

### 9.2 Comandos Disponíveis

| Comando | Descrição |
|---------|-----------|
| `create` | Criar nova carteira com seed phrase |
| `restore` | Restaurar carteira da seed phrase |
| `balance` | Ver saldo, stake, altura da chain |
| `send <dest> <valor>` | Enviar BYTE |
| `stake <valor>` | Bloquear BYTE como validador |
| `unstake` | Iniciar processo de liberação de stake |
| `peers` | Listar peers conectados |
| `connect <ip/url>` | Conectar a um peer específico |
| `disconnect <ip>` | Desconectar peer |
| `mempool` | Ver transações pendentes |
| `mine` | Forçar mineração (se tiver stake) |
| `getblock <altura>` | Informações de um bloco |
| `status` | Status resumido do nó |
| `getinfo` | Informações completas em JSON |
| `help` | Ajuda |

### 9.3 Exemplos de Uso

```bash
# Iniciar o daemon
./byted &

# Criar carteira
./byte-cli create

# Ver saldo
./byte-cli balance

# Enviar 100 BYTE
./byte-cli send BYTE7abc... 100

# Enviar 0.5 BYTE (com decimais)
./byte-cli send BYTE7abc... 0.5

# Fazer stake de 50 BYTE
./byte-cli stake 50

# Ver peers
./byte-cli peers

# Status
./byte-cli status
```

---

## 10. Sincronização e Forks

### 10.1 Sincronização Inicial

Quando um novo nó entra na rede:

1. Conecta aos seed nodes
2. Recebe a lista de peers
3. Pergunta a altura máxima da rede
4. Pede todos os blocos desde o bloco 0
5. Valida cada bloco
6. Reconstrói o estado completo (saldos, stakes)

Este processo é rápido — a Byte tem blocos de 30 segundos, por isso
mesmo meses de histórico são sincronizados em minutos.

### 10.2 Forks

Um **fork** ocorre quando dois validadores criam blocos diferentes na
mesma altura. Isto é normal e esperado em redes descentralizadas.

A Byte resolve forks com uma regra simples: **o bloco com menor hash
vence**. Isto é determinístico — todos os nós chegam à mesma conclusão
sem precisar de comunicação adicional.

```
Altura 100:
  Bloco A: hash = 0000abc...  ← menor hash, VENCE
  Bloco B: hash = 0000def...  ← descartado
```

### 10.3 Reorganização (Reorg)

Quando um nó descobre que está num fork minoritário:

1. Descartar os blocos do fork errado
2. Devolver as transações desses blocos à mempool
3. Reverter os saldos ao estado do ponto de fork
4. Adotar a chain mais longa/válida
5. Reaplicar as transações

Tudo isto é automático e transparente para o utilizador.

---

## 11. Segurança

### 11.1 Criptografia

A Byte usa:

- **ECDSA** (curva prime256v1 / NIST P-256) para assinaturas
- **SHA256** para hashes de transações, blocos e endereços

Ambos são standards internacionais auditados por décadas.

### 11.2 Propriedades de Segurança

| Propriedade | Como é Garantida |
|-------------|------------------|
| **Autenticidade** | Assinatura ECDSA — só o dono da chave privada pode gastar |
| **Integridade** | Hash SHA256 — qualquer alteração é detetada |
| **Não-repúdio** | Assinatura digital prova que o remetente autorizou |
| **Imutabilidade** | Encadeamento de blocos — alterar o passado requer refazer toda a chain |
| **Disponibilidade** | Rede descentralizada — sem ponto único de falha |

### 11.3 Anti-Double-Spend

Antes de aceitar uma transação na mempool, os validadores verificam se
as mesmas moedas já foram gastas nos últimos 100 blocos. Se sim, a
transação é rejeitada.

### 11.4 Self-Connect Prevention

Cada nó gera um ID aleatório (de `/dev/urandom`) no arranque. Quando
dois nós fazem handshake, comparam os IDs. Se forem iguais, a conexão
é fechada — o nó estava a tentar conectar-se a si mesmo.

Existe também uma **blacklist temporária** de 5 segundos para evitar
reconexões imediatas.

### 11.5 Slashing

Validadores mal-intencionados são punidos:

| Infração | Stake Queimado |
|----------|----------------|
| Double-sign (dois blocos na mesma altura) | 25% |
| Bloco com transações inválidas | 10% |
| Outras violações | 5% a 50% |

O stake queimado é enviado para `BYTE000...000`, reduzindo o supply.

---

## 12. Mempool

### 12.1 O que é a Mempool?

A **mempool** (memory pool) é a "sala de espera" das transações. Quando
envias uma transação, ela não entra imediatamente num bloco. Primeiro,
vai para a mempool, onde fica à espera que um validador a inclua.

### 12.2 Validação na Mempool

Antes de entrar na mempool, cada transação é verificada:

1. A assinatura é válida?
2. O remetente tem saldo suficiente?
3. A transação já não está na mempool (duplicada)?
4. A transação não foi gasta nos últimos 100 blocos?

Se passar em todas, entra na mempool e é transmitida para os peers.

### 12.3 Limites

A mempool tem capacidade para **5.000 transações** em memória. Isto é
suficiente para picos de utilização sem consumir RAM excessiva.

---

## 13. Compilar e Executar

### 13.1 Requisitos

- **Sistema**: Linux (x86_64) ou Android (Termux)
- **Compilador**: GCC ou Clang
- **Bibliotecas**: OpenSSL, SQLite3
- **Tor**: Para conectividade anónima
- **Make**: Para compilação

### 13.2 Compilar

```bash
git clone https://github.com/4bytes/byte.git
cd byte
make -j$(nproc)
```

Isto gera dois binários: `byted` e `byte-cli`.

### 13.3 Binário Estático (Sem Dependências)

```bash
gcc -static -o byte src/*.c -Iinclude -lssl -lcrypto -lz -lpthread -ldl -lm
```

### 13.4 Executar

```bash
# Terminal 1: Iniciar o daemon
./byted &

# Terminal 2: Criar carteira
./byte-cli create

# Fazer stake
./byte-cli stake 10

# Enviar BYTE
./byte-cli send BYTE7abc... 100
```

---

## 14. Quem Criou

A Byte não tem CEO, não tem escritório, não tem empresa registada.
Foi criada pela **4bytes** — uma legião anónima de desenvolvedores que
acreditam que o dinheiro deve ser livre.

**4bytes** não é uma pessoa. Não é uma equipa. É um coletivo. Somos 1/0 —
uma legião infinita de construtores que existem no digital. Não sabemos
de onde veio, nem para onde vai. Apenas construímos.

A identidade dos criadores é irrelevante. O que importa é o código — e
o código é aberto, auditável, e pertence à comunidade.

---

## 15. Conclusão

A Byte é dinheiro para todos. Para o adolescente que quer comprar um jogo
sem pedir o cartão aos pais. Para o trabalhador que quer enviar dinheiro
para a família sem perder 20% em taxas. Para quem vive num país onde a
moeda local perde valor todos os dias. Para quem não tem documentos, não
tem conta bancária, não tem "direito" ao sistema financeiro.

É uma blockchain completa — com Proof-of-Stake, carteiras seguras, rede
anónima sobre Tor, taxas justas com queima, e um supply limitado que
protege contra a inflação.

Não pede permissão. Não pede identidade. Não cobra taxas abusivas.

**Sem bancos. Sem fronteiras. Sem idade mínima. Sem permissão.**

**BYTE. Somos 1/0.**

---

<p align="center">
  <b>4bytes — Somos 1/0</b><br>
  <i>Uma legião. Infinita. Anónima.</i>
</p>
