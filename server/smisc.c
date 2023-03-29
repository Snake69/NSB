/*
    miscellaneous functions
*/

#include "sglobal.h"
#include "db.h"
#include "sproto.h"
#include <stdlib.h>

/*
    select starting players
*/
void
determine_starters (char ah) {
    int which, what, pitcher, hora, x, y;
    struct {
        int p, gs;
    } sort[14];

    for (x = 0; x < 14; x++)
        sort[x].p = sort[x].gs = 0;

    if (ah == 'h') {
        team = home;
        team2 = home_season;
        hora = 1;
    }
    else {
        team = visitor;
        team2 = visitor_season;
        hora = 0;
    }

    for (y = 0; y < 10; y++)
        starters[hora][y] = 99;

    /* determine starting pitcher by games started */
    for (pitcher = 0; pitcher < maxpitchers[hora]; pitcher++) {
        sort[pitcher].gs = team.pitchers[pitcher].pitching.games_started;
        sort[pitcher].p = pitcher;
    }
    /* sort pitchers into descending sequence by games started */
    for (x = 0; x < (maxpitchers[hora] - 1); x++)
        for (y = x + 1; y < maxpitchers[hora]; y++)
            if (sort[x].gs < sort[y].gs) {
                sort[13] = sort[x];
                sort[x] = sort[y];
                sort[y] = sort[13];
            }

    for (which = 99, pitcher = 0; pitcher < maxpitchers[hora]; pitcher++) {
        /* check if pitcher rested */
        if (team2.pitchers[sort[pitcher].p].id.starts_rest < 4)
            continue;
        if (team2.pitchers[sort[pitcher].p].id.ip_last4g[0] || (team2.pitchers[sort[pitcher].p].id.ip_last4g[0] +
                  team2.pitchers[sort[pitcher].p].id.ip_last4g[1] + team2.pitchers[sort[pitcher].p].id.ip_last4g[2] +
                  team2.pitchers[sort[pitcher].p].id.ip_last4g[3]) > 2)
            continue;
        /* ensure that this pitcher is indeed a starter */
        if (team.pitchers[sort[pitcher].p].pitching.games_started)
            /* check for injury */
            for (x = 0; x < maxplayers[hora]; x++)
                if (!strcmp (&team.pitchers[sort[pitcher].p].id.name[0], &team.batters[x].id.name[0]))
                    if (!team2.batters[x].id.injury) {
                        which = sort[pitcher].p;
                        pitcher = maxpitchers[hora];
                        break;
                    }
    }

    y = 0;
PitcherWithMostRest:
    if (which == 99)
        /* no pitcher selected ... pick the starter with the most rest */
        for (what = -1, pitcher = 0; pitcher < maxpitchers[hora]; pitcher++) {
            if ((team2.pitchers[sort[pitcher].p].id.ip_last4g[0] + team2.pitchers[sort[pitcher].p].id.ip_last4g[1] +
                 team2.pitchers[sort[pitcher].p].id.ip_last4g[2] + team2.pitchers[sort[pitcher].p].id.ip_last4g[3]) > y)
                continue;
            if (team.pitchers[pitcher].pitching.games_started > 0 && team2.pitchers[pitcher].id.starts_rest > what)
                /* check for injury */
                for (x = 0; x < 28; x++)
                    if (!strcmp (&team.pitchers[pitcher].id.name[0], &team.batters[x].id.name[0])) {
                        if (!team2.batters[x].id.injury) {
                            what = team2.pitchers[pitcher].id.starts_rest;
                            which = pitcher;
                        }
                        break;
                    }
        }
    if (which == 99)
        if (++y < 10)
            goto PitcherWithMostRest;
         
    if (which == 99)
        /* still no pitcher selected ... pick the first uninjured pitcher */
        for (pitcher = 0; pitcher < maxpitchers[hora]; pitcher++)
            for (x = 0; x < 28; x++)
                if (!strcmp (&team.pitchers[pitcher].id.name[0], &team.batters[x].id.name[0])) {
                    if (!team2.batters[x].id.injury) {
                        what = team2.pitchers[pitcher].id.starts_rest;
                        which = pitcher;
                    }
                    break;
                }

    gotateam = 1;
    tforfeit = hora;   /* set this now just in case we need it later */
    if (which == 99)
        /* no pitchers available */
        gotateam = 0;
    else {
        /* use batter iteration number (all players) rather than pitcher iteration number */
        for (x = 0; x < maxplayers[hora]; x++)
            if (!strcmp (&team.pitchers[which].id.name[0], &team.batters[x].id.name[0])) {
                starters[hora][1] = x;
                break;
            }
        /* save pitcher iteration */
        game_status.pitcher[hora] = pitching[hora].pitcher[0] = which;
    }

    /* before filling in the remainder of the lineup look for any positions where there's only one player who can play
       that position ... if so, stick him in the lineup at that position */
    Look4OnePlayerPos (hora);

    /* determine remaining starters by games played at that position */
    for (which = 2; which < 10; which++)
        if (starters[hora][which] == 99) {
            starters[hora][which] = select_player (which, hora);
            if (starters[hora][which] == 99)
                gotateam = 0;
        }

    /* do we need a DH ? */
    if (dhind == YES) {
        starters[hora][0] = select_player (0, hora);
        if (starters[hora][0] == 99)
            gotateam = 0;
    }
}

/*
    look for any positions where there is only one player available and fill them in
*/
void
Look4OnePlayerPos (int hora) {
    int x, player, which, ply[2], games;

    for (which = 2; which < 10; which++) {
        ply[0] = ply[1] = 99;
        for (player = 0; player < maxplayers[hora]; player++) {
            if (team2.batters[player].id.injury)
                continue;
            games = team.batters[player].fielding[which].games;
            if (games) {
                if (ply[0] == 99)
                    ply[0] = player;
                else
                    ply[1] = player;
            }
        }
        if (ply[0] != 99 && ply[1] == 99) {
            /* ensure this player hasn't already been selected for another position */
            for (x = 0; x < 10; x++)
                if (ply[0] == starters[hora][x])
                    break;
            if (x == 10)
                starters[hora][which] = ply[0]; 
        }
    }
}

