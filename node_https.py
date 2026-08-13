#!/usr/bin/env python3
# BYTE Wallet API v-unique

from flask import Flask, request, jsonify
from flask_cors import CORS
import socket
import json
import sqlite3
import time
import base64

app = Flask(__name__)
CORS(app)

DAEMON_HOST = '127.0.0.1'
DAEMON_PORT = 9334
DB_PATH = 'bytechain.db'
SATOSHI = 100_000_000

def daemon_command(cmd):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)
        s.connect((DAEMON_HOST, DAEMON_PORT))
        s.send(cmd.encode())
        resp = b''
        while True:
            chunk = s.recv(4096)
            if not chunk: break
            resp += chunk
        s.close()
        return resp.decode().strip()
    except Exception as e:
        return json.dumps({"status": "error", "msg": str(e)})

def format_byte(satoshis):
    return f"{float(satoshis) / SATOSHI:.8f}"

def b64_to_hex(b64_str):
    """Converte base64 para hex (o app envia base64, daemon espera hex)"""
    try:
        raw = base64.b64decode(b64_str)
        return raw.hex()
    except:
        return b64_str  # se falhar, retorna como está

# ==================== REDE ====================
@app.route('/ping', methods=['GET'])
def ping():
    return jsonify({"status": "ok", "version": "3.0.0", "timestamp": int(time.time())})

@app.route('/network/stats', methods=['GET'])
def network_stats():
    try:
        conn = sqlite3.connect(DB_PATH); cursor = conn.cursor()
        cursor.execute("SELECT COUNT(*) FROM blocks"); total_blocks = cursor.fetchone()[0]
        cursor.execute("SELECT COALESCE(SUM(amount), 0) FROM transactions WHERE from_addr = 'BYTE000000000000000000000000000000000000000000000000000000000000'"); circulating = cursor.fetchone()[0]
        cursor.execute("SELECT COALESCE(SUM(balance), 0) FROM balances WHERE address = 'BYTE000000000000000000000000000000000000000000000000000000000000'"); burned = cursor.fetchone()[0]
        cursor.execute("SELECT COUNT(*) FROM transactions WHERE from_addr != 'BYTE000000000000000000000000000000000000000000000000000000000000'"); total_txs = cursor.fetchone()[0]
        cursor.execute("SELECT COUNT(*) FROM validators WHERE stake > 0"); validators = cursor.fetchone()[0]
        cursor.execute("SELECT COALESCE(SUM(stake), 0) FROM validators"); total_stake = cursor.fetchone()[0]
        conn.close()
        return jsonify({"status":"ok","total_blocks":total_blocks,"circulating_supply_byte":format_byte(circulating),"circulating_supply_sat":circulating,"burned_byte":format_byte(burned),"total_transactions":total_txs,"active_validators":validators,"total_stake_byte":format_byte(total_stake)})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

@app.route('/blocks', methods=['GET'])
def blocks():
    try:
        limit = int(request.args.get('limit', 20))
        conn = sqlite3.connect(DB_PATH); conn.row_factory = sqlite3.Row; cursor = conn.cursor()
        cursor.execute("SELECT height, hash, prev_hash, timestamp, tx_count FROM blocks ORDER BY height DESC LIMIT ?", (limit,))
        rows = cursor.fetchall(); conn.close()
        return jsonify({"status":"ok","count":len(rows),"blocks":[dict(r) for r in rows]})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

@app.route('/getblock/<int:height>', methods=['GET'])
def getblock(height):
    try:
        conn = sqlite3.connect(DB_PATH); conn.row_factory = sqlite3.Row; cursor = conn.cursor()
        cursor.execute("SELECT height, hash, prev_hash, timestamp, tx_count FROM blocks WHERE height = ?", (height,))
        row = cursor.fetchone()
        if not row: conn.close(); return jsonify({"status":"error"}), 404
        cursor.execute("SELECT tx_hash, from_addr, to_addr, amount, fee, timestamp FROM transactions WHERE block_height = ?", (height,))
        txs = [{"txid":tx['tx_hash'],"from":tx['from_addr'],"to":tx['to_addr'],"amount_byte":format_byte(tx['amount']),"amount_sat":tx['amount'],"fee_byte":format_byte(tx['fee'])} for tx in cursor.fetchall()]
        conn.close()
        return jsonify({"status":"ok","block":dict(row),"transactions":txs})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

