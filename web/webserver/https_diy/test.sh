#!/bin/bash

echo "=== DIY ChaCha20 加密伺服器測試 ==="

cd "$(dirname "$0")"

echo "1. 編譯伺服器..."
gcc -o diy_server diy_encrypted_server.c
if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "2. 啟動伺服器..."
./diy_server 8443 &
SERVER_PID=$!
sleep 1

echo "3. 執行客戶端..."
python3 diy_client.py

echo ""
echo "4. 清理..."
kill $SERVER_PID 2>/dev/null

echo "=== 完成 ==="