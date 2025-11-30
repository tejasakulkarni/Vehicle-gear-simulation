#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// Use the gearbox logic from demo.c (compiled as a library)
// Build with: gcc gear_server.c demo.c -DGEARBOX_DLL_ONLY -lws2_32 -o gear_server.exe
void gearbox_reset(void);
void gearbox_step(int accelerating, int braking, double dt_seconds);
double gearbox_get_speed(void);
int gearbox_get_gear(void);

static const double SERVER_DT = 0.06; // 60 ms

static void send_404(SOCKET client) {
    const char *resp =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    send(client, resp, (int)strlen(resp), 0);
}

static void send_500(SOCKET client) {
    const char *resp =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    send(client, resp, (int)strlen(resp), 0);
}

static void send_json_response(SOCKET client, double speed, int gear) {
    char body[128];
    snprintf(body, sizeof(body), "{\"speed\":%.4f,\"gear\":%d}", speed, gear);

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(body));

    send(client, header, (int)strlen(header), 0);
    send(client, body, (int)strlen(body), 0);
}

static int read_file_into_buffer(const char *path, char **out_buf, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return 0;
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        return 0;
    }
    size_t read = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (read != (size_t)sz) {
        free(buf);
        return 0;
    }
    *out_buf = buf;
    *out_len = (size_t)sz;
    return 1;
}

static void send_file_response(SOCKET client, const char *path, const char *content_type) {
    char *buf = NULL;
    size_t len = 0;
    if (!read_file_into_buffer(path, &buf, &len)) {
        send_404(client);
        return;
    }

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             content_type, len);

    send(client, header, (int)strlen(header), 0);
    send(client, buf, (int)len, 0);
    free(buf);
}

static int parse_bool_param(const char *query, const char *name) {
    if (!query || !name) return 0;
    const char *p = strstr(query, name);
    if (!p) return 0;
    p += strlen(name);
    if (*p != '=') return 0;
    p++;
    return (*p == '1' || *p == 't' || *p == 'T' || *p == 'y' || *p == 'Y');
}

static void handle_client(SOCKET client) {
    char buf[4096];
    int received = recv(client, buf, sizeof(buf) - 1, 0);
    if (received <= 0) {
        closesocket(client);
        return;
    }
    buf[received] = '\0';

    // Very small HTTP parser: read first line: METHOD PATH HTTP/...
    char method[8] = {0};
    char path[256] = {0};
    if (sscanf(buf, "%7s %255s", method, path) != 2) {
        send_500(client);
        closesocket(client);
        return;
    }

    if (strcmp(method, "GET") == 0 && (strcmp(path, "/") == 0 || strncmp(path, "/index.html", 11) == 0)) {
        send_file_response(client, "index.html", "text/html; charset=utf-8");
    } else if (strcmp(method, "GET") == 0 && strncmp(path, "/step", 5) == 0) {
        const char *q = strchr(path, '?');
        const char *query = q ? q + 1 : NULL;
        int accelerating = parse_bool_param(query, "accelerate");
        int braking = parse_bool_param(query, "brake");

        gearbox_step(accelerating, braking, SERVER_DT);
        double speed = gearbox_get_speed();
        int gear = gearbox_get_gear();
        // Final safety check: ensure gear never exceeds 5
        if (gear > 5) gear = 5;
        if (gear < 1) gear = 1;
        send_json_response(client, speed, gear);
    } else if (strcmp(method, "GET") == 0 && strncmp(path, "/reset", 6) == 0) {
        gearbox_reset();
        int gear = gearbox_get_gear();
        // Final safety check: ensure gear never exceeds 5
        if (gear > 5) gear = 5;
        if (gear < 1) gear = 1;
        send_json_response(client, gearbox_get_speed(), gear);
    } else {
        send_404(client);
    }

    closesocket(client);
}

int main(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        WSACleanup();
        return 1;
    }

    u_long mode = 1;
    ioctlsocket(server, FIONBIO, &mode); // non-blocking accept optional, but fine

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }

    gearbox_reset();
    printf("Gearbox C server running on http://localhost:8080\n");

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client = accept(server, (struct sockaddr *)&client_addr, &client_len);
        if (client == INVALID_SOCKET) {
            // small sleep to avoid busy loop on non-blocking socket
            Sleep(10);
            continue;
        }
        handle_client(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}



