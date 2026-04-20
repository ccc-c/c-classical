#!/usr/bin/env python3
"""DIY ChaCha20 加密客戶端 - 修正 nonce 使用"""

import socket
import struct
import os

KEY = bytes([i for i in range(32)])

def chacha20_encrypt(key, nonce, counter, data):
    """純 Python ChaCha20 加密 - 修正版"""
    
    def rotl32(v, n):
        return ((v << n) | (v >> (32 - n))) & 0xFFFFFFFF
    
    def load32(b):
        return int.from_bytes(b[:4], 'little')
    
    def store32(v):
        return v.to_bytes(4, 'little')
    
    # 初始化 state
    constants = b'expand 32-byte k'
    state = [
        load32(constants[0:4]),
        load32(constants[4:8]),
        load32(constants[8:12]),
        load32(constants[12:16]),
        load32(key[0:4]),
        load32(key[4:8]),
        load32(key[8:12]),
        load32(key[12:16]),
        load32(key[16:20]),
        load32(key[20:24]),
        load32(key[24:28]),
        load32(key[28:32]),
        counter,
        load32(nonce[0:4]),
        load32(nonce[4:8]),
        load32(nonce[8:12])
    ]
    
    def quarter_round(st, a, b, c, d):
        st[a] = (st[a] + st[b]) & 0xFFFFFFFF
        st[d] ^= st[a]
        st[d] = rotl32(st[d], 16)
        st[c] = (st[c] + st[d]) & 0xFFFFFFFF
        st[b] ^= st[c]
        st[b] = rotl32(st[b], 12)
        st[a] = (st[a] + st[b]) & 0xFFFFFFFF
        st[d] ^= st[a]
        st[d] = rotl32(st[d], 8)
        st[c] = (st[c] + st[d]) & 0xFFFFFFFF
        st[b] ^= st[c]
        st[b] = rotl32(st[b], 7)
    
    def chacha20_block(st):
        x = st.copy()
        for _ in range(10):
            quarter_round(x, 0, 4, 8, 12)
            quarter_round(x, 1, 5, 9, 13)
            quarter_round(x, 2, 6, 10, 14)
            quarter_round(x, 3, 7, 11, 15)
            quarter_round(x, 0, 5, 10, 15)
            quarter_round(x, 1, 6, 11, 12)
            quarter_round(x, 2, 7, 8, 13)
            quarter_round(x, 3, 4, 9, 14)
        return [(x[i] + st[i]) & 0xFFFFFFFF for i in range(16)]
    
    output = bytearray()
    i = 0
    while i < len(data):
        block = chacha20_block(state)
        block_bytes = b''.join(store32(x) for x in block)
        
        for j in range(min(64, len(data) - i)):
            output.append(data[i + j] ^ block_bytes[j])
        
        i += 64
        state[12] = (state[12] + 1) & 0xFFFFFFFF
    
    return bytes(output)

# 測試 - 使用固定 nonce 方便調試
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('127.0.0.1', 8443))

msg = b'GET / HTTP/1.1\r\n\r\n'
print(f"原始請求: {msg}")

# 測試1: 使用全零 nonce
nonce = bytes(12)
ciphertext = chacha20_encrypt(KEY, nonce, 0, msg)
print(f"Nonce: {nonce.hex()}")
print(f"Ciphertext: {ciphertext.hex()}")

length = struct.pack('>I', len(msg))
sock.send(length)
sock.send(nonce)
sock.send(ciphertext)
print("已發送")

sock.settimeout(3)
try:
    # 接收回應
    resp_len_data = sock.recv(4)
    if resp_len_data:
        resp_len = struct.unpack('>I', resp_len_data)[0]
        print(f"\n回應長度: {resp_len}")
        
        resp_nonce = sock.recv(12)
        print(f"回應 nonce: {resp_nonce.hex()}")
        
        resp_ct = sock.recv(resp_len)
        
        # 使用伺服器提供的 nonce 解密
        resp_plain = chacha20_encrypt(KEY, resp_nonce, 0, resp_ct)
        
        # 找到 HTTP header 結尾
        header_end = resp_plain.find(b'\r\n\r\n')
        if header_end != -1:
            header = resp_plain[:header_end].decode('utf-8', errors='replace')
            print("\n=== HTTP 標頭 ===")
            print(header)
            print("\n=== 網頁內容 (前 200 字) ===")
            body = resp_plain[header_end + 4:header_end + 4 + 200]
            print(body.decode('utf-8', errors='replace'))
        else:
            print("回應:")
            print(resp_plain.decode('utf-8', errors='replace'))
except socket.timeout:
    print("超時")
except Exception as e:
    import traceback
    print(f"錯誤: {e}")
    traceback.print_exc()

sock.close()