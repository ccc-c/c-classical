#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building Certificate test ==="
gcc -I include -o test/certificate_test test/certificate_test.c \
    src/certificate.c

echo "=== Running Certificate test ==="
./test/certificate_test

echo "=== All Certificate tests passed ==="