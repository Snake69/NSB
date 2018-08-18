/*
   the server plays the actual game here
*/

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"

int firsttime, vruns, hruns, tind, whereto, gameover, flip2p, altpo, ri, hi, ei, outhre, rcd = 0, JBsw;

char *grounders[] = { "taps the ball to", "grounds to", "hits a high bounder to", "hits a grounder to", "chops a bouncer to", "hits a roller to",
                      "smacks a hot shot to", "whacks a hard bouncing shot to", "hits a one-timer to", "grounds a little tapper to",
                      "hits a chopper towards", "bounces one to", "grounds weakly to", "grounds sharply to", "grounds softly to" },
     *airball1[] = { "pops up to", "smacks a liner to", "pops softly to" },
     *airball2[] = { "pops up to", "smacks a hot liner to", "foul pops to", "hits a line drive to", "pops softly to" },
     *airball3[] = { "pops to", "foul pops to", "pops softly to", "pops up and down the chimney to" },
     *airball4[] = { "hits a flyball to", "flies in foul ground to", "lines to", "hits a liner to", "lines out to", "flies out to",
                     "hits a can of corn to", "hits a big fly to", "lifts the ball to", "hits a long out to", "flies softly to" },
     *airball5[] = { "lines to", "hits a fly ball to", "hits a can of corn to", "hits a big fly to", "lifts the ball to", "hits a long out to" },
     *singles1[] = { "gets an infield hit to", "chops a slow roller to", "hits a squibbler to", "gets a scratch hit to" },
     *singles2[] = { "smacks the ball to", "hits a texas leaguer to", "hits a shot just over the infielder's head to", "hits a screamer to",
                     "grounds in the hole to", "rips one to", "bloops the ball to", "flares a single to", "ropes a single to", "punches the ball to",
                     "cracks the baseball to", "gets a base knock to", "drills the ball on a line to", "spanks the ball to", "loops the ball to",
                     "pokes the baseball to", "smokes it to", "hits a groundball with eyes to", "slaps the ball to", "slashes the ball to",
                     "strokes the ball to", "cracks a shot to", "muscles the ball to" },
     *doubles[] = { "hits a line shot in the gap", "hits a shot down the line", "hits the ball off the wall", "hits a rope ... it's a gapper",
                    "smacks the ball one bounce to the wall", "hits a ball that finds the alley", "shoots the gap", "drills the ball into the gap",
                    "whacks a ball that's one bounce and over the wall" },
                  /* that last value should remain last in the doubles[] list if more entries are added */
     *triples[] = { "hits a ball that goes all the way to the wall", "smacks a ball that gets past the outfielder" },
     *homers[] = { "hits a shot ... going ... going ... gone", "gets hold of one ... it's outa here",
                   "clobbers the ball ... back ... back ... back ... out of here", "slugs the ball over the fence",
                   "tears the cover off the ball ... gone", "smashes the ball out of the ballpark", "clouts the ball over the fence",
                   "gets hold of one ... out of the ballpark", "got all of that ... a homerun", "gives the ball a ride over the fence",
                   "hits the ball into souvenir city", "wallops the ball into the upper deck", "hits a moonshot into the stands",
                   "rips the ball ... Goodbye Mr. Spaulding", "goes yard", "goes deep ... gone", "unloads ... what a shot",
                   "drives the ball a country mile", "hammers the ball and it finds the seats", "pounds the cowhide ... See Ya",
                   "launches the ball out of the park", "makes good contact ... it's high ... it's far ... it's gone",
                   "hits a tape measure home run", "rips it ... kiss it goodbye", "plants it in the stands", "crushes it ... gone", "mashes a homer",
                   "nails the ball into the cheap seats", "blasts the ball over the fence", "gives the cowhide a ride ... it's not coming back" };

void
playthegame () {
    int field;

    /* for some reason a delay is necessary during gameplay
       reason to be determined ... hopefully */
    delay.tv_sec = 0;
    delay.tv_nsec = 1;

    pwin = ploss = saver = 99;
    ri = hi = ei = forfeit = savei = JBsw = 0;
    build_game_status ();

    action_ind = tind = firsttime = gameover = 1;

    while (action_ind) {
        clr_vars ();
        if (tind == 1 && gameover == 1) {
            if (!nosend)
                send_tophalf ();
            tind = 0;
        }
        if (!firsttime) {
            field = 1 - (game_status.half_inning % 2);
            if ((!field && vmanager) || (field && hmanager)) {
                humanaction ();
                if (!nosend)
                    send_tophalf ();
                if (!vmanager || !hmanager)
                    compai ();
            }
            else {
                compai ();
                if (!nosend)
                    send_tophalf ();
                if (vmanager || hmanager)
                    humanaction ();
            }
            if (!nosend)
                send_tophalf ();
        }
        action ();                   /* action_ind gets set in action () */
        if (!nosend)
            send_bothalf ();
    }
    accumulate_stats ();
    if (!nosend) {
        if (JBsw) {
            sock_puts (sock, "PB\n");
            if (netgame)
                sock_puts (netsock, "PB\n");
        }
        send_boxscore ();
    }
    if (abb)
        send_boxscore ();
}

/* build the initial game status record */
void
build_game_status () {
    int x;

    game_status.half_inning = game_status.outs = pre_act.outs = 0;
    for (x = 0; x < 100; x++)
        game_status.inning_score[x][0] = game_status.inning_score[x][1] = 0;
    for (x = 0; x < 3; x++)
        game_status.baserunners[x] = 99;
    for (x = 0; x < 16; x++)
        game_status.status[x] = 0;
    for (x = 0; x < 2; x++) {
        game_status.warming_up[x] = 99;
        game_status.batter[x] = 0;
        game_status.lob[x] = 0;
    }

    earned.stop = earned.outs = 0;
    for (x = 0; x < 3; x++)
        earned.br[x] = 0;
}

/* send variable values to client for top half of the game play display */
void
send_tophalf () {
    int inning, x, y, tot[2];
    char *statuses[] = { "holding runner on first", "infield in at corners", "infield in",
                         "outfield shallow", "third baseman playing the line", "first baseman playing the line",
                         "both first and third basemen playing the lines" };

    inning = game_status.half_inning / 2 + (game_status.half_inning % 2);

    if (inning >= 9 && (game_status.half_inning % 2) == 0 && vruns > hruns)
        return;
    if (inning >= 9 && (game_status.half_inning % 2) == 1 && vruns < hruns)
        return;

    /* data id */
    strcpy (&buffer[0], "TD");
    /* current half-inning */
    strcat (&buffer[0], (char *) cnvt_int2str (2, game_status.half_inning));
    /* runs per inning */
    for (x = 0; x <= (game_status.half_inning / 2); x++) {
        strcat (&buffer[0], (char *) cnvt_int2str (2, game_status.inning_score[x][0]));
        strcat (&buffer[0], (char *) cnvt_int2str (2, game_status.inning_score[x][1]));
    }

    /* total hits */
    for (x = tot[0] = tot[1] = 0; x < 25; x++) {
        if (x < maxplayers[0])
            tot[0] += visitor_cur.batters[x].hitting.hits;
        if (x < maxplayers[1])
            tot[1] += home_cur.batters[x].hitting.hits;
    }
    strcat (&buffer[0], (char *) cnvt_int2str (2, tot[0]));
    strcat (&buffer[0], (char *) cnvt_int2str (2, tot[1]));

    /* total errors */
    for (x = tot[0] = tot[1] = 0; x < 25; x++)
        for (y = 0; y < 11; y++) {
            if (x < maxplayers[0])
                tot[0] += visitor_cur.batters[x].fielding[y].e;
            if (x < maxplayers[1])
                tot[1] += home_cur.batters[x].fielding[y].e;
        }
    strcat (&buffer[0], (char *) cnvt_int2str (2, tot[0]));
    strcat (&buffer[0], (char *) cnvt_int2str (2, tot[1]));

    /* pitcher */
    if ((game_status.half_inning % 2) == 0)
        strcat (&buffer[0], &home_cur.pitchers[game_status.pitcher[1]].id.name[0]);
    else
        strcat (&buffer[0], &visitor_cur.pitchers[game_status.pitcher[0]].id.name[0]);
    strcat (&buffer[0], ":");

    /* pitcher birthdate */
    if ((game_status.half_inning % 2) == 0) {
        for (x = 0; x < 25; x++)
            if (!strcmp (&home_cur.pitchers[game_status.pitcher[1]].id.name[0], &home_cur.batters[x].id.name[0]))
                break;
        if (x < 25) {
            strcat (&buffer[0], (char *) cnvt_int2str (2, home_cur.batters[x].dob.month));
            strcat (&buffer[0], (char *) cnvt_int2str (2, home_cur.batters[x].dob.day));
            strcat (&buffer[0], (char *) cnvt_int2str (4, home_cur.batters[x].dob.year));
        }
        else
            strcat (&buffer[0], "00000000");
    }
    else {
        for (x = 0; x < 25; x++)
            if (!strcmp (&visitor_cur.pitchers[game_status.pitcher[0]].id.name[0], &visitor_cur.batters[x].id.name[0]))
                break;
        if (x < 25) {
            strcat (&buffer[0], (char *) cnvt_int2str (2, visitor_cur.batters[x].dob.month));
            strcat (&buffer[0], (char *) cnvt_int2str (2, visitor_cur.batters[x].dob.day));
            strcat (&buffer[0], (char *) cnvt_int2str (4, visitor_cur.batters[x].dob.year));
        }
        else
            strcat (&buffer[0], "00000000");
    }

    /* outs */
    strcat (&buffer[0], (char *) cnvt_int2str (1, game_status.outs));

    /* batter */
    if ((game_status.half_inning % 2) == 0)
        strcat (&buffer[0], &visitor_cur.batters[border[0][game_status.batter[0]].player[0]].id.name[0]);
    else
        strcat (&buffer[0], &home_cur.batters[border[1][game_status.batter[1]].player[0]].id.name[0]);
    strcat (&buffer[0], ":");

    /* batter birthdate */
    if ((game_status.half_inning % 2) == 0) {
        strcat (&buffer[0], (char *) cnvt_int2str (2, visitor_cur.batters[border[0][game_status.batter[0]].player[0]].dob.month));
        strcat (&buffer[0], (char *) cnvt_int2str (2, visitor_cur.batters[border[0][game_status.batter[0]].player[0]].dob.day));
        strcat (&buffer[0], (char *) cnvt_int2str (4, visitor_cur.batters[border[0][game_status.batter[0]].player[0]].dob.year));
    }
    else {
        strcat (&buffer[0], (char *) cnvt_int2str (2, home_cur.batters[border[1][game_status.batter[1]].player[0]].dob.month));
        strcat (&buffer[0], (char *) cnvt_int2str (2, home_cur.batters[border[1][game_status.batter[1]].player[0]].dob.day));
        strcat (&buffer[0], (char *) cnvt_int2str (4, home_cur.batters[border[1][game_status.batter[1]].player[0]].dob.year));
    }

    /* baserunners */
    for (x = 0; x < 3; x++) {
        if (game_status.baserunners[x] != 99) {
            if ((game_status.half_inning % 2) == 0)
                strcat (&buffer[0], &visitor_cur.batters[border[0][game_status.baserunners[x]].player[0]].id.name[0]);
            else
                strcat (&buffer[0], &home_cur.batters[border[1][game_status.baserunners[x]].player[0]].id.name[0]);
        }
        strcat (&buffer[0], ":");
    }

    /* on deck */
    if ((game_status.half_inning % 2) == 0) {
        if (game_status.batter[0] == 8)
            x = 0;
        else
            x = game_status.batter[0] + 1;
        strcat (&buffer[0], &visitor_cur.batters[border[0][x].player[0]].id.name[0]);
    }
    else {
        if (game_status.batter[1] == 8)
            x = 0;
        else
            x = game_status.batter[1] + 1;
        strcat (&buffer[0], &home_cur.batters[border[1][x].player[0]].id.name[0]);
    }
    strcat (&buffer[0], ":");

    /* in the hole */
    if ((game_status.half_inning % 2) == 0) {
        x = game_status.batter[0] + 2;
        if (x > 8)
            x -= 9;
        strcat (&buffer[0], &visitor_cur.batters[border[0][x].player[0]].id.name[0]);
    }
    else {
        x = game_status.batter[1] + 2;
        if (x > 8)
            x -= 9;
        strcat (&buffer[0], &home_cur.batters[border[1][x].player[0]].id.name[0]);
    }
    strcat (&buffer[0], ":");

    /* status messages
       we want to make sure to pass 4 and only 4 colons */
    if (game_status.status[0])
        strcat (&buffer[0], statuses[0]);
    strcat (&buffer[0], ":");
    if (game_status.status[2])
        strcat (&buffer[0], statuses[game_status.status[2]]);
    strcat (&buffer[0], ":");
    if (game_status.status[3])
        strcat (&buffer[0], statuses[3]);
    strcat (&buffer[0], ":");
    if (game_status.status[6])
        strcat (&buffer[0], statuses[game_status.status[6] + 3]);
    strcat (&buffer[0], ":");

    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);
    if (netgame)
        sock_puts (netsock, &buffer[0]);
}

/* perform an action

   action_ind gets set to:
   0  game over
   1  play ball (beginning of game)
   2  single (runners advance 1 base)
   3  double (runners advance 2 bases)
   4  triple
   5  homer
   6  walk
   7  strikeout
   8  hit by pitch
   9  ground out
  10  fly out
  11  ground into dp (if less than 2 outs and a runner on firstbase)
  12  sacrifice fly (if less than 2 outs and a runner on third)
  13  single (runners advance 2 bases)
  14  double (runners advance 3 bases)
  15  wild pitch
  16  balk
  17  passed ball
  18  error
  19  successful sacrifice or squeeze
  20  successful pickoff attempt
  21  stolen base(s)
  22  caught stealing
  23  flyout, runner on third is out trying to score
*/
void
action () {
    int x, r, pa, singles, doubles, triples, homers, bb, hbp, so, groundout, ha, assists,
        flyout, gidp, sf, result, innings, totatt, failed, y, pos2, w2b, w3b;
    float tiredness;

    if ((game_status.half_inning % 2) == 0)
        ha = 1;
    else
        ha = 0;

    for (y = 0; y < 9; y++)
        if (border[ha][y].pos[0] == 2) {
            pos2 = border[ha][y].player[0];
            break;
        }

    action_ind = 99;

    innings = (game_status.half_inning - 1) / 2 + 1;
    for (vruns = hruns = x = 0; x < innings; x++) {
        vruns += game_status.inning_score[x][0];
        hruns += game_status.inning_score[x][1];
    }

    if (!gotateam || forfeit == -1)
        gameover = 0;
    if (innings >= 9 && (game_status.half_inning % 2) == 0 && vruns > hruns)
        gameover = 0;
    if (innings >= 9 && (game_status.half_inning % 2) == 1 && vruns < hruns)
        gameover = 0;

    if (gameover == 0) {
        action_ind = 0;
        update_stats ();
        if (forfeit != -1)
            if (vruns > hruns)
                winners = 'v';
            else
                winners = 'h';
        else {
            if (tforfeit) {
                winners = 'v';
                if (syslog_ent == YES)
                    syslog (LOG_INFO, "Team %d %s forfeited a game", home.year, (char *) GetTeamName (home.id));
            }
            else {
                winners = 'h';
                if (syslog_ent == YES)
                    syslog (LOG_INFO, "Team %d %s forfeited a game", visitor.year, (char *) GetTeamName (visitor.id));
            }
            /* change a couple of statuses of some players even though the game was forfeited */
            for (x = 0; x < 25; x++) {
                if (visitor_season.batters[x].id.injury)
                    visitor_season.batters[x].id.injury--;
                if (home_season.batters[x].id.injury)
                    home_season.batters[x].id.injury--;
            }
            for (x = 0; x < 11; x++) {
                home_season.pitchers[x].id.starts_rest++;
                visitor_season.pitchers[x].id.starts_rest++;
            }
        }
        if (!gotateam) {
            if (tforfeit)
                winners = 'v';
            else
                winners = 'h';
            /* change a couple of statuses of some players even though the we didn't play the game */
            for (x = 0; x < 25; x++) {
                if (visitor_season.batters[x].id.injury)
                    visitor_season.batters[x].id.injury--;
                if (home_season.batters[x].id.injury)
                    home_season.batters[x].id.injury--;
            }
            for (x = 0; x < 11; x++) {
                home_season.pitchers[x].id.starts_rest++;
                visitor_season.pitchers[x].id.starts_rest++;
            }
        }
        return;
    }
    if (game_status.half_inning == 0 && game_status.batter[0] == 0 && firsttime == 1) {
        action_ind = 1;
        firsttime = 0;
        update_stats ();
        return;
    }

    /* check for an intentional walk */
    if (game_status.status[4]) {
        action_ind = 6;
        update_stats ();
        update_status ();
        return;
    }

    /* check for a pickoff attempt */
    if (game_status.status[1]) {
        /* a pickoff attempt supercedes certain offensive actions */
        game_status.status[7] = game_status.status[8] = game_status.status[9] = game_status.status[10] = 0;
        if (!(int) ((float) 50 * rand () / (RAND_MAX + 1.0))) {
            action_ind = 20;
            hldbase = game_status.status[1];
            update_stats ();
            upd_fielding_stats ();
            update_status ();
            return;
        }
        check_for_errors ();
        if (action_ind == 18) {
            upd_fielding_stats ();
            update_status ();
        }
        else
            action_ind = 99;
        return;
    }

    /* check for hit and run */
    if (game_status.status[8]) {
        if (game_status.status[5]) {
            /* if the defense pitched out then a hit and run becomes a stolen base attempt */
            game_status.status[13]++;  /* hitter always swings and misses when there's a pitchout (or at least he should :) */
            game_status.status[7] = game_status.status[5] = 0;
            if (game_status.baserunners[2] != 99)
                game_status.status[7] = 4;
            if (game_status.baserunners[1] != 99)
                game_status.status[7] += 3;
            if (game_status.baserunners[0] != 99)
                game_status.status[7] += 2;
            goto sb_attempt;
        }

        if ((game_status.half_inning % 2) == 0) {
            totatt = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats;
            if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.so == -1)
                /* strikeouts for the batter wasn't always a recorded stat */
                failed = (home.pitchers[game_status.pitcher[1]].pitching.so / 2) / 10;
            else
                failed = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.so / 10;
        }
        else {
            totatt = home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats;
            if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.so == -1)
                /* strikeouts for the batter wasn't always a recorded stat */
                failed = (visitor.pitchers[game_status.pitcher[0]].pitching.so / 2) / 10;
            else
                failed = home.batters[border[1][game_status.batter[1]].player[0]].hitting.so / 10;
        }
        if ((int) ((float) totatt * rand () / (RAND_MAX + 1.0)) < failed) {
            game_status.status[13]++;
            if (game_status.baserunners[1] != 99)
                game_status.status[7] = 3;
            else
                game_status.status[7] = 2;
        }
    }
    else
        if (game_status.status[5])
            game_status.status[14]++;
