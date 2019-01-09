/*
    the computer makes decisions here
*/

#include <stdlib.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"

struct {
    int pitcher, stat[2];
} work[12];

void
compai () {
    int x, field, innings, vruns, hruns;

    if (vmanager && hmanager)
        /* neither manager is the computer */
        return;
    if (!action_ind)
        /* no processable action */
        return;
    if (forfeit == -1)
        /* game forfeited ... no action acceptable */
        return;

    innings = (game_status.half_inning - 1) / 2 + 1;
    for (vruns = hruns = x = 0; x < innings; x++) {
        vruns += game_status.inning_score[x][0];
        hruns += game_status.inning_score[x][1];
    }
    /* check to see if the game is over */
    if (innings >= 9 && (game_status.half_inning % 2) == 0 && vruns > hruns)
        return;
    if (innings >= 9 && (game_status.half_inning % 2) == 1 && vruns < hruns)
        return;

    field = 1 - (game_status.half_inning % 2);

    if (!game_status.outs && game_status.baserunners[0] == 99 && game_status.baserunners[1] == 99 && game_status.baserunners[2] == 99) {
        /* it's the beginning of a half-inning */

        /* do defensive strategies */
        if (!field && !vmanager)
            boi_def_checks (field);
        if (field && !hmanager)
            boi_def_checks (field);
    }
    else {
        /* at each new batter */

        /* do defensive strategies */
        if (!field && !vmanager)
            eb_def_checks (field);
        if (field && !hmanager)
            eb_def_checks (field);
    }

    /* do offensive strategies */
    if (field && !vmanager)
        eb_off_checks (!field);
    if (!field && !hmanager)
        eb_off_checks (!field);
}

/* make beginning of inning defensive decisions */
void
boi_def_checks (int ha) {
    int innchk, curruns, x, innings, runs[2], inc[3], reppit1 = 0, reppit2 = 0, reppit3 = 0;

    /* if there were any pinch runners, pinch hitters, or injuries the previous half inning then we need to bring replacement fielders into the game */
    if ((reppit1 = replace_player (ha, 10)) == -1) {
        forfeit = -1;
        tforfeit = ha;
        return;
    }
    if ((reppit2 = replace_player (ha, 11)) == -1) {
        forfeit = -1;
        tforfeit = ha;
        return;
    }
    if (game_status.status[15]) {
        if ((reppit3 = replace_player2 (ha)) == -1) {
            forfeit = -1;
            tforfeit = ha;
            return;
        }
        game_status.status[15] = 0;
    }

    /* calculate number of innings played and the total number of runs scored for each team */
    innings = game_status.half_inning / 2 + 1;
    for (runs[0] = runs[1] = x = 0; x < innings; x++) {
        runs[0] += game_status.inning_score[x][0];
        runs[1] += game_status.inning_score[x][1];
    }

    if (ha) {
        innchk = home.pitchers[game_status.pitcher[1]].id.inn_target;
        curruns = home_cur.pitchers[game_status.pitcher[1]].pitching.runs;
    }
    else {
        innchk = visitor.pitchers[game_status.pitcher[0]].id.inn_target;
        curruns = visitor_cur.pitchers[game_status.pitcher[0]].pitching.runs;
    }
    if ((innchk <= pitching[ha].innings[0] && (pitching[ha].pitcher[1] == 99 && curruns > 3)) ||
               reppit1 || reppit2 || reppit3 || (innchk < pitching[ha].innings[0] && pitching[ha].pitcher[1] != 99) ||
              (innings > 8 && (runs[ha] == (runs[!ha] + 1) || runs[ha] == (runs[!ha] + 2) || runs[ha] == (runs[!ha] + 3)) && runs[!ha])) {
        /* if the pitcher has been in there longer than an average stint for him and the other team has scored more
           than thrice (if starting pitcher) or he's been pinched hit for or he's been injured then replace him
           also, bring in closer if it's the last inning in a save situation & not a shutout */

        /* first make sure there is a pitcher available to come in */
        if (available_pitcher (ha) == -1) {
            if (reppit1 || reppit2 || reppit3) {
                forfeit = -1;
                tforfeit = ha;
                return;
            }
            else
                goto chk_rep_def;
        }

        /* determine which pitcher to bring in */
        if (innings > 8)
            if (runs[ha] > (runs[!ha] + 3) || runs[ha] <= runs[!ha]) {
                /* it's the last inning and we're ahead by more than 3 runs, or we're not ahead at all; bring in a reliever but not the normal closer */
                inc[0] = 1;     /* most to least sequence */
                inc[1] = 1;     /* most to least sequence */
                inc[2] = 1;     /* throw out the pitcher with the highest number of saves */
                for (x = 0; x < maxpitchers[ha]; x++) {
                    work[x].pitcher = x;
                    if (!ha) {
                        work[x].stat[0] = visitor.pitchers[x].pitching.gf;
                        work[x].stat[1] = visitor.pitchers[x].pitching.games - visitor.pitchers[x].pitching.games_started;
                    }
                    else {
                        work[x].stat[0] = home.pitchers[x].pitching.gf;
                        work[x].stat[1] = home.pitchers[x].pitching.games - home.pitchers[x].pitching.games_started;
                    }
                }
            }
            else {
                if (runs[ha] > runs[!ha]) {
                    /* it's the last inning and we're ahead by less than 4 runs; bring in the closer */
                    inc[0] = 1;     /* most to least sequence */
                    inc[1] = 1;     /* most to least sequence */
                    inc[2] = 0;     /* don't worry about throwing out any pitchers */
                    for (x = 0; x < maxpitchers[ha]; x++) {
                        work[x].pitcher = x;
                        if (!ha) {
                            work[x].stat[0] = visitor.pitchers[x].pitching.saves;
                            work[x].stat[1] = visitor.pitchers[x].pitching.gf;
                        }
                        else {
                            work[x].stat[0] = home.pitchers[x].pitching.saves;
                            work[x].stat[1] = home.pitchers[x].pitching.gf;
                        }
                    }
                }
            }
        else {
            inc[0] = 1;     /* most to least sequence */
            inc[1] = 0;     /* least to most sequence */
            inc[2] = 1;     /* throw out the pitcher with the highest number of saves */
            for (x = 0; x < maxpitchers[ha]; x++) {
                work[x].pitcher = x;
                if (!ha) {
                    work[x].stat[0] = visitor.pitchers[x].pitching.games - visitor.pitchers[x].pitching.games_started;
                    work[x].stat[1] = visitor.pitchers[x].pitching.gf;
                }
                else {
                    work[x].stat[0] = home.pitchers[x].pitching.games - home.pitchers[x].pitching.games_started;
                    work[x].stat[1] = home.pitchers[x].pitching.gf;
                }
            }
        }
        replace_pitcher (ha, inc[0], inc[1], inc[2], runs[0], runs[1], reppit1, reppit2, reppit3);
    }
chk_rep_def:
    /* check to replace players for defense */
    if (innings > 6)
        if (runs[ha] > (runs[!ha] + 2))
            replace_def (ha);
}

