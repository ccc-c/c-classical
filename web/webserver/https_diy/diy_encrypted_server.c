// diy_encrypted_server.c — 自製 ChaCha20 加密伺服器
// 編譯: gcc -o diy_server diy_encrypted_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE     8192
#define PUBLIC_DIR      "./public"
#define DEFAULT_PORT    8443
#define KEY_SIZE        32
#define NONCE_SIZE      12
#define BLOCK_SIZE      64

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

#define ROTL(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
#define LOAD32_LE(p) (((u32)(p)[0]) | ((u32)(p)[1] << 8) | ((u32)(p)[2] << 16) | ((u32)(p)[3] << 24))
#define STORE32_LE(p, v) do { (p)[0] = (u8)(v); (p)[1] = (u8)((v) >> 8); (p)[2] = (u8)((v) >> 16); (p)[3] = (u8)((v) >> 24); } while(0)

static const u8 DEFAULT_KEY[KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

static void chacha20_block(u32 out[16], const u32 in[16]) {
    u32 x[16];
    for (int i = 0; i < 16; i++) x[i] = in[i];

    for (int i = 0; i < 10; i++) {
        #define QR(a,b,c,d) \
            x[a] += x[b]; x[d] ^= x[a]; x[d] = ROTL(x[d], 16); \
            x[c] += x[d]; x[b] ^= x[c]; x[b] = ROTL(x[b], 12); \
            x[a] += x[b]; x[d] ^= x[a]; x[d] = ROTL(x[d], 8); \
            x[c] += x[d]; x[b] ^= x[c]; x[b] = ROTL(x[b], 7);

        QR(0, 4, 8, 12)
        QR(1, 5, 9, 13)
        QR(2, 6, 10, 14)
        QR(3, 7, 11, 15)
        QR(0, 5, 10, 15)
        QR(1, 6, 11, 12)
        QR(2, 7, 8, 13)
        QR(3, 4, 9, 14)
    }

    for (int i = 0; i < 16; i++) out[i] = x[i] + in[i];
}

typedef struct {
    u32 state[16];
    u8 block[64];
    int block_pos;
} ChaChaContext;

static void chacha20_init(ChaChaContext *ctx, const u8 *key, const u8 *nonce, u32 counter) {
    const char *constants = "expand 32-byte k";
    ctx->state[0] = LOAD32_LE((u8*)constants + 0);
    ctx->state[1] = LOAD32_LE((u8*)constants + 4);
    ctx->state[2] = LOAD32_LE((u8*)constants + 8);
    ctx->state[3] = LOAD32_LE((u8*)constants + 12);

    for (int i = 0; i < 8; i++) ctx->state[4 + i] = LOAD32_LE(key + i * 4);

    ctx->state[12] = counter;
    ctx->state[13] = LOAD32_LE(nonce + 0);
    ctx->state[14] = LOAD32_LE(nonce + 4);
    ctx->state[15] = LOAD32_LE(nonce + 8);

    ctx->block_pos = 64;
}

static void chacha20_gen_block(ChaChaContext *ctx) {
    u32 block[16];
    chacha20_block(block, ctx->state);
    for (int i = 0; i < 16; i++) STORE32_LE(ctx->block + i * 4, block[i]);
    ctx->state[12]++;
    ctx->block_pos = 0;
}

static void chacha20_xor(ChaChaContext *ctx, const u8 *input, u8 *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ctx->block_pos >= 64) {
            chacha20_gen_block(ctx);
        }
        output[i] = input[i] ^ ctx->block[ctx->block_pos++];
    }
}

// ------------------------------------------------------------
// 簡化加密/解密 (不使用認證標籤)
// ------------------------------------------------------------
static void encrypt_message(const u8 *key, const u8 *plaintext, size_t plain_len,
                            const u8 *nonce, u8 *ciphertext) {
    ChaChaContext ctx;
    chacha20_init(&ctx, key, nonce, 0);
    chacha20_xor(&ctx, plaintext, ciphertext, plain_len);
}

static void decrypt_message(const u8 *key, const u8 *ciphertext, size_t cipher_len,
                            const u8 *nonce, u8 *plaintext) {
    ChaChaContext ctx;
    chacha20_init(&ctx, key, nonce, 0);
    chacha20_xor(&ctx, ciphertext, plaintext, cipher_len);
}

