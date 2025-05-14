#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT "443"
#define USER_AGENT "client_https"

void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    // init la bibliothèque 
    init_openssl();

    // init contexte client
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
    {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    // connexion tcp
    struct addrinfo hints, *res;
    int sock;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(hostname, PORT, &hints, &res);

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(sock, res->ai_addr, res->ai_addrlen);

    // init connexion tls
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }

    // Construire la requête GET
    char request[1024];
    snprintf(request, sizeof(request),
             "GET / HTTP/1.0\r\n"
             "Host: %s:443\r\n"
             "User-Agent: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             hostname, USER_AGENT);

    // Envoyer la requête
    SSL_write(ssl, request, strlen(request));

    // Lire la réponse
    char buffer[4096];
    int bytes;
    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes] = 0;
        printf("%s", buffer);
    }

    // Nettoyage
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    freeaddrinfo(res);

    return 0;
}
