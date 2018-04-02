/*
    allow the human to interact with the gameplay and make decisions here
*/

#include <stdlib.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"

void
humanaction () {
    int x, field, innings, vruns, hruns;

    if (!vmanager && !hmanager)
        /* neither manager is a human player */
        return;
    if (!action_ind)
        /* no processable action */
        return;
    if (forfeit == -1)
        /* game forfeited ... no human action acceptable */
        return;

    innings = (game_status.half_inning - 1) / 2 + 1;
    for (vruns = hruns = x = 0; x < innings; x++) {
        vruns += game_status.inning_score[x][0];
        hruns += game_status.inning_score[x][1];
    }
    field = 1 - (game_status.half_inning % 2);

    /* check to see if the game is over */
    if (innings >= 9 && (game_status.half_inning % 2) == 0 && vruns > hruns)
        return;
    if (innings >= 9 && (game_status.half_inning % 2) == 1 && vruns < hruns)
        return;

    if (game_status.outs == 0 && game_status.baserunners[0] == 99 && game_status.baserunners[1] == 99 && game_status.baserunners[2] == 99) {
        /* it's the beginning of a half-inning */

        /* check if any players need to be replaced */
        if (!field && vmanager)
            rep_plyr (field);
        if (field && hmanager)
            rep_plyr (field);
    }

    /* do defensive strategies */
    if (!field && vmanager)
        def_choices (field);
    if (field && hmanager)
        def_choices (field);
    if (!nosend)
        send_tophalf ();

    /* do offensive strategies */
    if (field && vmanager)
        off_choices (!field);
    if (!field && hmanager)
        off_choices (!field);
}

int
def_choices (int ha) {
    char sbuf[20], rbuf[20];
    int x, field;

    field = 1 - (game_status.half_inning % 2);

    /* always allow normal, see lineups, autogame plus other stuff */
    strcpy (&sbuf[0], "DCNLAIWCOTF");
    if (game_status.baserunners[0] != 99 || game_status.baserunners[1] != 99 || game_status.baserunners[2] != 99)
        strcat (&sbuf[0], "P");
    if (game_status.baserunners[0] != 99) {
        if (game_status.baserunners[1] == 99)
            strcat (&sbuf[0], "H");
        strcat (&sbuf[0], "1");
    }
    if (game_status.baserunners[1] != 99)
        strcat (&sbuf[0], "2");
    if (game_status.baserunners[2] != 99)
        strcat (&sbuf[0], "3");

    strcat (&sbuf[0], "\n");
    if (netgame && (!field && vmanager)) {
        sock_puts (netsock, sbuf);
        if (sock_gets (netsock, &rbuf[0], sizeof (rbuf)) < 0)
            return -1;
    }
    else {
        sock_puts (sock, sbuf);
        if (sock_gets (sock, &rbuf[0], sizeof (rbuf)) < 0)
            return -1;
    }

    for (x = 0; x < strlen (&rbuf[0]); x++) {
        if (rbuf[x] == 'W')             /* entire infield in */
            game_status.status[2] = 2;
        if (rbuf[x] == 'C')             /* infield in at corners */
            game_status.status[2] = 1;
        if (rbuf[x] == 'H')             /* hold runner on first */
            game_status.status[0] = 1;
        if (rbuf[x] == 'O')             /* outfield in */
            game_status.status[3] = 1;
        if (rbuf[x] == 'F')             /* guard firstbase line */
            game_status.status[6] = 2;
        if (rbuf[x] == 'T')             /* guard thirdbase line */
            game_status.status[6] = 1;
        if (rbuf[x] == 'B')             /* guarding both lines */
            game_status.status[6] = 3;
        if (rbuf[x] == 'N')             /* pitch to batter */
            break;
        if (rbuf[x] == 'I') {           /* intentional walk */
            game_status.status[4] = 1;
            break;
        }
        if (rbuf[x] == 'L') {           /* see lineups */
            if (send_lineup_info (1))
                return -1;
            else
                def_choices (ha);
        }

        if (rbuf[x] == 'A') {           /* switch to autogame */
            if (ha)
                hmanager = 0;
            else
                vmanager = 0;
            break;
        }
        if (rbuf[x] == 'P') {           /* pitchout */
            game_status.status[5] = 1;
            break;
        }
        if (rbuf[x] == '1') {           /* pickoff attempt at firstbase */
            game_status.status[1] = 1;
            break;
        }
        if (rbuf[x] == '2') {           /* pickoff attempt at secondbase */
            game_status.status[1] = 2;
            break;
        }
        if (rbuf[x] == '3') {           /* pickoff attempt at thirdbase */
            game_status.status[1] = 3;
            break;
        }
    }
    return 0;
}