sb_attempt:
    /* check for an attempted steal */
    if (game_status.status[7]) {
        int wbr;
        
        if (game_status.status[7] == 2)
            wbr = 0;
        if (game_status.status[7] == 3 || game_status.status[7] == 5)
            wbr = 1;
        if (game_status.status[7] == 4 || game_status.status[7] == 6 || game_status.status[7] == 7 || game_status.status[7] == 9)
            wbr = 2;

        if ((game_status.half_inning % 2) == 0) {
            totatt = visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb;
            if (home.pitchers[game_status.pitcher[1]].pitching.sb == -1)
                /* stolen bases against for the pitcher wasn't always a recorded stat */
                totatt *= 2;
            else
                totatt += home.pitchers[game_status.pitcher[1]].pitching.sb;
            if (home.pitchers[game_status.pitcher[1]].pitching.cs == -1) {
                /* caught stealing against for the pitcher wasn't always a recorded stat */
                totatt += (totatt / 5);
                failed = totatt / 5;
            }
            else {
                totatt += home.pitchers[game_status.pitcher[1]].pitching.cs;
                failed = home.pitchers[game_status.pitcher[1]].pitching.cs;
            }
            if (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.cs == -1) {
                /* caught stealing for the batter wasn't always a recorded stat */
                if (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb > 99) {
                    totatt += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 5);
                    failed += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 5);
                }
                else
                    if (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb > 49) {
                        totatt += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 3);
                        failed += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 3);
                    }
                    else {
                        totatt += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 2);
                        failed += (visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.sb / 2);
                    }
            }
            else {
                totatt += visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.cs;
                failed += visitor.batters[border[0][game_status.baserunners[wbr]].player[0]].hitting.cs;
            }
        }
        else {
            totatt = home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb;
            if (visitor.pitchers[game_status.pitcher[0]].pitching.sb == -1)
                /* stolen bases against for the pitcher wasn't always a recorded stat */
                totatt *= 2;
            else
                totatt += visitor.pitchers[game_status.pitcher[0]].pitching.sb;
            if (visitor.pitchers[game_status.pitcher[0]].pitching.cs == -1) {
                /* caught stealing against for the pitcher wasn't always a recorded stat */
                totatt += (totatt / 5);
                failed = totatt / 5;
            }
            else {
                totatt += visitor.pitchers[game_status.pitcher[0]].pitching.cs;
                failed = visitor.pitchers[game_status.pitcher[0]].pitching.cs;
            }
            if (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.cs == -1) {
                /* caught stealing for the batter wasn't always a recorded stat */
                if (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb > 99) {
                    totatt += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 5);
                    failed += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 5);
                }
                else
                    if (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb > 99) {
                        totatt += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 3);
                        failed += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 3);
                    }
                    else {
                        totatt += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 2);
                        failed += (home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.sb / 2);
                    }
            }
            else {
                totatt += home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.cs;
                failed += home.batters[border[1][game_status.baserunners[wbr]].player[0]].hitting.cs;
            }
        }
        if (totatt < 1)
            totatt = 1;
        if (failed < 1)
            failed = 1;
        x = ((float) failed / (float) totatt) * 100.0;
        if (totatt < 10 && x < 50)
            x = 50;
        if (game_status.status[7] == 2 && !game_status.status[0])
            /* if the runner at first is not being held on improve his chances */
            x /= 2;
        if (game_status.status[7] == 3 || game_status.status[7] == 5)
            /* it's a little harder to steal third than second */
            x += (5 * (totatt / failed));
        if (game_status.status[7] == 4 || game_status.status[7] == 6 || game_status.status[7] == 7 || game_status.status[7] == 9)
            /* it's a lot harder to steal home */
            x += (15 * (totatt / failed));

        if (game_status.status[8])
            /* hit & run - make it a little harder */
            x += (2 * (totatt / failed));
        if (game_status.status[5])
            /* pitchout - make it harder */
            x += (10 * (totatt / failed));
        if (x > 99)
            x = 95;   /* there should always be a chance of being successfull */
        if ((int) ((float) 100 * rand () / (RAND_MAX + 1.0)) < x)
            action_ind = 22;
        else
            action_ind = 21;

        update_stats ();
        upd_fielding_stats ();
        update_status ();
        return;
    }

    /* check for a sacrifice */
    if (game_status.status[9] || game_status.status[10]) {
        if (game_status.status[5]) {
            /* if the defense pitched out then a sacrifice/squeeze play becomes a stolen base attempt */
            game_status.status[7] = game_status.status[9] = game_status.status[10] = 0;
            if (game_status.baserunners[2] != 99)
                game_status.status[7] = 4;
            if (game_status.baserunners[1] != 99)
                game_status.status[7] += 3;
            if (game_status.baserunners[0] != 99)
                game_status.status[7] += 2;
            goto sb_attempt;
        }

        if ((game_status.half_inning % 2) == 0) {
            totatt = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats;
            /* failed is really successful in this case */
            failed = (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh + 1) * 75;
        }
        else {
            totatt = home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats;
            /* failed is really successful in this case */
            failed = (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh + 1) * 75;
        }
        if (game_status.status[9] && game_status.baserunners[1] != 99)
            failed -= (failed * .1);
        if (game_status.status[10])
            failed /= 4;
        if (game_status.status[2] == 1)
            failed /= 2;
        if (game_status.status[2] == 2)
            failed /= 2.2;

        if (failed > totatt)
            /* they gotta screw up sometimes */
            totatt = failed + (failed * .05);
        if (game_status.status[9] && game_status.baserunners[1] == 99)
            /* to sac bunt a runner to 2nd should never be worse than 50/50 */
            if (failed < (totatt / 2))
                failed = totatt / 2;
        if ((int) ((float) totatt * rand () / (RAND_MAX + 1.0)) < failed)
            action_ind = 19;
        else
            action_ind = 9;
        goto update_stuff;
    }

    /* check for too many swinging misses by the batter */
    if (game_status.status[13] > 1) {
        action_ind = 7;
        goto update_stuff;
    }

    /* check for too many pitchouts */
    if (game_status.status[14] > 2) {
        action_ind = 6;
        goto update_stuff;
    }

    /* calculate tiredness of pitcher */
    if ((game_status.half_inning % 2) == 0) {
        x = home.pitchers[game_status.pitcher[1]].id.inn_target;
        y = home_cur.pitchers[game_status.pitcher[1]].pitching.innings +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[0] / 2) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[1] / 3) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[2] / 4) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[3] / 5);
    }
    else {
        x = visitor.pitchers[game_status.pitcher[0]].id.inn_target;
        y = visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[0] / 2) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[1] / 3) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[2] / 4) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[3] / 5);
    }
    if (y > (x + 5))
        tiredness = 0.2;
    else
        if (y > (x + 3))
            tiredness = 0.15;
        else
            if (y > (x + 2))
                tiredness = 0.1;
            else
                if (y > x)
                    tiredness = 0.05;
                else
                    tiredness = 0.0;   /* not tired */

    /* check for a wild pitch, a passed ball, or a balk */
    if (game_status.baserunners[0] != 99 || game_status.baserunners[1] != 99 || game_status.baserunners[2] != 99) {
        if ((game_status.half_inning % 2) == 0) {
            result = (int) ((float) ((home.pitchers[game_status.pitcher[1]].pitching.innings * 33) +
                                     (home.pitchers[game_status.pitcher[1]].pitching.thirds * 11)) * rand () / (RAND_MAX + 1.0));
            if (result < (home.pitchers[game_status.pitcher[1]].pitching.wp * 11 +
                         (home.pitchers[game_status.pitcher[1]].pitching.wp * 11 * tiredness)))
                action_ind = 15;
            else
                if (result < (home.pitchers[game_status.pitcher[1]].pitching.balks * 15 +
                             (home.pitchers[game_status.pitcher[1]].pitching.balks * 15 * tiredness)))
                    action_ind = 16;
                else {
                    result = (int) ((float) ((home.batters[pos2].fielding[2].a +
                                              home.batters[pos2].fielding[2].po) * 3) * rand () / (RAND_MAX + 1.0));
                    if (result < (home.batters[pos2].fielding[2].pb / 1.5))
                        action_ind = 17;
                }
        }
        else {
            result = (int) ((float) ((visitor.pitchers[game_status.pitcher[0]].pitching.innings * 33) +
              (visitor.pitchers[game_status.pitcher[0]].pitching.thirds * 11)) * rand () / (RAND_MAX + 1.0));
            if (result < (visitor.pitchers[game_status.pitcher[0]].pitching.wp * 11 +
                         (visitor.pitchers[game_status.pitcher[0]].pitching.wp * 11 * tiredness)))
                action_ind = 15;
            else
                if (result < (visitor.pitchers[game_status.pitcher[0]].pitching.balks * 15 +
                             (visitor.pitchers[game_status.pitcher[0]].pitching.balks * 15 * tiredness)))
                    action_ind = 16;
                else {
                    result = (int) ((float) ((visitor.batters[pos2].fielding[2].a +
                      visitor.batters[pos2].fielding[2].po) * 3) * rand () / (RAND_MAX + 1.0));
                    if (result < (visitor.batters[pos2].fielding[2].pb / 1.5))
                        action_ind = 17;
                }
        }
    }
    if (action_ind > 14 && action_ind < 18) {
        find_fielder ();
        check_for_errors ();
        update_stats ();
        upd_fielding_stats ();
        update_status ();
        return;
    }

    /* at this point if there was a pitchout get the next defensive decision */
    if (game_status.status[5]) {
        action_ind = 99;
        return;
    }

    if ((game_status.half_inning % 2) == 0) {
        if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sf == -1)
            /* sacrifice flies wasn't always individually recorded ... in this case the sacrifice hits stat combines
               sacrifice flies and sacrifice hits */
            pa = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.bb +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hbp;
        else
            pa = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.bb +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hbp +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh +
                 visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sf;
        if (home.pitchers[game_status.pitcher[1]].pitching.bfp == -1)
            /* batters facing pitcher wasn't always a recorded stat */
            pa += ((home.pitchers[game_status.pitcher[1]].pitching.innings * 3) +
                          home.pitchers[game_status.pitcher[1]].pitching.thirds +
                            home.pitchers[game_status.pitcher[1]].pitching.hits +
                           home.pitchers[game_status.pitcher[1]].pitching.walks +
                                home.pitchers[game_status.pitcher[1]].pitching.hb);
        else
            pa += home.pitchers[game_status.pitcher[1]].pitching.bfp;

        if (border[0][game_status.batter[0]].pos[0] == 1)
            pa *= 2;

        singles = visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hits -
         (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles +
          visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.triples +
          visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers);
        if (home.pitchers[game_status.pitcher[1]].pitching.doubles == -1) {
            /* doubles allowed and triples allowed weren't always recorded stats */
            singles += (home.pitchers[game_status.pitcher[1]].pitching.hits -
                         ((home.pitchers[game_status.pitcher[1]].pitching.homers * 2) + /* best guess for doubles */
                         (home.pitchers[game_status.pitcher[1]].pitching.homers / 5) + /* best guess for triples */
                         home.pitchers[game_status.pitcher[1]].pitching.homers));
            singles += ((home.pitchers[game_status.pitcher[1]].pitching.hits -
                         ((home.pitchers[game_status.pitcher[1]].pitching.homers * 2) +
                         (home.pitchers[game_status.pitcher[1]].pitching.homers / 5) +
                         home.pitchers[game_status.pitcher[1]].pitching.homers)) * tiredness);
        }
        else {
            singles += (home.pitchers[game_status.pitcher[1]].pitching.hits -
                    (home.pitchers[game_status.pitcher[1]].pitching.doubles +
                     home.pitchers[game_status.pitcher[1]].pitching.triples +
                        home.pitchers[game_status.pitcher[1]].pitching.homers));
            singles += ((home.pitchers[game_status.pitcher[1]].pitching.hits -
                     (home.pitchers[game_status.pitcher[1]].pitching.doubles +
                      home.pitchers[game_status.pitcher[1]].pitching.triples +
                        home.pitchers[game_status.pitcher[1]].pitching.homers)) * tiredness);
        }

        if (game_status.status[6]) {    /* is the defense playing the line? */
            singles += (singles * .1);
            if (game_status.status[6] == 3)  /* guarding both lines */
                singles += (singles * .08);
        }
        if (game_status.status[0])    /* is the runner on first being held? */
            singles += (singles * .05);
        if (game_status.status[2] == 1) /* is the infield in at the corners? */
            singles += (singles * .05);
        if (game_status.status[2] == 2)    /* is the entire infield in? */
            singles += (singles * .1);
        if (game_status.status[3])    /* is the outfield playing in? */
            singles -= (singles * .05);

        if (home.pitchers[game_status.pitcher[1]].pitching.doubles == -1) {
            /* doubles given up by pitcher wasn't always a recorded stat */
            w2b = home.pitchers[game_status.pitcher[1]].pitching.innings / 8;
            w2b += (home.pitchers[game_status.pitcher[1]].pitching.innings / 8 * tiredness);
        }
        else {
            w2b = home.pitchers[game_status.pitcher[1]].pitching.doubles;
            w2b += (home.pitchers[game_status.pitcher[1]].pitching.doubles * tiredness);
        }
        if (home.pitchers[game_status.pitcher[1]].pitching.triples == -1) {
            /* triples given up by pitcher wasn't always a recorded stat */
            w3b = home.pitchers[game_status.pitcher[1]].pitching.innings / 32;
            w3b += (home.pitchers[game_status.pitcher[1]].pitching.innings / 32 * tiredness);
        }
        else {
            w3b = home.pitchers[game_status.pitcher[1]].pitching.triples;
            w3b += (home.pitchers[game_status.pitcher[1]].pitching.triples * tiredness);
        }

        doubles = singles + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles + w2b;
        if (game_status.status[6]) {    /* is the defense playing the line? */
            doubles -= ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles + w2b) * .05);
            if (game_status.status[6] == 3)  /* guarding both lines */
                doubles -= ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles + w2b) * .03);
        }
        if (game_status.status[3])    /* is the outfield playing in? */
            doubles += ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles + w2b) * .05);

        triples = doubles + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.triples + w3b;
        if (game_status.status[6]) {    /* is the defense playing the line? */
            triples -= ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.triples + w3b) * .05);
            if (game_status.status[6] == 3)  /* guarding both lines */
                triples -= ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.triples + w3b) * .03);
        }
        if (game_status.status[3])    /* is the outfield playing in? */
            triples += ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.triples + w3b) * .05);

        homers = triples + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers +
                 home.pitchers[game_status.pitcher[1]].pitching.homers +
                 (home.pitchers[game_status.pitcher[1]].pitching.homers * tiredness);
        if (game_status.status[3])    /* is the outfield playing in? */
            homers += ((visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers +
                    home.pitchers[game_status.pitcher[1]].pitching.homers) * .05);

        bb = homers + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.bb +
             home.pitchers[game_status.pitcher[1]].pitching.walks +
             (home.pitchers[game_status.pitcher[1]].pitching.walks * tiredness);
        if (game_status.status[14])    /* has there been pitchouts */
            bb += (bb * (.25 * game_status.status[14]));
        if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.so == -1)
            /* strike outs for the batter wasn't always a recorded stat */
            so = bb + (home.pitchers[game_status.pitcher[1]].pitching.so * 2);
        else
            so = bb + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.so +
                 home.pitchers[game_status.pitcher[1]].pitching.so;
        if (game_status.status[13])    /* has there been swinging misses */
            so += (so * (.3 * game_status.status[13]));
        hbp = so + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.hbp +
              home.pitchers[game_status.pitcher[1]].pitching.hb +
              (home.pitchers[game_status.pitcher[1]].pitching.hb * tiredness);
        if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.gidp == -1)
            /* ground into DP for the batter wasn't always a recorded stat */
            if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sb > 20)
                gidp = hbp + 80;
            else
                gidp = hbp + 180;
        else
            gidp = hbp + (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.gidp * 10);
        if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sf == -1)
            /* sacrifice flies wasn't always individually recorded ... in this case the sacrifice hits stat combines
               sacrifice flies and sacrifice hits */
            if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers < 20)
                sf = gidp + (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh / 2);
            else
                if (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.homers > 29)
                    sf = gidp + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh;
                else
                    sf = gidp + (visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sh * 0.8);
        else
            sf = gidp + visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.sf;
        if (home.pitchers[game_status.pitcher[1]].pitching.sf == -1) {
            /* sacrifice flies given up by the pitcher wasn't always a recorded stat */
            x = home.pitchers[game_status.pitcher[1]].pitching.innings / 28 +
                (home.pitchers[game_status.pitcher[1]].pitching.innings / 28 * tiredness);
            if (!x)
                x = 1;
            sf += (x * 30);
        }
        else
            sf += ((home.pitchers[game_status.pitcher[1]].pitching.sf * 30) +
                  (home.pitchers[game_status.pitcher[1]].pitching.sf * 30 * tiredness));
    }
    else {
        if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sf == -1)
            /* sacrifice flies wasn't always individually recorded ... in this case the sacrifice hits stat combines
               sacrifice flies and sacrifice hits */
            pa = home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.bb +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.hbp;
        else
            pa = home.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.bb +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.hbp +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh +
                 home.batters[border[1][game_status.batter[1]].player[0]].hitting.sf;
        if (visitor.pitchers[game_status.pitcher[0]].pitching.bfp == -1)
            /* batters facing pitcher wasn't always a recorded stat */
            pa += ((visitor.pitchers[game_status.pitcher[0]].pitching.innings * 3) +
                          visitor.pitchers[game_status.pitcher[0]].pitching.thirds +
                            visitor.pitchers[game_status.pitcher[0]].pitching.hits +
                           visitor.pitchers[game_status.pitcher[0]].pitching.walks +
                                visitor.pitchers[game_status.pitcher[0]].pitching.hb);
        else
            pa += visitor.pitchers[game_status.pitcher[0]].pitching.bfp;

        if (border[1][game_status.batter[1]].pos[0] == 1)
            pa *= 2;

        singles = home.batters[border[1][game_status.batter[1]].player[0]].hitting.hits -
         (home.batters[border[1][game_status.batter[1]].player[0]].hitting.doubles +
          home.batters[border[1][game_status.batter[1]].player[0]].hitting.triples +
          home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers);
        if (visitor.pitchers[game_status.pitcher[0]].pitching.doubles == -1) {
            /* doubles allowed and triples allowed weren't always recorded stats */
            singles += (visitor.pitchers[game_status.pitcher[0]].pitching.hits -
                         ((visitor.pitchers[game_status.pitcher[0]].pitching.homers * 2) + /* best guess for doubles */
                         (visitor.pitchers[game_status.pitcher[0]].pitching.homers / 5) + /* best guess for triples */
                         visitor.pitchers[game_status.pitcher[0]].pitching.homers));
            singles += ((visitor.pitchers[game_status.pitcher[0]].pitching.hits -
                         ((visitor.pitchers[game_status.pitcher[0]].pitching.homers * 2) +
                         (visitor.pitchers[game_status.pitcher[0]].pitching.homers / 5) +
                         visitor.pitchers[game_status.pitcher[0]].pitching.homers)) * tiredness);
        }
        else {
            singles += (visitor.pitchers[game_status.pitcher[0]].pitching.hits -
                    (visitor.pitchers[game_status.pitcher[0]].pitching.doubles +
                     visitor.pitchers[game_status.pitcher[0]].pitching.triples +
                        visitor.pitchers[game_status.pitcher[0]].pitching.homers));
            singles += ((visitor.pitchers[game_status.pitcher[0]].pitching.hits -
                    (visitor.pitchers[game_status.pitcher[0]].pitching.doubles +
                     visitor.pitchers[game_status.pitcher[0]].pitching.triples +
                        visitor.pitchers[game_status.pitcher[0]].pitching.homers)) * tiredness);
        }

        if (game_status.status[6]) {    /* is the defense playing the line? */
            singles += (singles * .1);
            if (game_status.status[6] == 3)  /* guarding both lines */
                singles += (singles * .08);
        }
        if (game_status.status[0])    /* is the runner on first being held? */
            singles += (singles * .05);
        if (game_status.status[2] == 1) /* is the infield in at the corners? */
            singles += (singles * .05);
        if (game_status.status[2] == 2)    /* is the entire infield in? */
            singles += (singles * .1);
        if (game_status.status[3])    /* is the outfield playing in? */
            singles -= (singles * .05);

        if (visitor.pitchers[game_status.pitcher[0]].pitching.doubles == -1) {
            /* doubles given up by pitcher wasn't always a recorded stat */
            w2b = visitor.pitchers[game_status.pitcher[0]].pitching.innings / 8;
            w2b += (visitor.pitchers[game_status.pitcher[0]].pitching.innings / 8 * tiredness);
        }
        else {
            w2b = visitor.pitchers[game_status.pitcher[0]].pitching.doubles;
            w2b += (visitor.pitchers[game_status.pitcher[0]].pitching.doubles * tiredness);
        }
        if (visitor.pitchers[game_status.pitcher[0]].pitching.triples == -1) {
            /* triples given up by pitcher wasn't always a recorded stat */
            w3b = visitor.pitchers[game_status.pitcher[0]].pitching.innings / 32;
            w3b += (visitor.pitchers[game_status.pitcher[0]].pitching.innings / 32 * tiredness);
        }
        else {
            w3b = visitor.pitchers[game_status.pitcher[0]].pitching.triples;
            w3b += (visitor.pitchers[game_status.pitcher[0]].pitching.triples * tiredness);
        }

        doubles = singles + home.batters[border[1][game_status.batter[1]].player[0]].hitting.doubles + w2b;
        if (game_status.status[6]) {    /* is the defense playing the line? */
            doubles -= ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.doubles + w2b) * .05);
            if (game_status.status[6] == 3)  /* guarding both lines */
                doubles -= ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.doubles + w2b) * .03);
        }
        if (game_status.status[3])    /* is the outfield playing in? */
            doubles += ((home.batters[border[0][game_status.batter[1]].player[1]].hitting.doubles + w2b) * .05);

        triples = doubles + home.batters[border[1][game_status.batter[1]].player[0]].hitting.triples + w3b;
        if (game_status.status[6]) {   /* is the defense playing the line? */
            triples -= ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.triples + w3b) * .05);
            if (game_status.status[6] == 3)  /* guarding both lines */
                triples -= ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.triples + w3b) * .03);
        }
        if (game_status.status[3])    /* is the outfield playing in? */
            triples += ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.triples + w3b) * .05);

        homers = triples + home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers +
                 visitor.pitchers[game_status.pitcher[0]].pitching.homers +
                 (visitor.pitchers[game_status.pitcher[0]].pitching.homers * tiredness);
        if (game_status.status[3])    /* is the outfield playing in? */
            homers += ((home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers +
                       visitor.pitchers[game_status.pitcher[0]].pitching.homers) * .05);
        bb = homers + home.batters[border[1][game_status.batter[1]].player[0]].hitting.bb +
             visitor.pitchers[game_status.pitcher[0]].pitching.walks +
             (visitor.pitchers[game_status.pitcher[0]].pitching.walks * tiredness);
        if (game_status.status[14])    /* has there been pitchouts */
            bb += (bb * (.25 * game_status.status[14]));
        if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.so == -1)
            /* strike outs for the batter wasn't always a recorded stat */
            so = bb + (visitor.pitchers[game_status.pitcher[0]].pitching.so * 2);
        else
            so = bb + home.batters[border[1][game_status.batter[1]].player[0]].hitting.so +
                 visitor.pitchers[game_status.pitcher[0]].pitching.so;
        if (game_status.status[13])    /* has there been swinging misses */
            so += (so * (.3 * game_status.status[13]));
        hbp = so + home.batters[border[1][game_status.batter[1]].player[0]].hitting.hbp +
              visitor.pitchers[game_status.pitcher[0]].pitching.hb +
              (visitor.pitchers[game_status.pitcher[0]].pitching.hb * tiredness);
        if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.gidp == -1)
            /* ground into DP for the batter wasn't always a recorded stat */
            if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sb > 20)
                gidp = hbp + 80;
            else
                gidp = hbp + 180;
        else
            gidp = hbp + (home.batters[border[1][game_status.batter[1]].player[0]].hitting.gidp * 10);
        if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sf == -1)
            /* sacrifice flies wasn't always individually recorded ... in this case the sacrifice hits stat combines
               sacrifice flies and sacrifice hits */
            if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers < 20)
                sf = gidp + (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh / 2);
            else
                if (home.batters[border[1][game_status.batter[1]].player[0]].hitting.homers > 29)
                    sf = gidp + home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh;
                else
                    sf = gidp + (home.batters[border[1][game_status.batter[1]].player[0]].hitting.sh * 0.8);
        else
            sf = gidp + home.batters[border[1][game_status.batter[1]].player[0]].hitting.sf;
        if (visitor.pitchers[game_status.pitcher[0]].pitching.sf == -1) {
            /* sacrifice flies given up by the pitcher wasn't always a recorded stat */
            x = visitor.pitchers[game_status.pitcher[0]].pitching.innings / 28 +
                (visitor.pitchers[game_status.pitcher[0]].pitching.innings / 28 * tiredness);
            if (!x)
                x = 1;
            sf += (x * 30);
        }
        else
            sf += ((visitor.pitchers[game_status.pitcher[0]].pitching.sf * 30) +
                   (visitor.pitchers[game_status.pitcher[0]].pitching.sf * 30 * tiredness));
    }
    groundout = sf + (pa - sf) / 2;
    flyout = pa;

    result = (int) ((float) pa * rand () / (RAND_MAX + 1.0));
    if (result <= singles)
        action_ind = 2;
    if (result > singles && result <= doubles)
        action_ind = 3;
    if (result > doubles && result <= triples)
        action_ind = 4;
    if (result > triples && result <= homers)
        action_ind = 5;
    if (result > homers && result <= bb) {
        if (game_status.status[8])
            action_ind = 9;
        else
            action_ind = 6;
    }
    if (result > bb && result <= so)
        action_ind = 7;
    if (result > so && result <= hbp)
        action_ind = 8;
    if (result > hbp && result <= gidp) {
        if (game_status.status[8]) {
            action_ind = 9;
            goto adjust_action;
        }
        else
            action_ind = 11;
        if (game_status.status[2] == 2)
            action_ind = 9;
        else
            if (game_status.status[2] == 1) {
                if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0)))
                    action_ind = 11;
                else
                    action_ind = 9;
            }
    }
    if (result > gidp && result <= sf) {
        if (!game_status.status[3])
            action_ind = 12;
        else
            action_ind = 10;
    }
    if (result > sf && result <= groundout)
        action_ind = 9;
    if (result > groundout && result <= flyout)
        action_ind = 10;

    /* adjust some actions
     * if action is GIDP then make sure there's less than 2 outs and a baserunner on first
     * if action is SF then make sure there is less than 2 outs and a baserunner on third
     * if action is SINGLE then determine if runners should advance one or two bases
     * if action is DOUBLE then determine if runners should advance two or three bases
    */
