#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building SHA test ==="
gcc -I include -o test/sha_test test/sha_test.c src/sha.c

echo "=== Running SHA test ==="
./test/sha_test

echo "=== All tests passed ==="