/* make defensive decisions at each new batter */
void
eb_def_checks (int ha) {
    int r, x, y, z, zz, zind, innings, runs[2], inc[3], runnersonbase;

    innings = game_status.half_inning / 2 + 1;
    for (runs[0] = runs[1] = x = 0; x < innings; x++) {
        runs[0] += game_status.inning_score[x][0];
        runs[1] += game_status.inning_score[x][1];
    }

    /* decide if to change pitcher */
    if (pitching[ha].pitcher[1] == 99)
        r = 5;
    else
        r = 3;

    x = 0;
    if (innings < 7) {
        if (runs[ha] <= runs[!ha]) {
            if (ha) {
                if (home_cur.pitchers[pitching[ha].pitcher[0]].pitching.runs >= r)
                    x = 1;
            }
            else
                if (visitor_cur.pitchers[pitching[ha].pitcher[0]].pitching.runs >= r)
                    x = 1;
        }
    }
    else
        if ((runs[0] - runs[1]) >= -3 && (runs[0] - runs[1]) <= 3) {
            if (ha) {
                if (home_cur.pitchers[pitching[ha].pitcher[0]].pitching.runs >= 2)
                    x = 1;
            }
            else
                if (visitor_cur.pitchers[pitching[ha].pitcher[0]].pitching.runs >= 2)
                    x = 1;
        }
    /* check if pitcher was injured on previous play */
    if (game_status.status[15]) {
        int pinj;

        pinj = 0;
        for (r = 0; r < 25; r++)
            if (ha) {
                if (!strcmp (&home_cur.pitchers[pitching[ha].pitcher[0]].id.name[0], &home_cur.batters[r].id.name[0]))
                    break;
            }
            else
                if (!strcmp (&visitor_cur.pitchers[pitching[ha].pitcher[0]].id.name[0], &visitor_cur.batters[r].id.name[0]))
                    break;
        if (r < 25) {
            if (ha) {
                if (home_cur.batters[r].id.injury) {
                    x = 1;
                    pinj = 1;
                }
            }
            else
                if (visitor_cur.batters[r].id.injury) {
                    x = 1;
                    pinj = 1;
                }
            if (pinj)
                game_status.status[15]--;
        }
    }

    if (x) {
        /* determine which pitcher to bring in */

        /* first make sure there is a pitcher available to come in */
        if (available_pitcher (ha) == -1)
            goto chk_inj_def;

        if (innings > 8)
            if (runs[ha] > (runs[!ha] + 3) || runs[ha] <= runs[!ha]) {
                /* it's the last inning and we're ahead by more than 3 runs, or we're not ahead at all; bring in a reliever but not the normal closer */
                inc[0] = 1;     /* most to least sequence */
                inc[1] = 1;     /* most to least sequence */
                inc[2] = 1;     /* throw out the pitcher with most number of saves */
                for (x = 0; x < maxpitchers[ha]; x++) {
                    work[x].pitcher = x;
                    if (!ha) {
                        work[x].stat[0] = visitor.pitchers[x].pitching.gf;
                        work[x].stat[1] = visitor.pitchers[x].pitching.games - visitor.pitchers[x].pitching.games_started;
                    }
                    else {
                        work[x].stat[0] = home.pitchers[x].pitching.gf;
                        work[x].stat[1] = home.pitchers[x].pitching.games - home.pitchers[x].pitching.games_started;
                    }
                }
            }
            else {
                if (runs[ha] > runs[!ha]) {
                    /* it's the last inning and we're ahead by less than 4 runs; bring in the closer */
                    inc[0] = 1;     /* most to least sequence */
                    inc[1] = 1;     /* most to least sequence */
                    inc[2] = 0;
                    for (x = 0; x < maxpitchers[ha]; x++) {
                        work[x].pitcher = x;
                        if (!ha) {
                            work[x].stat[0] = visitor.pitchers[x].pitching.saves;
                            work[x].stat[1] = visitor.pitchers[x].pitching.gf;
                        }
                        else {
                            work[x].stat[0] = home.pitchers[x].pitching.saves;
                            work[x].stat[1] = home.pitchers[x].pitching.gf;
                        }
                    }
                }
            }
        else {
            inc[0] = 1;     /* most to least sequence */
            inc[1] = 0;     /* least to most sequence */
            inc[2] = 1;     /* throw out the pitcher with the highest number of saves */
            for (x = 0; x < maxpitchers[ha]; x++) {
                work[x].pitcher = x;
                if (!ha) {
                    work[x].stat[0] = visitor.pitchers[x].pitching.games - visitor.pitchers[x].pitching.games_started;
                    work[x].stat[1] = visitor.pitchers[x].pitching.gf;
                }
                else {
                    work[x].stat[0] = home.pitchers[x].pitching.games - home.pitchers[x].pitching.games_started;
                    work[x].stat[1] = home.pitchers[x].pitching.gf;
                }
            }
        }
        replace_pitcher (ha, inc[0], inc[1], inc[2], runs[0], runs[1], 0, 0, 0);
    }
chk_inj_def:
    /* check for injuries to any defensive players */
    if (game_status.status[15])
        for (x = 0; x < 9; x++) {
            if (ha) {
                if (!home_cur.batters[border[ha][x].player[0]].id.injury)
                    continue;
            }
            else
                if (!visitor_cur.batters[border[ha][x].player[0]].id.injury)
                    continue;

            /* don't replace the DH or the P here */
            if (border[ha][x].pos[0] > 1 && border[ha][x].pos[0] < 10) {
                if (!nosend) {
                    int tinj;
                    char action[4096];

                    action[0] = '\0';

                    strcpy (&action[0], "BD");
                    if (ha) {
                        tinj = home_cur.batters[border[x][y].player[1]].id.injury;
                        switch_name (&action[0], &home_cur.batters[border[x][y].player[1]].id.name[0]);
                    }
                    else {
                        tinj = visitor_cur.batters[border[x][y].player[1]].id.injury;
                        switch_name (&action[0], &visitor_cur.batters[border[x][y].player[1]].id.name[0]);
                    }
                    strcat (&action[0], " is injured for ");
                    if (tinj < 10)
                        strcat (&action[0], (char *) cnvt_int2str (1, tinj));
                    else
                        if (tinj < 100)
                            strcat (&action[0], (char *) cnvt_int2str (2, tinj));
                        else
                            strcat (&action[0], (char *) cnvt_int2str (3, tinj));
                    if (tinj == 1)
                        strcat (&action[0], " game\n");
                    else
                        strcat (&action[0], " games\n");
                    sock_puts (sock, &action[0]);
                    if (netgame)
                        sock_puts (netsock, &action[0]);
                }

                /* find a replacement */
                for (y = 0; y < maxplayers[ha]; y++) {
                    if (ha) {
                        if (!home.batters[y].fielding[border[ha][x].pos[0]].games)
                            continue;
                        /* check if player is currently injured */
                        if (home_season.batters[y].id.injury || home_cur.batters[y].id.injury)
                            continue;
                    }
                    else {
                        if (!visitor.batters[y].fielding[border[ha][x].pos[0]].games)
                            continue;
                        /* check if player is currently injured */
                        if (visitor_season.batters[y].id.injury || visitor_cur.batters[y].id.injury)
                            continue;
                    }
                    /* check to see if player has already been used */
                    for (zind = z = 0; z < 9; z++)
                        for (zz = 0; zz < 30; zz++)
                            if (border[ha][z].player[zz] == y)
                                zind = 1;
                    if (zind)
                        continue;

                    /* bring in new player */
                    for (z = 29; z > 0; z--) {
                        border[ha][x].player[z] = border[ha][x].player[z - 1];
                        border[ha][x].pos[z] = border[ha][x].pos[z - 1];
                    }
                    border[ha][x].player[0] = y;
                    lineupchg[ha][x] = 1;
                    if (border[ha][x].pos[0] > 9)
                        for (z = 1; z < 30; z++)
                            if (border[ha][x].pos[z] < 10) {
                                border[ha][x].pos[0] = border[ha][x].pos[z];
                                break;
                            }

                    if (!ha) {
                        visitor_cur.batters[border[0][x].player[0]].fielding[border[0][x].pos[0]].games = 1;
                        visitor_cur.batters[border[0][x].player[0]].hitting.games = 1;
                    }
                    else {
                        home_cur.batters[border[1][x].player[0]].fielding[border[1][x].pos[0]].games = 1;
                        home_cur.batters[border[1][x].player[0]].hitting.games = 1;
                    }
                    game_status.status[15]--;

                    break;
                }

                if (y == maxplayers[ha])
                    /* there's no uninjured player on the bench who has played this position
                       pick the first player available */
                    for (y = 0; y < maxplayers[ha]; y++) {
                        if (ha) {
                            /* check if player is currently injured */
                            if (home_season.batters[y].id.injury || home_cur.batters[y].id.injury)
                                continue;
                        }
                        else {
                            /* check if player is currently injured */
                            if (visitor_season.batters[y].id.injury || visitor_cur.batters[y].id.injury)
                                continue;
                        }
                        /* check to see if player has already been used */
                        for (zind = z = 0; z < 9; z++)
                            for (zz = 0; zz < 30; zz++)
                                if (border[ha][z].player[zz] == y)
                                    zind = 1;
                        if (zind)
                            continue;

                        /* bring in new player */
                        for (z = 29; z > 0; z--) {
                            border[ha][x].player[z] = border[ha][x].player[z - 1];
                            border[ha][x].pos[z] = border[ha][x].pos[z - 1];
                        }
                        border[ha][x].player[0] = y;
                        lineupchg[ha][x] = 1;
                        if (border[ha][x].pos[0] > 9)
                            for (z = 1; z < 30; z++)
                                if (border[ha][x].pos[z] < 10) {
                                    border[ha][x].pos[0] = border[ha][x].pos[z];
                                    break;
                                }

                        if (!ha) {
                            visitor_cur.batters[border[0][x].player[0]].fielding[border[0][x].pos[0]].games = 1;
                            visitor_cur.batters[border[0][x].player[0]].hitting.games = 1;
                        }
                        else {
                            home_cur.batters[border[1][x].player[0]].fielding[border[1][x].pos[0]].games = 1;
                            home_cur.batters[border[1][x].player[0]].hitting.games = 1;
                        }
                        game_status.status[15]--;

                        break;
                    }

                if (y == maxplayers[ha]) {
                    /* there's no one on the bench to come in */
                    forfeit = -1;
                    tforfeit = ha;
                    return;
                }
            }
        }

    /* decide if to hold runner on first */
    if (game_status.baserunners[0] != 99 && game_status.baserunners[1] == 99) {
        if (runs[ha] == runs[!ha] && innings > 8 && !ha && game_status.baserunners[2] != 99)
            /* if it's the 9th inning or later, the score is tied, the home team is atbat, and there's a runner on third never hold the runner on first */
            goto ex_checkhold;
        else
            if (runs[ha] < (runs[!ha] + 5)) {
                int cs;

                game_status.status[0] = 1;
                /* decide to attempt a pickoff */
                if (ha) {
                    if (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                        cs = 0;
                    else
                        cs = visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs;

                    if (do_divide (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb,
                                   visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb + cs) > .50) {
                        /* rarely ever more than a 50% chance of a pickoff attempt */
                        x = (int) ((float) (do_divide (200, (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb + 1))) *
                                            rand () / (RAND_MAX + 1.0));
                        if (!x) 
                            game_status.status[1] = 1;
                        /* decide to throw a pitchout */
                        /* rarely ever more than a 25% chance for a pitchout */
                        x = (int) ((float) (do_divide (400, (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb + 1))) *
                                            rand () / (RAND_MAX + 1.0));
                        if (!x) 
                            game_status.status[5] = 1;
                    }
                }
                else {
                    if (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                        cs = 0;
                    else
                        cs = home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs;

                    if (do_divide (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb,
                                   home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb + cs) > .50) {
                        /* never more than a 25% chance of a pickoff attempt */
                        x = (int) ((float) (do_divide (400, (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb + 1))) *
                                            rand () / (RAND_MAX + 1.0));
                        if (!x) 
                            game_status.status[1] = 1;
                        /* decide to throw a pitchout */
                        /* never more than a 10% chance for a pitchout */
                        x = (int) ((float) (do_divide (1000, (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb + 1))) *
                                            rand () / (RAND_MAX + 1.0));
                        if (!x) 
                            game_status.status[5] = 1;
                    }
                }
            }
    }
ex_checkhold:
    /* decide if to play infield in */
    if (innings > 7 && game_status.outs < 2 && ((runs[0] - runs[1]) >= -1 && (runs[0] - runs[1]) <= 1) && game_status.baserunners[2] != 99) {
        if (game_status.baserunners[0] != 99)
            if (game_status.outs == 1)
                game_status.status[2] = 1;
            else
                game_status.status[2] = 2;
        else
            game_status.status[2] = 2;
    }

    if (innings > 6 && game_status.outs < 2 && ((runs[0] - runs[1]) >= -1 && (runs[0] - runs[1]) <= 1)) {
        if (game_status.baserunners[0] != 99 && game_status.baserunners[1] == 99 && game_status.baserunners[2] == 99)
            if (ha) {
                if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers < 10 &&
                          do_divide (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hits, 
                          visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats) < .250) 
                    game_status.status[2] = 1;
            }
            else {
                if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers < 10 &&
                          do_divide (home.batters[border[1][game_status.batter[1]].player[0]].hitting.hits, 
                          home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats) < .250) 
                    game_status.status[2] = 1;
            }
        else
            if (game_status.baserunners[0] != 99 || game_status.baserunners[1] != 99 || game_status.baserunners[2] != 99) {
                if (!game_status.outs && game_status.baserunners[0] == 99 && game_status.baserunners[2] == 99) {
                    if (ha) {
                        if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers < 10 &&
                                  do_divide (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hits, 
                                  visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats) < .250) 
                            game_status.status[2] = 1;
                    }
                    else {
                        if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers < 10 &&
                                do_divide (home.batters[border[1][game_status.batter[1]].player[0]].hitting.hits, 
                                home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats) < .250) 
                            game_status.status[2] = 1;
                    }
                }
                if (game_status.baserunners[2] != 99)
                    game_status.status[2] = 1;
            }
    }

    /* decide if to guard line */
    /* we don't want to both guard the line and hold a runner on first */
    if (!game_status.status[0]) {
        if (runs[ha] == runs[!ha] && innings > 8 && !ha && game_status.baserunners[2] != 99)
            /* if it's the 9th inning or later, the score is tied, the home team is atbat, and there's a runner on third never guard either line */
            goto ex_checkguard;
        else
            if (game_status.baserunners[0] == 99 && game_status.baserunners[1] == 99 && game_status.baserunners[2] == 99)
                if (innings > 7)
                    if ((runs[0] - runs[1]) >= -1 && (runs[0] - runs[1]) <= 1) {
                        if (ha)
                            if (!visitor.batters[border[0][game_status.batter[0]].player[0]].id.batslr ||
                                                   (visitor.batters[border[0][game_status.batter[0]].player[0]].id.batslr == 2 &&
                                                    home.pitchers[pitching[1].pitcher[0]].id.throwslr))
                                game_status.status[6] = 1;
                            else
                                game_status.status[6] = 2;
                        else
                            if (!home.batters[border[1][game_status.batter[1]].player[0]].id.batslr ||
                                                   (home.batters[border[1][game_status.batter[1]].player[0]].id.batslr == 2 &&
                                                    visitor.pitchers[pitching[0].pitcher[0]].id.throwslr))
                                game_status.status[6] = 1;
                            else
                                game_status.status[6] = 2;
                    }
    }