int
off_choices (int ha) {
    int field;
    char sbuf[20], rbuf[20];

    field = 1 - (game_status.half_inning % 2);

    /* always allow normal, see lineups, and autogame */
    strcpy (&sbuf[0], "OCNLA");

    if (game_status.baserunners[2] != 99 && game_status.outs < 2)
        /* allow squeeze play */
        strcat (&sbuf[0], "Q");
    if ((game_status.baserunners[1] != 99 || game_status.baserunners[0] != 99) && game_status.baserunners[2] == 99 && game_status.outs < 2)
        /* allow sacrifice bunt */
        strcat (&sbuf[0], "B");
    if (game_status.baserunners[0] != 99 && game_status.baserunners[2] == 99 && game_status.outs < 2)
        /* allow hit & run */
        strcat (&sbuf[0], "R");
    /* allow stealing */
    if (game_status.baserunners[2] != 99)
        strcat (&sbuf[0], "4");
    else
        if (game_status.baserunners[1] != 99)
            strcat (&sbuf[0], "3");
        else
            if (game_status.baserunners[0] != 99)
                strcat (&sbuf[0], "2");
    if (game_status.baserunners[2] == 99 && game_status.baserunners[1] != 99 && game_status.baserunners[0] != 99)
        strcat (&sbuf[0], "5");
    if (game_status.baserunners[2] != 99 && game_status.baserunners[1] == 99 && game_status.baserunners[0] != 99)
        strcat (&sbuf[0], "26");
    if (game_status.baserunners[2] != 99 && game_status.baserunners[1] != 99 && game_status.baserunners[0] == 99)
        strcat (&sbuf[0], "7");
    if (game_status.baserunners[2] != 99 && game_status.baserunners[1] != 99 && game_status.baserunners[0] != 99)
        strcat (&sbuf[0], "9");

    strcat (&sbuf[0], "\n");
    if (netgame && (field && vmanager)) {
        sock_puts (netsock, sbuf);
        if (sock_gets (netsock, &rbuf[0], sizeof (rbuf)) < 0)
            return -1;
    }
    else {
        sock_puts (sock, sbuf);
        if (sock_gets (sock, &rbuf[0], sizeof (rbuf)) < 0)
            return -1;
    }

    if (rbuf[0] == 'L') {                   /* see lineups */
        if (send_lineup_info (0))
            return -1;
        else
            off_choices (ha);
    }
                                            /* steal */
    if (rbuf[0] == '2' || rbuf[0] == '3' || rbuf[0] == '4' || rbuf[0] == '5' || rbuf[0] == '6' || rbuf[0] == '7' || rbuf[0] == '9')
        game_status.status[7] = rbuf[0] - '0';
    if (rbuf[0] == 'R')                     /* hit and run */
        game_status.status[8] = 1;
    if (rbuf[0] == 'B')                     /* sacrifice bunt */
        game_status.status[9] = 1;
    if (rbuf[0] == 'Q')                     /* squeeze play */
        game_status.status[10] = 1;
    if (rbuf[0] == 'A') {                   /* switch to autogame */
        if (!ha)
            vmanager = 0;
        else
            hmanager = 0;
    }

    return 0;
}
 
