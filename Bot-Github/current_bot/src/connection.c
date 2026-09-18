#include "connection.h"
#include "log.h"

int set_nonblocking(Connection *conn)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(conn->sock, FIONBIO, &mode);
#else
    int flags = fcntl(conn->sock, F_GETFL, 0);
    if (flags < 0) return -1;

    return fcntl(conn->sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

int connect_server(const char *ip, unsigned short port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close_socket(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close_socket(sock);
        return -1;
    }

    return sock;
}

bool send_packet(Connection *conn, bool enc)
{
    if (!conn || conn->sock < 0) { LOGI("[NET] send_packet: invalid connection/socket"); return false; }
    if (conn->size < 4 || conn->size > sizeof(conn->data)) { LOGI("[NET] send_packet: invalid packet size=%u", (unsigned)conn->size); return false; }
    if (enc) EncryptData(conn->data + 4, conn->size - 4, conn->data + 4, ENCRYPTION_KEY);
    int sent = send(conn->sock, (const char *)conn->data, (int)conn->size, 0);
    if (sent < 0) {
#ifdef _WIN32
        LOGI("[NET] send FAILED bytes=%u socket_error=%d", (unsigned)conn->size, WSAGetLastError());
#else
        LOGI("[NET] send FAILED bytes=%u errno=%d", (unsigned)conn->size, errno);
#endif
        return false;
    }
    LOGI("[NET] send result=%d/%u", sent, (unsigned)conn->size);
    if ((uint16_t)sent != conn->size) { LOGI("[NET] PARTIAL packet transmission: sent=%d expected=%u", sent, (unsigned)conn->size); return false; }
    return true;
}

void disconnect(Connection *c)
{
    if (c->sock >= 0)
        close_socket(c->sock);
        c->sock = -1;
}


void reset_connection(Connection *c)
{
    c->sock = -1;
    
    memset(c, 0, sizeof(*c));
}