/*
    select starting player
*/
int
select_player (int pos, int hora) {
    int player, which, what, games, x;

    /* make the determination based upon games played */
    for (player = 0, which = 99, what = -1; player < maxplayers[hora]; player++) {
        /* check for injury */
        if (team2.batters[player].id.injury)     /* team2 contains the seasonal NSB stats */
            continue;
        games = team.batters[player].fielding[pos].games;

        if (games && games > what) {
            /* don't select the same player twice */
            for (x = 0; x < 10; x++)
                if (starters[hora][x] == player)
                    break;

            if (x == 10) {
                what = games;
                which = player;
            }
        }
    }

    if (what == -1) {
        /* noone selected ... we must pick someone */
        for (player = 0, which = 99; player < maxplayers[hora]; player++) {
            if (team2.batters[player].id.injury)     /* team2 contains the seasonal NSB stats */
                continue;
            if (pos > 6 || pos == 2 || pos == 3)
                if (team.batters[player].fielding[7].po == -1)
                    /* sometimes the stats for all three outfield positions are combined into one */
                    games = team.batters[player].fielding[10].games + team.batters[player].fielding[2].games + team.batters[player].fielding[3].games;
                else
                    games = team.batters[player].fielding[7].games + team.batters[player].fielding[8].games + team.batters[player].fielding[9].games +
                            team.batters[player].fielding[2].games + team.batters[player].fielding[3].games;
            else
                games = team.batters[player].fielding[4].games + team.batters[player].fielding[5].games + team.batters[player].fielding[6].games;
            if (!pos) {
                if (team.batters[player].fielding[7].po == -1)
                    /* sometimes the stats for all three outfield positions are combined into one */
                    games += (team.batters[player].fielding[10].games + team.batters[player].fielding[2].games + team.batters[player].fielding[3].games);
                else
                    games += (team.batters[player].fielding[7].games + team.batters[player].fielding[8].games + team.batters[player].fielding[9].games +
                             team.batters[player].fielding[2].games + team.batters[player].fielding[3].games);
            }

            if (games && games > what) {
                /* don't select the same player twice */
                for (x = 0; x < 10; x++)
                    if (starters[hora][x] == player)
                        break;

                if (x == 10) {
                    what = games;
                    which = player;
                }
            }
        }
    }

    if (what == -1)
        /* still noone selected ... we absolutely have to pick someone */
        for (player = 0, which = 99; player < maxplayers[hora]; player++) {
            if (team2.batters[player].id.injury)     /* team2 contains the seasonal NSB stats */
                continue;
            games = team.batters[player].hitting.games;

            if (games > what) {
                /* don't select the same player twice */
                for (x = 0; x < 10; x++)
                    if (starters[hora][x] == player)
                        break;

                if (x == 10) {
                    what = games;
                    which = player;
                }
            }
        }

    return which;
}

/*
    select batting order
*/
void
determine_battingorder (char ah) {
    int hora, player, which, pos, x;
    float whatf;

    if (ah == 'h') {
        team = home;
        hora = 1;
    }
    else {
        team = visitor;
        hora = 0;
    }

    /* first select the number three and four hitters based upon slugging pct */
    /* sel_order (3, hora); */
    /* sel_order (2, hora); */

    /* select the cleanup batter based upon slugging pct + power + rbis */
    for (player = which = 0, whatf = -99999.0; player < 10; player++) {
        if (player == 1)
            /* the pitcher always bats ninth */
            continue;
        if (dhind == NO && player == 0)
            /* skip the DH if applicable */
            continue;

        if ((float) ((float) (team.batters[starters[hora][player]].hitting.hits +
                             team.batters[starters[hora][player]].hitting.doubles +
                             (team.batters[starters[hora][player]].hitting.triples * 2.0) +
                             (team.batters[starters[hora][player]].hitting.homers * 3.0)) /
                     (float) team.batters[starters[hora][player]].hitting.atbats) + 

                     (float) (float) (team.batters[starters[hora][player]].hitting.homers * 10.0) /
                            (float) team.batters[starters[hora][player]].hitting.atbats +

                     (float) (float) team.batters[starters[hora][player]].hitting.rbi /
                            (float) team.batters[starters[hora][player]].hitting.games > whatf) {

            whatf = (float) ((float) (team.batters[starters[hora][player]].hitting.hits +
                                      team.batters[starters[hora][player]].hitting.doubles +
                                     (team.batters[starters[hora][player]].hitting.triples * 2.0) +
                                     (team.batters[starters[hora][player]].hitting.homers * 3.0)) /
                              (float) team.batters[starters[hora][player]].hitting.atbats + 
                     (float) (float) (team.batters[starters[hora][player]].hitting.homers * 10.0) /
                              (float) team.batters[starters[hora][player]].hitting.atbats +
                      (float) (float) team.batters[starters[hora][player]].hitting.rbi /
                              (float) team.batters[starters[hora][player]].hitting.games);

            which = starters[hora][player];
            pos = player;
        }
    }
    border[hora][3].player[0] = which;
    border[hora][3].pos[0] = pos;

    /* select the number three batter based upon slugging pct + batting avg */
    for (player = which = 0, whatf = -99999.0; player < 10; player++) {
        if (player == 1)
            /* the pitcher always bats ninth */
            continue;
        if (dhind == NO && player == 0)
            /* skip the DH if applicable */
            continue;
        if (starters[hora][player] == border[hora][3].player[0])
            /* skip if player already selected */
            continue;

        if ((float) ((float) (team.batters[starters[hora][player]].hitting.hits +
                             team.batters[starters[hora][player]].hitting.doubles +
                             (team.batters[starters[hora][player]].hitting.triples * 2.0) +
                             (team.batters[starters[hora][player]].hitting.homers * 3.0)) /
                     (float) team.batters[starters[hora][player]].hitting.atbats) + 

                     (float) (float) team.batters[starters[hora][player]].hitting.hits /
                            (float) team.batters[starters[hora][player]].hitting.atbats > whatf) {

            whatf = (float) ((float) (team.batters[starters[hora][player]].hitting.hits +
                                      team.batters[starters[hora][player]].hitting.doubles +
                                     (team.batters[starters[hora][player]].hitting.triples * 2.0) +
                                     (team.batters[starters[hora][player]].hitting.homers * 3.0)) /
                              (float) team.batters[starters[hora][player]].hitting.atbats + 
                      (float) (float) team.batters[starters[hora][player]].hitting.hits /
                              (float) team.batters[starters[hora][player]].hitting.atbats);

            which = starters[hora][player];
            pos = player;
        }
    }
    border[hora][2].player[0] = which;
    border[hora][2].pos[0] = pos;

    /* select the leadoff batter based upon at-bats + on-base percentage + speed */
    for (player = which = 0, whatf = -99999.0; player < 10; player++) {
        if (player == 1)
            /* the pitcher always bats ninth */
            continue;
        if (dhind == NO && player == 0)
            /* skip the DH if applicable */
            continue;
        if (starters[hora][player] == border[hora][2].player[0] || starters[hora][player] == border[hora][3].player[0])
            /* skip if player already selected */
            continue;

        if ((float) ((float) (team.batters[starters[hora][player]].hitting.atbats +
                                 team.batters[starters[hora][player]].hitting.bb +
                                 team.batters[starters[hora][player]].hitting.sf +
                                 team.batters[starters[hora][player]].hitting.sh +
                                  team.batters[starters[hora][player]].hitting.hbp) /
                                     (team.batters[starters[hora][player]].hitting.games * 10.0) +

                     (float) (float) (team.batters[starters[hora][player]].hitting.hits +
                                         team.batters[starters[hora][player]].hitting.bb +
                                          team.batters[starters[hora][player]].hitting.hbp) /
                            (float) (team.batters[starters[hora][player]].hitting.atbats +
                                         team.batters[starters[hora][player]].hitting.bb +
                                         team.batters[starters[hora][player]].hitting.sf +
                                         team.batters[starters[hora][player]].hitting.sh +
                                          team.batters[starters[hora][player]].hitting.hbp) +

                     (float) (float) ((team.batters[starters[hora][player]].hitting.sb * 2.0) /
                            (float) (team.batters[starters[hora][player]].hitting.hits +
                                         team.batters[starters[hora][player]].hitting.bb +
                                          team.batters[starters[hora][player]].hitting.hbp)) > whatf)) {

            whatf = (float) ((float) (team.batters[starters[hora][player]].hitting.atbats +
                                      team.batters[starters[hora][player]].hitting.bb +
                                      team.batters[starters[hora][player]].hitting.sf +
                                      team.batters[starters[hora][player]].hitting.sh +
                                      team.batters[starters[hora][player]].hitting.hbp) /
                                     (team.batters[starters[hora][player]].hitting.games * 10.0) +
                     (float) (float) (team.batters[starters[hora][player]].hitting.hits +
                                      team.batters[starters[hora][player]].hitting.bb +
                                      team.batters[starters[hora][player]].hitting.hbp) /
                             (float) (team.batters[starters[hora][player]].hitting.atbats +
                                      team.batters[starters[hora][player]].hitting.bb +
                                      team.batters[starters[hora][player]].hitting.sf +
                                      team.batters[starters[hora][player]].hitting.sh +
                                      team.batters[starters[hora][player]].hitting.hbp) +
                     (float) (float) (team.batters[starters[hora][player]].hitting.sb * 2.0) /
                             (float) (team.batters[starters[hora][player]].hitting.hits +
                                      team.batters[starters[hora][player]].hitting.bb +
                                      team.batters[starters[hora][player]].hitting.hbp));
            which = starters[hora][player];
            pos = player;
        }
    }
    border[hora][0].player[0] = which;
    border[hora][0].pos[0] = pos;

    /* select the second batter based upon bat control + speed */
    for (player = which = 0, whatf = -99999.0; player < 10; player++) {
        if (player == 1)
            /* the pitcher always bats ninth */
            continue;
        if (dhind == NO && player == 0)
            /* skip the DH if applicable */
            continue;
        if (starters[hora][player] == border[hora][2].player[0] ||
            starters[hora][player] == border[hora][3].player[0] ||
            starters[hora][player] == border[hora][0].player[0])
            /* skip if player already selected */
            continue;

        if ((float) ((float) team.batters[starters[hora][player]].hitting.atbats / (float) (team.batters[starters[hora][player]].hitting.so * 10.0)) + 
                     (float) (float) ((team.batters[starters[hora][player]].hitting.sb * 2.0) /
                            (float) (team.batters[starters[hora][player]].hitting.hits + team.batters[starters[hora][player]].hitting.bb + team.batters[starters[hora][player]].hitting.hbp)) > whatf) {

            whatf = (float) ((float) team.batters[starters[hora][player]].hitting.atbats / (float) (team.batters[starters[hora][player]].hitting.so * 10.0) + 
                    (float) (float) (team.batters[starters[hora][player]].hitting.sb * 2.0) /
                            (float) (team.batters[starters[hora][player]].hitting.hits + team.batters[starters[hora][player]].hitting.bb + team.batters[starters[hora][player]].hitting.hbp));
            which = starters[hora][player];
            pos = player;
        }
    }
    border[hora][1].player[0] = which;
    border[hora][1].pos[0] = pos;

    /* select the rest of the batting order based upon slugging percentage */
    if (dhind == NO) {
        /* the pitcher always bats ninth */
        border[hora][8].player[0] = starters[hora][1];
        border[hora][8].pos[0] = 1;
        for (x = 4; x < 8; x++)
            sel_order (x, hora);
    }
    else
        for (x = 4; x < 9; x++)
            sel_order (x, hora);
}