adjust_action:
    if (action_ind == 11) /* GIDP */
        if (game_status.outs == 2 || game_status.baserunners[0] == 99)
            action_ind = 9;
    if (action_ind == 12) /* SF */
        if (game_status.outs == 2 || game_status.baserunners[2] == 99)
            action_ind = 10;
    if (action_ind == 2) {  /* SINGLE */
        if (game_status.baserunners[1] != 99 || game_status.baserunners[0] != 99) {
            if (game_status.baserunners[1] != 99)
                x = 1;
            else
                x = 0;
            r = (int) ((float) 999 * rand () / (RAND_MAX + 1.0));
            if ((game_status.half_inning % 2) == 0)
                if (visitor.batters[border[0][game_status.baserunners[x]].player[0]].hitting.cs == -1)
                    /* caught stealing for the batter wasn't always a recorded stat */
                    r += (visitor.batters[border[0][game_status.baserunners[x]].player[0]].hitting.sb * .67);
                else
                    r += visitor.batters[border[0][game_status.baserunners[x]].player[0]].hitting.sb -
                         visitor.batters[border[0][game_status.baserunners[x]].player[0]].hitting.cs;
            else
                if (home.batters[border[1][game_status.baserunners[x]].player[0]].hitting.cs == -1)
                    /* caught stealing for the batter wasn't always a recorded stat */
                    r += (home.batters[border[1][game_status.baserunners[x]].player[0]].hitting.sb * .67);
                else
                    r += home.batters[border[1][game_status.baserunners[x]].player[0]].hitting.sb -
                         home.batters[border[1][game_status.baserunners[x]].player[0]].hitting.cs;
            if (game_status.outs == 2)
                r += 200;
        }
        if (r > 300 || game_status.status[8])
            action_ind = 13;
    }
    if (action_ind == 3) {  /* DOUBLE */
        if (game_status.baserunners[0] != 99) {
            r = (int) ((float) 999 * rand () / (RAND_MAX + 1.0));
            if ((game_status.half_inning % 2) == 0)
                if (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                    /* caught stealing for the batter wasn't always a recorded stat */
                    r += (visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb * .67);
                else
                    r += visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb -
                         visitor.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs;
            else
                if (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs == -1)
                    /* caught stealing for the batter wasn't always a recorded stat */
                    r += (home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb * .67);
                else
                    r += home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb -
                         home.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs;
            if (game_status.outs == 2)
                r += 250;
        }
        if (r > 200 || game_status.status[8])
            action_ind = 14;
    }
update_stuff:
    find_fielder ();
    check_for_errors ();

    if (action_ind == 12) {
        /* Sacrifice Fly - check to see if baserunner is thrown out by the outfielder */
        if (post_act.half_inning % 2) {
            for (x = 0; x < 9; x++)
                if (border[0][x].pos[0] == whereto)
                    break;
            if (visitor.batters[border[0][x].player[0]].fielding[whereto].a == -1)
                assists = visitor.batters[border[0][x].player[0]].fielding[10].a;
            else
                assists = visitor.batters[border[0][x].player[0]].fielding[whereto].a;
            if (home.batters[border[1][game_status.baserunners[2]].player[0]].hitting.sb > 49)
                x = 50;
            else
                x = 40;
        }
        else {
            for (x = 0; x < 9; x++)
                if (border[1][x].pos[0] == whereto)
                    break;
            if (home.batters[border[1][x].player[0]].fielding[whereto].a == -1)
                assists = home.batters[border[1][x].player[0]].fielding[10].a;
            else
                assists = home.batters[border[1][x].player[0]].fielding[whereto].a;
            if (visitor.batters[border[0][game_status.baserunners[2]].player[0]].hitting.sb > 49)
                x = 50;
            else
                x = 40;
        }
        if ((int) ((float) x * rand () / (RAND_MAX + 1.0)) <= assists)
            action_ind = 23;  /* baserunner is thrown out */
    }

    update_stats ();
    upd_fielding_stats ();
    check_for_injury ();
    update_status ();
}

