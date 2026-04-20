#!/bin/bash

set -e
cd "$(dirname "$0")"

echo "=== Building SSL test ==="
gcc -I include -o test/ssl_test test/ssl_test.c src/ssl.c src/crypto.c src/sha.c src/aes.c src/bignum.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Running SSL test ==="
./test/ssl_test

echo "=== All SSL tests passed ==="