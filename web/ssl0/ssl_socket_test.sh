#!/bin/bash

set -e
cd "$(dirname "$0")"

echo "=== Building ssl_socket_test ==="
gcc -I include -o test/ssl_socket_test test/ssl_socket_test.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Running ssl_socket test ==="
./test/ssl_socket_test

echo "=== All ssl_socket tests passed ==="