/*
  manage the pool of users waiting to play a game against another user over a network

  both users must be connected to the same iteration of nsbserver on the same machine

  all communications from the client to this module pass through the client's child server process
    until a user (client) determines he wants to play a game against another user by adding his nsbuserid
    to the waiting pool ... then the client and this module will communicate directly only to the extent
    as to determine when a game is a go
*/

#include "net.h"
#include "pglobal.h"
#include "sproto.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <syslog.h>
#include <pthread.h>

/*
    output a usage message
*/
void
usage () {
    fprintf (stderr, "\nUsage:  nsbpoolmngr -v\n");
    fprintf (stderr, "The -v option prints version info and exits.\n\n");
    fprintf (stderr, "Otherwise, this program should be invoked only by nsbserver.\n\n");
}

/*
    output a version message
*/
void
version () {
    fprintf (stderr, "\nVersion info:  nsbpoolmngr v0.9.9.7\n\n");
}

/*
    main processing
*/
int
main (int argc, char *argv[]) {
    int port = -1, x, pid;

    listensock = -1;
    syslog_ent = YES;  /* always provide syslog entries */

    if (argc > 2) {
        usage ();
        exit (-1);
    }
    if (argc == 2) {
        if (!strncmp (argv[1], "-v", 2)) {
            version ();
            exit (0);
        }
        else {
            usage ();
            exit (-1);
        }
    }
    else {
        FILE *in;
        char dummy[100], *argwp[5];

        strcpy (&dummy[0], "/tmp/nsbpoolmngr-ind");
        if ((in = fopen (dummy, "r")) != NULL)
            fclose (in);
        else {
            syslog (LOG_INFO, "being invoked from something other than nsbserver");
            exit (0);
        }

        if ((pid = fork ()) < 0) {
            /* error */
            syslog (LOG_INFO, "had to quit - fork error");
            exit (0);
        }
        else
            if (!pid) {
                /* remove the indicator file */
                argwp[0] = "/bin/rm";
                argwp[1] = "/tmp/nsbpoolmngr-ind";
                argwp[2] = NULL;
                if (execv (argwp[0], argwp) < 0)  {
                    syslog (LOG_INFO, "could not remove /tmp/nsbpoolmngr-ind");
                    exit (0);
                }
            }
            else
                wait (0);
    }

    port = atoport ("nsbpoolmngr", "tcp", 0);
    if (port == -1) {
        fprintf (stderr, "Unable to find service: %s\n", "nsbpoolmngr");
        exit (-1);
    }

    for (x = 0; x < 100; x++)
        wpool[x].status = 0;

    if (syslog_ent == YES) {
        openlog ("nsbpoolmngr", LOG_NDELAY, LOG_USER);
        syslog (LOG_INFO, "accepting communications");
    }

    while (1) {
        pthread_t p;

        sock = get_connection_nofork (SOCK_STREAM, port, &listensock);

        /* we have a connection */

        /* use a new thread to communicate so we can continue to look for new connections */
        if (pthread_create (&p, NULL, &communicate, (void *) sock)) {
            if (syslog_ent == YES)
                syslog (LOG_INFO, "error trying to create new thread");
            sock_puts (sock, "ERROR\n");
            close (sock);
        }
    }
}

