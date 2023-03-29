#ifndef _SGLOBAL_H_
#define _SGLOBAL_H_

#include <time.h>

#define NO 0
#define YES 1
#define MAX_YEAR 2022
#define YEAR_SPREAD (MAX_YEAR - 1901) + 1

int listensock;          /* so that we can close sockets on ctrl-c */
int connectsock;
char host[256], domain[256];               /* who we are */
char buffer[1000000], buffer1[1000000], numstr[7], defplyrsurname[50], winners;
int sock, sockwp, netgame, netsock, action_ind, hldbase, gotateam;
int vmanager, hmanager;  /* type of manager for visiting team and home team
                            0 = computer, 1 = human */
int syslog_ent;          /* set to YES if syslog entries desired */
int no_pool;             /* set to YES to disallow network play */
int starters[2][10];     /* starters for the visiting team and the home team */
int maxplayers[2], maxpitchers[2];     /* the number of players and pitchers on the visiting and home teams (no more
                                          than 28 players and 13 pitchers) */
int dhind;               /* YES if DH allowed, NO if not */
int dhcode;              /* 0 = no DH, 1 = DH in AL games only, 2 = DH in NL games only, 3 DH in all games */
int pwin, ploss,         /* used to hold the current pitcher ID so that if the game ended at any point then we would know who gets the win and the loss */
    saver, savei,        /* help for figuring save */
    nosend,              /* if 1 then certain info is NOT sent to client */
    forfeit, tforfeit,   /* used to indicate that the current game is a forfeit */
    abb;                 /* used to indicate that server is getting commands from League Autoplay */
struct timespec delay;

/*
   batting order for the visiting and the home team
   structure allows for 2 teams, 9 batting order positions, and 30 slots in each batting order position (the player currently in the game
     always occupies slot 0, the previous player in that batting order position occupies slot 1, the player previous to that occupies slot 2, etc.)
   the associated pos = 0 for DH, 1-9 for the fielding positions, 10 for ph, and 11 for pr
   the same player can appear more than one time in the same batting order position if he moves around on the field, but a player cannot move batting order positions
*/
struct {
    int player[30],
        pos[30];
} border[2][9];

/*
   pitchers that appear in the game for the two teams

   structure allows for 2 teams and 15 pitchers for each team (the pitcher currently in the game always occupies slot 0, the previous pitcher occupies slot 1, the pitcher previous to that occupies slot 2, etc.)
   the number of innings pitched and the number of thirds of an inning pitched is associated with each pitcher
*/
struct {
    int pitcher[15],
        innings[15],
        thirds[15];
} pitching[2];

/*
   structure to keep track of earned runs, number of outs, and baserunner status as if the errors were outs
*/
struct {
    int stop, /* if non-zero then earned runs stop incrementing for remainder of this half-inning */
        h[2],
        br[3],
        pbr[3],
        hpbr[2],
        outs;
} earned;

struct {
    int half_inning;
    int inning_score[100][2];
    int outs, pbr[3];
    int baserunners[3], warming_up[2], batter[2], pitcher[2], lob[2],
        status[16];      /* pos 0: 0 = normal, 1 = holding runner on 1B
                            pos 1: 0 = normal, 1,2,3 = pitcher to attempt a pick-off to 1B, 2B, or 3B
                            pos 2: 0 = normal, 1 = infield in at corners, 2 = entire infield in
                            pos 3: 0 = normal, 1 = outfield in
                            pos 4: 0 = normal, 1 = intentional walk
                            pos 5: 0 = normal, 1 = pitchout
                            pos 6: 0 = normal, 1 = play the line at 3B, 2 = play the line at 1B, 3 = play both lines
                            pos 7: 0 = normal, 2 = steal 2B, 3 = steal 3B, 4 = steal home, 5 = steal 2B & 3B, 6 = steal 2B & home, 7 = steal 3B & home, 9 = steal 2B, 3B, home
                            pos 8: 0 = normal, 1 = hit & run
                            pos 9: 0 = normal, 1 = sacrifice bunt
                            pos 10: 0 = normal, 1 = squeeze play
                            pos 11: 0 = normal, 1 = pinch hitter
                            pos 12: 0 = normal, 1 = pinch runner
                            pos 13: 0 = normal, x = count of swinging misses to this batter
                            pos 14: 0 = normal, x = count of pitchouts to this batter
                            pos 15: 0 = normal, x = number of injured players */
} game_status, pre_act, post_act;

/* to indicate changes made to the lineup */
int lineupchg[2][9], dhpchg[2], AlreadySentData;

struct data {
    char name[50];
    char id[20];        /* human userid ... this field is used only in top level Records file */
    char uctname[50];   /* user-created team name ... only used when this player is from a user-created team */
    int tyear, teamid, stat[7], day, month, year, dis;
    /* dis = day in schedule */
    /* stat[5] always holds the primary stat (that is, the stat the user requested), stat[0] - stat[4] hold stats associated with
       percentage primary stats and stat[6] holds minimum requirements associated with percentage primary stats as follows:
       when stat[5] is batting average stat[0] contains at-bats, stat[1] contains hits, & stat[6] contains minimum plate
       appearances (number of games team played X 3.1)
       when stat[5] is slugging average stat[0] contains at-bats, stat[1] contains hits, stat[2] contains doubles, stat[3] contains
       triples, stat[4] contains homers & stat[6] contains minimum plate appearances (number of games team played X 3.1)
       when stat[5] is on base average stat[0] contains plate appearances, stat[1] contains hits, stat[2] contains walks, stat[3]
       contains hit by pitch & stat[6] contains minimum plate appearances (number of games team played X 3.1)
       when stat[5] is earned run average stat[0] contains innings pitched, stat[1] contains third of an innings pitched, stat[2]
       contains earned runs & stat[6] contains minimum innings pitched (number of games team played X 1)
       when stat[5] is won/loss percentage stat[0] contains wins, stat[1] contains losses & stat[6] contains minimum number of
       decisions (number of games team played / 12)
       when stat[5] is batting average against stat[0] contains at-bats against, stat[1] contains hits allowed & stat[6] contains
       minimum innings pitched (number of games team played X 1)
       when stat[5] is fielding average stat[0] contains total chances, stat[1] contains errors & stat[6] contains minimum games
       or, in the case of the pitcher, minimum innings pitched (for catcher, number of games team played / 2; for pitcher,
       number of games team played X 1; all other position players, number of games team played / 3 X 2)
       when stat[5] is IP stat[0] is thirds of an inning */
};

struct {
    struct data hitting[20][50];
    struct data fielding[8][7][50];
    struct data pitching[32][50];
} records[2], lrecords[2], srecords[2];

#endif