void
sel_order (int ord, int hora) {
    int sortptr, x, y, z;
    struct {
        int player, pos;
        float slugging;
    } sort[10];

    /* calculate remaining starters' slugging pct and prepare for sorting */
    for (x = sortptr = 0; x < 10; x++) {
        if (dhind == NO && x == 0)
            /* skip DH if applicable */
            continue;
        if (x == 1)
            /* skip pitcher */
            continue;

        /* make sure player hasn't been already selected */
        for (z = y = 0; y < 9; y++)
            if (starters[hora][x] == border[hora][y].player[0])
                z = 1;
        if (z)
            continue;

        sort[sortptr].player = starters[hora][x];
        sort[sortptr].pos = x;
        sort[sortptr].slugging = (float) ((float) (team.batters[starters[hora][x]].hitting.hits -
                                          (float) (team.batters[starters[hora][x]].hitting.doubles +
                                                   team.batters[starters[hora][x]].hitting.triples +
                                                   team.batters[starters[hora][x]].hitting.homers) +
                                          (float) (2.0 * team.batters[starters[hora][x]].hitting.doubles) +
                                          (float) (3.0 * team.batters[starters[hora][x]].hitting.triples) +
                                          (float) (4.0 * team.batters[starters[hora][x]].hitting.homers)) /
                                          (float) team.batters[starters[hora][x]].hitting.atbats);
        sortptr++;
    }

    /* sort remaining starters into ascending sequence by slugging pct */
    for (x = 0; x < (sortptr - 2); x++)
        for (y = x + 1; y < (sortptr - 1); y++)
            if (sort[x].slugging < sort[y].slugging) {
                sort[9] = sort[x];
                sort[x] = sort[y];
                sort[y] = sort[9];
            }

    /* insert starter into batting order */
    border[hora][ord].player[0] = sort[0].player;
    border[hora][ord].pos[0] = sort[0].pos;
}

/*
    convert int (max length 6 positions) to string
*/
char *
cnvt_int2str (int retlen, int num) {
    int div, x, y;

    for (div = 100000, x = 0; x < 6; x++) {
        numstr[x] = (num / div) + '0';
        num = num - ((num / div) * div);
        div = div / 10;
    }
    numstr[6] = '\0';

    for (x = 6; x > retlen; x--)
        for (y = 0; numstr[y] != '\0'; y++)
            numstr[y] = numstr[y + 1];

    return (&numstr[0]);
}

/*
    return the surname of the player on defense
*/
char *
ReturnDefName (int pos) {
    int x;
    char *comma;

    if (pos == 1 && dhind)
        if (pre_act.half_inning % 2) {
            comma = (char *) index (&visitor.pitchers[game_status.pitcher[0]].id.name[0], ',');
            *comma = '\0';
            strcpy (&defplyrsurname[0], &visitor.pitchers[game_status.pitcher[0]].id.name[0]);
            *comma = ',';
        }
        else {
            comma = (char *) index (&home.pitchers[game_status.pitcher[1]].id.name[0], ',');
            *comma = '\0';
            strcpy (&defplyrsurname[0], &home.pitchers[game_status.pitcher[1]].id.name[0]);
            *comma = ',';
        }
    else
        if (pre_act.half_inning % 2) {
            for (x = 0; x < 9; x++)
                if (border[0][x].pos[0] == pos)
                    break;
            comma = (char *) index (&visitor_cur.batters[border[0][x].player[0]].id.name[0], ',');
            *comma = '\0';
            strcpy (&defplyrsurname[0], &visitor_cur.batters[border[0][x].player[0]].id.name[0]);
            *comma = ',';
        }
        else {
            for (x = 0; x < 9; x++)
                if (border[1][x].pos[0] == pos)
                    break;
            comma = (char *) index (&home_cur.batters[border[1][x].player[0]].id.name[0], ',');
            *comma = '\0';
            strcpy (&defplyrsurname[0], &home_cur.batters[border[1][x].player[0]].id.name[0]);
            *comma = ',';
        }

    return (&defplyrsurname[0]);
}