/* send a scrolling message to client describing what's going on */
void
send_bothalf () {
    char action[4096];
    int x, y, inj;

    action[0] = '\0';

    if (AlreadySentData < 3) {
        if (AlreadySentData == 1)
            x = 1;
        else
            x = 0;
        for (; x < 2; x++) {
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
            if (AlreadySentData == 2)
                break;
        }
    }
    AlreadySentData = 0;

    if (game_status.status[15])
        for (x = 0; x < 2; x++) {
            for (y = 0; y < 9; y++)
                if (lineupchg[x][y]) {
                    if (x) {
                        if (home_cur.batters[border[x][y].player[1]].id.injury) {
                            strcpy (&action[0], "BD");
                            switch_name (&action[0], &home_cur.batters[border[x][y].player[1]].id.name[0]);
                        }
                    }
                    else
                        if (visitor_cur.batters[border[x][y].player[1]].id.injury) {
                            strcpy (&action[0], "BD");
                            switch_name (&action[0], &visitor_cur.batters[border[x][y].player[1]].id.name[0]);
                        }
                    strcat (&action[0], " is injured for ");
                    if (x)
                        inj = home_cur.batters[border[x][y].player[1]].id.injury;
                    else
                        inj = visitor_cur.batters[border[x][y].player[1]].id.injury;
                    if (!inj)
                        strcat (&action[0], "the remainder of this game\n");
                    else {
                        if (inj < 10)
                            strcat (&action[0], (char *) cnvt_int2str (1, inj));
                        else
                            if (inj < 100)
                                strcat (&action[0], (char *) cnvt_int2str (2, inj));
                            else
                                strcat (&action[0], (char *) cnvt_int2str (3, inj));
                        if (inj == 1)
                            strcat (&action[0], " game\n");
                        else
                            strcat (&action[0], " games\n");
                    }
                    sock_puts (sock, &action[0]);
                    if (netgame)
                        sock_puts (netsock, &action[0]);

                    game_status.status[15] = 0;
                }
            if (dhpchg[x]) {
                if (x) {
                    for (y = 0; y < 25; y++)
                        if (!strcmp (&home_cur.batters[y].id.name[0], &home_cur.pitchers[pitching[x].pitcher[0]].id.name[0]))
                            break;
                    if (home_cur.batters[y].id.injury) {
                        strcpy (&action[0], "BD");
                        switch_name (&action[0], &home_cur.batters[y].id.name[0]);
                    }
                }
                else {
                    for (y = 0; y < 25; y++)
                        if (!strcmp (&visitor_cur.batters[y].id.name[0], &visitor_cur.pitchers[pitching[x].pitcher[0]].id.name[0]))
                            break;
                    if (visitor_cur.batters[y].id.injury) {
                        strcpy (&action[0], "BD");
                        switch_name (&action[0], &visitor_cur.batters[y].id.name[0]);
                    }
                }
                strcat (&action[0], " is injured for ");
                if (x)
                    inj = home_cur.batters[y].id.injury;
                else
                    inj = visitor_cur.batters[y].id.injury;
                if (!inj)
                    strcat (&action[0], "the remainder of this game\n");
                else {
                    if (inj < 10)
                        strcat (&action[0], (char *) cnvt_int2str (1, inj));
                    else
                        if (inj < 100)
                            strcat (&action[0], (char *) cnvt_int2str (2, inj));
                        else
                            strcat (&action[0], (char *) cnvt_int2str (3, inj));
                    if (inj == 1)
                        strcat (&action[0], " game\n");
                    else
                        strcat (&action[0], " games\n");
                }
                sock_puts (sock, &action[0]);
                if (netgame)
                    sock_puts (netsock, &action[0]);

                game_status.status[15] = 0;
            }
        }

    if (game_status.status[2] == 1) {
        sock_puts (sock, "BDThe infield is in at the corners\n");
        if (netgame)
            sock_puts (netsock, "BDThe infield is in at the corners\n");
    }
    if (game_status.status[2] == 2) {
        sock_puts (sock, "BDThe entire infield is playing in\n");
        if (netgame)
            sock_puts (netsock, "BDThe entire infield is playing in\n");
    }
    if (game_status.status[0]) {
        sock_puts (sock, "BDThe runner on first is being held on\n");
        if (netgame)
            sock_puts (netsock, "BDThe runner on first is being held on\n");
    }
    if (game_status.status[3]) {
        sock_puts (sock, "BDThe outfield is playing in\n");
        if (netgame)
            sock_puts (netsock, "BDThe outfield is playing in\n");
    }
    if (game_status.status[6] == 1) {
        sock_puts (sock, "BDThe third baseman is guarding the line\n");
        if (netgame)
            sock_puts (netsock, "BDThe third baseman is guarding the line\n");
    }
    if (game_status.status[6] == 2) {
        sock_puts (sock, "BDThe first baseman is guarding the line\n");
        if (netgame)
            sock_puts (netsock, "BDThe first baseman is guarding the line\n");
    }
    if (game_status.status[6] == 3) {
        sock_puts (sock, "BDBoth the first and third basemen are guarding the lines\n");
        if (netgame)
            sock_puts (netsock, "BDBoth the first and third basemen are guarding the lines\n");
    }

    if (game_status.status[9] || game_status.status[10]) {
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        strcat (&action[0], " lays down a bunt\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        if (action_ind == 19) {
            if (game_status.status[9])
                strcpy (&action[0], "BDIt's a good bunt.  ");
            else
                strcpy (&action[0], "BDIt's a great bunt.  ");
            runners_adv (&action[0]);
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
            strcat (&action[0], " is thrown out at first by ");
            x = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
            switch (whereto) {
                case 1:
                    if (x)
                        strcat (&action[0], "the pitcher");
                    else
                        strcat (&action[0], (char *) ReturnDefName (1));
                    break;
                case 2:
                    if (x)
                        strcat (&action[0], "the catcher");
                    else
                        strcat (&action[0], (char *) ReturnDefName (2));
                    break;
                case 3:
                    if (x)
                        strcat (&action[0], "the first baseman");
                    else
                        strcat (&action[0], (char *) ReturnDefName (3));
                    break;
                case 4:
                    if (x)
                        strcat (&action[0], "the second baseman");
                    else
                        strcat (&action[0], (char *) ReturnDefName (4));
                    break;
                case 5:
                    if (x)
                        strcat (&action[0], "the third baseman");
                    else
                        strcat (&action[0], (char *) ReturnDefName (5));
                    break;
                default:
                    if (x)
                        strcat (&action[0], "the shortstop");
                    else
                        strcat (&action[0], (char *) ReturnDefName (6));
            }
        }
        else {
            strcpy (&action[0], "BD");
            runner_cutdown (&action[0]);
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
            strcat (&action[0], " is safe at first");
        }

        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (game_status.status[1] == 1) {
        sock_puts (sock, "BDThere's a throw over to first\n");
        if (netgame)
            sock_puts (netsock, "BDThere's a throw over to first\n");
    }
    if (game_status.status[1] == 2) {
        sock_puts (sock, "BDThere's a pick-off attempt at second base\n");
        if (netgame)
            sock_puts (netsock, "BDThere's a pick-off attempt at second base\n");
    }
    if (game_status.status[1] == 3) {
        sock_puts (sock, "BDThere's a pick-off attempt at third base\n");
        if (netgame)
            sock_puts (netsock, "BDThere's a pick-off attempt at third base\n");
    }

    if (game_status.status[8]) {
        sock_puts (sock, "BDThe hit and run is on\n");
        if (netgame)
            sock_puts (netsock, "BDThe hit and run is on\n");
    }

    if (game_status.status[5]) {
        sock_puts (sock, "BDIt's a pitchout\n");
        if (netgame)
            sock_puts (netsock, "BDIt's a pitchout\n");
        if (game_status.status[8]) {
            strcpy (&action[0], "BD");
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
            strcat (&action[0], " swings and misses\n");
            strcat (&action[0], "\n");
            sock_puts (sock, &action[0]);
            if (netgame)
                sock_puts (netsock, &action[0]);
        }
    }

    if (action_ind == 20) {
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[hldbase - 1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[hldbase - 1]].player[0]].id.name[0]);
        strcat (&action[0], " is picked off ");
        if (hldbase == 3)
            strcat (&action[0], "third");
        else
        if (hldbase == 2)
                strcat (&action[0], "second");
        else
                strcat (&action[0], "first");
        strcat (&action[0], " base\n");
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (action_ind == 21 || action_ind == 22) {
        strcpy (&action[0], "BD");
        if (game_status.status[7] == 9 || game_status.status[7] == 7 || game_status.status[7] == 6 || game_status.status[7] == 4) {
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
            if (action_ind == 21)
                strcat (&action[0], " steals home.  ");
            else
                strcat (&action[0], " is caught trying to steal home");
            if (game_status.status[7] != 4) {
                strcat (&action[0], ".  ");
                if (action_ind == 21)
                    runners_adv (&action[0]);
                else
                    runner_cutdown (&action[0]);
            }
            else
                if (action_ind == 21) {
                    if (pre_act.half_inning % 2)
                        switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
                    else
                        switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
                    strcat (&action[0], " scores");
                }
        }
        if (game_status.status[7] == 5 || game_status.status[7] == 3) {
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
            if (action_ind == 21)
                strcat (&action[0], " steals third");
            else
                strcat (&action[0], " is caught trying to steal third");
            if (game_status.status[7] == 5 && (post_act.outs || (!pre_act.outs && !post_act.outs))) {
                strcat (&action[0], ".  ");
                if (pre_act.half_inning % 2)
                    switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
                else
                    switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
                strcat (&action[0], " moves to second base");
            }
        }
        if (game_status.status[7] == 2) {
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
            if (action_ind == 21)
                strcat (&action[0], " steals second");
            else
                strcat (&action[0], " is caught trying to steal second");
        }
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (action_ind == 0) {
        int yr;
        char tn[100], wb[500];

        if (forfeit == -1) {
            if (tforfeit) {
                yr = home.year;
                strcpy (&tn[0], (char *) GetTeamName (home.id));
            }
            else {
                yr = visitor.year;
                strcpy (&tn[0], (char *) GetTeamName (visitor.id));
            }
            strcpy (&wb[0], "BD\nGame over - ");
            strcat (&wb[0], (char *) cnvt_int2str (4, yr));
            strcat (&wb[0], " ");
            strcat (&wb[0], &tn[0]);
            strcat (&wb[0], " cannot field a team - Forfeit\n");

            sock_puts (sock, &wb[0]);
            if (netgame)
                sock_puts (netsock, &wb[0]);
        }
        else
            if (!gotateam) {
                if (tforfeit) {
                    yr = home.year;
                    strcpy (&tn[0], (char *) GetTeamName (home.id));
                }
                else {
                    yr = visitor.year;
                    strcpy (&tn[0], (char *) GetTeamName (visitor.id));
                }
                strcpy (&wb[0], "BD\nNo game - ");
                strcat (&wb[0], (char *) cnvt_int2str (4, yr));
                strcat (&wb[0], " ");
                strcat (&wb[0], &tn[0]);
                strcat (&wb[0], " cannot field a team - Forfeit\n");

                sock_puts (sock, &wb[0]);
                if (netgame)
                    sock_puts (netsock, &wb[0]);
            }
            else {
                if (!outhre) {
                    x = pre_act.half_inning / 2 + 1;
                    sock_puts (sock, "BD\n");
                    if (netgame)
                        sock_puts (netsock, "BD\n");

                    strcpy (&action[0], "BD");
                    if (ri < 10)
                        strcat (&action[0], (char *) cnvt_int2str (1, ri));
                    else
                        strcat (&action[0], (char *) cnvt_int2str (2, ri));
                    if (ri == 1)
                        strcat (&action[0], " run on ");
                    else
                        strcat (&action[0], " runs on ");
                    if (hi < 10)
                        strcat (&action[0], (char *) cnvt_int2str (1, hi));
                    else
                        strcat (&action[0], (char *) cnvt_int2str (2, hi));
                    if (hi == 1)
                        strcat (&action[0], " hit and ");
                    else
                        strcat (&action[0], " hits and ");
                    if (ei < 10)
                        strcat (&action[0], (char *) cnvt_int2str (1, ei));
                    else
                        strcat (&action[0], (char *) cnvt_int2str (2, ei));
                    if (ei == 1)
                        strcat (&action[0], " error");
                    else
                        strcat (&action[0], " errors");
                    strcat (&action[0], "\n");
                    sock_puts (sock, &action[0]);
                    if (netgame)
                        sock_puts (netsock, &action[0]);

                    strcpy (&action[0], "BDThat's the end of the ");
                    if (x > 9)
                        strcat (&action[0], (char *) cnvt_int2str (2, x));
                    else
                        strcat (&action[0], (char *) cnvt_int2str (1, x));
                    switch (x % 10) {
                        case 1:
                            if (x != 11)
                                strcat (&action[0], "st ");
                            else
                                strcat (&action[0], "th ");
                            break;
                        case 2:
                            if (x != 11)
                                strcat (&action[0], "nd ");
                            else
                                strcat (&action[0], "th ");
                            break;
                        case 3:
                            if (x != 11)
                                strcat (&action[0], "rd ");
                            else
                                strcat (&action[0], "th ");
                            break;
                        default:
                            strcat (&action[0], "th ");
                    }
                    strcat (&action[0], "inning\n");
                    sock_puts (sock, &action[0]);
                    if (netgame)
                        sock_puts (netsock, &action[0]);

                    sock_puts (sock, "BD\n");
                    if (netgame)
                        sock_puts (netsock, "BD\n");
                }
                sock_puts (sock, "BDGame over\n");
                if (netgame)
                    sock_puts (netsock, "BDGame over\n");
            }
        return;
    }

    if (action_ind == 1) {
        sock_puts (sock, "BDPlay Ball !\n");
        sock_puts (sock, "BD\n");
        if (netgame) {
            sock_puts (netsock, "BDPlay Ball !\n");
            sock_puts (netsock, "BD\n");
        }

        strcpy (&action[0], "BD1st inning\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);

        strcpy (&action[0], "BD\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (action_ind == 15) {
        strcpy (&action[0], "BDWild pitch.  ");
        runners_adv (&action[0]);
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        return;
    }

    if (action_ind == 17) {
        strcpy (&action[0], "BDPassed ball.  ");
        runners_adv (&action[0]);
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        return;
    }

    if (action_ind == 16) {
        strcpy (&action[0], "BDThe pitcher balks.  ");
        runners_adv (&action[0]);
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        return;
    }

    if (action_ind == 18) {
        strcpy (&action[0], "BDError.  ");
        runners_adv (&action[0]);
        if (game_status.baserunners[0] != 99) {
            if (strcmp (&action[strlen (&action[0]) - 3], ".  "))
                strcat (&action[0], ".  ");
            if (post_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][post_act.baserunners[0]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][post_act.baserunners[0]].player[0]].id.name[0]);
            strcat (&action[0], " is on at first base");
        }
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        return;
    }

    if (action_ind == 6) {
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        if (game_status.status[4])
            if ((int) (rand () / (RAND_MAX + 1.0)))
                strcat (&action[0], " is intentionally walked");
            else
                strcat (&action[0], " intentionally walks");
        else
            strcat (&action[0], " walks");

        force_runners_ahead (&action[0]);
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (action_ind == 7) {
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        x = (int) ((float) 9 * rand () / (RAND_MAX + 1.0));
        switch (x) {
            case 1:
                strcat (&action[0], " strikes out");
                break;
            case 2:
                strcat (&action[0], " strikes out looking");
                break;
            case 3:
                strcat (&action[0], " is rung up");
                break;
            case 4:
                strcat (&action[0], " is caught looking");
                break;
            case 5:
                strcat (&action[0], " looked at one too many");
                break;
            case 6:
                strcat (&action[0], " caught window shopping");
                break;
            case 7:
                strcat (&action[0], " goes down swinging");
                break;
            case 8:
                strcat (&action[0], " goes down looking");
                break;
            default:
                strcat (&action[0], " strikes out swinging");
        }

        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (action_ind == 8) {
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        x = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
        if (x)
            strcat (&action[0], " is hit by the pitch");
        else
            strcat (&action[0], " is hit by a pitch");

        force_runners_ahead (&action[0]);
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if ((action_ind > 1 && action_ind < 6) || (action_ind > 8 && action_ind < 19) || action_ind == 23) {
        if (action_ind == 9 && (game_status.status[9] || game_status.status[10]))
            return;
        strcpy (&action[0], "BD");
        if (pre_act.half_inning % 2)
            switch_name (&action[0], &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        strcat (&action[0], " ");
        if (action_ind == 13)
            strcat (&action[0], singles2[(int) ((float) 23 * rand () / (RAND_MAX + 1.0))]);
        if (action_ind == 3 || action_ind == 14) {
            if (action_ind == 14)
                /* don't allow a ground-rule double */
                strcat (&action[0], doubles[(int) ((float) 8 * rand () / (RAND_MAX + 1.0))]);
            else
                strcat (&action[0], doubles[(int) ((float) 9 * rand () / (RAND_MAX + 1.0))]);
            strcat (&action[0], " for a double");
            if (action_ind == 3)
                move_runners (2, &action[0]);
        }
        if (action_ind == 4) {
            strcat (&action[0], triples[(int) ((float) 2 * rand () / (RAND_MAX + 1.0))]);
            strcat (&action[0], " for a triple");
            move_runners (3, &action[0]);
        }
        if (action_ind == 5) {
            strcat (&action[0], homers[(int) ((float) 30 * rand () / (RAND_MAX + 1.0))]);
            move_runners (4, &action[0]);
        }
        if (action_ind == 9) {
            if (whereto != 2) {
                strcat (&action[0], grounders[(int) ((float) 15 * rand () / (RAND_MAX + 1.0))]);
                x = (int) ((float) 3 * rand () / (RAND_MAX + 1.0));
                switch (whereto) {
                    case 1:
                        if (x == 2)
                            strcat (&action[0], " the pitcher");
                        if (x == 1)
                            strcat (&action[0], " the mound");
                        if (!x) {
                            strcat (&action[0], " ");
                            strcat (&action[0], (char *) ReturnDefName (1));
                        }
                        break;
                    case 3:
                        if (x == 2)
                            strcat (&action[0], " the first baseman");
                        if (x == 1)
                            strcat (&action[0], " first");
                        if (!x) {
                            strcat (&action[0], " ");
                            strcat (&action[0], (char *) ReturnDefName (3));
                        }
                        break;
                    case 4:
                        if (x == 2)
                            strcat (&action[0], " the second baseman");
                        if (x == 1)
                            strcat (&action[0], " second");
                        if (!x) {
                            strcat (&action[0], " ");
                            strcat (&action[0], (char *) ReturnDefName (4));
                        }
                        break;
                    case 5:
                        if (x == 2)
                            strcat (&action[0], " the third baseman");
                        if (x == 1)
                            strcat (&action[0], " third");
                        if (!x) {
                            strcat (&action[0], " ");
                            strcat (&action[0], (char *) ReturnDefName (5));
                        }
                        break;
                    default:
                        if (x == 2)
                            strcat (&action[0], " the shortstop");
                        if (x == 1)
                            strcat (&action[0], " short");
                        if (!x) {
                            strcat (&action[0], " ");
                            strcat (&action[0], (char *) ReturnDefName (6));
                        }
                }
                strcat (&action[0], " who ");
                x = (int) ((float) 7 * rand () / (RAND_MAX + 1.0));
                if (whereto == 3 && !rcd)
                    if (!flip2p)
                        if ((int) (rand () / (RAND_MAX + 1.0)))
                            strcat (&action[0], "steps on the bag");
                        else
                            strcat (&action[0], "makes the play unassisted");
                    else {
                        flip2p = 0;
                        if ((int) (rand () / (RAND_MAX + 1.0)))
                            strcat (&action[0], "flips to the pitcher covering the bag");
                        else {
                            strcat (&action[0], "flips to ");
                            strcat (&action[0], (char *) ReturnDefName (1));
                            strcat (&action[0], " covering the bag");
                        }
                    }
                else {
                    switch (x) {
                        case 0:
                            strcat (&action[0], "throws");
                            break;
                        case 1:
                            strcat (&action[0], "pegs the ball");
                            break;
                        case 2:
                            strcat (&action[0], "guns it");
                            break;
                        case 3:
                            strcat (&action[0], "rifles the ball");
                            break;
                        case 4:
                            strcat (&action[0], "fires");
                            break;
                        case 5:
                            strcat (&action[0], "whips it");
                            break;
                        default:
                            strcat (&action[0], "tosses");
                    }
                    strcat (&action[0], " to ");
                    if (rcd) {
                        strcat (&action[0], "the catcher");
                        pre_act.baserunners[2] = 99;
                    }
                    else {
                        x = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
                        if (altpo && !game_status.status[8])
                            if (altpo == 4)
                                if (x)
                                    strcat (&action[0], "the second baseman covering");
                                else {
                                    strcat (&action[0], (char *) ReturnDefName (4));
                                    strcat (&action[0], " covering second");
                                }
                            else
                                if (x)
                                    strcat (&action[0], "the shortstop covering the bag");
                                else {
                                    strcat (&action[0], (char *) ReturnDefName (6));
                                    strcat (&action[0], " covering second");
                                }
                        else
                            if (x)
                                strcat (&action[0], "first");
                            else
                                strcat (&action[0], (char *) ReturnDefName (3));
                    }
                    strcat (&action[0], " for the out");
                }
                rcd = 0;
            }
            else {
                strcat (&action[0], grounders[0]);
                strcat (&action[0], " the catcher who fires to first for the out");
            }
            if (post_act.outs)
                move_runners (1, &action[0]);
        }
        if (action_ind == 10) {
            if (whereto == 1 || whereto == 4 || whereto == 6)
                strcat (&action[0], airball1[(int) ((float) 3 * rand () / (RAND_MAX + 1.0))]);
            if (whereto == 3 || whereto == 5)
                strcat (&action[0], airball2[(int) ((float) 5 * rand () / (RAND_MAX + 1.0))]);
            if (whereto == 2)
                strcat (&action[0], airball3[(int) ((float) 4 * rand () / (RAND_MAX + 1.0))]);
            if (whereto == 7 || whereto == 9)
                strcat (&action[0], airball4[(int) ((float) 11 * rand () / (RAND_MAX + 1.0))]);
            if (whereto == 8)
                strcat (&action[0], airball5[(int) ((float) 6 * rand () / (RAND_MAX + 1.0))]);

            x = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
            switch (whereto) {
                case 1:
                    if (x)
                        strcat (&action[0], " the pitcher");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (1));
                    }
                    break;
                case 2:
                    if (x)
                        strcat (&action[0], " the catcher");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (2));
                    }
                    break;
                case 3:
                    if (x)
                        strcat (&action[0], " the first baseman");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (3));
                    }
                    break;
                case 4:
                    if (x)
                        strcat (&action[0], " the second baseman");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (4));
                    }
                    break;
                case 5:
                    if (x)
                        strcat (&action[0], " third");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (5));
                    }
                    break;
                case 6:
                    if (x)
                        strcat (&action[0], " the shortstop");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (6));
                    }
                    break;
                case 7:
                    if (x)
                        strcat (&action[0], " left");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (7));
                    }
                    break;
                case 8:
                    if (x)
                        strcat (&action[0], " the center fielder");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (8));
                    }
                    break;
                case 9:
                    if (x)
                        strcat (&action[0], " right field");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (9));
                    }
            }
        }

        if (action_ind == 2) {
            if (whereto > 0 && whereto < 7)
                strcat (&action[0], singles1[(int) ((float) 4 * rand () / (RAND_MAX + 1.0))]);
            else
                strcat (&action[0], singles2[(int) ((float) 23 * rand () / (RAND_MAX + 1.0))]);

            switch (whereto) {
                case 1:
                    strcat (&action[0], " the pitcher");
                    break;
                case 2:
                    strcat (&action[0], " the catcher");
                    break;
                case 3:
                    strcat (&action[0], " the first baseman");
                    break;
                case 4:
                    strcat (&action[0], " the second baseman");
                    break;
                case 5:
                    strcat (&action[0], " third");
                    break;
                case 6:
                    strcat (&action[0], " the shortstop");
                    break;
                case 7:
                    strcat (&action[0], " left for a single");
                    break;
                case 8:
                    strcat (&action[0], " the center fielder for a single");
                    break;
                case 9:
                    strcat (&action[0], " right field for a single");
            }
            if (whereto > 0 && whereto < 7)
                strcat (&action[0], ".  The batter is safe at first");
            move_runners (1, &action[0]);
        }

        if (action_ind == 13) {
            switch (whereto) {
                case 7:
                    strcat (&action[0], " left for a single");
                    break;
                case 8:
                    strcat (&action[0], " center for a single");
                    break;
                default:
                    strcat (&action[0], " right for a single");
            }
            move_runners (2, &action[0]); 
        }

        if (action_ind == 12 || action_ind == 23) {
            if (whereto == 7 || whereto == 9)
                strcat (&action[0], airball4[(int) ((float) 11 * rand () / (RAND_MAX + 1.0))]);
            if (whereto == 8)
                strcat (&action[0], airball5[(int) ((float) 6 * rand () / (RAND_MAX + 1.0))]);

            x = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
            switch (whereto) {
                case 7:
                    if (x)
                        strcat (&action[0], " the left fielder");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (7));
                    }
                    break;
                case 8:
                    if (x)
                        strcat (&action[0], " center");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (8));
                    }
                    break;
                default:
                    if (x)
                        strcat (&action[0], " right field");
                    else {
                        strcat (&action[0], " ");
                        strcat (&action[0], (char *) ReturnDefName (9));
                    }
            }
            strcat (&action[0], ".  ");
            if (pre_act.half_inning % 2)
                switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
            else
                switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);

            if (action_ind == 23)
                strcat (&action[0], " is thrown out trying to score from third");
            else
                strcat (&action[0], " scores from third");
        }

        if (action_ind == 11) {
            if (whereto != 2) {
                strcat (&action[0],
                    grounders[(int) ((float) 15 * rand () / (RAND_MAX + 1.0))]);
                switch (whereto) {
                    case 1:
                        strcat (&action[0], " the pitcher who throws to the shortstop for one and the relay to first is in time for two");
                        break;
                    case 3:
                        strcat (&action[0],
                          " the first baseman who steps on the base for one and fires to the shortstop who tags the runner for the double play");
                        break;
                    case 4:
                        strcat (&action[0], " the second baseman who throws to the shortstop for one out ... and the relay to first is in time");
                        break;
                    case 5:
                        strcat (&action[0], " third who fires to second ... and to first for the double play");
                        break;
                    default:
                        strcat (&action[0], " the shortstop who throws to second and the turn to first ... got the runner by a step");
                }
            }
            else {
                strcat (&action[0], grounders[0]);
                strcat (&action[0], " the catcher who fires to the shortstop for one and the throw to first gets the batter");
            }
            if (!pre_act.outs && pre_act.baserunners[2] != 99) {
                strcat (&action[0], ".  ");
                if (post_act.half_inning % 2)
                    switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
                else
                    switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
                strcat (&action[0], " scores from third");
            }
            if (!pre_act.outs && pre_act.baserunners[1] != 99) {
                strcat (&action[0], ".  ");
                if (post_act.half_inning % 2)
                    switch_name (&action[0], &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
                else
                    switch_name (&action[0], &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
                strcat (&action[0], " moves to third");
            }
        }

        if (action_ind == 14)
            move_runners (3, &action[0]);

        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
    }

    if (pre_act.outs && !post_act.outs) {
        outhre = 1;
        x = pre_act.half_inning / 2 + 1;
        y = post_act.half_inning % 2;

        strcpy (&action[0], "BD\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);

        strcpy (&action[0], "BD");
        if (ri < 10)
            strcat (&action[0], (char *) cnvt_int2str (1, ri));
        else
            strcat (&action[0], (char *) cnvt_int2str (2, ri));
        if (ri == 1)
            strcat (&action[0], " run on ");
        else
            strcat (&action[0], " runs on ");
        if (hi < 10)
            strcat (&action[0], (char *) cnvt_int2str (1, hi));
        else
            strcat (&action[0], (char *) cnvt_int2str (2, hi));
        if (hi == 1)
            strcat (&action[0], " hit and ");
        else
            strcat (&action[0], " hits and ");
        if (ei < 10)
            strcat (&action[0], (char *) cnvt_int2str (1, ei));
        else
            strcat (&action[0], (char *) cnvt_int2str (2, ei));
        if (ei == 1)
            strcat (&action[0], " error");
        else
            strcat (&action[0], " errors");
        strcat (&action[0], "\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);
        ei = ri = hi = 0;

        strcpy (&action[0], "BD");
        if (y)
            strcat (&action[0], "That's the end of the top half of the ");
        else
            strcat (&action[0], "That's the end of the ");
        if (x > 9)
            strcat (&action[0], (char *) cnvt_int2str (2, x));
        else
            strcat (&action[0], (char *) cnvt_int2str (1, x));
        switch (x % 10) {
            case 1:
                if (x != 11)
                    strcat (&action[0], "st ");
                else
                    strcat (&action[0], "th ");
                break;
            case 2:
                if (x != 11)
                    strcat (&action[0], "nd ");
                else
                    strcat (&action[0], "th ");
                break;
            case 3:
                if (x != 11)
                    strcat (&action[0], "rd ");
                else
                    strcat (&action[0], "th ");
                break;
            default:
                strcat (&action[0], "th ");
        }
        strcat (&action[0], "inning\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);

        strcpy (&action[0], "BD\n");
        sock_puts (sock, &action[0]);
        if (netgame)
            sock_puts (netsock, &action[0]);

        if (!y) {
            x++;
            if (x > 9 && vruns > hruns) {
                strcpy (&action[0], "BD\n");
                sock_puts (sock, &action[0]);
                if (netgame)
                    sock_puts (netsock, &action[0]);
            }
            else {
                strcpy (&action[0], "BD");

                if (x > 9)
                    strcat (&action[0], (char *) cnvt_int2str (2, x));
                else
                    strcat (&action[0], (char *) cnvt_int2str (1, x));
                switch (x % 10) {
                    case 1:
                        if (x != 11)
                            strcat (&action[0], "st ");
                        else
                            strcat (&action[0], "th ");
                        break;
                    case 2:
                        if (x != 11)
                            strcat (&action[0], "nd ");
                        else
                            strcat (&action[0], "th ");
                        break;
                    case 3:
                        if (x != 11)
                            strcat (&action[0], "rd ");
                        else
                            strcat (&action[0], "th ");
                        break;
                    default:
                        strcat (&action[0], "th ");
                }
                strcat (&action[0], "inning\n");
                sock_puts (sock, &action[0]);
                if (netgame)
                    sock_puts (netsock, &action[0]);

                strcpy (&action[0], "BD\n");
                sock_puts (sock, &action[0]);
                if (netgame)
                    sock_puts (netsock, &action[0]);
            }
        }
    }
    else
        outhre = 0;
}

/* determine where the ball is hit to */
void
find_fielder () {
    int x;
    char side;

    whereto = 99;

    /* determine which side of the plate the batter is hitting from */
    if (action_ind > 1 && action_ind < 20) {
        if ((game_status.half_inning % 2) == 0)
            if (visitor_cur.batters[border[0][game_status.batter[0]].player[0]].id.batslr == 1 ||
                (visitor_cur.batters[border[0][game_status.batter[0]].player[0]].id.batslr == 2 &&
                 home_cur.pitchers[game_status.pitcher[1]].id.throwslr == 0))
                side = 'l';
            else
                side = 'r';
        else
            if (home_cur.batters[border[1][game_status.batter[1]].player[0]].id.batslr == 1 ||
                (home_cur.batters[border[1][game_status.batter[1]].player[0]].id.batslr == 2 &&
                 visitor_cur.pitchers[game_status.pitcher[0]].id.throwslr == 0))
                side = 'l';
            else
                side = 'r';
    }

    if (game_status.status[9]) {
        if (game_status.status[2]) {
            whereto = (int) ((float) 4 * rand () / (RAND_MAX + 1.0)) + 1;
            if (whereto == 4)
                whereto++;
        }
        else
            whereto = (int) ((float) 2 * rand () / (RAND_MAX + 1.0)) + 1;
        return;
    }

    if (game_status.status[10]) {
        if (game_status.status[2]) {
            whereto = (int) ((float) 3 * rand () / (RAND_MAX + 1.0)) + 1;
            if (whereto == 2)
                whereto = 5;
        }
        else
            whereto = 1;
        return;
    }

    if (action_ind == 9 || action_ind == 11) {
        if (game_status.status[8]) {
            if (!(int) ((float) 4 * rand () / (RAND_MAX + 1.0)))
                whereto = 3;
            else
                whereto = 4;
        }
        else {
            x = (int) ((float) 1000 * rand () / (RAND_MAX + 1.0));
            if (x < 5)
                whereto = 2;
            if (x > 4 && x < 55)
                whereto = 1;
            if (side == 'l') {
                if (x > 54 && x < 125)
                    whereto = 5;
                if (x > 124 && x < 380)
                    whereto = 6;
                if (x > 379 && x < 560)
                    whereto = 3;
                if (x > 555 && x < 1000)
                    whereto = 4;
            }
            else {
                if (x > 54 && x < 125)
                    whereto = 3;
                if (x > 124 && x < 380)
                    whereto = 4;
                if (x > 379 && x < 560)
                    whereto = 5;
                if (x > 555 && x < 1000)
                    whereto = 6;
            }
        }
    }
    if ((action_ind > 2 && action_ind < 6) || (action_ind > 11 && action_ind < 15)) {
        x = (int) ((float) 68 * rand () / (RAND_MAX + 1.0));
        if (x < 25)
            whereto = 8;
        else
            if (side == 'l')
                if (x > 24 && x < 35)
                    whereto = 7;
                else
                    whereto = 9;
            else
                if (x > 24 && x < 35)
                    whereto = 9;
                else
                    whereto = 7;
    }

    if (action_ind == 2 || action_ind == 10) {
        x = (int) ((float) 100 * rand () / (RAND_MAX + 1.0));
        if (x < 2)
            whereto = 1;
        if (x > 1 && x < 4)
            whereto = 2;
        if (x > 3 && x < 30)
            whereto = 8;
        if (side == 'l') {
            if (x > 29 && x < 32)
                whereto = 5;
            if (x > 31 && x < 36)
                whereto = 6;
            if (x > 35 && x < 40)
                whereto = 3;
            if (x > 39 && x < 45)
                whereto = 4;
            if (x > 44 && x < 64)
                whereto = 7;
            if (x > 63 && x < 100)
                whereto = 9;
        }
        else {
            if (x > 29 && x < 32)
                whereto = 3;
            if (x > 31 && x < 36)
                whereto = 4;
            if (x > 35 && x < 40)
                whereto = 5;
            if (x > 39 && x < 45)
                whereto = 6;
            if (x > 44 && x < 64)
                whereto = 9;
            if (x > 63 && x < 100)
                whereto = 7;
        }
    }
}

/* check for an error */
void
check_for_errors () {
    int result;
    int y, pos[10], spos, ha;

    if (game_status.status[9] || game_status.status[10])
        /* if the batter is bunting and there's an error it screws things up
           return for now, fix this later */
        return;

    if ((game_status.half_inning % 2) == 0)
        ha = 1;
    else
        ha = 0;

    for (y = 0; y < 10; y++)
        pos[y] = 99;

    for (spos = 1; spos < 10; spos++)
        for (y = 0; y < 9; y++)
            if (border[ha][y].pos[0] == spos) {
                pos[spos] = border[ha][y].player[0];
                break;
            }
    /* ensure pitcher position is filled in */
    if (dhind)
        for (y = 0; y < 25; y++) {
            if (ha) {
                if (!strcmp (&home.pitchers[game_status.pitcher[ha]].id.name[0], &home.batters[y].id.name[0])) {
                    pos[1] = y;
                    break;
                }
            }
            else
                if (!strcmp (&visitor.pitchers[game_status.pitcher[ha]].id.name[0], &visitor.batters[y].id.name[0])) {
                    pos[1] = y;
                    break;
                }
        }

    if (game_status.status[1]) {
        /* y holds a pointer to the fielder who the pitcher is throwing to */
        if (game_status.status[1] == 1)
            y = 3;
        else
            if (game_status.status[1] == 3)
                y = 5;
            else
                if (!(int) ((float) 4 * rand () / (RAND_MAX + 1.0)))
                    y = 4;
                else
                    y = 6;

        if (ha) {
            if (home.batters[pos[1]].fielding[1].games) {
                if (!(home.batters[pos[1]].fielding[1].a + home.batters[pos[1]].fielding[1].po + home.batters[pos[1]].fielding[1].e)) {
                    /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                    if (!(10 * rand () / (RAND_MAX + 1.0))) {
                        action_ind = 18;
                        whereto = 1;
                        return;
                    }
                }
                else {
                    result = (int) ((float) ((home.batters[pos[1]].fielding[1].a +
                      home.batters[pos[1]].fielding[1].po + home.batters[pos[1]].fielding[1].e) * 4) * rand () / (RAND_MAX + 1.0));
                    if (result <= home.batters[pos[1]].fielding[1].e) {
                        action_ind = 18;
                        whereto = 1;
                        return;
                    }
                }

                if (home.batters[pos[y]].fielding[y].games) {
                    if (!(home.batters[pos[y]].fielding[y].a + home.batters[pos[y]].fielding[y].po + home.batters[pos[y]].fielding[y].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0))) {
                            action_ind = 18;
                            whereto = y;
                            return;
                        }
                    }
                    else {
                        result = (int) ((float) ((home.batters[pos[y]].fielding[y].a +
                          home.batters[pos[y]].fielding[y].po + home.batters[pos[y]].fielding[y].e) * 4) * rand () / (RAND_MAX + 1.0));
                        if (result <= home.batters[pos[y]].fielding[y].e) {
                            action_ind = 18;
                            whereto = y;
                            return;
                        }
                    }
                }
                else {
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0))) {
                        action_ind = 18;
                        whereto = y;
                        return;
                    }
                }
            }
            else {
                if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0))) {
                    action_ind = 18;
                    whereto = 1;
                    return;
                }
            }
        }
        else {
            if (visitor.batters[pos[1]].fielding[1].games) {
                if (!(visitor.batters[pos[1]].fielding[1].a + visitor.batters[pos[1]].fielding[1].po + visitor.batters[pos[1]].fielding[1].e)) {
                    /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                    if (!(10 * rand () / (RAND_MAX + 1.0))) {
                        action_ind = 18;
                        whereto = 1;
                        return;
                    }
                }
                else {
                    result = (int) ((float) ((visitor.batters[pos[1]].fielding[1].a +
                      visitor.batters[pos[1]].fielding[1].po + visitor.batters[pos[1]].fielding[1].e) * 4) * rand () / (RAND_MAX + 1.0));
                    if (result <= visitor.batters[pos[1]].fielding[1].e) {
                        action_ind = 18;
                        whereto = 1;
                        return;
                    }
                }

                if (visitor.batters[pos[y]].fielding[y].games) {
                    if (!(visitor.batters[pos[y]].fielding[y].a + visitor.batters[pos[y]].fielding[y].po + visitor.batters[pos[y]].fielding[y].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0))) {
                            action_ind = 18;
                            whereto = y;
                            return;
                        }
                    }
                    else {
                        result = (int) ((float) ((visitor.batters[pos[y]].fielding[y].a +
                                 visitor.batters[pos[y]].fielding[y].po + visitor.batters[pos[y]].fielding[y].e) * 4) * rand () / (RAND_MAX + 1.0));
                        if (result <= visitor.batters[pos[y]].fielding[y].e) {
                            action_ind = 18;
                            whereto = y;
                            return;
                        }
                    }
                }
                else {
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0))) {
                        action_ind = 18;
                        whereto = y;
                        return;
                    }
                }
            }
            else {
                if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0))) {
                    action_ind = 18;
                    whereto = 1;
                    return;
                }
            }
        }
    }

    if (action_ind > 8 && action_ind < 13) {
        if (ha) {
            if (whereto > 6 && whereto < 10) {
                if (home.batters[pos[whereto]].fielding[whereto].games || home.batters[pos[whereto]].fielding[10].games) {
                    int ps;

                    /* sometimes all 3 outfield positions are combined into one stat */
                    if (home.batters[pos[whereto]].fielding[whereto].po == -1)
                        ps = 10;
                    else
                        ps = whereto;

                    if (!(home.batters[pos[whereto]].fielding[ps].a + home.batters[pos[whereto]].fielding[ps].po +
                                                                        home.batters[pos[whereto]].fielding[ps].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0)))
                            action_ind = 18;
                    }
                    else {
                        result = (int) ((float) (home.batters[pos[whereto]].fielding[ps].a + home.batters[pos[whereto]].fielding[ps].po +
                                            home.batters[pos[whereto]].fielding[ps].e) * rand () / (RAND_MAX + 1.0));
                        if (result <= home.batters[pos[whereto]].fielding[ps].e)
                            action_ind = 18;
                    }
                }
                else
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0)))
                        action_ind = 18;
            }
            else {
                if (home.batters[pos[whereto]].fielding[whereto].games) {
                    if (!(home.batters[pos[whereto]].fielding[whereto].a + home.batters[pos[whereto]].fielding[whereto].po +
                                                                             home.batters[pos[whereto]].fielding[whereto].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0)))
                            action_ind = 18;
                    }
                    else {
                        result = (int) ((float) (home.batters[pos[whereto]].fielding[whereto].a + home.batters[pos[whereto]].fielding[whereto].po +
                                                 home.batters[pos[whereto]].fielding[whereto].e) * rand () / (RAND_MAX + 1.0));
                        if (result <= home.batters[pos[whereto]].fielding[whereto].e)
                            action_ind = 18;
                    }
                }
                else
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0)))
                        action_ind = 18;
            }
        }
        else {
            if (whereto > 6 && whereto < 10) {
                if (visitor.batters[pos[whereto]].fielding[whereto].games || visitor.batters[pos[whereto]].fielding[10].games) {
                    int ps;

                    /* sometimes all 3 outfield positions are combined into one stat */
                    if (visitor.batters[pos[whereto]].fielding[whereto].po == -1)
                        ps = 10;
                    else
                        ps = whereto;

                    if (!(visitor.batters[pos[whereto]].fielding[ps].a + visitor.batters[pos[whereto]].fielding[ps].po +
                                                                           visitor.batters[pos[whereto]].fielding[ps].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0)))
                            action_ind = 18;
                    }
                    else {
                        result = (int) ((float) (visitor.batters[pos[whereto]].fielding[ps].a + visitor.batters[pos[whereto]].fielding[ps].po +
                                            visitor.batters[pos[whereto]].fielding[ps].e) * rand () / (RAND_MAX + 1.0));
                        if (result <= visitor.batters[pos[whereto]].fielding[ps].e)
                            action_ind = 18;
                    }
                }
                else
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0)))
                        action_ind = 18;
            }
            else {
                if (visitor.batters[pos[whereto]].fielding[whereto].games) {
                    if (!(visitor.batters[pos[whereto]].fielding[whereto].a + visitor.batters[pos[whereto]].fielding[whereto].po +
                                                                                visitor.batters[pos[whereto]].fielding[whereto].e)) {
                        /* if the fielder played games at the position in real life but had no chances then give them a 10% chance of committing an error */
                        if (!(10 * rand () / (RAND_MAX + 1.0)))
                            action_ind = 18;
                    }
                    else {
                        result = (int) ((float) (visitor.batters[pos[whereto]].fielding[whereto].a + visitor.batters[pos[whereto]].fielding[whereto].po +
                                          visitor.batters[pos[whereto]].fielding[whereto].e) * rand () / (RAND_MAX + 1.0));
                        if (result <= visitor.batters[pos[whereto]].fielding[whereto].e)
                            action_ind = 18;
                    }
                }
                else
                    if (!(int) ((float) 3 * rand () / (RAND_MAX + 1.0)))
                        action_ind = 18;
            }
        }
    }
}