// ------------------------------------------------------------
// HTTP 伺服器
// ------------------------------------------------------------
const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css")  == 0) return "text/css; charset=utf-8";
    if (strcmp(ext, ".js")   == 0) return "application/javascript; charset=utf-8";
    if (strcmp(ext, ".json") == 0) return "application/json; charset=utf-8";
    if (strcmp(ext, ".png")  == 0) return "image/png";
    if (strcmp(ext, ".jpg")  == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif")  == 0) return "image/gif";
    if (strcmp(ext, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico")  == 0) return "image/x-icon";
    if (strcmp(ext, ".txt")  == 0) return "text/plain; charset=utf-8";
    if (strcmp(ext, ".pdf")  == 0) return "application/pdf";
    return "application/octet-stream";
}

void send_error(int client_fd, int status_code, const char *status_text) {
    char body[512];
    snprintf(body, sizeof(body),
        "<!DOCTYPE html><html><head><title>%d %s</title></head>"
        "<body><h1>%d %s</h1></body></html>",
        status_code, status_text, status_code, status_text);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, (unsigned long)strlen(body));

    char *response = malloc(strlen(header) + strlen(body) + 1);
    sprintf(response, "%s%s", header, body);

    u8 nonce[NONCE_SIZE];
    u8 ciphertext[BUFFER_SIZE];
    for (int i = 0; i < NONCE_SIZE; i++) nonce[i] = rand() & 0xff;
    encrypt_message(DEFAULT_KEY, (u8 *)response, strlen(response), nonce, ciphertext);

    uint32_t net_len = htonl(strlen(response));
    write(client_fd, &net_len, 4);
    write(client_fd, nonce, NONCE_SIZE);
    write(client_fd, ciphertext, strlen(response));

    free(response);
}

void handle_request(int client_fd) {
    printf("收到連線 fd=%d\n", client_fd);
    fflush(stdout);

    uint32_t net_len = 0;
    ssize_t n = read(client_fd, &net_len, 4);
    if (n != 4) { close(client_fd); return; }
    uint32_t len = ntohl(net_len);
    printf("收到長度: %u\n", len);
    fflush(stdout);

    if (len > BUFFER_SIZE || len == 0) { close(client_fd); return; }

    u8 nonce[NONCE_SIZE];
    u8 ciphertext[BUFFER_SIZE];

    read(client_fd, nonce, NONCE_SIZE);
    read(client_fd, ciphertext, len);

    u8 plaintext[BUFFER_SIZE];
    decrypt_message(DEFAULT_KEY, ciphertext, len, nonce, plaintext);
    plaintext[len] = '\0';
    
    printf("解密成功: %d bytes\n", (int)len);
    printf("內容: %.50s\n", plaintext);
    fflush(stdout);

    if (strncmp((char *)plaintext, "GET ", 4) != 0) {
        send_error(client_fd, 405, "Method Not Allowed");
        close(client_fd);
        return;
    }

    char *path_start = (char *)plaintext + 4;
    char *path_end = strchr(path_start, ' ');
    if (!path_end) { close(client_fd); return; }
    *path_end = '\0';

    if (strcmp(path_start, "/") == 0) path_start = "/index.html";
    if (strstr(path_start, "..")) {
        send_error(client_fd, 403, "Forbidden");
        close(client_fd);
        return;
    }

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s%s", PUBLIC_DIR, path_start);

    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(filepath, sizeof(filepath), "%s%s/index.html", PUBLIC_DIR, path_start);
    }

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        send_error(client_fd, 404, "Not Found");
        close(client_fd);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    const char *content_type = get_content_type(filepath);
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        content_type, file_size);

    char *response = malloc(strlen(header) + file_size);
    memcpy(response, header, strlen(header));
    fread(response + strlen(header), 1, file_size, fp);
    size_t total_len = strlen(header) + file_size;

    u8 out_nonce[NONCE_SIZE];
    u8 out_ciphertext[BUFFER_SIZE + 4096];
    for (int i = 0; i < NONCE_SIZE; i++) out_nonce[i] = rand() & 0xff;
    encrypt_message(DEFAULT_KEY, (u8 *)response, total_len, out_nonce, out_ciphertext);

    uint32_t net_total = htonl(total_len);
    write(client_fd, &net_total, 4);
    write(client_fd, out_nonce, NONCE_SIZE);
    write(client_fd, out_ciphertext, total_len);

    fclose(fp);
    free(response);
    printf("  200 %s (%ld bytes)\n", filepath, file_size);
    close(client_fd);
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc >= 2) port = atoi(argv[1]);

    signal(SIGPIPE, SIG_IGN);
    srand(time(NULL));

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 128) < 0) { perror("listen"); exit(1); }

    printf("============================================\n");
    printf("  DIY ChaCha20 Server 啟動中...\n");
    printf("  端口: %d\n", port);
    printf("============================================\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[%s:%d] ", client_ip, ntohs(client_addr.sin_port));

        handle_request(client_fd);
    }

    close(server_fd);
    return 0;
}