int
send_lineup_info (int od) {
    char buf[500], work[10];
    int innings, runs[2], field, x, y, z, zz, last, bi, bl;

    field = 1 - (game_status.half_inning % 2);
    buf[0] = field + '0';
    buf[1] = dhind + '0';
    buf[2] = '\0';

    if (!od) {
        if (!field)
            strcat (&buf[0], (char *) cnvt_int2str (2, game_status.batter[1]));
        else
            strcat (&buf[0], (char *) cnvt_int2str (2, game_status.batter[0]));
        strcat (&buf[0], (char *) cnvt_int2str (2, game_status.baserunners[0]));
        strcat (&buf[0], (char *) cnvt_int2str (2, game_status.baserunners[1]));
        strcat (&buf[0], (char *) cnvt_int2str (2, game_status.baserunners[2]));
    }
    else
        strcat (&buf[0], "99999999");

    /* all players who appear in the batting order & their position */
    for (x = 0; x < 2; x++) {
        for (y = 0; y < 9; y++) {
            for (last = 0; last < 30; last++)
                if (border[x][y].player[last] == 99)
                    break;
                else {
                    strcat (&buf[0], (char *) cnvt_int2str (2, border[x][y].player[last]));
                    strcat (&buf[0], (char *) cnvt_int2str (2, border[x][y].pos[last]));
                }
            /* delimiter for that batting order position */
            strcat (&buf[0], ":");
        }
        /* two consecutive colons ends that team's stats */
        strcat (&buf[0], ":");
    }

    /* all pitchers who pitched */
    for (x = 0; x < 2; x++) {
        for (last = 0; last < 15; last++)
            if (pitching[x].pitcher[last] == 99)
                break;
            else
                strcat (&buf[0], (char *) cnvt_int2str (2, pitching[x].pitcher[last]));
        strcat (&buf[0], ":");
    }

    strcat (&buf[0], "\n");

    if (netgame && ((od && (!field && vmanager)) || (!od && (field && vmanager)))) {
        sock_puts (netsock, buf);
        if (sock_gets (netsock, &buf[0], sizeof (buf)) < 0)
            return -1;
    }
    else {
        sock_puts (sock, buf);
        if (sock_gets (sock, &buf[0], sizeof (buf)) < 0)
            return -1;
    }

    if (strncmp (&buf[0], "No Change", 9)) {
        for (bl = strlen (&buf[0]), bi = 0; bi < bl; bi += 6) {
            work[0] = buf[bi];
            work[1] = buf[bi + 1];
            work[2] = '\0';
            y = atoi (&work[0]);
            work[0] = buf[bi + 2];
            work[1] = buf[bi + 3];
            work[2] = '\0';
            x = atoi (&work[0]);
            work[0] = buf[bi + 4];
            work[1] = buf[bi + 5];
            work[2] = '\0';
            zz = atoi (&work[0]);

            /* bring in new player */
            if (!od)
                if (zz == 12) {
                    for (z = 29; z > 0; z--) {
                        border[!field][x].player[z] = border[!field][x].player[z - 1];
                        border[!field][x].pos[z] = border[!field][x].pos[z - 1];
                    }
                    game_status.status[12] = border[!field][x].player[0] = y;
                    border[!field][x].pos[0] = 11;
                    lineupchg[!field][x] = 1;
                }
                else {
                    for (z = 29; z > 0; z--) {
                        border[!field][game_status.batter[!field]].player[z] =
                                                                         border[!field][game_status.batter[!field]].player[z - 1];
                        border[!field][game_status.batter[!field]].pos[z] = border[!field][game_status.batter[!field]].pos[z - 1];
                    }

                    game_status.status[11] = border[!field][x].player[0] = y;
                    border[!field][x].pos[0] = 10;
                    lineupchg[!field][x] = 1;
                }
            else {
                if (!dhind || (dhind && zz != 1)) {
                    for (z = 29; z > 0; z--) {
                        border[field][x].player[z] = border[field][x].player[z - 1];
                        border[field][x].pos[z] = border[field][x].pos[z - 1];
                    }
                    border[field][x].player[0] = y;
                    border[field][x].pos[0] = zz;
                    lineupchg[field][x] = 1;
                }
                else
                    dhpchg[field] = 1;

                if (zz == 1) {
                    /* calculate number of innings played and the total number of runs scored for each team */
                    innings = game_status.half_inning / 2 + 1;
                    for (runs[0] = runs[1] = z = 0; z < innings; z++) {
                        runs[0] += game_status.inning_score[z][0];
                        runs[1] += game_status.inning_score[z][1];
                    }
 
                    /* make room for the new pitcher */
                    for (z = 14; z > 0; z--) {
                        pitching[field].pitcher[z] = pitching[field].pitcher[z - 1];
                        pitching[field].innings[z] = pitching[field].innings[z - 1];
                        pitching[field].thirds[z] = pitching[field].thirds[z - 1];
                    }

                    for (z = 0; z < 11; z++)
                        if (!field) {
                            if (!strcmp (&visitor_cur.batters[y].id.name[0], &visitor_cur.pitchers[z].id.name[0]))
                                break;
                        }
                        else
                            if (!strcmp (&home_cur.batters[y].id.name[0], &home_cur.pitchers[z].id.name[0]))
                                break;

                    game_status.pitcher[field] = pitching[field].pitcher[0] = z;
                    pitching[field].innings[0] = pitching[field].thirds[0] = 0;

                    if (field)
                        home_cur.pitchers[pitching[field].pitcher[0]].pitching.games = 1;
                    else
                        visitor_cur.pitchers[pitching[field].pitcher[0]].pitching.games = 1;

                    check_dec (field, runs[field], runs[!field]);
                    check_save (field, runs[field], runs[!field]);
                }
            }

            if (!od)
                if (field)
                    visitor_cur.batters[y].hitting.games = 1;
                else
                    home_cur.batters[y].hitting.games = 1;
            else
                if (!field) {
                    if (dhind && zz == 1) {
                        visitor_cur.batters[y].fielding[1].games = 1;
                        visitor_cur.batters[y].hitting.games = 1;
                    }
                    else {
                        visitor_cur.batters[border[0][x].player[0]].fielding[border[0][x].pos[0]].games = 1;
                        if (border[0][x].player[0] != border[0][x].player[1])
                            visitor_cur.batters[border[0][x].player[0]].hitting.games = 1;
                    }
                }
                else {
                    if (dhind && zz == 1) {
                        home_cur.batters[y].fielding[1].games = 1;
                        home_cur.batters[y].hitting.games = 1;
                    }
                    else {
                        home_cur.batters[border[1][x].player[0]].fielding[border[1][x].pos[0]].games = 1;
                        if (border[1][x].player[0] != border[1][x].player[1])
                            home_cur.batters[border[1][x].player[0]].hitting.games = 1;
                    }
                }
        }
        if (!nosend) {
            if (!od)
                Send2BotHalf (!field);
            else
                Send2BotHalf (field);
            send_tophalf ();
            if (!AlreadySentData)
                if (!od)
                    if (field)
                        AlreadySentData = 1;
                    else
                        AlreadySentData = 2;
                else
                    if (!field)
                        AlreadySentData = 1;
                    else
                        AlreadySentData = 2;
            else {
                if (AlreadySentData == 1) 
                    if ((!od && !field) || (od && field))
                        AlreadySentData = 3;
                if (AlreadySentData == 2)
                    if ((!od && field) || (od && !field))
                        AlreadySentData = 3;
            }
        }
    }
    return 0;
}