/* clear stats for current game */
void
clear_stats () {
    int b, s;

    for (b = 0; b < 28; b++) {
        team.batters[b].hitting.games = 0;
        team.batters[b].hitting.atbats = 0;
        team.batters[b].hitting.runs = 0;
        team.batters[b].hitting.hits = 0;
        team.batters[b].hitting.doubles = 0;
        team.batters[b].hitting.triples = 0;
        team.batters[b].hitting.homers = 0;
        team.batters[b].hitting.rbi = 0;
        team.batters[b].hitting.bb = 0;
        team.batters[b].hitting.so = 0;
        team.batters[b].hitting.hbp = 0;
        team.batters[b].hitting.gidp = 0;
        team.batters[b].hitting.sb = 0;
        team.batters[b].hitting.cs = 0;
        team.batters[b].hitting.ibb = 0;
        team.batters[b].hitting.sh = 0;
        team.batters[b].hitting.sf = 0;
        for (s = 0; s < 11; s++) {
            team.batters[b].fielding[s].games = 0;
            team.batters[b].fielding[s].po = 0;
            team.batters[b].fielding[s].dp = 0;
            team.batters[b].fielding[s].a = 0;
            team.batters[b].fielding[s].pb = 0;
            team.batters[b].fielding[s].e = 0;
        }
    }

    for (b = 0; b < 13; b++) {
        /* calculate inn_target (tired factor) while rounding up */
        team.pitchers[b].id.inn_target = (float) team.pitchers[b].pitching.innings / (float) team.pitchers[b].pitching.games + 0.5;

        team.pitchers[b].pitching.wins = 0;
        team.pitchers[b].pitching.losses = 0;
        team.pitchers[b].pitching.games = 0;
        team.pitchers[b].pitching.games_started = 0;
        team.pitchers[b].pitching.rbi = 0;
        team.pitchers[b].pitching.cg = 0;
        team.pitchers[b].pitching.gf = 0;
        team.pitchers[b].pitching.sho = 0;
        team.pitchers[b].pitching.sb = 0;
        team.pitchers[b].pitching.cs = 0;
        team.pitchers[b].pitching.svopp = 0;
        team.pitchers[b].pitching.saves = 0;
        team.pitchers[b].pitching.innings = 0;
        team.pitchers[b].pitching.thirds = 0;
        team.pitchers[b].pitching.bfp = 0;
        team.pitchers[b].pitching.ibb = 0;
        team.pitchers[b].pitching.sh = 0;
        team.pitchers[b].pitching.sf = 0;
        team.pitchers[b].pitching.wp = 0;
        team.pitchers[b].pitching.balks = 0;
        team.pitchers[b].pitching.hb = 0;
        team.pitchers[b].pitching.hits = 0;
        team.pitchers[b].pitching.runs = 0;
        team.pitchers[b].pitching.er = 0;
        team.pitchers[b].pitching.homers = 0;
        team.pitchers[b].pitching.walks = 0;
        team.pitchers[b].pitching.so = 0;
        team.pitchers[b].pitching.doubles = 0;
        team.pitchers[b].pitching.triples = 0;
        team.pitchers[b].pitching.opp_ab = 0;
    }
}

/* clear stats for visitor season structure */
void
zero_visitor_season () {
    int b, s, x;

    visitor_season = visitor;

    for (b = 0; b < 28; b++) {
        visitor_season.batters[b].id.injury = 0;
        visitor_season.batters[b].hitting.games = 0;
        visitor_season.batters[b].hitting.atbats = 0;
        visitor_season.batters[b].hitting.runs = 0;
        visitor_season.batters[b].hitting.hits = 0;
        visitor_season.batters[b].hitting.doubles = 0;
        visitor_season.batters[b].hitting.triples = 0;
        visitor_season.batters[b].hitting.homers = 0;
        visitor_season.batters[b].hitting.rbi = 0;
        visitor_season.batters[b].hitting.bb = 0;
        visitor_season.batters[b].hitting.so = 0;
        visitor_season.batters[b].hitting.hbp = 0;
        visitor_season.batters[b].hitting.gidp = 0;
        visitor_season.batters[b].hitting.sb = 0;
        visitor_season.batters[b].hitting.cs = 0;
        visitor_season.batters[b].hitting.ibb = 0;
        visitor_season.batters[b].hitting.sh = 0;
        visitor_season.batters[b].hitting.sf = 0;
        for (s = 0; s < 11; s++) {
            visitor_season.batters[b].fielding[s].games = 0;
            visitor_season.batters[b].fielding[s].po = 0;
            visitor_season.batters[b].fielding[s].dp = 0;
            visitor_season.batters[b].fielding[s].a = 0;
            visitor_season.batters[b].fielding[s].pb = 0;
            visitor_season.batters[b].fielding[s].e = 0;
        }
    }

    for (b = 0; b < 13; b++) {
        visitor_season.pitchers[b].id.starts_rest = 100;
        for (x = 0; x < 4; x++)
            visitor_season.pitchers[b].id.ip_last4g[x] = 0;
        /* calculate inn_target (tired factor) while rounding up */
        visitor_season.pitchers[b].id.inn_target =
                                          (float) visitor_season.pitchers[b].pitching.innings / (float) visitor_season.pitchers[b].pitching.games + 0.5;

        visitor_season.pitchers[b].pitching.wins = 0;
        visitor_season.pitchers[b].pitching.losses = 0;
        visitor_season.pitchers[b].pitching.games = 0;
        visitor_season.pitchers[b].pitching.games_started = 0;
        visitor_season.pitchers[b].pitching.rbi = 0;
        visitor_season.pitchers[b].pitching.cg = 0;
        visitor_season.pitchers[b].pitching.gf = 0;
        visitor_season.pitchers[b].pitching.sho = 0;
        visitor_season.pitchers[b].pitching.sb = 0;
        visitor_season.pitchers[b].pitching.cs = 0;
        visitor_season.pitchers[b].pitching.svopp = 0;
        visitor_season.pitchers[b].pitching.saves = 0;
        visitor_season.pitchers[b].pitching.innings = 0;
        visitor_season.pitchers[b].pitching.thirds = 0;
        visitor_season.pitchers[b].pitching.bfp = 0;
        visitor_season.pitchers[b].pitching.ibb = 0;
        visitor_season.pitchers[b].pitching.sh = 0;
        visitor_season.pitchers[b].pitching.sf = 0;
        visitor_season.pitchers[b].pitching.wp = 0;
        visitor_season.pitchers[b].pitching.balks = 0;
        visitor_season.pitchers[b].pitching.hb = 0;
        visitor_season.pitchers[b].pitching.hits = 0;
        visitor_season.pitchers[b].pitching.runs = 0;
        visitor_season.pitchers[b].pitching.er = 0;
        visitor_season.pitchers[b].pitching.homers = 0;
        visitor_season.pitchers[b].pitching.walks = 0;
        visitor_season.pitchers[b].pitching.so = 0;
        visitor_season.pitchers[b].pitching.doubles = 0;
        visitor_season.pitchers[b].pitching.triples = 0;
        visitor_season.pitchers[b].pitching.opp_ab = 0;
    }
}