/* update fielding stats */
void
upd_fielding_stats () {
    int y, pos[10], spos, ha;

    if ((game_status.half_inning % 2) == 0)
        ha = 1;
    else
        ha = 0;

    for (y = 0; y < 10; y++)
        pos[y] = 99;

    for (spos = 1; spos < 10; spos++)
        for (y = 0; y < 9; y++)
            if (border[ha][y].pos[0] == spos) {
                pos[spos] = border[ha][y].player[0];
                break;
            }
    if (dhind)
        for (y = 0; y < 25; y++) {
            if (ha) {
                if (!strcmp (&home.pitchers[game_status.pitcher[ha]].id.name[0], &home.batters[y].id.name[0])) {
                    pos[1] = y;
                    break;
                }
            }
            else
                if (!strcmp (&visitor.pitchers[game_status.pitcher[ha]].id.name[0], &visitor.batters[y].id.name[0])) {
                    pos[1] = y;
                    break;
                }
        }

    if (action_ind == 18 && whereto && whereto < 10) {
        if (ha)
            home_cur.batters[pos[whereto]].fielding[whereto].e++;
        else
            visitor_cur.batters[pos[whereto]].fielding[whereto].e++;
        ei++;
    }
    if (action_ind == 20) {
        if (ha) {
            home_cur.batters[pos[1]].fielding[1].a++;
            if (game_status.status[1] == 3)
                home_cur.batters[pos[5]].fielding[5].po++;
            else
                if (game_status.status[1] == 2)
                    if (!(int) ((float) 4 * rand () / (RAND_MAX + 1.0)))
                        home_cur.batters[pos[4]].fielding[4].po++;
                    else
                        home_cur.batters[pos[6]].fielding[6].po++;
                else
                    home_cur.batters[pos[3]].fielding[3].po++;
        }
        else {
            visitor_cur.batters[pos[1]].fielding[1].a++;
            if (game_status.status[1] == 3)
                visitor_cur.batters[pos[5]].fielding[5].po++;
            else
                if (game_status.status[1] == 2)
                    if (!(int) ((float) 4 * rand () / (RAND_MAX + 1.0)))
                        visitor_cur.batters[pos[4]].fielding[4].po++;
                    else
                        visitor_cur.batters[pos[6]].fielding[6].po++;
                else
                    visitor_cur.batters[pos[3]].fielding[3].po++;
        }
    }
    if (action_ind == 22) {
        if (ha) {
            if (game_status.status[7] == 9 || game_status.status[7] == 7 || game_status.status[7] == 6 || game_status.status[7] == 4) {
                home_cur.batters[pos[2]].fielding[2].po++;
                home_cur.batters[pos[1]].fielding[1].a++;
            }
            if (game_status.status[7] == 5 || game_status.status[7] == 3) {
                home_cur.batters[pos[2]].fielding[2].a++;
                home_cur.batters[pos[5]].fielding[5].po++;
            }
            if (game_status.status[7] == 2) {
                home_cur.batters[pos[2]].fielding[2].a++;
                if (!(int) ((float) 2 * rand () / (RAND_MAX + 1.0)))
                    home_cur.batters[pos[4]].fielding[4].po++;
                else
                    home_cur.batters[pos[6]].fielding[6].po++;
            }
        }
        else {
            if (game_status.status[7] == 9 || game_status.status[7] == 7 || game_status.status[7] == 6 || game_status.status[7] == 4) {
                visitor_cur.batters[pos[2]].fielding[2].po++;
                visitor_cur.batters[pos[1]].fielding[1].a++;
            }
            if (game_status.status[7] == 5 || game_status.status[7] == 3) {
                visitor_cur.batters[pos[2]].fielding[2].a++;
                visitor_cur.batters[pos[5]].fielding[5].po++;
            }
            if (game_status.status[7] == 2) {
                visitor_cur.batters[pos[2]].fielding[2].a++;
                if (!(int) ((float) 2 * rand () / (RAND_MAX + 1.0)))
                    visitor_cur.batters[pos[4]].fielding[4].po++;
                else
                    visitor_cur.batters[pos[6]].fielding[6].po++;
            }
        }
    }
    if (action_ind == 7) {
        if (ha)
            home_cur.batters[pos[2]].fielding[2].po++;
        else
            visitor_cur.batters[pos[2]].fielding[2].po++;
    }
    if (action_ind == 9) {
        if (game_status.status[9] || game_status.status[10]) {
            if ((game_status.half_inning % 2) == 0)
                home_cur.batters[pos[whereto]].fielding[whereto].a++;
            else
                visitor_cur.batters[pos[whereto]].fielding[whereto].a++;
            if (game_status.status[9])
                if (game_status.status[2]) {
                    if (ha)
                        home_cur.batters[pos[6]].fielding[6].po++;
                    else
                        visitor_cur.batters[pos[6]].fielding[6].po++;
                }
                else
                    if (game_status.baserunners[1] != 99) {
                        if (ha)
                            home_cur.batters[pos[5]].fielding[5].po++;
                        else
                            visitor_cur.batters[pos[5]].fielding[5].po++;
                    }
                    else {
                        if (ha)
                            home_cur.batters[pos[6]].fielding[6].po++;
                        else
                            visitor_cur.batters[pos[6]].fielding[6].po++;
                    }
            else {
                if (ha)
                    home_cur.batters[pos[2]].fielding[2].po++;
                else
                    visitor_cur.batters[pos[2]].fielding[2].po++;
            }
        }
        else
            if (whereto == 3)
                if (!(int) ((float) 4 * rand () / (RAND_MAX + 1.0))) {
                    /* firstbaseman makes the play unassisted */
                    flip2p = 0;
                    if (ha)
                        home_cur.batters[pos[3]].fielding[3].po++;
                    else
                        visitor_cur.batters[pos[3]].fielding[3].po++;
                }
                else {
                    flip2p = 1;
                    if (ha) {
                        home_cur.batters[pos[3]].fielding[3].a++;
                        home_cur.batters[pos[1]].fielding[1].po++;
                    }
                    else {
                        visitor_cur.batters[pos[3]].fielding[3].a++;
                        visitor_cur.batters[pos[1]].fielding[1].po++;
                    }
                }
            else {
                altpo = 0;
                if (!game_status.status[8]) {
                    if (whereto == 4)
                        if (game_status.baserunners[0] != 99 && (int) ((float) 5 * rand () / (RAND_MAX + 1.0)))
                            altpo = 6;  /* the ss makes the po at 2nd */
                    if (whereto == 6)
                        if (game_status.baserunners[0] != 99 && (int) ((float) 6 * rand () / (RAND_MAX + 1.0)))
                            altpo = 4;  /* the 2b makes the po at 2nd */
                    if (whereto == 5)
                        if (game_status.baserunners[0] != 99 && (int) ((float) 5 * rand () / (RAND_MAX + 1.0)))
                            altpo = 4;  /* the 2b makes the po at 2nd */
                }
                if (ha) {
                    home_cur.batters[pos[whereto]].fielding[whereto].a++;
                    if (altpo)
                        home_cur.batters[pos[altpo]].fielding[altpo].po++;
                    else
                        home_cur.batters[pos[3]].fielding[3].po++;
                }
                else {
                    visitor_cur.batters[pos[whereto]].fielding[whereto].a++;
                    if (altpo)
                        visitor_cur.batters[pos[altpo]].fielding[altpo].po++;
                    else
                        visitor_cur.batters[pos[3]].fielding[3].po++;
                }
            }
    }
    if (action_ind == 19 && whereto && whereto < 10) {
        if (ha) {
            home_cur.batters[pos[4]].fielding[4].po++;
            home_cur.batters[pos[whereto]].fielding[whereto].a++;
        }
        else {
            visitor_cur.batters[pos[4]].fielding[4].po++;
            visitor_cur.batters[pos[whereto]].fielding[whereto].a++;
        }
    }

    if ((action_ind == 10 || action_ind == 12 || action_ind == 23) && whereto && whereto < 10) {
        if (ha)
            home_cur.batters[pos[whereto]].fielding[whereto].po++;
        else
            visitor_cur.batters[pos[whereto]].fielding[whereto].po++;
        if (action_ind == 23) {
            if (ha) {
                home_cur.batters[pos[whereto]].fielding[whereto].a++;
                home_cur.batters[pos[whereto]].fielding[whereto].dp++;
                home_cur.batters[pos[2]].fielding[2].po++;
                home_cur.batters[pos[2]].fielding[2].dp++;
            }
            else {
                visitor_cur.batters[pos[whereto]].fielding[whereto].a++;
                visitor_cur.batters[pos[whereto]].fielding[whereto].dp++;
                visitor_cur.batters[pos[2]].fielding[2].po++;
                visitor_cur.batters[pos[2]].fielding[2].dp++;
            }
        }
    }
    if (action_ind == 11 && whereto && whereto < 10) {
        if (ha) {
            home_cur.batters[pos[whereto]].fielding[whereto].a++;
            home_cur.batters[pos[whereto]].fielding[whereto].dp++;
            home_cur.batters[pos[3]].fielding[3].po++;
            home_cur.batters[pos[3]].fielding[3].dp++;
        }
        else {
            visitor_cur.batters[pos[whereto]].fielding[whereto].a++;
            visitor_cur.batters[pos[whereto]].fielding[whereto].dp++;
            visitor_cur.batters[pos[3]].fielding[3].po++;
            visitor_cur.batters[pos[3]].fielding[3].dp++;
        }
        if (whereto == 5 || whereto == 6)
            if (ha) {
                home_cur.batters[pos[4]].fielding[4].a++;
                home_cur.batters[pos[4]].fielding[4].po++;
                home_cur.batters[pos[4]].fielding[4].dp++;
            }
            else {
                visitor_cur.batters[pos[4]].fielding[4].a++;
                visitor_cur.batters[pos[4]].fielding[4].po++;
                visitor_cur.batters[pos[4]].fielding[4].dp++;
            }
        else
            if (ha) {
                home_cur.batters[pos[6]].fielding[6].a++;
                home_cur.batters[pos[6]].fielding[6].po++;
                home_cur.batters[pos[6]].fielding[6].dp++;
            }
            else {
                visitor_cur.batters[pos[6]].fielding[6].a++;
                visitor_cur.batters[pos[6]].fielding[6].po++;
                visitor_cur.batters[pos[6]].fielding[6].dp++;
            }
    }
}

