#!/bin/bash

set -e
cd "$(dirname "$0")"

echo "=== Building rand test ==="
gcc -I include -o test/rand_test test/rand_test.c src/rand.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== Running rand test ==="
./test/rand_test

echo "=== All rand tests passed ==="