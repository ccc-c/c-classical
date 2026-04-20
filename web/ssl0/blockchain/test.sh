#!/bin/bash

cd ..
echo "=== Compiling blockchain.c ==="
gcc -o blockchain/blockchain blockchain/blockchain.c src/sha.c -Iinclude

if [ $? -ne 0 ]; then
    echo "Compilation FAILED!"
    exit 1
fi

echo "Compilation SUCCESS!"
echo ""

echo "=== Running blockchain ==="
./blockchain/blockchain

echo ""
echo "=== Test Complete ==="

exit 0