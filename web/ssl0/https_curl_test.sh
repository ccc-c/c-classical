#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building httpd_ssl0 ==="
gcc -o https/httpd_ssl0 https/httpd_ssl0.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/sha1.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c \
    -I include

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Starting server ==="
cd https
./httpd_ssl0 8443 &
SERVER_PID=$!
sleep 2

echo "=== Testing with curl ==="
curl -k https://localhost:8443/ 2>/dev/null | head -20 || echo "curl failed"

echo ""
echo "=== Killing server ==="
kill $SERVER_PID 2>/dev/null || true

echo "=== Test complete ==="
