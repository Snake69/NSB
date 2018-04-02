#ifndef _TEAMS_H_
#define _TEAMS_H_

#define NUMBER_OF_TEAMS 67   /* number of possible team names */

struct {
    int  id, yrspan[4];
    char filename[50],
         teamname[50],
         teamabbrev[11];
} teaminfo[NUMBER_OF_TEAMS + 1];  /* 1 extra for user-created teams */

struct {
    int          id,
                 year;
    char         league,        /* A = American, N = National */
                 division;      /* E = Eastern, C = Central, W = Western, or space for none */
    struct bttr  batters[25];
    struct ptchr pitchers[11];
} team, team2, visitor, home, visitor_cur, home_cur, visitor_season, home_season, dteam;

#endif