void
rep_plyr (int ha) {
    rep_plyr2 (ha, 10);
    rep_plyr2 (ha, 11);
    if (game_status.status[15])
        rep_plyr3 (ha);

    if (!nosend) {
        Send2BotHalf (ha);
        send_tophalf ();
        if (!AlreadySentData)
            AlreadySentData = ha + 1;
        else {
            if (AlreadySentData == 1) 
                if (ha)
                    AlreadySentData = 3;
            if (AlreadySentData == 2)
                if (!ha)
                    AlreadySentData = 3;
        }
    }
}

/* replace a player in the lineup */
void
rep_plyr2 (int ha, int st) {
    int pl;

    for (pl = 0; pl < 9; pl++) {
        if (border[ha][pl].pos[0] != st)
            continue;

        send_lineup_info2 (ha, pl, 0);
    }
}

/* replace a player in the lineup who's been injured */
void
rep_plyr3 (int ha) {
    int pl;

    for (pl = 0; pl < 9; pl++) {
        if (ha) {
            if (!home_cur.batters[border[ha][pl].player[0]].id.injury)
                continue;
        }
        else
            if (!visitor_cur.batters[border[ha][pl].player[0]].id.injury)
                continue;

        send_lineup_info2 (ha, pl, 1);
    }

    if (nosend)
        /* otherwise this status will be turned off when the injury play-by-play item is sent */
        game_status.status[15] = 0;
}