/* send info necessary for the client to display a boxscore */
void
send_boxscore () {
    char box[3000];
    int inning, x, y, z, last;

    inning = (game_status.half_inning - 1) / 2 + 1;

    /* data id */
    strcpy (&box[0], "BX");

    /* number of innings */
    strcat (&box[0], (char *) cnvt_int2str (2, inning));

    /* runs by inning */
    for (x = 0; x < inning; x++) {
        strcat (&box[0], (char *) cnvt_int2str (2, game_status.inning_score[x][0]));
        strcat (&box[0], (char *) cnvt_int2str (2, game_status.inning_score[x][1]));
    }

    /* left on base */
    strcat (&box[0], (char *) cnvt_int2str (2, game_status.lob[0]));
    strcat (&box[0], (char *) cnvt_int2str (2, game_status.lob[1]));

    /* all players who appeared in the batting order & their position */
    for (x = 0; x < 2; x++) {
        for (y = 0; y < 9; y++) {
            for (last = 0; last < 30; last++)
                if (border[x][y].player[last] == 99) {
                    /* the last shall be first */
                    for (z = (last - 1); z > -1; z--) {
                        strcat (&box[0], (char *) cnvt_int2str (2, border[x][y].player[z]));
                        strcat (&box[0], (char *) cnvt_int2str (2, border[x][y].pos[z]));
                    }
                    break;
                }
            /* delimiter for that batting order position */
            strcat (&box[0], ":");
        }
        /* two consecutive colons ends that team's stats */
        strcat (&box[0], ":");
    }

    /* all pitchers who appeared */
    for (x = 0; x < 2; x++) {
        for (last = 0; last < 15; last++)
            if (pitching[x].pitcher[last] == 99) {
                /* the last shall be first */
                for (z = (last - 1); z > -1; z--)
                    strcat (&box[0], (char *) cnvt_int2str (2, pitching[x].pitcher[z]));
                break;
            }
        strcat (&box[0], ":");
    }

    strcat (&box[0], "\n");
    sock_puts (sock, &box[0]);
    if (netgame)
        sock_puts (netsock, &box[0]);

    send_stats (sock, 'v');   /* send stats               */
    send_stats (sock, 'h');   /*            for this game */

    send_stats (sock, 'a');   /* send stats                */
    send_stats (sock, 'b');   /*            for the season */

    if (netgame) {
        send_stats (netsock, 'v');   /* send stats               */
        send_stats (netsock, 'h');   /*            for this game */

        send_stats (netsock, 'a');   /* send stats                */
        send_stats (netsock, 'b');   /*            for the season */
    }
}

/* update the status of the current game */
void
update_status () {
    int x, hold1, hold2, hpbr1, hpbr2, runs[4], rbi, pruns[4];

    pre_act = game_status;

    earned.h[0] = earned.h[1] = 0;
    hold1 = hold2 = 99;
    for (rbi = x = 0; x < 4; runs[x++] = 99);

    if (action_ind == 2 || action_ind == 18) {
        hold1 = game_status.baserunners[0];
        hpbr1 = game_status.pbr[0];
        if (action_ind == 18 && game_status.status[1])
            game_status.pbr[0] = game_status.baserunners[0] = 99;
        if (action_ind == 2 || (action_ind == 18 && !game_status.status[1])) {
            game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
            if (action_ind != 18)
                rbi++;
        }
        game_status.baserunners[2] = game_status.baserunners[1];
        game_status.pbr[2] = game_status.pbr[1];
        game_status.baserunners[1] = hold1;
        game_status.pbr[1] = hpbr1;

        if (!earned.stop) {
            if (action_ind == 2) {
                earned.h[0] = earned.br[0];
                earned.hpbr[0] = earned.pbr[0];
                earned.br[0] = 1;
                earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
                if (earned.br[2]) {
                    if (!(game_status.half_inning % 2))
                        home_cur.pitchers[earned.pbr[2]].pitching.er++;
                    else
                        visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                }
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.br[1] = earned.h[0];
                earned.pbr[1] = earned.hpbr[0];
            }
            else
                if (++earned.outs >= 3)
                    earned.stop = 1;
        }
    }

    if (action_ind == 3 || action_ind == 13) {
        hold1 = game_status.baserunners[0];
        hpbr1 = game_status.pbr[0];
        hold2 = game_status.baserunners[1];
        hpbr2 = game_status.pbr[1];
        if (action_ind == 3) {
            game_status.baserunners[0] = 99;
            game_status.baserunners[1] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[1] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
        else {
            game_status.baserunners[1] = 99;
            game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
            rbi++;
        }
        if (hold2 != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[1] = hold2;
            pruns[1] = hpbr2;
            chk_wls (hpbr2);
            rbi++;
        }
        game_status.baserunners[2] = hold1;
        game_status.pbr[2] = hpbr1;

        if (!earned.stop) {
            earned.h[0] = earned.br[0];
            earned.h[1] = earned.br[1];
            earned.hpbr[0] = earned.pbr[0];
            earned.hpbr[1] = earned.pbr[1];
            if (action_ind == 3) {
                earned.br[0] = 0;
                earned.br[1] = 1;
                earned.pbr[1] = game_status.pitcher[!(game_status.half_inning % 2)];
            }
            else {
                earned.br[1] = 0;
                earned.br[0] = 1;
                earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
            }
            if (earned.br[2]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.pbr[2]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
            }
            if (earned.h[1]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.hpbr[1]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.hpbr[1]].pitching.er++;
            }
            earned.br[2] = earned.h[0];
            earned.pbr[2] = earned.hpbr[0];
        }
    }

    if (action_ind == 4 || action_ind == 14) {
        for (x = 2; x >= 0; x--) {                                /* process runs in order as they score */
            if (game_status.baserunners[x] != 99) {
                game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                runs[x] = game_status.baserunners[x];
                pruns[x] = game_status.pbr[x];
                chk_wls (game_status.pbr[x]);
                rbi++;
            }
            game_status.baserunners[x] = 99;
        }
        if (action_ind == 4) {
            game_status.baserunners[2] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[2] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
        else {
            game_status.baserunners[1] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[1] = game_status.pitcher[!(game_status.half_inning % 2)];
        }

        if (!earned.stop) {
            for (x = 0; x < 3; x++) {
                if (earned.br[x]) {
                    if (!(game_status.half_inning % 2))
                        home_cur.pitchers[earned.pbr[x]].pitching.er++;
                    else
                        visitor_cur.pitchers[earned.pbr[x]].pitching.er++;
                }
                earned.br[x] = 0;
            }
            if (action_ind == 4) {
                earned.br[2] = 1;
                earned.pbr[2] = game_status.pitcher[!(game_status.half_inning % 2)];
            }
            else {
                earned.br[1] = 1;
                earned.pbr[1] = game_status.pitcher[!(game_status.half_inning % 2)];
            }
        }
    }

    if (action_ind == 5) {
        for (x = 2; x >= 0; x--) {                                /* process runs in order as they score */
            if (game_status.baserunners[x] != 99) {
                game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                runs[x] = game_status.baserunners[x];
                pruns[x] = game_status.pbr[x];
                chk_wls (game_status.pbr[x]);
                rbi++;
            }
            game_status.baserunners[x] = 99;
        }
        game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
        runs[3] = game_status.batter[game_status.half_inning % 2];
        pruns[3] = game_status.pitcher[!(game_status.half_inning % 2)];
        chk_wls (game_status.pitcher[!(game_status.half_inning % 2)]);
        rbi++;

        if (!earned.stop) {
            if (!(game_status.half_inning % 2))
                home_cur.pitchers[game_status.pitcher[1]].pitching.er++;
            else
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.er++;
            for (x = 0; x < 3; x++) {
                if (earned.br[x]) {
                    if (!(game_status.half_inning % 2))
                        home_cur.pitchers[earned.pbr[x]].pitching.er++;
                    else
                        visitor_cur.pitchers[earned.pbr[x]].pitching.er++;
                }
                earned.br[x] = 0;
            }
        }
    }

    if (action_ind == 6 || action_ind == 8) {
        if (game_status.baserunners[0] != 99) {
            hold1 = game_status.baserunners[0];
            hpbr1 = game_status.pbr[0];
            if (game_status.baserunners[1] != 99) {
                hold2 = game_status.baserunners[1];
                hpbr2 = game_status.pbr[1];
                if (game_status.baserunners[2] != 99) {
                    game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                    runs[2] = game_status.baserunners[2];
                    pruns[2] = game_status.pbr[2];
                    chk_wls (game_status.pbr[2]);
                    rbi++;
                }
                game_status.baserunners[2] = hold2;
                game_status.pbr[2] = hpbr2;
            }
            game_status.baserunners[1] = hold1;
            game_status.pbr[1] = hpbr1;
        }
        game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
        game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];

        if (!earned.stop) {
            if (earned.br[0]) {
                earned.h[0] = earned.br[0];
                earned.hpbr[0] = earned.pbr[0];
                if (earned.br[1]) {
                    earned.h[1] = earned.br[1];
                    earned.hpbr[1] = earned.pbr[1];
                    if (earned.br[2]) {
                        if (!(game_status.half_inning % 2))
                            home_cur.pitchers[earned.pbr[2]].pitching.er++;
                        else
                            visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                    }
                    earned.br[2] = earned.h[1];
                    earned.pbr[2] = earned.hpbr[1];
                }
                earned.br[1] = earned.h[0];
                earned.pbr[1] = earned.hpbr[0];
            }
            earned.br[0] = 1;
            earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
    }

    if (action_ind == 9) {
        if ((game_status.status[9] || game_status.status[10]) && game_status.outs < 2) {
            /* unsuccessful sacrifice bunt or squeeze play */
            if (game_status.baserunners[2] != 99) {
                game_status.baserunners[2] = game_status.baserunners[1];
                game_status.pbr[2] = game_status.pbr[1];
                game_status.baserunners[1] = game_status.baserunners[0];
                game_status.pbr[1] = game_status.pbr[0];
            }
            else
                if (game_status.baserunners[1] != 99) {
                    game_status.baserunners[1] = game_status.baserunners[0];
                    game_status.pbr[1] = game_status.pbr[0];
                }
            game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
            game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
        }
        else
            if (game_status.status[2] && game_status.baserunners[2] != 99) {
                /* infield is in */
                if ((game_status.status[2] == 1 && (whereto == 5 || whereto == 3)) || (game_status.status[2] == 2 && (whereto > 2 && whereto < 7)))
                    rcd = 1;
                else {
                    if (game_status.outs < 2) {
                        game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                        runs[2] = game_status.baserunners[2];
                        pruns[2] = game_status.pbr[2];
                        chk_wls (game_status.pbr[2]);
                        rbi++;
                    }
                }
                game_status.baserunners[2] = game_status.baserunners[1];
                game_status.pbr[2] = game_status.pbr[1];
                if (altpo)
                    /* the runner on first is out going to second */
                    game_status.pbr[1] = game_status.baserunners[1] = 99;
                else {
                    game_status.baserunners[1] = game_status.baserunners[0];
                    game_status.pbr[1] = game_status.pbr[0];
                }
                if (rcd || altpo) {
                    game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
                    game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
                }
            }
            else
                if (game_status.outs < 2) {
                    if (game_status.baserunners[2] != 99) {
                        game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                        runs[2] = game_status.baserunners[2];
                        pruns[2] = game_status.pbr[2];
                        chk_wls (game_status.pbr[2]);
                        rbi++;
                    }
                    game_status.baserunners[2] = game_status.baserunners[1];
                    game_status.pbr[2] = game_status.pbr[1];
                    if (altpo) {
                        /* the runner on first is out going to second */
                        game_status.pbr[1] = game_status.baserunners[1] = 99;
                        game_status.baserunners[0] = game_status.batter[game_status.half_inning % 2];
                        game_status.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
                    }
                    else {
                        game_status.baserunners[1] = game_status.baserunners[0];
                        game_status.pbr[1] = game_status.pbr[0];
                        game_status.baserunners[0] = 99;
                    }
                }
        if (!earned.stop) {
            if ((game_status.status[9] || game_status.status[10]) && earned.outs < 2) {
                if (earned.br[2]) {
                    earned.br[2] = earned.br[1];
                    earned.pbr[2] = earned.pbr[1];
                    earned.br[1] = earned.br[0];
                    earned.pbr[1] = earned.pbr[0];
                }
                else
                    if (earned.br[1]) {
                        earned.br[1] = earned.br[0];
                        earned.pbr[1] = earned.pbr[0];
                    }
                earned.br[0] = 1;
                earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
            }
            else
                if (game_status.status[2] && earned.br[2]) {
                    if ((game_status.status[2] == 1 && (whereto == 5 || whereto == 3)) || (game_status.status[2] == 2 && (whereto > 2 && whereto < 7))) {
                        earned.br[0] = 1;
                        earned.pbr[2] = earned.br[2] = 0;
                        earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
                    }
                    else {
                        if (earned.outs < 2) {
                            if (!(game_status.half_inning % 2))
                                home_cur.pitchers[earned.pbr[2]].pitching.er++;
                            else
                                visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                        }
                    }

                    earned.br[2] = earned.br[1];
                    earned.pbr[2] = earned.pbr[1];
                    if (altpo)
                        /* the runner on first is out going to second */
                        earned.br[1] = 0;
                    else {
                        earned.br[1] = earned.br[0];
                        earned.pbr[1] = earned.pbr[0];
                    }
                    if (rcd || altpo)
                        earned.br[0] = 1;
                }
                else
                    if (earned.outs < 2) {
                        if (earned.br[2]) {
                            if (!(game_status.half_inning % 2))
                                home_cur.pitchers[earned.pbr[2]].pitching.er++;
                            else
                                visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                        }
                        earned.br[2] = earned.br[1];
                        earned.pbr[2] = earned.pbr[1];
                        if (altpo) {
                            earned.pbr[1] = earned.br[1] = 0;
                            earned.br[0] = 1;
                            earned.pbr[0] = game_status.pitcher[!(game_status.half_inning % 2)];
                        }
                        else {
                            earned.br[1] = earned.br[0];
                            earned.pbr[1] = earned.pbr[0];
                            earned.pbr[0] = earned.br[0] = 0;
                        }
                    }
        }
    }

    if (action_ind == 11 && !game_status.outs) {
        game_status.baserunners[0] = 99;
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
        }
        game_status.baserunners[2] = game_status.baserunners[1];
        game_status.pbr[2] = game_status.pbr[1];
        game_status.baserunners[1] = 99;

        if (!earned.outs && !earned.stop) {
            earned.pbr[0] = earned.br[0] = 0;
            if (earned.br[2]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.pbr[2]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
            }
            earned.br[2] = earned.br[1];
            earned.pbr[2] = earned.pbr[1];
            earned.pbr[1] = earned.br[1] = 0;
        }
    }

    if (action_ind == 12 && game_status.outs < 2) {
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
            rbi++;
            game_status.baserunners[2] = 99;
        }
        if (earned.outs < 2 && !earned.stop)
            if (earned.br[2]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.pbr[2]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                earned.pbr[2] = earned.br[2] = 0;
            }
    }

    if (action_ind == 23 && !game_status.outs) {
        if (game_status.baserunners[2] != 99)
            game_status.baserunners[2] = 99;
        if (!earned.outs && !earned.stop)
            earned.pbr[2] = earned.br[2] = 0;
    }

    if (action_ind > 14 && action_ind < 18) {
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
        }
        game_status.baserunners[2] = game_status.baserunners[1];
        game_status.pbr[2] = game_status.pbr[1];
        game_status.baserunners[1] = game_status.baserunners[0];
        game_status.pbr[1] = game_status.pbr[0];
        game_status.baserunners[0] = 99;

        if (!earned.stop && action_ind != 17) {
            if (earned.br[2]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.pbr[2]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
            }
            earned.br[2] = earned.br[1];
            earned.pbr[2] = earned.pbr[1];
            earned.br[1] = earned.br[0];
            earned.pbr[1] = earned.pbr[0];
            earned.pbr[0] = earned.br[0] = 0;
        }
    }

    if (action_ind == 19) {
        if (game_status.baserunners[2] != 99) {
            game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
            runs[2] = game_status.baserunners[2];
            pruns[2] = game_status.pbr[2];
            chk_wls (game_status.pbr[2]);
            rbi++;
        }
        game_status.baserunners[2] = game_status.baserunners[1];
        game_status.pbr[2] = game_status.pbr[1];
        game_status.baserunners[1] = game_status.baserunners[0];
        game_status.pbr[1] = game_status.pbr[0];
        game_status.baserunners[0] = 99;

        if (!earned.stop) {
            if (earned.br[2]) {
                if (!(game_status.half_inning % 2))
                    home_cur.pitchers[earned.pbr[2]].pitching.er++;
                else
                    visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
            }
            earned.br[2] = earned.br[1];
            earned.pbr[2] = earned.pbr[1];
            earned.br[1] = earned.br[0];
            earned.pbr[1] = earned.pbr[0];
            earned.pbr[0] = earned.br[0] = 0;
        }
    }

    if (action_ind == 20) {
        if (game_status.status[1] == 3) {
            game_status.baserunners[2] = 99;
            earned.br[2] = 0;
        }
        else
            if (game_status.status[1] == 2) {
                game_status.baserunners[1] = 99;
                earned.br[1] = 0;
            }
            else {
                game_status.baserunners[0] = 99;
                earned.br[0] = 0;
            }
        game_status.status[0] = game_status.status[7] = game_status.status[8] = game_status.status[9] = 0;
    }

    if (action_ind == 21) {
        if (game_status.status[7] == 9 || game_status.status[7] == 7 || game_status.status[7] == 6 || game_status.status[7] == 4)
            if (game_status.baserunners[2] != 99) {
                game_status.inning_score[game_status.half_inning / 2][game_status.half_inning % 2]++;
                runs[2] = game_status.baserunners[2];
                pruns[2] = game_status.pbr[2];
                chk_wls (game_status.pbr[2]);
            }
        if (game_status.status[7] == 9) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = 99;
        }
        if (game_status.status[7] == 7) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = 99;
        }
        if (game_status.status[7] == 6) {
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = game_status.baserunners[2] = 99;
        }
        if (game_status.status[7] == 5) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = 99;
        }
        if (game_status.status[7] == 4)
            game_status.baserunners[2] = 99;
        if (game_status.status[7] == 3) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = 99;
        }
        if (game_status.status[7] == 2) {
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = 99;
        }

        if (!earned.stop) {
            if (game_status.status[7] == 9 || game_status.status[7] == 7 || game_status.status[7] == 6 || game_status.status[7] == 4)
                if (earned.br[2]) {
                    if (!(game_status.half_inning % 2))
                        home_cur.pitchers[earned.pbr[2]].pitching.er++;
                    else
                        visitor_cur.pitchers[earned.pbr[2]].pitching.er++;
                }
            if (game_status.status[7] == 9) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.br[0] = 0;
            }
            if (game_status.status[7] == 7) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.pbr[1] = earned.br[1] = 0;
            }
            if (game_status.status[7] == 6) {
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.pbr[2] = earned.br[0] = earned.br[2] = 0;
            }
            if (game_status.status[7] == 5) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.br[0] = 0;
            }
            if (game_status.status[7] == 4)
                earned.br[2] = 0;
            if (game_status.status[7] == 3) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.pbr[1] = earned.br[1] = 0;
            }
            if (game_status.status[7] == 2) {
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.br[0] = 0;
            }
        }
    }

    if (action_ind == 22) {
        if (game_status.status[7] == 9) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = 99;
        }
        if (game_status.status[7] == 7) {
            game_status.baserunners[2] = game_status.baserunners[1];
            game_status.pbr[2] = game_status.pbr[1];
            game_status.baserunners[1] = 99;
        }
        if (game_status.status[7] == 6) {
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[0] = game_status.baserunners[2] = 99;
        }
        if (game_status.status[7] == 5) {
            game_status.baserunners[1] = game_status.baserunners[0];
            game_status.pbr[1] = game_status.pbr[0];
            game_status.baserunners[2] = game_status.baserunners[0] = 99;
        }
        if (game_status.status[7] == 4)
            game_status.baserunners[2] = 99;
        if (game_status.status[7] == 3)
            game_status.baserunners[2] = game_status.baserunners[1] = 99;
        if (game_status.status[7] == 2)
            game_status.baserunners[1] = game_status.baserunners[0] = 99;

        if (!earned.stop) {
            if (game_status.status[7] == 9) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.br[0] = 0;
            }
            if (game_status.status[7] == 7) {
                earned.br[2] = earned.br[1];
                earned.pbr[2] = earned.pbr[1];
                earned.pbr[1] = earned.br[1] = 0;
            }
            if (game_status.status[7] == 6) {
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[0] = earned.pbr[2] = earned.br[0] = earned.br[2] = 0;
            }
            if (game_status.status[7] == 5) {
                earned.br[1] = earned.br[0];
                earned.pbr[1] = earned.pbr[0];
                earned.pbr[2] = earned.pbr[0] = earned.br[2] = earned.br[0] = 0;
            }
            if (game_status.status[7] == 4)
                earned.pbr[2] = earned.br[2] = 0;
            if (game_status.status[7] == 3)
                earned.pbr[2] = earned.pbr[1] = earned.br[2] = earned.br[1] = 0;
            if (game_status.status[7] == 2)
                earned.pbr[1] = earned.pbr[0] = earned.br[1] = earned.br[0] = 0;
        }
    }

    /* update the stats for runs scored & rbi's */
    update_more_stats (runs, pruns, rbi);
    /* point to the next hitter */
    if (action_ind < 15 || action_ind == 19 || action_ind == 23 || (action_ind == 18 && !game_status.status[1])) {
        game_status.status[13] = game_status.status[14] = 0;
        if (++game_status.batter[game_status.half_inning % 2] == 9)
            game_status.batter[game_status.half_inning % 2] = 0;
    }

    /* add to outs if appropriate, go to next inning if 3 outs & check for end of game */
    if (action_ind == 7 || action_ind == 9 || action_ind == 10 || action_ind == 11 || action_ind == 12 ||
        action_ind == 20 || action_ind == 19 || action_ind == 22 || action_ind == 23) {
        if ((action_ind == 11 || action_ind == 23) && game_status.outs < 2) {
            game_status.outs++;
            earned.outs++;
        }
        if (++earned.outs >= 3)
            earned.stop = 1;
        if (++game_status.outs == 3) {
            for (x = 0; x < 3; x++)
                if (game_status.baserunners[x] != 99)
                    game_status.lob[game_status.half_inning % 2]++;
            game_status.outs = 0;
            for (x = 0; x < 3; x++)
                game_status.baserunners[x] = 99;
            earned.stop = earned.outs = 0;
            for (x = 0; x < 3; x++)
                earned.br[x] = 0;
            game_status.half_inning++;
        }
    }

    /* mark to update top half of user interface display */
    tind = 1;

    post_act = game_status;
}

