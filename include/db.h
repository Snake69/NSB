#ifndef _DB_H_
#define _DB_H_

char hname[256], name[256];   /* host name and userid */
int user;                     /* pointer to info for user in userinfo file */

/* date of birth info will appear only once in each batter record */
struct DOB {
    int month, day, year;
};

/* user id and status information */
struct {
    char id[20],          /* NetStats Baseball ID */
         user[50],        /* userid */
         site[50],        /* siteid */
         passwd[10];      /* NetStats Baseball password */
    int  login_ct,        /* number of times connecting */
         status;          /* activity indicator; 0 = inactive, 1 = active */
} nsbdb[100];

/* player id info (this info will appear for each iteration of batter and each iteration of pitcher) */
struct playerid {
    char name[50];        /* name */
    int  teamid,          /* id of team this player is a member of in real life */
         batslr,          /* 0 = bats right, 1 = bats left, 2 = switchhitter */
         throwslr,        /* 0 = throws right, 1 = throws left */
         year,            /* year player represents */
         injury,          /* games injured (this stat is correct only in the batters section) */
         starts_rest,     /* games since last start (meaningful pertaining to pitchers only) */
         ip_last4g[4],    /* innings pitched in previous 4 games (meaningful pertaining to pitchers only)
                             occurrence 0 is the immediately previous game,
                             occurrence 1 is the previous game to that, etc */
         inn_target;      /* the number of innings this pitcher can pitch before losing effectiveness (meaningful pertaining to pitchers only) */
};

/* hitting stats */
struct hit {
    int games,            /* games */
        atbats,           /* at bats */
        runs,             /* runs scored */
        hits,             /* hits */
        doubles,          /* doubles */
        triples,          /* triples */
        homers,           /* home runs */
        rbi,              /* runs batted in */
        bb,               /* walks */
        so,               /* strike outs */
        hbp,              /* hit by pitcher */
        gidp,             /* grounded into double plays */
        sb,               /* stolen bases */
        cs,               /* caught stealing */
        ibb,              /* intentional walks */
        sh,               /* sacrifice hits */
        sf,               /* sacrifice flies */
        statsind;         /* 0 = real stats, 1 = generated stats */
};

/* fielding stats */
struct field {
    int games,            /* games */
        po,               /* put outs */
        dp,               /* double plays */
        a,                /* assists */
        pb,               /* passed balls - catchers only */
        e;                /* errors */
};

/* pitching stats */
struct pitch {
    int games,            /* games */
        games_started,    /* games started */
        innings,          /* innings pitched */
        thirds,           /* thirds of an inning pitched */
        wins,             /* wins */
        losses,           /* losses */
        saves,            /* saves */
        bfp,              /* batters facing pitcher */
        hits,             /* hits allowed */
        doubles,          /* doubles allowed */
        triples,          /* triples allowed */
        homers,           /* homers allowed */
        runs,             /* runs allowed */
        er,               /* earned runs allowed */
        rbi,              /* runs batted in allowed */
        cg,               /* complete games */
        gf,               /* games finished */
        sho,              /* shut-outs */
        svopp,            /* save opportunities */
        sb,               /* stolen bases allowed */
        cs,               /* caught stealings allowed */
        walks,            /* walks allowed */
        so,               /* strike outs */
        ibb,              /* intentional walks allowed */
        sh,               /* sacrifice hits */
        sf,               /* sacrifice flies */
        wp,               /* wild pitches */
        balks,            /* balks */
        hb,               /* hit batters */
        opp_ab,           /* opponents' at bats */
        statsind;         /* see above for a description */
};

struct ptchr {
    struct playerid id;
    struct pitch pitching;
};

struct bttr {
    struct playerid id;
    struct DOB dob;
    struct hit hitting;
    struct field fielding[11];   /* 10 different positions on the field plus one for all of outfield
                                 occurrence 0 = designated hitter
                                 occurrence 1 = pitcher
                                 occurrence 2 = catcher
                                 occurrence 3 = first base
                                 occurrence 4 = second base
                                 occurrence 5 = third base
                                 occurrence 6 = shortstop
                                 occurrence 7 = left field
                                 occurrence 8 = center field
                                 occurrence 9 = right field
                                 occurrence 10 = outfield
                                   NOTE - occurrence 10 is used if occurrences 7, 8, and 9 are not available */
};

#include "teams.h"

#endif

