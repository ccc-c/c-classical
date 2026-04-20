#!/bin/bash

set -e
cd "$(dirname "$0")"

echo "=== Building RSA test ==="
gcc -I include -o test/rsa_test test/rsa_test.c src/bignum.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Running RSA test ==="
./test/rsa_test

echo "=== All RSA tests passed ==="