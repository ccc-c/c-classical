# HTTPS Server 替換 OpenSSL 規劃

## 1. 現有 https_server.c 使用的 OpenSSL 功能

```c
// SSL 初始化
SSL_library_init();
SSL_load_error_strings();
OpenSSL_add_all_algorithms();

// SSL Context 建立
SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());

// 憑證/金鑰載入
SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM);
SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);
SSL_CTX_check_private_key(ctx);

// SSL 連線建立
SSL *ssl = SSL_new(ctx);
SSL_set_fd(ssl, client_fd);
SSL_accept(ssl);

// 資料讀寫
SSL_read(ssl, buffer, size);
SSL_write(ssl, data, size);

// 清理
SSL_free(ssl);
SSL_CTX_free(ctx);
EVP_cleanup();
```

## 2. ssl0 目前已支援的功能

| 函式 | 功能 |
|------|------|
| `ssl_compute_master_secret()` | 計算 Master Secret |
| `ssl_derive_keys()` | 從 Master Secret 導出金鑰 |
| `ssl_handshake_client()` | Client 端 TLS 握手 |
| `ssl_encrypt_record()` | 加密 TLS Record |
| `ssl_decrypt_record()` | 解密 TLS Record |

## 3. 缺少的基礎函式庫

### 3.1 X.509 憑證解析 (certificate.h / certificate.c)
- 解析 PEM 格式的 X.509 憑證
- 提取公鑰 (RSA public key)
- 驗證憑證基本結構

```c
int x509_parse_from_pem(const char *pem, x509_cert *cert);
int x509_get_public_key(const x509_cert *cert, uint8_t *n, size_t *n_len, uint8_t *e, size_t *e_len);
```

### 3.2 亂數生成器 (rand.h / rand.c)
- TLS 需要安全的亂數來源
- 可使用 /dev/urandom 或 arc4random

```c
void rand_bytes(uint8_t *buf, size_t len);
```

### 3.3 TLS 握手伺服器端 (ssl.h 擴充)
- 實作 ServerHello
- 處理 Certificate, ServerKeyExchange, ServerHelloDone
- 產生 ServerRandom

```c
int ssl_handshake_server(ssl_context *ctx,
                         const uint8_t *server_cert, size_t cert_len,
                         uint8_t *server_random);
```

### 3.4 TCP/SSL 簡易包裝 (ssl.h 擴充)
- 將 socket fd 與 ssl_context 綁定
- 簡化 read/write 介面

```c
typedef struct {
    int fd;
    ssl_context ctx;
} ssl_connection;

int ssl_accept(ssl_connection *conn, int fd);
int ssl_read(ssl_connection *conn, uint8_t *buf, size_t len);
int ssl_write(ssl_connection *conn, const uint8_t *buf, size_t len);
void ssl_close(ssl_connection *conn);
```

## 4. 實作順序

1. **rand.h/c** - 亂數生成器（最簡單）
2. **certificate.h/c** - X.509 憑證解析（中等複雜度）
3. **ssl.h/c 擴充** - Server 端握手與連線管理
4. **修改 https_server.c** - 替換 OpenSSL

## 5. 預期結果

修改後的 https_server.c：
- 不需要 `-lssl -lcrypto`
- 使用 ssl0 的 static library 或直接 linking
- 從檔案讀取 PEM 憑證（可用 openssl 產生）
- 提供 HTTPS 服務

## 6. 限制與簡化

- 暂不支援 client certificate authentication
- 暂不支援完整 TLS handshake 驗證
- 暂不支援 SNI (Server Name Indication)
- 暂不支援 session resumption
- 僅支援 TLS 1.2 / RSA