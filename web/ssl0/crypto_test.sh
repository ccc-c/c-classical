#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building crypto test ==="
gcc -I include -o test/crypto_test test/crypto_test.c src/crypto.c src/sha.c src/aes.c src/bignum.c src/rsa.c

echo "=== Running crypto test ==="
./test/crypto_test

echo "=== All tests passed ==="