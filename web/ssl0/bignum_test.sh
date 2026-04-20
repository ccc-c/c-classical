#!/bin/bash

set -e
cd "$(dirname "$0")"

echo "=== Building bignum test ==="
gcc -I include -o test/bignum_test test/bignum_test.c src/bignum.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Running bignum test ==="
./test/bignum_test

echo "=== All bignum tests passed ==="