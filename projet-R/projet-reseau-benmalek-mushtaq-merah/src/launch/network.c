// network.c

#include "../headers/network.h"
#include "../headers/peers.h"
#include "../headers/struct_mess.h"
#include "../headers/structs/config.h"

#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>

int create_udp_socket(char *mcast_addr, uint16_t port, char *iface)
{
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket creation failed");
        return -1;
    }
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl O_NONBLOCK failed");
        close(sock);
        return -1;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(sock);
        return -1;
    }

    // Bind to any address
    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        return -1;
    }

    // Join multicast group
    struct ipv6_mreq mreq;
    inet_pton(AF_INET6, mcast_addr, &mreq.ipv6mr_multiaddr);
    int loopback = 0; // Disable loopback (recevoir les msgs que jai moi meme envoye sur le grp multicast )
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0)
    {
        perror("setsockopt IPV6_MULTICAST_LOOP failed");
        close(sock);
        return -1;
    }
    mreq.ipv6mr_interface = if_nametoindex(iface);

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt IPV6_ADD_MEMBERSHIP failed");
        close(sock);
        return -1;
    }

    return sock;
}

int create_tcp_listener(uint16_t port)
{
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("TCP socket creation failed");
        return -1;
    }
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl O_NONBLOCK failed");
        close(sock);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(sock);
        return -1;
    }

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("TCP bind failed");
        close(sock);
        return -1;
    }

    if (listen(sock, 10) < 0)
    {
        perror("TCP listen failed");
        close(sock);
        return -1;
    }

    return sock;
}

void print_ipv6_address(uint8_t ip[16])
{
    char ip_str[INET6_ADDRSTRLEN]; // Buffer to store the string representation of the IP
    struct in6_addr addr;

    // Copy the uint8_t array into the in6_addr structure
    memcpy(&addr.s6_addr, ip, 16);

    // Convert the binary IPv6 address to a readable string
    if (inet_ntop(AF_INET6, &addr, ip_str, sizeof(ip_str)) == NULL)
    {
        perror("inet_ntop failed");
        return;
    }
}

int accept_tcp_connection(int tcp_sock)
{
    struct sockaddr_in6 client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Accept the incoming connection
    int client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &addr_len);
    if (client_sock < 0)
    {
        perror("accept failed");
        return -1;
    }

    // Convert client IP to printable string for logging
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &client_addr.sin6_addr, client_ip, INET6_ADDRSTRLEN);

    return client_sock;
}

int connect_to_peer(uint8_t ip[16], uint16_t port)
{
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("TCP socket creation failed");
        return -1;
    }

    struct sockaddr_in6 peer_addr = {0};
    peer_addr.sin6_family = AF_INET6;
    memcpy(&peer_addr.sin6_addr.s6_addr, ip, 16);
    peer_addr.sin6_port = htons(port);

    if (connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0)
    {
        perror("TCP connect failed");
        close(sock);
        return -1;
    }

    return sock;
}

ssize_t send_message_udp_multicast(int udp_sock, char *buffer, int len, char *ip, int port)
{
    // Validate parameters
    if (udp_sock < 0 || !buffer)
    {
        errno = EINVAL;
        return -1;
    }
    // Prepare multicast destination address
    struct sockaddr_in6 mcast_addr = {0};
    mcast_addr.sin6_family = AF_INET6;
    mcast_addr.sin6_port = htons(port);

    // Convert multicast IP string to binary format
    if (inet_pton(AF_INET6, ip, &mcast_addr.sin6_addr) != 1)
    {
        perror("inet_pton failed");
        return -1;
    }

    // Set the interface index (scope) - important for multicast
    mcast_addr.sin6_scope_id = if_nametoindex("eth0"); // Or your interface name

    // Send the message to multicast group
    ssize_t sent = sendto(udp_sock,
                          buffer,
                          len,
                          0,
                          (struct sockaddr *)&mcast_addr,
                          sizeof(mcast_addr));

    if (sent < 0)
    {
        perror("sendto failed");
    }

    return sent;
}

int send_message_udp_unicast(int udp_sock, message *msg, struct sockaddr_in6 *dest_addr)
{
    char buffer[sizeof(message)];
    int r = struct2buf(msg, buffer); // Using your existing struct2buf function

    // Send the message
    ssize_t bytes_sent = sendto(udp_sock,
                                buffer,
                                r,
                                0,
                                (struct sockaddr *)dest_addr,
                                sizeof(struct sockaddr_in6));

    if (bytes_sent < 0)
    {
        perror("UDP unicast send failed");
        return -1;
    }

    // Optional debug output
    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &dest_addr->sin6_addr, addr_str, INET6_ADDRSTRLEN);
    printf("Sent %zd bytes to [%s]:%d\n",
           bytes_sent,
           addr_str,
           ntohs(dest_addr->sin6_port));

    return 0;
}

ssize_t send_message(int sock, int len, char *buf)
{
    ssize_t total_sent = 0;
    while (total_sent < len)
    {
        ssize_t sent = send(sock, buf + total_sent, len - total_sent, 0);
        if (sent <= 0)
        {

            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

int receive_message_udp_and_extract_adr(int sock, message **msg, struct sockaddr_in6 *src_addr, socklen_t *addrlen, int timeout_sec)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    struct timeval tv = {timeout_sec, 0};

    int ready = select(sock + 1, &fds, NULL, NULL, timeout_sec >= 0 ? &tv : NULL);
    if (ready <= 0)
    {
        return ready; // Timeout or error
    }

    char buffer[1024];
    ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0,
                           (struct sockaddr *)src_addr, addrlen);
    if (len <= 0)
    {
        return -1; // Error or connection closed
    }

    int n = 0;
    *msg = buf2struct(buffer, &n);
    return 1;
}

int receive_message_tcp(int sock, char *buf, int timeout_sec)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    struct timeval tv = {timeout_sec, 0};

    int ready = select(sock + 1, &fds, NULL, NULL, timeout_sec >= 0 ? &tv : NULL);
    if (ready <= 0)
    {
        printf("We have a timeout or an error\n");
        return ready;
    }
    // TODO : faire une boucle de lecture
    int len = recv(sock, buf, MTU, 0);
    if (len <= 0)
    {
        perror("tcp recv");
        return 0;
    }

    return len;
}

void close_socket(int sock)
{
    if (sock >= 0)
    {
        close(sock);
    }
}