/* clear stats for home team season structure */
void
zero_home_season () {
    int b, s, x;

    home_season = home;

    for (b = 0; b < 28; b++) {
        home_season.batters[b].id.injury = 0;
        home_season.batters[b].hitting.games = 0;
        home_season.batters[b].hitting.atbats = 0;
        home_season.batters[b].hitting.runs = 0;
        home_season.batters[b].hitting.hits = 0;
        home_season.batters[b].hitting.doubles = 0;
        home_season.batters[b].hitting.triples = 0;
        home_season.batters[b].hitting.homers = 0;
        home_season.batters[b].hitting.rbi = 0;
        home_season.batters[b].hitting.bb = 0;
        home_season.batters[b].hitting.so = 0;
        home_season.batters[b].hitting.hbp = 0;
        home_season.batters[b].hitting.gidp = 0;
        home_season.batters[b].hitting.sb = 0;
        home_season.batters[b].hitting.cs = 0;
        home_season.batters[b].hitting.ibb = 0;
        home_season.batters[b].hitting.sh = 0;
        home_season.batters[b].hitting.sf = 0;
        for (s = 0; s < 11; s++) {
            home_season.batters[b].fielding[s].games = 0;
            home_season.batters[b].fielding[s].po = 0;
            home_season.batters[b].fielding[s].dp = 0;
            home_season.batters[b].fielding[s].a = 0;
            home_season.batters[b].fielding[s].pb = 0;
            home_season.batters[b].fielding[s].e = 0;
        }
    }

    for (b = 0; b < 13; b++) {
        home_season.pitchers[b].id.starts_rest = 100;
        for (x = 0; x < 4; x++)
            home_season.pitchers[b].id.ip_last4g[x] = 0;
        /* calculate inn_target (tired factor) while rounding up */
        home_season.pitchers[b].id.inn_target = (float) home.pitchers[b].pitching.innings / (float) home.pitchers[b].pitching.games + 0.5;

        home_season.pitchers[b].pitching.wins = 0;
        home_season.pitchers[b].pitching.losses = 0;
        home_season.pitchers[b].pitching.games = 0;
        home_season.pitchers[b].pitching.games_started = 0;
        home_season.pitchers[b].pitching.rbi = 0;
        home_season.pitchers[b].pitching.cg = 0;
        home_season.pitchers[b].pitching.gf = 0;
        home_season.pitchers[b].pitching.sho = 0;
        home_season.pitchers[b].pitching.sb = 0;
        home_season.pitchers[b].pitching.cs = 0;
        home_season.pitchers[b].pitching.svopp = 0;
        home_season.pitchers[b].pitching.saves = 0;
        home_season.pitchers[b].pitching.innings = 0;
        home_season.pitchers[b].pitching.thirds = 0;
        home_season.pitchers[b].pitching.bfp = 0;
        home_season.pitchers[b].pitching.ibb = 0;
        home_season.pitchers[b].pitching.sh = 0;
        home_season.pitchers[b].pitching.sf = 0;
        home_season.pitchers[b].pitching.wp = 0;
        home_season.pitchers[b].pitching.balks = 0;
        home_season.pitchers[b].pitching.hb = 0;
        home_season.pitchers[b].pitching.hits = 0;
        home_season.pitchers[b].pitching.runs = 0;
        home_season.pitchers[b].pitching.er = 0;
        home_season.pitchers[b].pitching.homers = 0;
        home_season.pitchers[b].pitching.walks = 0;
        home_season.pitchers[b].pitching.so = 0;
        home_season.pitchers[b].pitching.doubles = 0;
        home_season.pitchers[b].pitching.triples = 0;
        home_season.pitchers[b].pitching.opp_ab = 0;
    }
}

/* set up for current game stats */
void
setup_cgs () {
    team = visitor;
    clear_stats ();
    visitor_cur = team;

    team = home;
    clear_stats ();
    home_cur = team;
}

/* send entire team structure to client */
void
send_stats (int sock, char which) {
    int x, y;

    if (which == 'h')
        team = home_cur;
    if (which == 'v')
        team = visitor_cur;
    if (which == 'a')
        team = home_season;
    if (which == 'b')
        team = visitor_season;
    if (which == 'c')
        team = home;
    if (which == 'd')
        team = visitor;

    sprintf (buffer1, "%d\n%d\n%c\n%c\n", team.id, team.year, team.league, team.division);
    /* send stats to client */
    sock_puts (sock, &buffer1[0]);

    for (x = 0; x < 28; x++) {
        sprintf (buffer1, "%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].id.name, team.batters[x].id.teamid,
                 team.batters[x].id.batslr, team.batters[x].id.throwslr, team.batters[x].id.year, team.batters[x].id.injury,
                 team.batters[x].id.starts_rest, team.batters[x].id.ip_last4g[0], team.batters[x].id.ip_last4g[1], team.batters[x].id.ip_last4g[2],
                 team.batters[x].id.ip_last4g[3], team.batters[x].id.inn_target);
        /* send stats to client */
        sock_puts (sock, &buffer1[0]);

        sprintf (buffer1, "%d\n%d\n%d\n", team.batters[x].dob.month, team.batters[x].dob.day, team.batters[x].dob.year);
        sock_puts (sock, &buffer1[0]);

        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].hitting.games, team.batters[x].hitting.atbats,
                 team.batters[x].hitting.runs, team.batters[x].hitting.hits, team.batters[x].hitting.doubles, team.batters[x].hitting.triples);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].hitting.homers, team.batters[x].hitting.rbi, team.batters[x].hitting.bb,
                 team.batters[x].hitting.so, team.batters[x].hitting.hbp, team.batters[x].hitting.gidp);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].hitting.sb, team.batters[x].hitting.cs, team.batters[x].hitting.ibb,
                 team.batters[x].hitting.sh, team.batters[x].hitting.sf, team.batters[x].hitting.statsind);
        sock_puts (sock, &buffer1[0]);
        for (y = 0; y < 11; y++) {
            sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].fielding[y].games, team.batters[x].fielding[y].po,
                     team.batters[x].fielding[y].dp, team.batters[x].fielding[y].a, team.batters[x].fielding[y].pb, team.batters[x].fielding[y].e);
            sock_puts (sock, &buffer1[0]);
        }
    }
    for (x = 0; x < 13; x++) {
        sprintf (buffer1, "%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].id.name, team.pitchers[x].id.teamid,
                 team.pitchers[x].id.batslr, team.pitchers[x].id.throwslr, team.pitchers[x].id.year, team.pitchers[x].id.injury,
                 team.pitchers[x].id.starts_rest, team.pitchers[x].id.ip_last4g[0], team.pitchers[x].id.ip_last4g[1], team.pitchers[x].id.ip_last4g[2],
                 team.pitchers[x].id.ip_last4g[3], team.pitchers[x].id.inn_target);
        /* send stats to client */
        sock_puts (sock, &buffer1[0]);

        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.games, team.pitchers[x].pitching.games_started,
                 team.pitchers[x].pitching.innings, team.pitchers[x].pitching.thirds, team.pitchers[x].pitching.wins, team.pitchers[x].pitching.losses, team.pitchers[x].pitching.saves);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.bfp, team.pitchers[x].pitching.hits,
                 team.pitchers[x].pitching.doubles, team.pitchers[x].pitching.triples, team.pitchers[x].pitching.homers, team.pitchers[x].pitching.runs);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.er, team.pitchers[x].pitching.rbi, team.pitchers[x].pitching.cg,
                 team.pitchers[x].pitching.gf, team.pitchers[x].pitching.sho, team.pitchers[x].pitching.svopp);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.sb, team.pitchers[x].pitching.cs,
                 team.pitchers[x].pitching.walks, team.pitchers[x].pitching.so, team.pitchers[x].pitching.ibb, team.pitchers[x].pitching.sh);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.sf, team.pitchers[x].pitching.wp,
                 team.pitchers[x].pitching.balks, team.pitchers[x].pitching.hb, team.pitchers[x].pitching.opp_ab, team.pitchers[x].pitching.statsind);
        sock_puts (sock, &buffer1[0]);
    }
}

