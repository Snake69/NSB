#ifndef _CGLOBAL_H_
#define _CGLOBAL_H_

#include <time.h>

#define NO 0
#define YES 1
#define MAX_YEAR 2022
#define YEAR_SPREAD (MAX_YEAR - 1901) + 1

int sock, wsock, sockhvh, poolmngr, ThreadRunning, netgame, challenger_ind, countsel, pbpcnt, dh, RLCatLeadersMY, curbeg, Speaking, fdlock;
char buffer[1000000], buffer1[1000000], *env_logname, *env_user, cname[100], sname[100], sid[256], nsbid[256], lct[10], resultstr[10], visiting_team[50], home_team[50], teamyr[5], vteamyr[5], hteamyr[5],
     game_time[6], playbyplay[100000], currentd, currenttype, RequesteeID[30], tfname[256], tfnewname[256], usercreatedtname[256], DefSetPath[1024];
int prtbutrlrespnt, prtbutcatpnt, prtbuttmpnt, prtbutttpnt, prtbutrlppnt, prtbuttdibpnt, prtbutBSpnt, prtbutBTpnt, whichcatl[4096], whichur[4096];
char prtbutrlrescmd[256][1024], prtbutcatcmd[4096][1024], prtbuttmcmd[4096][1024], prtbutttcmd[4096][1024], prtbuttdibcmd[4096][6], prtuctm[4096][50],
     prtbutrlpcmd[4096][1024], prtbutBScmd[4096][1024], prtbutBTcmd[4096][1024], whichpsorrs[4096];
time_t dt;
struct tm dc;
struct {
    int player, pos;
} lineup[11];

struct {
    int ShowStartingLineups,  /* 0 = do not show starting lineups before single games start,
                                 1 = show starting lineups before single games start */
        IncludeUCTeams,       /* 0 = do not include user-created teams when selecting teams at random,
                                 1 = include user-created teams when selecting teams at random */
        PlaySounds,           /* 0 = do not play sounds, 1 = play sounds */
        SpeakPBP,             /* 0 = do not speak play-by-play, 1 = speak play-by-play */
        ShowPlayerPics,       /* 0 = do not show player pics during gameplay, 1 = show player pics during gameplay */
        ShowTDIBAtBoot,       /* 0 = do not show This Day in Baseball at boot, 1 = show This Day in Baseball at boot */
        MovingPlayerPics,     /* 0 = do not make Player Pics moveable during gameplay, 1 = make Player Pics moveable during gameplay */
        AssumeAllYears,       /* 0 = ask to "assume all years" when "Include Years" field is empty, 1 = don't ask */
        Speed_sec,            /* gameplay speed, seconds */
        Speed_nsec;           /* gameplay speed, nanoseconds */
} preferences;

#endif