@app.route('/stake/info', methods=['GET'])
def stake_info():
    try:
        conn = sqlite3.connect(DB_PATH); conn.row_factory = sqlite3.Row; cursor = conn.cursor()
        cursor.execute("SELECT address, stake, since, last_block, blocks_mined FROM validators WHERE stake > 0 ORDER BY stake DESC LIMIT 100")
        validators = []; total_stake = 0
        for row in cursor.fetchall():
            total_stake += row['stake']
            validators.append({"address":row['address'],"stake_byte":format_byte(row['stake']),"stake_sat":row['stake'],"since_block":row['since'],"last_block":row['last_block'],"blocks_mined":row['blocks_mined']})
        conn.close()
        return jsonify({"status":"ok","total_validators":len(validators),"total_stake_byte":format_byte(total_stake),"validators":validators})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

@app.route('/address/<address>', methods=['GET'])
def address_info(address):
    try:
        conn = sqlite3.connect(DB_PATH); conn.row_factory = sqlite3.Row; cursor = conn.cursor()
        cursor.execute("SELECT balance FROM balances WHERE address = ?", (address,)); bal = cursor.fetchone(); balance = bal['balance'] if bal else 0
        cursor.execute("SELECT stake FROM validators WHERE address = ?", (address,)); stk = cursor.fetchone(); stake_amount = stk['stake'] if stk else 0
        cursor.execute("SELECT tx_hash, from_addr, to_addr, amount, fee, timestamp, block_height FROM transactions WHERE from_addr = ? OR to_addr = ? ORDER BY block_height DESC LIMIT 50", (address, address))
        txs = [{"txid":tx['tx_hash'],"from":tx['from_addr'],"to":tx['to_addr'],"amount_byte":format_byte(tx['amount']),"amount_sat":tx['amount'],"fee_byte":format_byte(tx['fee']),"height":tx['block_height'],"direction":"sent" if tx['from_addr']==address else "received"} for tx in cursor.fetchall()]
        conn.close()
        return jsonify({"status":"ok","address":address,"balance_byte":format_byte(balance),"balance_sat":balance,"stake_byte":format_byte(stake_amount),"stake_sat":stake_amount,"transaction_count":len(txs),"transactions":txs})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

@app.route('/transaction/<txid>', methods=['GET'])
def transaction_detail(txid):
    try:
        conn = sqlite3.connect(DB_PATH); conn.row_factory = sqlite3.Row; cursor = conn.cursor()
        cursor.execute("SELECT * FROM transactions WHERE tx_hash = ?", (txid,)); row = cursor.fetchone(); conn.close()
        if not row: return jsonify({"status":"error"}), 404
        return jsonify({"status":"ok","txid":row['tx_hash'],"from":row['from_addr'],"to":row['to_addr'],"amount_byte":format_byte(row['amount']),"amount_sat":row['amount'],"fee_byte":format_byte(row['fee']),"timestamp":row['timestamp'],"height":row['block_height']})
    except Exception as e:
        return jsonify({"status":"error","msg":str(e)})

# ==================== BROADCAST ====================
@app.route('/broadcast', methods=['POST'])
def broadcast():
    data = request.json
    if not data or 'from' not in data or 'to' not in data or 'amount' not in data:
        return jsonify({"status":"error","msg":"campos obrigatorios"}), 400
    
    fee = data.get('fee', '100000000')
    sig = b64_to_hex(data.get('signature', ''))  # Converte base64 → hex
    ts = data.get('timestamp', str(int(time.time())))
    pub = data.get('public_key', '')  # Converte base64 → hex
    
    cmd = f"broadcast {data['from']} {data['to']} {data['amount']} {fee} {sig} {pub} {ts}"
    result = daemon_command(cmd)
    try:
        return jsonify(json.loads(result))
    except:
        return jsonify({"status":"ok","raw":result})

@app.route('/mempool', methods=['GET'])
def mempool():
    result = daemon_command('mempool')
    try: return jsonify(json.loads(result))
    except: return jsonify({"status":"ok","mempool":result})

@app.route('/search', methods=['GET'])
def search():
    query = request.args.get('q','')
    if not query: return jsonify({"status":"error"}), 400
    try: return jsonify({"type":"block","redirect":f"/getblock/{int(query)}"})
    except: pass
    if query.startswith('BYTE'): return jsonify({"type":"address","redirect":f"/address/{query}"})
    if len(query)==64: return jsonify({"type":"txid","redirect":f"/transaction/{query}"})
    return jsonify({"status":"error"}), 404

@app.route('/', methods=['GET'])
def index():
    return jsonify({"name":"BYTE Wallet API v-unique","endpoints":{"rede":["/ping","/network/stats","/blocks","/getblock/:h","/stake/info"],"consulta":["/address/:addr","/transaction/:txid","/search?q="],"propagacao":["/broadcast (POST)","/mempool"]}})

if __name__ == '__main__':
    print("BYTE API v-unique - Porta 8080")
    app.run(host='0.0.0.0', port=8080, debug=False)
