#!/bin/bash

echo "=== Compiling blockchain.c ==="
gcc -o blockchain blockchain.c

if [ $? -ne 0 ]; then
    echo "Compilation FAILED!"
    exit 1
fi

echo "Compilation SUCCESS!"
echo ""

echo "=== Running blockchain ==="
./blockchain

echo ""
echo "=== Test Complete ==="

exit 0