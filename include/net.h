#ifndef _NET_H_
#define _NET_H_

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>

struct in_addr *atoaddr (char *address);
int sock_gets (int sockfd, char *str, size_t count), sock_puts (int sockfd, const char *str),
    get_connection (int socket_type, u_short port, int *listener),
    get_connection_nofork (int socket_type, u_short port, int *listener),
    make_connection (char *service, int type, char *netaddress, int ind),
    sock_read (int sockfd, char *buf, size_t count), sock_write (int sockfd, const char *buf, size_t count),
    atoport (char *service, char *proto, int ind);

#endif
