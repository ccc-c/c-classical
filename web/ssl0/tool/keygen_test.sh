#!/bin/bash

cd /Users/Shared/ccc/c0py/_more/web/ssl0

echo "=== Compiling ssh-keygen.c ==="
gcc -o tool/ssh-keygen tool/ssh-keygen.c \
    src/bignum.c src/rand.c \
    -Iinclude

if [ $? -ne 0 ]; then
    echo "Compilation FAILED!"
    exit 1
fi

echo "Compilation SUCCESS!"
echo ""

echo "=== Running ssh-keygen ==="
./tool/ssh-keygen -b 512 -f tool/test_key

echo ""
echo "=== Checking key files ==="
if [ -f tool/test_key ]; then
    echo "Private key file exists: tool/test_key"
    echo "Content:"
    cat tool/test_key
    echo ""
fi

if [ -f tool/test_key.pub ]; then
    echo "Public key file exists: tool/test_key.pub"
    echo "Content:"
    cat tool/test_key.pub
    echo ""
fi

echo "=== Test Complete ==="

exit 0