ex_checkguard:
    /* decide if to play outfield shallow */
    if (innings >= 9 && (game_status.half_inning % 2) == 1 && runs[0] == runs[1] && game_status.outs < 2 && game_status.baserunners[2] != 99)
        game_status.status[3] = 1;

    if (!nosend) 
        send_tophalf ();

    /* determine number of runners on base */
    for (runnersonbase = 0; x < 3; x++)
        if (game_status.baserunners[x] != 99)
            runnersonbase++;

    /* decide if to intentionally walk the batter */
    if (game_status.batter[!ha] == 8)
        x = 0;
    else
        x = game_status.batter[!ha] + 1;
    if (innings > 4 && runs[ha] > (runs[!ha] + runnersonbase + 1) &&
          (game_status.baserunners[1] != 99 || game_status.baserunners[2] != 99) && game_status.baserunners[0] == 99) {
        if (ha) {
            if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers > 25 &&
                        do_divide (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hits,
                        visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats) > .300)
                if (visitor.batters[border[0][x].player[0]].hitting.homers < 20 && do_divide (visitor.batters[border[0][x].player[0]].hitting.hits,
                        visitor.batters[border[0][x].player[0]].hitting.atbats) < .280)
                    game_status.status[4] = 1;
        }
        else {
             if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers > 25 &&
                        do_divide (home.batters[border[1][game_status.batter[1]].player[0]].hitting.hits,
                        home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats) > .300)
                if (home.batters[border[1][x].player[0]].hitting.homers < 20 && do_divide (home.batters[border[1][x].player[0]].hitting.hits,
                                       home.batters[border[1][x].player[0]].hitting.atbats) < .280)
                    game_status.status[4] = 1;
        }
    }
    /* if the pitcher is the next batter, firstbase is open, a baserunner on either second or third, there are 2 outs,
       later than the 4th inning and the score is within a certain range then walk the hitter */
    if (x == 8 && !dhind && game_status.baserunners[0] == 99 &&
             (game_status.baserunners[1] != 99 || game_status.baserunners[2] != 99) && game_status.outs == 2 &&
             innings > 4 && runs[ha] > (runs[!ha] + runnersonbase + 1))
        game_status.status[4] = 1;
}

void
check_save (int ha, int rus, int rthem) {
    int acc = 2, x;

    if (rus > rthem) {
        saver = rus - rthem;
        for (x = 0; x < 3; x++)
            if (game_status.baserunners[x] != 99)
                acc++;
        if ((rus - rthem) <= acc)
            savei = 1;
        else {
            if (savei) {
                /* a blown save */
                if (!ha)
                    visitor_cur.pitchers[pitching[ha].pitcher[0]].pitching.svopp++;
                else   
                    home_cur.pitchers[pitching[ha].pitcher[0]].pitching.svopp++;
            }
            savei = 0;
        }
    }
}