void
send_lineup_info2 (int ha, int pl, int inji) {
    char buf[500], work[10];
    int innings, runs[2], x, y, z, zz, last;

    strcpy (&buf[0], "LU");

    buf[2] = pl / 10 + '0';
    buf[3] = pl % 10 + '0';
    buf[4] = inji + '0';
    buf[5] = ha + '0';
    buf[6] = '\0';

    for (y = 0; y < 9; y++) {
        for (last = 0; last < 30; last++)
            if (border[ha][y].player[last] == 99)
                break;
            else {
                strcat (&buf[0], (char *) cnvt_int2str (2, border[ha][y].player[last]));
                strcat (&buf[0], (char *) cnvt_int2str (2, border[ha][y].pos[last]));
            }
        /* delimiter for that batting order position */
        strcat (&buf[0], ":");
    }
    /* two consecutive colons ends that team's stats */
    strcat (&buf[0], ":");

    /* all pitchers who pitched */
    for (last = 0; last < 15; last++)
        if (pitching[ha].pitcher[last] == 99)
            break;
        else
            strcat (&buf[0], (char *) cnvt_int2str (2, pitching[ha].pitcher[last]));
    strcat (&buf[0], ":");

    strcat (&buf[0], "\n");

    if (netgame && !ha) {
        sock_puts (netsock, buf);
        if (sock_gets (netsock, &buf[0], sizeof (buf)) < 0)
            return;
    }
    else {
        sock_puts (sock, buf);
        if (sock_gets (sock, &buf[0], sizeof (buf)) < 0)
            return;
    }

    work[0] = buf[0];
    work[1] = buf[1];
    work[2] = '\0';
    y = atoi (&work[0]);
    work[0] = buf[2];
    work[1] = buf[3];
    work[2] = '\0';
    x = atoi (&work[0]);
    work[0] = buf[4];
    work[1] = buf[5];
    work[2] = '\0';
    zz = atoi (&work[0]);

    /* bring in new player */
    if (!dhind || (dhind && zz != 1)) {
        for (z = 29; z > 0; z--) {
            border[ha][x].player[z] = border[ha][x].player[z - 1];
            border[ha][x].pos[z] = border[ha][x].pos[z - 1];
        }
        border[ha][x].player[0] = y;
        border[ha][x].pos[0] = zz;
        lineupchg[ha][x] = 1;
    }
    else
        dhpchg[ha] = 1;

    if (!ha) {
        visitor_cur.batters[border[ha][x].player[0]].fielding[border[ha][x].pos[0]].games = 1;
        if (border[ha][x].player[0] != border[ha][x].player[1])
            visitor_cur.batters[border[ha][x].player[0]].hitting.games = 1;
    }
    else {
        home_cur.batters[border[ha][x].player[0]].fielding[border[ha][x].pos[0]].games = 1;
        if (border[ha][x].player[0] != border[ha][x].player[1])
            home_cur.batters[border[ha][x].player[0]].hitting.games = 1;
    }
    if (zz == 1) {
        /* calculate number of innings played and the total number of runs scored for each team */
        innings = game_status.half_inning / 2 + 1;
        for (runs[0] = runs[1] = z = 0; z < innings; z++) {
            runs[0] += game_status.inning_score[z][0];
            runs[1] += game_status.inning_score[z][1];
        }
 
        /* make room for the new pitcher */
        for (z = 14; z > 0; z--) {
            pitching[ha].pitcher[z] = pitching[ha].pitcher[z - 1];
            pitching[ha].innings[z] = pitching[ha].innings[z - 1];
            pitching[ha].thirds[z] = pitching[ha].thirds[z - 1];
        }

        for (z = 0; z < 11; z++)
            if (!ha) {
                if (!strcmp (&visitor_cur.batters[border[ha][x].player[0]].id.name[0], &visitor_cur.pitchers[z].id.name[0]))
                    break;
            }
            else
                if (!strcmp (&home_cur.batters[border[ha][x].player[0]].id.name[0], &home_cur.pitchers[z].id.name[0]))
                    break;

        game_status.pitcher[ha] = pitching[ha].pitcher[0] = z;
        pitching[ha].innings[0] = pitching[ha].thirds[0] = 0;

        if (ha)
            home_cur.pitchers[pitching[ha].pitcher[0]].pitching.games = 1;
        else
            visitor_cur.pitchers[pitching[ha].pitcher[0]].pitching.games = 1;

        check_dec (ha, runs[ha], runs[!ha]);
        check_save (ha, runs[ha], runs[!ha]);
    }
}

