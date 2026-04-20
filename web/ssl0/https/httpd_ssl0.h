#ifndef HTTPD_SSL0_H
#define HTTPD_SSL0_H

#include <stdint.h>

typedef struct {
    int fd;
    ssl_context ssl_ctx;
    uint8_t recv_buffer[16384];
    size_t recv_len;
    uint8_t send_buffer[16384];
    size_t send_len;
    size_t send_pos;
} httpd_ssl0_client;

int httpd_ssl0_init(const char *cert_file, const char *key_file);
int httpd_ssl0_accept(int server_fd, httpd_ssl0_client *client);
int httpd_ssl0_read(httpd_ssl0_client *client, uint8_t *buf, size_t len);
int httpd_ssl0_write(httpd_ssl0_client *client, const uint8_t *buf, size_t len);
void httpd_ssl0_close(httpd_ssl0_client *client);

#endif