/* update stats */
void
update_stats () {
    int x, innings, vruns, hruns, y, pos2, ha;

    innings = (game_status.half_inning - 1) / 2 + 1;
    for (vruns = hruns = x = 0; x < innings; x++) {
        vruns += game_status.inning_score[x][0];
        hruns += game_status.inning_score[x][1];
    }
    /* this is a pre-emptive check ... if it's the home half of the 9th or later inning and the home team is about to win the
       game with the current action then ensure no unnecessary additional runs are counted */
    if (innings >= 9 && (game_status.half_inning % 2) == 1 && (action_ind == 3 || action_ind == 4 || action_ind == 13 || action_ind == 14)) {
        if (vruns == hruns) {
            if (game_status.baserunners[2] != 99)
                action_ind = 2;
            else
                if (game_status.baserunners[1] != 99 && action_ind == 14)
                    action_ind = 13;
        }
        if ((vruns - 1) == hruns)
            if (game_status.baserunners[2] != 99 && game_status.baserunners[1] != 99)
                if (action_ind == 4 || action_ind == 14)
                    action_ind = 3;
    }

    if ((game_status.half_inning % 2) == 0)
        ha = 1;
    else
        ha = 0;

    for (y = 0; y < 9; y++)
        if (border[ha][y].pos[0] == 2) {
            pos2 = border[ha][y].player[0];
            break;
        }

    if (!action_ind) {
        if (visitor_cur.pitchers[game_status.pitcher[0]].pitching.games_started == 1) {
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.cg = 1;
            if (hruns == 0)
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.sho = 1;
        }
        else
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.gf = 1;
        if (home_cur.pitchers[game_status.pitcher[1]].pitching.games_started == 1) {
            home_cur.pitchers[game_status.pitcher[1]].pitching.cg = 1;
            if (vruns == 0)
                home_cur.pitchers[game_status.pitcher[1]].pitching.sho = 1;
        }
        else
            home_cur.pitchers[game_status.pitcher[1]].pitching.gf = 1;

        if (forfeit != -1)
            if (pwin != 99)
                if (vruns > hruns)
                    visitor_cur.pitchers[pwin].pitching.wins = 1;
                else
                    home_cur.pitchers[pwin].pitching.wins = 1;
            else
                if (vruns > hruns)
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.wins = 1;
                else
                    home_cur.pitchers[game_status.pitcher[1]].pitching.wins = 1;
        else
            if (pwin != 99)
                if (tforfeit)
                    visitor_cur.pitchers[pwin].pitching.wins = 1;
                else
                    home_cur.pitchers[pwin].pitching.wins = 1;
            else
                if (tforfeit)
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.wins = 1;
                else
                    home_cur.pitchers[game_status.pitcher[1]].pitching.wins = 1;


        if (forfeit != -1) {
            if (savei)
                if (vruns > hruns)
                    visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.saves = visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.svopp = 1;
                else
                    home_cur.pitchers[pitching[1].pitcher[0]].pitching.saves = home_cur.pitchers[pitching[1].pitcher[0]].pitching.svopp = 1;
            else
                if (vruns > hruns) {
                    if (saver < 4 && visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.innings >= 1)
                        visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.saves = visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.svopp = 1;
                }
                else
                    if (saver < 4 && home_cur.pitchers[pitching[1].pitcher[0]].pitching.innings >= 1)
                        home_cur.pitchers[pitching[1].pitcher[0]].pitching.saves = home_cur.pitchers[pitching[1].pitcher[0]].pitching.svopp = 1;
        }
        else
            if (savei) {
                if (vruns > hruns) {
                    if (tforfeit)
                        visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.saves = visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.svopp = 1;
                }
                else
                    if (!tforfeit)
                        home_cur.pitchers[pitching[1].pitcher[0]].pitching.saves = home_cur.pitchers[pitching[1].pitcher[0]].pitching.svopp = 1;
            }
            else
                if (vruns > hruns) {
                    if (saver < 4 && visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.innings >= 1)
                        if (tforfeit)
                            visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.saves = visitor_cur.pitchers[pitching[0].pitcher[0]].pitching.svopp = 1;
                }
                else
                    if (saver < 4 && home_cur.pitchers[pitching[1].pitcher[0]].pitching.innings >= 1)
                        if (!tforfeit)
                            home_cur.pitchers[pitching[1].pitcher[0]].pitching.saves = home_cur.pitchers[pitching[1].pitcher[0]].pitching.svopp = 1;

        if (forfeit != -1)
            if (ploss != 99)
                if (vruns < hruns)
                    visitor_cur.pitchers[ploss].pitching.losses = 1;
                else
                    home_cur.pitchers[ploss].pitching.losses = 1;
            else
                if (vruns < hruns)
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.losses = 1;
                else
                    home_cur.pitchers[game_status.pitcher[1]].pitching.losses = 1;
        else
            if (ploss != 99)
                if (!tforfeit)
                    visitor_cur.pitchers[ploss].pitching.losses = 1;
                else
                    home_cur.pitchers[ploss].pitching.losses = 1;
            else
                if (!tforfeit)
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.losses = 1;
                else
                    home_cur.pitchers[game_status.pitcher[1]].pitching.losses = 1;

    }

    if (action_ind == 1) {
        visitor_cur.pitchers[game_status.pitcher[0]].pitching.games = 1;
        visitor_cur.pitchers[game_status.pitcher[0]].pitching.games_started = 1;
        home_cur.pitchers[game_status.pitcher[1]].pitching.games = 1;
        home_cur.pitchers[game_status.pitcher[1]].pitching.games_started = 1;

        for (x = 0; x < 10; x++) {
            if (starters[0][x] != 99) {
                visitor_cur.batters[starters[0][x]].hitting.games = 1;
                visitor_cur.batters[starters[0][x]].fielding[x].games = 1;
            }
            if (starters[1][x] != 99) {
                home_cur.batters[starters[1][x]].hitting.games = 1;
                home_cur.batters[starters[1][x]].fielding[x].games = 1;
            }
        }
    }

    if (action_ind == 2 || action_ind == 13) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            hi++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            hi++;
        }
    }

    if (action_ind == 18) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
        }
    }

    if (action_ind == 3 || action_ind == 14) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.doubles++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.doubles++;
            hi++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.doubles++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.doubles++;
            hi++;
        }
    }

    if (action_ind == 4) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.triples++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.triples++;
            hi++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.triples++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.triples++;
            hi++;
        }
    }

    if (action_ind == 5) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.homers++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.hits++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.homers++;
            hi++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.homers++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.hits++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.homers++;
            hi++;
        }

        /* this is a pre-emptive check ... if it's the home half of the 9th or later inning and the home team is about to win the
           game while coming from behind with a homerun then we're going to alert the client so it can play a certain sound bite */
        if (innings >= 9 && (game_status.half_inning % 2) == 1) 
            if ((vruns - 3) == hruns)
                if (game_status.baserunners[0] != 99 && game_status.baserunners[1] != 99 && game_status.baserunners[2] != 99)
                    JBsw = 1;
    }
    if (action_ind == 6) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.bb++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.walks++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            if (game_status.status[4]) {
                visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.ibb++;
                home_cur.pitchers[game_status.pitcher[1]].pitching.ibb++;
            }
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.bb++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.walks++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            if (game_status.status[4]) {
                home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.ibb++;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.ibb++;
            }
        }
    }

    if (action_ind == 15) {
        if ((game_status.half_inning % 2) == 0)
            home_cur.pitchers[game_status.pitcher[1]].pitching.wp++;
        else
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.wp++;
    }

    if (action_ind == 17) {
        if (ha)
            home_cur.batters[pos2].fielding[2].pb++;
        else
            visitor_cur.batters[pos2].fielding[2].pb++;
    }

    if (action_ind == 16) {
        if ((game_status.half_inning % 2) == 0)
            home_cur.pitchers[game_status.pitcher[1]].pitching.balks++;
        else
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.balks++;
    }

    if (action_ind == 7) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.so++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.so++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.so++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.so++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
        }
    }

    if (action_ind == 7 || action_ind == 20) {
        if ((game_status.half_inning % 2) == 0) {
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }
    }

    if (action_ind == 8) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.hbp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.hb++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.hbp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.hb++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
        }
    }

    if (action_ind == 19) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.sh++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.sh++;
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.sh++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.sh++;
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }
    }

    if (action_ind == 9 || action_ind == 10) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }
    }

    if (action_ind == 11 || action_ind == 23) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.atbats++;
            if (action_ind == 11)
                visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.gidp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.opp_ab++;
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.atbats++;
            if (action_ind == 11)
                home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.gidp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.opp_ab++;
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }
    }

    if (action_ind == 12) {
        if ((game_status.half_inning % 2) == 0) {
            visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.sf++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.sf++;
            home_cur.pitchers[game_status.pitcher[1]].pitching.bfp++;
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.sf++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.sf++;
            visitor_cur.pitchers[game_status.pitcher[0]].pitching.bfp++;
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }
    }

    if (action_ind == 22) {
        if ((game_status.half_inning % 2) == 0) {
            if (++home_cur.pitchers[game_status.pitcher[1]].pitching.thirds == 3) {
                home_cur.pitchers[game_status.pitcher[1]].pitching.thirds = 0;
                home_cur.pitchers[game_status.pitcher[1]].pitching.innings++;
            }
            if (++pitching[1].thirds[0] == 3) {
                pitching[1].thirds[0] = 0;
                pitching[1].innings[0]++;
            }
        }
        else {
            if (++visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds == 3) {
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.thirds = 0;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings++;
            }
            if (++pitching[0].thirds[0] == 3) {
                pitching[0].thirds[0] = 0;
                pitching[0].innings[0]++;
            }
        }

        if (game_status.status[7] == 9 || game_status.status[7] == 7 ||
                game_status.status[7] == 6 || game_status.status[7] == 4) {
            if ((game_status.half_inning % 2) == 0) {
                visitor_cur.batters[border[0][game_status.baserunners[2]].player[0]].hitting.cs++;
                home_cur.pitchers[game_status.pitcher[1]].pitching.cs++;
            }
            else {
                home_cur.batters[border[1][game_status.baserunners[2]].player[0]].hitting.cs++;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.cs++;
            }
        }
        if (game_status.status[7] == 5 || game_status.status[7] == 3) {
            if ((game_status.half_inning % 2) == 0) {
                visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.cs++;
                home_cur.pitchers[game_status.pitcher[1]].pitching.cs++;
            }
            else {
                home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.cs++;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.cs++;
            }
        }
        if (game_status.status[7] == 2) {
            if ((game_status.half_inning % 2) == 0) {
                visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.cs++;
                home_cur.pitchers[game_status.pitcher[1]].pitching.cs++;
            }
            else {
                home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.cs++;
                visitor_cur.pitchers[game_status.pitcher[0]].pitching.cs++;
            }
        }

        if (game_status.outs != 2)
            switch (game_status.status[7]) {
                case 7:
                    if ((game_status.half_inning % 2) == 0) {
                        visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                        home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                    }
                    else {
                        home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                        visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                    }
                    break;
                case 9:
                    if ((game_status.half_inning % 2) == 0) {
                        visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                        home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                    }
                    else {
                        home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                        visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                    }
                case 6:
                case 5:
                    if ((game_status.half_inning % 2) == 0) {
                        visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb++;
                        home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                    }
                    else {
                        home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb++;
                        visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                    }
            }
    }

    if (action_ind == 21)
        switch (game_status.status[7]) {
            case 9:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[2]].player[0]].hitting.sb++;
                    visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                    visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb += 3;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[2]].player[0]].hitting.sb++;
                    home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                    home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb += 3;
                }
                break;
            case 7:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[2]].player[0]].hitting.sb++;
                    visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb += 2;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[2]].player[0]].hitting.sb++;
                    home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb += 2;
                }
                break;
            case 6:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[2]].player[0]].hitting.sb++;
                    visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb += 2;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[2]].player[0]].hitting.sb++;
                    home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb += 2;
                }
                break;
            case 5:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                    visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb += 2;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                    home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb += 2;
                }
                break;
            case 4:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[2]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[2]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                }
                break;
            case 3:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[1]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[1]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                }
                break;
            case 2:
                if ((game_status.half_inning % 2) == 0) {
                    visitor_cur.batters[border[0][game_status.baserunners[0]].player[0]].hitting.sb++;
                    home_cur.pitchers[game_status.pitcher[1]].pitching.sb++;
                }
                else {
                    home_cur.batters[border[1][game_status.baserunners[0]].player[0]].hitting.sb++;
                    visitor_cur.pitchers[game_status.pitcher[0]].pitching.sb++;
                }
        }
}

void
update_more_stats (int rs[], int prs[], int rbi) {
    int x;

    for (x = 0; x < 4; x++)
        if (rs[x] != 99) {
            if ((game_status.half_inning % 2) == 0) {
                visitor_cur.batters[border[0][rs[x]].player[0]].hitting.runs++;
                home_cur.pitchers[prs[x]].pitching.runs++;
                ri++;
            }
            else {
                home_cur.batters[border[1][rs[x]].player[0]].hitting.runs++;
                visitor_cur.pitchers[prs[x]].pitching.runs++;
                ri++;
            }
        }

    if ((game_status.half_inning % 2) == 0) {
        visitor_cur.batters[border[0][game_status.batter[0]].player[0]].hitting.rbi += rbi;
        home_cur.pitchers[game_status.pitcher[1]].pitching.rbi += rbi;
    }
    else {
        home_cur.batters[border[1][game_status.batter[1]].player[0]].hitting.rbi += rbi;
        visitor_cur.pitchers[game_status.pitcher[0]].pitching.rbi += rbi;
    }
}