void
check_dec (int ha, int rus, int rthem) {
    if (pwin == 99) {
        if (rus > rthem) {
            if (ha)
                if (home_cur.pitchers[pitching[ha].pitcher[1]].pitching.games_started)
                    if (home_cur.pitchers[pitching[ha].pitcher[1]].pitching.innings > 4)
                        pwin = pitching[ha].pitcher[1];
                    else
                        pwin = pitching[ha].pitcher[0];
                else
                    pwin = pitching[ha].pitcher[1];
            else   
                if (visitor_cur.pitchers[pitching[ha].pitcher[1]].pitching.games_started)
                    if (visitor_cur.pitchers[pitching[ha].pitcher[1]].pitching.innings > 4)
                        pwin = pitching[ha].pitcher[1];
                    else
                        pwin = pitching[ha].pitcher[0];
                else
                    pwin = pitching[ha].pitcher[1];
        }
        if (rus < rthem) {
            if (!ha) {
                if (home_cur.pitchers[pitching[1].pitcher[0]].pitching.games_started && home_cur.pitchers[pitching[1].pitcher[0]].pitching.innings > 4)
                    pwin = pitching[1].pitcher[0];
            }
            else   
                if (visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.games_started && visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.innings > 4)
                    pwin = pitching[0].pitcher[0];
        }
    }

    if (ploss == 99) {
        if (rus > rthem)
            ploss = pitching[!ha].pitcher[0];
        if (rus < rthem)
            ploss = pitching[ha].pitcher[1];
    }
}

void
update_some_stats (int ha, int reppit1, int reppit2, int reppit3) {
    int hitter, x, y, p;

    if (!ha) {
        /* find batter iteration number for pitcher */
        for (x = 0; x < 25; x++)
            if (!strcmp (&visitor_cur.pitchers[pitching[ha].pitcher[0]].id.name[0], &visitor_cur.batters[x].id.name[0])) {
                hitter = x;
                break;
            }

        visitor_cur.batters[hitter].fielding[1].games = 1;
        visitor_cur.batters[hitter].hitting.games = 1;
        visitor_cur.pitchers[pitching[ha].pitcher[0]].pitching.games = 1;
    }
    else {
        /* find batter iteration number for pitcher */
        for (x = 0; x < 25; x++)
            if (!strcmp (&home_cur.pitchers[pitching[ha].pitcher[0]].id.name[0], &home_cur.batters[x].id.name[0])) {
                hitter = x;
                break;
            }

        home_cur.batters[hitter].fielding[1].games = 1;
        home_cur.batters[hitter].hitting.games = 1;
        home_cur.pitchers[pitching[ha].pitcher[0]].pitching.games = 1;
    }

    if (reppit1 || reppit2)
        p = 1;
    else
        p = 0;

    for (x = 0; x < 9; x++)
        if (border[ha][x].pos[p] == 1) {
            for (y = 29; y > 0; y--) {
                border[ha][x].player[y] = border[ha][x].player[y - 1];
                border[ha][x].pos[y] = border[ha][x].pos[y - 1];
            }
            border[ha][x].player[0] = hitter;
            border[ha][x].pos[0] = 1;
            lineupchg[ha][x] = 1;
        }
    if (dhind)
        dhpchg[ha] = 1;
}

/* check to replace any players for defensive purposes */
void
replace_def (int field) {
    int x, y, z, zz, zind, holdy;
    float in, sub, holdsub;

    for (x = 8; x >= 0; x--)      /* start at the bottom of the batting order since that's where the weaker offensive players are normally located */
        /* don't replace the DH or the P here */

        /* sometimes we have only stats for all of the outfield positions combined */
        if (border[field][x].pos[0] > 1 && border[field][x].pos[0] < 10) {
            int pos;

            if (border[field][x].pos[0] > 6)
                if (field)
                    if (home.batters[border[field][x].player[0]].fielding[border[field][x].pos[0]].po == -1)
                        pos = 10;
                    else
                        pos = border[field][x].pos[0];
                else
                    if (visitor.batters[border[field][x].player[0]].fielding[border[field][x].pos[0]].po == -1)
                        pos = 10;
                    else
                        pos = border[field][x].pos[0];
            else
                pos = border[field][x].pos[0];

            if (field) {
                if ((in = do_divide (home.batters[border[field][x].player[0]].fielding[pos].po + home.batters[border[field][x].player[0]].fielding[pos].a,
                                     home.batters[border[field][x].player[0]].fielding[pos].e + home.batters[border[field][x].player[0]].fielding[pos].po +
                                      home.batters[border[field][x].player[0]].fielding[pos].a)) == -1)
                    continue;
            }
            else
                if ((in = do_divide (visitor.batters[border[field][x].player[0]].fielding[pos].po +
                                      visitor.batters[border[field][x].player[0]].fielding[pos].a,
                                      visitor.batters[border[field][x].player[0]].fielding[pos].e +
                                     visitor.batters[border[field][x].player[0]].fielding[pos].po +
                                      visitor.batters[border[field][x].player[0]].fielding[pos].a)) == -1)
                    continue;
            for (sub = y = holdsub = 0; y < maxplayers[field]; y++) {
                if (field) {
                    /* check for injury */
                    if (home_season.batters[y].id.injury || home_cur.batters[y].id.injury)
                        continue;
                    /* we always have the games stat, even for all the outfield positions */
                    if (home.batters[y].fielding[border[field][x].pos[0]].games < 30)
                        continue;
                    if ((sub = do_divide (home.batters[y].fielding[pos].po + home.batters[y].fielding[pos].a, home.batters[y].fielding[pos].e +
                                                  home.batters[y].fielding[pos].po + home.batters[y].fielding[pos].a)) == -1)
                        continue;
                }
                else {
                    /* check for injury */
                    if (visitor_season.batters[y].id.injury || visitor_cur.batters[y].id.injury)
                        continue;
                    /* we always have the games stat, even for all the outfield positions */
                    if (visitor.batters[y].fielding[border[field][x].pos[0]].games < 30)
                        continue;
                    if ((sub = do_divide (visitor.batters[y].fielding[pos].po + visitor.batters[y].fielding[pos].a, visitor.batters[y].fielding[pos].e +
                                               visitor.batters[y].fielding[pos].po + visitor.batters[y].fielding[pos].a)) == -1)
                        continue;
                }
                /* check to see if possible substitute is a better fielder than the currently playing player */
                if (sub <= in)
                    continue;
                /* check to see if player has already been used */
                for (zind = z = 0; z < 9; z++)
                    for (zz = 0; zz < 30; zz++)
                        if (border[field][z].player[zz] == y)
                            zind = 1;
                if (zind)
                    continue;

                if (sub > holdsub) {
                    /* hold info and check rest of roster */
                    holdy = y;
                    holdsub = sub;
                }
            }

            if (holdsub > 0) {
                /* bring in new player */
                for (z = 29; z > 0; z--) {
                    border[field][x].player[z] = border[field][x].player[z - 1];
                    border[field][x].pos[z] = border[field][x].pos[z - 1];
                }
                border[field][x].player[0] = holdy;
                lineupchg[field][x] = 1;
                if (border[field][x].pos[0] > 9)
                    for (z = 1; z < 30; z++)
                        if (border[field][x].pos[z] < 10) {
                            border[field][x].pos[0] = border[field][x].pos[z];
                            break;
                        }

                if (!field) {
                    visitor_cur.batters[border[0][x].player[0]].fielding[border[0][x].pos[0]].games = 1;
                    visitor_cur.batters[border[0][x].player[0]].hitting.games = 1;
                }
                else {
                    home_cur.batters[border[1][x].player[0]].fielding[border[1][x].pos[0]].games = 1;
                    home_cur.batters[border[1][x].player[0]].hitting.games = 1;
                }
            }
        }
}

float
do_divide (int dividend, int divisor) {
    if (!divisor)
        return -1;

    return ((float) dividend / (float) divisor);
}