/* send cummed stats for team to client */
void
send_cum_stats (int uc, char *tname) {
    int x, y, s1, s2, s3, s4, s5, s6, s7;

    sprintf (buffer1, "%d\n%d\n%c\n%c\n", team.id, team.year, team.league, team.division);
    sock_puts (sock, &buffer1[0]);

    if (uc) {
            strcpy (&buffer1[0], tname);
            strcat (&buffer1[0], "\n");
            sock_puts (sock, &buffer1[0]);
    }
    else
        if (!team.year) {
            strcpy (&buffer1[0], (char *) GetUCTeamname (team.id));
            strcat (&buffer1[0], "\n");
            sock_puts (sock, &buffer1[0]);
        }

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxplayers[0]; x++) {
        s1 += team.batters[x].hitting.games;
        s2 += team.batters[x].hitting.atbats;
        s3 += team.batters[x].hitting.runs;
        s4 += team.batters[x].hitting.hits;
        s5 += team.batters[x].hitting.doubles;
        s6 += team.batters[x].hitting.triples;
    }
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxplayers[0]; x++) {
        s1 += team.batters[x].hitting.homers;
        s2 += team.batters[x].hitting.rbi;
        s3 += team.batters[x].hitting.bb;
        if (team.batters[x].hitting.so != -1)
            s4 += team.batters[x].hitting.so;
        s5 += team.batters[x].hitting.hbp;
        if (team.batters[x].hitting.gidp != -1)
            s6 += team.batters[x].hitting.gidp;
    }
    if (s4 < 0)
        s4 = -1;
    if (s6 < 0)
        s6 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = x = 0; x < maxplayers[0]; x++) {
        s1 += team.batters[x].hitting.sb;
        if (team.batters[x].hitting.cs != -1)
            s2 += team.batters[x].hitting.cs;
        if (team.batters[x].hitting.ibb != -1)
            s3 += team.batters[x].hitting.ibb;
        s4 += team.batters[x].hitting.sh;
        if (team.batters[x].hitting.sf != -1)
            s5 += team.batters[x].hitting.sf;
    }
    if (s2 < 0)
        s2 = -1;
    if (s3 < 0)
        s3 = -1;
    if (s5 < 0)
        s5 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxplayers[0]; x++)
        for (y = 0; y < 11; y++) {
            s1 += team.batters[x].fielding[y].games;
            if (team.batters[x].fielding[y].po != -1)
                s2 += team.batters[x].fielding[y].po;
            s3 += team.batters[x].fielding[y].dp;
            if (team.batters[x].fielding[y].a != -1)
                s4 += team.batters[x].fielding[y].a;
            if (y == 2)
                s5 += team.batters[x].fielding[y].pb;
            if (team.batters[x].fielding[y].e != -1)
                s6 += team.batters[x].fielding[y].e;
        }
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = s7 = x = 0; x < maxpitchers[0]; x++) {
        s1 += team.pitchers[x].pitching.games;
        s2 += team.pitchers[x].pitching.games_started;
        s3 += team.pitchers[x].pitching.innings;
        s4 += team.pitchers[x].pitching.thirds;
        s5 += team.pitchers[x].pitching.wins;
        s6 += team.pitchers[x].pitching.losses;
        s7 += team.pitchers[x].pitching.saves;
    }
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6, s7);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxpitchers[0]; x++) {
        if (team.pitchers[x].pitching.bfp != -1)
            s1 += team.pitchers[x].pitching.bfp;
        s2 += team.pitchers[x].pitching.hits;
        if (team.pitchers[x].pitching.doubles != -1)
            s3 += team.pitchers[x].pitching.doubles;
        if (team.pitchers[x].pitching.triples != -1)
            s4 += team.pitchers[x].pitching.triples;
        s5 += team.pitchers[x].pitching.homers;
        s6 += team.pitchers[x].pitching.runs;
    }
    if (s1 < 0)
        s1 = -1;
    if (s3 < 0)
        s3 = -1;
    if (s4 < 0)
        s4 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxpitchers[0]; x++) {
        s1 += team.pitchers[x].pitching.er;
        if (team.pitchers[x].pitching.rbi != -1)
            s2 += team.pitchers[x].pitching.rbi;
        s3 += team.pitchers[x].pitching.cg;
        s4 += team.pitchers[x].pitching.gf;
        s5 += team.pitchers[x].pitching.sho;
        if (team.pitchers[x].pitching.svopp != -1)
            s6 += team.pitchers[x].pitching.svopp;
    }
    if (s2 < 0)
        s2 = -1;
    if (s6 < 0)
        s6 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = s6 = x = 0; x < maxpitchers[0]; x++) {
        if (team.pitchers[x].pitching.sb != -1)
            s1 += team.pitchers[x].pitching.sb;
        if (team.pitchers[x].pitching.cs != -1)
            s2 += team.pitchers[x].pitching.cs;
        s3 += team.pitchers[x].pitching.walks;
        s4 += team.pitchers[x].pitching.so;
        if (team.pitchers[x].pitching.ibb != -1)
            s5 += team.pitchers[x].pitching.ibb;
        if (team.pitchers[x].pitching.sh != -1)
            s6 += team.pitchers[x].pitching.sh;
    }
    if (s1 < 0)
        s1 = -1;
    if (s2 < 0)
        s2 = -1;
    if (s5 < 0)
        s5 = -1;
    if (s6 < 0)
        s6 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5, s6);
    sock_puts (sock, &buffer1[0]);

    for (s1 = s2 = s3 = s4 = s5 = x = 0; x < maxpitchers[0]; x++) {
        if (team.pitchers[x].pitching.sf != -1)
            s1 += team.pitchers[x].pitching.sf;
        s2 += team.pitchers[x].pitching.wp;
        s3 += team.pitchers[x].pitching.balks;
        s4 += team.pitchers[x].pitching.hb;
        s5 += team.pitchers[x].pitching.opp_ab;
    }
    if (s1 < 0)
        s1 = -1;
    sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n", s1, s2, s3, s4, s5);
    sock_puts (sock, &buffer1[0]);
}

