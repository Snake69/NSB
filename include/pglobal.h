#ifndef _PGLOBAL_H_
#define _PGLOBAL_H_

#define NO 0
#define YES 1

int listensock;          /* so that we can close sockets on ctrl-c */
char buffer[1000000], buffer1[1000000];
int sock, sockts;
int syslog_ent;          /* set to YES if syslog entries desired */

/* waiting pool */
struct {
    char id[20];          /* NetStats Baseball ID */
    int  sock,            /* primary socket connection */
         sock2,           /* secondary socket connection */
         status;          /* activity indicator; 0 = inactive, 1 = active, 2 = this id currently playing a game */
} wpool[100];

#endif