void
accumulate_stats () {
    int x, y;

    for (x = 0; x < maxplayers[0]; x++) {
        visitor_season.batters[x].hitting.games += visitor_cur.batters[x].hitting.games;
        visitor_season.batters[x].hitting.atbats += visitor_cur.batters[x].hitting.atbats;
        visitor_season.batters[x].hitting.runs += visitor_cur.batters[x].hitting.runs;
        visitor_season.batters[x].hitting.hits += visitor_cur.batters[x].hitting.hits;
        visitor_season.batters[x].hitting.doubles += visitor_cur.batters[x].hitting.doubles;
        visitor_season.batters[x].hitting.triples += visitor_cur.batters[x].hitting.triples;
        visitor_season.batters[x].hitting.homers += visitor_cur.batters[x].hitting.homers;
        visitor_season.batters[x].hitting.rbi += visitor_cur.batters[x].hitting.rbi;
        visitor_season.batters[x].hitting.bb += visitor_cur.batters[x].hitting.bb;
        visitor_season.batters[x].hitting.so += visitor_cur.batters[x].hitting.so;
        visitor_season.batters[x].hitting.hbp += visitor_cur.batters[x].hitting.hbp;
        visitor_season.batters[x].hitting.gidp += visitor_cur.batters[x].hitting.gidp;
        visitor_season.batters[x].hitting.sb += visitor_cur.batters[x].hitting.sb;
        visitor_season.batters[x].hitting.cs += visitor_cur.batters[x].hitting.cs;
        visitor_season.batters[x].hitting.ibb += visitor_cur.batters[x].hitting.ibb;
        visitor_season.batters[x].hitting.sh += visitor_cur.batters[x].hitting.sh;
        visitor_season.batters[x].hitting.sf += visitor_cur.batters[x].hitting.sf;
        visitor_season.batters[x].id.injury += visitor_cur.batters[x].id.injury;
        if (visitor_season.batters[x].id.injury)
            visitor_season.batters[x].id.injury--;

        for (y = 0; y < 11; y++) {
            visitor_season.batters[x].fielding[y].games += visitor_cur.batters[x].fielding[y].games;
            visitor_season.batters[x].fielding[y].po += visitor_cur.batters[x].fielding[y].po;
            visitor_season.batters[x].fielding[y].dp += visitor_cur.batters[x].fielding[y].dp;
            visitor_season.batters[x].fielding[y].a += visitor_cur.batters[x].fielding[y].a;
            visitor_season.batters[x].fielding[y].pb += visitor_cur.batters[x].fielding[y].pb;
            visitor_season.batters[x].fielding[y].e += visitor_cur.batters[x].fielding[y].e;
        }
    }

    for (x = 0; x < maxplayers[1]; x++) {
        home_season.batters[x].hitting.games += home_cur.batters[x].hitting.games;
        home_season.batters[x].hitting.atbats += home_cur.batters[x].hitting.atbats;
        home_season.batters[x].hitting.runs += home_cur.batters[x].hitting.runs;
        home_season.batters[x].hitting.hits += home_cur.batters[x].hitting.hits;
        home_season.batters[x].hitting.doubles += home_cur.batters[x].hitting.doubles;
        home_season.batters[x].hitting.triples += home_cur.batters[x].hitting.triples;
        home_season.batters[x].hitting.homers += home_cur.batters[x].hitting.homers;
        home_season.batters[x].hitting.rbi += home_cur.batters[x].hitting.rbi;
        home_season.batters[x].hitting.bb += home_cur.batters[x].hitting.bb;
        home_season.batters[x].hitting.so += home_cur.batters[x].hitting.so;
        home_season.batters[x].hitting.hbp += home_cur.batters[x].hitting.hbp;
        home_season.batters[x].hitting.gidp += home_cur.batters[x].hitting.gidp;
        home_season.batters[x].hitting.sb += home_cur.batters[x].hitting.sb;
        home_season.batters[x].hitting.cs += home_cur.batters[x].hitting.cs;
        home_season.batters[x].hitting.ibb += home_cur.batters[x].hitting.ibb;
        home_season.batters[x].hitting.sh += home_cur.batters[x].hitting.sh;
        home_season.batters[x].hitting.sf += home_cur.batters[x].hitting.sf;
        home_season.batters[x].id.injury += home_cur.batters[x].id.injury;
        if (home_season.batters[x].id.injury)
            home_season.batters[x].id.injury--;

        for (y = 0; y < 11; y++) {
            home_season.batters[x].fielding[y].games += home_cur.batters[x].fielding[y].games;
            home_season.batters[x].fielding[y].po += home_cur.batters[x].fielding[y].po;
            home_season.batters[x].fielding[y].dp += home_cur.batters[x].fielding[y].dp;
            home_season.batters[x].fielding[y].a += home_cur.batters[x].fielding[y].a;
            home_season.batters[x].fielding[y].pb += home_cur.batters[x].fielding[y].pb;
            home_season.batters[x].fielding[y].e += home_cur.batters[x].fielding[y].e;
        }
    }

    for (x = 0; x < maxpitchers[0]; x++) {
        if (visitor_cur.pitchers[x].pitching.games_started)
            visitor_season.pitchers[x].id.starts_rest = 0;
        else
            visitor_season.pitchers[x].id.starts_rest++;

        visitor_season.pitchers[x].id.ip_last4g[3] = visitor_season.pitchers[x].id.ip_last4g[2];
        visitor_season.pitchers[x].id.ip_last4g[2] = visitor_season.pitchers[x].id.ip_last4g[1];
        visitor_season.pitchers[x].id.ip_last4g[1] = visitor_season.pitchers[x].id.ip_last4g[0];
        if (visitor_cur.pitchers[x].pitching.games)
            visitor_season.pitchers[x].id.ip_last4g[0] = visitor_cur.pitchers[x].pitching.innings;
        else
            visitor_season.pitchers[x].id.ip_last4g[0] = 0;

        visitor_season.pitchers[x].pitching.games += visitor_cur.pitchers[x].pitching.games;
        visitor_season.pitchers[x].pitching.games_started += visitor_cur.pitchers[x].pitching.games_started;
        visitor_season.pitchers[x].pitching.innings += visitor_cur.pitchers[x].pitching.innings;
        visitor_season.pitchers[x].pitching.thirds += visitor_cur.pitchers[x].pitching.thirds;

        if (visitor_season.pitchers[x].pitching.thirds > 2) {
            visitor_season.pitchers[x].pitching.thirds -= 3;
            visitor_season.pitchers[x].pitching.innings++;
        }

        visitor_season.pitchers[x].pitching.wins += visitor_cur.pitchers[x].pitching.wins;
        visitor_season.pitchers[x].pitching.losses += visitor_cur.pitchers[x].pitching.losses;
        visitor_season.pitchers[x].pitching.saves += visitor_cur.pitchers[x].pitching.saves;
        visitor_season.pitchers[x].pitching.svopp += visitor_cur.pitchers[x].pitching.svopp;
        visitor_season.pitchers[x].pitching.bfp += visitor_cur.pitchers[x].pitching.bfp;
        visitor_season.pitchers[x].pitching.hits += visitor_cur.pitchers[x].pitching.hits;
        visitor_season.pitchers[x].pitching.doubles += visitor_cur.pitchers[x].pitching.doubles;
        visitor_season.pitchers[x].pitching.triples += visitor_cur.pitchers[x].pitching.triples;
        visitor_season.pitchers[x].pitching.homers += visitor_cur.pitchers[x].pitching.homers;
        visitor_season.pitchers[x].pitching.runs += visitor_cur.pitchers[x].pitching.runs;
        visitor_season.pitchers[x].pitching.er += visitor_cur.pitchers[x].pitching.er;
        visitor_season.pitchers[x].pitching.rbi += visitor_cur.pitchers[x].pitching.rbi;
        visitor_season.pitchers[x].pitching.cg += visitor_cur.pitchers[x].pitching.cg;
        visitor_season.pitchers[x].pitching.gf += visitor_cur.pitchers[x].pitching.gf;
        visitor_season.pitchers[x].pitching.sho += visitor_cur.pitchers[x].pitching.sho;
        visitor_season.pitchers[x].pitching.sb += visitor_cur.pitchers[x].pitching.sb;
        visitor_season.pitchers[x].pitching.cs += visitor_cur.pitchers[x].pitching.cs;
        visitor_season.pitchers[x].pitching.walks += visitor_cur.pitchers[x].pitching.walks;
        visitor_season.pitchers[x].pitching.so += visitor_cur.pitchers[x].pitching.so;
        visitor_season.pitchers[x].pitching.ibb += visitor_cur.pitchers[x].pitching.ibb;
        visitor_season.pitchers[x].pitching.sh += visitor_cur.pitchers[x].pitching.sh;
        visitor_season.pitchers[x].pitching.sf += visitor_cur.pitchers[x].pitching.sf;
        visitor_season.pitchers[x].pitching.wp += visitor_cur.pitchers[x].pitching.wp;
        visitor_season.pitchers[x].pitching.balks += visitor_cur.pitchers[x].pitching.balks;
        visitor_season.pitchers[x].pitching.hb += visitor_cur.pitchers[x].pitching.hb;
        visitor_season.pitchers[x].pitching.opp_ab += visitor_cur.pitchers[x].pitching.opp_ab;
    }

    for (x = 0; x < maxpitchers[1]; x++) {
        if (home_cur.pitchers[x].pitching.games_started)
            home_season.pitchers[x].id.starts_rest = 0;
        else
            home_season.pitchers[x].id.starts_rest++;

        home_season.pitchers[x].id.ip_last4g[3] = home_season.pitchers[x].id.ip_last4g[2];
        home_season.pitchers[x].id.ip_last4g[2] = home_season.pitchers[x].id.ip_last4g[1];
        home_season.pitchers[x].id.ip_last4g[1] = home_season.pitchers[x].id.ip_last4g[0];
        if (home_cur.pitchers[x].pitching.games)
            home_season.pitchers[x].id.ip_last4g[0] = home_cur.pitchers[x].pitching.innings;
        else
            home_season.pitchers[x].id.ip_last4g[0] = 0;

        home_season.pitchers[x].pitching.games += home_cur.pitchers[x].pitching.games;
        home_season.pitchers[x].pitching.games_started += home_cur.pitchers[x].pitching.games_started;
        home_season.pitchers[x].pitching.innings += home_cur.pitchers[x].pitching.innings;
        home_season.pitchers[x].pitching.thirds += home_cur.pitchers[x].pitching.thirds;

        if (home_season.pitchers[x].pitching.thirds > 2) {
            home_season.pitchers[x].pitching.thirds -= 3;
            home_season.pitchers[x].pitching.innings++;
        }

        home_season.pitchers[x].pitching.wins += home_cur.pitchers[x].pitching.wins;
        home_season.pitchers[x].pitching.losses += home_cur.pitchers[x].pitching.losses;
        home_season.pitchers[x].pitching.saves += home_cur.pitchers[x].pitching.saves;
        home_season.pitchers[x].pitching.svopp += home_cur.pitchers[x].pitching.svopp;
        home_season.pitchers[x].pitching.bfp += home_cur.pitchers[x].pitching.bfp;
        home_season.pitchers[x].pitching.hits += home_cur.pitchers[x].pitching.hits;
        home_season.pitchers[x].pitching.doubles += home_cur.pitchers[x].pitching.doubles;
        home_season.pitchers[x].pitching.triples += home_cur.pitchers[x].pitching.triples;
        home_season.pitchers[x].pitching.homers += home_cur.pitchers[x].pitching.homers;
        home_season.pitchers[x].pitching.runs += home_cur.pitchers[x].pitching.runs;
        home_season.pitchers[x].pitching.er += home_cur.pitchers[x].pitching.er;
        home_season.pitchers[x].pitching.rbi += home_cur.pitchers[x].pitching.rbi;
        home_season.pitchers[x].pitching.cg += home_cur.pitchers[x].pitching.cg;
        home_season.pitchers[x].pitching.gf += home_cur.pitchers[x].pitching.gf;
        home_season.pitchers[x].pitching.sho += home_cur.pitchers[x].pitching.sho;
        home_season.pitchers[x].pitching.sb += home_cur.pitchers[x].pitching.sb;
        home_season.pitchers[x].pitching.cs += home_cur.pitchers[x].pitching.cs;
        home_season.pitchers[x].pitching.walks += home_cur.pitchers[x].pitching.walks;
        home_season.pitchers[x].pitching.so += home_cur.pitchers[x].pitching.so;
        home_season.pitchers[x].pitching.ibb += home_cur.pitchers[x].pitching.ibb;
        home_season.pitchers[x].pitching.sh += home_cur.pitchers[x].pitching.sh;
        home_season.pitchers[x].pitching.sf += home_cur.pitchers[x].pitching.sf;
        home_season.pitchers[x].pitching.wp += home_cur.pitchers[x].pitching.wp;
        home_season.pitchers[x].pitching.balks += home_cur.pitchers[x].pitching.balks;
        home_season.pitchers[x].pitching.hb += home_cur.pitchers[x].pitching.hb;
        home_season.pitchers[x].pitching.opp_ab += home_cur.pitchers[x].pitching.opp_ab;
    }
}

void
clr_vars () {
    int x, y;

    for (x = 0; x < 13; game_status.status[x++] = 0);
    for (x = 0; x < 9; x++)
        lineupchg[0][x] = lineupchg[1][x] = 0;
    rcd = altpo = dhpchg[0] = dhpchg[1] = 0;

    /* check for forfeit */
    for (y = 0; y < 2; y++)
        for (x = 0; x < 9; x++)
            if (border[y][x].player[0] > maxplayers[y]) {
                forfeit = -1;
                tforfeit = y;
            }
}

void
check_for_injury () {
    int x, y, pos[10], spos, grel, gstat, ha;
    float tiredness;

    if (game_status.half_inning % 2)
        ha = 0;
    else
        ha = 1;

    /* we want to check the pitcher, the hitter, and the fielder */
    if (whereto == 99)
        return;

    /* calculate tiredness of pitcher */
    if ((game_status.half_inning % 2) == 0) {
        x = home.pitchers[game_status.pitcher[1]].id.inn_target;
        y = home_cur.pitchers[game_status.pitcher[1]].pitching.innings +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[0] / 2) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[1] / 3) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[2] / 4) +
            (home.pitchers[game_status.pitcher[1]].id.ip_last4g[3] / 5);
    }
    else {
        x = visitor.pitchers[game_status.pitcher[0]].id.inn_target;
        y = visitor_cur.pitchers[game_status.pitcher[0]].pitching.innings +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[0] / 2) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[1] / 3) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[2] / 4) +
            (visitor.pitchers[game_status.pitcher[0]].id.ip_last4g[3] / 5);
    }
    if (y > (x + 5))
        tiredness = 0.2;
    else
        if (y > (x + 3))
            tiredness = 0.15;
        else
            if (y > (x + 2))
                tiredness = 0.1;
            else
                if (y > x)
                    tiredness = 0.05;
                else
                    tiredness = 0.0;   /* not tired */

    /* check for injury to pitcher */
    if ((int) ((float) 6000 * rand () / (RAND_MAX + 1.0)) < (15 + (15 * tiredness))) {
        if (game_status.half_inning % 2) {
            if ((visitor.pitchers[pitching[0].pitcher[0]].pitching.innings / visitor.pitchers[pitching[0].pitcher[0]].pitching.games) > 3)
                gstat = 35;
            else
                gstat = 85;
            grel = gstat - visitor.pitchers[pitching[0].pitcher[0]].pitching.games;
            if (grel < 0)
                grel = 0;
            /* find batter iteration number for pitcher */
            for (x = 0; x < 25; x++)
                if (!strcmp (&visitor_cur.pitchers[pitching[0].pitcher[0]].id.name[0], &visitor_cur.batters[x].id.name[0]))
                    break;

            if (grel >= (int) ((float) gstat * rand () / (RAND_MAX + 1.0))) {
                visitor_cur.batters[x].id.injury = (int) ((float) gstat * rand () / (RAND_MAX + 1.0));
                game_status.status[15]++;
            }
        }
        else {
            if ((home.pitchers[pitching[1].pitcher[0]].pitching.innings / home.pitchers[pitching[1].pitcher[0]].pitching.games) > 3)
                gstat = 35;
            else
                gstat = 85;
            grel = gstat - home.pitchers[pitching[1].pitcher[0]].pitching.games;
            if (grel < 0)
                grel = 0;
            /* find batter iteration number for pitcher */
            for (x = 0; x < 25; x++)
                if (!strcmp (&home_cur.pitchers[pitching[1].pitcher[0]].id.name[0], &home_cur.batters[x].id.name[0]))
                    break;

            if (grel >= (int) ((float) gstat * rand () / (RAND_MAX + 1.0))) {
                home_cur.batters[x].id.injury = (int) ((float) gstat * rand () / (RAND_MAX + 1.0));
                game_status.status[15]++;
            }
        }
    }

    /* check for injury to batter */
    if ((int) ((float) 6500 * rand () / (RAND_MAX + 1.0)) < 15) {
        if (game_status.half_inning % 2)
            x = 162 - home.batters[border[1][game_status.batter[1]].player[0]].hitting.games;
        else
            x = 162 - visitor.batters[border[0][game_status.batter[0]].player[0]].hitting.games;
        if (x < 0)
            x = 0;
        if (x >= (int) ((float) 162 * rand () / (RAND_MAX + 1.0))) {
            if (game_status.half_inning % 2)
                home_cur.batters[border[1][game_status.batter[1]].player[0]].id.injury = (int) ((float) (x + 5) * rand () / (RAND_MAX + 1.0));
            else
                visitor_cur.batters[border[0][game_status.batter[0]].player[0]].id.injury = (int) ((float) (x + 5) * rand () / (RAND_MAX + 1.0));
            game_status.status[15]++;
        }
    }

    /* check for injury to the fielder unless he's the pitcher */
    if (whereto != 1)
        if ((int) ((float) 6500 * rand () / (RAND_MAX + 1.0)) < 15) {
            for (spos = 1 + dhind; spos < 10; spos++)
                for (y = 0; y < 9; y++)
                    if (border[ha][y].pos[0] == spos) {
                        pos[spos] = border[ha][y].player[0];
                        break;
                    }
            if (dhind)
                for (y = 0; y < 25; y++) {
                    if (ha) {
                        if (!strcmp (&home.pitchers[game_status.pitcher[ha]].id.name[0], &home.batters[y].id.name[0])) {
                            pos[1] = y;
                            break;
                        }
                    }
                    else
                        if (!strcmp (&visitor.pitchers[game_status.pitcher[ha]].id.name[0], &visitor.batters[y].id.name[0])) {
                            pos[1] = y;
                            break;
                        }
                }
            if (!ha)
                x = 162 - visitor.batters[pos[whereto]].hitting.games;
            else
                x = 162 - home.batters[pos[whereto]].hitting.games;
            if (x < 0)
                x = 0;
            if (x >= (int) ((float) 162 * rand () / (RAND_MAX + 1.0))) {
                if (!ha)
                    visitor_cur.batters[pos[whereto]].id.injury = (int) ((float) (x + 5) * rand () / (RAND_MAX + 1.0));
                else
                    home_cur.batters[pos[whereto]].id.injury = (int) ((float) (x + 5) * rand () / (RAND_MAX + 1.0));
                game_status.status[15]++;
            }
        }
}

/* reverse player first and last name */
void
switch_name (char *str1, char *str2) {
    char *comma;

    comma = (char *) index (str2, ',');
    strcat (str1, comma + 2);
    strcat (str1, " ");
    *comma = '\0';
    strcat (str1, str2);
    *comma = ',';
}

void
runners_adv (char *str) {
    if (pre_act.outs && !post_act.outs)
        return;
    if (pre_act.baserunners[2] != 99) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
        strcat (str, " scores");
        if (pre_act.baserunners[1] != 99 || pre_act.baserunners[0] != 99 || action_ind == 19)
            strcat (str, ".  ");
    }
    if (pre_act.baserunners[1] != 99) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
        strcat (str, " moves to third base");
        if (pre_act.baserunners[0] != 99 || action_ind == 19)
            strcat (str, ".  ");
    }
    if (pre_act.baserunners[0] != 99) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
        strcat (str, " moves to second base");
        if (action_ind == 19)
            strcat (str, ".  ");
    }
}

void
runner_cutdown (char *str) {
    int bsw = 0;

    if (pre_act.baserunners[2] != 99) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
        strcat (str, " is cut down at the plate");
        bsw = 1;
        if ((pre_act.baserunners[1] != 99 || pre_act.baserunners[0] != 99) || (pre_act.baserunners[1] == 99 && pre_act.baserunners[0] == 99))
            strcat (str, ".  ");
    }
    if (pre_act.baserunners[1] != 99 && pre_act.outs < 2) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
        if (!bsw) {
            strcat (str, " is thrown out at third base.  ");
            bsw = 1;
        }
        else
            if (post_act.outs || (!pre_act.outs && !post_act.outs)) {
                strcat (str, " moves to third base");
                if (pre_act.baserunners[0] != 99 || post_act.baserunners[0] != 99)
                    strcat (str, ".  ");
            }
    }
    if (pre_act.baserunners[0] != 99 && pre_act.outs < 2) {
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
        if (!bsw)
            strcat (str, " is thrown out at second base.  ");
        else
            if (post_act.outs || (!pre_act.outs && !post_act.outs))
                strcat (str, " moves to second base.  ");
    }
}

void
force_runners_ahead (char *str) {
    if (pre_act.baserunners[0] != 99) {
        strcat (str, ".  ");
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
        strcat (str, " moves to second");
    
        if (pre_act.baserunners[1] != 99) {
            strcat (str, ".  ");
            if (pre_act.half_inning % 2)
                switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
            else
                switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
            strcat (str, " goes to third");

            if (pre_act.baserunners[2] != 99) {
                strcat (str, ".  ");
                if (pre_act.half_inning % 2)
                    switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
                else
                    switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
                strcat (str, " scores");
            }
        }
    }
}

void
move_runners (int num, char *str) {
    if (pre_act.baserunners[2] != 99) {
        strcat (str, ".  ");
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[2]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[2]].player[0]].id.name[0]);
        strcat (str, " scores");
    }

    if (pre_act.baserunners[1] != 99) {
        strcat (str, ".  ");
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[1]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[1]].player[0]].id.name[0]);
        if (num > 1)
            strcat (str, " scores");
        else
            strcat (str, " moves to third");
    }

    if (pre_act.baserunners[0] != 99) {
        strcat (str, ".  ");
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.baserunners[0]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.baserunners[0]].player[0]].id.name[0]);
        if (num > 2)
            strcat (str, " scores");
        if (num == 2)
            strcat (str, " moves to third");
        if (num == 1) {
            if (!altpo)
                strcat (str, " moves to second");
            else {
                altpo = 0;
                strcat (str, " is out at second.  ");
                if (pre_act.half_inning % 2)
                    switch_name (str, &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
                else
                    switch_name (str, &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
                strcat (str, " is safe at first");
            }
        }
    }

    if (num == 4) {
        strcat (str, ".  ");
        if (pre_act.half_inning % 2)
            switch_name (str, &home_cur.batters[border[1][pre_act.batter[1]].player[0]].id.name[0]);
        else
            switch_name (str, &visitor_cur.batters[border[0][pre_act.batter[0]].player[0]].id.name[0]);
        strcat (str, " scores");
        
        if (pre_act.baserunners[0] != 99 && pre_act.baserunners[1] != 99 && pre_act.baserunners[2] != 99) {
            if ((int) ((float) 4 * rand () / (RAND_MAX + 1.0)))
                strcat (str, ".  GRAND SLAM Home Run");
            else
                strcat (str, ".  GRAND SALAMI");
        }
    }
}