void
replace_pitcher (int ha, int i1, int i2, int i3, int r1, int r2, int reppit1, int reppit2, int reppit3) {
    int ct, x, y, inc[2], runs[2], tempip = 0, tempx;

    inc[0] = i1;
    inc[1] = i2;
    runs[0] = r1;
    runs[1] = r2;

    /* sort pitchers */
    for (x = 0; x < (maxpitchers[ha] - 1); x++)
        for (y = x + 1; y < maxpitchers[ha]; y++) {
            if (inc[0]) {
                if (work[x].stat[0] < work[y].stat[0]) {
                    work[11] = work[x];
                    work[x] = work[y];
                    work[y] = work[11];
                }
            }
            else
                if (work[x].stat[0] > work[y].stat[0]) {
                    work[11] = work[x];
                    work[x] = work[y];
                    work[y] = work[11];
                }
            if (work[x].stat[0] == work[y].stat[0]) {
                if (inc[1]) {
                    if (work[x].stat[1] < work[y].stat[1]) {
                        work[11] = work[x];
                        work[x] = work[y];
                        work[y] = work[11];
                    }
                }
                else
                    if (work[x].stat[1] > work[y].stat[1]) {
                        work[11] = work[x];
                        work[x] = work[y];
                        work[y] = work[11];
                    }
            }
        }
    /* actually bring in new pitcher */
    for (x = 0; x < maxpitchers[ha]; x++) {
        /* make sure we haven't used him yet this game */
        for (y = 0; y < 15; y++)
            if (pitching[ha].pitcher[y] == work[x].pitcher)
                break;
        if (y != 15) {
            work[x].pitcher = 99;
            continue;
        }
        /* make sure he's not injured */
        if (!ha) {
            for (y = 0; y < 25; y++)
                if (!strcmp (&visitor_cur.pitchers[work[x].pitcher].id.name[0], &visitor_cur.batters[y].id.name[0]))
                    break;
            if (visitor_season.batters[y].id.injury || visitor_cur.batters[y].id.injury) {
                work[x].pitcher = 99;
                continue;
            }
        }
        else {
            for (y = 0; y < 25; y++)
                if (!strcmp (&home_cur.pitchers[work[x].pitcher].id.name[0], &home_cur.batters[y].id.name[0]))
                    break;
            if (home_season.batters[y].id.injury || home_cur.batters[y].id.injury) {
                work[x].pitcher = 99;
                continue;
            }
        }

        /* it's possible that the pitcher has already played (or is playing) some other position */
        for (y = 0; y < maxplayers[ha]; y++)
            if (ha) {
                if (!strcmp (&home_cur.pitchers[work[x].pitcher].id.name[0], &home_cur.batters[y].id.name[0]))
                    if (home_cur.batters[y].hitting.games)
                        break;
            }
            else
                if (!strcmp (&visitor_cur.pitchers[work[x].pitcher].id.name[0], &visitor_cur.batters[y].id.name[0]))
                    if (visitor_cur.batters[y].hitting.games)
                        break;
        if (y < maxplayers[ha])
            continue;

        /* we don't want to use a pitcher who only starts games */
        if (!ha) {
            if (!(visitor.pitchers[work[x].pitcher].pitching.games - visitor.pitchers[work[x].pitcher].pitching.games_started))
                continue;
        }
        else
            if (!(home.pitchers[work[x].pitcher].pitching.games - home.pitchers[work[x].pitcher].pitching.games_started))
                continue;
        /* don't select an overworked pitcher */
        if (!ha) {
            if ((visitor_season.pitchers[work[x].pitcher].id.ip_last4g[0] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                  visitor_season.pitchers[work[x].pitcher].id.ip_last4g[2] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[3]) >=
                                               (visitor_season.pitchers[work[x].pitcher].id.inn_target * 2))
                continue;
            for (ct = y = 0; y < 4; y++)
                if (visitor_season.pitchers[work[x].pitcher].id.ip_last4g[y])
                    ct++;
            if (ct >= 3)
                continue;
        }
        else {
            if ((home_season.pitchers[work[x].pitcher].id.ip_last4g[0] + home_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                  home_season.pitchers[work[x].pitcher].id.ip_last4g[2] + home_season.pitchers[work[x].pitcher].id.ip_last4g[3]) >=
                                                      (home_season.pitchers[work[x].pitcher].id.inn_target * 2))
                continue;
            for (ct = y = 0; y < 4; y++)
                if (home_season.pitchers[work[x].pitcher].id.ip_last4g[y])
                    ct++;
            if (ct >= 3)
                continue;
        }
        /* don't use a pitcher with the highest number of saves on the team (on a user-created team there could be several pitchers with a high number
           of saves) in non-save situations */
        if (i3) {
            int hsaves, id;

            if (!ha) {
                for (hsaves = y = 0; y < maxpitchers[ha]; y++)
                    if (visitor.pitchers[work[y].pitcher].pitching.saves > hsaves) {
                        hsaves = visitor.pitchers[work[y].pitcher].pitching.saves;
                        id = y;
                    }
                if (visitor.pitchers[work[x].pitcher].pitching.saves == hsaves && id == x)
                    continue;

            }
            else {
                for (hsaves = y = 0; y < maxpitchers[ha]; y++)
                    if (home.pitchers[work[y].pitcher].pitching.saves > hsaves) {
                        hsaves = home.pitchers[work[y].pitcher].pitching.saves;
                        id = y;
                    }
                if (home.pitchers[work[x].pitcher].pitching.saves == hsaves && id == x)
                    continue;

            }
        }
        break;
    }
    if (x == maxpitchers[ha]) {
        /* if no pitcher selected then go back through list and pick the least overworked pitcher of non-starters */
        for (tempip = 99, x = 0; x < maxpitchers[ha]; x++) {
            if (work[x].pitcher == 99)
                continue;
            else
                if (!ha) {
                    if (visitor.pitchers[work[x].pitcher].pitching.games_started > (visitor.pitchers[work[x].pitcher].pitching.games / 5))
                        continue;
                    if ((visitor_season.pitchers[work[x].pitcher].id.ip_last4g[0] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                           visitor_season.pitchers[work[x].pitcher].id.ip_last4g[2] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[3]) < tempip)
                        if (!visitor_season.pitchers[work[x].pitcher].id.injury && !visitor_cur.pitchers[work[x].pitcher].id.injury) {
                            tempip = visitor_season.pitchers[work[x].pitcher].id.ip_last4g[0] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                                     visitor_season.pitchers[work[x].pitcher].id.ip_last4g[2] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[3];
                            tempx = x;
                        }
                }
                else {
                    if (home.pitchers[work[x].pitcher].pitching.games_started > (home.pitchers[work[x].pitcher].pitching.games / 5))
                        continue;
                    if ((home_season.pitchers[work[x].pitcher].id.ip_last4g[0] + home_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                         home_season.pitchers[work[x].pitcher].id.ip_last4g[2] + home_season.pitchers[work[x].pitcher].id.ip_last4g[3]) < tempip)
                        if (!home_season.pitchers[work[x].pitcher].id.injury || !home_cur.pitchers[work[x].pitcher].id.injury) {
                            tempip = home_season.pitchers[work[x].pitcher].id.ip_last4g[0] + home_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                                     home_season.pitchers[work[x].pitcher].id.ip_last4g[2] + home_season.pitchers[work[x].pitcher].id.ip_last4g[3];
                            tempx = x;
                        }
                }
        }
        x = tempx;
    }

    if (x == maxpitchers[ha]) {
        /* if no pitcher selected then go back through list and pick the least overworked pitcher with no regard as to whether or not they were starters */
        for (tempip = 99, x = 0; x < maxpitchers[ha]; x++) {
            if (work[x].pitcher == 99)
                continue;
            else
                if (!ha) {
                    if ((visitor_season.pitchers[work[x].pitcher].id.ip_last4g[0] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                           visitor_season.pitchers[work[x].pitcher].id.ip_last4g[2] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[3]) < tempip)
                        if (!visitor_season.pitchers[work[x].pitcher].id.injury && !visitor_cur.pitchers[work[x].pitcher].id.injury) {
                            tempip = visitor_season.pitchers[work[x].pitcher].id.ip_last4g[0] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                                     visitor_season.pitchers[work[x].pitcher].id.ip_last4g[2] + visitor_season.pitchers[work[x].pitcher].id.ip_last4g[3];
                            tempx = x;
                        }
                }
                else
                    if ((home_season.pitchers[work[x].pitcher].id.ip_last4g[0] + home_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                         home_season.pitchers[work[x].pitcher].id.ip_last4g[2] + home_season.pitchers[work[x].pitcher].id.ip_last4g[3]) < tempip)
                        if (!home_season.pitchers[work[x].pitcher].id.injury || !home_cur.pitchers[work[x].pitcher].id.injury) {
                            tempip = home_season.pitchers[work[x].pitcher].id.ip_last4g[0] + home_season.pitchers[work[x].pitcher].id.ip_last4g[1] +
                                     home_season.pitchers[work[x].pitcher].id.ip_last4g[2] + home_season.pitchers[work[x].pitcher].id.ip_last4g[3];
                            tempx = x;
                        }
        }
        x = tempx;
    }

    if (tempip == 99)
        /* if no pitcher still selected then there ain't none */
        return;

    /* make room for the new pitcher */
    for (y = 14; y > 0; y--) {
        pitching[ha].pitcher[y] = pitching[ha].pitcher[y - 1];
        pitching[ha].innings[y] = pitching[ha].innings[y - 1];
        pitching[ha].thirds[y] = pitching[ha].thirds[y - 1];
    }

    game_status.pitcher[ha] = pitching[ha].pitcher[0] = work[x].pitcher;
    pitching[ha].innings[0] = pitching[ha].thirds[0] = 0;

    check_dec (ha, runs[ha], runs[!ha]);
    check_save (ha, runs[ha], runs[!ha]);
    update_some_stats (ha, reppit1, reppit2, reppit3);
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

/* make offensive decisions at each new batter */
void
eb_off_checks (int ha) {
    int x, y, br, sub, z, zz, zind, innings, runs[2], yesph, innchk, curruns;
    float avg, avg2;
    struct {
        int plyr;
        float stat;
    } work[26];

    innings = game_status.half_inning / 2 + 1;
    for (runs[0] = runs[1] = x = 0; x < innings; x++) {
        runs[0] += game_status.inning_score[x][0];
        runs[1] += game_status.inning_score[x][1];
    }

    /* decide if to bring in a pinch runner */
    if (innings > 7)
        if ((runs[ha] + 2) == runs[!ha] || (runs[ha] + 1) == runs[!ha])
            if (game_status.baserunners[1] != 99 || game_status.baserunners[0] != 99) {
                int cs;

                if (game_status.baserunners[1] != 99)
                    br = 1;
                else
                    br = 0;
                x = 0;
                if (!ha) {
                    if (visitor.batters[border[0][game_status.baserunners[br]].player[0]].hitting.cs == -1)
                        cs = 0;
                    else
                        cs = visitor.batters[border[0][game_status.baserunners[br]].player[0]].hitting.cs;

                    if (do_divide (visitor.batters[border[0][game_status.baserunners[br]].player[0]].hitting.sb, 
                             visitor.batters[border[0][game_status.baserunners[br]].player[0]].hitting.sb + cs) < .5 &&
                             visitor.batters[border[0][game_status.baserunners[br]].player[0]].hitting.sb < 5)
                        x = 1;
                }
                else {
                    if (home.batters[border[1][game_status.baserunners[br]].player[0]].hitting.cs == -1)
                        cs = 0;
                    else
                        cs = home.batters[border[1][game_status.baserunners[br]].player[0]].hitting.cs;

                    if (do_divide (home.batters[border[1][game_status.baserunners[br]].player[0]].hitting.sb, 
                             home.batters[border[1][game_status.baserunners[br]].player[0]].hitting.sb + cs) < .5 &&
                             home.batters[border[1][game_status.baserunners[br]].player[0]].hitting.sb < 5)
                        x = 1;
                }
                if (x)
                    /* find a faster runner */
                    for (sub = y = 0; y < maxplayers[ha]; y++) {
                        int cs;

                        if (ha) {
                            if (home.batters[y].hitting.sb < 30)
                                continue;
                            if (home_season.batters[y].id.injury || home_cur.batters[y].id.injury)
                                continue;
                            if (home.batters[y].hitting.cs == -1)
                                cs = 0;
                            else
                                cs = home.batters[y].hitting.cs;

                            sub = do_divide (home.batters[y].hitting.sb, home.batters[y].hitting.sb + cs);
                            if (sub < .7 && home.batters[y].hitting.sb < 5)
                                continue;
                        }
                        else {
                            if (visitor.batters[y].hitting.sb < 30)
                                continue;
                            if (visitor_season.batters[y].id.injury || visitor_cur.batters[y].id.injury)
                                continue;
                            if (visitor.batters[y].hitting.cs == -1)
                                cs = 0;
                            else
                                cs = visitor.batters[y].hitting.cs;

                            sub = do_divide (visitor.batters[y].hitting.sb, visitor.batters[y].hitting.sb + cs);
                            if (sub < .7 && visitor.batters[y].hitting.sb < 5)
                                continue;
                        }
                        /* check to see if player has already been used */
                        for (zind = z = 0; z < 9; z++)
                            for (zz = 0; zz < 30; zz++)
                                if (border[ha][z].player[zz] == y)
                                    zind = 1;
                        if (zind)
                            continue;

                        /* bring in new player */
                        for (z = 29; z > 0; z--) {
                            border[ha][game_status.baserunners[br]].player[z] = border[ha][game_status.baserunners[br]].player[z - 1];
                            border[ha][game_status.baserunners[br]].pos[z] = border[ha][game_status.baserunners[br]].pos[z - 1];
                        }
                        game_status.status[12] = border[ha][game_status.baserunners[br]].player[0] = y;
                        border[ha][game_status.baserunners[br]].pos[0] = 11;
                        lineupchg[ha][game_status.baserunners[br]] = 1;

                        if (!ha)
                            visitor_cur.batters[y].hitting.games = 1;
                        else
                            home_cur.batters[y].hitting.games = 1;

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
                        break;
                    }
            }
    /* decide if to bring in a pinch hitter */
    yesph = 0;

    /* if the pitcher is up to bat check to see if he's going to be relieved at the beginning of the next half-inning,
       if so and there's a pitcher available to come in then pinch hit for him */
    if (border[ha][game_status.batter[ha]].pos[0] == 1) {
        if (ha) {
            innchk = home.pitchers[game_status.pitcher[1]].id.inn_target;
            curruns = home_cur.pitchers[game_status.pitcher[1]].pitching.runs;
        }
        else {
            innchk = visitor.pitchers[game_status.pitcher[0]].id.inn_target;
            curruns = visitor_cur.pitchers[game_status.pitcher[0]].pitching.runs;
        }
        if ((innchk <= pitching[ha].innings[0] && (pitching[ha].pitcher[1] == 99 && curruns > 3)) ||
                  (innchk < pitching[ha].innings[0] && pitching[ha].pitcher[1] != 99) ||
                  (innings > 8 && (runs[ha] == (runs[!ha] + 1) || runs[ha] == (runs[!ha] + 2) || runs[ha] == (runs[!ha] + 3)) && runs[!ha]))
            /* make sure there is a pitcher available to come in */
            if (!available_pitcher (ha))
                yesph = 1;
    }

    if ((innings > 7 && runs[ha] < runs[!ha]) || yesph || (innings == 5 && (runs[ha] + 4) <= runs[!ha] && border[ha][game_status.batter[ha]].pos[0] == 1) ||
                                                          (innings == 6 && (runs[ha] + 3) <= runs[!ha] && border[ha][game_status.batter[ha]].pos[0] == 1) ||
                                                          (innings == 7 && (runs[ha] + 2) <= runs[!ha] && border[ha][game_status.batter[ha]].pos[0] == 1)) {
        /* first make sure there is a pitcher available to come in */
        if (border[ha][game_status.batter[ha]].pos[0] == 1)
            if (available_pitcher (ha) == -1)
                goto dec_steal;

        /* if it's not the pitcher and it's not the last inning of the game make sure there is a player available who can play
           that position to come in */
        if (border[ha][game_status.batter[ha]].pos[0] != 1 && innings < 9)
            if (available_player (border[ha][game_status.batter[ha]].pos[0], ha) == -1)
                goto dec_steal;

        x = 0;
        if (border[ha][game_status.batter[ha]].pos[0] == 1) {
            /* if the pitcher is at bat then don't do the batting average check */
            x = 1;
            avg = 0;
        }
        else
            if (!ha) {
                if (((avg = do_divide (visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                         visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats)) < .250) &&
                         visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.homers < 15)
                    x = 1;
            }
            else
                if (((avg = do_divide (home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                         home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats)) < .250) &&
                         home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.homers < 15)
                    x = 1;
        if (x) {
            /* find a better hitter */
            for (y = 0; y < maxplayers[ha]; y++) {
                work[y].plyr = y;
                if (!ha)
                    work[y].stat = do_divide (visitor.batters[y].hitting.hits, visitor.batters[y].hitting.atbats);
                else
                    work[y].stat = do_divide (home.batters[y].hitting.hits, home.batters[y].hitting.atbats);
            }

            for (x = 0; x < (maxplayers[ha] - 1); x++)
                for (z = x + 1; z < maxplayers[ha]; z++)
                    if (work[x].stat < work[z].stat) {
                        work[25] = work[x];
                        work[x] = work[z];
                        work[z] = work[25];
                    }

            for (zind = y = 0; y < maxplayers[ha]; y++, zind = 0) {
                /* check to see if player has already been used */
                for (z = 0; z < 9; z++)
                    for (zz = 0; zz < 30; zz++)
                        if (border[ha][z].player[zz] == work[y].plyr)
                            zind = 1;

                if (zind)
                    continue;
                /* check for injury */
                if (!ha) {
                    if (visitor_season.batters[work[y].plyr].id.injury || visitor_cur.batters[work[y].plyr].id.injury) 
                        continue;
                }
                else
                    if (home_season.batters[work[y].plyr].id.injury || home_cur.batters[work[y].plyr].id.injury) 
                        continue;
                /* we don't want to pinch hit with a pitcher */
                for (z = 0; z < maxpitchers[ha]; z++)
                    if (ha) {
                        if (!strcmp (&home_season.batters[work[y].plyr].id.name[0], &home_season.pitchers[z].id.name[0]))
                            break;
                    }
                    else
                        if (!strcmp (&visitor_season.batters[work[y].plyr].id.name[0], &visitor_season.pitchers[z].id.name[0]))
                            break;
                if (z < maxpitchers[ha])
                    continue;

                if (avg >= work[y].stat)
                    continue;

                /* bring in new player */
                for (z = 29; z > 0; z--) {
                    border[ha][game_status.batter[ha]].player[z] = border[ha][game_status.batter[ha]].player[z - 1];
                    border[ha][game_status.batter[ha]].pos[z] = border[ha][game_status.batter[ha]].pos[z - 1];
                }

                game_status.status[11] = border[ha][game_status.batter[ha]].player[0] = work[y].plyr;
                border[ha][game_status.batter[ha]].pos[0] = 10;
                lineupchg[ha][game_status.batter[ha]] = 1;

                if (!ha)
                    visitor_cur.batters[work[y].plyr].hitting.games = 1;
                else
                    home_cur.batters[work[y].plyr].hitting.games = 1;

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
                break;
            }
        }
    }
dec_steal:
    /* decide if to attempt a steal */
    if (game_status.baserunners[0] != 99 && game_status.baserunners[1] == 99)
        if (((runs[ha] + 3) >= runs[!ha] && innings < 7) || ((runs[ha] + 1) >= runs[!ha] && innings > 6)) {
            int cs;

            if (ha) {
                if (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                    cs = 0;
                else
                    cs = home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs;

                x = home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb - cs;
            }
            else {
                if (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                    cs = 0;
                else
                    cs = visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs;

                x = visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb - cs;
            }

            if (x < 0)
                x = 0;
            if (x > 75) {
                if (((int) ((float) 3 * rand () / (RAND_MAX + 1.0))) < 2)
                    game_status.status[7] = 2;
            }
            else
                if (x > 40) {
                    if (((int) ((float) 10 * rand () / (RAND_MAX + 1.0))) < 4)
                        game_status.status[7] = 2;
                }
                else {
                    if (!x)
                        goto dec_hitrun;
                    x = (101 - x) / 2;
                    if (((int) ((float) x * rand () / (RAND_MAX + 1.0))) < 3)
                        game_status.status[7] = 2;
                }
        }
dec_hitrun:
    /* decide if to attempt a hit and run */
    if (innings > 5 && !game_status.status[7])
        if (game_status.outs < 2)
            if (game_status.baserunners[0] != 99 && game_status.baserunners[2] == 99)
                if ((runs[0] - runs[1]) >= -2 && (runs[0] - runs[1]) <= 2) {
                    if (ha) {
                        avg = do_divide (home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                         home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats);
                        avg2 = do_divide (home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.so, 
                                         home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats);
                    }
                    else {
                        avg = do_divide (visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                         visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats);
                        avg2 = do_divide (visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.so, 
                                         visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats);
                    }

                    if (avg > .250 && avg < .270 && avg2 <= .15)
                        game_status.status[8] = 1;
                }

    /* decide if to attempt a sacrifice */
    if ((innings >= 8 && (runs[ha] + 1) == runs[!ha]) || ((innings == 6 || innings == 7) &&
               ((runs[ha] + 1) == runs[!ha] || (runs[ha] + 2) == runs[!ha])) || border[ha][game_status.batter[ha]].pos[0] == 1)
        if (!game_status.status[7] && !game_status.status[8])
            if (game_status.outs < 2)
                if (game_status.baserunners[0] != 99 && game_status.baserunners[2] == 99)
                    if (!(int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                        if (ha) {
                            if ((do_divide (home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                     home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats) < .240)
                                     && home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.sh)
                                game_status.status[9] = 1;
                        }
                        else
                            if ((do_divide (visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                   visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats) < .240)
                                   && visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.sh)
                                game_status.status[9] = 1;
                    }

    /* decide if to attempt a squeeze play */
    if (innings > 6 || border[ha][game_status.batter[ha]].pos[0] == 1)
        if (!game_status.status[7] && !game_status.status[8])
            if ((runs[0] - runs[1]) >= -1 && (runs[0] - runs[1]) <= 1)
                if (game_status.outs < 2)
                    if (game_status.baserunners[2] != 99) {
                        if (ha) {
                            if ((do_divide (home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                     home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats) < .240)
                                     && home.batters[border[ha][game_status.batter[ha]].player[0]].hitting.sh)
                                game_status.status[10] = 1;
                        }
                        else
                            if ((do_divide (visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.hits, 
                                   visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.atbats) < .240)
                                   && visitor.batters[border[ha][game_status.batter[ha]].player[0]].hitting.sh)
                                game_status.status[10] = 1;
                    }
}

/* replace a player in the lineup */
int
replace_player (int ha, int st) {
    int x, y, pl, z, zz, zind, pind = 0, pospl;
    struct {
        int player, stat;
    } work[26];

    for (pl = 0; pl < 9; pl++) {
        if (border[ha][pl].pos[0] != st)
            continue;

        if (border[ha][pl].pos[1] == 1) {
            /* if it's the pitcher the replacement will be done elsewhere */
            pind = 1;
            continue;
        }

        for (x = 1; x < 30; x++)
            if (border[ha][pl].pos[x] < 10)
                pospl = x;

        for (x = 0; x < maxplayers[ha]; x++) {
            work[x].player = x;
            if (ha)
                if (border[ha][pl].pos[pospl] > 6)
                    /* sometimes the 3 outfield fielding stats we have are combined */
                    if (home.batters[x].fielding[border[ha][pl].pos[pospl]].po == -1)
                        work[x].stat = home.batters[x].fielding[10].games;
                    else
                        work[x].stat = home.batters[x].fielding[border[ha][pl].pos[pospl]].games;
                else
                    work[x].stat = home.batters[x].fielding[border[ha][pl].pos[pospl]].games;
            else
                if (border[ha][pl].pos[pospl] > 6)
                    /* sometimes the 3 outfield fielding stats we have are combined */
                    if (visitor.batters[x].fielding[border[ha][pl].pos[pospl]].po == -1)
                        work[x].stat = visitor.batters[x].fielding[10].games;
                    else
                        work[x].stat = visitor.batters[x].fielding[border[ha][pl].pos[pospl]].games;
                else
                    work[x].stat = visitor.batters[x].fielding[border[ha][pl].pos[pospl]].games;
        }

        for (x = 0; x < maxplayers[ha]; x++)
            for (y = x + 1; y < maxplayers[ha]; y++)
                if (work[x].stat < work[y].stat) {
                    work[25] = work[x];
                    work[x] = work[y];
                    work[y] = work[25];
                }

        for (x = 0; x < maxplayers[ha]; x++) {
            /* if the best match is the "pinch" player then we want to bring him in */
            if (border[ha][pl].player[0] == work[x].player)
                break;

            /* check to see if player has already been used */
            for (zind = z = 0; z < 9; z++)
                for (zz = 0; zz < 30; zz++)
                    if (border[ha][z].player[zz] == work[x].player)
                        zind = 1;
            if (zind)
                continue;

            /* make sure he's not injured */
            if (!ha) {
                if (visitor_season.batters[work[x].player].id.injury || visitor_cur.batters[work[x].player].id.injury)
                    continue;
            }
            else
                if (home_season.batters[work[x].player].id.injury || home_cur.batters[work[x].player].id.injury)
                    continue;

            break;
        }

        if (x == maxplayers[ha] || !work[x].stat)
            /* if there are no subs left or there is noone remaining that has played this position then we gotta use the "pinch" player */
            for (x = 0; x < maxplayers[ha]; x++)
                if (work[x].player == border[ha][pl].player[0])
                    break;
        if (x == maxplayers[ha])
            /* there's noone to bring in */
            return -1;

        /* bring in new player */
        for (z = 29; z > 0; z--) {
            border[ha][pl].player[z] = border[ha][pl].player[z - 1];
            border[ha][pl].pos[z] = border[ha][pl].pos[z - 1];
        }
        border[ha][pl].player[0] = work[x].player;
        border[ha][pl].pos[0] = border[ha][pl].pos[pospl + 1];

        lineupchg[ha][pl] = 1;

        if (ha)
            home_cur.batters[border[ha][pl].player[0]].fielding[border[ha][pl].pos[0]].games = 1;
        else
            visitor_cur.batters[border[ha][pl].player[0]].fielding[border[ha][pl].pos[0]].games = 1;

        if (border[ha][pl].player[0] != border[ha][pl].player[1]) {
            if (!ha)
                visitor_cur.batters[border[ha][pl].player[0]].hitting.games = 1;
            else
                home_cur.batters[border[ha][pl].player[0]].hitting.games = 1;
        }
    }
    game_status.status[st + 1] = 0;

    return pind;
}

/* replace a player in the lineup who's been injured */
int
replace_player2 (int ha) {
    int x, y, pl, z, zz, zind, retcde = 0;
    struct {
        int player, stat;
    } work[26];

    for (pl = 0; pl < 9; pl++) {
        if (ha) {
            if (!home_cur.batters[border[ha][pl].player[0]].id.injury)
                continue;
        }
        else
            if (!visitor_cur.batters[border[ha][pl].player[0]].id.injury)
                continue;

        if (border[ha][pl].pos[0] == 1) {
            /* if it's the pitcher the replacement will be done elsewhere */
            retcde = 1;
            continue;
        }
        if (border[ha][pl].pos[0] > 9)
            /* if it's a pinch hitter or pinch runner the replacement was done or will be done elsewhere */
            continue;

        for (x = 0; x < maxplayers[ha]; x++) {
            work[x].player = x;
            if (ha)
                if (border[ha][pl].pos[0] > 6)
                    /* sometimes the 3 outfield fielding stats we have are combined */
                    if (home.batters[x].fielding[border[ha][pl].pos[0]].po == -1)
                        work[x].stat = home.batters[x].fielding[10].games;
                    else
                        work[x].stat = home.batters[x].fielding[border[ha][pl].pos[0]].games;
                else
                    work[x].stat = home.batters[x].fielding[border[ha][pl].pos[0]].games;
            else
                if (border[ha][pl].pos[0] > 6)
                    /* sometimes the 3 outfield fielding stats we have are combined */
                    if (visitor.batters[x].fielding[border[ha][pl].pos[0]].po == -1)
                        work[x].stat = visitor.batters[x].fielding[10].games;
                    else
                        work[x].stat = visitor.batters[x].fielding[border[ha][pl].pos[0]].games;
                else
                    work[x].stat = visitor.batters[x].fielding[border[ha][pl].pos[0]].games;
        }

        for (x = 0; x < maxplayers[ha]; x++)
            for (y = x + 1; y < maxplayers[ha]; y++)
                if (work[x].stat < work[y].stat) {
                    work[25] = work[x];
                    work[x] = work[y];
                    work[y] = work[25];
                }

        for (x = 0; x < maxplayers[ha]; x++) {
            /* check to see if player has already been used */
            for (zind = z = 0; z < 9; z++)
                for (zz = 0; zz < 30; zz++)
                    if (border[ha][z].player[zz] == work[x].player)
                        zind = 1;
            if (zind)
                continue;

            /* make sure he's not injured */
            if (!ha) {
                if (visitor_season.batters[work[x].player].id.injury || visitor_cur.batters[work[x].player].id.injury)
                    continue;
            }
            else
                if (home_season.batters[work[x].player].id.injury || home_cur.batters[work[x].player].id.injury)
                    continue;

            break;
        }

        if (x == maxplayers[ha])
            /* if there are no subs left then we forfeit */
            return -1;

        /* bring in new player */
        for (z = 29; z > 0; z--) {
            border[ha][pl].player[z] = border[ha][pl].player[z - 1];
            border[ha][pl].pos[z] = border[ha][pl].pos[z - 1];
        }
        border[ha][pl].player[0] = work[x].player;
        border[ha][pl].pos[0] = border[ha][pl].pos[1];

        lineupchg[ha][pl] = 1;

        if (ha)
            home_cur.batters[border[ha][pl].player[0]].fielding[border[ha][pl].pos[0]].games = 1;
        else
            visitor_cur.batters[border[ha][pl].player[0]].fielding[border[ha][pl].pos[0]].games = 1;

        if (!ha)
            visitor_cur.batters[border[ha][pl].player[0]].hitting.games = 1;
        else
            home_cur.batters[border[ha][pl].player[0]].hitting.games = 1;
    }

    return retcde;
}

int
available_pitcher (int ha) {
    int x, y;

    for (x = 0; x < maxpitchers[ha]; x++) {
        /* make sure we haven't used him yet */
        for (y = 0; y < 15; y++)
            if (pitching[ha].pitcher[y] == x)
                break;
        if (y != 15)
            continue;

        /* we don't want to use a pitcher who only starts games */
        if (!ha) {
            if (!(visitor.pitchers[x].pitching.games - visitor.pitchers[x].pitching.games_started))
                continue;
        }
        else
            if (!(home.pitchers[x].pitching.games - home.pitchers[x].pitching.games_started))
                continue;

        /* it's possible that the pitcher has already played (or is playing) some other position */
        for (y = 0; y < maxplayers[ha]; y++)
            if (ha) {
                if (!strcmp (&home_cur.pitchers[x].id.name[0], &home_cur.batters[y].id.name[0]))
                    if (home_cur.batters[y].hitting.games)
                        break;
            }
            else
                if (!strcmp (&visitor_cur.pitchers[x].id.name[0], &visitor_cur.batters[y].id.name[0]))
                    if (visitor_cur.batters[y].hitting.games)
                        break;
        if (y < maxplayers[ha])
            continue;

        /* make sure he's not injured */
        if (!ha) {
            for (y = 0; y < maxplayers[ha]; y++)
                if (!strcmp (&visitor_cur.pitchers[x].id.name[0], &visitor_cur.batters[y].id.name[0]))
                    break;
            if (!visitor_season.batters[y].id.injury && !visitor_cur.batters[y].id.injury)
                break;
        }
        else {
            for (y = 0; y < maxplayers[ha]; y++)
                if (!strcmp (&home_cur.pitchers[x].id.name[0], &home_cur.batters[y].id.name[0]))
                    break;
            if (!home_season.batters[y].id.injury && !home_cur.batters[y].id.injury)
                break;
        }
    }

    if (x == maxpitchers[ha])
        return -1;
    else
        return 0;
}

int
available_player (int pos, int ha) {
    int x;

    for (x = 0; x < maxplayers[ha]; x++)
        if (ha) {
            /* find a player who's played this position */
            if (home.batters[x].fielding[pos].games) {
                /* make sure we haven't used him in this game yet */
                if (home_cur.batters[x].hitting.games)
                    continue;
                /* make sure this player is not injured */
                if (home_season.batters[x].id.injury || home_cur.batters[x].id.injury)
                    continue;
                /* we found someone on the bench who can play this position */
                break;
            }
        }
        else
            /* find a player who's played this position */
            if (visitor.batters[x].fielding[pos].games) {
                /* make sure we haven't used him in this game yet */
                if (visitor_cur.batters[x].hitting.games)
                    continue;
                /* make sure this player is not injured */
                if (visitor_season.batters[x].id.injury || visitor_cur.batters[x].id.injury)
                    continue;
                /* we found someone on the bench who can play this position */
                break;
            }

    if (x == maxplayers[ha])
        return -1;
    else
        return 0;
}

void
Send2BotHalf (int x) {
    int y;
    char action[4096];

    action[0] = '\0';

    for (y = 0; y < 9; y++)
        if (lineupchg[x][y]) {
            strcpy (&action[0], "BD");
            if (x)
                switch_name (&action[0], &home_cur.batters[border[x][y].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[x][y].player[0]].id.name[0]);
            strcat (&action[0], " is ");
            if (border[x][y].pos[0] < 10)
                strcat (&action[0], "now ");
            switch (border[x][y].pos[0]) {
                case 0:
                    strcat (&action[0], "the designated hitter");
                    break;
                case 1:
                    strcat (&action[0], "pitching");
                    break;
                case 2:
                    strcat (&action[0], "catching");
                    break;
                case 3:
                    strcat (&action[0], "playing first base");
                    break;
                case 4:
                    strcat (&action[0], "playing second base");
                    break;
                case 5:
                    strcat (&action[0], "playing third base");
                    break;
                case 6:
                    strcat (&action[0], "playing shortstop");
                    break;
                case 7:
                    strcat (&action[0], "playing left field");
                    break;
                case 8:
                    strcat (&action[0], "playing center field");
                    break;
                case 9:
                    strcat (&action[0], "playing right field");
                    break;
                case 10:
                    strcat (&action[0], "hitting for ");
                    break;
                case 11:
                    strcat (&action[0], "running for ");
            }
            if (border[x][y].pos[0] > 9) {
                if (x)
                    switch_name (&action[0], &home_cur.batters[border[x][y].player[1]].id.name[0]);
                else
                    switch_name (&action[0], &visitor_cur.batters[border[x][y].player[1]].id.name[0]);
            }
            strcat (&action[0], "\n");
            sock_puts (sock, &action[0]);
            if (netgame)
                sock_puts (netsock, &action[0]);
        }
    if (dhpchg[x]) {
        strcpy (&action[0], "BD");
        if (x) {
            for (y = 0; y < 25; y++)
                if (!strcmp (&home_cur.batters[y].id.name[0], &home_cur.pitchers[pitching[x].pitcher[0]].id.name[0]))
                    break;
            switch_name (&action[0], &home_cur.batters[y].id.name[0]);
        }
        else {
            for (y = 0; y < 25; y++)
                if (!strcmp (&visitor_cur.batters[y].id.name[0], &visitor_cur.pitchers[pitching[x].pitcher[0]].id.name[0]))
                    break;
            switch_name (&action[0], &visitor_cur.batters[y].id.name[0]);
        }
        strcat (&action[0], " is now pitching");
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }
}