/* a run has scored, check for a change in status for win, loss, and save */
void
chk_wls (int pitcher) {
    int field, runs[2], innings, x;

    /* which team is pitching? */
    field = 1 - (game_status.half_inning % 2);

    /* calculate number of innings played and the total number of runs scored for each team */
    innings = game_status.half_inning / 2 + 1;
    for (runs[0] = runs[1] = x = 0; x < innings; x++) {
        runs[0] += game_status.inning_score[x][0];
        runs[1] += game_status.inning_score[x][1];
    }

    /* the score just got tied up */
    if (runs[field] == runs[!field]) {
        pwin = ploss = saver = 99;
        if (savei) {
            /* a blown save */
            if (!field)
                visitor_cur.pitchers[pitching[field].pitcher[0]].pitching.svopp++;
            else   
                home_cur.pitchers[pitching[field].pitcher[0]].pitching.svopp++;
        }
        savei = 0;
    }

    /* the team at bat just went up by a run */
    if (runs[!field] == (runs[field] + 1)) {
        ploss = pitcher;

        if (field)
            if (visitor_cur.pitchers[pitching[!field].pitcher[0]].pitching.games_started) {
                if (visitor_cur.pitchers[pitching[!field].pitcher[0]].pitching.innings > 4)
                    pwin = pitching[!field].pitcher[0];
            }
            else
                pwin = pitching[!field].pitcher[0];
        else   
            if (home_cur.pitchers[pitching[!field].pitcher[0]].pitching.games_started) {
                if (home_cur.pitchers[pitching[!field].pitcher[0]].pitching.innings > 4)
                    pwin = pitching[!field].pitcher[0];
            }
            else
                pwin = pitching[!field].pitcher[0];
    }
}

void
combine_stats () {
    int x, y;

    for (x = 0; x < 28; x++) {
        team.batters[x].hitting.games += team2.batters[x].hitting.games;
        team.batters[x].hitting.atbats += team2.batters[x].hitting.atbats;
        team.batters[x].hitting.runs += team2.batters[x].hitting.runs;
        team.batters[x].hitting.hits += team2.batters[x].hitting.hits;
        team.batters[x].hitting.doubles += team2.batters[x].hitting.doubles;
        team.batters[x].hitting.triples += team2.batters[x].hitting.triples;
        team.batters[x].hitting.homers += team2.batters[x].hitting.homers;
        team.batters[x].hitting.rbi += team2.batters[x].hitting.rbi;
        team.batters[x].hitting.bb += team2.batters[x].hitting.bb;
        team.batters[x].hitting.so += team2.batters[x].hitting.so;
        team.batters[x].hitting.hbp += team2.batters[x].hitting.hbp;
        team.batters[x].hitting.gidp += team2.batters[x].hitting.gidp;
        team.batters[x].hitting.sb += team2.batters[x].hitting.sb;
        team.batters[x].hitting.cs += team2.batters[x].hitting.cs;
        team.batters[x].hitting.ibb += team2.batters[x].hitting.ibb;
        team.batters[x].hitting.sh += team2.batters[x].hitting.sh;
        team.batters[x].hitting.sf += team2.batters[x].hitting.sf;
        team.batters[x].id.injury += team2.batters[x].id.injury;
        team.batters[x].id.injury = 0;

        for (y = 0; y < 11; y++) {
            team.batters[x].fielding[y].games += team2.batters[x].fielding[y].games;
            team.batters[x].fielding[y].po += team2.batters[x].fielding[y].po;
            team.batters[x].fielding[y].dp += team2.batters[x].fielding[y].dp;
            team.batters[x].fielding[y].a += team2.batters[x].fielding[y].a;
            team.batters[x].fielding[y].pb += team2.batters[x].fielding[y].pb;
            team.batters[x].fielding[y].e += team2.batters[x].fielding[y].e;
        }
    }

    for (x = 0; x < 13; x++) {
        team.pitchers[x].id.starts_rest = 0;

        team.pitchers[x].id.ip_last4g[3] = team.pitchers[x].id.ip_last4g[2] = team.pitchers[x].id.ip_last4g[1] =
          team.pitchers[x].id.ip_last4g[0] = 0;

        team.pitchers[x].pitching.games += team2.pitchers[x].pitching.games;
        team.pitchers[x].pitching.games_started += team2.pitchers[x].pitching.games_started;
        team.pitchers[x].pitching.innings += team2.pitchers[x].pitching.innings;
        team.pitchers[x].pitching.thirds += team2.pitchers[x].pitching.thirds;

        if (team.pitchers[x].pitching.thirds > 2) {
            team.pitchers[x].pitching.thirds -= 3;
            team.pitchers[x].pitching.innings++;
        }

        team.pitchers[x].pitching.wins += team2.pitchers[x].pitching.wins;
        team.pitchers[x].pitching.losses += team2.pitchers[x].pitching.losses;
        team.pitchers[x].pitching.saves += team2.pitchers[x].pitching.saves;
        team.pitchers[x].pitching.bfp += team2.pitchers[x].pitching.bfp;
        team.pitchers[x].pitching.hits += team2.pitchers[x].pitching.hits;
        team.pitchers[x].pitching.doubles += team2.pitchers[x].pitching.doubles;
        team.pitchers[x].pitching.triples += team2.pitchers[x].pitching.triples;
        team.pitchers[x].pitching.homers += team2.pitchers[x].pitching.homers;
        team.pitchers[x].pitching.runs += team2.pitchers[x].pitching.runs;
        team.pitchers[x].pitching.er += team2.pitchers[x].pitching.er;
        team.pitchers[x].pitching.rbi += team2.pitchers[x].pitching.rbi;
        team.pitchers[x].pitching.cg += team2.pitchers[x].pitching.cg;
        team.pitchers[x].pitching.gf += team2.pitchers[x].pitching.gf;
        team.pitchers[x].pitching.sho += team2.pitchers[x].pitching.sho;
        team.pitchers[x].pitching.svopp += team2.pitchers[x].pitching.svopp;
        team.pitchers[x].pitching.sb += team2.pitchers[x].pitching.sb;
        team.pitchers[x].pitching.cs += team2.pitchers[x].pitching.cs;
        team.pitchers[x].pitching.walks += team2.pitchers[x].pitching.walks;
        team.pitchers[x].pitching.so += team2.pitchers[x].pitching.so;
        team.pitchers[x].pitching.ibb += team2.pitchers[x].pitching.ibb;
        team.pitchers[x].pitching.sh += team2.pitchers[x].pitching.sh;
        team.pitchers[x].pitching.sf += team2.pitchers[x].pitching.sf;
        team.pitchers[x].pitching.wp += team2.pitchers[x].pitching.wp;
        team.pitchers[x].pitching.balks += team2.pitchers[x].pitching.balks;
        team.pitchers[x].pitching.hb += team2.pitchers[x].pitching.hb;
        team.pitchers[x].pitching.opp_ab += team2.pitchers[x].pitching.opp_ab;
    }
}