/* communicate with client */
void *
communicate (void *sck) {
    int connected = 1, x, PlayYourself;

    pthread_detach (pthread_self ());

    while (connected) {
        /* get communication */
        if (sock_gets ((int) sck, &buffer[0], sizeof (buffer)) < 0)
            connected = 0;

        if (buffer[0] == 'V') {
            /* user wants to view the names in the waiting pool */
            buffer1[0] = '\0';
            for (x = 0; x < 100; x++)
                if (wpool[x].status == 1) {
                    strcat (&buffer1[0], &wpool[x].id[0]);
                    strcat (&buffer1[0], " ");
                }
            strcat (&buffer1[0], "\n");
            sock_puts ((int) sck, &buffer1[0]);
        }

        if (buffer[0] == 'A') {
            char hn[255], port[100], *blank;

            /* user wants to add his ID to the waiting pool */
            for (x = 0; x < 100; x++)
                if (wpool[x].status && !strcmp (&wpool[x].id[0], &buffer[1]))
                    break;
            if (x < 100)
                sock_puts ((int) sck, "DUP\n");
            else {
                for (x = 0; x < 100; x++)
                    if (!wpool[x].status) {
                        wpool[x].status = 1;
                        wpool[x].sock = (int) sck;
                        strcpy (&wpool[x].id[0], &buffer[1]);
                        break;
                    }
                if (x < 100) {
                    int sockts;

                    /* send ok to client and get back the hostname and port number the client will listen on for further info */
                    sock_puts ((int) sck, "OK\n");
                    if (sock_gets ((int) sck, &buffer[0], sizeof (buffer)) < 0) {
                        connected = 0;
                        continue;
                    }

                    blank = index (&buffer[0], ' ');
                    *blank = '\0';
                    strcpy (&hn[0], &buffer[0]);
                    blank++;
                    strcpy (&port[0], blank);
                    sleep (2);  /* give the client time to open the port before connecting */
                    sockts = make_connection (&port[0], SOCK_STREAM, &hn[0], 0);
                    if (sockts == -1) {
                        syslog (LOG_INFO, "error opening secondary socket to client");
                        connected = 0;
                    }
                    else {
                        /* save socket id to use to alert user if someone wants to play him/her */
                        wpool[x].sock2 = sockts;
                        buffer[0] = '\0';
                    }
                }
                else
                    sock_puts ((int) sck, "ERROR\n");
            }
        }

        if (buffer[0] == 'R') {
            /* user wants to remove his ID from the waiting pool */
            for (x = 0; x < 100; x++)
                if (wpool[x].status)
                    if (!strcmp (&buffer[1], &wpool[x].id[0])) {
                        wpool[x].status = 0;
                        break;
                    }
            if (x < 100) {
                sock_puts (wpool[x].sock2, "R\n");
                sock_puts ((int) sck, "OK\n");
                shutdown (wpool[x].sock2, SHUT_RDWR);
                close (wpool[x].sock2);
            }
        }

        if (buffer[0] == 'P') {
            char challenger[50], challengee[50], *blank;

            /* user wants to play a game against another user in the waiting pool */
            blank = index (&buffer[1], ' ');
            *blank = '\0';
            strcpy (&challengee[0], &buffer[1]);
            blank++;
            strcpy (&challenger[0], blank);

            for (PlayYourself = x = 0; x < 100; x++)
                if (wpool[x].status)
                    if (!strcmp (&challengee[0], &wpool[x].id[0])) {
                        if ((int) sck == wpool[x].sock) {
                            PlayYourself = 1;
                            break;
                        }
                        /* the requestee ID is in the waiting pool
                           verify with requestee that he/she wants to play this requester */
                        buffer1[0] = 'C';
                        strcpy (&buffer1[1], &challenger[0]);
                        strcat (&buffer1[0], "\n");
                        /* send a msg to the requestee's client saying a user is requesting to play him/her */
                        sock_puts (wpool[x].sock2, &buffer1[0]);
                        if (sock_gets (wpool[x].sock2, &buffer1[0], sizeof (buffer1)) < 0)
                            break;
                        if (!strncmp (&buffer1[0], "OK ", 3)) {
                            /* everything is hunky-dory ... set it up */
                            char tsc[6];

                            /* save port number to use for communications between the requester client and the
                               requestee child server process */
                            strcpy (&tsc[0], &buffer1[3]);

                            /* pass port number onto requester client */
                            strcpy (&buffer1[0], "COMM ");
                            strcat (&buffer1[0], &tsc[0]);
                            strcat (&buffer1[0], "\n");
                            sock_puts ((int) sck, &buffer1[0]);

                            /* close secondary socket */
                            shutdown (wpool[x].sock2, SHUT_RDWR);
                            close (wpool[x].sock2);
                            /* remove requestee from Waiting Pool */
                            wpool[x].status = 0;
                        }
                        else
                            sock_puts ((int) sck, "NOTPLAY\n");
                        break;
                    }
            if (x == 100)
                sock_puts ((int) sck, "NOTHERE\n");
            if (PlayYourself)
                sock_puts ((int) sck, "PLAYSELF\n");
        }
    }

    /* this socket closed for one reason or another, make sure the user is removed from the waiting pool if he's in it */
    for (x = 0; x < 100; x++)
        if (wpool[x].status && wpool[x].sock == (int) sck) {
            wpool[x].status = 0;
            shutdown (wpool[x].sock2, SHUT_RDWR);
            close (wpool[x].sock2);
            break;
        }

    return 0;
}

