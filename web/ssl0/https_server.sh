#!/bin/bash

# 切換到腳本所在目錄 (ssl0 根目錄)
cd "$(dirname "$0")"

# 停止可能還卡在背景的伺服器
killall httpd_ssl0 2>/dev/null || true
echo "已清理佔用 8443 port 的舊行程"

echo "=== 編譯 httpd_ssl0 ==="
gcc -o https/httpd_ssl0 https/httpd_ssl0.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/sha1.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c \
    -I include

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== 啟動伺服器 ==="
echo "請開啟你的瀏覽器並前往: https://localhost:8443"
echo " (如果 Chrome 警告這是自簽憑證，請點擊「進階」並選擇「繼續前往」)"
echo ""

# 切換到 https 目錄，確保能正確載入憑證與靜態檔目錄 (./public)
cd https
./httpd_ssl0 8443