/* convert a team ID to it's alphanumeric team name */
char *
GetTeamName (int ID) {
    int x;
    char *error = ("ERROR");

    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
        if (teaminfo[x].id == ID)
            return (&teaminfo[x].teamname[0]);

    /* will never get here unless there's a problem */
    return (error);
}

/* get complete team structure from client */
int
get_stats (int sock) {
    int x, y;

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
        return -1;

    team.id = atoi (&buffer[0]);
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.year = atoi (&buffer[0]);
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.league = buffer[0];
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.division = buffer[0];

    for (x = 0; x < 28; x++) {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        strcpy (&team.batters[x].id.name[0], &buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.teamid = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.batslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.throwslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.year = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.injury = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.starts_rest = atoi (&buffer[0]);
        for (y = 0; y < 4; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].id.ip_last4g[y] = atoi (&buffer[0]);
        }
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.inn_target = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.atbats = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.bb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.hbp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.gidp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.statsind = atoi (&buffer[0]);

        for (y = 0; y < 11; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].po = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].dp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].a = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].pb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].e = atoi (&buffer[0]);
        }
    }
    for (x = 0; x < 13; x++) {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        strcpy (&team.pitchers[x].id.name[0], &buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.teamid = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.batslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.throwslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.year = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.injury = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.starts_rest = atoi (&buffer[0]);
        for (y = 0; y < 4; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.pitchers[x].id.ip_last4g[y] = atoi (&buffer[0]);
        }
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.inn_target = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.games_started = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.innings = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.thirds = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.wins = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.losses = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.saves = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.bfp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.er = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.cg = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.gf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sho = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.svopp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.walks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.wp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.balks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.hb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.opp_ab = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.statsind = atoi (&buffer[0]);
    }

    return 0;
}

void
send_lineup () {
    int x, y;

    buffer1[0] = dhind + '0';
    buffer1[1] = '\0';
    for (x = 0; x < 2; x++)
        for (y = 0; y < 9; y++) {
            if (x)
                strcat (&buffer1[0], &home.batters[border[x][y].player[0]].id.name[0]);
            else
                strcat (&buffer1[0], &visitor.batters[border[x][y].player[0]].id.name[0]);

            strcat (&buffer1[0], "  ");
            strcat (&buffer1[0], cnvt_int2str (1, border[x][y].pos[0]));
            strcat (&buffer1[0], "  ");

            if (y == 8 && dhind == YES) {
                if (x)
                    strcat (&buffer1[0], &home.pitchers[pitching[x].pitcher[0]].id.name[0]);
                else
                    strcat (&buffer1[0], &visitor.pitchers[pitching[x].pitcher[0]].id.name[0]);
                strcat (&buffer1[0], "  ");
            }
        }

    strcat (&buffer1[0], "\n");
    sock_puts (sock, &buffer1[0]);
    if (netgame)
        sock_puts (netsock, &buffer1[0]);
}

void
send_DOB () {
    int x, y, plyrs[2];

    buffer1[0] = '\0';

    /* determine the number of players on the two teams */
    for (plyrs[0] = 0; plyrs[0] < 28; plyrs[0]++)
        if (home.batters[plyrs[0]].id.name[0] == ' ' || !strlen (&home.batters[plyrs[0]].id.name[0]))
            break;
    for (plyrs[1] = 0; plyrs[1] < 28; plyrs[1]++)
        if (visitor.batters[plyrs[1]].id.name[0] == ' ' || !strlen (&visitor.batters[plyrs[1]].id.name[0]))
            break;

    strcat (&buffer1[0], cnvt_int2str (2, plyrs[0]));
    strcat (&buffer1[0], cnvt_int2str (2, plyrs[1]));

    for (x = 0; x < 2; x++)
        for (y = 0; y < plyrs[x]; y++) {
            if (!x)
                strcat (&buffer1[0], &home.batters[y].id.name[0]);
            else
                strcat (&buffer1[0], &visitor.batters[y].id.name[0]);
            strcat (&buffer1[0], ":");
            if (!x) {
                strcat (&buffer1[0], cnvt_int2str (2, home.batters[y].dob.month));
                strcat (&buffer1[0], cnvt_int2str (2, home.batters[y].dob.day));
                strcat (&buffer1[0], cnvt_int2str (4, home.batters[y].dob.year));
            }
            else {
                strcat (&buffer1[0], cnvt_int2str (2, visitor.batters[y].dob.month));
                strcat (&buffer1[0], cnvt_int2str (2, visitor.batters[y].dob.day));
                strcat (&buffer1[0], cnvt_int2str (4, visitor.batters[y].dob.year));
            }
        }

    strcat (&buffer1[0], "\n");
    sock_puts (sock, &buffer1[0]);
    if (netgame)
        sock_puts (netsock, &buffer1[0]);
}

int
GetTotGames (int year, int teamid) {
    FILE *in;
    char fname[500], results[5000], tname[100], work[100], *pnt;
    int tgames, pos, thold1, thold2;

    strcpy (&fname[0], "/var/NSB/RealLifeStats/");
    strcat (&fname[0], (char *) cnvt_int2str (4, year));
    strcat (&fname[0], "/Results");
    if ((in = fopen (fname, "r")) != NULL) {
        fread (&results, sizeof results, 1, in);
        fclose (in);
    }

    /* get number of games team played during the regular season
       in 1981 each team appears twice in the Results file, so in the scan loop of the results area below don't break out of it
         when a team match is found and accumulate games rather than replacing them */

    /* get the team name */
    strcpy (&tname[0], (char *) GetTeamName (teamid));
    /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
    if (!strncmp (&tname[0], "St ", 3)) {
        strcpy (&work[0], &tname[0]);
        tname[2] = '.';
        tname[3] = ' ';
        strcpy (&tname[4], &work[3]);
    }

    for (tgames = pos = 0; pos < strlen (&results[0]); pos++) {
        if (!strncmp (&results[pos], "Post Season", strlen ("Post Season")))
            break;
        if (!strncmp (&results[pos], &tname[0], strlen (&tname[0]))) {
            /* find the first character 0 - 9, that will be the first position of team wins */
            for ( ; pos < strlen (&results[0]); pos++)
                if (results[pos] >= '0' && results[pos] <= '9')
                    break;
            pnt = index (&results[pos], ' ');
            *pnt = '\0';
            thold1 = atoi (&results[pos]);
            *pnt = ' ';
            /* get to first position of team losses */
            for ( ; results[pos] != ' '; pos++);
            for ( ; pos < strlen (&results[0]); pos++)
                if (results[pos] >= '0' && results[pos] <= '9')
                    break;
            pnt = index (&results[pos], ' ');
            *pnt = '\0';
            thold2 = atoi (&results[pos]);
            *pnt = ' ';
            tgames += (thold1 + thold2);
        }
    }

    return tgames;
}

