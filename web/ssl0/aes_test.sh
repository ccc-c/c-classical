#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building AES test ==="
gcc -I include -o test/aes_test test/aes_test.c src/aes.c

echo "=== Running AES test ==="
./test/aes_test

echo "=== All tests passed ==="