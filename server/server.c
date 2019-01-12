/*
   NetStatsBaseball server

   top-level requests from client:
     A - awards
     B - find best teams
     c - cancel
     C - change NSBID
     D - disconnected
     e - auto-create a user-created team
     E - save user-created team
     f - check number of user-created teams
     F - send user-created team to client
     G - delete user-created team
     g - rename user-created team
     h - play a game (user will play a non-season game against another human on the same computer)
     H - play a game (user will play the next scheduled season game against another human on the same computer)
     I - season injury report
     k - play portion of series
     l - play portion of season
     L - establish season
     M - establish season based upon a specific real life year, a specific team name, or at random
     N - get info for the next scheduled season game
     O - prepare for network game
     p - play a game (user will play a non-season game against the computer)
     P - play a game (user will play the next scheduled season game against the computer)
     Q - establish series
     R - remove NSBID
     S - stats
     U - list of users
     v - find best player offensive seasons
     V - find best player pitching seasons
     w - play a game (user will watch a non-season game)
     W - play a game (user will watch the next scheduled season game)
     X - cancel
     Z - waiting pool requests
*/

#include "net.h"
#include "db.h"
#include "sglobal.h"
#include "sproto.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <syslog.h>

#define DIV      0
#define ODIV     1
#define LEAGUE   2
#define AM       0
#define NAT      1

char *start1, *w1, *p, dummy[256], parent[256], yearv[5], yearh[5], yearhold[5], visiting_team[100], home_team[100], netport[100],
     pol;
int eb1, eb, ebb, ebuc, wperr, grand, win_id, lose_id, win_year, lose_year, actcnt, connected, ALWCteamID, ALWCteamYR, NLWCteamID,
    NLWCteamYR, ALWCswitch, NLWCswitch, ALEDswitch, ALCDswitch, ALWDswitch, NLEDswitch, NLCDswitch, NLWDswitch, ALEDteamID,
    ALEDteamYR, ALCDteamID, ALCDteamYR, ALWDteamID, ALWDteamYR, NLEDteamID, NLEDteamYR, NLCDteamID, NLCDteamYR, NLWDteamID,
    NLWDteamYR, AL2teamID, AL2teamYR, NL2teamID, NL2teamYR, AL2switch, NL2switch, ALdone;

struct {
    int id, yr, wins, losses, windec;
} AnotherChanceTeam[100];

struct {
    char div[300];
    int ids[300],
        years[300],
        lcnt,
        ecnt, ccnt, wcnt,
        divegames, divcgames, divwgames;
} AL, NL;

struct workteams {
    int id, year, wins, losses;
    float pct;
} teams[6][300], hteams;

/* Standings file structure */
struct {
    int id,          /* id of this team */
        year;        /* year of this team */
    char league, div;
    struct {
        int id, year, wins;
    } opp[300];       /* one occurrence for each team in the season */
} teamwins[300];      /* for each team in the season */

/* league setup */
struct {
    int numleagues,  /* number of leagues in season - 1 or 2 */
        numdivs,     /* number of divisions in each league - 1, 2 or 3 ... (per league) each league will contain the same number of divisions */
        numwc,       /* number of wild card teams in each league - 0, 1 or 2 ... (per league) each league will have the same number of wild card teams */
        nummaxgames[4];   /* maximum number of games in each post-season round - 0, 1, 3, 5, 7 or 9 ... 0 will indicate no round
                             each league will have the same number of rounds ... 1 round represents the finals,
                             also a 1-game WC vs WC PS game will constitute a round
                             e.g., a replay of the 1958 season nummaxgames will be 7,0,0,0 signifying 1 round and it being the World Series; a replay of the
                             2012 season nummaxgames will be 1,5,7,7 (round 1 being the WC vs WC game) plus 1 round for the World Series */

        /* NOTE - there is always one winner in each division */
} league_setup;

struct data leaders[50];

char schedule[244][3000];     /* the last occurence holds the DH indicator only */
int series, leftover, totgames, champs, champsyr;
int xx, yy;
char gdsw, gdl[25], teamv[5], teamvyr[5], teamh[5], teamhyr[5];
int gdi, gdc, teamvi, teamviyr, teamhi, teamhiyr, eos_sw, pdh;

/*
    end things
*/
void
closeup () {
    close (sock);
    if (syslog_ent == YES) {
        if (user > 99)
            syslog (LOG_INFO, "disconnect from ?");
        else
            if (ebb)
                syslog (LOG_INFO, "disconnect from %s@%s", &name[0], &hname[0]);
            else
                syslog (LOG_INFO, "disconnect from %s@%s", &nsbdb[user].user[0], &nsbdb[user].site[0]);
        closelog ();
    }
}

/*
    This waits for all children, so that they don't become zombies
*/
void
sig_chld (int signal_type) {
    int pid;
    int status;

    while ((pid = wait3 (&status, WNOHANG, NULL)) > 0);
}

/*
    output a usage message
*/
void
usage () {
    fprintf (stderr, "\nUsage:  nsbserver [-hlv]\n");
    fprintf (stderr, "Use the -h option to get this message\n");
    fprintf (stderr, "Use the -l option if syslog entries are desired\n");
    fprintf (stderr, "Use the -v option to show version info and exit\n\n");
}

/*
    output a version message
*/
void
version () {
    fprintf (stderr, "\nVersion info:  nsbserver v0.9.9.7\n\n");
}

int
get_rl_vis () {
    int x, y;
    FILE *in;

    if (yearv[0] == '0') {
        strcpy (&dummy[0], "/var/NSB/");
        strcat (&dummy[0], &nsbdb[user].id[0]);
        strcat (&dummy[0], "/UserTeams/");
        strcat (&dummy[0], &visiting_team[13]);
    }
    else {
        strcpy (&dummy[0], &parent[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], &yearv[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], &visiting_team[0]);
    }

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&visitor.id, sizeof visitor.id, 1, in);
        fread (&visitor.year, sizeof visitor.year, 1, in);
        fread (&visitor.league, sizeof visitor.league, 1, in);
        fread (&visitor.division, sizeof visitor.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&visitor.batters[x].id, sizeof visitor.batters[x].id, 1, in);
            fread (&visitor.batters[x].dob, sizeof visitor.batters[x].dob, 1, in);
            fread (&visitor.batters[x].hitting, sizeof visitor.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&visitor.batters[x].fielding[y], sizeof visitor.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&visitor.pitchers[x].id, sizeof visitor.pitchers[x].id, 1, in);
            fread (&visitor.pitchers[x].pitching, sizeof visitor.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_ERR, "couldn't open %s: %s", dummy, strerror (errno));
        if (netgame) {
            if (sock_puts (netsock, "fuckup\n") < 0)
                return -1;
        }
        else
            if (sock_puts (sock, "fuckup\n") < 0)
                return -1;
    }
    /* determine the number of players and pitchers this team has */
    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
        if (visitor.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&visitor.batters[maxplayers[0]].id.name[0]))
            break;
    for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
        if (visitor.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&visitor.pitchers[maxpitchers[0]].id.name[0]))
            break;
    return 0;
}

int
get_rl_home () {
    int x, y;
    FILE *in;

    if (yearh[0] == '0') {
        strcpy (&dummy[0], "/var/NSB/");
        strcat (&dummy[0], &nsbdb[user].id[0]);
        strcat (&dummy[0], "/UserTeams/");
        strcat (&dummy[0], &home_team[13]);
    }
    else {
        strcpy (&dummy[0], &parent[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], &yearh[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], &home_team[0]);
    }

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&home.id, sizeof home.id, 1, in);
        fread (&home.year, sizeof home.year, 1, in);
        fread (&home.league, sizeof home.league, 1, in);
        fread (&home.division, sizeof home.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&home.batters[x].id, sizeof home.batters[x].id, 1, in);
            fread (&home.batters[x].dob, sizeof home.batters[x].dob, 1, in);
            fread (&home.batters[x].hitting, sizeof home.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&home.batters[x].fielding[y], sizeof home.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&home.pitchers[x].id, sizeof home.pitchers[x].id, 1, in);
            fread (&home.pitchers[x].pitching, sizeof home.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_ERR, "couldn't open %s: %s", dummy, strerror (errno));
        if (sock_puts (sock, "fuckup\n") < 0)
            return -1;
    }
    /* determine the number of players and pitchers this team has */
    for (maxplayers[1] = 0; maxplayers[1] < 25; maxplayers[1]++)
        if (home.batters[maxplayers[1]].id.name[0] == ' ' || !strlen (&home.batters[maxplayers[1]].id.name[0]))
            break;
    for (maxpitchers[1] = 0; maxpitchers[1] < 11; maxpitchers[1]++)
        if (home.pitchers[maxpitchers[1]].id.name[0] == ' ' || !strlen (&home.pitchers[maxpitchers[1]].id.name[0]))
            break;
    return 0;
}

void
get_teams () {
    if (!eb1)
        dhind = buffer[0] - '0';
    /* get the two teams to play */
    start1 = index (&buffer[1], ' ');
    *start1 = '\0';
    strncpy (&yearv[0], &buffer[1], 4);
    yearv[4] = '\0';
    if (yearv[0] == '0') {
        strcpy (&visiting_team[0], "User-Created:");
        strcat (&visiting_team[0], &buffer[5]);
    }
    else
        strcpy (&visiting_team[0], &buffer[5]);
    *start1 = ' ';
    p = start1 + 1;
    strncpy (&yearh[0], p, 4);
    yearh[4] = '\0';
    p += 4;
    if (yearh[0] == '0') {
        strcpy (&home_team[0], "User-Created:");
        strcat (&home_team[0], p);
    }
    else
        strcpy (&home_team[0], p);
}

/* get 2 random teams in order to play a game */
void
get2random_teams () {
    int x, y, team, year, numteams, key, ucteams;
    char yearc[5];
    DIR *fnames;
    struct dirent *dir;

    for (y = 0; y < 2; y++) {
        /* check if UserTeams directory exists */
        ucteams = key = 0;
        strcpy (&dummy[0], "/var/NSB/");
        strcat (&dummy[0], &nsbdb[user].id[0]);
        strcat (&dummy[0], "/UserTeams");
        if ((fnames = opendir (&dummy[0])) == NULL)
            key = YEAR_SPREAD;
        else {
            /* the directory is present but make sure there is at least 1 user-created team */
            while ((dir = readdir (fnames))) {
                if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                    continue;

                ucteams++;       /* count available teams */
                key = YEAR_SPREAD;
            }
            if (key == 0)
                key = YEAR_SPREAD;
            else
                key = YEAR_SPREAD + 1;
            closedir (fnames);
        }
        if ((eb && !ebuc) || grand == 2)
            /* if eBaseball is doing the request and a valid ID command was not input then don't include any user-created teams in the randomness
               OR
               if user explicity does not want to include user-created teams in randomness */
            key = YEAR_SPREAD;
        /* get a random number between 1901 and MAX_YEAR (or MAX_YEAR + 1) inclusive (year ... MAX_YEAR + 1 indicates the UserTeams directory if present) */
        year = (int) ((float) key * rand () / (RAND_MAX + 1.0)) + 1901;
        numteams = 0;
        if (year != (MAX_YEAR + 1)) {
            strcpy (&yearc[0], (char *) cnvt_int2str (4, year));

            strcpy (&dummy[0], "/var/NSB/RealLifeStats/");
            strcat (&dummy[0], &yearc[0]);

            /* get the number of teams in that year */
            fnames = opendir (&dummy[0]);
            while ((dir = readdir (fnames)) != NULL)
                /* don't count the . and the .. and the Results files */
                if (strcmp (dir->d_name, ".") && strcmp (dir->d_name, "..") && strcmp (dir->d_name, "Results"))
                    numteams++;
            closedir (fnames);
        }
        else
            strcpy (&yearc[0], "0000");

        /* get a random number in order to select the team */
        if (year == (MAX_YEAR + 1))
            team = (int) ((float) ucteams * rand () / (RAND_MAX + 1.0));
        else
            team = (int) ((float) numteams * rand () / (RAND_MAX + 1.0));

        /* get the team */
        fnames = opendir (&dummy[0]);
        for (x = 0; x < (team + 1); ) {
            dir = readdir (fnames);
            /* skip the . and .. files */
            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                continue;
            /* skip the Results file */
            if (!strcmp (dir->d_name, "Results"))
                continue;
            x++;
        }

        if (!y) {
            strcpy (&yearv[0], &yearc[0]);
            if (year == (MAX_YEAR + 1)) {
                strcpy (&visiting_team[0], "User-Created:");
                strcat (&visiting_team[0], dir->d_name);
            }
            else
                strcpy (&visiting_team[0], dir->d_name);
        }
        else {
            strcpy (&yearh[0], &yearc[0]);
            if (year == (MAX_YEAR + 1)) {
                strcpy (&home_team[0], "User-Created:");
                strcat (&home_team[0], dir->d_name);
            }
            else
                strcpy (&home_team[0], dir->d_name);
        }
        closedir (fnames);
    }
}

/*
    Get user information
*/
int
get_userinfo () {
    FILE *f1;
    int index;

    /* open and read userinfo file */
    if ((f1 = fopen ("/var/NSB/userinfo", "r")) != NULL) {
        fread (&nsbdb, sizeof nsbdb, 1, f1);
        fclose (f1);
    }

    /* check if user already exists */
    for (index = 0, user = 100; index < 100; index++)
        if (!strcmp (&name[0], &nsbdb[index].user[0]) && !strcmp (&hname[0], &nsbdb[index].site[0]) && nsbdb[index].status == 1) {
            user = index;
            index = 100;
        }
    if (user > 99) {
        /* this is a first-time user */

        /* get NSB id */
        sock_puts (sock, "getid\n");
get_id:
        if (ebb)
            return 0;
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
            return -1;
        /* make sure id user chose is unique */
        for (index = 0, user = 100; index < 100; index++)
            if (nsbdb[index].status == 1)
                if (!strcmp (&nsbdb[index].id[0], &buffer[0])) {
                    sock_puts (sock, "dup\n");
                    goto get_id;
                }
        /* there's a couple of ids that we don't want the user to use */
        if (!strcmp (&buffer[0], "RealLifeStats") || !strcmp (&buffer[0], "XXeBaseballXX") || !strcmp (&buffer[0], "XXaBaseballXX")) {
            sock_puts (sock, "inv\n");
            goto get_id;
        }

        /* fill up slot in userinfo file */
        for (index = 0, user = 100; index < 100; index++)
            if (nsbdb[index].status != 1) {
                nsbdb[index].status = 1;
                strcpy (&nsbdb[index].id[0], &buffer[0]);
                strcpy (&nsbdb[index].user[0], &name[0]);
                strcpy (&nsbdb[index].site[0], &hname[0]);
                nsbdb[index].login_ct = 0;
                user = index;
                index = 100;
            }
        if (user > 99) {
            /* userinfo db full */
            if (syslog_ent == YES)
                syslog (LOG_INFO, "/var/NSB/userinfo full");
            sock_puts (sock, "err\n");
            return -1;
        }
        else
            sock_puts (sock, "ok\n");
    }
    else
        sock_puts (sock, "ok\n");
    /* increment user access count */
    nsbdb[user].login_ct++;

    /* write out userinfo file */
    if ((f1 = fopen ("/var/NSB/userinfo", "w")) != NULL) {
        fwrite (&nsbdb, sizeof nsbdb, 1, f1);
        fclose (f1);
    }
    return 0;
}

void
do_schedule (int league, char div) {
    int tdiv[300], tyear[300], e, c, w, x, y, z, zz, limit, dlimit, day;
    /* accomodate up to 100 teams */
    int matchups[1485][3], dailyg[50][2];
    int number_matchups, number_games_per_day, factor, cfactor, ats, muno;

    if (league == AM) {
        limit = AL.lcnt;
        for (x = e = c = w = 0; x < limit; x++) {
            if (AL.div[x] == 'E' && div == 'E') {
                tdiv[e] = AL.ids[x];
                tyear[e++] = AL.years[x];
            }
            if (AL.div[x] == 'C' && div == 'C') {
                tdiv[c] = AL.ids[x];
                tyear[c++] = AL.years[x];
            }
            if (AL.div[x] == 'W' && div == 'W') {
                tdiv[w] = AL.ids[x];
                tyear[w++] = AL.years[x];
            }
        }
        if (div == 'E') {
            series = AL.divegames / 4;
            leftover = AL.divegames % 4;
        }
        if (div == 'C') {
            series = AL.divcgames / 4;
            leftover = AL.divcgames % 4;
        }
        if (div == 'W') {
            series = AL.divwgames / 4;
            leftover = AL.divwgames % 4;
        }
    }
    else {
        limit = NL.lcnt;
        for (x = e = c = w = 0; x < limit; x++) {
            if (NL.div[x] == 'E' && div == 'E') {
                tdiv[e] = NL.ids[x];
                tyear[e++] = NL.years[x];
            }
            if (NL.div[x] == 'C' && div == 'C') {
                tdiv[c] = NL.ids[x];
                tyear[c++] = NL.years[x];
            }
            if (NL.div[x] == 'W' && div == 'W') {
                tdiv[w] = NL.ids[x];
                tyear[w++] = NL.years[x];
            }
        }
        if (div == 'E') {
            series = NL.divegames / 4;
            leftover = NL.divegames % 4;
        }
        if (div == 'C') {
            series = NL.divcgames / 4;
            leftover = NL.divcgames % 4;
        }
        if (div == 'W') {
            series = NL.divwgames / 4;
            leftover = NL.divwgames % 4;
        }
    }

    /* clear things */
    for (z = 0; z < 50; z++)
        dailyg[z][0] = dailyg[z][1] = -1;
    for (z = 0; z < 1485; z++)
        matchups[z][0] = matchups[z][1] = -1;

    /* determine all possible match-ups */
    dlimit = 0;
    if (div == 'E')
        dlimit = e;
    if (div == 'C')
        dlimit = c;
    if (div == 'W')
        dlimit = w;
    if (!dlimit)
        return;

    for (z = x = 0; x < (dlimit - 1); x++)
        for (y = x + 1; y < dlimit && z < 1485; y++, z++) {
            matchups[z][0] = x;
            matchups[z][1] = y;
            matchups[z][2] = 0;
        }
    number_matchups = z;

    cfactor = day = 0;
    number_games_per_day = dlimit / 2;
    factor = number_matchups / number_games_per_day;

    for (ats = z = 0; z < series; ) {    /* z gets incremented below */
        for (x = 0; x < number_games_per_day; x++)
            for (y = 0; ; y++) {
                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] <= y) {
                        for (zz = 0; zz < x; zz++)
                            if (dailyg[zz][0] == matchups[muno][0] || dailyg[zz][0] == matchups[muno][1] ||
                                           dailyg[zz][1] == matchups[muno][0] || dailyg[zz][1] == matchups[muno][1])
                                break;
                        if (zz == x) {
                            dailyg[x][0] = matchups[muno][0];
                            dailyg[x][1] = matchups[muno][1];
                            matchups[muno][2]++;
                            break;
                        }
                    }
                if (muno < number_matchups)
                    break;

                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] <= y) {
                        for (zz = 0; zz < x; zz++)
                            if ((dailyg[zz][0] == matchups[muno][0] && dailyg[zz][1] == matchups[muno][1]) ||
                                           (dailyg[zz][0] == matchups[muno][1] && dailyg[zz][1] == matchups[muno][0]))
                                break;
                        if (zz == x) {
                            dailyg[x][0] = matchups[muno][0];
                            dailyg[x][1] = matchups[muno][1];
                            matchups[muno][2]++;
                            break;
                        }
                    }
                if (muno < number_matchups)
                    break;

                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] <= y) {
                        dailyg[x][0] = matchups[muno][0];
                        dailyg[x][1] = matchups[muno][1];
                        matchups[muno][2]++;
                        break;
                    }
                if (muno < number_matchups)
                    break;
            }

        for (x = 0; x < 4 && day < 243; x++, day++)
            for (y = 0; y < 50; y++) {
                if (dailyg[y][0] == -1)
                    break;
                if (!(ats % 2)) {
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][1]]));
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][1]]));
                    strcat (&schedule[day][0], "-");
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][0]]));
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][0]]));
                }
                else {
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][0]]));
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][0]]));
                    strcat (&schedule[day][0], "-");
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][1]]));
                    strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][1]]));
                }
                strcat (&schedule[day][0], " ");
            }

        for (x = 0; x < 50; x++)
            dailyg[x][0] = dailyg[x][1] = -1;

        if (++cfactor >= factor) {
            cfactor = 0;
            z++;
        }
        if (!((day / 4) % (number_matchups / number_games_per_day)))
            ats++;
    }

    for (ats = z = 0; z < leftover; ) {      /* z gets incremented below */
        for (x = 0; x < number_games_per_day; x++)
            for (y = 0; ; y++) {
                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] == y) {
                        for (zz = 0; zz < x; zz++)
                            if (dailyg[zz][0] == matchups[muno][0] || dailyg[zz][0] == matchups[muno][1] ||
                                           dailyg[zz][1] == matchups[muno][0] || dailyg[zz][1] == matchups[muno][1])
                                break;
                        if (zz == x) {
                            dailyg[x][0] = matchups[muno][0];
                            dailyg[x][1] = matchups[muno][1];
                            matchups[muno][2]++;
                            break;
                        }
                    }
                if (muno < number_matchups)
                    break;

                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] <= y) {
                        for (zz = 0; zz < x; zz++)
                            if ((dailyg[zz][0] == matchups[muno][0] && dailyg[zz][1] == matchups[muno][1]) ||
                                           (dailyg[zz][0] == matchups[muno][1] && dailyg[zz][1] == matchups[muno][0]))
                                break;
                        if (zz == x) {
                            dailyg[x][0] = matchups[muno][0];
                            dailyg[x][1] = matchups[muno][1];
                            matchups[muno][2]++;
                            break;
                        }
                    }
                if (muno < number_matchups)
                    break;

                for (muno = 0; muno < number_matchups; muno++)
                    if (matchups[muno][2] <= y) {
                        dailyg[x][0] = matchups[muno][0];
                        dailyg[x][1] = matchups[muno][1];
                        matchups[muno][2]++;
                        break;
                    }
                if (muno < number_matchups)
                    break;
            }

        for (y = 0; y < 50; y++) {
            if (dailyg[y][0] == -1)
                break;
            if (!(ats % 2)) {
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][1]]));
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][1]]));
                strcat (&schedule[day][0], "-");
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][0]]));
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][0]]));
            }
            else {
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][0]]));
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][0]]));
                strcat (&schedule[day][0], "-");
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tyear[dailyg[y][1]]));
                strcat (&schedule[day][0], (char *) cnvt_int2str (4, tdiv[dailyg[y][1]]));
            }
            strcat (&schedule[day][0], " ");
        }

        if (++day == 243)
            break;

        for (x = 0; x < 50; x++)
            dailyg[x][0] = dailyg[x][1] = -1;

        if (++cfactor >= factor) {
            cfactor = 0;
            z++;
        }
        if (!((day / 4) % (number_matchups / number_games_per_day)))
            ats++;
    }
}

/*
    use correct stat for devising a category leaders table
*/
int
cmp_stat (int x, int y) {
    int pos, stat_acc, singles, chances, errors, games, ip, wtotg, sf;

    if (buffer[6] == '1') {
        if (buffer[7] == '1')
            if (team.batters[x].hitting.games >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '2')
            if (team.batters[x].hitting.atbats >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '3')
            if (team.batters[x].hitting.runs >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '4')
            if (team.batters[x].hitting.hits >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '5')
            if (team.batters[x].hitting.doubles >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '6')
            if (team.batters[x].hitting.triples >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '7')
            if (team.batters[x].hitting.homers >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '8')
            if (team.batters[x].hitting.rbi >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '9')
            if (team.batters[x].hitting.bb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'a')
            if (team.batters[x].hitting.so >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'b')
            if (team.batters[x].hitting.hbp >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'c')
            if (team.batters[x].hitting.gidp >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'd')
            if (team.batters[x].hitting.sb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'e')
            if (team.batters[x].hitting.cs >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'f')
            if (team.batters[x].hitting.ibb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'g')
            if (team.batters[x].hitting.sh >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'h')
            if (team.batters[x].hitting.sf >= leaders[y].stat[5])
                return 1;

        /* sacrifice flies wasn't always a recorded stat */
        if (team.batters[x].hitting.sf == -1)
            sf = 0;
        else
            sf = team.batters[x].hitting.sf;

        if (buffer[7] == 'i')
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                            sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames)
                if (((int) (((float) team.batters[x].hitting.hits / (float) team.batters[x].hitting.atbats) * 1000.0)) >= leaders[y].stat[5])
                    return 1;
        if (buffer[7] == 'j')
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                            sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames) {
                singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers + team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
                if (((int) ((((float) (team.batters[x].hitting.homers * 4) + (float) (team.batters[x].hitting.triples * 3) +
                                                                    (float) (team.batters[x].hitting.doubles * 2) +
                           (float) singles) / (float) team.batters[x].hitting.atbats) * 1000.0)) >= leaders[y].stat[5])
                    return 1;
            }
        if (buffer[7] == 'k')
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                            sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames)
                if (((int) (((float) team.batters[x].hitting.hits + (float) team.batters[x].hitting.bb +
                                    (float) team.batters[x].hitting.hbp) / ((float) team.batters[x].hitting.atbats +
                                    (float) team.batters[x].hitting.bb + (float) sf + (float) team.batters[x].hitting.hbp) * 1000.0)) >= leaders[y].stat[5])
                    return 1;
    }

    if (buffer[6] == '2') {
        if (buffer[7] == '1')
            if (team.pitchers[x].pitching.games >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '2')
            if (team.pitchers[x].pitching.games_started >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '3') {
            if (team.pitchers[x].pitching.innings >= leaders[y].stat[5])
                return 1;
            if (team.pitchers[x].pitching.innings == leaders[y].stat[5])
                if (team.pitchers[x].pitching.thirds >= leaders[y].stat[0])
                    return 1;
        }
        if (buffer[7] == '4')
            if (team.pitchers[x].pitching.wins >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '5')
            if (team.pitchers[x].pitching.losses >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '6')
            if (team.pitchers[x].pitching.saves >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '7')
            if (team.pitchers[x].pitching.bfp >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '8')
            if (team.pitchers[x].pitching.hits >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == '9')
            if (team.pitchers[x].pitching.doubles >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'a')
            if (team.pitchers[x].pitching.triples >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'b')
            if (team.pitchers[x].pitching.homers >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'c')
            if (team.pitchers[x].pitching.runs >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'd')
            if (team.pitchers[x].pitching.er >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'e')
            if (team.pitchers[x].pitching.rbi >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'f')
            if (team.pitchers[x].pitching.cg >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'g')
            if (team.pitchers[x].pitching.gf >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'h')
            if (team.pitchers[x].pitching.sho >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'i')
            if (team.pitchers[x].pitching.svopp >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'j')
            if (team.pitchers[x].pitching.sb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'k')
            if (team.pitchers[x].pitching.cs >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'l')
            if (team.pitchers[x].pitching.walks >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'm')
            if (team.pitchers[x].pitching.so >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'n')
            if (team.pitchers[x].pitching.ibb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'o')
            if (team.pitchers[x].pitching.sh >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'p')
            if (team.pitchers[x].pitching.sf >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'q')
            if (team.pitchers[x].pitching.wp >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'r')
            if (team.pitchers[x].pitching.balks >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 's')
            if (team.pitchers[x].pitching.hb >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 't')
            if (team.pitchers[x].pitching.opp_ab >= leaders[y].stat[5])
                return 1;
        if (buffer[7] == 'u')
            if (team.pitchers[x].pitching.innings >= totgames && totgames)
                if (((int) (((float) (team.pitchers[x].pitching.er * 9.0) / ((float) team.pitchers[x].pitching.innings +
                                 (float) team.pitchers[x].pitching.thirds / 3.0)) * 100.0)) <= leaders[y].stat[5])
                    return 1;
        if (buffer[7] == 'v')
            if ((team.pitchers[x].pitching.wins + team.pitchers[x].pitching.losses) >= (totgames / 12) && totgames)
                if (((int) (((float) team.pitchers[x].pitching.wins / ((float) team.pitchers[x].pitching.wins +
                            (float) team.pitchers[x].pitching.losses)) * 1000.0)) >= leaders[y].stat[5])
                    return 1;
        if (buffer[7] == 'w')
            if (team.pitchers[x].pitching.innings >= totgames && totgames)
                if (team.pitchers[x].pitching.opp_ab > 0)
                    /* opponents' ABs weren't always a recorded stat */
                    if (((int) (((float) team.pitchers[x].pitching.hits / (float) team.pitchers[x].pitching.opp_ab) * 1000.0)) <= leaders[y].stat[5])
                        return 1;
    }

    if (buffer[6] == '3') {
        if (buffer[8] == '1')
            pos = 10;
        if (buffer[8] == '2')
            pos = 3;
        if (buffer[8] == '3')
            pos = 4;
        if (buffer[8] == '4')
            pos = 5;
        if (buffer[8] == '5')
            pos = 6;
        if (buffer[8] == '6')
            pos = 1;
        if (buffer[8] == '7')
            pos = 2;
        if (buffer[8] == '8')
            pos = 0;

        stat_acc = chances = errors = games = 0;
        do {
            if (buffer[7] == '1')
                /* sometimes the stats for all 3 outfield positions are combined into one */
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].games;
            if (buffer[7] == '2')
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].po;
            if (buffer[7] == '3')
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].dp;
            if (buffer[7] == '4')
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].a;
            if (buffer[7] == '5')
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].pb;
            if (buffer[7] == '6')
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].e;
            if (buffer[7] == '7') {
                int y;
                if (pos == 1) {
                    for (y = 0; y < maxpitchers[0]; y++)
                        if (!strcmp (&team.pitchers[y].id.name[0], &team.batters[x].id.name[0])) {
                            ip = team.pitchers[y].pitching.innings;
                            break;
                        }
                }
                else
                    if (team.batters[x].fielding[pos].po != -1)
                        games += team.batters[x].fielding[pos].games;
                if (team.batters[x].fielding[pos].po != -1) {
                    errors += team.batters[x].fielding[pos].e;
                    chances += (team.batters[x].fielding[pos].po + team.batters[x].fielding[pos].a);
                }
            }
            pos--;
        } while (pos > 6);

        if (buffer[7] == '7') {
            switch (pos + 1) {  /* add 1 to pos since 1 was substracted in the do loop above */
                case 1:
                    wtotg = totgames;
                    break;
                case 2:
                    wtotg = totgames / 2;
                    break;
                default:
                    wtotg = totgames * 2 / 3;
            }
            if ((pos + 1) == 1)
                if (ip >= wtotg && (chances || errors))
                    stat_acc = (int) (((float) chances / ((float) chances + (float) errors)) * 1000.0);
                else
                    stat_acc = -1;
            else
                if ((games >= wtotg) && (chances || errors))
                    stat_acc = (int) (((float) chances / ((float) chances + (float) errors)) * 1000.0);
                else
                    stat_acc = -1;
        }
        if (stat_acc >= leaders[y].stat[5])
            return 1;
    }
    return 0;
}

void
cp_stat (int x, int y) {
    int pos, singles, chances, errors, sf;
    float pct;

    if (buffer[6] == '1') {
        if (buffer[7] == '1')
            leaders[y].stat[5] = team.batters[x].hitting.games;
        if (buffer[7] == '2')
            leaders[y].stat[5] = team.batters[x].hitting.atbats;
        if (buffer[7] == '3')
            leaders[y].stat[5] = team.batters[x].hitting.runs;
        if (buffer[7] == '4')
            leaders[y].stat[5] = team.batters[x].hitting.hits;
        if (buffer[7] == '5')
            leaders[y].stat[5] = team.batters[x].hitting.doubles;
        if (buffer[7] == '6')
            leaders[y].stat[5] = team.batters[x].hitting.triples;
        if (buffer[7] == '7')
            leaders[y].stat[5] = team.batters[x].hitting.homers;
        if (buffer[7] == '8')
            leaders[y].stat[5] = team.batters[x].hitting.rbi;
        if (buffer[7] == '9')
            leaders[y].stat[5] = team.batters[x].hitting.bb;
        if (buffer[7] == 'a')
            leaders[y].stat[5] = team.batters[x].hitting.so;
        if (buffer[7] == 'b')
            leaders[y].stat[5] = team.batters[x].hitting.hbp;
        if (buffer[7] == 'c')
            leaders[y].stat[5] = team.batters[x].hitting.gidp;
        if (buffer[7] == 'd')
            leaders[y].stat[5] = team.batters[x].hitting.sb;
        if (buffer[7] == 'e')
            leaders[y].stat[5] = team.batters[x].hitting.cs;
        if (buffer[7] == 'f')
            leaders[y].stat[5] = team.batters[x].hitting.ibb;
        if (buffer[7] == 'g')
            leaders[y].stat[5] = team.batters[x].hitting.sh;
        if (buffer[7] == 'h')
            leaders[y].stat[5] = team.batters[x].hitting.sf;
        if (buffer[7] == 'i') {
            pct = (float) team.batters[x].hitting.hits / (float) team.batters[x].hitting.atbats + 0.0005;              /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);

            leaders[y].stat[0] = team.batters[x].hitting.atbats;
            leaders[y].stat[1] = team.batters[x].hitting.hits;
            leaders[y].stat[6] = totgames * 3.1;
        }
        if (buffer[7] == 'j') {
            singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers + team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
            pct = ((float) (team.batters[x].hitting.homers * 4) + (float) (team.batters[x].hitting.triples * 3) +
                   (float) (team.batters[x].hitting.doubles * 2) + (float) singles) / (float) team.batters[x].hitting.atbats + 0.0005;  /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);
            leaders[y].stat[0] = team.batters[x].hitting.atbats;
            leaders[y].stat[1] = team.batters[x].hitting.hits;
            leaders[y].stat[2] = team.batters[x].hitting.doubles;
            leaders[y].stat[3] = team.batters[x].hitting.triples;
            leaders[y].stat[4] = team.batters[x].hitting.homers;
            leaders[y].stat[6] = totgames * 3.1;
        }
        if (buffer[7] == 'k') {
            /* sacrifice flies wasn't always a recorded stat */
            if (team.batters[x].hitting.sf == -1) 
                sf = 0;
            else
                sf = team.batters[x].hitting.sf;

            pct = ((float) team.batters[x].hitting.hits + (float) team.batters[x].hitting.bb + (float) team.batters[x].hitting.hbp) /
                                        ((float) team.batters[x].hitting.atbats + (float) team.batters[x].hitting.bb +
                                         (float) sf + (float) team.batters[x].hitting.sh + (float) team.batters[x].hitting.hbp) + 0.0005;    /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);
            leaders[y].stat[0] = team.batters[x].hitting.atbats + team.batters[x].hitting.bb +
                                 sf + team.batters[x].hitting.sh + team.batters[x].hitting.hbp;
            leaders[y].stat[1] = team.batters[x].hitting.hits;
            leaders[y].stat[2] = team.batters[x].hitting.bb;
            leaders[y].stat[3] = team.batters[x].hitting.hbp;
            leaders[y].stat[6] = totgames * 3.1;
        }
    }
    if (buffer[6] == '2') {
        if (buffer[7] == '1')
            leaders[y].stat[5] = team.pitchers[x].pitching.games;
        if (buffer[7] == '2')
            leaders[y].stat[5] = team.pitchers[x].pitching.games_started;
        if (buffer[7] == '3') {
            leaders[y].stat[5] = team.pitchers[x].pitching.innings;
            leaders[y].stat[0] = team.pitchers[x].pitching.thirds;
        }
        if (buffer[7] == '4')
            leaders[y].stat[5] = team.pitchers[x].pitching.wins;
        if (buffer[7] == '5')
            leaders[y].stat[5] = team.pitchers[x].pitching.losses;
        if (buffer[7] == '6')
            leaders[y].stat[5] = team.pitchers[x].pitching.saves;
        if (buffer[7] == '7')
            leaders[y].stat[5] = team.pitchers[x].pitching.bfp;
        if (buffer[7] == '8')
            leaders[y].stat[5] = team.pitchers[x].pitching.hits;
        if (buffer[7] == '9')
            leaders[y].stat[5] = team.pitchers[x].pitching.doubles;
        if (buffer[7] == 'a')
            leaders[y].stat[5] = team.pitchers[x].pitching.triples;
        if (buffer[7] == 'b')
            leaders[y].stat[5] = team.pitchers[x].pitching.homers;
        if (buffer[7] == 'c')
            leaders[y].stat[5] = team.pitchers[x].pitching.runs;
        if (buffer[7] == 'd')
            leaders[y].stat[5] = team.pitchers[x].pitching.er;
        if (buffer[7] == 'e')
            leaders[y].stat[5] = team.pitchers[x].pitching.rbi;
        if (buffer[7] == 'f')
            leaders[y].stat[5] = team.pitchers[x].pitching.cg;
        if (buffer[7] == 'g')
            leaders[y].stat[5] = team.pitchers[x].pitching.gf;
        if (buffer[7] == 'h')
            leaders[y].stat[5] = team.pitchers[x].pitching.sho;
        if (buffer[7] == 'i')
            leaders[y].stat[5] = team.pitchers[x].pitching.svopp;
        if (buffer[7] == 'j')
            leaders[y].stat[5] = team.pitchers[x].pitching.sb;
        if (buffer[7] == 'k')
            leaders[y].stat[5] = team.pitchers[x].pitching.cs;
        if (buffer[7] == 'l')
            leaders[y].stat[5] = team.pitchers[x].pitching.walks;
        if (buffer[7] == 'm')
            leaders[y].stat[5] = team.pitchers[x].pitching.so;
        if (buffer[7] == 'n')
            leaders[y].stat[5] = team.pitchers[x].pitching.ibb;
        if (buffer[7] == 'o')
            leaders[y].stat[5] = team.pitchers[x].pitching.sh;
        if (buffer[7] == 'p')
            leaders[y].stat[5] = team.pitchers[x].pitching.sf;
        if (buffer[7] == 'q')
            leaders[y].stat[5] = team.pitchers[x].pitching.wp;
        if (buffer[7] == 'r')
            leaders[y].stat[5] = team.pitchers[x].pitching.balks;
        if (buffer[7] == 's')
            leaders[y].stat[5] = team.pitchers[x].pitching.hb;
        if (buffer[7] == 't')
            leaders[y].stat[5] = team.pitchers[x].pitching.opp_ab;
        if (buffer[7] == 'u') {
            pct = ((float) (team.pitchers[x].pitching.er * 9.0) / ((float) team.pitchers[x].pitching.innings +
                                              (float) team.pitchers[x].pitching.thirds / 3.0)) + 0.005;   /* round up */
            leaders[y].stat[5] = (int) (pct * 100.0);
            leaders[y].stat[0] = team.pitchers[x].pitching.innings;
            leaders[y].stat[1] = team.pitchers[x].pitching.thirds;
            leaders[y].stat[2] = team.pitchers[x].pitching.er;
            leaders[y].stat[6] = totgames;
        }
        if (buffer[7] == 'v') {
            pct =  ((float) team.pitchers[x].pitching.wins /
                   ((float) team.pitchers[x].pitching.wins + (float) team.pitchers[x].pitching.losses)) + 0.0005;    /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);
            leaders[y].stat[0] = team.pitchers[x].pitching.wins;
            leaders[y].stat[1] = team.pitchers[x].pitching.losses;
            leaders[y].stat[6] = totgames / 12;
        }
        if (buffer[7] == 'w') {
            pct = ((float) team.pitchers[x].pitching.hits / (float) team.pitchers[x].pitching.opp_ab) + 0.0005;    /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);
            leaders[y].stat[0] = team.pitchers[x].pitching.opp_ab;
            leaders[y].stat[1] = team.pitchers[x].pitching.hits;
            leaders[y].stat[6] = totgames;
        }
    }
    if (buffer[6] == '3') {
        if (buffer[8] == '1')
            pos = 10;
        if (buffer[8] == '2')
            pos = 3;
        if (buffer[8] == '3')
            pos = 4;
        if (buffer[8] == '4')
            pos = 5;
        if (buffer[8] == '5')
            pos = 6;
        if (buffer[8] == '6')
            pos = 1;
        if (buffer[8] == '7')
            pos = 2;
        if (buffer[8] == '8')
            pos = 0;

        leaders[y].stat[5] = chances = errors = 0;
        /* this do routine is only to accumulate all outfield positions */
        do {
            if (buffer[7] == '1')
                /* sometimes the stats for all 3 outfield positions are combined into one */
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].games;
            if (buffer[7] == '2')
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].po;
            if (buffer[7] == '3')
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].dp;
            if (buffer[7] == '4')
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].a;
            if (buffer[7] == '5')
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].pb;
            if (buffer[7] == '6')
                if (team.batters[x].fielding[pos].po != -1)
                    leaders[y].stat[5] += team.batters[x].fielding[pos].e;
            if (buffer[7] == '7')
                if (team.batters[x].fielding[pos].po != -1) {
                    errors += team.batters[x].fielding[pos].e;
                    chances += (team.batters[x].fielding[pos].po + team.batters[x].fielding[pos].a);
                }
            pos--;
        } while (pos > 6);
        if (buffer[7] == '7') {
            pct = ((float) chances / ((float) chances + (float) errors)) + 0.0005;   /* round up */
            leaders[y].stat[5] = (int) (pct * 1000.0);
            leaders[y].stat[0] = chances;
            leaders[y].stat[1] = errors;

            if ((pos + 1) == 1)
                leaders[y].stat[6] = totgames;
            else
                if ((pos + 1) == 2)
                    leaders[y].stat[6] = totgames / 2;
                else
                    leaders[y].stat[6] = totgames * 2 / 3;
        }
    }
}

/*
    main processing
*/
int
main (int argc, char *argv[]) {
    int port = -1, x, y, z, zz, zzz, w, l, pos, sgame, sday, holdc, pid;
    char *nxtbl, dummy2[256], dummyi[256], dummyo[256], lornl, let, work[2], *argwp[5];
    struct sigaction act, oldact;
    struct dirent *dir, *dir2;
    DIR *fnames, *fnames2;
    FILE *in, *out;

    listensock = connectsock = -1;
    AlreadySentData = 0;
    syslog_ent = dhind = NO;

    if (argc > 2) {
        usage ();
        exit (-1);
    }
    if (argc == 2) {
        if (strlen (argv[1]) != 2) {
            usage ();
            exit (-1);
        }
        if (!strncmp (argv[1], "-v", 2)) {
            version ();
            exit (0);
        }
        if (!strncmp (argv[1], "-l", 2))
            syslog_ent = YES;
        else {
            usage ();
            exit (0);
        }
    }

    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sig_chld;
    sigaction (SIGCHLD, &act, &oldact);

    port = atoport ("nsbserver", "tcp", 0);
    if (port == -1) {
        fprintf (stderr, "Unable to find service: %s\n", "nsbserver");
        exit (-1);
    }

    if (gethostname (host, 255) < 0) {
        fprintf (stderr, "\nCan't get our own hostname\n");
        exit (-1);
    }
    if (getdomainname (domain, 255) < 0) {
        fprintf (stderr, "\nCan't get our own domainname\n");
        exit (-1);
    }

    if ((out = fopen ("/var/NSBtest", "w")) != NULL) {
        fclose (out);
        unlink ("/var/NSBtest");
    }
    else {
        fprintf (stderr, "\nnsbserver needs to have read and write permissions in /var\n");
        fprintf (stderr, "nsbserver exiting ...\n\n");
        exit (-1);
    }

    if (syslog_ent == YES) {
        openlog ("nsbserver", LOG_NDELAY, LOG_USER);
        syslog (LOG_INFO, "accepting connections");
    }

    /* start the server which controls the waiting pool for human versus human play over a network */
    wperr = 0;
    work[0] = 'X';
    work[1] = '\0';

    if ((out = fopen ("/tmp/nsbpoolmngr-ind", "w")) != NULL) {
        fwrite (&work, sizeof work, 1, out);
        fclose (out);
    }
    else {
        syslog (LOG_INFO, "could not invoke nsbpoolmngr - cannot create /tmp/nsbpoolmngr-ind");
        wperr = 1;
    }

    if (!wperr) {
        if ((pid = fork ()) < 0) {
            /* error */
            syslog (LOG_INFO, "could not invoke nsbpoolmngr - fork error");
            wperr = 1;
        }
        else
            if (!pid) {
                argwp[0] = "/usr/local/bin/nsbpoolmngr";
                argwp[1] = NULL;
                if (execv (argwp[0], argwp) < 0) {
                    /* error */
                    syslog (LOG_INFO, "could not invoke nsbpoolmngr - exec error");
                    wperr = 1;
                }
            }
    }

    populate ();

    /* wait for a client to connect */
    sock = get_connection (SOCK_STREAM, port, &listensock);

    /* we have a client */
    connectsock = sock;
    user = 100;
    abb = 0;

    /* connect to the nsbpoolmngr process if there was no error in starting it */
    if (!wperr)
        sockwp = make_connection ("49998", SOCK_STREAM, "localhost", 0);

    /* let the client know who it's talking to */
    strcpy (&buffer[0], &host[0]);
    if (strcmp (&domain[0], "(none)") && strncmp (&domain[0], "local", 5))
        strcat (&buffer[0], &domain[0]);
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    /* get user name from client
       in the case of the email client we'll pick up the user name and the host name */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
        close (sock);
    if (!strncmp (&buffer[0], "XXeBaseballXX", 13) || !strncmp (&buffer[0], "XXaBaseballXX", 13)) {
        ebb = 1;
        p = index (&buffer[0], '@');
        strncpy (&name[0], &buffer[13], (p - &buffer[13]));
        name[p - &buffer[13]] = '\0';
        if (!strncmp (&buffer[0], "XXeBaseballXX", 13)) {
            /* overwrite host name picked up in get_connection() */
            if (!strcmp (p + 1, "localhost.localdomain"))
                strcpy (&hname[0], "localhost");
            else
                strcpy (&hname[0], p + 1);
        }
        if (!strncmp (&buffer[0], "XXaBaseballXX", 13))
            abb = 1;
    }
    else {
        ebb = 0;
        strcpy (&name[0], &buffer[0]);
    }

    /* ensure /var/NSB exists */
    if ((fnames = opendir ("/var/NSB")) == NULL )
        mkdir ("/var/NSB", 0755);
    else
        closedir (fnames);

    /* find info about user if available */
    if (get_userinfo () == -1)
        close (sock);

    if (syslog_ent == YES) {
        if (user > 99)
            syslog (LOG_INFO, "connect from ?");
        else
            if (ebb)
                syslog (LOG_INFO, "connect from %s@%s", &name[0], &hname[0]);
            else
                syslog (LOG_INFO, "connect from %s@%s", &nsbdb[user].user[0], &nsbdb[user].site[0]);
    }

    /* create a directory for this user if it doesn't already exist */
    strcpy (&parent[0], "/var/NSB/");
    strcat (&parent[0], &nsbdb[user].id[0]);
    if ((fnames = opendir (&parent[0])) == NULL) {
        if (mkdir (&parent[0], 0700)) {
            if (syslog_ent == YES)
                syslog (LOG_INFO, "couldn't create %s: %s", parent, strerror (errno));
            return -1;
        }
    }
    else
        closedir (fnames);

    get_records ();

    /* send client remaining info about user */
    strcpy (&buffer[0], &nsbdb[user].site[0]);
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);
    strcpy (&buffer[0], &nsbdb[user].id[0]);
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);
    strcpy (&buffer[0], (char *) cnvt_int2str (6, nsbdb[user].login_ct));
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    netgame = 0;
    connected = 1;
    while (connected) {
        srand ((unsigned) time (NULL));
        /* get client request */
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            connected = 0;
            continue;
        }
        buffer1[0] = '\0';

        if (buffer[0] == 'D') {
            /* user disconnected */
            connected = 0;
            /* tell waiting pool thread to end */
            sprintf (&buffer1[0], "R%s\n", nsbdb[user].id);
            sock_puts (sockwp, &buffer1[0]);

            continue;
        }

        if (buffer[0] == 'f') {
            /* check for max number (500) of user-created teams */
            int ucteams;

            /* check if UserTeams directory exists */
            ucteams = 0;
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/UserTeams");
            if ((fnames = opendir (&dummy[0])) == NULL)
                sock_puts (sock, "OK\n");
            else {
                /* the directory is present, count the number of teams */
                while ((dir = readdir (fnames))) {
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;

                    ucteams++;       /* count available teams */
                }
                if (ucteams > 499)
                    sock_puts (sock, "MAX\n");
                else
                    sock_puts (sock, "OK\n");
                closedir (fnames);
            }
        }

        if (buffer[0] == 'F') {
            /* user wants to load a team s/he created */
            DIR *fnames;
            FILE *in;
            struct dirent *dir;
            char tname[256], dirt[256], dirtfn[256];
            int x, y;

            strcpy (&tname[0], &buffer[1]);

            strcpy (&dirt[0], "/var/NSB/");
            strcat (&dirt[0], &nsbdb[user].id[0]);
            strcat (&dirt[0], "/UserTeams");
            /* check if directory exists */
            if ((fnames = opendir (&dirt[0])) == NULL) {
                sock_puts (sock, "NONE\n");
                continue;
            }
            else
                closedir (fnames);

            strcpy (&dirtfn[0], &dirt[0]);
            strcat (&dirtfn[0], "/");
            strcat (&dirtfn[0], &tname[0]);
            /* read data if it exists */
            if ((in = fopen (dirtfn, "r")) != NULL) {
                fread (&team.id, sizeof team.id, 1, in);
                fread (&team.year, sizeof team.year, 1, in);
                fread (&team.league, sizeof team.league, 1, in);
                fread (&team.division, sizeof team.division, 1, in);
                for (x = 0; x < 25; x++) {
                    fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                    fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                    fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                    for (y = 0; y < 11; y++)
                        fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                }
                for (x = 0; x < 11; x++) {
                    fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                    fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                }
                fclose (in);

                strcpy (&buffer1[0], "OK");
                strcat (&buffer1[0], &tname[0]);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
                send_stats (sock, 't');
            }
            else {
                strcpy (&buffer1[0], "NOMATCH");

                fnames = opendir (&dirt[0]);
                while ((dir = readdir (fnames))) {
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;

                    strcat (&buffer1[0], dir->d_name);
                    strcat (&buffer1[0], " ");
                }
                closedir (fnames);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }
        }

        if (buffer[0] == 'G') {
            /* user wants to delete a team s/he created */
            DIR *fnames;
            FILE *in;
            struct dirent *dir;
            char tname[256], dirt[256], dirtfn[256];

            strcpy (&tname[0], &buffer[1]);

            strcpy (&dirt[0], "/var/NSB/");
            strcat (&dirt[0], &nsbdb[user].id[0]);
            strcat (&dirt[0], "/UserTeams");
            /* check if directory exists */
            if ((fnames = opendir (&dirt[0])) == NULL) {
                sock_puts (sock, "NONE\n");
                continue;
            }
            else
                closedir (fnames);

            strcpy (&dirtfn[0], &dirt[0]);
            strcat (&dirtfn[0], "/");
            strcat (&dirtfn[0], &tname[0]);
            /* check if team exists */
            if ((in = fopen (dirtfn, "r")) != NULL) {
                fclose (in);

                strcpy (&buffer1[0], "MATCH\n");
                sock_puts (sock, &buffer1[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                if (!strcmp (&buffer[0], "DOIT")) {
                    unlink (dirtfn);
                    sock_puts (sock, "OK\n");
                }

                continue;
            }
            else {
                strcpy (&buffer1[0], "NOMATCH");

                fnames = opendir (&dirt[0]);
                while ((dir = readdir (fnames))) {
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;

                    strcat (&buffer1[0], dir->d_name);
                    strcat (&buffer1[0], " ");
                }
                closedir (fnames);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }
        }

        if (buffer[0] == 'g') {
            /* user wants to rename a team s/he created */
            DIR *fnames;
            FILE *in;
            struct dirent *dir;
            char tname[256], tnewname[256], dirt[256], dirtfn[256], dirtnewfn[256], *blank;

            blank = index (&buffer[1], ' ');
            *blank = '\0';
            strcpy (&tname[0], &buffer[1]);     /* current name */
            blank++;
            strcpy (&tnewname[0], blank);       /* new name */

            strcpy (&dirt[0], "/var/NSB/");
            strcat (&dirt[0], &nsbdb[user].id[0]);
            strcat (&dirt[0], "/UserTeams");
            /* check if directory exists */
            if ((fnames = opendir (&dirt[0])) == NULL) {
                sock_puts (sock, "NONE\n");
                continue;
            }
            else
                closedir (fnames);

            strcpy (&dirtfn[0], &dirt[0]);
            strcat (&dirtfn[0], "/");
            strcat (&dirtfn[0], &tname[0]);
            strcpy (&dirtnewfn[0], &dirt[0]);
            strcat (&dirtnewfn[0], "/");
            strcat (&dirtnewfn[0], &tnewname[0]);
            /* check if team exists */
            if ((in = fopen (dirtfn, "r")) != NULL) {
                fclose (in);

                strcpy (&buffer1[0], "MATCH\n");
                sock_puts (sock, &buffer1[0]);

                rename (dirtfn, dirtnewfn);

                continue;
            }
            else {
                strcpy (&buffer1[0], "NOMATCH");

                fnames = opendir (&dirt[0]);
                while ((dir = readdir (fnames))) {
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;

                    strcat (&buffer1[0], dir->d_name);
                    strcat (&buffer1[0], " ");
                }
                closedir (fnames);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }
        }

        if (buffer[0] == 'E') {
            /* user wants to save the team s/he created by hand on the client */
            DIR *fnames;
            FILE *out;
            struct dirent *dir;
            char tname[256], dirt[256];
            int nope, x, y;

            strcpy (&tname[0], &buffer[1]);

            if (get_stats (sock) == -1) {
                sock_puts (sock, "ERR\n");
                continue;
            }
            /* create the directory to store user-created teams if it doesn't already exist */
            strcpy (&dirt[0], "/var/NSB/");
            strcat (&dirt[0], &nsbdb[user].id[0]);
            strcat (&dirt[0], "/UserTeams");
            if ((fnames = opendir (&dirt[0])) == NULL) {
                if (mkdir (&dirt[0], 0700)) {
                    if (syslog_ent == YES)
                        syslog (LOG_INFO, "couldn't create %s: %s", dirt, strerror (errno));
                    sock_puts (sock, "ERR\n");
                    continue;
                }
            }
            else
                closedir (fnames);

            /* check if team name is already being used */
            nope = 0;
            if ((fnames = opendir (&dirt[0])) != NULL) {
                while ((dir = readdir (fnames)))
                    if (!strcmp (dir->d_name, &tname[0])) {
                        sock_puts (sock, "DUP\n");
                        sock_gets (sock, &buffer[0], sizeof (buffer));
                        if (!strcmp (&buffer[0], "OK"))
                            break;
                        else {
                            nope = 1;
                            break;
                        }
                    }
                closedir (fnames);
                if (nope)
                    continue;
            }
            else {
                sock_puts (sock, "ERR\n");
                continue;
            }

            strcat (&dirt[0], "/");
            strcat (&dirt[0], &tname[0]);

            if ((out = fopen (dirt, "w")) != NULL) {
                fwrite (&team.id, sizeof team.id, 1, out);
                fwrite (&team.year, sizeof team.year, 1, out);
                fwrite (&team.league, sizeof team.league, 1, out);
                fwrite (&team.division, sizeof team.division, 1, out);
                for (x = 0; x < 25; x++) {
                    fwrite (&team.batters[x].id, sizeof team.batters[x].id, 1, out);
                    fwrite (&team.batters[x].dob, sizeof team.batters[x].dob, 1, out);
                    fwrite (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, out);
                    for (y = 0; y < 11; y++)
                        fwrite (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, out);
                }
                for (x = 0; x < 11; x++) {
                    fwrite (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, out);
                    fwrite (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, out);
                }
                fclose (out);
                sock_puts (sock, "OK\n");
            }
            else
                sock_puts (sock, "ERR\n");

            continue;
        }

        if (buffer[0] == 'e') {
            /* user wants to auto-create a new team */
            int res;

            res = CreateTeam ();
            /* to avoid a compile warning */
            if (res)
                continue;
            else
                continue;
        }

        if (buffer[0] == 'O') {
            /* a human versus human game over a network is about to start */
            int port = 0, listennetsock = -1;

            netgame = 1;

            /* open a port for listening for communications from the challenger's client (this child server process is
               that of the challengee's which will control the game) */
            netsock = get_connection_nofork (SOCK_STREAM, port, &listennetsock);

            continue;
        }

        if (buffer[0] == 'R') {
            /* user wants to remove his NSBID */
            int pid;
            char *argrm[5];
            FILE *f1;

            /* delete /var/NSB/<userid> */
            if ((pid = fork ()) < 0) {
                /* tell client there was an error */
                strcpy (&buffer1[0], "-1\n");
                sock_puts (sock, &buffer1[0]);
                continue;
            }
            else
                if (!pid) {
                    char nsbidp[100];

                    argrm[0] = "/bin/rm";
                    argrm[1] = "-rf";
                    strcpy (&nsbidp[0], "/var/NSB/");
                    strcat (&nsbidp[0], &nsbdb[user].id[0]);
                    argrm[2] = &nsbidp[0];
                    argrm[3] = NULL;
                    if (execv (argrm[0], argrm) < 0) {
                        /* tell client there was an error */
                        strcpy (&buffer1[0], "-1\n");
                        sock_puts (sock, &buffer1[0]);
                        continue;
                    }
                }
                else
                    wait (0);

            /* we'll get here only if there were no errors, any error will kick us out of this before here */

            /* open up slot in userinfo file and write it out */
            nsbdb[user].status = 0;
            if ((f1 = fopen ("/var/NSB/userinfo", "w")) != NULL) {
                fwrite (&nsbdb, sizeof nsbdb, 1, f1);
                fclose (f1);
            }

            /* tell client it worked */
            strcpy (&buffer1[0], "OK\n");
            sock_puts (sock, &buffer1[0]);

            if (syslog_ent == YES)
                syslog (LOG_INFO, "%s@%s removed from user file", &nsbdb[user].user[0], &nsbdb[user].site[0]);

            /* disconnect user */
            connected = 0;

            /* tell waiting pool thread to end */
            sprintf (&buffer1[0], "R%s\n", nsbdb[user].id);
            sock_puts (sockwp, &buffer1[0]);
        }

        if (buffer[0] == 'C') {
            /* user wants to change his NSBID */
            char newid[100], *argrm[5], nsbidp[300];
            int pid, index;
            FILE *f1;

            strcpy (&newid[0], &buffer[1]);

            /* make sure id user chose is unique */
            for (index = 0; index < 100; index++)
                if (nsbdb[index].status == 1)
                    if (!strcmp (&nsbdb[index].id[0], &newid[0])) {
                        sock_puts (sock, "dup\n");
                        continue;
                    }
            /* there's a couple of ids that we don't want the user to use */
            if (!strcmp (&buffer[0], "RealLifeStats") || !strcmp (&buffer[0], "XXeBaseballXX") || !strcmp (&buffer[0], "XXaBaseballXX")) {
                sock_puts (sock, "inv\n");
                continue;
            }

            /* delete /var/NSB/<userid>/Lifetime/Records */
            strcpy (&nsbidp[0], "/var/NSB/");
            strcat (&nsbidp[0], &nsbdb[user].id[0]);
            strcat (&nsbidp[0], "/Lifetime/Records");
            unlink (nsbidp);

            /* move user's directory in /var/NSB */
            if ((pid = fork ()) < 0) {
                /* tell client there was an error */
                strcpy (&buffer1[0], "-1\n");
                sock_puts (sock, &buffer1[0]);
                continue;
            }
            else
                if (!pid) {
                    char nsbidp1[300], nsbidp2[300];

                    argrm[0] = "/bin/mv";
                    argrm[1] = "-f";
                    strcpy (&nsbidp1[0], "/var/NSB/");
                    strcat (&nsbidp1[0], &nsbdb[user].id[0]);
                    argrm[2] = &nsbidp1[0];
                    strcpy (&nsbidp2[0], "/var/NSB/");
                    strcat (&nsbidp2[0], &newid[0]);
                    argrm[3] = &nsbidp2[0];
                    argrm[4] = NULL;
                    if (execv (argrm[0], argrm) < 0) {
                        /* tell client there was an error */
                        strcpy (&buffer1[0], "-1\n");
                        sock_puts (sock, &buffer1[0]);
                        continue;
                    }
                }
                else
                    wait (0);

            /* we'll get here only if there were no errors, any error will kick us out of this before here */

            if (syslog_ent == YES)
                syslog (LOG_INFO, "%s@%s changed NSBID from %s to %s", &nsbdb[user].user[0], &nsbdb[user].site[0],
                                                                       &nsbdb[user].id[0], &newid[0]);

            /* change ID in userinfo file */
            strcpy (&nsbdb[user].id[0], &newid[0]);

            /* write userinfo file */
            if ((f1 = fopen ("/var/NSB/userinfo", "w")) != NULL) {
                fwrite (&nsbdb, sizeof nsbdb, 1, f1);
                fclose (f1);
            }
            /* reset records */
            get_records ();

            sock_puts (sock, "ok\n");    /* tell client everything is cool */
        }

        if (buffer[0] == 'A') {
            /* user wants to see awards for current season (MVP, Cy Young, Gold Gloves or Silver Sluggers) */

            int x, y, z, tdividend, tdivisor, sf, psteamcnt;
            float tscore;
            char tbuf[20], psteams[8][100];

            struct {
                char name[50];
                char uctname[50];   /* user-created team name ... only used when this player is from a user-created team */
                int tyear, teamid;
                float score;
            } mvp[20];   /* MVP candidates */
            struct {
                char name[50];
                char uctname[50];   /* user-created team name ... only used when this pitcher is from a user-created team */
                int tyear, teamid;
                float score;
            } cyyoung[20];   /* Cy Young candidates */
            struct {
                char name[50];
                char uctname[50];   /* user-created team name ... only used when this pitcher is from a user-created team */
                int tyear, teamid, tc, err;
                float pct;
            } goldglove[10];   /* Gold Gloves */
            struct {
                char name[50];
                char uctname[50];   /* user-created team name ... only used when this pitcher is from a user-created team */
                int tyear, teamid;
                float score;
            } silverslugger[10];   /* Silver Sluggers */

            /* the regular season has to have ended */
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            psteamcnt = 0;
            if ((fnames = opendir (&dummy[0])) != NULL) {
                while ((dir = readdir (fnames)))
                    /* we're looking for the presence of a post-season team file */
                    if (strstr (dir->d_name, "-PS"))
                        /* save post-season teamnames for use later */
                        if (strcmp (dir->d_name, "Schedule-PS")) {
                            strcpy (&psteams[psteamcnt][0], dir->d_name);
                            psteamcnt++;
                        }
                closedir (fnames);
            }
            if (!psteamcnt) {
                /* load league set-up */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/LeagueSetup");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&league_setup, sizeof league_setup, 1, in);
                    fclose (in);
                }
                else {
                    sock_puts (sock, "-1\n");     /* tell client */
                    continue;                     /* look for next client request */
                }

                if (league_setup.nummaxgames[0]) {
                    /* the regular season is still on-going or there is no season */
                    sock_puts (sock, "-1\n");     /* tell client */
                    continue;                     /* look for next client request */
                }
            }
            /* remove the "-PS" from the post-season teamnames */
            for (x = 0; x < psteamcnt; x++)
                psteams[x][strlen (&psteams[x][0]) - 3] = '\0';

            for (x = 0; x < 20; x++) {
                mvp[x].name[0] = cyyoung[x].name[0] = '\0';
                mvp[x].score = cyyoung[x].score = 0.0;
            }
            for (x = 0; x < 10; x++) {
                goldglove[x].name[0] = silverslugger[x].name[0] = '\0';
                goldglove[x].tc = goldglove[x].err = 0;
                silverslugger[x].score = 0.0;
            }

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);

            if ((fnames = opendir (&dummy[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the schedule, standings or records */
                    if (!strcmp (dir->d_name, "Schedule") || !strcmp (dir->d_name, "Standings") ||
                                 !strcmp (dir->d_name, "PlayoffResultsAL") || !strcmp (dir->d_name, "PlayoffResultsNL") ||
                                                        !strcmp (dir->d_name, "Series") || !strcmp (dir->d_name, "Records"))
                        continue;
                    /* don't process the Results file */
                    if (strstr (dir->d_name, "Results"))
                        continue;
                    /* don't process post-season stats */
                    if (strstr (dir->d_name, "-PS"))
                        continue;
                    /* don't process the Lifetime directory */
                    if (!strcmp (dir->d_name, "Lifetime"))
                        continue;
                    /* don't process the UserTeams directory */
                    if (!strcmp (dir->d_name, "UserTeams"))
                        continue;
                    /* don't process the LeagueSetup file */
                    if (!strcmp (dir->d_name, "LeagueSetup"))
                        continue;

                    strcpy (&dummy2[0], &dummy[0]);
                    strcat (&dummy2[0], "/");
                    strcat (&dummy2[0], dir->d_name);

                    totgames = 0;
                    if ((in = fopen (dummy2, "r")) != NULL) {
                        fread (&team.id, sizeof team.id, 1, in);
                        fread (&team.year, sizeof team.year, 1, in);
                        fread (&team.league, sizeof team.league, 1, in);
                        fread (&team.division, sizeof team.division, 1, in);
                        for (x = 0; x < 25; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 11; x++) {
                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                            totgames += team.pitchers[x].pitching.games_started;
                        }
                        fclose (in);
                    }
                    else {
                        if (syslog_ent == YES)
                            syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                        return -1;
                    }
                    /* determine number of non-pitchers this team has */
                    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                            break;
                    /* determine number of pitchers this team has */
                    for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                        if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                            break;

                    if (buffer[1] == 'M')
                        /* MVP
                           calculation - (for hitters) (TB + SB + BB + HBP - CS) / (AB - H + CS + GIDP)
                           plus:
                             .050 points for playing on a playoff team
                             .025 points for up-the-middle position */

                        for (x = 0; x < maxplayers[0]; x++) {
                            /* to be considered player must have a minimum number of plate appearances */
                            /* sacrifice flies wasn't always a recorded stat */
                            if (team.batters[x].hitting.sf == -1)
                                sf = 0;
                            else
                                sf = team.batters[x].hitting.sf;
                            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                                                         sf + team.batters[x].hitting.sh) < (totgames * 3.1))
                                continue;

                            /* total bases */
                            tdividend = team.batters[x].hitting.hits - (team.batters[x].hitting.homers +
                                                               team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
                            tdividend += ((team.batters[x].hitting.homers * 4) + (team.batters[x].hitting.triples * 3) +
                                                                                 (team.batters[x].hitting.doubles * 2));
                            /* add stolen bases, walks & hit by pitches */
                            tdividend += (team.batters[x].hitting.sb + team.batters[x].hitting.bb + team.batters[x].hitting.hbp);
                            /* subtract caught stealing */
                            tdividend -= team.batters[x].hitting.cs;

                            /* at bats */
                            tdivisor = team.batters[x].hitting.atbats;
                            /* subtract hits */
                            tdivisor -= team.batters[x].hitting.hits;
                            /* add caught stealing & grounded into DP */
                            tdivisor += (team.batters[x].hitting.cs + team.batters[x].hitting.gidp);

                            /* figure score */
                            tscore = (float) tdividend / (float) tdivisor;

                            /* add to score for being on a playoff team */
                            for (y = 0; y < psteamcnt; y++)
                                if (!strcmp (&psteams[y][0], dir->d_name)) {
                                    tscore += .05;
                                    break;
                                }

                            /* add to score for playing an up-the-middle position (C, 2B, SS, CF) */
                            if (team.batters[x].fielding[2].games >= (totgames / 2) || team.batters[x].fielding[4].games >= (totgames / 2) ||
                                         team.batters[x].fielding[6].games >= (totgames / 2) || team.batters[x].fielding[8].games >= (totgames / 2))
                                tscore += .025;

                            for (y = 0; y < 20; y++)
                                if (tscore > mvp[y].score) {
                                    for (z = 18; z >= y; z--)
                                        mvp[z + 1] = mvp[z];
                                    mvp[y].score = tscore;
                                    mvp[y].tyear = team.year;
                                    mvp[y].teamid = team.id;
                                    strcpy (&mvp[y].uctname[0], dir->d_name);
                                    /* check for "-PS" */
                                    if (!strncmp (&mvp[y].uctname[strlen (&mvp[y].uctname[0]) - 3], "-PS", 3))
                                        mvp[y].uctname[strlen (&mvp[y].uctname[0]) - 3] = '\0';
                                    strcpy (&mvp[y].name[0], &team.batters[x].id.name[0]);
                                    break;
                                }
                        }

                    if (buffer[1] == 'C')
                        /* Cy Young
                           calculation - ((5 * IP / 9) - ER) + (SO / 12) + (SV * 2.5) + Shutouts + ((W * 6) - (L * 2)) + VB
                             VB: Victory Bonus - 12-point bonus awarded for leading your team to the post-season */

                        for (x = 0; x < maxpitchers[0]; x++) {
                            /* to be considered pitcher must have a minimum number of innings pitched */
                            if (team.pitchers[x].pitching.innings < totgames)
                                continue;

                            /* innings pitched */
                            tscore = 5.0 * (float) team.pitchers[x].pitching.innings / 9.0;
                            /* subtract earned runs */
                            tscore -= (float) team.pitchers[x].pitching.er;
                            /* add strikeouts */
                            tscore += ((float) team.pitchers[x].pitching.so / 12.0);
                            /* add saves */
                            tscore += ((float) team.pitchers[x].pitching.saves * 2.5);
                            /* add shutouts */
                            tscore += (float) team.pitchers[x].pitching.sho;
                            /* add wins & subtract losses */
                            tscore += (((float) team.pitchers[x].pitching.wins * 6.0) - ((float) team.pitchers[x].pitching.losses * 2.0));

                            /* add to score for being on a playoff team */
                            for (y = 0; y < psteamcnt; y++)
                                if (!strcmp (&psteams[y][0], dir->d_name)) {
                                    tscore += 12.0;
                                    break;
                                }

                            for (y = 0; y < 20; y++)
                                if (tscore > cyyoung[y].score) {
                                    for (z = 18; z >= y; z--)
                                        cyyoung[z + 1] = cyyoung[z];
                                    cyyoung[y].score = tscore;
                                    cyyoung[y].tyear = team.year;
                                    cyyoung[y].teamid = team.id;
                                    strcpy (&cyyoung[y].uctname[0], dir->d_name);
                                    /* check for "-PS" */
                                    if (!strncmp (&cyyoung[y].uctname[strlen (&cyyoung[y].uctname[0]) - 3], "-PS", 3))
                                        cyyoung[y].uctname[strlen (&cyyoung[y].uctname[0]) - 3] = '\0';
                                    strcpy (&cyyoung[y].name[0], &team.pitchers[x].id.name[0]);
                                    break;
                                }
                        }

                    if (buffer[1] == 'G')
                        /* Gold Gloves
                           minimum - pitchers IP => G team played
                                     catchers G => G team played / 2
                                     all other positions G (at the position in question) => G team played * 2 / 3 */
                        for (x = 0; x < maxplayers[0]; x++)
                            for (y = 1; y < 10; y++) {
                                /* DH doesn't qualify for a Gold Glove award */
                                /* to be considered player must have a minimum amount of time at that position */
                                if (y == 1) {
                                    /* find pitching stats for this player */
                                    for (z = 0; z < maxpitchers[0]; z++)
                                        if (!strcmp (&team.batters[x].id.name[0], &team.pitchers[z].id.name[0])) {
                                            if (team.pitchers[z].pitching.innings < totgames)
                                                z = maxpitchers[0];
                                            break;
                                        }
                                    if (z == maxpitchers[0])
                                        continue;
                                }
                                else
                                    if (y == 2) {
                                        if (team.batters[x].fielding[y].games < (totgames / 2))
                                            continue;
                                    }
                                    else
                                        if (team.batters[x].fielding[y].games < (totgames * 2 / 3))
                                            continue;

                                tdividend = team.batters[x].fielding[y].po + team.batters[x].fielding[y].a;
                                tdivisor = team.batters[x].fielding[y].po + team.batters[x].fielding[y].a + team.batters[x].fielding[y].e;
                                tscore = (float) tdividend / (float) tdivisor;
                                if (tscore > goldglove[y].pct || (tscore == goldglove[y].pct && tdivisor > goldglove[y].tc)) {
                                    goldglove[y].pct = tscore;
                                    goldglove[y].tyear = team.year;
                                    goldglove[y].teamid = team.id;
                                    goldglove[y].err = team.batters[x].fielding[y].e;
                                    goldglove[y].tc = team.batters[x].fielding[y].e + team.batters[x].fielding[y].po + team.batters[x].fielding[y].a;
                                    strcpy (&goldglove[y].uctname[0], dir->d_name);
                                    /* check for "-PS" */
                                    if (!strncmp (&goldglove[y].uctname[strlen (&goldglove[y].uctname[0]) - 3], "-PS", 3))
                                        goldglove[y].uctname[strlen (&goldglove[y].uctname[0]) - 3] = '\0';
                                    strcpy (&goldglove[y].name[0], &team.batters[x].id.name[0]);
                                }
                            }

                    if (buffer[1] == 'S')
                        /* Silver Sluggers
                           minimum - pitchers IP => G team played
                                     catchers G => G team played / 2
                                     all other positions G (at the position in question) => G team played * 2 / 3
                           calculation - (TB + SB + BB + HBP - CS) / (AB - H + CS + GIDP) */
                        for (x = 0; x < maxplayers[0]; x++)
                            for (y = 0; y < 10; y++) {
                                /* to be considered player must have a minimum amount of time at that position */
                                if (y == 1) {
                                    /* find pitching stats for this player */
                                    for (z = 0; z < maxpitchers[0]; z++)
                                        if (!strcmp (&team.batters[x].id.name[0], &team.pitchers[z].id.name[0])) {
                                            if (team.pitchers[z].pitching.innings < totgames)
                                                z = maxpitchers[0];
                                            break;
                                        }
                                    if (z == maxpitchers[0])
                                        continue;
                                }
                                else
                                    if (y == 2) {
                                        if (team.batters[x].fielding[y].games < (totgames / 2))
                                            continue;
                                    }
                                    else
                                        if (team.batters[x].fielding[y].games < (totgames * 2 / 3))
                                            continue;

                                /* total bases */
                                tdividend = team.batters[x].hitting.hits - (team.batters[x].hitting.homers +
                                                               team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
                                tdividend += ((team.batters[x].hitting.homers * 4) + (team.batters[x].hitting.triples * 3) +
                                                                                     (team.batters[x].hitting.doubles * 2));
                                /* add stolen bases, walks & hit by pitches */
                                tdividend += (team.batters[x].hitting.sb + team.batters[x].hitting.bb + team.batters[x].hitting.hbp);
                                /* subtract caught stealing */
                                tdividend -= team.batters[x].hitting.cs;

                                /* at bats */
                                tdivisor = team.batters[x].hitting.atbats;
                                /* subtract hits */
                                tdivisor -= team.batters[x].hitting.hits;
                                /* add caught stealing & grounded into DP */
                                tdivisor += (team.batters[x].hitting.cs + team.batters[x].hitting.gidp);

                                tscore = (float) tdividend / (float) tdivisor;
                                if (tscore > silverslugger[y].score) {
                                    silverslugger[y].score = tscore;
                                    silverslugger[y].tyear = team.year;
                                    silverslugger[y].teamid = team.id;
                                    strcpy (&silverslugger[y].uctname[0], dir->d_name);
                                    /* check for "-PS" */
                                    if (!strncmp (&silverslugger[y].uctname[strlen (&silverslugger[y].uctname[0]) - 3], "-PS", 3))
                                        silverslugger[y].uctname[strlen (&silverslugger[y].uctname[0]) - 3] = '\0';
                                    strcpy (&silverslugger[y].name[0], &team.batters[x].id.name[0]);
                                }
                            }
                }
                closedir (fnames);
                buffer1[0] = '\0';

                if (buffer[1] == 'M')
                    for (x = 0; x < 20; x++)
                        if (strlen (&mvp[x].name[0])) {
                            strcat (&buffer1[0], &mvp[x].name[0]);
                            strcat (&buffer1[0], "??");
                            strcat (&buffer1[0], (char *) cnvt_int2str (4, mvp[x].tyear));
                            strcat (&buffer1[0], (char *) cnvt_int2str (2, mvp[x].teamid));
                            if (!mvp[x].tyear) {
                                strcat (&buffer1[0], " ");
                                strcat (&buffer1[0], &mvp[x].uctname[0]);
                                strcat (&buffer1[0], " ");
                            }
                            sprintf (tbuf, "%10.5f", mvp[x].score);
                            strcat (&buffer1[0], &tbuf[0]);
                        }
                        else
                            break;

                if (buffer[1] == 'C')
                    for (x = 0; x < 20; x++)
                        if (strlen (&cyyoung[x].name[0])) {
                            strcat (&buffer1[0], &cyyoung[x].name[0]);
                            strcat (&buffer1[0], "??");
                            strcat (&buffer1[0], (char *) cnvt_int2str (4, cyyoung[x].tyear));
                            strcat (&buffer1[0], (char *) cnvt_int2str (2, cyyoung[x].teamid));
                            if (!cyyoung[x].tyear) {
                                strcat (&buffer1[0], " ");
                                strcat (&buffer1[0], &cyyoung[x].uctname[0]);
                                strcat (&buffer1[0], " ");
                            }
                            sprintf (tbuf, "%10.5f", cyyoung[x].score);
                            strcat (&buffer1[0], &tbuf[0]);
                        }
                        else
                            break;

                if (buffer[1] == 'G')
                    for (x = 1; x < 10; x++)
                        if (strlen (&goldglove[x].name[0])) {
                            strcat (&buffer1[0], &goldglove[x].name[0]);
                            strcat (&buffer1[0], "??");
                            strcat (&buffer1[0], (char *) cnvt_int2str (4, goldglove[x].tyear));
                            strcat (&buffer1[0], (char *) cnvt_int2str (2, goldglove[x].teamid));
                            if (!goldglove[x].tyear) {
                                strcat (&buffer1[0], " ");
                                strcat (&buffer1[0], &goldglove[x].uctname[0]);
                                strcat (&buffer1[0], " ");
                            }
                            sprintf (tbuf, "%10.5f", goldglove[x].pct);
                            strcat (&buffer1[0], &tbuf[0]);
                        }
                        else
                            strcat (&buffer1[0], "NONAME??1111000000000000");

                if (buffer[1] == 'S')
                    for (x = 0; x < 10; x++)
                        if (strlen (&silverslugger[x].name[0])) {
                            strcat (&buffer1[0], &silverslugger[x].name[0]);
                            strcat (&buffer1[0], "??");
                            strcat (&buffer1[0], (char *) cnvt_int2str (4, silverslugger[x].tyear));
                            strcat (&buffer1[0], (char *) cnvt_int2str (2, silverslugger[x].teamid));
                            if (!silverslugger[x].tyear) {
                                strcat (&buffer1[0], " ");
                                strcat (&buffer1[0], &silverslugger[x].uctname[0]);
                                strcat (&buffer1[0], " ");
                            }
                            sprintf (tbuf, "%10.5f", silverslugger[x].score);
                            strcat (&buffer1[0], &tbuf[0]);
                        }
                        else
                            strcat (&buffer1[0], "NONAME??1111000000000000");

                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }
        }

        if (buffer[0] == 'B') {
            /* user wants to see best real life teams over certain years */
            int x, z, pos, yrs[YEAR_SPREAD], rfor, ragainst, yr, totr, nteams, avgr, psrdiffg;
            float rspct, pspct, rsrdiff, rsgdiff, rsraa, psrdiff, w;
            char work[500], results[5000], dummy1[256], dummy2[256], tname[100], *period, *nl, *dash;

            struct {
                int tyear, tid;
                float score;
            } teams[101];

            for (x = 0; x < YEAR_SPREAD; x++)
                yrs[x] = 0;

            for (x = 0; x < 101; x++)
                teams[x].score = 0.0;

            /* get regular season won/loss percentage strength */
            for (x = 0, pos = 1; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            rspct = (float) atoi (&work[0]) / 100.0;

            /* get post season won/loss percentage strength */
            for (pos++, x = 0; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            pspct = (float) atoi (&work[0]) / 100.0;

            /* get regular season run differential strength */
            for (pos++, x = 0; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            rsrdiff = (float) atoi (&work[0]) / 100.0;

            /* get regular season runs above average strength */
            for (pos++, x = 0; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            rsraa = (float) atoi (&work[0]) / 100.0;

            /* get regular season games ahead of 2nd place team strength */
            for (pos++, x = 0; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            rsgdiff = (float) atoi (&work[0]) / 100.0;

            /* get post season run differential strength */
            for (pos++, x = 0; buffer[pos] != ' '; x++, pos++)
                work[x] = buffer[pos];
            work[x] = '\0';
            psrdiff = (float) atoi (&work[0]) / 100.0;

            /* get post season run differential indicator to take into account # of PS games */
            pos++;
            if (buffer[pos] == '1')
                psrdiffg = 1;
            else
                psrdiffg = 0;
            pos++;

            /* get years to include in search */
            for (pos++, x = 0; x < YEAR_SPREAD; x++, pos++)
                if (buffer[pos] == '1')
                    yrs[x] = 1;
            /* 1981 is NEVER included */
            yrs[1981 - 1901] = 0;

            /* get data to use */
            strcpy (&dummy[0], "/var/NSB/RealLifeStats/");
            for (yr = 0; yr < YEAR_SPREAD; yr++)
                if (yrs[yr]) {
                    for (x = 0; x < 5000; x++)
                        results[x] = '\0';

                    strcpy (&dummy1[0], &dummy[0]);
                    strcat (&dummy1[0], (char *) cnvt_int2str (4, (1901 + yr)));
                    strcat (&dummy1[0], "/Results");

                    if ((in = fopen (dummy1, "r")) != NULL) {
                        fread (&results, sizeof results, 1, in);
                        fclose (in);
                    }
                    else {
                        strcpy (&buffer1[0], "-1\n");
                        sock_puts (sock, &buffer1[0]);
                        continue;
                    }

                    strcpy (&dummy1[0], &dummy[0]);
                    strcat (&dummy1[0], (char *) cnvt_int2str (4, (1901 + yr)));

                    /* go through all the teams twice ... this first time through accumulate total runs scored in the league and number
                       of teams in league so as to find average runs per team */
                    if (rsraa > 0.0) {
                        totr = nteams = 0;
                        if ((fnames = opendir (&dummy1[0])) != NULL) {
                            while ((dir = readdir (fnames))) {
                                /* don't process . and .. files */
                                if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                    continue;
                                /* don't process the Results file */
                                if (strstr (dir->d_name, "Results"))
                                    continue;

                                strcpy (&dummy2[0], &dummy1[0]);
                                strcat (&dummy2[0], "/");
                                strcat (&dummy2[0], dir->d_name);

                                if ((in = fopen (dummy2, "r")) != NULL) {
                                    fread (&team.id, sizeof team.id, 1, in);
                                    fread (&team.year, sizeof team.year, 1, in);
                                    fread (&team.league, sizeof team.league, 1, in);
                                    fread (&team.division, sizeof team.division, 1, in);
                                    for (x = 0; x < 25; x++) {
                                        fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                        fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                        fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                        for (y = 0; y < 11; y++)
                                            fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                                    }
                                    for (x = 0; x < 11; x++) {
                                        fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                        fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                                    }
                                    fclose (in);
                                }
                                else {
                                    if (syslog_ent == YES)
                                        syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                                    return -1;
                                }

                                /* determine the number of players and pitchers this team has */
                                for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                                    if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                        break;
                                for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                                    if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                        break;

                                /* accumulate total runs */
                                for (x = 0; x < maxplayers[0]; x++)
                                    totr += team.batters[x].hitting.runs;
                                /* accumulate number of teams in league */
                                nteams++;
                            }
                        }
                        avgr = totr / nteams;
                    }

                    if ((fnames = opendir (&dummy1[0])) != NULL) {
                        while ((dir = readdir (fnames))) {
                            /* don't process . and .. files */
                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                continue;
                            /* don't process the Results file */
                            if (strstr (dir->d_name, "Results"))
                                continue;

                            strcpy (&dummy2[0], &dummy1[0]);
                            strcat (&dummy2[0], "/");
                            strcat (&dummy2[0], dir->d_name);

                            if ((in = fopen (dummy2, "r")) != NULL) {
                                fread (&team.id, sizeof team.id, 1, in);
                                fread (&team.year, sizeof team.year, 1, in);
                                fread (&team.league, sizeof team.league, 1, in);
                                fread (&team.division, sizeof team.division, 1, in);
                                for (x = 0; x < 25; x++) {
                                    fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                    fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                    fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                    for (y = 0; y < 11; y++)
                                        fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                                }
                                for (x = 0; x < 11; x++) {
                                    fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                    fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                                }
                                fclose (in);
                            }
                            else {
                                if (syslog_ent == YES)
                                    syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                                return -1;
                            }

                            /* get the team name */
                            strcpy (&tname[0], (char *) GetTeamName (team.id));
                            /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                            if (!strncmp (&tname[0], "St ", 3)) {
                                strcpy (&work[0], &tname[0]);
                                tname[2] = '.';
                                tname[3] = ' ';
                                strcpy (&tname[4], &work[3]);
                            }

                            /* determine the number of players and pitchers this team has */
                            for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                                if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                    break;
                            for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                                if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                    break;

                            teams[100].score = 0.0;
                            teams[100].tyear = team.year;
                            teams[100].tid = team.id;

                            if (rsrdiff > 0.0 || rsraa > 0.0)
                                /* accumulate runs for */
                                for (rfor = x = 0; x < maxplayers[0]; x++)
                                    rfor += team.batters[x].hitting.runs;

                            if (rsrdiff > 0.0) {
                                /* accumulate runs against */
                                for (ragainst = x = 0; x < maxpitchers[0]; x++)
                                    ragainst += team.pitchers[x].pitching.runs;

                                teams[100].score = ((float) rfor - (float) ragainst) * rsrdiff;
                            }

                            if (rsraa > 0.0)
                                if (rfor > avgr)
                                    teams[100].score += ((rfor - avgr) * rsraa);

                            if (rspct > 0.0 || pspct > 0.0 || psrdiff > 0.0 || rsgdiff > 0.0)
                                /* find team in regular season results */
                                for (pos = 0; pos < strlen (&results[0]); pos++)
                                    if (!strncmp (&results[pos], &tname[0], strlen (&tname[0]))) {
                                        if (rspct > 0.0) {
                                            period = index (&results[pos + 3], '.');   /* need to start past a possible "St." start to team name */
                                            strncpy (&work[0], period + 1, 3);
                                            work[3] = '\0';
                                            z = atoi (&work[0]);
                                            teams[100].score += ((float) z * rspct);
                                        }
                                        if (rsgdiff > 0.0) {
                                            nl = index (&results[pos], '\n');
                                            *nl = '\0';
                                            dash = index (&results[pos], '-');
                                            *nl = '\n';
                                            if (dash != NULL) {
                                                for (x = pos; x < strlen (&results[0]); x++)
                                                    if (results[x] == '\n') {
                                                        x++;
                                                        break;
                                                    }
                                                if (x != strlen (&results[0])) {
                                                    /* need to start past a possible "St." start to team name */
                                                    period = index (&results[x + 3], '.');
                                                    period += 4;       /* get past percentage */
                                                    /* get to the first digit of games back */
                                                    for ( ; *period != '\n' && *period != '\0'; period++)
                                                        if (*period != ' ')
                                                            break;
                                                    for (x = 0; *period != '\n' && *period != '.' && *period != '\0'; x++, period++)
                                                        work[x] = *period;
                                                    work[x] = '\0';
                                                    w = atof (&work[0]);
                                                    /* 1/2 game? */
                                                    if (*period == '.')
                                                        w += 0.5;
                                                    teams[100].score += (w * rsgdiff);
                                                }
                                            }
                                        }

                                        if (pspct > 0.0 || psrdiff > 0.0) {
                                            int psw, wins, losses, rf, ra, pct;
                                            float rdiff;
                                            char *dash, wr[3];

                                            wins = losses = rf = ra = rdiff = 0;

                                            for ( ; pos < strlen (&results[0]); pos++)
                                                if (!strncmp (&results[pos], "Post Season Play", 16)) {
                                                    for ( ; pos < strlen (&results[0]); pos++)
                                                        if (results[pos] == '\n') {
                                                            pos++;
                                                            break;
                                                        }
                                                    break;
                                                }
                                            if (pos >= strlen (&results[0]))
                                                break;
                                            /* look for team in post season */
                                            for ( ; pos < strlen (&results[0]); pos++)
                                                if (!strncmp (&results[pos], &tname[0], strlen (&tname[0]))) {
                                                     for (x = pos; x < strlen (&results[0]); x++)
                                                         if (results[x] == '\n') {
                                                             results[x] = '\0';
                                                             if (strstr (&results[pos], " beat "))
                                                                 psw = 1;
                                                             else
                                                                 psw = 0;

                                                             dash = strstr (&results[pos], " - ");
                                                             if (dash != NULL) {
                                                                 if (psw) {
                                                                     wins += (*(dash - 1) - '0');
                                                                     losses += (*(dash + 3) - '0');
                                                                 }
                                                                 else {
                                                                     wins += (*(dash + 3) - '0');
                                                                     losses += (*(dash - 1) - '0');
                                                                 }
                                                             }
                                                             results[x] = '\n';
                                                             break;
                                                         }
                                                     /* go to next line (game score line) */
                                                     for (x = pos; x < strlen (&results[0]); x++)
                                                         if (results[x] == '\n')
                                                             break;
                                                     x += 2;    /* first position of first game score */
                                                     if (psw) {
                                                         for (y = 0; results[x] != '-'; x++, y++)
                                                             wr[y] = results[x];
                                                         wr[y] = '\0';
                                                         rf += atoi (&wr[0]);

                                                         x++;

                                                         for (y = 0; results[x] != ',' && results[x] != ')'; x++, y++)
                                                             wr[y] = results[x];
                                                         wr[y] = '\0';
                                                         ra += atoi (&wr[0]);
                                                     }
                                                     else {
                                                         for (y = 0; results[x] != '-'; x++, y++)
                                                             wr[y] = results[x];
                                                         wr[y] = '\0';
                                                         ra += atoi (&wr[0]);

                                                         x++;

                                                         for (y = 0; results[x] != ',' && results[x] != ')'; x++, y++)
                                                             wr[y] = results[x];
                                                         wr[y] = '\0';
                                                         rf += atoi (&wr[0]);
                                                     }
                                                }

                                            if (pspct > 0.0 && (wins || losses)) {
                                                pct = (int) (((float) wins / (float) (wins + losses)) * 1000);
                                                teams[100].score += ((float) pct * pspct);
                                            }
                                            if (psrdiff > 0.0 && (rf || ra)) {
                                                if (psrdiffg)
                                                    /* take # of PS games into account */
                                                    rdiff = (float) (rf - ra) / (float) (wins + losses);
                                                else
                                                    rdiff = (float) (rf - ra);
                                                teams[100].score += rdiff * psrdiff;
                                            }
                                        }
                                        break;
                                    }

                            for (x = 0; x < 100; x++)
                                if (teams[100].score > teams[x].score) {
                                    for (z = 98; z >= x; z--)
                                        teams[z + 1] = teams[z];
                                    teams[x].score = teams[100].score;
                                    teams[x].tyear = teams[100].tyear;
                                    teams[x].tid = teams[100].tid;

                                    break;
                                }
                        }
                    }
                    closedir (fnames);
                    buffer1[0] = '\0';
                }

            for (buffer1[0] = '\0', x = 0; x < 100; x++)
                if (teams[x].score != 0.0) {
                    sprintf (work, "%d %d %10.10f ", teams[x].tyear, teams[x].tid, teams[x].score);
                    strcat (&buffer1[0], &work[0]);
                }
                else
                    break;

            strcat (&buffer1[0], "\n");
            sock_puts (sock, &buffer1[0]);
        }

        if (buffer[0] == 'V' || buffer[0] == 'v') {
            int res;

            res = formula ();
            /* to avoid a compile warning */
            if (res)
                continue;
            else
                continue;
        }

        if (buffer[0] == 'S') {
            /*
               user wants to see some stats
 
               the valid requests are:
               S1 - NSB season standings
               S3YYYY - Real Life season results for year YYYY
               SXYYYYABCDE - stat category leaders
                             X = 2 for Real Life, 4 for NSB current regular season, 5 for records,
                                 a for lifetime regular season, b for NSB current post-season,
                                 c for lifetime post-season
                             YYYY = year (if X = 2, there will be a span of years starting in position 12)
                             A = 1 for batting, 2 for pitching, 3 for fielding
                             B = X for stat category (see code)
                             C = n for position (meaningful only for fielding)
                             D = 1 for game, 2 for season (meaningful only for records)
                             E = 1 for user, 2 for all users (meaningful only for records)
               S6 - names of NSB teams available including post-season teams
               S61 - names of teams with NSB Lifetime stats available including post-season stats
               S62 - Real Life years available
               S63 - all Real Life teams available
               S64 - all user-created teams available
               S6YYYY - names of Real Life teams available for year YYYY
               S6XYYYYZZ - team stats
                           X = 2 for Real Life, 4 for NSB current regular season, 5 for NSB lifetime regular season,
                               6 for NSB current post-season, 7 for NSB lifetime post-season
                           YYYY = year
                           ZZ = team ID
               S68teamname - user-created team stats
               S691 - team totals for current NSB season regular season
               S692 - team totals for current NSB season post season
               S693 - team totals for user-created teams
               S691YYYY - team totals for real life regular season
                          YYYY = year
               S7 - check for the presence of a currently operating season
               S71 - check for the presence of residuals left over from a season (in this case the post-season
                     is completed and a new season has yet to be formed)
               S81 - check for the existence of records for this user
               S82 - check for the existence of records for all users
               S9playername - return all stats for the player matching playername
               S0 - check for the presence of a currently operating series
               SA - send back series status
            */
            if (buffer[1] == '1') {
                /* NSB season standings */
                struct {
                    int id, year, wins;
                } pteams[10];
                char wid[5], wy[5], buf[5000];
                int x, pos, vid, vy, hid, hy, vinc, hinc;

                for (x = 0; x < 10; x++)
                    pteams[x].id = pteams[x].wins = 0;

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Standings");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&teamwins[0], sizeof teamwins, 1, in);
                    fclose (in);
                }
                else {
                    strcpy (&buffer1[0], "-1\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;
                }

                for (buffer1[0] = work[1] = '\0', w = l = x = 0; teamwins[x].id != 0 && x < 300; x++, w = l = 0) {
                    int z;

                    for (y = 0; y < 300; y++)
                        if (teamwins[x].opp[y].id)
                            w += teamwins[x].opp[y].wins;
                    for (y = 0; teamwins[y].id != 0 && y < 300; y++)
                        if (teamwins[x].id != teamwins[y].id || teamwins[x].year != teamwins[y].year)
                            for (z = 0; z < 300; z++)
                                if (teamwins[y].opp[z].id == teamwins[x].id && teamwins[y].opp[z].year == teamwins[x].year)
                                    l += teamwins[y].opp[z].wins;
                    work[0] = teamwins[x].league;
                    strcat (&buffer1[0], &work[0]);
                    work[0] = teamwins[x].div;
                    strcat (&buffer1[0], &work[0]);
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, teamwins[x].year));
                    strcat (&buffer1[0], (char *) cnvt_int2str (3, teamwins[x].id));
                    if (teamwins[x].id >= 900) {
                        DIR *fnames;
                        struct dirent *dir;
                        char parent[256];

                        /* this is a user-created team ... we need to add the file/team name after the ID */
                        strcpy (&parent[0], "/var/NSB/");
                        strcat (&parent[0], &nsbdb[user].id[0]);
                        if ((fnames = opendir (&parent[0])) != NULL)
                            while ((dir = readdir (fnames))) {
                                /* don't process . and .. files */
                                if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                    continue;
                                /* don't process these files */
                                if (!strcmp (dir->d_name, "Schedule-PS") || !strcmp (dir->d_name, "Standings") ||
                                           !strcmp (dir->d_name, "PlayoffResultsAL") || !strcmp (dir->d_name, "PlayoffResultsNL")
                                           || !strcmp (dir->d_name, "Series"))
                                    continue;
                                if (!strcmp (dir->d_name, "Lifetime") || !strcmp (dir->d_name, "Records") || !strcmp (dir->d_name, "UserTeams"))
                                    continue;
                                if (strstr (dir->d_name, "-PS"))
                                    continue;
                                /* don't process the LeagueSetup file */
                                if (!strcmp (dir->d_name, "LeagueSetup"))
                                    continue;

                                /* look for any teams */
                                strcpy (&dummy[0], &parent[0]);
                                strcat (&dummy[0], "/");
                                strcat (&dummy[0], dir->d_name);

                                if ((in = fopen (dummy, "r")) != NULL) {
                                    fread (&team.id, sizeof team.id, 1, in);
                                    fread (&team.year, sizeof team.year, 1, in);
                                    fread (&team.league, sizeof team.league, 1, in);
                                    fread (&team.division, sizeof team.division, 1, in);
                                    for (z = 0; z < 25; z++) {
                                        fread (&team.batters[z].id, sizeof team.batters[z].id, 1, in);
                                        fread (&team.batters[z].dob, sizeof team.batters[z].dob, 1, in);
                                        fread (&team.batters[z].hitting, sizeof team.batters[z].hitting, 1, in);
                                        for (y = 0; y < 11; y++)
                                            fread (&team.batters[z].fielding[y], sizeof team.batters[z].fielding[y], 1, in);
                                    }
                                    for (z = 0; z < 11; z++) {
                                        fread (&team.pitchers[z].id, sizeof team.pitchers[z].id, 1, in);
                                        fread (&team.pitchers[z].pitching, sizeof team.pitchers[z].pitching, 1, in);
                                    }
                                    fclose (in);
                                }
                                if (team.id == teamwins[x].id) {
                                    strcat (&buffer1[0], dir->d_name);
                                    break;
                                }
                            }
                        closedir (fnames);
                    }
                    strcat (&buffer1[0], " ");
                    strcat (&buffer1[0], (char *) cnvt_int2str (3, w));
                    strcat (&buffer1[0], " ");
                    strcat (&buffer1[0], (char *) cnvt_int2str (3, l));
                    strcat (&buffer1[0], " ");
                }
                strcat (&buffer1[0], "::");
                /* append specific info depending upon status of completion of season */
                /*
                   position following double-colons is an indicator as such:
                   : - within regular season ... no further info
                   1 - completed regular season ... attach teams in post-season with first round match-ups
                   2 - within post-season ... results of completed series & status of any on-going series
                   3 - completed post-season ... results of series & name of champion
                   4, 5 - playoff results (in this case the data will be followed by :: which in turn will be
                          followed by :, 1, 2, or 3 above
                   6 - no post-season
                */

                /* load league set-up */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/LeagueSetup");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&league_setup, sizeof league_setup, 1, in);
                    fclose (in);
                }
                else {
                    if (syslog_ent == YES)
                        syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
                    return -1;
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Schedule-PS");
                if ((in = fopen (dummy, "r")) != NULL) {
                    for (x = 0; x < 37; x++)
                        fgets (&schedule[x][0], 3000, in);
                    fclose (in);
                }
                else {
                    strcat (&buffer1[0], ":\n");       /* within regular season */
                    sock_puts (sock, &buffer1[0]);
                    continue;                          /* get next request from client */
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/PlayoffResultsAL");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&buf, sizeof buf, 1, in);
                    fclose (in);

                    strcat (&buffer1[0], "4::");         /* AL playoff */
                    strcat (&buffer1[0], &buf[0]);
                    strcat (&buffer1[0], "::");
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/PlayoffResultsNL");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&buf, sizeof buf, 1, in);
                    fclose (in);

                    strcat (&buffer1[0], "5::");         /* NL playoff */
                    strcat (&buffer1[0], &buf[0]);
                    strcat (&buffer1[0], "::");
                }

                if (!strncmp (&schedule[0][4], "00000000", 8)) {
                    strcat (&buffer1[0], "6\n");       /* no post-season */
                    sock_puts (sock, &buffer1[0]);
                    continue;                          /* get next request from client */
                }

                if (schedule[0][12] == '-') {
                    int id;
                    char idc[5];

                    strcat (&buffer1[0], "1");         /* completed regular season, haven't started post-season yet */
                    /* attach match-ups */
                    strncat (&buffer1[0], &schedule[0][4], 8);
                    if (schedule[0][9] == '9') {
                        /* user-created team */
                        for (x = 0; x < 4; x++)
                            idc[x] = schedule[0][8 + x];
                        idc[4] = '\0';
                        id = atoi (&idc[0]);
                        strcat (&buffer1[0], " ");
                        strcat (&buffer1[0], GetUCTeamname (id));
                        strcat (&buffer1[0], " ");
                    }
                    strncat (&buffer1[0], &schedule[0][13], 8);
                    if (schedule[0][18] == '9') {
                        /* user-created team */
                        for (x = 0; x < 4; x++)
                            idc[x] = schedule[0][17 + x];
                        idc[4] = '\0';
                        id = atoi (&idc[0]);
                        strcat (&buffer1[0], " ");
                        strcat (&buffer1[0], GetUCTeamname (id));
                        strcat (&buffer1[0], " ");
                    }
                    if (strlen (&schedule[0][0]) > 23) {
                        strncat (&buffer1[0], &schedule[0][22], 8);
                        if (schedule[0][27] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][26 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                        strncat (&buffer1[0], &schedule[0][31], 8);
                        if (schedule[0][36] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][35 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                    }
                    if (strlen (&schedule[0][0]) > 41) {
                        strncat (&buffer1[0], &schedule[0][40], 8);
                        if (schedule[0][45] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][44 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                        strncat (&buffer1[0], &schedule[0][49], 8);
                        if (schedule[0][54] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][53 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                    }
                    if (strlen (&schedule[0][0]) > 59) {
                        strncat (&buffer1[0], &schedule[0][58], 8);
                        if (schedule[0][63] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][62 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                        strncat (&buffer1[0], &schedule[0][67], 8);
                        if (schedule[0][72] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][71 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                    }
                    if (strlen (&schedule[0][0]) > 77) {
                        strncat (&buffer1[0], &schedule[0][76], 8);
                        if (schedule[0][81] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][80 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                        strncat (&buffer1[0], &schedule[0][85], 8);
                        if (schedule[0][72] == '9') {
                            /* user-created team */
                            for (x = 0; x < 4; x++)
                                idc[x] = schedule[0][89 + x];
                            idc[4] = '\0';
                            id = atoi (&idc[0]);
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (id));
                            strcat (&buffer1[0], " ");
                        }
                    }
                    strcat (&buffer1[0], "\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;                          /* get next request from client */
                }
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Schedule");
                if ((in = fopen (dummy, "r")) != NULL) {
                    strcat (&buffer1[0], "2");         /* within post-season */
                    fclose (in);
                }
                else
                    strcat (&buffer1[0], "3");         /* completed post-season */
                /* attach series results and, whatever is applicable, status of current series or champions */
                strncpy (&wy[0], &schedule[0][4], 4);
                strncpy (&wid[0], &schedule[0][8], 4);
                wy[4] = wid[4] = '\0';
                pteams[0].id = atoi (&wid[0]);
                pteams[0].year = atoi (&wy[0]);
                strncpy (&wy[0], &schedule[0][13], 4);
                strncpy (&wid[0], &schedule[0][17], 4);
                wy[4] = wid[4] = '\0';
                pteams[1].id = atoi (&wid[0]);
                pteams[1].year = atoi (&wy[0]);
                if (strlen (&schedule[0][0]) > 23) {
                    strncpy (&wy[0], &schedule[0][22], 4);
                    strncpy (&wid[0], &schedule[0][26], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[2].id = atoi (&wid[0]);
                    pteams[2].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[0][31], 4);
                    strncpy (&wid[0], &schedule[0][35], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[3].id = atoi (&wid[0]);
                    pteams[3].year = atoi (&wy[0]);
                }
                if (strlen (&schedule[0][0]) > 41) {
                    strncpy (&wy[0], &schedule[0][40], 4);
                    strncpy (&wid[0], &schedule[0][44], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[4].id = atoi (&wid[0]);
                    pteams[4].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[0][49], 4);
                    strncpy (&wid[0], &schedule[0][53], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[5].id = atoi (&wid[0]);
                    pteams[5].year = atoi (&wy[0]);
                }
                if (strlen (&schedule[0][0]) > 59) {
                    strncpy (&wy[0], &schedule[0][58], 4);
                    strncpy (&wid[0], &schedule[0][62], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[6].id = atoi (&wid[0]);
                    pteams[6].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[0][67], 4);
                    strncpy (&wid[0], &schedule[0][71], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[7].id = atoi (&wid[0]);
                    pteams[7].year = atoi (&wy[0]);
                }
                if (strlen (&schedule[0][0]) > 77) {
                    strncpy (&wy[0], &schedule[0][76], 4);
                    strncpy (&wid[0], &schedule[0][80], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[8].id = atoi (&wid[0]);
                    pteams[8].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[0][85], 4);
                    strncpy (&wid[0], &schedule[0][89], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[9].id = atoi (&wid[0]);
                    pteams[9].year = atoi (&wy[0]);
                }
                for (x = 0; x < 9; x++)
                    for (pos = 12; pos < strlen (&schedule[x][0]); pos += 18) {
                        strncpy (&wy[0], &schedule[x][pos - 8], 4);
                        strncpy (&wid[0], &schedule[x][pos - 4], 4);
                        wy[4] = wid[4] = '\0';
                        vid = atoi (&wid[0]);
                        vy = atoi (&wy[0]);
                        strncpy (&wy[0], &schedule[x][pos + 1], 4);
                        strncpy (&wid[0], &schedule[x][pos + 5], 4);
                        wy[4] = wid[4] = '\0';
                        hid = atoi (&wid[0]);
                        hy = atoi (&wy[0]);

                        for (vinc = 0; vinc < 10; vinc++)
                            if (vid == pteams[vinc].id && vy == pteams[vinc].year)
                                break;
                        for (hinc = 0; hinc < 10; hinc++)
                            if (hid == pteams[hinc].id && hy == pteams[hinc].year)
                                break;
                        if (schedule[x][pos] == 'V')
                            pteams[vinc].wins++;
                        if (schedule[x][pos] == 'H')
                            pteams[hinc].wins++;
                    }
                for (x = 0; x < 10 && pteams[x].id; x += 2) {
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].year));
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].id));
                    if (pteams[x].id >= 900) {
                        /* user-created team */
                        strcat (&buffer1[0], " ");
                        strcat (&buffer1[0], GetUCTeamname (pteams[x].id));
                        strcat (&buffer1[0], " ");
                    }
                    if (pteams[x].wins == league_setup.nummaxgames[0] / 2 + 1)
                        switch (pteams[x].wins) {
                            case 1:
                                strcat (&buffer1[0], "A");
                                break;
                            case 2:
                                strcat (&buffer1[0], "B");
                                break;
                            case 3:
                                strcat (&buffer1[0], "C");
                                break;
                            case 4:
                                strcat (&buffer1[0], "D");
                                break;
                            case 5:
                                strcat (&buffer1[0], "E");
                        }
                    else
                        strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x].wins));

                    strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].year));
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].id));
                    if (pteams[x + 1].id >= 900) {
                        /* user-created team */
                        strcat (&buffer1[0], " ");
                        strcat (&buffer1[0], GetUCTeamname (pteams[x + 1].id));
                        strcat (&buffer1[0], " ");
                    }
                    if (pteams[x + 1].wins == league_setup.nummaxgames[0] / 2 + 1)
                        switch (pteams[x + 1].wins) {
                            case 1:
                                strcat (&buffer1[0], "A");
                                break;
                            case 2:
                                strcat (&buffer1[0], "B");
                                break;
                            case 3:
                                strcat (&buffer1[0], "C");
                                break;
                            case 4:
                                strcat (&buffer1[0], "D");
                                break;
                            case 5:
                                strcat (&buffer1[0], "E");
                        }
                    else
                        strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x + 1].wins));
                }
                strcat (&buffer1[0], ":");                 /* end of first round play */

                /* look for second round action */
                if (strlen (&schedule[9][0]) < 6 && schedule[9][3] == 'X') {
                    /* there are only two teams in the post-season and therefore one round of play */
                    strcat (&buffer1[0], "X");                 /* end of post-season */
                    goto EndStandings;
                }
                if (strlen (&schedule[9][0]) > 5 && schedule[9][12] != '-') {
                    for (x = 0; x < 10; x++)
                        pteams[x].id = pteams[x].wins = 0;

                    strncpy (&wy[0], &schedule[9][4], 4);
                    strncpy (&wid[0], &schedule[9][8], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[0].id = atoi (&wid[0]);
                    pteams[0].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[9][13], 4);
                    strncpy (&wid[0], &schedule[9][17], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[1].id = atoi (&wid[0]);
                    pteams[1].year = atoi (&wy[0]);
                    if (strlen (&schedule[9][0]) > 23) {
                        strncpy (&wy[0], &schedule[9][22], 4);
                        strncpy (&wid[0], &schedule[9][26], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[2].id = atoi (&wid[0]);
                        pteams[2].year = atoi (&wy[0]);
                        strncpy (&wy[0], &schedule[9][31], 4);
                        strncpy (&wid[0], &schedule[9][35], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[3].id = atoi (&wid[0]);
                        pteams[3].year = atoi (&wy[0]);
                    }
                    if (strlen (&schedule[9][0]) > 41) {
                        strncpy (&wy[0], &schedule[9][40], 4);
                        strncpy (&wid[0], &schedule[9][44], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[4].id = atoi (&wid[0]);
                        pteams[4].year = atoi (&wy[0]);
                        strncpy (&wy[0], &schedule[9][49], 4);
                        strncpy (&wid[0], &schedule[9][53], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[5].id = atoi (&wid[0]);
                        pteams[5].year = atoi (&wy[0]);
                    }
                    if (strlen (&schedule[9][0]) > 59) {
                        strncpy (&wy[0], &schedule[9][58], 4);
                        strncpy (&wid[0], &schedule[9][62], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[6].id = atoi (&wid[0]);
                        pteams[6].year = atoi (&wy[0]);
                        strncpy (&wy[0], &schedule[9][67], 4);
                        strncpy (&wid[0], &schedule[9][71], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[7].id = atoi (&wid[0]);
                        pteams[7].year = atoi (&wy[0]);
                    }
                    for (x = 9; x < 18; x++)
                        for (pos = 12; pos < strlen (&schedule[x][0]); pos += 18) {
                            strncpy (&wy[0], &schedule[x][pos - 8], 4);
                            strncpy (&wid[0], &schedule[x][pos - 4], 4);
                            wy[4] = wid[4] = '\0';
                            vid = atoi (&wid[0]);
                            vy = atoi (&wy[0]);
                            strncpy (&wy[0], &schedule[x][pos + 1], 4);
                            strncpy (&wid[0], &schedule[x][pos + 5], 4);
                            wy[4] = wid[4] = '\0';
                            hid = atoi (&wid[0]);
                            hy = atoi (&wy[0]);

                            for (vinc = 0; vinc < 8; vinc++)
                                if (vid == pteams[vinc].id && vy == pteams[vinc].year)
                                    break;
                            for (hinc = 0; hinc < 8; hinc++)
                                if (hid == pteams[hinc].id && hy == pteams[hinc].year)
                                    break;
                            if (schedule[x][pos] == 'V')
                                pteams[vinc].wins++;
                            if (schedule[x][pos] == 'H')
                                pteams[hinc].wins++;
                        }
                    for (x = 0; x < 8 && pteams[x].id; x += 2) {
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].id));
                        if (pteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x].wins == league_setup.nummaxgames[1] / 2 + 1)
                            switch (pteams[x].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x].wins));

                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].id));
                        if (pteams[x + 1].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x + 1].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x + 1].wins == league_setup.nummaxgames[1] / 2 + 1)
                            switch (pteams[x + 1].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x + 1].wins));
                    }
                    strcat (&buffer1[0], ":");                 /* end of second round play */
                }

                /* look for third round action */
                if (strlen (&schedule[18][0]) < 6 && schedule[18][3] == 'X') {
                    /* only two rounds of post-season play were necessary */
                    strcat (&buffer1[0], "X");                 /* end of post-season */
                    goto EndStandings;
                }
                if (strlen (&schedule[18][0]) > 5 && schedule[18][12] != '-') {
                    for (x = 0; x < 10; x++)
                        pteams[x].id = pteams[x].wins = 0;

                    strncpy (&wy[0], &schedule[18][4], 4);
                    strncpy (&wid[0], &schedule[18][8], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[0].id = atoi (&wid[0]);
                    pteams[0].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[18][13], 4);
                    strncpy (&wid[0], &schedule[18][17], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[1].id = atoi (&wid[0]);
                    pteams[1].year = atoi (&wy[0]);

                    if (strlen (&schedule[18][0]) > 23) {
                        strncpy (&wy[0], &schedule[18][22], 4);
                        strncpy (&wid[0], &schedule[18][26], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[2].id = atoi (&wid[0]);
                        pteams[2].year = atoi (&wy[0]);
                        strncpy (&wy[0], &schedule[18][31], 4);
                        strncpy (&wid[0], &schedule[18][35], 4);
                        wy[4] = wid[4] = '\0';
                        pteams[3].id = atoi (&wid[0]);
                        pteams[3].year = atoi (&wy[0]);
                    }

                    for (x = 18; x < 27; x++)
                        for (pos = 12; pos < strlen (&schedule[x][0]); pos += 18) {
                            strncpy (&wy[0], &schedule[x][pos - 8], 4);
                            strncpy (&wid[0], &schedule[x][pos - 4], 4);
                            wy[4] = wid[4] = '\0';
                            vid = atoi (&wid[0]);
                            vy = atoi (&wy[0]);
                            strncpy (&wy[0], &schedule[x][pos + 1], 4);
                            strncpy (&wid[0], &schedule[x][pos + 5], 4);
                            wy[4] = wid[4] = '\0';
                            hid = atoi (&wid[0]);
                            hy = atoi (&wy[0]);

                            for (vinc = 0; vinc < 4; vinc++)
                                if (vid == pteams[vinc].id && vy == pteams[vinc].year)
                                    break;
                            for (hinc = 0; hinc < 4; hinc++)
                                if (hid == pteams[hinc].id && hy == pteams[hinc].year)
                                    break;
                            if (schedule[x][pos] == 'V')
                                pteams[vinc].wins++;
                            if (schedule[x][pos] == 'H')
                                pteams[hinc].wins++;
                        }
                    for (x = 0; x < 4 && pteams[x].id; x += 2) {
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].id));
                        if (pteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x].wins == league_setup.nummaxgames[2] / 2 + 1)
                            switch (pteams[x].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x].wins));

                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].id));
                        if (pteams[x + 1].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x + 1].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x + 1].wins == league_setup.nummaxgames[2] / 2 + 1)
                            switch (pteams[x + 1].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x + 1].wins));
                    }
                    strcat (&buffer1[0], ":");                 /* end of third round play */
                }

                /* look for fourth round action */
                if (strlen (&schedule[27][0]) < 6 && schedule[27][3] == 'X') {
                    /* only three rounds of post-season play were necessary */
                    strcat (&buffer1[0], "X");                 /* end of post-season */
                    goto EndStandings;
                }
                if (strlen (&schedule[27][0]) > 5 && schedule[27][12] != '-') {
                    for (x = 0; x < 10; x++)
                        pteams[x].id = pteams[x].wins = 0;

                    strncpy (&wy[0], &schedule[27][4], 4);
                    strncpy (&wid[0], &schedule[27][8], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[0].id = atoi (&wid[0]);
                    pteams[0].year = atoi (&wy[0]);
                    strncpy (&wy[0], &schedule[27][13], 4);
                    strncpy (&wid[0], &schedule[27][17], 4);
                    wy[4] = wid[4] = '\0';
                    pteams[1].id = atoi (&wid[0]);
                    pteams[1].year = atoi (&wy[0]);

                    for (x = 27; x < 36; x++)
                        for (pos = 12; pos < strlen (&schedule[x][0]); pos += 18) {
                            strncpy (&wy[0], &schedule[x][pos - 8], 4);
                            strncpy (&wid[0], &schedule[x][pos - 4], 4);
                            wy[4] = wid[4] = '\0';
                            vid = atoi (&wid[0]);
                            vy = atoi (&wy[0]);
                            strncpy (&wy[0], &schedule[x][pos + 1], 4);
                            strncpy (&wid[0], &schedule[x][pos + 5], 4);
                            wy[4] = wid[4] = '\0';
                            hid = atoi (&wid[0]);
                            hy = atoi (&wy[0]);

                            for (vinc = 0; vinc < 4; vinc++)
                                if (vid == pteams[vinc].id && vy == pteams[vinc].year)
                                    break;
                            for (hinc = 0; hinc < 4; hinc++)
                                if (hid == pteams[hinc].id && hy == pteams[hinc].year)
                                    break;
                            if (schedule[x][pos] == 'V')
                                pteams[vinc].wins++;
                            if (schedule[x][pos] == 'H')
                                pteams[hinc].wins++;
                        }
                    for (x = 0; x < 2 && pteams[x].id; x += 2) {
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x].id));
                        if (pteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x].wins == league_setup.nummaxgames[3] / 2 + 1)
                            switch (pteams[x].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x].wins));

                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, pteams[x + 1].id));
                        if (pteams[x + 1].id >= 900) {
                            /* user-created team */
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], GetUCTeamname (pteams[x + 1].id));
                            strcat (&buffer1[0], " ");
                        }
                        if (pteams[x + 1].wins == league_setup.nummaxgames[3] / 2 + 1)
                            switch (pteams[x + 1].wins) {
                                case 1:
                                    strcat (&buffer1[0], "A");
                                    break;
                                case 2:
                                    strcat (&buffer1[0], "B");
                                    break;
                                case 3:
                                    strcat (&buffer1[0], "C");
                                    break;
                                case 4:
                                    strcat (&buffer1[0], "D");
                                    break;
                                case 5:
                                    strcat (&buffer1[0], "E");
                            }
                        else
                            strcat (&buffer1[0], (char *) cnvt_int2str (1, pteams[x + 1].wins));
                    }
                    strcat (&buffer1[0], ":");                 /* end of fourth round play */
                }

                if (schedule[35][3] == 'X')
                    /* four rounds of post-season play is the most that there can be */
                    strcat (&buffer1[0], "X");                 /* end of post-season */
EndStandings:
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }

            if (buffer[1] == '3') {
                /* real life season results */
                strcpy (&dummy[0], "/var/NSB/RealLifeStats/");
                strncat (&dummy[0], &buffer[2], 4);
                strcat (&dummy[0], "/Results");
                if ((in = fopen (dummy, "r")) != NULL) {
                    x = 0;
                    while ((buffer1[x] = fgetc (in)) != EOF) {
                        if (buffer1[x] == '\n' || buffer1[x] == '\r')
                            buffer1[x] = '*';
                        x++;
                    }
                    buffer1[x] = '\n';
                    buffer1[x + 1] = '\0';
                    fclose (in);
                }
                else {
                    strcpy (&buffer1[0], "-1\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;
                }
                sock_puts (sock, &buffer1[0]);
            }

            if (buffer[1] == '2' || buffer[1] == '4' || buffer[1] == 'a' || buffer[1] == 'b' || buffer[1] == 'c') {
                int eraoppbasw, xxx;

                /* stat category leaders */
                for (x = 0; x < 50; x++) {
                    leaders[x].name[0] = '\0';
                    for (xxx = 0; xxx < 7; xxx++)
                         leaders[x].stat[xxx] = 0;
                }
                strcpy (&dummy[0], "/var/NSB/");
                if (buffer[1] != '2')
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                if (buffer[1] == 'a' || buffer[1] == 'c')
                    strcat (&dummy[0], "/Lifetime");

                if (buffer[6] == '2' && (buffer[7] == 'u' || buffer[7] == 'w'))
                    for (x = 0; x < 50; x++)
                        leaders[x].stat[5] = 99999;

                if (buffer[1] == '2') {
                    /* user wants to see best real life stats over certain years */
                    int x, z, pos, yrs[YEAR_SPREAD], yr;
                    char dummy1[256], dummy2[256];

                    strcat (&dummy[0], "RealLifeStats/");

                    for (x = 0; x < YEAR_SPREAD; x++)
                        yrs[x] = 0;

                    /* get years to include in search */
                    for (pos = 11, x = 0; x < YEAR_SPREAD; x++, pos++)
                        if (buffer[pos] == '1')
                            yrs[x] = 1;

                    /* get data to use */
                    for (yr = 0; yr < YEAR_SPREAD; yr++)
                        if (yrs[yr]) {
                            strcpy (&dummy1[0], &dummy[0]);
                            strcat (&dummy1[0], (char *) cnvt_int2str (4, (1901 + yr)));

                            if ((fnames = opendir (&dummy1[0])) != NULL) {
                                while ((dir = readdir (fnames))) {
                                    /* don't process . and .. files */
                                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                        continue;
                                    /* don't process the Results file */
                                    if (strstr (dir->d_name, "Results"))
                                        continue;

                                    strcpy (&dummy2[0], &dummy1[0]);
                                    strcat (&dummy2[0], "/");
                                    strcat (&dummy2[0], dir->d_name);

                                    if ((in = fopen (dummy2, "r")) != NULL) {
                                        fread (&team.id, sizeof team.id, 1, in);
                                        fread (&team.year, sizeof team.year, 1, in);
                                        fread (&team.league, sizeof team.league, 1, in);
                                        fread (&team.division, sizeof team.division, 1, in);
                                        for (x = 0; x < 25; x++) {
                                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                            for (y = 0; y < 11; y++)
                                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                                        }
                                        for (x = 0; x < 11; x++) {
                                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                                        }
                                        fclose (in);

                                        totgames = GetTotGames (1901 + yr, team.id);
                                    }
                                    else {
                                        if (syslog_ent == YES)
                                            syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                                        return -1;
                                    }
                                    /* determine the number of players and pitchers this team has */
                                    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                            break;
                                    for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                                        if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                            break;

                                    if (buffer[6] == '1') {
                                        for (x = 0; x < maxplayers[0]; x++)
                                            for (y = 0; y < 50; y++)
                                                if (cmp_stat (x, y)) {
                                                    for (z = 48; z >= y; z--)
                                                        leaders[z + 1] = leaders[z];
                                                    cp_stat (x, y);
                                                    leaders[y].tyear = team.year;
                                                    leaders[y].teamid = team.id;
                                                    strcpy (&leaders[y].name[0], &team.batters[x].id.name[0]);
                                                    break;
                                                }
                                    }
                                    if (buffer[6] == '2') {
                                        for (x = 0; x < maxpitchers[0]; x++)
                                            for (y = 0; y < 50; y++)
                                                if (cmp_stat (x, y)) {
                                                    for (z = 48; z >= y; z--)
                                                        leaders[z + 1] = leaders[z];
                                                    cp_stat (x, y);
                                                    leaders[y].tyear = team.year;
                                                    leaders[y].teamid = team.id;
                                                    strcpy (&leaders[y].name[0], &team.pitchers[x].id.name[0]);
                                                    break;
                                                }
                                    }
                                    if (buffer[6] == '3') {
                                        for (x = 0; x < maxplayers[0]; x++)
                                            for (y = 0; y < 50; y++)
                                                if (cmp_stat (x, y)) {
                                                    for (z = 48; z >= y; z--)
                                                        leaders[z + 1] = leaders[z];
                                                    cp_stat (x, y);
                                                    leaders[y].tyear = team.year;
                                                    leaders[y].teamid = team.id;
                                                    strcpy (&leaders[y].name[0], &team.batters[x].id.name[0]);
                                                    break;
                                                }
                                    }
                                }
                                closedir (fnames);
                            }
                        }
                }
                else
                    if ((fnames = opendir (&dummy[0])) != NULL) {
                        while ((dir = readdir (fnames))) {
                            if (buffer[1] == 'b' || buffer[1] == 'c') {
                                if (strstr (dir->d_name, "-PS"))
                                    if (!strstr (dir->d_name, "Schedule"))
                                        goto do_team;
                                    else
                                        continue;
                                else
                                    continue;
                            }
                            /* don't process . and .. files */
                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                continue;
                            /* don't process the schedule, standings or records */
                            if (!strcmp (dir->d_name, "Schedule") || !strcmp (dir->d_name, "Standings") ||
                                         !strcmp (dir->d_name, "PlayoffResultsAL") || !strcmp (dir->d_name, "PlayoffResultsNL") ||
                                         !strcmp (dir->d_name, "Series") || !strcmp (dir->d_name, "Records"))
                                continue;
                            /* don't process the Results file */
                            if (strstr (dir->d_name, "Results"))
                                continue;
                            /* don't process post-season stats */
                            if (strstr (dir->d_name, "-PS"))
                                continue;
                            /* don't process the Lifetime directory */
                            if (!strcmp (dir->d_name, "Lifetime"))
                                continue;
                            /* don't process the UserTeams directory */
                            if (!strcmp (dir->d_name, "UserTeams"))
                                continue;
                            /* don't process the LeagueSetup file */
                            if (!strcmp (dir->d_name, "LeagueSetup"))
                                continue;
do_team:
                            strcpy (&dummy2[0], &dummy[0]);
                            strcat (&dummy2[0], "/");
                            strcat (&dummy2[0], dir->d_name);

                            totgames = 0;
                            if ((in = fopen (dummy2, "r")) != NULL) {
                                fread (&team.id, sizeof team.id, 1, in);
                                fread (&team.year, sizeof team.year, 1, in);
                                fread (&team.league, sizeof team.league, 1, in);
                                fread (&team.division, sizeof team.division, 1, in);
                                for (x = 0; x < 25; x++) {
                                    fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                    fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                    fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                    for (y = 0; y < 11; y++)
                                        fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                                }
                                for (x = 0; x < 11; x++) {
                                    fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                    fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                                    if (buffer[1] != '2')
                                        totgames += team.pitchers[x].pitching.games_started;
                                }
                                fclose (in);
                            }
                            else {
                                if (syslog_ent == YES)
                                    syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                                return -1;
                            }
                            /* determine the number of players and pitchers this team has */
                            for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                                if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                    break;
                            for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                                if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                    break;

                            if (buffer[6] == '1') {
                                for (x = 0; x < maxplayers[0]; x++)
                                    for (y = 0; y < 50; y++)
                                        if (cmp_stat (x, y)) {
                                            for (z = 48; z >= y; z--)
                                                leaders[z + 1] = leaders[z];
                                            cp_stat (x, y);
                                            leaders[y].tyear = team.year;
                                            leaders[y].teamid = team.id;
                                            strcpy (&leaders[y].uctname[0], dir->d_name);
                                            /* check for "-PS" */
                                            if (!strncmp (&leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3], "-PS", 3))
                                                leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3] = '\0';
                                            strcpy (&leaders[y].name[0], &team.batters[x].id.name[0]);
                                            break;
                                        }
                            }
                            if (buffer[6] == '2') {
                                for (x = 0; x < maxpitchers[0]; x++)
                                    for (y = 0; y < 50; y++)
                                        if (cmp_stat (x, y)) {
                                            for (z = 48; z >= y; z--)
                                                leaders[z + 1] = leaders[z];
                                            cp_stat (x, y);
                                            leaders[y].tyear = team.year;
                                            leaders[y].teamid = team.id;
                                            strcpy (&leaders[y].uctname[0], dir->d_name);
                                            /* check for "-PS" */
                                            if (!strncmp (&leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3], "-PS", 3))
                                                leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3] = '\0';
                                            strcpy (&leaders[y].name[0], &team.pitchers[x].id.name[0]);
                                            break;
                                        }
                            }
                            if (buffer[6] == '3') {
                                for (x = 0; x < maxplayers[0]; x++)
                                    for (y = 0; y < 50; y++)
                                        if (cmp_stat (x, y)) {
                                            for (z = 48; z >= y; z--)
                                                leaders[z + 1] = leaders[z];
                                            cp_stat (x, y);
                                            leaders[y].tyear = team.year;
                                            leaders[y].teamid = team.id;
                                            strcpy (&leaders[y].uctname[0], dir->d_name);
                                            /* check for "-PS" */
                                            if (!strncmp (&leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3], "-PS", 3))
                                                leaders[y].uctname[strlen (&leaders[y].uctname[0]) - 3] = '\0';
                                            strcpy (&leaders[y].name[0], &team.batters[x].id.name[0]);
                                            break;
                                        }
                            }
                        }
                        closedir (fnames);
                    }

                buffer1[0] = '\0';
                if (strlen (&leaders[0].name[0]) == 0)
                    strcat (&buffer1[0], "-1");
                else {
                    if (buffer[6] == '2' && (buffer[7] == 'u' || buffer[7] == 'w'))
                        eraoppbasw = 1;
                    else
                        eraoppbasw = 0;
                    sortleaders (eraoppbasw, buffer[7]);  /* this sort will rearrange ties */
                    for (x = 0; x < 50; x++)
                        if (strlen (&leaders[x].name[0]) && (eraoppbasw || (!eraoppbasw && leaders[x].stat[5]))) {
                            strcat (&buffer1[0], &leaders[x].name[0]);
                            strcat (&buffer1[0], "??");
                            strcat (&buffer1[0], (char *) cnvt_int2str (4, leaders[x].tyear));
                            strcat (&buffer1[0], (char *) cnvt_int2str (2, leaders[x].teamid));
                            if (!leaders[x].tyear) {
                                strcat (&buffer1[0], " ");
                                strcat (&buffer1[0], &leaders[x].uctname[0]);
                                strcat (&buffer1[0], " ");
                            }
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[0]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[1]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[2]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[3]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[4]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[5]));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, leaders[x].stat[6]));
                        }
                        else
                            break;
                }

                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }

            if (buffer[1] == '5') {
                /* records in stat categories */
                zz = buffer[6] - '0';
                pos = (buffer[8] - '0') - 1;
                z = (buffer[9] - '0') - 1; /* game or season */
                zzz = (buffer[10] - '0');     /* for this user or all users */
                if (buffer[7] <= '9')
                    y = (buffer[7] - '0') - 1;
                else
                    for (y = 9, let = 'a'; y < 100; y++, let += 0x01)
                        if (buffer[7] == let)
                            break;

                if (zzz == 1) {
                    get_lifetime_records ();
                    records[0] = lrecords[0];
                    records[1] = lrecords[1];
                }
                if (zzz == 2) {
                    get_server_records ();
                    records[0] = srecords[0];
                    records[1] = srecords[1];
                }

                buffer1[0] = '\0';
                if (zz == 1)
                    for (x = 0; x < 50; x++) {
                        if (!records[z].hitting[y][x].stat[5])
                            break;
                        strcat (&buffer1[0], &records[z].hitting[y][x].name[0]);
                        strcat (&buffer1[0], "??");
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].hitting[y][x].tyear));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].hitting[y][x].teamid));
                        if (!records[z].hitting[y][x].tyear) {
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], &records[z].hitting[y][x].uctname[0]);
                            strcat (&buffer1[0], " ");
                        }
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[0]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[1]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[2]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[3]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[4]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[5]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].hitting[y][x].stat[6]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].hitting[y][x].month));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].hitting[y][x].day));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].hitting[y][x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (3, records[z].hitting[y][x].dis));
                        if (zzz == 2) {
                            strcat (&buffer1[0], &records[z].hitting[y][x].id[0]);
                            strcat (&buffer1[0], "??");
                        }
                    }

                if (zz == 2)
                    for (x = 0; x < 50; x++) {
                        if (!records[z].pitching[y][x].stat[5] && (buffer[7] != 'u' && buffer[7] != 'w'))
                            break;
                        strcat (&buffer1[0], &records[z].pitching[y][x].name[0]);
                        strcat (&buffer1[0], "??");
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].pitching[y][x].tyear));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].pitching[y][x].teamid));
                        if (!records[z].pitching[y][x].tyear) {
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], &records[z].pitching[y][x].uctname[0]);
                            strcat (&buffer1[0], " ");
                        }
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[0]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[1]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[2]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[3]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[4]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[5]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].pitching[y][x].stat[6]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].pitching[y][x].month));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].pitching[y][x].day));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].pitching[y][x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (3, records[z].pitching[y][x].dis));
                        if (zzz == 2) {
                            strcat (&buffer1[0], &records[z].pitching[y][x].id[0]);
                            strcat (&buffer1[0], "??");
                        }
                    }

                if (zz == 3)
                    for (x = 0; x < 50; x++) {
                        if (!records[z].fielding[pos][y][x].stat[5])
                            break;
                        strcat (&buffer1[0], &records[z].fielding[pos][y][x].name[0]);
                        strcat (&buffer1[0], "??");
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].fielding[pos][y][x].tyear));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].fielding[pos][y][x].teamid));
                        if (!records[z].fielding[pos][y][x].tyear) {
                            strcat (&buffer1[0], " ");
                            strcat (&buffer1[0], &records[z].fielding[pos][y][x].uctname[0]);
                            strcat (&buffer1[0], " ");
                        }
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[0]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[1]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[2]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[3]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[4]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[5]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (5, records[z].fielding[pos][y][x].stat[6]));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].fielding[pos][y][x].month));
                        strcat (&buffer1[0], (char *) cnvt_int2str (2, records[z].fielding[pos][y][x].day));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, records[z].fielding[pos][y][x].year));
                        strcat (&buffer1[0], (char *) cnvt_int2str (3, records[z].fielding[pos][y][x].dis));
                        if (zzz == 2) {
                            strcat (&buffer1[0], &records[z].fielding[pos][y][x].id[0]);
                            strcat (&buffer1[0], "??");
                        }
                    }

                if (!strncmp (&buffer1[0], "??", 2))
                    strcpy (&buffer1[0], "-1");
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }

            if (buffer[1] == '6') {
                /* by team */
                strcpy (&parent[0], "/var/NSB");

                if ((fnames = opendir (&parent[0])) == NULL) {
                    strcpy (&buffer1[0], "-2\n");
                    sock_puts (sock, &buffer1[0]);
                    connected = 0;
                    break;
                }
                else
                    closedir (fnames);

                buffer1[0] = '\0';
                if ((strlen (&buffer[0]) == 4 || strlen (&buffer[0]) == 8) && buffer[2] == '9')
                    goto TeamTotals;

                if (strlen (&buffer[0]) == 2 || (strlen (&buffer[0]) == 3 && (buffer[2] == '1' || buffer[2] == '4'))) {
                    strcpy (&parent[0], "/var/NSB/");
                    strcat (&parent[0], &nsbdb[user].id[0]);
                    strcat (&buffer1[0], "U ");
                    if (strlen (&buffer[0]) == 3 && buffer[2] == '1')
                        strcat (&parent[0], "/Lifetime");
                    if (strlen (&buffer[0]) == 3 && buffer[2] == '4')
                        strcat (&parent[0], "/UserTeams");
                }
                else
                    if (strlen (&buffer[0]) < 9 && buffer[2] != '8') {
                        strcpy (&parent[0], "/var/NSB/RealLifeStats");
                        strcat (&buffer1[0], "R ");
                    }

                if (strlen (&buffer[0]) < 9 && buffer[2] != '8') {
                    /* send back what's available */
                    if (strlen (&buffer[0]) == 6) {
                        strcat (&parent[0], "/");
                        strncat (&parent[0], &buffer[2], 4);
                    }
                    if ((fnames = opendir (&parent[0])) != NULL) {
                        char lev1[5000][50], lev2[201][50][50], templev[50];
                        int lev1ptr = 0, lev2ptr = 0, lev2subptr[201], tempx, tempy, tempz;

                        for (tempx = 0; tempx < 200; tempx++)
                            lev2subptr[tempx] = 0;

                        while ((dir = readdir (fnames))) {
                            /* don't process . and .. files */
                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                continue;
                            /* don't process the Results file */
                            if (strstr (dir->d_name, "Results"))
                                continue;
                            if (strstr (dir->d_name, "Schedule"))
                                continue;
                            if (strstr (dir->d_name, "Series"))
                                continue;
                            if (strstr (dir->d_name, "PlayoffResultsAL"))
                                continue;
                            if (strstr (dir->d_name, "PlayoffResultsNL"))
                                continue;
                            if (strstr (dir->d_name, "Standings"))
                                continue;
                            if (!strcmp (dir->d_name, "Records"))
                                continue;
                            if (!strcmp (dir->d_name, "Lifetime"))
                                continue;
                            if (!strcmp (dir->d_name, "UserTeams"))
                                continue;
                            if (!strcmp (dir->d_name, "LeagueSetup"))
                                continue;

                            strcpy (&lev1[lev1ptr][0], dir->d_name);

                            if (buffer[2] == '3') {
                                strcpy (&dummy[0], &parent[0]);
                                strcat (&dummy[0], "/");
                                strcat (&dummy[0], dir->d_name);   /* year directory */

                                if ((fnames2 = opendir (&dummy[0])) != NULL) {
                                    while ((dir2 = readdir (fnames2))) {
                                        if (!strcmp (dir2->d_name, ".") || !strcmp (dir2->d_name, ".."))
                                            continue;
                                        if (strstr (dir2->d_name, "Results") || strstr (dir2->d_name, "Records"))
                                            continue;
                                        strcpy (&lev2[lev2ptr][lev2subptr[lev2ptr]][0], dir2->d_name);
                                        lev2subptr[lev2ptr]++;
                                    }
                                    closedir (fnames2);
                                }
                                lev2ptr++;
                            }
                            lev1ptr++;
                        }
                        closedir (fnames);

                        /* sort */
                        for (tempx = 0; tempx < (lev1ptr - 1); tempx++)
                            for (tempy = tempx + 1; tempy < lev1ptr; tempy++)
                                if (strcmp (&lev1[tempx][0], &lev1[tempy][0]) > 0) {
                                    strcpy (&templev[0], &lev1[tempx][0]);
                                    strcpy (&lev1[tempx][0], &lev1[tempy][0]);
                                    strcpy (&lev1[tempy][0], &templev[0]);
                                    if (lev2ptr) {
                                        for (tempz = 0; tempz < 50; tempz++) {
                                            strcpy (&lev2[200][tempz][0], &lev2[tempx][tempz][0]);
                                            strcpy (&lev2[tempx][tempz][0], &lev2[tempy][tempz][0]);
                                            strcpy (&lev2[tempy][tempz][0], &lev2[200][tempz][0]);
                                        }

                                        lev2subptr[200] = lev2subptr[tempx];
                                        lev2subptr[tempx] = lev2subptr[tempy];
                                        lev2subptr[tempy] = lev2subptr[200];
                                    }
                                }
                        if (lev2ptr)
                            for (tempz = 0; tempz < lev2ptr; tempz++)
                                for (tempx = 0; tempx < (lev2subptr[tempz] - 1); tempx++)
                                    for (tempy = tempx + 1; tempy < lev2subptr[tempz]; tempy++)
                                        if (strcmp (&lev2[tempz][tempx][0], &lev2[tempz][tempy][0]) > 0) {
                                            strcpy (&templev[0], &lev2[tempz][tempx][0]);
                                            strcpy (&lev2[tempz][tempx][0], &lev2[tempz][tempy][0]);
                                            strcpy (&lev2[tempz][tempy][0], &templev[0]);
                                        }
                        /* move sorted items to output buffer */
                        for (tempx = 0; tempx < lev1ptr; tempx++) {
                            strcat (&buffer1[0], &lev1[tempx][0]);
                            strcat (&buffer1[0], " ");

                            if (lev2ptr)
                                for (tempy = 0; tempy < lev2subptr[tempx]; tempy++) {
                                    strcat (&buffer1[0], &lev2[tempx][tempy][0] );
                                    strcat (&buffer1[0], " ");
                                }
                        }
                    }
                    /* let user know what choices are available */
                    strcat (&buffer1[0], "\n");
                    if (sock_puts (sock, &buffer1[0]) < 0)
                        connected = 0;

                    /* get next client request */
                    continue;
                }
TeamTotals:
                if ((strlen (&buffer[0]) == 4 || strlen (&buffer[0]) == 8) && buffer[2] == '9') {
                    /* user wants team totals (current season) */
                    strcpy (&parent[0], "/var/NSB/");
                    if (strlen (&buffer[0]) == 4)
                        strcat (&parent[0], &nsbdb[user].id[0]);
                    else {
                        strcat (&parent[0], "RealLifeStats/");
                        strcat (&parent[0], &buffer[4]);
                    }

                    /* user wants user-created teams */
                    if (buffer[3] == '3')
                        strcat (&parent[0], "/UserTeams");

                    if ((fnames = opendir (&parent[0])) != NULL) {
                        while ((dir = readdir (fnames))) {
                            /* don't process . and .. files */
                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                continue;
                            /* don't process the Results file */
                            if (strstr (dir->d_name, "Results"))
                                continue;
                            if (strstr (dir->d_name, "Schedule"))
                                continue;
                            if (strstr (dir->d_name, "Series"))
                                continue;
                            if (strstr (dir->d_name, "PlayoffResultsAL"))
                                continue;
                            if (strstr (dir->d_name, "PlayoffResultsNL"))
                                continue;
                            if (strstr (dir->d_name, "Standings"))
                                continue;
                            if (!strcmp (dir->d_name, "Records"))
                                continue;
                            if (!strcmp (dir->d_name, "Lifetime"))
                                continue;
                            if (!strcmp (dir->d_name, "UserTeams"))
                                continue;
                            if (!strcmp (dir->d_name, "LeagueSetup"))
                                continue;

                            if (buffer[3] == '1') {
                                /* regular season */
                                if (strstr (dir->d_name, "-PS"))
                                    continue;
                            }
                            else {
                                if (buffer[3] == '2')
                                    /* post season */
                                    if (!strstr (dir->d_name, "-PS"))
                                        continue;
                            }

                            /* got a team ... read data, cum stats and send to client */
                            strcpy (&dummy2[0], &parent[0]);
                            strcat (&dummy2[0], "/");
                            strcat (&dummy2[0], dir->d_name);

                            if ((in = fopen (dummy2, "r")) == NULL) {
                                if (syslog_ent == YES)
                                    syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                                return -1;
                            }

                            fread (&team.id, sizeof team.id, 1, in);
                            fread (&team.year, sizeof team.year, 1, in);
                            fread (&team.league, sizeof team.league, 1, in);
                            fread (&team.division, sizeof team.division, 1, in);
                            for (x = 0; x < 25; x++) {
                                fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                for (y = 0; y < 11; y++)
                                    fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                            }
                            for (x = 0; x < 11; x++) {
                                fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                            }
                            fclose (in);
                            /* determine the number of players this team has */
                            for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                                if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                                    break;
                            /* determine the number of pitchers this team has */
                            for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                                if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                                    break;

                            if (buffer[3] == '3')
                                send_cum_stats (1, dir->d_name);
                            else
                                send_cum_stats (0, NULL);
                        }

                        /* no more teams */
                        sprintf (buffer1, "%d\n", -1);
                        sock_puts (sock, &buffer1[0]);
                    }
                    else {
                        /* no stats */
                        sprintf (buffer1, "%d\n", -1);
                        sock_puts (sock, &buffer1[0]);
                    }
                    continue;
                }

                /* at this point the user wants stats for a specific team */
                if (buffer[2] == '2') {
                    strcpy (&parent[0], "/var/NSB/RealLifeStats/");
                    strncat (&parent[0], &buffer[3], 4);
                    strcat (&parent[0], "/");
                }
                else {
                    strcpy (&parent[0], "/var/NSB/");
                    strcat (&parent[0], &nsbdb[user].id[0]);
                    strcat (&parent[0], "/");
                    if (buffer[2] == '5' || buffer[2] == '7')
                        strcat (&parent[0], "Lifetime/");
                    if (buffer[2] == '8') {
                        strcat (&parent[0], "UserTeams/");
                        strcat (&parent[0], &buffer[3]);
                    }
                    else
                        if (buffer[3] == '1' || buffer[3] == '2') {
                            strncat (&parent[0], &buffer[3], 4);
                            strcat (&parent[0], "\0");
                        }
                        else {
                            strcat (&parent[0], &buffer[3]);
                            goto readinstats;
                        }
                }

                if (buffer[2] != '8') {
                    /* grab team ID */
                    dummy2[0] = buffer[7];
                    dummy2[1] = buffer[8];
                    dummy2[2] = '\0';
                    x = atoi (&dummy2[0]);

                    /* tack on team filename */
                    for (z = 0; z <= NUMBER_OF_TEAMS; z++)
                        if (teaminfo[z].id == x)
                            strcat (&parent[0], &teaminfo[z].filename[0]);

                    if (buffer[2] == '6' || buffer[2] == '7')
                        strcat (&parent[0], "-PS");
                }
readinstats:
                /* read in stats */
                if ((in = fopen (parent, "r")) != NULL) {
                    fread (&team.id, sizeof team.id, 1, in);
                    fread (&team.year, sizeof team.year, 1, in);
                    fread (&team.league, sizeof team.league, 1, in);
                    fread (&team.division, sizeof team.division, 1, in);
                    for (x = 0; x < 25; x++) {
                        fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                        fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                        fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                        for (y = 0; y < 11; y++)
                            fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                    }
                    for (x = 0; x < 11; x++) {
                        fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                        fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                    }
                    fclose (in);
                }
                else {
                    if (syslog_ent == YES)
                        syslog (LOG_ERR, "There is something wrong with %s: %s", parent, strerror (errno));
                    return -1;
                }

                if (syslog_ent == YES) {
                    if (ebb)
                        syslog (LOG_INFO, "Sending stats to %s@%s", &name[0], &hname[0]);
                    else
                        syslog (LOG_INFO, "Sending stats to %s@%s", &nsbdb[user].user[0], &nsbdb[user].site[0]);
                }
                /* send stats to client */
                send_stats (sock, 't');
            }
            if (buffer[1] == '7') {
                /* check for season */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Schedule");
                if ((in = fopen (dummy, "r")) != NULL) {
                    sock_puts (sock, "OK\n");
                    fclose (in);
                }
                else
                    if (strlen (&buffer[0]) == 3 && buffer[2] == '1') {
                        strcat (&dummy[0], "-PS");
                        if ((in = fopen (dummy, "r")) != NULL) {
                            sock_puts (sock, "OK\n");
                            fclose (in);
                        }
                        else
                            sock_puts (sock, "NOLEAGUE\n");
                    }
                    else
                        sock_puts (sock, "NOLEAGUE\n");
            }
            if (buffer[1] == '0') {
                int x, serieslen, wins[2];
                char buf[256], w[256], *pnt;

                /* check for series */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Series");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&buf, sizeof buf, 1, in);
                    fclose (in);

                    strncpy (&w[0], &buf[1], 3);
                    w[3] = '\0';
                    serieslen = atoi (&w[0]);
                    pnt = strrchr (&buf[0], ' ');
                    pnt++;
                    strncpy (&w[0], pnt, 3);
                    w[3] = '\0';
                    wins[0] = atoi (&w[0]);
                    pnt += 3;
                    strncpy (&w[0], pnt, 3);
                    w[3] = '\0';
                    wins[1] = atoi (&w[0]);

                    x = serieslen / 2 + 1;
                    if (wins[0] == x || wins[1] == x)
                        sock_puts (sock, "EOS\n");
                    else
                        sock_puts (sock, "OK\n");
                }
                else
                    sock_puts (sock, "NOSERIES\n");
            }
            if (buffer[1] == 'A') {
                char dummy[256], buf[256];

                /* send back series status */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Series");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&buf, sizeof buf, 1, in);
                    fclose (in);
                }
                else
                    buf[0] = '\0';

                strcpy (&buffer1[0], &buf[0]);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
            }
            if (buffer[1] == '8') {
                /* check for records */
                strcpy (&dummy[0], "/var/NSB/");
                if (buffer[2] == '1') {
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/Lifetime/");
                }
                strcat (&dummy[0], "Records");
                if ((in = fopen (dummy, "r")) != NULL) {
                    sock_puts (sock, "OK\n");
                    fclose (in);
                }
                else
                    sock_puts (sock, "NORECORDS\n");
            }
            if (buffer[1] == '9') {
                /* return all stats matching the supplied name */
                char year[5], name[256], work[1024], *sp, temp[2];
                int yr, dobct, dobm, dobd, doby;
                struct {
                    int m, d, y;
                } tdob[50];

                buffer1[0] = '\0';
                strcpy (&work[0], &buffer[2]);
                /* put name into the format "lastname, firstname" */
                for (x = 0; x < strlen (&work[0]); x++)
                    /* squeeze multiple consecutive spaces into 1 space */
                    if (work[x] == ' ' && work[x + 1] == ' ')
                        for (y = x; y < strlen (&work[0]); y++)
                            work[y] = work [y + 1];
                sp = rindex (&work[0], ' ');  /* the client will ensure the player name contains at least 1 space */
                strcpy (&name[0], sp + 1);
                strcat (&name[0], ", ");
                *sp = '\0';
                strcat (&name[0], &work[0]);
                *sp = ' ';
                strcpy (&buffer1[0], &name[0]);
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);

                strcpy (&parent[0], "/var/NSB/RealLifeStats/");
                dobct = 0;

                /* go through all the teams for all the years twice ... once to see if there is more than one player with the same name, and the
                   second time to get all the stats */
                for (yr = 1901; yr < (MAX_YEAR + 1); yr++) {
                    strcpy (&year[0], (char *) cnvt_int2str (4, yr));
                    strcpy (&dummy[0], &parent[0]);
                    strcat (&dummy[0], &year[0]);
                    if ((fnames = opendir (&dummy[0])) == NULL)
                        /* no year directory */
                        continue;

                    while ((dir = readdir (fnames))) {
                        /* don't process . and .. files */
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        /* don't process the Results file */
                        if (strstr (dir->d_name, "Results"))
                            continue;

                        strcpy (&dummy2[0], &dummy[0]); 
                        strcat (&dummy2[0], "/");
                        strcat (&dummy2[0], dir->d_name);   /* team name */

                        if ((in = fopen (dummy2, "r")) == NULL) {
                            if (syslog_ent == YES)
                                syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                            closedir (fnames);
                            goto NoPMatch;
                        }

                        fread (&team.id, sizeof team.id, 1, in);
                        fread (&team.year, sizeof team.year, 1, in);
                        fread (&team.league, sizeof team.league, 1, in);
                        fread (&team.division, sizeof team.division, 1, in);
                        for (x = 0; x < 25; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 11; x++) {
                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                        }
                        fclose (in);

                        for (x = 0; x < 25; x++)
                            if (!strcasecmp (&name[0], &team.batters[x].id.name[0])) {
                                 for (y = 0; y < dobct; y++)
                                     if (team.batters[x].dob.month == tdob[y].m && team.batters[x].dob.day == tdob[y].d &&
                                                                                     team.batters[x].dob.year == tdob[y].y)
                                         break;
                                 if (y == dobct) {
                                     tdob[dobct].m = team.batters[x].dob.month;
                                     tdob[dobct].d = team.batters[x].dob.day;
                                     tdob[dobct].y = team.batters[x].dob.year;
                                     dobct++;
                                 }
                            }
                    }
                    closedir (fnames);
                }

                if (dobct > 1) {
                    /* more than 1 player with the same exact name */
                    strcpy (&work[0], "MULT ");
                    for (x = 0; x < dobct; x++) {
                        strcat (&work[0], (char *) cnvt_int2str (2, tdob[x].m));
                        strcat (&work[0], (char *) cnvt_int2str (2, tdob[x].d));
                        strcat (&work[0], (char *) cnvt_int2str (4, tdob[x].y));

                        strcat (&work[0], " ");
                    }

                    strcat (&work[0], "\n");
                    sock_puts (sock, &work[0]);

                    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                        connected = 0;
                        continue;
                    }
                    if (buffer[0] == 'X')
                        /* user canceled */
                        continue;

                    work[0] = buffer[0];
                    work[1] = buffer[1];
                    work[2] = '\0';
                    dobm = atoi (&work[0]);

                    work[0] = buffer[2];
                    work[1] = buffer[3];
                    work[2] = '\0';
                    dobd = atoi (&work[0]);

                    work[0] = buffer[4];
                    work[1] = buffer[5];
                    work[2] = buffer[6];
                    work[3] = buffer[7];
                    work[4] = '\0';
                    doby = atoi (&work[0]);
                }
                else {
                    dobm = tdob[0].m;
                    dobd = tdob[0].d;
                    doby = tdob[0].y;
                }

                for (yr = 1901; yr < (MAX_YEAR + 1); yr++) {
                    strcpy (&year[0], (char *) cnvt_int2str (4, yr));
                    strcpy (&dummy[0], &parent[0]);
                    strcat (&dummy[0], &year[0]);
                    if ((fnames = opendir (&dummy[0])) == NULL)
                        /* no year directory */
                        continue;

                    while ((dir = readdir (fnames))) {
                        /* don't process . and .. files */
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        /* don't process the Results file */
                        if (strstr (dir->d_name, "Results"))
                            continue;

                        strcpy (&dummy2[0], &dummy[0]); 
                        strcat (&dummy2[0], "/");
                        strcat (&dummy2[0], dir->d_name);   /* team name */

                        if ((in = fopen (dummy2, "r")) == NULL) {
                            if (syslog_ent == YES)
                                syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                            closedir (fnames);
                            goto NoPMatch;
                        }

                        fread (&team.id, sizeof team.id, 1, in);
                        fread (&team.year, sizeof team.year, 1, in);
                        fread (&team.league, sizeof team.league, 1, in);
                        fread (&team.division, sizeof team.division, 1, in);
                        for (x = 0; x < 25; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 11; x++) {
                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                        }
                        fclose (in);

                        for (x = 0; x < 25; x++) {
                            if (!strcasecmp (&name[0], &team.batters[x].id.name[0]) && team.batters[x].dob.month == dobm &&
                                                        team.batters[x].dob.day == dobd && team.batters[x].dob.year == doby) {
                                strcpy (&work[0], "BSTATS");
                                strcat (&work[0], " ");
                                strcat (&work[0], &year[0]);
                                strcat (&work[0], " ");
                                strcat (&work[0], (char *) cnvt_int2str (2, team.id));
                                strcat (&work[0], " ");
                                temp[1] = '\0';
                                temp[0] = team.batters[x].id.batslr + '0';
                                strcat (&work[0], &temp[0]);
                                strcat (&work[0], " ");
                                temp[0] = team.batters[x].id.throwslr + '0';
                                strcat (&work[0], &temp[0]);
                                strcat (&work[0], (char *) cnvt_int2str (2, team.batters[x].dob.month));
                                strcat (&work[0], (char *) cnvt_int2str (2, team.batters[x].dob.day));
                                strcat (&work[0], (char *) cnvt_int2str (4, team.batters[x].dob.year));
                                strcat (&work[0], "\n");
                                sock_puts (sock, &work[0]);

                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n",
                                         team.batters[x].hitting.games, team.batters[x].hitting.atbats,
                                         team.batters[x].hitting.runs, team.batters[x].hitting.hits,
                                         team.batters[x].hitting.doubles, team.batters[x].hitting.triples);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n", team.batters[x].hitting.homers, team.batters[x].hitting.rbi,
                                         team.batters[x].hitting.bb, team.batters[x].hitting.so, team.batters[x].hitting.hbp, team.batters[x].hitting.gidp);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n",
                                         team.batters[x].hitting.sb, team.batters[x].hitting.cs, team.batters[x].hitting.ibb,
                                         team.batters[x].hitting.sh, team.batters[x].hitting.sf);
                                sock_puts (sock, &work[0]);
                                for (y = 0; y < 11; y++) {
                                    sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n",
                                             team.batters[x].fielding[y].games, team.batters[x].fielding[y].po,
                                             team.batters[x].fielding[y].dp, team.batters[x].fielding[y].a,
                                             team.batters[x].fielding[y].pb, team.batters[x].fielding[y].e);
                                    sock_puts (sock, &work[0]);
                                }
                                break;
                            }
                        }

                        if (x == 25)
                            /* if the player doesn't appear in the batters section then don't look for pitcher data */
                            continue;

                        for (x = 0; x < 11; x++) {
                            if (!strcasecmp (&name[0], &team.pitchers[x].id.name[0])) {
                                strcpy (&work[0], "PSTATS");
                                strcat (&work[0], " ");
                                strcat (&work[0], &year[0]);
                                strcat (&work[0], " ");
                                strcat (&work[0], (char *) cnvt_int2str (2, team.id));
                                strcat (&work[0], " ");
                                temp[1] = '\0';
                                temp[0] = team.pitchers[x].id.batslr + '0';
                                strcat (&work[0], &temp[0]);
                                strcat (&work[0], " ");
                                temp[0] = team.pitchers[x].id.throwslr + '0';
                                strcat (&work[0], &temp[0]);
                                strcat (&work[0], "\n");
                                sock_puts (sock, &work[0]);

                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
                                         team.pitchers[x].pitching.games, team.pitchers[x].pitching.games_started,
                                         team.pitchers[x].pitching.innings, team.pitchers[x].pitching.thirds,
                                         team.pitchers[x].pitching.wins, team.pitchers[x].pitching.losses,
                                         team.pitchers[x].pitching.saves);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n",
                                         team.pitchers[x].pitching.bfp, team.pitchers[x].pitching.hits,
                                         team.pitchers[x].pitching.doubles, team.pitchers[x].pitching.triples,
                                         team.pitchers[x].pitching.homers, team.pitchers[x].pitching.runs);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n",
                                         team.pitchers[x].pitching.er, team.pitchers[x].pitching.rbi, team.pitchers[x].pitching.cg,
                                         team.pitchers[x].pitching.gf, team.pitchers[x].pitching.sho, team.pitchers[x].pitching.svopp);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.sb, team.pitchers[x].pitching.cs,
                                         team.pitchers[x].pitching.walks, team.pitchers[x].pitching.so,
                                          team.pitchers[x].pitching.ibb, team.pitchers[x].pitching.sh);
                                sock_puts (sock, &work[0]);
                                sprintf (work, "%d\n%d\n%d\n%d\n%d\n", team.pitchers[x].pitching.sf, team.pitchers[x].pitching.wp,
                                         team.pitchers[x].pitching.balks, team.pitchers[x].pitching.hb, team.pitchers[x].pitching.opp_ab);
                                sock_puts (sock, &work[0]);
                                break;
                            }
                        }
                    }
                    closedir (fnames);
                }
NoPMatch:
                strcpy (&buffer1[0], "-1\n");
                sock_puts (sock, &buffer1[0]);
            }
        }
        /*
         *
         * end of 'S' request (get stats) from client
         *
        */

        if (buffer[0] == 'N') {
            int x, y, z, holdc, tvi, thi, tvyi, thyi;
            char dummy[256], nxtgame[512], tv[5], th[5], tvyr[5], thyr[5];
            FILE *in;

            /* user wants to know the next game in season */
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Schedule");
            if ((in = fopen (dummy, "r")) != NULL) {
                for (x = 0; x < 244; x++)
                    fgets (&schedule[x][0], 3000, in);
                fclose (in);
            }
            else {
                sock_puts (sock, "NO LEAGUE\n");
                continue;
            }

            if (chk_eos ()) {
                strcat (&dummy[0], "-PS");
                if ((in = fopen (dummy, "r")) != NULL) {
                    for (x = 0; x < 37; x++)
                        fgets (&schedule[x][0], 3000, in);
                    fclose (in);
                }
                else {
                    sock_puts (sock, "NO LEAGUE\n");
                    continue;
                }
                holdc = 36;
            }
            else
                holdc = 243;

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/LeagueSetup");
            if ((in = fopen (dummy, "r")) != NULL) {
                fread (&league_setup, sizeof league_setup, 1, in);
                fclose (in);
            }

            for (x = 0; x < holdc; x++) {
                if (holdc == 243)
                    if (strlen (&schedule[x][0]) < 21)
                        schedule[x][3] = 'X';
                if (schedule[x][3] == 'X')
                    continue;
                for (y = 12; y < strlen (&schedule[x][0]); y += 18) {
                    if (schedule[x][y] != '-')
                        continue;
                    for (z = 0; z < 4; z++) {
                        tv[z] = schedule[x][y - 4 + z];
                        tvyr[z] = schedule[x][y - 8 + z];
                        th[z] = schedule[x][y + 5 + z];
                        thyr[z] = schedule[x][y + 1 + z];
                    }
                    tv[4] = th[4] = tvyr[4] = thyr[4] = '\0';
                    tvi = atoi (&tv[0]);
                    thi = atoi (&th[0]);
                    tvyi = atoi (&tvyr[0]);
                    thyi = atoi (&thyr[0]);
                    break;
                }
                break;
            }
            if (x == holdc)
                sock_puts (sock, "LEAGUE COMPLETED\n");
            else {
                if (tvyi) {
                    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                        if (teaminfo[x].id == tvi) {
                            strcpy (&nxtgame[0], &tvyr[0]);
                            strcat (&nxtgame[0], " ");
                            strcat (&nxtgame[0], &teaminfo[x].filename[0]);
                        }
                }
                else
                    strcpy (&nxtgame[0], GetUCTeamname (tvi));

                strcat (&nxtgame[0], " at the ");

                if (thyi) {
                    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                        if (teaminfo[x].id == thi) {
                            strcat (&nxtgame[0], &thyr[0]);
                            strcat (&nxtgame[0], " ");
                            strcat (&nxtgame[0], &teaminfo[x].filename[0]);
                        }
                }
                else
                    strcat (&nxtgame[0], GetUCTeamname (thi));

                strcat (&nxtgame[0], "\n");
                sock_puts (sock, &nxtgame[0]);
            }
        }
        /*
         *
         * end of 'N' request (next game in season) from client
         *
        */

        if (buffer[0] == 'L') {
            int ucid, numl, numd, numwc, r1g, r2g, r3g, r4g;
            char dummy3[256];

            /* user wants to form a season */
            strcpy (&parent[0], "/var/NSB");

            if ((fnames = opendir (&parent[0])) == NULL) {
                strcpy (&buffer1[0], "-2\n");
                sock_puts (sock, &buffer1[0]);
                connected = 0;
                continue;
            }
            else
                closedir (fnames);

            /* check if user already has a season set up */
            strcpy (&dummy[0], &parent[0]);
            strcat (&dummy[0], "/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            z = 0;
            if ((fnames = opendir (&dummy[0])) != NULL) {
                while ((dir = readdir (fnames)))
                    if (!strcmp (dir->d_name, "Schedule")) {
                        z = 1;
                        break;
                    }
                closedir (fnames);
            }
            if (z) {
                /* let user know he already has a season going */
                sock_puts (sock, "League Already\n");
                if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                    connected = 0;
                    continue;
                }
                if (buffer[0] == 'X')
                    /* user wants to forget about creating a new season */
                    continue;
                /* user wants to start a new season */
                kill_league ();
            }
            /* create the new season */

            /* first delete the old files */
            strcpy (&dummy3[0], &parent[0]);
            strcat (&dummy3[0], "/");
            strcat (&dummy3[0], &nsbdb[user].id[0]);
            if ((fnames = opendir (&dummy3[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Lifetime directory */
                    if (!strcmp (dir->d_name, "Lifetime"))
                        continue;
                    /* don't process the UserTeams directory */
                    if (!strcmp (dir->d_name, "UserTeams"))
                        continue;
                    /* delete everything else */
                    strcpy (&dummyi[0], &dummy3[0]);
                    strcat (&dummyi[0], "/");
                    strcat (&dummyi[0], dir->d_name);

                    unlink (dummyi);
                }
                closedir (fnames);
            }

            strcat (&parent[0], "/RealLifeStats/");

            buffer1[0] = '\0';
            if ((fnames = opendir (&parent[0])) != NULL) {
                int x, y, z, numyears;
                struct {
                    char year[5],
                         teams[101][50];
                    int numteams;
                } years[201];

                for (x = 0; x < 200; x++)
                    years[x].numteams = 0;
                numyears = 0;
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Results file or the Records file */
                    if (strstr (dir->d_name, "Results") || strstr (dir->d_name, "Records"))
                        continue;

                    strcpy (&dummy[0], &parent[0]); 
                    strcat (&dummy[0], dir->d_name);   /* year directory */
                    strcat (&dummy[0], "/");

                    strcpy (&years[numyears].year[0], dir->d_name);

                    if ((fnames2 = opendir (&dummy[0])) != NULL) {
                        while ((dir2 = readdir (fnames2))) {
                            if (!strcmp (dir2->d_name, ".") || !strcmp (dir2->d_name, ".."))
                                continue;
                            if (strstr (dir2->d_name, "Results") || strstr (dir2->d_name, "Records"))
                                continue;
                            strcpy (&years[numyears].teams[years[numyears].numteams][0], dir2->d_name);
                            years[numyears].numteams++;
                        }
                        closedir (fnames2);
                    }
                    numyears++;
                }
                closedir (fnames);

                /* sort teams in each year */
                for (x = 0; x < numyears; x++)
                    for (y = 0; y < (years[x].numteams - 1); y++)
                        for (z = y + 1; z < years[x].numteams; z++)
                            if (strcmp (&years[x].teams[y][0], &years[x].teams[z][0]) > 0) {
                                strcpy (&years[x].teams[100][0], &years[x].teams[y][0]);
                                strcpy (&years[x].teams[y][0], &years[x].teams[z][0]);
                                strcpy (&years[x].teams[z][0], &years[x].teams[100][0]);
                            }

                /* sort years */
                for (x = 0; x < (numyears - 1); x++)
                    for (y = x + 1; y < numyears; y++)
                        if (strcmp (&years[x].year[0], &years[y].year[0]) > 0) {
                            years[200] = years[x];
                            years[x] = years[y];
                            years[y] = years[200];
                        }

                /* move to buffer area for sending to client */
                for (x = 0; x < numyears; x++) {
                    strcat (&buffer1[0], &years[x].year[0]);
                    strcat (&buffer1[0], " ");

                    for (y = 0; y < years[x].numteams; y++) {
                        strcat (&buffer1[0], &years[x].teams[y][0]);
                        strcat (&buffer1[0], " ");
                    }
                }
                /* add user-created teams if there are any */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/UserTeams");
                if ((fnames = opendir (&dummy[0])) != NULL) {
                    int ucbegin = years[numyears].numteams;

                    while ((dir = readdir (fnames)))
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        else {
                            strcpy (&years[numyears].teams[years[numyears].numteams][0], dir->d_name);
                            years[numyears].numteams++;
                        }
                    /* the directory is present but make sure there is at least 1 user-created team */
                    if (!years[numyears].numteams) {
                        closedir (fnames);
                        goto sendteams2client;
                    }
                    strcpy (&years[numyears].year[0], "0000");

                    /* sort teams */
                    for (y = 0; y < (years[numyears].numteams - 1); y++)
                        for (z = y + 1; z < years[numyears].numteams; z++)
                            if (strcmp (&years[numyears].teams[y][0], &years[numyears].teams[z][0]) > 0) {
                                strcpy (&years[numyears].teams[100][0], &years[numyears].teams[y][0]);
                                strcpy (&years[numyears].teams[y][0], &years[numyears].teams[z][0]);
                                strcpy (&years[numyears].teams[z][0], &years[numyears].teams[100][0]);
                            }

                    /* move to buffer area for sending to client */
                    strcat (&buffer1[0], &years[numyears].year[0]);
                    strcat (&buffer1[0], " ");

                    for (y = ucbegin; y < years[numyears].numteams; y++) {
                        strcat (&buffer1[0], &years[numyears].teams[y][0]);
                        strcat (&buffer1[0], " ");
                    }
                    numyears++;
                }
                closedir (fnames);
            }
            else {
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
                continue;
            }
sendteams2client:
            /* let user know what years and teams are available */
            strcat (&buffer1[0], "\n");
            if (sock_puts (sock, &buffer1[0]) < 0) {
                connected = 0;
                continue;
            }
            /* get team selections */
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                connected = 0;
                continue;
            }
            if (buffer[0] == 'X')
                /* user cancelled */
                continue;
            AL.lcnt = AL.ecnt = AL.ccnt = AL.wcnt = NL.lcnt = NL.ecnt = NL.ccnt = NL.wcnt = 0;
            dhcode = buffer[0] - '0';
            numl = buffer[1] - '0';
            numd = buffer[2] - '0';
            numwc = buffer[3] - '0';
            r1g = buffer[4] - '0';
            r2g = buffer[5] - '0';
            r3g = buffer[6] - '0';
            r4g = buffer[7] - '0';

            for (ucid = 900, p = &buffer[8], start1 = &buffer1[0]; p < (&buffer[0] + strlen (&buffer[0])); p++, start1 = index (start1, ' ') + 1) {
                w1 = index (start1, ' ');
                if ((w1 - start1) == 4) {
                    /* hit a year */
                    strncpy (&yearhold[0], start1, 4);
                    yearhold[4] = '\0';
                    p--;        /* to compensate for the p++ in the for loop */
                    continue;   /* get next field in buffer input */
                }
                if (*p != '0') {
                    strcpy (&dummyo[0], "/var/NSB/");
                    strcat (&dummyo[0], &nsbdb[user].id[0]);
                    strcat (&dummyo[0], "/");
                    if (yearhold[0] == '0') {
                        strcpy (&dummyi[0], "/var/NSB/");
                        strcat (&dummyi[0], &nsbdb[user].id[0]);
                        strcat (&dummyi[0], "/UserTeams");
                    }
                    else {
                        strcat (&dummyo[0], &yearhold[0]);
                        strcpy (&dummyi[0], &parent[0]);
                        strcat (&dummyi[0], "/");
                        strcat (&dummyi[0], &yearhold[0]);
                    }
                    strcat (&dummyi[0], "/");
                    nxtbl = index (start1, ' ');
                    *nxtbl = '\0';
                    strcat (&dummyo[0], start1);
                    strcat (&dummyi[0], start1);
                    *nxtbl = ' ';
                    if ((in = fopen (dummyi, "r")) != NULL) {
                        fread (&team.id, sizeof team.id, 1, in);
                        fread (&team.year, sizeof team.year, 1, in);
                        fread (&team.league, sizeof team.league, 1, in);
                        fread (&team.division, sizeof team.division, 1, in);
                        for (x = 0; x < 25; x++) {
                            fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                            fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                            fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                            for (y = 0; y < 11; y++)
                                fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                        }
                        for (x = 0; x < 11; x++) {
                            fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                            fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                        }
                        fclose (in);
                    }
                    clear_stats ();
                    if (*p == '1') {
                        team.league = 'A';
                        team.division = 'E';
                    }
                    if (*p == '2') {
                        team.league = 'A';
                        if (numd == 1)
                            team.division = 'E';
                        else
                            if (numd == 2)
                                team.division = 'W';
                            else
                                team.division = 'C';
                    }
                    if (*p == '3') {
                        team.league = 'A';
                        if (numd == 1)
                            team.division = 'E';
                        else
                            team.division = 'W';
                    }
                    if (*p == '4') {
                        team.league = 'N';
                        team.division = 'E';
                    }
                    if (*p == '5') {
                        team.league = 'N';
                        if (numd == 1)
                            team.division = 'E';
                        else
                            if (numd == 2)
                                team.division = 'W';
                            else
                                team.division = 'C';
                    }
                    if (*p == '6') {
                        team.league = 'N';
                        if (numd == 1)
                            team.division = 'E';
                        else
                            team.division = 'W';
                    }
                    if (!team.id) {
                        /* a temporary ID assigned to user-created teams */
                        team.id = ucid;
                        ucid++;
                    }
                    if ((out = fopen (dummyo, "w")) != NULL) {
                        fwrite (&team.id, sizeof team.id, 1, out);
                        fwrite (&team.year, sizeof team.year, 1, out);
                        fwrite (&team.league, sizeof team.league, 1, out);
                        fwrite (&team.division, sizeof team.division, 1, out);
                        for (xx = 0; xx < 25; xx++) {
                            fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                            fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                            fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                            for (yy = 0; yy < 11; yy++)
                                fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                        }
                        for (xx = 0; xx < 11; xx++) {
                            fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                            fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                        }
                        fclose (out);
                    }
                    if (team.league == 'A') {
                        AL.div[AL.lcnt] = team.division;
                        AL.ids[AL.lcnt] = team.id;
                        AL.years[AL.lcnt] = team.year;
                        /* adding to lcnt must happen after the previous statement */
                        AL.lcnt++;
                        if (team.division == 'E')
                            AL.ecnt++;
                        if (team.division == 'C')
                            AL.ccnt++;
                        if (team.division == 'W')
                            AL.wcnt++;
                    }
                    else {
                        NL.div[NL.lcnt] = team.division;
                        NL.ids[NL.lcnt] = team.id;
                        NL.years[NL.lcnt] = team.year;
                        /* adding to lcnt must happen after the previous statement */
                        NL.lcnt++;
                        if (team.division == 'E')
                            NL.ecnt++;
                        if (team.division == 'C')
                            NL.ccnt++;
                        if (team.division == 'W')
                            NL.wcnt++;
                    }
                }
            }
            /* create league setup file */

            /* ensure user-supplied max games per PS round are correct - make them sane if need be */
            if (numl == 1) {
                if ((numl + numd + numwc) == 2 || (numl + numd + numwc) == 3) {
                    /* 1 round and 1 round only */
                    if (!r1g)
                        r1g = 7;
                    r2g = r3g = r4g = 0;
                }
                if ((numl + numd + numwc) == 4 || (numl + numd + numwc) == 5) {
                    /* 2 rounds and 2 rounds only */
                    if (!r1g)
                        r1g = 7;
                    if (!r2g)
                        r2g = 7;
                    r3g = r4g = 0;
                }
                if ((numl + numd + numwc) == 6) {
                    /* 3 rounds and 3 rounds only */
                    if (!r1g)
                        r1g = 7;
                    if (!r2g)
                        r2g = 7;
                    if (!r3g)
                        r3g = 7;
                    r4g = 0;
                }
            }
            else {
                if ((numl + numd + numwc) == 3) {
                    /* 1 round and 1 round only */
                    if (!r1g)
                        r1g = 7;
                    r2g = r3g = r4g = 0;
                }
                if ((numl + numd + numwc) == 4) {
                    /* 2 rounds and 2 rounds only */
                    if (!r1g)
                        r1g = 7;
                    if (!r2g)
                        r2g = 7;
                    r3g = r4g = 0;
                }
                if ((numl + numd + numwc) == 5 || (numl + numd + numwc) == 6) {
                    /* 3 rounds and 3 rounds only */
                    if (!r1g)
                        r1g = 7;
                    if (!r2g)
                        r2g = 7;
                    if (!r3g)
                        r3g = 7;
                    r4g = 0;
                }
                if ((numl + numd + numwc) == 7) {
                    /* 4 rounds */
                    if (!r1g)
                        r1g = 7;
                    if (!r2g)
                        r2g = 7;
                    if (!r3g)
                        r3g = 7;
                    if (!r4g)
                        r3g = 7;
                }
            }

            /*
               num leagues     1  1  1  1  1  1  1  1  1  2  2  2  2  2  2  2  2  2
               num divisions   1  2  3  1  2  3  1  2  3  1  2  3  1  2  3  1  2  3
               num WCs         0  0  0  1  1  1  2  2  2  0  0  0  1  1  1  2  2  2
               num PS rounds   1  team #1 & team #2 play
                    "             1  team #1 in DIV 1 plays team #1 in DIV 2
                    "                2  team with best record gets bye
                    "                   1  team #1 & WC play
                    "                      2  team with best record gets bye
                    "                         2  team #1 & WC play, team #2 & team #3 play; winners play
                    "                            2  WC teams play; winner plays team #1
                    "                               2  WC #2 plays team with best record, WC #1 plays 2nd team; winners play
                    "                                  3  WC teams play; winner plays #1 team, #2 plays #3; winners play
                    "                                     1  team #1 in league #1 plays team #1 in league #2
                    "                                        rest are the same as cases 2 - 9 plus another round for World Series
            */
            league_setup.numleagues = numl;
            league_setup.numdivs = numd;
            league_setup.numwc = numwc;
            league_setup.nummaxgames[0] = r1g;
            league_setup.nummaxgames[1] = r2g;
            league_setup.nummaxgames[2] = r3g;
            league_setup.nummaxgames[3] = r4g;

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/LeagueSetup");
            if ((out = fopen (dummy, "w")) != NULL) {
                fwrite (&league_setup, sizeof league_setup, 1, out);
                fclose (out);
            }
            else
                closeup ();
figure_schedule:
            /* set up schedule */
            if (AL.lcnt) {
                AL.divegames = 162 / (AL.ecnt - 1);
                if (AL.divegames < 0)
                    AL.divegames = 0;
                AL.divcgames = 162 / (AL.ccnt - 1);
                if (AL.divcgames < 0)
                    AL.divcgames = 0;
                AL.divwgames = 162 / (AL.wcnt - 1);
                if (AL.divwgames < 0)
                    AL.divwgames = 0;
            }
            if (NL.lcnt) {
                NL.divegames = 162 / (NL.ecnt - 1);
                if (NL.divegames < 0)
                    NL.divegames = 0;
                NL.divcgames = 162 / (NL.ccnt - 1);
                if (NL.divcgames < 0)
                    NL.divcgames = 0;
                NL.divwgames = 162 / (NL.wcnt - 1);
                if (NL.divwgames < 0)
                    NL.divwgames = 0;
            }

            for (x = 0; x < 243; x++) {
                strcpy (&schedule[x][0], (char *) cnvt_int2str (3, x + 1));
                strcat (&schedule[x][0], " ");
            }
            schedule[243][0] = dhcode + '0';
            schedule[243][1] = '\0';

            for (x = 0; x < 300; x++) {
                teamwins[x].id = teamwins[x].year = 0;
                for (y = 0; y < 300; y++)
                    teamwins[x].opp[y].id = teamwins[x].opp[y].year = teamwins[x].opp[y].wins = 0;
            }
            for (x = 0; x < AL.lcnt; x++) {
                teamwins[x].league = 'A';
                teamwins[x].div = AL.div[x];
                teamwins[x].id = AL.ids[x];
                teamwins[x].year = AL.years[x];
                for (z = 0; z < AL.lcnt; z++) {
                    teamwins[x].opp[z].id = AL.ids[z];
                    teamwins[x].opp[z].year = AL.years[z];
                }
            }
            for (y = 0; y < NL.lcnt; y++) {
                teamwins[x + y].league = 'N';
                teamwins[x + y].div = NL.div[y];
                teamwins[x + y].id = NL.ids[y];
                teamwins[x + y].year = NL.years[y];
                for (z = 0; z < NL.lcnt; z++) {
                    teamwins[x + y].opp[z].id = NL.ids[z];
                    teamwins[x + y].opp[z].year = NL.years[z];
                }
            }

            do_schedule (AM, 'E');
            do_schedule (AM, 'C');
            do_schedule (AM, 'W');
            do_schedule (NAT, 'E');
            do_schedule (NAT, 'C');
            do_schedule (NAT, 'W');

            for (x = 0; x < 243; x++)
                strcat (&schedule[x][0], "\n");

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Schedule");
            if ((out = fopen (dummy, "w")) != NULL) {
                for (x = 0; x < 244; x++)
                    fwrite (&schedule[x][0], strlen (&schedule[x][0]), 1, out);
                fclose (out);
            }
            else
                closeup ();

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Standings");
            if ((out = fopen (dummy, "w")) != NULL) {
                fwrite (&teamwins[0], sizeof teamwins, 1, out);
                fclose (out);
            }
            else
                closeup ();

            continue;
        }
        /*
         *
         * end of 'L' request (form a season) from client
         *
        */

        if (buffer[0] == 'M') {
            int yr, numl, numd, numt, numwc, pos, yrs[YEAR_SPREAD];
            char dummy3[256], yra[5], work[50], saver, tname[100], *blank;

            /* lazy user wants to form an auto season */
            strcpy (&parent[0], "/var/NSB");

            if ((fnames = opendir (&parent[0])) == NULL) {
                strcpy (&buffer1[0], "-2\n");
                sock_puts (sock, &buffer1[0]);
                connected = 0;
                continue;
            }
            else
                closedir (fnames);

            saver = buffer[1];
            if (saver == 'R') {
                /* random */
                int r1g, r2g, r3g, r4g;

                numl = buffer[2] - '0';
                numd = buffer[3] - '0';
                strncpy (&work[0], &buffer[4], 2);
                work[2] = '\0';
                numt = atoi (&work[0]);
                numwc = buffer[6] - '0';
                dhcode = buffer[7] - '0';
                r1g = buffer[8] - '0';
                r2g = buffer[9] - '0';
                r3g = buffer[10] - '0';
                r4g = buffer[11] - '0';

                /* ensure supplied max games per PS round are correct - make them sane if need be
                   (NOTE - the round-1 value will always be correct) */
                if (numl == 1) {
                    if ((numl + numd + numwc) == 2 || (numl + numd + numwc) == 3)
                        /* 1 round and 1 round only */
                        r2g = r3g = r4g = 0;
                    if ((numl + numd + numwc) == 4 || (numl + numd + numwc) == 5) {
                        /* 2 rounds and 2 rounds only */
                        if (!r2g)
                            r2g = 7;
                        r3g = r4g = 0;
                    }
                    if ((numl + numd + numwc) == 6) {
                        /* 3 rounds and 3 rounds only */
                        if (!r2g)
                            r2g = 7;
                        if (!r3g)
                            r3g = 7;
                        r4g = 0;
                    }
                }
                else {
                    if ((numl + numd + numwc) == 2) {
                        /* 1 round and 1 round only */
                        if (!r1g)
                            r1g = 7;
                        r2g = r3g = r4g = 0;
                    }
                    if ((numl + numd + numwc) == 3 || (numl + numd + numwc) == 4) {
                        /* 2 rounds and 2 rounds only */
                        if (!r2g)
                            r2g = 7;
                        r3g = r4g = 0;
                    }
                    if ((numl + numd + numwc) == 5 || (numl + numd + numwc) == 6) {
                        /* 3 rounds and 3 rounds only */
                        if (!r2g)
                            r2g = 7;
                        if (!r3g)
                            r3g = 7;
                        r4g = 0;
                    }
                    if ((numl + numd + numwc) == 7) {
                        /* 4 rounds */
                        if (!r2g)
                            r2g = 7;
                        if (!r3g)
                            r3g = 7;
                        if (!r4g)
                            r3g = 7;
                    }
                }
                league_setup.nummaxgames[0] = r1g;
                league_setup.nummaxgames[1] = r2g;
                league_setup.nummaxgames[2] = r3g;
                league_setup.nummaxgames[3] = r4g;
            }
            else
                if (saver == 'T') {
                    /* one team over multiple years */
                    int r1g, r2g;

                    numl = numd = 1;
                    blank = strchr (&buffer[0], ' ');
                    *blank = '\0';
                    strcpy (&tname[0], &buffer[2]);
                    *blank = ' ';

                    for (x = 0; x < YEAR_SPREAD; x++)
                        yrs[x] = 0;

                    for (pos = 0; pos < strlen (&buffer[0]); pos++)
                        if (buffer[pos] == ' ')
                            break;
                    pos++;
                    /* get # WC teams */
                    numwc = buffer[pos] - '0';
                    pos++;
                    dhcode = buffer[pos] - '0';
                    pos++;
                    r1g = buffer[pos] - '0';
                    pos++;
                    r2g = buffer[pos] - '0';
                    pos++;
                    /* get years to include in search */
                    for (x = 0; x < YEAR_SPREAD; x++, pos++)
                        if (buffer[pos] == '1')
                            yrs[x] = 1;

                    league_setup.nummaxgames[0] = r1g;
                    if (numwc == 2)
                        if (!r2g)
                            league_setup.nummaxgames[1] = 7;
                        else
                            league_setup.nummaxgames[1] = r2g;
                    else
                        league_setup.nummaxgames[1] = 0;
                    league_setup.nummaxgames[2] = league_setup.nummaxgames[3] = 0;
                }
                else {
                    strncpy (&yra[0], &buffer[1], 4);
                    yra[4] = '\0';
                    yr = atoi (&yra[0]);

                    numl = 2;
                    if (yr < 1969) {
                        numd = 1;
                        numwc = 0;
                        league_setup.nummaxgames[1] = league_setup.nummaxgames[2] = league_setup.nummaxgames[3] = 0;
                        if (yr == 1901 || yr == 1902 || yr == 1904)
                            league_setup.nummaxgames[0] = 0;      /* no WS in these years */
                        else
                            if (yr == 1903 || yr == 1919 || yr == 1920 || yr == 1921)
                                league_setup.nummaxgames[0] = 9;      /* best of 9 WS in these years */
                            else
                                league_setup.nummaxgames[0] = 7;
                    }
                    if (yr > 1968 && yr < 1994) {
                        numd = 2;
                        numwc = 0;
                        league_setup.nummaxgames[1] = 7;
                        league_setup.nummaxgames[2] = league_setup.nummaxgames[3] = 0;
                        if (yr < 1985)
                            league_setup.nummaxgames[0] = 5;
                        else
                            league_setup.nummaxgames[0] = 7;
                    }
                    if (yr > 1993 && yr < 2012) {
                        numd = 3;
                        numwc = 1;
                        if (yr == 1994)
                            /* no post-season */
                            league_setup.nummaxgames[0] = league_setup.nummaxgames[1] = league_setup.nummaxgames[2] = league_setup.nummaxgames[3] = 0;
                        else {
                            league_setup.nummaxgames[0] = 5;
                            league_setup.nummaxgames[1] = league_setup.nummaxgames[2] = 7;
                            league_setup.nummaxgames[3] = 0;
                        }
                    }
                    if (yr > 2011) {
                        numd = 3;
                        numwc = 2;
                        league_setup.nummaxgames[0] = 1;
                        league_setup.nummaxgames[1] = 5;
                        league_setup.nummaxgames[2] = league_setup.nummaxgames[3] = 7;
                    }
                }

            /* create the new season */

            /* first delete the old files */
            strcpy (&dummy3[0], &parent[0]);
            strcat (&dummy3[0], "/");
            strcat (&dummy3[0], &nsbdb[user].id[0]);
            if ((fnames = opendir (&dummy3[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Lifetime directory */
                    if (!strcmp (dir->d_name, "Lifetime"))
                        continue;
                    /* don't process the UserTeams directory */
                    if (!strcmp (dir->d_name, "UserTeams"))
                        continue;
                    /* delete everything else */
                    strcpy (&dummyi[0], &dummy3[0]);
                    strcat (&dummyi[0], "/");
                    strcat (&dummyi[0], dir->d_name);

                    unlink (dummyi);
                }
                closedir (fnames);
            }

            /* create league setup file */
            league_setup.numleagues = numl;
            league_setup.numdivs = numd;
            league_setup.numwc = numwc;

            strcpy (&dummy3[0], &parent[0]);
            strcat (&dummy3[0], "/");
            strcat (&dummy3[0], &nsbdb[user].id[0]);
            strcat (&dummy3[0], "/LeagueSetup");
            if ((out = fopen (dummy3, "w")) != NULL) {
                fwrite (&league_setup, sizeof league_setup, 1, out);
                fclose (out);
            }
            else {
                if (syslog_ent == YES)
                    syslog (LOG_INFO, "couldn't write %s: %s", dummy3, strerror (errno));
                return -1;
            }

            AL.lcnt = AL.ecnt = AL.ccnt = AL.wcnt = NL.lcnt = NL.ecnt = NL.ccnt = NL.wcnt = 0;
            strcat (&parent[0], "/RealLifeStats/");

            if (saver != 'R' && saver != 'T') {
                strcat (&parent[0], &yra[0]);
                strcat (&parent[0], "/");

                if (yr > 1972)
                    dhcode = 1;
                else
                    dhcode = 0;
                buffer1[0] = '\0';

                if ((fnames = opendir (&parent[0])) != NULL) {
                    int x, y;

                    while ((dir = readdir (fnames))) {
                        /* don't process . and .. files */
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        /* don't process the Results file or the Records file */
                        if (strstr (dir->d_name, "Results") || strstr (dir->d_name, "Records"))
                            continue;

                        strcpy (&dummyo[0], "/var/NSB/");
                        strcat (&dummyo[0], &nsbdb[user].id[0]);
                        strcat (&dummyo[0], "/");
                        strcat (&dummyo[0], &yra[0]);

                        strcpy (&dummyi[0], &parent[0]);

                        strcat (&dummyo[0], dir->d_name);
                        strcat (&dummyi[0], dir->d_name);

                        if ((in = fopen (dummyi, "r")) != NULL) {
                            fread (&team.id, sizeof team.id, 1, in);
                            fread (&team.year, sizeof team.year, 1, in);
                            fread (&team.league, sizeof team.league, 1, in);
                            fread (&team.division, sizeof team.division, 1, in);
                            for (x = 0; x < 25; x++) {
                                fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                for (y = 0; y < 11; y++)
                                    fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                            }
                            for (x = 0; x < 11; x++) {
                                fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                            }
                            fclose (in);
                        }

                        /* for real life seasons where there was only one division (and other instances) that field will be blank */
                        if (team.division != 'E' && team.division != 'C' && team.division != 'W')
                            team.division = 'E';
                        clear_stats ();

                        if ((out = fopen (dummyo, "w")) != NULL) {
                            fwrite (&team.id, sizeof team.id, 1, out);
                            fwrite (&team.year, sizeof team.year, 1, out);
                            fwrite (&team.league, sizeof team.league, 1, out);
                            fwrite (&team.division, sizeof team.division, 1, out);
                            for (xx = 0; xx < 25; xx++) {
                                fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                                fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                                fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                                for (yy = 0; yy < 11; yy++)
                                    fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                            }
                            for (xx = 0; xx < 11; xx++) {
                                fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                                fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                            }
                            fclose (out);
                        }
                        if (team.league == 'A') {
                            AL.div[AL.lcnt] = team.division;
                            AL.ids[AL.lcnt] = team.id;
                            AL.years[AL.lcnt] = team.year;
                            /* adding to lcnt must happen after the previous statement */
                            AL.lcnt++;
                            if (team.division == 'E')
                                AL.ecnt++;
                            if (team.division == 'C')
                                AL.ccnt++;
                            if (team.division == 'W')
                                AL.wcnt++;
                        }
                        else {
                            NL.div[NL.lcnt] = team.division;
                            NL.ids[NL.lcnt] = team.id;
                            NL.years[NL.lcnt] = team.year;
                            /* adding to lcnt must happen after the previous statement */
                            NL.lcnt++;
                            if (team.division == 'E')
                                NL.ecnt++;
                            if (team.division == 'C')
                                NL.ccnt++;
                            if (team.division == 'W')
                                NL.wcnt++;
                        }
                    }
                    closedir (fnames);
                }
                else {
                    strcpy (&buffer1[0], "-3\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;
                }
            }
            if (saver == 'R') {
                int x, y, z, cntid, matchut;

                cntid = 900;
                /* select teams ramdomly */
                for (x = 0; x < numl; x++)
                    for (y = 0; y < numd; y++)
                        for (z = 0; z < numt; z++) {
                            strcpy (&parent[0], "/var/NSB/RealLifeStats/");

                            while (1) {
                                matchut = 0;
                                /* we can use this routine to get 2 teams at random but we'll use only 1 team */
                                get2random_teams ();
                                get_rl_home ();

                                /* using the same team more than once in the season screws things up */
                                if (yearh[0] == '0') {
                                    strcpy (&dummy3[0], &parent[0]);
                                    strcat (&dummy3[0], "/");
                                    strcat (&dummy3[0], &nsbdb[user].id[0]);
                                    if ((fnames = opendir (&dummy3[0])) != NULL) {
                                        while ((dir = readdir (fnames))) {
                                            /* don't process . and .. files */
                                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                                continue;
                                            /* don't process the Lifetime directory */
                                            if (!strcmp (dir->d_name, "Lifetime"))
                                                continue;
                                            /* don't process the UserTeams directory */
                                            if (!strcmp (dir->d_name, "UserTeams"))
                                                continue;

                                            if (!strcmp (&home_team[13], dir->d_name)) {
                                                matchut = 1;
                                                break;
                                            }
                                        }
                                        closedir (fnames);
                                    }

                                    if (matchut)
                                        continue;
                                }
                                else {
                                    for (xx = 0; xx < AL.lcnt; xx++)
                                        if (home.id == AL.ids[xx] && home.year == AL.years[xx])
                                            break;
                                    if (xx != AL.lcnt)
                                        continue;

                                    for (xx = 0; xx < NL.lcnt; xx++)
                                        if (home.id == NL.ids[xx] && home.year == NL.years[xx])
                                            break;
                                    if (xx != NL.lcnt)
                                        continue;
                                }

                                break;
                            }

                            strcpy (&dummyo[0], "/var/NSB/");
                            strcat (&dummyo[0], &nsbdb[user].id[0]);
                            strcat (&dummyo[0], "/");
                            if (yearh[0] != '0') {
                                strcat (&dummyo[0], &yearh[0]);
                                strcat (&dummyo[0], &home_team[0]);
                            }
                            else {
                                home.id = cntid;
                                cntid++;
                                strcat (&dummyo[0], &home_team[13]);
                            }

                            if (!x)
                                home.league = 'A';
                            else
                                home.league = 'N';
                            if (!y)
                                home.division = 'E';
                            else
                                if (y == 1)
                                    if (numd == 3)
                                        home.division = 'C';
                                    else
                                        home.division = 'W';
                                else
                                    home.division = 'W';

                            team = home;
                            clear_stats ();

                            if ((out = fopen (dummyo, "w")) != NULL) {
                                fwrite (&team.id, sizeof team.id, 1, out);
                                fwrite (&team.year, sizeof team.year, 1, out);
                                fwrite (&team.league, sizeof team.league, 1, out);
                                fwrite (&team.division, sizeof team.division, 1, out);
                                for (xx = 0; xx < 25; xx++) {
                                    fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                                    fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                                    fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                                    for (yy = 0; yy < 11; yy++)
                                        fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                                }
                                for (xx = 0; xx < 11; xx++) {
                                    fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                                    fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                                }
                                fclose (out);
                            }
                            if (home.league == 'A') {
                                AL.div[AL.lcnt] = home.division;
                                AL.ids[AL.lcnt] = home.id;
                                AL.years[AL.lcnt] = home.year;
                                /* adding to lcnt must happen after the previous statement */
                                AL.lcnt++;
                                if (home.division == 'E')
                                    AL.ecnt++;
                                if (home.division == 'C')
                                    AL.ccnt++;
                                if (home.division == 'W')
                                    AL.wcnt++;
                            }
                            else {
                                NL.div[NL.lcnt] = home.division;
                                NL.ids[NL.lcnt] = home.id;
                                NL.years[NL.lcnt] = home.year;
                                /* adding to lcnt must happen after the previous statement */
                                NL.lcnt++;
                                if (home.division == 'E')
                                    NL.ecnt++;
                                if (home.division == 'C')
                                    NL.ccnt++;
                                if (home.division == 'W')
                                    NL.wcnt++;
                            }
                        }
            }
            if (saver == 'T') {
                /* select one team across multiple years */
                for (yr = 0; yr < YEAR_SPREAD && AL.lcnt < 50; yr++)
                    if (yrs[yr]) {
                        strcpy (&parent[0], "/var/NSB/RealLifeStats/");
                        strcat (&parent[0], (char *) cnvt_int2str (4, (1901 + yr)));
                        strcat (&parent[0], "/");
                        strcat (&parent[0], &tname[0]);

                        if ((in = fopen (parent, "r")) != NULL)
                            fclose (in);
                        else
                            continue;

                        if ((in = fopen (parent, "r")) != NULL) {
                            fread (&team.id, sizeof team.id, 1, in);
                            fread (&team.year, sizeof team.year, 1, in);
                            fread (&team.league, sizeof team.league, 1, in);
                            fread (&team.division, sizeof team.division, 1, in);
                            for (x = 0; x < 25; x++) {
                                fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                                fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                                fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                                for (y = 0; y < 11; y++)
                                    fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                            }
                            for (x = 0; x < 11; x++) {
                                fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                                fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                            }
                            fclose (in);
                        }
                        else {
                            if (syslog_ent == YES)
                                syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                            return -1;
                        }

                        strcpy (&dummyo[0], "/var/NSB/");
                        strcat (&dummyo[0], &nsbdb[user].id[0]);
                        strcat (&dummyo[0], "/");
                        strcat (&dummyo[0], (char *) cnvt_int2str (4, (1901 + yr)));
                        strcat (&dummyo[0], &tname[0]);

                        team.league = 'A';
                        team.division = 'E';

                        clear_stats ();

                        if ((out = fopen (dummyo, "w")) != NULL) {
                            fwrite (&team.id, sizeof team.id, 1, out);
                            fwrite (&team.year, sizeof team.year, 1, out);
                            fwrite (&team.league, sizeof team.league, 1, out);
                            fwrite (&team.division, sizeof team.division, 1, out);
                            for (xx = 0; xx < 25; xx++) {
                                fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                                fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                                fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                                for (yy = 0; yy < 11; yy++)
                                    fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                            }
                            for (xx = 0; xx < 11; xx++) {
                                fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                                fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                            }
                            fclose (out);
                        }
                        AL.div[AL.lcnt] = team.division;
                        AL.ids[AL.lcnt] = team.id;
                        AL.years[AL.lcnt] = team.year;
                        /* adding to lcnt must happen after the previous statement */
                        AL.lcnt++;
                        AL.ecnt++;
                    }
                if (AL.lcnt < 2) {
                    strcpy (&buffer1[0], "-4\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;
                }
            }
            sock_puts (sock, "OK\n");
            goto figure_schedule;
        }
        /*
         *
         * end of 'M' request (form an auto season) from client
         *
        */

        if (buffer[0] == 'U') {
            /* client wants to see a list of the users in the database */
            for (x = 0; x < 100; x++)
                if (nsbdb[x].status) {
                    strcat (&buffer1[0], &nsbdb[x].id[0]);
                    strcat (&buffer1[0], "\t");
                    strcat (&buffer1[0], &nsbdb[x].user[0]);
                    strcat (&buffer1[0], "\t");
                    strcat (&buffer1[0], &nsbdb[x].site[0]);
                    strcat (&buffer1[0], "\t");
                    strcat (&buffer1[0], (char *) cnvt_int2str (6, nsbdb[x].login_ct));
                    strcat (&buffer1[0], "\t");
                }
            strcat (&buffer1[0], "\n");
            if (sock_puts (sock, &buffer1[0]) < 0) {
                connected = 0;
                continue;
            }
        }
        /*
         *
         * end of 'U' request (see users) from client
         *
        */

        if (buffer[0] == 'I') {
            /* client wants an injury report */
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);

            buffer1[0] = '\0';
            if ((fnames = opendir (&dummy[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process certain stuff */
                    if (!strcmp (dir->d_name, "Schedule") || !strcmp (dir->d_name, "Standings") ||
                                             !strcmp (dir->d_name, "Lifetime") || !strcmp (dir->d_name, "Records") ||
                                             !strcmp (dir->d_name, "UserTeams") || !strcmp (dir->d_name, "Series") ||
                                   !strcmp (dir->d_name, "PlayoffResultsAL") || !strcmp (dir->d_name, "PlayoffResultsNL"))
                        continue;
                    /* don't process the LeagueSetup file */
                    if (!strcmp (dir->d_name, "LeagueSetup"))
                        continue;
                    /* take care of the -PS files below */
                    if (strstr (dir->d_name, "-PS"))
                        continue;

                    strcpy (&dummy2[0], &dummy[0]);
                    strcat (&dummy2[0], "/");
                    strcat (&dummy2[0], dir->d_name);
                    strcat (&dummy2[0], "-PS"); /* look for a post-season file first since it contains the latest injury info */

                    if ((in = fopen (dummy2, "r")) == NULL) {
                        strcpy (&dummy2[0], &dummy[0]);
                        strcat (&dummy2[0], "/");
                        strcat (&dummy2[0], dir->d_name);
                        if ((in = fopen (dummy2, "r")) == NULL) {
                            if (syslog_ent == YES)
                                syslog (LOG_ERR, "There is something wrong with %s: %s", dummy2, strerror (errno));
                            return -1;
                        }
                    }

                    fread (&team.id, sizeof team.id, 1, in);
                    fread (&team.year, sizeof team.year, 1, in);
                    fread (&team.league, sizeof team.league, 1, in);
                    fread (&team.division, sizeof team.division, 1, in);
                    for (x = 0; x < 25; x++) {
                        fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                        fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                        fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                        for (y = 0; y < 11; y++)
                            fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                    }
                    for (x = 0; x < 11; x++) {
                        fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                        fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                    }
                    fclose (in);
                    /* determine the number of players this team has */
                    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                        if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                            break;

                    for (x = 0; x < maxplayers[0]; x++)
                        if (team.batters[x].id.injury) {
                            p = index (&team.batters[x].id.name[0], ' ');
                            *p = '\0';
                            strcat (&buffer1[0], &team.batters[x].id.name[0]);
                            *p = ' ';
                            strcat (&buffer1[0], " ");
                            p++;
                            strcat (&buffer1[0], p);
                            strcat (&buffer1[0], " ");

                            strcat (&buffer1[0], (char *) cnvt_int2str (4, team.year));
                            strcat (&buffer1[0], (char *) cnvt_int2str (5, team.id));
                            if (team.id >= 900)
                                /* if user-created team append teamname */
                                strcat (&buffer1[0], dir->d_name);
                            strcat (&buffer1[0], " ");

                            strcat (&buffer1[0], (char *) cnvt_int2str (5, team.batters[x].id.injury));
                            strcat (&buffer1[0], " ");
                        }
                }
                closedir (fnames);
            }

            strcat (&buffer1[0], "\n");
            if (sock_puts (sock, &buffer1[0]) < 0) {
                connected = 0;
                continue;
            }
        }
        /*
         *
         * end of 'I' request (injury report) from client
         *
        */

        if (buffer[0] == 'Z') {
            if (buffer[1] == 'V')
                /* client wants to see who's waiting to play */
                strcpy (&buffer1[0], "V");
            if (buffer[1] == 'A') {
                /* client wants to add his NSBID to the pool */
                strcpy (&buffer1[0], "A");
                strcat (&buffer1[0], &nsbdb[user].id[0]);
            }
            if (buffer[1] == 'R') {
                /* client wants to remove his NSBID to the pool */
                strcpy (&buffer1[0], "R");
                strcat (&buffer1[0], &nsbdb[user].id[0]);
            }
            if (buffer[1] == 'P') {
                /* client wants to challenge an NSBID in the pool to a game */
                strcpy (&buffer1[0], "P");
                strcat (&buffer1[0], &buffer[2]);  /* challengee NSBID */
                strcat (&buffer1[0], " ");
                strcat (&buffer1[0], &nsbdb[user].id[0]);  /* challenger NSBID */
            }
            if (buffer[1] == 'H')
                /* client wants to send nsbpoolmgr further info with regard to adding his ID to the waiting pool */
                strcpy (&buffer1[0], &buffer[2]);  /* client hostname and port number it will watch on */

            strcat (&buffer1[0], "\n");
            if (sock_puts (sockwp, &buffer1[0]) < 0) {
                strcpy (&buffer1[0], "ERROR\n");
                if (sock_puts (sock, &buffer1[0]) < 0) {
                    connected = 0;
                    continue;
                }
            }
            else
                if (buffer[1] != 'H') {
                    if (sock_gets (sockwp, &buffer1[0], sizeof (buffer1)) < 0)
                        strcpy (&buffer1[0], "ERROR");

                    /* pass the info on to the client */
                    strcat (&buffer1[0], "\n");
                    if (sock_puts (sock, &buffer1[0]) < 0) {
                        connected = 0;
                        continue;
                    }
                }
        }
        /*
         *
         * end of 'Z' request (waiting pool transactions) from client
         *
        */

        if (buffer[0] == 'c' || buffer[0] == 'X') {
            if (netgame) {
                sock_puts (netsock, "CANCEL\n");
                close (netsock);
                netgame = 0;
            }
            continue;
        }

        if (buffer[0] == 'k') {
            int games2play, serieslen, wins[2], z;
            char series[500], tyr[2][5], tnm[2][50], tid[2][5], w[100], *pnt, *pnt2;
            /* client wants to play a portion of his series */

            pol = 'l';
            nosend = 1;
            eos_sw = 0;
            vmanager = hmanager = 0; /* the computer will manage both teams */
            strncpy (&w[0], &buffer[1], 3);
            w[3] = '\0';
            games2play = atoi (&w[0]);

            if (games2play > 1)
                strcpy (&gdl[0], "games");
            else
                strcpy (&gdl[0], "game");
            if (syslog_ent == YES)
                syslog (LOG_INFO, "%s@%s playing %d %s in his series", &nsbdb[user].user[0], &nsbdb[user].site[0], games2play, gdl);

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Series");
            if ((in = fopen (dummy, "r")) != NULL) {
                fgets (&series[0], 500, in);
                fclose (in);
            }
            else {
                sock_puts (sock, "CANNOT PLAY\n");
                continue;
            }

            dhcode = series[0] - '0';
            strncpy (&w[0], &series[1], 3);
            w[3] = '\0';
            serieslen = atoi (&w[0]);
            strncpy (&tyr[0][0], &series[4], 4);
            pnt = strchr (&series[4], ' ');
            *pnt = '\0';
            strcpy (&tnm[0][0], &series[8]);
            *pnt = ' ';
            pnt++;
            strncpy (&tyr[1][0], pnt, 4);
            tyr[0][4] = tyr[1][4] = '\0';
            pnt += 4;
            pnt2 = strchr (pnt, ' ');
            *pnt2 = '\0';
            strcpy (&tnm[1][0], pnt);
            *pnt2 = ' ';
            pnt2++;
            strncpy (&w[0], pnt2, 3);
            w[3] = '\0';
            wins[0] = atoi (&w[0]);
            pnt2 += 3;
            strncpy (&w[0], pnt2, 3);
            w[3] = '\0';
            wins[1] = atoi (&w[0]);

            for (z = 0; z < 2; z++)
                if (tyr[z][0] != '0') {
                    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                        if (!strcmp (&tnm[z][0], &teaminfo[x].filename[0]))
                            strncpy (&tid[z][0], (char *) cnvt_int2str (4, teaminfo[x].id), 4);
                }
                else {
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/");
                    strcat (&dummy[0], &tnm[z][0]);
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&team.id, sizeof team.id, 1, in);
                        fclose (in);
                    }
                    strncpy (&tid[z][0], (char *) cnvt_int2str (4, team.id), 4);
                }
            tid[0][4] = tid[1][4] = '\0';

            for (x = 0; x < games2play; x++ ) {
                if (wins[0] == (serieslen / 2 + 1) || wins[1] == (serieslen / 2 + 1)) {
                    sock_puts (sock, "EOS\n");
                    break;
                }

                if (!((wins[0] + wins[1]) % 2)) {
                    strcpy (&teamv[0], &tid[0][0]);
                    strcpy (&teamvyr[0], &tyr[0][0]);
                    strcpy (&teamh[0], &tid[1][0]);
                    strcpy (&teamhyr[0], &tyr[1][0]);
                }
                else {
                    strcpy (&teamv[0], &tid[1][0]);
                    strcpy (&teamvyr[0], &tyr[1][0]);
                    strcpy (&teamh[0], &tid[0][0]);
                    strcpy (&teamhyr[0], &tyr[0][0]);
                }

                teamvi = atoi (&teamv[0]);
                teamviyr = atoi (&teamvyr[0]);
                teamhi = atoi (&teamh[0]);
                teamhiyr = atoi (&teamhyr[0]);

                if (play_league_game () < 0) {
                    connected = 0;
                    sock_puts (sock, "CANNOT PLAY\n");
                    break;
                }
                if (winners == 'v')
                    if (!strcmp (&teamv[0], &tid[0][0]) && !strcmp (&teamvyr[0], &tyr[0][0]))
                        wins[0]++;
                    else
                        wins[1]++;
                else
                    if (!strcmp (&teamh[0], &tid[0][0]) && !strcmp (&teamhyr[0], &tyr[0][0]))
                        wins[0]++;
                    else
                        wins[1]++;

                if (wins[0] == (serieslen / 2 + 1) || wins[1] == (serieslen / 2 + 1)) {
                    sock_puts (sock, "EOS\n");
                    break;
                }
            }
            if (x == games2play)
                sock_puts (sock, "OK\n");

            /* update series status */
            pnt = strrchr (&series[0], ' ');
            pnt++;
            strncpy (pnt, (char *) cnvt_int2str (3, wins[0]), 3);
            pnt += 3;
            strncpy (pnt, (char *) cnvt_int2str (3, wins[1]), 3);

            /* write new series status file */
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Series");
            if ((out = fopen (dummy, "w")) != NULL) {
                fwrite (&series[0], strlen (&series[0]), 1, out);
                fclose (out);
            }
            else
                closeup ();

            nosend = 0;
        }

        if (buffer[0] == 'l') {
            pol = 'l';
            nosend = 1;
            vmanager = hmanager = 0; /* the computer will manage both teams */
            /* client wants to play a portion of his season */
            gdsw = buffer[1];
            for (x = 0, p = &buffer[2]; p < (&buffer[0] + strlen (&buffer[0])); x++, p++)
                gdl[x] = *p;
            gdl[x] = '\0';
            gdi = atoi (&gdl[0]);
            if (gdsw == 'g') {
                if (gdi > 1)
                    strcpy (&gdl[0], "games");
                else
                    strcpy (&gdl[0], "game");
            }
            else {
                if (gdi > 1)
                    strcpy (&gdl[0], "days");
                else
                    strcpy (&gdl[0], "day");
                strcat (&gdl[0], " worth of games");
            }
            if (syslog_ent == YES)
                syslog (LOG_INFO, "%s@%s playing %d %s in his season", &nsbdb[user].user[0], &nsbdb[user].site[0], gdi, gdl);

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Schedule");
            if ((in = fopen (dummy, "r")) != NULL) {
                for (x = 0; x < 244; x++)
                    fgets (&schedule[x][0], 3000, in);
                fclose (in);
                dhcode = schedule[243][0] - '0';
            }
            else {
                sock_puts (sock, "NO LEAGUE\n");
                continue;
            }
            if (chk_eos ()) {
                eos_sw = 1;
                strcat (&dummy[0], "-PS");
                if ((in = fopen (dummy, "r")) != NULL) {
                    for (x = 0; x < 37; x++)
                        fgets (&schedule[x][0], 3000, in);
                    fclose (in);
                    dhcode = schedule[36][0] - '0';
                }
                else {
                    sock_puts (sock, "NO LEAGUE\n");
                    continue;
                }
                holdc = 36;
            }
            else {
                eos_sw = 0;
                holdc = 243;
            }

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/LeagueSetup");
            if ((in = fopen (dummy, "r")) != NULL) {
                fread (&league_setup, sizeof league_setup, 1, in);
                fclose (in);
            }

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Standings");
            if ((in = fopen (dummy, "r")) != NULL) {
                fread (&teamwins[0], sizeof teamwins, 1, in);
                fclose (in);
            }
            else {
                sock_puts (sock, "NO LEAGUE\n");
                continue;
            }
            for (gdc = sday = 0; sday < holdc && gdc < gdi; sday++) {
                if (holdc == 243)
                    if (strlen (&schedule[sday][0]) < 21)
                        schedule[sday][3] = 'X';
                if (schedule[sday][3] == 'X')
                    continue;
                for (sgame = 12; sgame < strlen (&schedule[sday][0]) && gdc < gdi; sgame += 18) {
                    if (schedule[sday][sgame] != '-')
                        continue;
                    for (z = 0; z < 4; z++) {
                        teamv[z] = schedule[sday][sgame - 4 + z];
                        teamvyr[z] = schedule[sday][sgame - 8 + z];
                        teamh[z] = schedule[sday][sgame + 5 + z];
                        teamhyr[z] = schedule[sday][sgame + 1 + z];
                    }
                    teamv[4] = teamh[4] = teamvyr[4] = teamhyr[4] = '\0';
                    teamvi = atoi (&teamv[0]);
                    teamviyr = atoi (&teamvyr[0]);
                    teamhi = atoi (&teamh[0]);
                    teamhiyr = atoi (&teamhyr[0]);
                    if (play_league_game () < 0) {
                        connected = 0;
                        sday = holdc;
                        sock_puts (sock, "CANNOT PLAY\n");
                        goto getoutofhere;
                    }

                    /* update team wins data */
                    if (winners == 'v') {
                        if (visitor.year)
                            win_id = visitor.id;
                        else
                            win_id = visitor_season.id;
                        win_year = visitor.year;
                        if (home.year)
                            lose_id = home.id;
                        else
                            lose_id = home_season.id;
                        lose_year = home.year;
                    }
                    else {
                        if (home.year)
                            win_id = home.id;
                        else
                            win_id = home_season.id;
                        win_year = home.year;
                        if (visitor.year)
                            lose_id = visitor.id;
                        else
                            lose_id = visitor_season.id;
                        lose_year = visitor.year;
                    }
                    if (holdc == 243)
                        for (z = 0; z < 300; z++)
                            if (teamwins[z].id == win_id && teamwins[z].year == win_year)
                                for (zz = 0; zz < 300; zz++)
                                    if (teamwins[z].opp[zz].id == lose_id && teamwins[z].opp[zz].year == lose_year)
                                        teamwins[z].opp[zz].wins++;

                    if (holdc == 243)
                        update_records (sday, teamvi, teamhi, NULL);

                    /* update schedule */
                    if (holdc == 243)
                        schedule[sday][sgame] = 'X';
                    else {
                        if (winners == 'v')
                            schedule[sday][sgame] = 'V';
                        else
                            schedule[sday][sgame] = 'H';
                        update_sch_ps ();
                    }

                    if (gdsw == 'g')
                        gdc++;
                }
                if (sgame >= strlen (&schedule[sday][0]))
                    schedule[sday][3] = 'X';

                if (gdsw == 'd')
                    gdc++;
            }
            if (holdc == 243) {
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Standings");
                if ((out = fopen (dummy, "w")) != NULL) {
                    fwrite (&teamwins[0], sizeof teamwins, 1, out);
                    fclose (out);
                }
                else {
                    sock_puts (sock, "ERROR\n");
                    closeup ();
                }

                put_records ();
            }

            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Schedule");
            if (holdc != 243)
                strcat (&dummy[0], "-PS");

            if ((out = fopen (dummy, "w")) != NULL) {
                for (x = 0; x < (holdc + 1); x++)
                    fwrite (&schedule[x][0], strlen (&schedule[x][0]), 1, out);
                fclose (out);
            }
            else {
                sock_puts (sock, "ERROR\n");
                closeup ();
            }
            if (holdc == 243) {
                if (chk_eos ()) {
                    /* the season is completed */
                    sock_puts (sock, "EOS\n");
                    if (setup_postseason () == -1) {
                        sock_puts (sock, "FUCKED\n");
                        continue;
                    }
                    if (ebb)
                        sock_puts (sock, "OK\n");
                }
                else
                    sock_puts (sock, "OK\n");
            }
            else
                if (chk_eops ()) {
                    /* the post-season is completed */
                    strcpy (&buffer1[0], "EOPS");
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, champsyr));
                    strcat (&buffer1[0], (char *) cnvt_int2str (4, champs));
                    if (champs >= 900)
                        strcat (&buffer1[0], GetUCTeamname (champs));
                    strcat (&buffer1[0], "\n");
                    sock_puts (sock, buffer1);
                    kill_league ();
                }
                else
                    sock_puts (sock, "OK\n");
            sock_puts (sock, "OK\n");
getoutofhere:
            nosend = 0;
        }
        /*
         *
         * end of 'l' request (play portion of season) from client
         *
        */

        eb = eb1 = grand = 0;
        if (buffer[0] == 'w' || buffer[0] == 'W' || buffer[0] == 'h' || buffer[0] == 'H' || buffer[0] == 'p' || buffer[0] == 'P') {
            pol = buffer[0];
            vmanager = hmanager = ebuc = 0;

            if (buffer[0] == 'w' || buffer[0] == 'p' || buffer[0] == 'h') {
                lornl = 'n';

                if (buffer[1] == 'R' || buffer[1] == 'r') {
                    if (buffer[1] == 'R')
                        grand = 1;
                    else
                        grand = 2;
                    dhind = buffer[2] - '0';
                }
            }
            else
                lornl = 'l';
            if (buffer[1] == 'e' && buffer[2] == 'B' && buffer[3] == 'r') {
                eb = 1;
                dhind = buffer[4] - '0';
                if (buffer[5] == '1')
                    ebuc = 1;
            }
            if (buffer[1] == 'e' && buffer[2] == 'B' && buffer[3] == 's') {
                eb1 = 1;
                dhind = buffer[4] - '0';
            }
            /* client wants to play a game */
            if (syslog_ent == YES) {
                if (ebb)
                    syslog (LOG_INFO, "%s@%s playing a game", &name[0], &hname[0]);
                else
                    syslog (LOG_INFO, "%s@%s playing a game", &nsbdb[user].user[0], &nsbdb[user].site[0]);
            }

            if (buffer[0] == 'p' || buffer[0] == 'P') {
                if (sock_gets (sock, &buffer1[0], sizeof (buffer1)) < 0)
                    return -1;
                if (!strcmp (&buffer1[0], "V"))
                    vmanager = 1;
                else
                    hmanager = 1;
            }
            if (buffer[0] == 'h' || buffer[0] == 'H')
                vmanager = hmanager = 1;

            if (lornl == 'l') {
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Schedule");
                if ((in = fopen (dummy, "r")) != NULL) {
                    for (x = 0; x < 244; x++)
                        fgets (&schedule[x][0], 3000, in);
                    fclose (in);
                    dhcode = schedule[243][0] - '0';
                }
                else {
                    sock_puts (sock, "NO LEAGUE\n");
                    continue;
                }
                if (chk_eos ()) {
                    eos_sw = 1;
                    strcat (&dummy[0], "-PS");
                    if ((in = fopen (dummy, "r")) != NULL) {
                        for (x = 0; x < 37; x++)
                            fgets (&schedule[x][0], 3000, in);
                        fclose (in);
                        dhcode = schedule[36][0] - '0';
                    }
                    else {
                        sock_puts (sock, "NO LEAGUE\n");
                        continue;
                    }
                    holdc = 36;
                }
                else {
                    eos_sw = 0;
                    holdc = 243;
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/LeagueSetup");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&league_setup, sizeof league_setup, 1, in);
                    fclose (in);
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Standings");
                if ((in = fopen (dummy, "r")) != NULL) {
                    fread (&teamwins[0], sizeof teamwins, 1, in);
                    fclose (in);
                }
                else {
                    sock_puts (sock, "NO LEAGUE\n");
                    continue;
                }

                for (x = 0; x < holdc; x++) {
                    if (holdc == 243)
                        if (strlen (&schedule[x][0]) < 21)
                            schedule[x][3] = 'X';
                    if (schedule[x][3] == 'X')
                        continue;
                    for (y = 12; y < strlen (&schedule[x][0]); y += 18) {
                        if (schedule[x][y] != '-')
                            continue;
                        for (z = 0; z < 4; z++) {
                            teamv[z] = schedule[x][y - 4 + z];
                            teamvyr[z] = schedule[x][y - 8 + z];
                            teamh[z] = schedule[x][y + 5 + z];
                            teamhyr[z] = schedule[x][y + 1 + z];
                        }
                        teamv[4] = teamh[4] = teamvyr[4] = teamhyr[4] = '\0';
                        teamvi = atoi (&teamv[0]);
                        teamviyr = atoi (&teamvyr[0]);
                        teamhi = atoi (&teamh[0]);
                        teamhiyr = atoi (&teamhyr[0]);
                        if (play_league_game () == -1) {
                            connected = 0;
                            x = holdc;
                            break;
                        }
 
                        /* update team wins data */
                        if (winners == 'v') {
                            if (visitor.year)
                                win_id = visitor.id;
                            else
                                win_id = visitor_season.id;
                            win_year = visitor.year;
                            if (home.year)
                                lose_id = home.id;
                            else
                                lose_id = home_season.id;
                            lose_year = home.year;
                        }
                        else {
                            if (home.year)
                                win_id = home.id;
                            else
                                win_id = home_season.id;
                            win_year = home.year;
                            if (visitor.year)
                                lose_id = visitor.id;
                            else
                                lose_id = visitor_season.id;
                            lose_year = visitor.year;
                        }
                        if (holdc == 243)
                            for (z = 0; z < 300; z++)
                                if (teamwins[z].id == win_id && teamwins[z].year == win_year)
                                    for (zz = 0; zz < 300; zz++)
                                        if (teamwins[z].opp[zz].id == lose_id && teamwins[z].opp[zz].year == lose_year)
                                            teamwins[z].opp[zz].wins++;

                        if (holdc == 243)
                            update_records (x, teamvi, teamhi, NULL);
                        if (holdc == 243)
                            schedule[x][y] = 'X';
                        else {
                            if (winners == 'v')
                                schedule[x][y] = 'V';
                            else
                                schedule[x][y] = 'H';
                            update_sch_ps ();
                        }
                        break;
                    }
                    if ((y + 18) >= strlen (&schedule[x][0]))
                        schedule[x][3] = 'X';
                    break;
                }
                if (holdc == 243) {
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/Standings");
                    if ((out = fopen (dummy, "w")) != NULL) {
                        fwrite (&teamwins[0], sizeof teamwins, 1, out);
                        fclose (out);
                    }
                    else
                        closeup ();

                    put_records ();
                }

                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/Schedule");
                if (holdc != 243)
                    strcat (&dummy[0], "-PS");

                if ((out = fopen (dummy, "w")) != NULL) {
                    for (x = 0; x < (holdc + 1); x++)
                        fwrite (&schedule[x][0], strlen (&schedule[x][0]), 1, out);
                    fclose (out);
                }
                else {
                    sock_puts (sock, "ERROR\n");
                    closeup ();
                }
                if (holdc == 243) {
                    if (chk_eos ()) {
                        /* the season has been completed */
                        sock_puts (sock, "EOS\n");
                        if (setup_postseason () == -1) {
                            sock_puts (sock, "FUCKED\n");
                            continue;
                        }
                        if (ebb)
                            sock_puts (sock, "OK\n");
                    }
                    else
                        sock_puts (sock, "OK\n");
                }
                else
                    if (chk_eops ()) {
                        /* the post-season is completed */
                        strcpy (&buffer1[0], "EOPS");
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, champsyr));
                        strcat (&buffer1[0], (char *) cnvt_int2str (4, champs));
                        if (champs >= 900)
                            strcat (&buffer1[0], GetUCTeamname (champs));
                        strcat (&buffer1[0], "\n");
                        sock_puts (sock, buffer1);
                        kill_league ();
                    }
                    else
                        sock_puts (sock, "OK\n");
                sock_puts (sock, "OK\n");
            }
            else {
                z = 0;
                buffer1[0] = '\0';

                strcpy (&parent[0], "/var/NSB/RealLifeStats");

                if (eb || grand)
                    goto SelectTeams;

                /* find all the teams */
                if ((fnames = opendir (&parent[0])) != NULL) {
                    int x, y, z, numyears;
                    struct {
                        char year[5],
                             teams[501][50];
                        int numteams;
                    } years[201];

                    for (x = 0; x < 200; x++)
                        years[x].numteams = 0;
                    numyears = 0;
                    while ((dir = readdir (fnames))) {
                        /* don't process . and .. files */
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        /* don't process the Results file */
                        if (strstr (dir->d_name, "Results"))
                            continue;
                        if (!strcmp (dir->d_name, "Schedule"))
                            continue;
                        if (!strcmp (dir->d_name, "Series"))
                            continue;
                        if (!strcmp (dir->d_name, "PlayoffResultsAL"))
                            continue;
                        if (!strcmp (dir->d_name, "PlayoffResultsNL"))
                            continue;
                        /* don't process post-season stats */
                        if (strstr (dir->d_name, "-PS"))
                            continue;
                        if (!strcmp (dir->d_name, "Standings"))
                            continue;
                        if (!strcmp (dir->d_name, "Records"))
                            continue;
                        if (!strcmp (dir->d_name, "Lifetime"))
                            continue;
                        if (!strcmp (dir->d_name, "UserTeams"))
                            continue;
                        /* don't process the LeagueSetup file */
                        if (!strcmp (dir->d_name, "LeagueSetup"))
                            continue;

                        strcpy (&years[numyears].year[0], dir->d_name);

                        strcpy (&dummy[0], &parent[0]);
                        strcat (&dummy[0], "/");
                        strcat (&dummy[0], dir->d_name);   /* year directory */

                        if ((fnames2 = opendir (&dummy[0])) != NULL) {
                            while ((dir2 = readdir (fnames2))) {
                                if (!strcmp (dir2->d_name, ".") || !strcmp (dir2->d_name, ".."))
                                    continue;
                                if (strstr (dir2->d_name, "Results") || strstr (dir2->d_name, "Records"))
                                    continue;
                                strcpy (&years[numyears].teams[years[numyears].numteams][0], dir2->d_name);
                                years[numyears].numteams++;
                            }
                            closedir (fnames2);
                        }
                        numyears++;
                    }
                    closedir (fnames);

                    /* sort teams in each year */
                    for (x = 0; x < numyears; x++)
                        for (y = 0; y < (years[x].numteams - 1); y++)
                            for (z = y + 1; z < years[x].numteams; z++)
                                if (strcmp (&years[x].teams[y][0], &years[x].teams[z][0]) > 0) {
                                    strcpy (&years[x].teams[500][0], &years[x].teams[y][0]);
                                    strcpy (&years[x].teams[y][0], &years[x].teams[z][0]);
                                    strcpy (&years[x].teams[z][0], &years[x].teams[500][0]);
                                }

                    /* sort years */
                    for (x = 0; x < (numyears - 1); x++)
                        for (y = x + 1; y < numyears; y++)
                            if (strcmp (&years[x].year[0], &years[y].year[0]) > 0) {
                                years[200] = years[x];
                                years[x] = years[y];
                                years[y] = years[200];
                            }

                    /* move to buffer area for sending to client */
                    for (x = 0; x < numyears; x++) {
                        strcat (&buffer1[0], &years[x].year[0]);
                        strcat (&buffer1[0], " ");
 
                        for (y = 0; y < years[x].numteams; y++) {
                            strcat (&buffer1[0], &years[x].teams[y][0]);
                            strcat (&buffer1[0], " ");
                        }
                    }
                    /* add user-created teams if there are any */
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/UserTeams");
                    if ((fnames = opendir (&dummy[0])) != NULL) {
                        while ((dir = readdir (fnames)))
                            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                                continue;
                            else {
                                strcpy (&years[numyears].teams[years[numyears].numteams][0], dir->d_name);
                                years[numyears].numteams++;
                            }
                        /* the directory is present but make sure there is at least 1 user-created team */
                        if (!years[numyears].numteams) {
                            closedir (fnames);
                            goto sendteams2client2;
                        }
                        strcpy (&years[numyears].year[0], "0000");

                        /* move to buffer area for sending to client */
                        strcat (&buffer1[0], &years[numyears].year[0]);
                        strcat (&buffer1[0], " ");

                        for (y = 0; y < years[numyears].numteams; y++) {
                            strcat (&buffer1[0], &years[numyears].teams[y][0]);
                            strcat (&buffer1[0], " ");
                        }
                        numyears++;
                    }
                    closedir (fnames);
                }
                else {
                    strcat (&buffer1[0], "\n");
                    sock_puts (sock, &buffer1[0]);
                    continue;
                }
sendteams2client2:
                /* let user know what choices are available */
                strcat (&buffer1[0], "\n");
                if (sock_puts (sock, &buffer1[0]) < 0) {
                    connected = 0;
                    continue;
                }
                if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                    return -1;
                if (!strcmp (&buffer[0], "X")) {
                    if (netgame) {
                        sock_puts (netsock, "CANCEL\n");
                        close (netsock);
                        netgame = 0;
                    }
                    continue;
                }
SelectTeams:
                /* get the two teams to play and let user know */
                if (eb || grand)
                    get2random_teams ();
                else
                    get_teams ();
                buffer1[0] = '\0';
                if (yearv[0] != '0')
                    strcpy (&buffer1[0], &yearv[0]);
                strcat (&buffer1[0], &visiting_team[0]);
                strcat (&buffer1[0], " ");
                if (yearh[0] != '0')
                    strcat (&buffer1[0], &yearh[0]);
                strcat (&buffer1[0], &home_team[0]);

                strcat (&buffer1[0], "\n");
                if (sock_puts (sock, &buffer1[0]) < 0) {
                    connected = 0;
                    continue;
                }
                if (netgame) {
                    if (sock_puts (netsock, &buffer1[0]) < 0) {
                        connected = 0;
                        continue;
                    }
                }

                /* get real life stats for visiting team */
                if (get_rl_vis ()) {
                    connected = 0;
                    continue;
                }
                zero_visitor_season ();

                /* get real life stats for home team */
                if (get_rl_home ()) {
                    connected = 0;
                    continue;
                }
                zero_home_season ();

                /* clear arrays for starters, batting order, and pitchers (set outside of range, 0 is valid) */
                for (x = 0; x < 2; x++)
                    for (y = 0; y < 10; y++)
                        starters[x][y] = 99;
                for (x = 0; x < 2; x++)
                    for (y = 0; y < 9; y++)
                        for (z = 0; z < 30; z++)
                            border[x][y].player[z] = border[x][y].pos[z] = 99;
                for (x = 0; x < 2; x++)
                    for (y = 0; y < 15; y++) {
                        pitching[x].pitcher[y] = 99;
                        pitching[x].innings[y] = pitching[x].thirds[y] = 0;
                    }

                /* set up for current game stats */
                setup_cgs ();

                if (hmanager) {
                    send_stats (sock, 'a');
                    send_stats (sock, 'c');
                    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                        closeup ();
                    if (!strncmp (&buffer[0], "FORGET IT", 9)) {
                        determine_starters ('h');
                        determine_battingorder ('h');
                    }
                    else {
                        team = home;
                        for (y = x = 0; x < (9 + dhind); x++) {
                            z = 0;
                            while (buffer[y] != ' ')
                                dummy2[z++] = buffer[y++];
                            y++;
                            dummy2[z] = '\0';
                            if (x != 9)
                                border[1][x].player[0] = atoi (&dummy2[0]);
                            else
                                pdh = atoi (&dummy2[0]);

                            z = 0;
                            while (buffer[y] != ' ')
                            dummy2[z++] = buffer[y++];
                            y++;
                            dummy2[z] = '\0';
                            if (x != 9)
                                border[1][x].pos[0] = atoi (&dummy2[0]);

                            if (!dhind) {
                                if (border[1][x].pos[0] == 1) {
                                    starters[1][1] = border[1][x].player[0];
                                    for (z = 0; z < 11; z++)
                                        if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[border[1][x].player[0]].id.name[0])) {
                                            game_status.pitcher[1] = pitching[1].pitcher[0] = z;
                                            break;
                                        }
                                }
                            }
                            else
                                if (x == 9) {
                                    starters[1][1] = pdh;
                                    for (z = 0; z < 11; z++)
                                        if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[pdh].id.name[0])) {
                                            game_status.pitcher[1] = pitching[1].pitcher[0] = z;
                                            break;
                                        }
                                }
                        }
                        for (x = 0; x < 10; x++)
                            for (y = 0; y < 9; y++)
                                if (border[1][y].pos[0] == x)
                                    starters[1][x] = border[1][y].player[0];

                        gotateam = 1;
                    }
                }
                else {
                    determine_starters ('h');
                    determine_battingorder ('h');
                }

                if (vmanager) {
                    if (netgame) {
                        buffer1[0] = dhind + '0';
                        buffer1[1] = '\n';
                        buffer1[2] = '\0';
                        sock_puts (netsock, &buffer1[0]);
                        send_stats (netsock, 'b');
                        send_stats (netsock, 'd');
                    }
                    else {
                        send_stats (sock, 'b');
                        send_stats (sock, 'd');
                    }
                    if (netgame) {
                        if (sock_gets (netsock, &buffer[0], sizeof (buffer)) < 0)
                            closeup ();
                    }
                    else
                        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                            closeup ();
                    if (!strncmp (&buffer[0], "FORGET IT", 9)) {
                        determine_starters ('v');
                        determine_battingorder ('v');
                    }
                    else {
                        team = visitor;
                        for (y = x = 0; x < (9 + dhind); x++) {
                            z = 0;
                            while (buffer[y] != ' ')
                                dummy2[z++] = buffer[y++];
                            y++;
                            dummy2[z] = '\0';
                            if (x != 9)
                                border[0][x].player[0] = atoi (&dummy2[0]);
                            else
                                pdh = atoi (&dummy2[0]);

                            z = 0;
                            while (buffer[y] != ' ')
                                dummy2[z++] = buffer[y++];
                            y++;
                            dummy2[z] = '\0';
                            if (x != 9)
                                border[0][x].pos[0] = atoi (&dummy2[0]);

                            if (!dhind) {
                                if (border[0][x].pos[0] == 1) {
                                    starters[0][1] = border[0][x].player[0];
                                    for (z = 0; z < 11; z++)
                                        if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[border[0][x].player[0]].id.name[0])) {
                                            game_status.pitcher[0] = pitching[0].pitcher[0] = z;
                                            break;
                                        }
                                }
                            }
                            else
                                if (x == 9) {
                                    starters[0][1] = pdh;
                                    for (z = 0; z < 11; z++)
                                        if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[pdh].id.name[0])) {
                                            game_status.pitcher[0] = pitching[0].pitcher[0] = z;
                                            break;
                                        }
                                }
                        }
                        for (x = 0; x < 10; x++)
                            for (y = 0; y < 9; y++)
                                if (border[0][y].pos[0] == x)
                                    starters[0][x] = border[0][y].player[0];
 
                        gotateam = 1;
                    }
                }
                else {
                    determine_starters ('v');
                    determine_battingorder ('v');
                }
                send_lineup ();
                send_DOB ();
                playthegame ();
                netgame = 0;

                /* wait to continue for eBaseball */
                if (eb || eb1)
                    sleep (1);
            }
        }
        /*
         *
         * end of 'W', 'w', 'H', 'h', 'P' and 'p' request (play a game) from client
         *
        */

        if (buffer[0] == 'Q') {
            int serieslen;
            char w[1000], *pnt, dummy3[256];

            /* user wants to establish a series between two teams */
            buffer1[0] = '\0';

            strcpy (&parent[0], "/var/NSB/RealLifeStats");

            /* find all the teams */
            if ((fnames = opendir (&parent[0])) != NULL) {
                int x, y, z, numyears;
                struct {
                    char year[5],
                         teams[101][50];
                    int numteams;
                } years[201];

                for (x = 0; x < 200; x++)
                    years[x].numteams = 0;
                numyears = 0;
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Results file */
                    if (strstr (dir->d_name, "Results"))
                        continue;
                    if (!strcmp (dir->d_name, "Schedule"))
                        continue;
                    if (!strcmp (dir->d_name, "Series"))
                        continue;
                    if (!strcmp (dir->d_name, "PlayoffResultsAL"))
                        continue;
                    if (!strcmp (dir->d_name, "PlayoffResultsNL"))
                        continue;
                    /* don't process post-season stats */
                    if (strstr (dir->d_name, "-PS"))
                        continue;
                    if (!strcmp (dir->d_name, "Standings"))
                        continue;
                    if (!strcmp (dir->d_name, "Records"))
                        continue;
                    if (!strcmp (dir->d_name, "Lifetime"))
                        continue;
                    if (!strcmp (dir->d_name, "UserTeams"))
                        continue;
                    /* don't process the LeagueSetup file */
                    if (!strcmp (dir->d_name, "LeagueSetup"))
                        continue;

                    strcpy (&years[numyears].year[0], dir->d_name);

                    strcpy (&dummy[0], &parent[0]);
                    strcat (&dummy[0], "/");
                    strcat (&dummy[0], dir->d_name);   /* year directory */

                    if ((fnames2 = opendir (&dummy[0])) != NULL) {
                        while ((dir2 = readdir (fnames2))) {
                            if (!strcmp (dir2->d_name, ".") || !strcmp (dir2->d_name, ".."))
                                continue;
                            if (strstr (dir2->d_name, "Results") || strstr (dir2->d_name, "Records"))
                                continue;
                            strcpy (&years[numyears].teams[years[numyears].numteams][0], dir2->d_name);
                            years[numyears].numteams++;
                        }
                        closedir (fnames2);
                    }
                    numyears++;
                }
                closedir (fnames);

                /* sort teams in each year */
                for (x = 0; x < numyears; x++)
                    for (y = 0; y < (years[x].numteams - 1); y++)
                        for (z = y + 1; z < years[x].numteams; z++)
                            if (strcmp (&years[x].teams[y][0], &years[x].teams[z][0]) > 0) {
                                strcpy (&years[x].teams[100][0], &years[x].teams[y][0]);
                                strcpy (&years[x].teams[y][0], &years[x].teams[z][0]);
                                strcpy (&years[x].teams[z][0], &years[x].teams[100][0]);
                            }

                /* sort years */
                for (x = 0; x < (numyears - 1); x++)
                    for (y = x + 1; y < numyears; y++)
                        if (strcmp (&years[x].year[0], &years[y].year[0]) > 0) {
                            years[200] = years[x];
                            years[x] = years[y];
                            years[y] = years[200];
                        }

                /* move to buffer area for sending to client */
                for (x = 0; x < numyears; x++) {
                    strcat (&buffer1[0], &years[x].year[0]);
                    strcat (&buffer1[0], " ");

                    for (y = 0; y < years[x].numteams; y++) {
                        strcat (&buffer1[0], &years[x].teams[y][0]);
                        strcat (&buffer1[0], " ");
                    }
                }
                /* add user-created teams if there are any */
                strcpy (&dummy[0], "/var/NSB/");
                strcat (&dummy[0], &nsbdb[user].id[0]);
                strcat (&dummy[0], "/UserTeams");
                if ((fnames = opendir (&dummy[0])) != NULL) {
                    while ((dir = readdir (fnames)))
                        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                            continue;
                        else {
                            strcpy (&years[numyears].teams[years[numyears].numteams][0], dir->d_name);
                            years[numyears].numteams++;
                        }
                    /* the directory is present but make sure there is at least 1 user-created team */
                    if (!years[numyears].numteams) {
                        closedir (fnames);
                        goto sendteams2client3;
                    }
                    strcpy (&years[numyears].year[0], "0000");

                    /* sort teams */
                    for (y = 0; y < (years[numyears].numteams - 1); y++)
                        for (z = y + 1; z < years[numyears].numteams; z++)
                            if (strcmp (&years[numyears].teams[y][0], &years[numyears].teams[z][0]) > 0) {
                                strcpy (&years[numyears].teams[100][0], &years[numyears].teams[y][0]);
                                strcpy (&years[numyears].teams[y][0], &years[numyears].teams[z][0]);
                                strcpy (&years[numyears].teams[z][0], &years[numyears].teams[100][0]);
                            }

                    /* move to buffer area for sending to client */
                    strcat (&buffer1[0], &years[numyears].year[0]);
                    strcat (&buffer1[0], " ");

                    for (y = 0; y < years[numyears].numteams; y++) {
                        strcat (&buffer1[0], &years[numyears].teams[y][0]);
                        strcat (&buffer1[0], " ");
                    }
                    numyears++;
                }
                closedir (fnames);
            }
            else {
                strcat (&buffer1[0], "\n");
                sock_puts (sock, &buffer1[0]);
                continue;
            }
sendteams2client3:
            /* let user know what choices are available */
            strcat (&buffer1[0], "\n");
            if (sock_puts (sock, &buffer1[0]) < 0) {
                connected = 0;
                continue;
            }
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                return -1;
            if (!strcmp (&buffer[0], "X"))
                continue;

            /* first delete any old files */
            strcpy (&dummy3[0], "/var/NSB/");
            strcat (&dummy3[0], &nsbdb[user].id[0]);
            if ((fnames = opendir (&dummy3[0])) != NULL) {
                while ((dir = readdir (fnames))) {
                    /* don't process . and .. files */
                    if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                        continue;
                    /* don't process the Lifetime directory */
                    if (!strcmp (dir->d_name, "Lifetime"))
                        continue;
                    /* don't process the UserTeams directory */
                    if (!strcmp (dir->d_name, "UserTeams"))
                        continue;
                    /* delete everything else */
                    strcpy (&dummyi[0], &dummy3[0]);
                    strcat (&dummyi[0], "/");
                    strcat (&dummyi[0], dir->d_name);

                    unlink (dummyi);
                }
                closedir (fnames);
            }

            w[0] = buffer[0];
            if (buffer[1] == 'R') {
                serieslen = (int) ((float) 999 * rand () / (RAND_MAX + 1.0)) + 1;
                /* need an odd number */
                if (!(serieslen % 2))
                    serieslen--;
                strcpy (&w[1], (char *) cnvt_int2str (3, serieslen));
            }
            else
                strncpy (&w[1], &buffer[1], 3);
            if (buffer[5] == 'R') {
                get2random_teams ();
                strcpy (&w[4], &yearv[0]);
                strcat (&w[0], &visiting_team[0]);
                strcat (&w[0], " ");
                strcat (&w[0], &yearh[0]);
                strcat (&w[0], &home_team[0]);
            }
            else {
                strcpy (&w[4], &buffer[5]);
                strncpy (&yearv[0], &buffer[5], 4);
                pnt = strchr (&buffer[5], ' ');
                *pnt = '\0';
                if (yearv[0] == '0') {
                    strcpy (&visiting_team[0], "User-Created:");
                    strcat (&visiting_team[0], &buffer[9]);
                }
                else
                    strcpy (&visiting_team[0], &buffer[9]);
                *pnt = ' ';
                pnt++;
                strncpy (&yearh[0], pnt, 4);
                yearv[4] = yearh[4] = '\0';
                pnt += 4;
                if (yearh[0] == '0') {
                    strcpy (&home_team[0], "User-Created:");
                    strcat (&home_team[0], pnt);
                }
                else
                    strcpy (&home_team[0], pnt);
            }
            strcat (&w[0], " 000000");

            /* write series setup */
            strcpy (&dummy[0], "/var/NSB/");
            strcat (&dummy[0], &nsbdb[user].id[0]);
            strcat (&dummy[0], "/Series");
            if ((out = fopen (dummy, "w")) != NULL) {
                fwrite (&w[0], strlen (&w[0]), 1, out);
                fclose (out);
            }
            else
                closeup ();

            /* get real life stats for team #1 */
            if (get_rl_vis ()) {
                connected = 0;
                continue;
            }
            team = visitor;
            if (!team.id)
                /* a temporary ID assigned to user-created teams */
                team.id = 900;
            clear_stats ();
            strcpy (&dummyo[0], "/var/NSB/");
            strcat (&dummyo[0], &nsbdb[user].id[0]);
            strcat (&dummyo[0], "/");
            if (yearv[0] != '0') {
                strcat (&dummyo[0], &yearv[0]);
                strcat (&dummyo[0], &visiting_team[0]);
            }
            else
                strcat (&dummyo[0], &visiting_team[13]);

            if ((out = fopen (dummyo, "w")) != NULL) {
                fwrite (&team.id, sizeof team.id, 1, out);
                fwrite (&team.year, sizeof team.year, 1, out);
                fwrite (&team.league, sizeof team.league, 1, out);
                fwrite (&team.division, sizeof team.division, 1, out);
                for (xx = 0; xx < 25; xx++) {
                    fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                    fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                    fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                    for (yy = 0; yy < 11; yy++)
                        fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                }
                for (xx = 0; xx < 11; xx++) {
                    fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                    fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                }
                fclose (out);
            }

            /* get real life stats for team #2 */
            if (get_rl_home ()) {
                connected = 0;
                continue;
            }
            team = home;
            if (!team.id)
                /* a temporary ID assigned to user-created teams */
                team.id = 901;
            clear_stats ();
            strcpy (&dummyo[0], "/var/NSB/");
            strcat (&dummyo[0], &nsbdb[user].id[0]);
            strcat (&dummyo[0], "/");
            if (yearh[0] != '0') {
                strcat (&dummyo[0], &yearh[0]);
                strcat (&dummyo[0], &home_team[0]);
            }
            else
                strcat (&dummyo[0], &home_team[13]);

            if ((out = fopen (dummyo, "w")) != NULL) {
                fwrite (&team.id, sizeof team.id, 1, out);
                fwrite (&team.year, sizeof team.year, 1, out);
                fwrite (&team.league, sizeof team.league, 1, out);
                fwrite (&team.division, sizeof team.division, 1, out);
                for (xx = 0; xx < 25; xx++) {
                    fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                    fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                    fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                    for (yy = 0; yy < 11; yy++)
                        fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                }
                for (xx = 0; xx < 11; xx++) {
                    fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                    fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                }
                fclose (out);
            }
        }
        /*
         *
         * end of 'Q' request (establish a series) from client
         *
        */
    }
    closeup ();
    return 0;
}

int
play_league_game () {
    char parent[256], parent2[256], dummy[256], visiting_team[50], home_team[50];
    int x, y, z, xx, yy;
    FILE *in, *out;

    strcpy (&parent[0], "/var/NSB/RealLifeStats");
    strcpy (&parent2[0], "/var/NSB/");
    strcat (&parent2[0], &nsbdb[user].id[0]);

    if (teamviyr) {
        for (x = 0; x <= NUMBER_OF_TEAMS; x++)
            if (teaminfo[x].id == teamvi)
                strcpy (&visiting_team[0], &teaminfo[x].filename[0]);
    }
    else
        strcpy (&visiting_team[0], GetUCTeamname (teamvi));

    if (teamhiyr) {
        for (x = 0; x <= NUMBER_OF_TEAMS; x++)
            if (teaminfo[x].id == teamhi)
                strcpy (&home_team[0], &teaminfo[x].filename[0]);
    }
    else
        strcpy (&home_team[0], GetUCTeamname (teamhi));

    if (!nosend || abb) {
        if (abb) {
            strcpy (&buffer1[0], "TEAMS");
            strcat (&buffer1[0], (char *) cnvt_int2str (4, teamviyr));
        }
        else
            strcpy (&buffer1[0], (char *) cnvt_int2str (4, teamviyr));
        strcat (&buffer1[0], &visiting_team[0]);
        strcat (&buffer1[0], " ");
        strcat (&buffer1[0], (char *) cnvt_int2str (4, teamhiyr));
        strcat (&buffer1[0], &home_team[0]);
        strcat (&buffer1[0], "\n");
        if (sock_puts (sock, &buffer1[0]) < 0)
            return -1;
        if (netgame)
            if (sock_puts (netsock, &buffer1[0]) < 0)
                return -1;
    }

    /* get real life (or user-created) stats for visiting team */
    if (teamviyr) {
        strcpy (&dummy[0], &parent[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], (char *) cnvt_int2str (4, teamviyr));
    }
    else {
        strcpy (&dummy[0], &parent2[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], "UserTeams");
    }
    strcat (&dummy[0], "/");
    strcat (&dummy[0], &visiting_team[0]);

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&visitor.id, sizeof visitor.id, 1, in);
        fread (&visitor.year, sizeof visitor.year, 1, in);
        fread (&visitor.league, sizeof visitor.league, 1, in);
        fread (&visitor.division, sizeof visitor.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&visitor.batters[x].id, sizeof visitor.batters[x].id, 1, in);
            fread (&visitor.batters[x].dob, sizeof visitor.batters[x].dob, 1, in);
            fread (&visitor.batters[x].hitting, sizeof visitor.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&visitor.batters[x].fielding[y], sizeof visitor.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&visitor.pitchers[x].id, sizeof visitor.pitchers[x].id, 1, in);
            fread (&visitor.pitchers[x].pitching, sizeof visitor.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_ERR, "There is something wrong with %s: %s", dummy, strerror (errno));
        if (!nosend)
            if (sock_puts (sock, "fuckup\n") < 0)
                return -2;
    }
    /* determine the number of players and pitchers this team has */
    for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
        if (visitor.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&visitor.batters[maxplayers[0]].id.name[0]))
            break;
    for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
        if (visitor.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&visitor.pitchers[maxpitchers[0]].id.name[0]))
            break;

    /* get total season stats for visiting team */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/");
    if (teamviyr)
        strcat (&dummy[0], (char *) cnvt_int2str (4, teamviyr));
    strcat (&dummy[0], &visiting_team[0]);
    if (eos_sw)
        strcat (&dummy[0], "-PS");

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&visitor_season.id, sizeof visitor_season.id, 1, in);
        fread (&visitor_season.year, sizeof visitor_season.year, 1, in);
        fread (&visitor_season.league, sizeof visitor_season.league, 1, in);
        fread (&visitor_season.division, sizeof visitor_season.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&visitor_season.batters[x].id, sizeof visitor_season.batters[x].id, 1, in);
            fread (&visitor_season.batters[x].dob, sizeof visitor_season.batters[x].dob, 1, in);
            fread (&visitor_season.batters[x].hitting, sizeof visitor_season.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&visitor_season.batters[x].fielding[y], sizeof visitor_season.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&visitor_season.pitchers[x].id, sizeof visitor_season.pitchers[x].id, 1, in);
            fread (&visitor_season.pitchers[x].pitching, sizeof visitor_season.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else
        /* there are no total season stats available this must be the first game for this team and this user */
        zero_visitor_season ();

    /* get real life (or user-created) stats for home team */
    if (teamhiyr) {
        strcpy (&dummy[0], &parent[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], (char *) cnvt_int2str (4, teamhiyr));
    }
    else {
        strcpy (&dummy[0], &parent2[0]);
        strcat (&dummy[0], "/");
        strcat (&dummy[0], "UserTeams");
    }
    strcat (&dummy[0], "/");
    strcat (&dummy[0], &home_team[0]);

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&home.id, sizeof home.id, 1, in);
        fread (&home.year, sizeof home.year, 1, in);
        fread (&home.league, sizeof home.league, 1, in);
        fread (&home.division, sizeof home.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&home.batters[x].id, sizeof home.batters[x].id, 1, in);
            fread (&home.batters[x].dob, sizeof home.batters[x].dob, 1, in);
            fread (&home.batters[x].hitting, sizeof home.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&home.batters[x].fielding[y], sizeof home.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&home.pitchers[x].id, sizeof home.pitchers[x].id, 1, in);
            fread (&home.pitchers[x].pitching, sizeof home.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_ERR, "There is something wrong with %s: %s", dummy, strerror (errno));
        if (!nosend)
            if (sock_puts (sock, "fuckup\n") < 0)
                return -3;
    }
    /* determine the number of players and pitchers this team has */
    for (maxplayers[1] = 0; maxplayers[1] < 25; maxplayers[1]++)
        if (home.batters[maxplayers[1]].id.name[0] == ' ' || !strlen (&home.batters[maxplayers[1]].id.name[0]))
            break;
    for (maxpitchers[1] = 0; maxpitchers[1] < 11; maxpitchers[1]++)
        if (home.pitchers[maxpitchers[1]].id.name[0] == ' ' || !strlen (&home.pitchers[maxpitchers[1]].id.name[0]))
            break;

    /* get total season stats for home team */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/");
    if (teamhiyr)
        strcat (&dummy[0], (char *) cnvt_int2str (4, teamhiyr));
    strcat (&dummy[0], &home_team[0]);
    if (eos_sw)
        strcat (&dummy[0], "-PS");

    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&home_season.id, sizeof home_season.id, 1, in);
        fread (&home_season.year, sizeof home_season.year, 1, in);
        fread (&home_season.league, sizeof home_season.league, 1, in);
        fread (&home_season.division, sizeof home_season.division, 1, in);
        for (x = 0; x < 25; x++) {
            fread (&home_season.batters[x].id, sizeof home_season.batters[x].id, 1, in);
            fread (&home_season.batters[x].dob, sizeof home_season.batters[x].dob, 1, in);
            fread (&home_season.batters[x].hitting, sizeof home_season.batters[x].hitting, 1, in);
            for (y = 0; y < 11; y++)
                fread (&home_season.batters[x].fielding[y], sizeof home_season.batters[x].fielding[y], 1, in);
        }
        for (x = 0; x < 11; x++) {
            fread (&home_season.pitchers[x].id, sizeof home_season.pitchers[x].id, 1, in);
            fread (&home_season.pitchers[x].pitching, sizeof home_season.pitchers[x].pitching, 1, in);
        }
        fclose (in);
    }
    else
        /* there are no total season stats available; this must be the first game for this team and this user */
        zero_home_season ();

    /* clear arrays for starters, batting order, and pitchers (set outside of range, 0 is valid) */
    for (x = 0; x < 2; x++)
        for (y = 0; y < 10; y++)
            starters[x][y] = 99;
    for (x = 0; x < 2; x++)
        for (y = 0; y < 9; y++)
            for (z = 0; z < 30; z++)
                border[x][y].player[z] = border[x][y].pos[z] = 99;
    for (x = 0; x < 2; x++)
        for (y = 0; y < 15; y++) {
            pitching[x].pitcher[y] = 99;
            pitching[x].innings[y] = pitching[x].thirds[y] = 0;
        }

    /* determine if to use DH */
    if (dhcode == 3)
        dhind = YES;
    else
        if (dhcode == 1 && home_season.league == 'A')
            dhind = YES;
        else
            if (dhcode == 2 && home_season.league == 'N')
                dhind = YES;
            else
                dhind = NO;

    /* set up for current game stats */
    setup_cgs ();
    if (!nosend) {
        buffer1[0] = dhind + '0';
        buffer1[1] = '\n';
        buffer1[2] = '\0';
        if (sock_puts (sock, &buffer1[0]) < 0)
            return -4;
        if (netgame)
            if (sock_puts (netsock, &buffer1[0]) < 0)
                return -4;
    }

    if (hmanager) {
        send_stats (sock, 'a');
        send_stats (sock, 'c');
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
            closeup ();
        if (!strncmp (&buffer[0], "FORGET IT", 9)) {
            determine_starters ('h');
            determine_battingorder ('h');
        }
        else {
            team = home;
            for (y = x = 0; x < (9 + dhind); x++) {
                z = 0;
                while (buffer[y] != ' ')
                    dummy[z++] = buffer[y++];
                y++;
                dummy[z] = '\0';
                if (x != 9)
                    border[1][x].player[0] = atoi (&dummy[0]);
                else
                    pdh = atoi (&dummy[0]);

                z = 0;
                while (buffer[y] != ' ')
                    dummy[z++] = buffer[y++];
                y++;
                dummy[z] = '\0';
                if (x != 9)
                    border[1][x].pos[0] = atoi (&dummy[0]);

                if (!dhind) {
                    if (border[1][x].pos[0] == 1) {
                        starters[1][1] = border[1][x].player[0];
                        for (z = 0; z < 11; z++)
                            if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[border[1][x].player[0]].id.name[0])) {
                                game_status.pitcher[1] = pitching[1].pitcher[0] = z;
                                break;
                            }
                    }
                }
                else
                    if (x == 9) {
                        starters[1][1] = pdh;
                        for (z = 0; z < 11; z++)
                            if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[pdh].id.name[0])) {
                                game_status.pitcher[1] = pitching[1].pitcher[0] = z;
                                break;
                            }
                    }
            }
            for (x = 0; x < 10; x++)
                for (y = 0; y < 9; y++)
                    if (border[1][y].pos[0] == x)
                        starters[1][x] = border[1][y].player[0];

            gotateam = 1;
        }
    }
    else {
        determine_starters ('h');
        determine_battingorder ('h');
    }

    if (vmanager) {
        if (netgame) {
            buffer1[0] = dhind + '0';
            buffer1[1] = '\n';
            buffer1[2] = '\0';
            sock_puts (netsock, &buffer1[0]);
            send_stats (netsock, 'b');
            send_stats (netsock, 'd');
        }
        else {
            send_stats (sock, 'b');
            send_stats (sock, 'd');
        }
        if (netgame) {
            if (sock_gets (netsock, &buffer[0], sizeof (buffer)) < 0)
                closeup ();
        }
        else
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                closeup ();
        if (!strncmp (&buffer[0], "FORGET IT", 9)) {
            determine_starters ('v');
            determine_battingorder ('v');
        }
        else {
            team = visitor;
            for (y = x = 0; x < (9 + dhind); x++) {
                z = 0;
                while (buffer[y] != ' ')
                    dummy[z++] = buffer[y++];
                y++;
                dummy[z] = '\0';
                if (x != 9)
                    border[0][x].player[0] = atoi (&dummy[0]);
                else
                    pdh = atoi (&dummy[0]);

                z = 0;
                while (buffer[y] != ' ')
                    dummy[z++] = buffer[y++];
                y++;
                dummy[z] = '\0';
                if (x != 9)
                    border[0][x].pos[0] = atoi (&dummy[0]);

                if (!dhind) {
                    if (border[0][x].pos[0] == 1) {
                        starters[0][1] = border[0][x].player[0];
                        for (z = 0; z < 11; z++)
                            if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[border[0][x].player[0]].id.name[0])) {
                                game_status.pitcher[0] = pitching[0].pitcher[0] = z;
                                break;
                            }
                    }
                }
                else
                    if (x == 9) {
                        starters[0][1] = pdh;
                        for (z = 0; z < 11; z++)
                            if (!strcmp (&team.pitchers[z].id.name[0], &team.batters[pdh].id.name[0])) {
                                game_status.pitcher[0] = pitching[0].pitcher[0] = z;
                                break;
                            }
                    }
            }
            for (x = 0; x < 10; x++)
                for (y = 0; y < 9; y++)
                    if (border[0][y].pos[0] == x)
                        starters[0][x] = border[0][y].player[0];

            gotateam = 1;
        }
    }
    else {
        determine_starters ('v');
        determine_battingorder ('v');
    }
    if (pol != 'l') {
        /* if user is not playing a portion of his season then send starting lineups & send ID info */
        send_lineup ();
        send_DOB ();
    }
    playthegame ();
    netgame = 0;

    /* write total season stats for visiting team */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/");
    if (teamviyr)
        strcat (&dummy[0], (char *) cnvt_int2str (4, visitor_season.year));
    strcat (&dummy[0], &visiting_team[0]);
    if (eos_sw)
        strcat (&dummy[0], "-PS");
    if ((out = fopen (dummy, "w")) != NULL) {
        fwrite (&visitor_season.id, sizeof visitor_season.id, 1, out);
        fwrite (&visitor_season.year, sizeof visitor_season.year, 1, out);
        fwrite (&visitor_season.league, sizeof visitor_season.league, 1, out);
        fwrite (&visitor_season.division, sizeof visitor_season.division, 1, out);
        for (xx = 0; xx < 25; xx++) {
            fwrite (&visitor_season.batters[xx].id, sizeof visitor_season.batters[xx].id, 1, out);
            fwrite (&visitor_season.batters[xx].dob, sizeof visitor_season.batters[xx].dob, 1, out);
            fwrite (&visitor_season.batters[xx].hitting, sizeof visitor_season.batters[xx].hitting, 1, out);
            for (yy = 0; yy < 11; yy++)
                fwrite (&visitor_season.batters[xx].fielding[yy], sizeof visitor_season.batters[xx].fielding[yy], 1, out);
        }
        for (xx = 0; xx < 11; xx++) {
            fwrite (&visitor_season.pitchers[xx].id, sizeof visitor_season.pitchers[xx].id, 1, out);
            fwrite (&visitor_season.pitchers[xx].pitching, sizeof visitor_season.pitchers[xx].pitching, 1, out);
        }
        fclose (out);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s for writing: %s", dummy, strerror (errno));
        return -5;
    }

    /* write total season stats for home team */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/");
    if (teamhiyr)
        strcat (&dummy[0], (char *) cnvt_int2str (4, home_season.year));
    strcat (&dummy[0], &home_team[0]);
    if (eos_sw)
        strcat (&dummy[0], "-PS");
    if ((out = fopen (dummy, "w")) != NULL) {
        fwrite (&home_season.id, sizeof home_season.id, 1, out);
        fwrite (&home_season.year, sizeof home_season.year, 1, out);
        fwrite (&home_season.league, sizeof home_season.league, 1, out);
        fwrite (&home_season.division, sizeof home_season.division, 1, out);
        for (xx = 0; xx < 25; xx++) {
            fwrite (&home_season.batters[xx].id, sizeof home_season.batters[xx].id, 1, out);
            fwrite (&home_season.batters[xx].dob, sizeof home_season.batters[xx].dob, 1, out);
            fwrite (&home_season.batters[xx].hitting, sizeof home_season.batters[xx].hitting, 1, out);
            for (yy = 0; yy < 11; yy++)
                fwrite (&home_season.batters[xx].fielding[yy], sizeof home_season.batters[xx].fielding[yy], 1, out);
        }
        for (xx = 0; xx < 11; xx++) {
            fwrite (&home_season.pitchers[xx].id, sizeof home_season.pitchers[xx].id, 1, out);
            fwrite (&home_season.pitchers[xx].pitching, sizeof home_season.pitchers[xx].pitching, 1, out);
        }
        fclose (out);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s for writing: %s", dummy, strerror (errno));
        return -6;
    }

    return 0;
}

int
chk_eos () {
    int x, cntx;

    for (cntx = x = 0; x < 243; x++) {
        if (strlen (&schedule[x][0]) < 21)
            schedule[x][3] = 'X';
        if (schedule[x][3] == 'X')
            cntx++;
    }

    if (cntx == 243)
        return 1;
    else
        return 0;
}

int
chk_eops () {
    int x, cntx;
    char dummy[256];
    FILE *out;

    update_sch_ps ();

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Schedule-PS");
    if ((out = fopen (dummy, "w")) != NULL) {
        for (x = 0; x < 37; x++)
            fwrite (&schedule[x][0], strlen (&schedule[x][0]), 1, out);
        fclose (out);
    }
    else
        closeup ();

    for (cntx = x = 0; x < 36; x++) {
        if (schedule[x][3] == 'X')
            cntx++;
    }

    if (cntx == 36)
        return 1;
    else
        return 0;
}

void
kill_league () {
    int x, y;
    char parent[256], dummy[256], dummy2[256], dummyi[256], dummyo[256];
    struct dirent *dir;
    DIR *fnames, *fnames2;
    FILE *in, *out;

    strcpy (&parent[0], "/var/NSB");
    strcpy (&dummy[0], &parent[0]);
    strcat (&dummy[0], "/");
    strcat (&dummy[0], &nsbdb[user].id[0]);

    strcpy (&dummy2[0], &dummy[0]);
    strcat (&dummy2[0], "/Schedule");
    unlink (dummy2);

    strcpy (&dummy2[0], &dummy[0]);
    /* check that Lifetime directory exists and if it doesn't then create it */
    strcat (&dummy2[0], "/Lifetime");
    if ((fnames2 = opendir (&dummy2[0])) == NULL) {
        if (mkdir (&dummy2[0], 0700))
            closeup ();
    }
    else
        closedir (fnames2);
    /* update Lifetime & Server Records */
    get_records ();

    strcpy (&dummy2[0], &dummy[0]);
    strcat (&dummy2[0], "/Lifetime");
    /* combine current season teams with same teams existing in Lifetime directory and delete current season teams */
    if ((fnames = opendir (&dummy[0])) != NULL) {
        while ((dir = readdir (fnames))) {
            /* don't process . and .. files */
            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                continue;
            /* don't process these files - they will get deleted when the user forms a new season */
            if (!strcmp (dir->d_name, "Schedule-PS") || !strcmp (dir->d_name, "Standings") || !strcmp (dir->d_name, "Series"))
                continue;
            if (!strcmp (dir->d_name, "PlayoffResultsNL") || !strcmp (dir->d_name, "PlayoffResultsAL"))
                continue;
            /* don't process the LeagueSetup file */
            if (!strcmp (dir->d_name, "LeagueSetup"))
                continue;
            /* look for any teams */
            if (!strcmp (dir->d_name, "Lifetime") || !strcmp (dir->d_name, "Records") || !strcmp (dir->d_name, "UserTeams"))
                continue;
            strcpy (&dummyi[0], &dummy[0]);
            strcat (&dummyi[0], "/");
            strcat (&dummyi[0], dir->d_name);

            if ((in = fopen (dummyi, "r")) != NULL) {
                fread (&team.id, sizeof team.id, 1, in);
                fread (&team.year, sizeof team.year, 1, in);
                fread (&team.league, sizeof team.league, 1, in);
                fread (&team.division, sizeof team.division, 1, in);
                for (x = 0; x < 25; x++) {
                    fread (&team.batters[x].id, sizeof team.batters[x].id, 1, in);
                    fread (&team.batters[x].dob, sizeof team.batters[x].dob, 1, in);
                    fread (&team.batters[x].hitting, sizeof team.batters[x].hitting, 1, in);
                    for (y = 0; y < 11; y++)
                        fread (&team.batters[x].fielding[y], sizeof team.batters[x].fielding[y], 1, in);
                }
                for (x = 0; x < 11; x++) {
                    fread (&team.pitchers[x].id, sizeof team.pitchers[x].id, 1, in);
                    fread (&team.pitchers[x].pitching, sizeof team.pitchers[x].pitching, 1, in);
                }
                fclose (in);
            }

            /* determine the number of players and pitchers this team has */
            for (maxplayers[0] = 0; maxplayers[0] < 25; maxplayers[0]++)
                if (team.batters[maxplayers[0]].id.name[0] == ' ' || !strlen (&team.batters[maxplayers[0]].id.name[0]))
                    break;
            for (maxpitchers[0] = 0; maxpitchers[0] < 11; maxpitchers[0]++)
                if (team.pitchers[maxpitchers[0]].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers[0]].id.name[0]))
                    break;
            totgames = 160;
            update_records (-1, 0, 0, dir->d_name);    /* update the seasonal records */

            strcpy (&dummyo[0], &dummy2[0]);
            strcat (&dummyo[0], "/");
            strcat (&dummyo[0], dir->d_name);
            if ((in = fopen (dummyo, "r")) != NULL) {
                fread (&team2.id, sizeof team2.id, 1, in);
                fread (&team2.year, sizeof team2.year, 1, in);
                fread (&team2.league, sizeof team2.league, 1, in);
                fread (&team2.division, sizeof team2.division, 1, in);
                for (x = 0; x < 25; x++) {
                    fread (&team2.batters[x].id, sizeof team2.batters[x].id, 1, in);
                    fread (&team2.batters[x].dob, sizeof team2.batters[x].dob, 1, in);
                    fread (&team2.batters[x].hitting, sizeof team2.batters[x].hitting, 1, in);
                    for (y = 0; y < 11; y++)
                        fread (&team2.batters[x].fielding[y], sizeof team2.batters[x].fielding[y], 1, in);
                }
                for (x = 0; x < 11; x++) {
                    fread (&team2.pitchers[x].id, sizeof team2.pitchers[x].id, 1, in);
                    fread (&team2.pitchers[x].pitching, sizeof team2.pitchers[x].pitching, 1, in);
                }
                fclose (in);
                /* accumulate stats */
                if (team2.id < 900)
                    /* do not accumulate lifetime stats for User-Created teams */
                    combine_stats ();
            }
            if (team.id < 900) {
                /* do not write lifetime stats for User-Created teams since we don't want to cum them */
                out = fopen (dummyo, "w");
                fwrite (&team.id, sizeof team.id, 1, out);
                fwrite (&team.year, sizeof team.year, 1, out);
                fwrite (&team.league, sizeof team.league, 1, out);
                fwrite (&team.division, sizeof team.division, 1, out);
                for (xx = 0; xx < 25; xx++) {
                    fwrite (&team.batters[xx].id, sizeof team.batters[xx].id, 1, out);
                    fwrite (&team.batters[xx].dob, sizeof team.batters[xx].dob, 1, out);
                    fwrite (&team.batters[xx].hitting, sizeof team.batters[xx].hitting, 1, out);
                    for (yy = 0; yy < 11; yy++)
                        fwrite (&team.batters[xx].fielding[yy], sizeof team.batters[xx].fielding[yy], 1, out);
                }
                for (xx = 0; xx < 11; xx++) {
                    fwrite (&team.pitchers[xx].id, sizeof team.pitchers[xx].id, 1, out);
                    fwrite (&team.pitchers[xx].pitching, sizeof team.pitchers[xx].pitching, 1, out);
                }
                fclose (out);
            }
        }
        closedir (fnames);
    }

    get_lifetime_records ();
    update_lifetime_records ();
    put_lifetime_records ();
    update_server_records ();
}

/*
   the regular season is finished, setup for the post-season
*/
int
setup_postseason () {
    int x, y, z, n, nteams[6], loop, lloop, divloop, hti;
    char buf[500], buf1[500];
    struct workteams teamh;
    float hpct, hpct1, hpct2;
    FILE *in, *out;
    struct {
        int id, yr;
        float pct;
    } pteams[10];
    struct {
        int id, yr;
        float pct;
    } holdteams[300], ht, ht4[4];

    /* load standings */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Standings");
    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&teamwins[0], sizeof teamwins, 1, in);
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
        return -1;
    }

    /* load league set-up */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/LeagueSetup");
    if ((in = fopen (dummy, "r")) != NULL) {
        fread (&league_setup, sizeof league_setup, 1, in);
        fclose (in);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
        return -1;
    }

    /* clear */
    for (loop = 0; loop < 6; loop++) {
        nteams[loop] = 0;
        for (x = 0; x < 300; x++) {
            teams[loop][x].id = teams[loop][x].wins = teams[loop][x].losses = 0;
            teams[loop][x].pct = 0.0;
        }
    }

    if (!league_setup.nummaxgames[0])
        /* no post-season */
        goto BuildPSSchedule;

    /* fill in work area */
    for (x = 0; teamwins[x].id != 0 && x < 300; x++) {
        if (teamwins[x].league == 'A' || league_setup.numleagues == 1)
            if (teamwins[x].div == 'E')
                loop = 0;
            else
                if (teamwins[x].div == 'C')
                    loop = 1;
                else
                    if (league_setup.numdivs == 2)
                        loop = 1;
                    else
                        loop = 2;
        else
            if (teamwins[x].div == 'E')
                loop = 3;
            else
                if (teamwins[x].div == 'C')
                    loop = 4;
                else
                    if (league_setup.numdivs == 2)
                        loop = 4;
                    else
                        loop = 5;
        teams[loop][nteams[loop]].id = teamwins[x].id;
        teams[loop][nteams[loop]].year = teamwins[x].year;
        for (y = 0; y < 300; y++)
            if (teamwins[x].opp[y].id)
                teams[loop][nteams[loop]].wins += teamwins[x].opp[y].wins;
        for (y = 0; teamwins[y].id != 0 && y < 300; y++)
            if (teamwins[x].id != teamwins[y].id || teamwins[x].year != teamwins[y].year)
                for (z = 0; z < 300; z++)
                    if (teamwins[y].opp[z].id == teamwins[x].id && teamwins[y].opp[z].year == teamwins[x].year)
                        teams[loop][nteams[loop]].losses += teamwins[y].opp[z].wins;
        teams[loop][nteams[loop]].pct = (float) teams[loop][nteams[loop]].wins / (float) (teams[loop][nteams[loop]].wins + teams[loop][nteams[loop]].losses);
        nteams[loop]++;
    }

    /* sort by won/lost percentage */
    for (loop = 0; loop < 6; loop++)
        if (nteams[loop])
            for (x = 0; x < (nteams[loop] - 1); x++)
                for (y = x + 1; y < nteams[loop]; y++)
                    if (teams[loop][x].pct < teams[loop][y].pct) {
                        teamh = teams[loop][x];
                        teams[loop][x] = teams[loop][y];
                        teams[loop][y] = teamh;
                    }

    for (x = 0; x < 10; x++)
        pteams[x].id = 0;

    /* find division winner(s) */
    for (lloop = 0; lloop < league_setup.numleagues; lloop++) {
        for (divloop = 0; divloop < league_setup.numdivs; divloop++) {
            hpct = teams[(lloop * 3) + divloop][0].pct;
            /* store the pct of the 2nd place teams in the other divisions */
            if (!divloop) {
                hpct1 = teams[(lloop * 3) + 1][1].pct;
                hpct2 = teams[(lloop * 3) + 2][1].pct;
            }
            if (divloop == 1) {
                hpct1 = teams[(lloop * 3)][1].pct;
                hpct2 = teams[(lloop * 3) + 2][1].pct;
            }
            if (divloop == 2) {
                hpct1 = teams[(lloop * 3)][1].pct;
                hpct2 = teams[(lloop * 3) + 1][1].pct;
            }

            /* find number of teams with the same highest W/L percentage in division */
            for (hti = x = 0; x < nteams[(lloop * 3) + divloop]; x++)
                if (teams[(lloop * 3) + divloop][x].pct == hpct) {
                    holdteams[hti].id = teams[(lloop * 3) + divloop][x].id;
                    holdteams[hti].yr = teams[(lloop * 3) + divloop][x].year;
                    holdteams[hti].pct = teams[(lloop * 3) + divloop][x].pct;
                    hti++;
                }
            if (hti > 1) {
                /* a tie for the division */
                if (league_setup.numwc)
                    /* if the non-division-winner is the WC then determine the division winner by (1) head-to-head winning pct, (2) coin flip */
                    if (hpct > hpct1 && hpct > hpct2 && hpct > teams[(lloop * 3) + divloop][2].pct) {
                        int t1, t2;

                        for (t1 = z = 0; z < 300; z++)
                            if (teamwins[z].id == teams[(lloop * 3) + divloop][0].id && teamwins[z].year == teams[(lloop * 3) + divloop][0].year)
                                for (x = 0; x < 300; x++)
                                    if (teamwins[z].opp[x].id == teams[(lloop * 3) + divloop][1].id &&
                                                                                 teamwins[z].opp[x].year == teams[(lloop * 3) + divloop][1].year)
                                        t1 = teamwins[z].opp[x].wins;
                        for (t2 = z = 0; z < 300; z++)
                            if (teamwins[z].id == teams[(lloop * 3) + divloop][1].id && teamwins[z].year == teams[(lloop * 3) + divloop][1].year)
                                for (x = 0; x < 300; x++)
                                    if (teamwins[z].opp[x].id == teams[(lloop * 3) + divloop][0].id &&
                                                                                 teamwins[z].opp[x].year == teams[(lloop * 3) + divloop][0].year)
                                        t2 = teamwins[z].opp[x].wins;
                        if (t1 == t2)
                            /* coin flip */
                            if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                                hteams = teams[(lloop * 3) + divloop][0];
                                teams[(lloop * 3) + divloop][0] = teams[(lloop * 3) + divloop][1];
                                teams[(lloop * 3) + divloop][1] = hteams;
                                strcpy (&buf[0], "7");
                            }
                            else
                                strcpy (&buf[0], "8");
                        else
                            if (t2 > t1) {
                                hteams = teams[(lloop * 3) + divloop][0];
                                teams[(lloop * 3) + divloop][0] = teams[(lloop * 3) + divloop][1];
                                teams[(lloop * 3) + divloop][1] = hteams;
                                strcpy (&buf[0], "5");
                            }
                            else
                                strcpy (&buf[0], "6");

                        /* save results of the tied teams */
                        strcat (&buf[0], (char *) cnvt_int2str (4, teams[(lloop * 3) + divloop][0].year));
                        if (teams[(lloop * 3) + divloop][0].id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (teams[(lloop * 3) + divloop][0].id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, teams[(lloop * 3) + divloop][0].id));
                        strcat (&buf[0], (char *) cnvt_int2str (4, teams[(lloop * 3) + divloop][1].year));
                        if (teams[(lloop * 3) + divloop][1].id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (teams[(lloop * 3) + divloop][1].id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, teams[(lloop * 3) + divloop][1].id));

                        strcat (&buf[0], "::");
                        strcpy (&dummy[0], "/var/NSB/");
                        strcat (&dummy[0], &nsbdb[user].id[0]);
                        strcat (&dummy[0], "/PlayoffResults");
                        if (!lloop)
                            strcat (&dummy[0], "AL");
                        else
                            strcat (&dummy[0], "NL");
                        /* if the file already exists then append (if the string doesn't already exist) */
                        if ((in = fopen (dummy, "r")) != NULL) {
                            fread (&buf1, sizeof buf1, 1, in);
                            fclose (in);
                            if (!strstr (&buf1[0], &buf[0])) {
                                strcat (&buf1[0], &buf[0]);
                                strcpy (&buf[0], &buf1[0]);

                                if ((out = fopen (dummy, "w")) != NULL) {
                                    fwrite (&buf[0], sizeof buf, 1, out);
                                    fclose (out);
                                }
                                else
                                    closeup ();
                            }
                        }
                        else
                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        hti = 1;
                    }

                if (hti == 2) {
                    /* playoff game to break tie */
                    int tw;

                    /* flip a coin for home field */
                    if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                        teamvi = holdteams[0].id;
                        teamviyr = holdteams[0].yr;
                        teamhi = holdteams[1].id;
                        teamhiyr = holdteams[1].yr;
                    }
                    else {
                        teamvi = holdteams[1].id;
                        teamviyr = holdteams[1].yr;
                        teamhi = holdteams[0].id;
                        teamhiyr = holdteams[0].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "d");
                    else
                        strcpy (&buf[0], "9");

                    /* update work area and re-sort
                       don't add to losses so as not to exclude losing team from wild card consideration */
                    for (x = 0; x < 300; x++)
                        if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                            teams[(lloop * 3) + divloop][x].wins++;
                            tw = x;
                        }
                    teams[(lloop * 3) + divloop][tw].pct = (float) teams[(lloop * 3) + divloop][tw].wins /
                                                           (float) (teams[(lloop * 3) + divloop][tw].wins + teams[(lloop * 3) + divloop][tw].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti == 3) {
                    /* 3 teams tied for the Division - 2 playoff games */
                    int holdwid = 0, holdwyr = 0, holdlid = 0, holdlyr = 0, tw;

                    x = (int) ((float) 6 * rand () / (RAND_MAX + 1.0));
                    switch (x) {
                        /* if x == 0 teams stay as they are */
                        case 1:
                            ht = holdteams[1];
                            holdteams[1] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                        case 2:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[1];
                            holdteams[1] = ht;
                            break;
                        case 3:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[1];
                            holdteams[1] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                        case 4:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[2];
                            holdteams[2] = holdteams[1];
                            holdteams[1] = ht;
                            break;
                        case 5:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                    }
                    teamvi = holdteams[1].id;
                    teamviyr = holdteams[1].yr;
                    teamhi = holdteams[0].id;
                    teamhiyr = holdteams[0].yr;
                    if (!PlayoffGame ())
                        return 0;

                    holdwid = win_id;
                    holdwyr = win_year;
                    holdlid = lose_id;
                    holdlyr = lose_year;
                    /* update work area and re-sort
                       don't add to losses so as not to exclude losing team from wild card consideration */
                    for (x = 0; x < 300; x++)
                        if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                            teams[(lloop * 3) + divloop][x].wins++;
                            tw = x;
                        }
                    teams[(lloop * 3) + divloop][tw].pct = (float) teams[(lloop * 3) + divloop][tw].wins /
                                                            (float) (teams[(lloop * 3) + divloop][tw].wins + teams[(lloop * 3) + divloop][tw].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    teamvi = holdteams[2].id;
                    teamviyr = holdteams[2].yr;
                    if (win_id == home.id && win_year == home.year) {
                        teamhi = holdteams[0].id;
                        teamhiyr = holdteams[0].yr;
                    }
                    else {
                        teamhi = holdteams[1].id;
                        teamhiyr = holdteams[1].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* update work area and re-sort
                       don't add to losses so as not to exclude losing team from wild card consideration */
                    for (x = 0; x < 300; x++)
                        if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                            teams[(lloop * 3) + divloop][x].wins++;
                            tw = x;
                        }
                    teams[(lloop * 3) + divloop][tw].pct = (float) teams[(lloop * 3) + divloop][tw].wins /
                                                            (float) (teams[(lloop * 3) + divloop][tw].wins + teams[(lloop * 3) + divloop][tw].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "e");
                    else
                        strcpy (&buf[0], "a");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr));
                    if (holdwid >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr));
                    if (holdlid >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid));
                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti == 4) {
                    /* 4 teams tied for the Division - 3 playoff games */
                    int holdwid[2], holdwyr[2], holdlid[2], holdlyr[2], tw;

                    for (x = 0; x < 4; x++)
                        ht4[x] = holdteams[x];
                    for (y = 0, z = 4; z > 0; z--, y++) {
                        x = (int) ((float) z * rand () / (RAND_MAX + 1.0));
                        holdteams[y] = ht4[x];
                        for (; x < 4; x++)
                            ht4[x] = ht4[x + 1];
                    }
                    for (x = 0; x < 3; x += 2) {
                        teamvi = holdteams[x + 1].id;
                        teamviyr = holdteams[x + 1].yr;
                        teamhi = holdteams[x].id;
                        teamhiyr = holdteams[x].yr;
                        if (!PlayoffGame ())
                            return 0;

                        /* update work area and re-sort
                           don't add to losses so as not to exclude losing team from wild card consideration */
                        for (x = 0; x < 300; x++)
                            if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                teams[(lloop * 3) + divloop][x].wins++;
                                tw = x;
                            }
                        teams[(lloop * 3) + divloop][tw].pct = (float) teams[(lloop * 3) + divloop][tw].wins /
                                                                (float) (teams[(lloop * 3) + divloop][tw].wins + teams[(lloop * 3) + divloop][tw].losses);
                        for (loop = 0; loop < 6; loop++)
                            if (nteams[loop])
                                for (x = 0; x < (nteams[loop] - 1); x++)
                                    for (y = x + 1; y < nteams[loop]; y++)
                                        if (teams[loop][x].pct < teams[loop][y].pct) {
                                            teamh = teams[loop][x];
                                            teams[loop][x] = teams[loop][y];
                                            teams[loop][y] = teamh;
                                        }

                        ht4[x].id = win_id;
                        ht4[x].yr = win_year;
                        if (!x) {
                            holdwid[0] = win_id;
                            holdwyr[0] = win_year;
                            holdlid[0] = lose_id;
                            holdlyr[0] = lose_year;
                        }
                        else {
                            holdwid[1] = win_id;
                            holdwyr[1] = win_year;
                            holdlid[1] = lose_id;
                            holdlyr[1] = lose_year;
                        }
                    }
                    if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                        teamvi = ht4[0].id;
                        teamviyr = ht4[0].yr;
                        teamhi = ht4[2].id;
                        teamhiyr = ht4[2].yr;
                    }
                    else {
                        teamvi = ht4[2].id;
                        teamviyr = ht4[2].yr;
                        teamhi = ht4[0].id;
                        teamhiyr = ht4[0].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* update work area and re-sort
                       don't add to losses so as not to exclude losing team from wild card consideration */
                    for (x = 0; x < 300; x++)
                        if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                            teams[(lloop * 3) + divloop][x].wins++;
                            tw = x;
                        }
                    teams[(lloop * 3) + divloop][tw].pct = (float) teams[(lloop * 3) + divloop][tw].wins /
                                                            (float) (teams[(lloop * 3) + divloop][tw].wins + teams[(lloop * 3) + divloop][tw].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "f");
                    else
                        strcpy (&buf[0], "b");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[0]));
                    if (holdwid[0] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid[0]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[0]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[0]));
                    if (holdlid[0] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid[0]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[0]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[1]));
                    if (holdwid[1] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid[1]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[1]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[1]));
                    if (holdlid[1] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid[1]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[1]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti > 4) {
                    /* more than 4 teams tied for the Division - determine the winning team via a coin flip */
                    x = (int) ((float) hti * rand () / (RAND_MAX + 1.0));
                    if (!lloop) {
                        if (!divloop) {
                            ALEDteamID = holdteams[x].id;
                            ALEDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 1) {
                            ALCDteamID = holdteams[x].id;
                            ALCDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 2) {
                            ALWDteamID = holdteams[x].id;
                            ALWDteamYR = holdteams[x].yr;
                        }
                    }
                    else {
                        if (!divloop) {
                            NLEDteamID = holdteams[x].id;
                            NLEDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 1) {
                            NLCDteamID = holdteams[x].id;
                            NLCDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 2) {
                            NLWDteamID = holdteams[x].id;
                            NLWDteamYR = holdteams[x].yr;
                        }
                    }

                    /* save results of the tied teams */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "g");
                    else
                        strcpy (&buf[0], "c");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                    if (holdteams[x].id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                    for (x = 0; x < hti; x++) {
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                        if (holdteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                    }
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
            }

            /* save division winner */
            pteams[(lloop * 5) + divloop].id = teams[(lloop * 3) + divloop][0].id;
            pteams[(lloop * 5) + divloop].yr = teams[(lloop * 3) + divloop][0].year;
            pteams[(lloop * 5) + divloop].pct = teams[(lloop * 3) + divloop][0].pct;
        }

        for (divloop = 0; divloop < league_setup.numdivs; divloop++) {
            /* remove team which won division from wild card consideration */
            for (x = 0; x < nteams[(lloop * 3) + divloop]; x++)
                teams[(lloop * 3) + divloop][x] = teams[(lloop * 3) + divloop][x + 1];
            nteams[(lloop * 3) + divloop]--;
        }
    }

    /* find wild card team(s) */
    if (league_setup.numwc)
        for (lloop = 0; lloop < league_setup.numleagues; lloop++) {
            if (teams[(lloop * 3)][0].pct >= teams[(lloop * 3) + 1][0].pct && teams[(lloop * 3)][0].pct >= teams[(lloop * 3) + 2][0].pct)
                hpct = teams[(lloop * 3)][0].pct;
            else
                if (teams[(lloop * 3) + 1][0].pct >= teams[(lloop * 3)][0].pct && teams[(lloop * 3) + 1][0].pct >= teams[(lloop * 3) + 2][0].pct)
                    hpct = teams[(lloop * 3) + 1][0].pct;
                else
                    hpct = teams[(lloop * 3) + 2][0].pct;
            for (hti = divloop = 0; divloop < league_setup.numdivs; divloop++)
                /* find number of remaining teams with the same highest W/L percentage in league */
                for (x = 0; x < nteams[(lloop * 3) + divloop]; x++)
                    if (teams[(lloop * 3) + divloop][x].pct == hpct) {
                        holdteams[hti].id = teams[(lloop * 3) + divloop][x].id;
                        holdteams[hti].yr = teams[(lloop * 3) + divloop][x].year;
                        holdteams[hti].pct = teams[(lloop * 3) + divloop][x].pct;
                        hti++;
                    }
            if (league_setup.numwc == 2) {
                /* there are two wild card teams */
                if (hti > 2) {
                    /* playoff game(s) to determine wild card team */
                    if (hti == 3) {
                        /* 3 teams tied for the wild card - 2 playoff games */
                        int holdwid = 0, holdwyr = 0, holdlid = 0, holdlyr = 0, tw, tl, dw, dl;

                        x = (int) ((float) 6 * rand () / (RAND_MAX + 1.0));
                        switch (x) {
                            /* if x == 0 teams stay as they are */
                            case 1:
                                ht = holdteams[1];
                                holdteams[1] = holdteams[2];
                                holdteams[2] = ht;
                                break;
                            case 2:
                                ht = holdteams[0];
                                holdteams[0] = holdteams[1];
                                holdteams[1] = ht;
                                break;
                            case 3:
                                ht = holdteams[0];
                                holdteams[0] = holdteams[1];
                                holdteams[1] = holdteams[2];
                                holdteams[2] = ht;
                                break;
                            case 4:
                                ht = holdteams[0];
                                holdteams[0] = holdteams[2];
                                holdteams[2] = holdteams[1];
                                holdteams[1] = ht;
                                break;
                            case 5:
                                ht = holdteams[0];
                                holdteams[0] = holdteams[2];
                                holdteams[2] = ht;
                                break;
                        }
                        teamvi = holdteams[1].id;
                        teamviyr = holdteams[1].yr;
                        teamhi = holdteams[0].id;
                        teamhiyr = holdteams[0].yr;
                        if (!PlayoffGame ())
                            return 0;

                        /* update work area */
                        for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                            for (x = 0; x < 300; x++) {
                                if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                    teams[(lloop * 3) + divloop][x].wins++;
                                    tw = x;
                                    dw = divloop;
                                }
                                if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                    teams[(lloop * 3) + divloop][x].losses++;
                                    tl = x;
                                    dl = divloop;
                                }
                            }
                        teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                          (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                        teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                          (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);

                        holdwid = win_id;
                        holdwyr = win_year;
                        holdlid = lose_id;
                        holdlyr = lose_year;
                        teamvi = holdteams[2].id;
                        teamviyr = holdteams[2].yr;
                        if (win_id == home.id && win_year == home.year) {
                            teamhi = holdteams[0].id;
                            teamhiyr = holdteams[0].yr;
                        }
                        else {
                            teamhi = holdteams[1].id;
                            teamhiyr = holdteams[1].yr;
                        }
                        if (!PlayoffGame ())
                            return 0;

                        /* update work area and re-sort */
                        for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                            for (x = 0; x < 300; x++) {
                                if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                    teams[(lloop * 3) + divloop][x].wins++;
                                    tw = x;
                                    dw = divloop;
                                }
                                if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                    teams[(lloop * 3) + divloop][x].losses++;
                                    tl = x;
                                    dl = divloop;
                                }
                            }
                        teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                          (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                        teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                          (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                        for (loop = 0; loop < 6; loop++)
                            if (nteams[loop])
                                for (x = 0; x < (nteams[loop] - 1); x++)
                                    for (y = x + 1; y < nteams[loop]; y++)
                                        if (teams[loop][x].pct < teams[loop][y].pct) {
                                            teamh = teams[loop][x];
                                            teams[loop][x] = teams[loop][y];
                                            teams[loop][y] = teamh;
                                        }

                        holdteams[0].id = win_id;
                        holdteams[0].yr = win_year;
                        holdteams[0].pct = teams[(lloop * 3) + dw][tw].pct;

                        /* save playoff results */
                        if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                            strcpy (&buf[0], "i");
                        else
                            strcpy (&buf[0], "2");
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr));
                        if (holdwid >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdwid));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdwid));
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr));
                        if (holdlid >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdlid));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdlid));
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                        if (win_id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (win_id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                        if (lose_id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (lose_id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                        strcat (&buf[0], "::");
                        strcpy (&dummy[0], "/var/NSB/");
                        strcat (&dummy[0], &nsbdb[user].id[0]);
                        strcat (&dummy[0], "/PlayoffResults");
                        if (!lloop)
                            strcat (&dummy[0], "AL");
                        else
                            strcat (&dummy[0], "NL");
                        /* if the file already exists then append (if the string doesn't already exist) */
                        if ((in = fopen (dummy, "r")) != NULL) {
                            fread (&buf1, sizeof buf1, 1, in);
                            fclose (in);
                            if (!strstr (&buf1[0], &buf[0])) {
                                strcat (&buf1[0], &buf[0]);
                                strcpy (&buf[0], &buf1[0]);

                                if ((out = fopen (dummy, "w")) != NULL) {
                                    fwrite (&buf[0], sizeof buf, 1, out);
                                    fclose (out);
                                }
                                else
                                    closeup ();
                            }
                        }
                        else
                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                    }
                    if (hti == 4) {
                        /* 4 teams tied for the WC - 3 playoff games */
                        int holdwid[2], holdwyr[2], holdlid[2], holdlyr[2], tw, tl, dw, dl;

                        for (x = 0; x < 4; x++)
                            ht4[x] = holdteams[x];
                        for (y = 0, z = 4; z > 0; z--, y++) {
                            x = (int) ((float) z * rand () / (RAND_MAX + 1.0));
                            holdteams[y] = ht4[x];
                            for (; x < 4; x++)
                                ht4[x] = ht4[x + 1];
                        }
                        for (x = 0; x < 3; x += 2) {
                            teamvi = holdteams[x + 1].id;
                            teamviyr = holdteams[x + 1].yr;
                            teamhi = holdteams[x].id;
                            teamhiyr = holdteams[x].yr;
                            if (!PlayoffGame ())
                                return 0;

                            /* update work area and re-sort */
                            for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                                for (x = 0; x < 300; x++) {
                                    if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                        teams[(lloop * 3) + divloop][x].wins++;
                                        tw = x;
                                        dw = divloop;
                                    }
                                    if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                        teams[(lloop * 3) + divloop][x].losses++;
                                        tl = x;
                                        dl = divloop;
                                    }
                                }
                            teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                              (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                            teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                              (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                            for (loop = 0; loop < 6; loop++)
                                if (nteams[loop])
                                    for (x = 0; x < (nteams[loop] - 1); x++)
                                        for (y = x + 1; y < nteams[loop]; y++)
                                            if (teams[loop][x].pct < teams[loop][y].pct) {
                                                teamh = teams[loop][x];
                                                teams[loop][x] = teams[loop][y];
                                                teams[loop][y] = teamh;
                                            }

                            ht4[x].id = win_id;
                            ht4[x].yr = win_year;
                            if (!x) {
                                holdwid[0] = win_id;
                                holdwyr[0] = win_year;
                                holdlid[0] = lose_id;
                                holdlyr[0] = lose_year;
                            }
                            else {
                                holdwid[1] = win_id;
                                holdwyr[1] = win_year;
                                holdlid[1] = lose_id;
                                holdlyr[1] = lose_year;
                            }
                        }
                        if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                            teamvi = ht4[0].id;
                            teamviyr = ht4[0].yr;
                            teamhi = ht4[2].id;
                            teamhiyr = ht4[2].yr;
                        }
                        else {
                            teamvi = ht4[2].id;
                            teamviyr = ht4[2].yr;
                            teamhi = ht4[0].id;
                            teamhiyr = ht4[0].yr;
                        }
                        if (!PlayoffGame ())
                            return 0;

                        /* update work area and re-sort */
                        for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                            for (x = 0; x < 300; x++) {
                                if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                    teams[(lloop * 3) + divloop][x].wins++;
                                    tw = x;
                                    dw = divloop;
                                }
                                if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                    teams[(lloop * 3) + divloop][x].losses++;
                                    tl = x;
                                    dl = divloop;
                                }
                            }
                        teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                          (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                        teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                          (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                        for (loop = 0; loop < 6; loop++)
                            if (nteams[loop])
                                for (x = 0; x < (nteams[loop] - 1); x++)
                                    for (y = x + 1; y < nteams[loop]; y++)
                                        if (teams[loop][x].pct < teams[loop][y].pct) {
                                            teamh = teams[loop][x];
                                            teams[loop][x] = teams[loop][y];
                                            teams[loop][y] = teamh;
                                        }

                        holdteams[0].id = win_id;
                        holdteams[0].yr = win_year;
                        holdteams[0].pct = teams[(lloop * 3) + dw][tw].pct;

                        /* save playoff results */
                        if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                            strcpy (&buf[0], "j");
                        else
                            strcpy (&buf[0], "3");
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[0]));
                        if (holdwid[0] >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdwid[0]));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[0]));
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[0]));
                        if (holdlid[0] >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdlid[0]));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[0]));
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[1]));
                        if (holdwid[1] >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdwid[1]));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[1]));
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[1]));
                        if (holdlid[1] >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdlid[1]));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[1]));
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                        if (win_id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (win_id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                        if (lose_id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (lose_id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                        strcat (&buf[0], "::");
                        strcpy (&dummy[0], "/var/NSB/");
                        strcat (&dummy[0], &nsbdb[user].id[0]);
                        strcat (&dummy[0], "/PlayoffResults");
                        if (!lloop)
                            strcat (&dummy[0], "AL");
                        else
                            strcat (&dummy[0], "NL");
                        /* if the file already exists then append (if the string doesn't already exist) */
                        if ((in = fopen (dummy, "r")) != NULL) {
                            fread (&buf1, sizeof buf1, 1, in);
                            fclose (in);
                            if (!strstr (&buf1[0], &buf[0])) {
                                strcat (&buf1[0], &buf[0]);
                                strcpy (&buf[0], &buf1[0]);

                                if ((out = fopen (dummy, "w")) != NULL) {
                                    fwrite (&buf[0], sizeof buf, 1, out);
                                    fclose (out);
                                }
                                else
                                    closeup ();
                            }
                        }
                        else
                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                    }
                    if (hti > 4) {
                        /* more than 4 teams tied for the WC - determine the winning team via a coin flip */
                        x = (int) ((float) hti * rand () / (RAND_MAX + 1.0));

                        /* save results of the tied teams */
                        if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                            strcpy (&buf[0], "k");
                        else
                            strcpy (&buf[0], "4");
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                        if (holdteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                        for (x = 0; x < hti; x++) {
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                            if (holdteams[x].id >= 900) {
                                /* user-created team */
                                strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                                strcat (&buf[0], " ");
                            }
                            else
                                strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                        }
                        strcat (&buf[0], "::");
                        strcpy (&dummy[0], "/var/NSB/");
                        strcat (&dummy[0], &nsbdb[user].id[0]);
                        strcat (&dummy[0], "/PlayoffResults");
                        if (!lloop)
                            strcat (&dummy[0], "AL");
                        else
                            strcat (&dummy[0], "NL");
                        /* if the file already exists then append (if the string doesn't already exist) */
                        if ((in = fopen (dummy, "r")) != NULL) {
                            fread (&buf1, sizeof buf1, 1, in);
                            fclose (in);
                            if (!strstr (&buf1[0], &buf[0])) {
                                strcat (&buf1[0], &buf[0]);
                                strcpy (&buf[0], &buf1[0]);

                                if ((out = fopen (dummy, "w")) != NULL) {
                                    fwrite (&buf[0], sizeof buf, 1, out);
                                    fclose (out);
                                }
                                else
                                    closeup ();
                            }
                        }
                        else
                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                    }
                }

                /* save wild card team */
                pteams[(lloop * 5) + 3].id = holdteams[0].id;
                pteams[(lloop * 5) + 3].yr = holdteams[0].yr;
                pteams[(lloop * 5) + 3].pct = holdteams[0].pct;

                /* remove team which won wild card from second wild card team consideration */
                for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                    for (x = 0; x < nteams[(lloop * 3) + divloop]; x++)
                        if (holdteams[0].id == teams[(lloop * 3) + divloop][x].id && holdteams[0].yr == teams[(lloop * 3) + divloop][x].year) {
                            for (y = x; y < nteams[(lloop * 3) + divloop]; y++)
                                teams[(lloop * 3) + divloop][y] = teams[(lloop * 3) + divloop][y + 1];
                            nteams[(lloop * 3) + divloop]--;
                            break;
                        }
            }

            /* if there are two WC teams then the second team is selected here
               if there is one WC team it is selected here */
            if (teams[(lloop * 3)][0].pct >= teams[(lloop * 3) + 1][0].pct && teams[(lloop * 3)][0].pct >= teams[(lloop * 3) + 2][0].pct)
                hpct = teams[(lloop * 3)][0].pct;
            else
                if (teams[(lloop * 3) + 1][0].pct >= teams[(lloop * 3)][0].pct && teams[(lloop * 3) + 1][0].pct >= teams[(lloop * 3) + 2][0].pct)
                    hpct = teams[(lloop * 3) + 1][0].pct;
                else
                    hpct = teams[(lloop * 3) + 2][0].pct;
            for (hti = divloop = 0; divloop < league_setup.numdivs; divloop++)
                /* find number of remaining teams with the same highest W/L percentage in league */
                for (x = 0; x < nteams[(lloop * 3) + divloop]; x++)
                    if (teams[(lloop * 3) + divloop][x].pct == hpct) {
                        holdteams[hti].id = teams[(lloop * 3) + divloop][x].id;
                        holdteams[hti].yr = teams[(lloop * 3) + divloop][x].year;
                        holdteams[hti].pct = teams[(lloop * 3) + divloop][x].pct;
                        hti++;
                    }

            if (hti > 1) {
                /* playoff game(s) to determine wild card team */
                if (hti == 2) {
                    /* flip a coin for home field */
                    int tw, tl, dw, dl;

                    if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                        teamvi = holdteams[0].id;
                        teamviyr = holdteams[0].yr;
                        teamhi = holdteams[1].id;
                        teamhiyr = holdteams[1].yr;
                    }
                    else {
                        teamvi = holdteams[1].id;
                        teamviyr = holdteams[1].yr;
                        teamhi = holdteams[0].id;
                        teamhiyr = holdteams[0].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* update work area and re-sort */
                    for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                        for (x = 0; x < 300; x++) {
                            if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                teams[(lloop * 3) + divloop][x].wins++;
                                tw = x;
                                dw = divloop;
                            }
                            if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                teams[(lloop * 3) + divloop][x].losses++;
                                tl = x;
                                dl = divloop;
                            }
                        }
                    teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                      (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                    teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                      (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    holdteams[0].id = win_id;
                    holdteams[0].yr = win_year;
                    holdteams[0].pct = teams[(lloop * 3) + dw][tw].pct;

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "h");
                    else
                        strcpy (&buf[0], "1");
                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti == 3) {
                    /* 3 teams tied for the wild card - 2 playoff games */
                    int holdwid = 0, holdwyr = 0, holdlid = 0, holdlyr = 0, tw, tl, dw, dl;

                    x = (int) ((float) 6 * rand () / (RAND_MAX + 1.0));
                    switch (x) {
                        /* if x == 0 teams stay as they are */
                        case 1:
                            ht = holdteams[1];
                            holdteams[1] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                        case 2:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[1];
                            holdteams[1] = ht;
                            break;
                        case 3:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[1];
                            holdteams[1] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                        case 4:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[2];
                            holdteams[2] = holdteams[1];
                            holdteams[1] = ht;
                            break;
                        case 5:
                            ht = holdteams[0];
                            holdteams[0] = holdteams[2];
                            holdteams[2] = ht;
                            break;
                    }
                    teamvi = holdteams[1].id;
                    teamviyr = holdteams[1].yr;
                    teamhi = holdteams[0].id;
                    teamhiyr = holdteams[0].yr;
                    if (!PlayoffGame ())
                        return 0;

                    holdwid = win_id;
                    holdwyr = win_year;
                    holdlid = lose_id;
                    holdlyr = lose_year;
                    /* update work area and re-sort */
                    for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                        for (x = 0; x < 300; x++) {
                            if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                teams[(lloop * 3) + divloop][x].wins++;
                                tw = x;
                                dw = divloop;
                            }
                            if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                teams[(lloop * 3) + divloop][x].losses++;
                                tl = x;
                                dl = divloop;
                            }
                        }
                    teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                      (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                    teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                      (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    teamvi = holdteams[2].id;
                    teamviyr = holdteams[2].yr;
                    if (win_id == home.id && win_year == home.year) {
                        teamhi = holdteams[0].id;
                        teamhiyr = holdteams[0].yr;
                    }
                    else {
                        teamhi = holdteams[1].id;
                        teamhiyr = holdteams[1].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* update work area and re-sort */
                    for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                        for (x = 0; x < 300; x++) {
                            if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                teams[(lloop * 3) + divloop][x].wins++;
                                tw = x;
                                dw = divloop;
                            }
                            if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                teams[(lloop * 3) + divloop][x].losses++;
                                tl = x;
                                dl = divloop;
                            }
                        }
                    teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                      (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                    teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                      (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    holdteams[0].id = win_id;
                    holdteams[0].yr = win_year;
                    holdteams[0].pct = teams[(lloop * 3) + dw][tw].pct;

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "i");
                    else
                        strcpy (&buf[0], "2");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr));
                    if (holdwid >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr));
                    if (holdlid >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid));
                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti == 4) {
                    /* 4 teams tied for the WC - 3 playoff games */
                    int holdwid[2], holdwyr[2], holdlid[2], holdlyr[2], tw, tl, dw, dl;

                    for (x = 0; x < 4; x++)
                        ht4[x] = holdteams[x];
                    for (y = 0, z = 4; z > 0; z--, y++) {
                        x = (int) ((float) z * rand () / (RAND_MAX + 1.0));
                        holdteams[y] = ht4[x];
                        for (; x < 4; x++)
                            ht4[x] = ht4[x + 1];
                    }
                    for (x = 0; x < 3; x += 2) {
                        teamvi = holdteams[x + 1].id;
                        teamviyr = holdteams[x + 1].yr;
                        teamhi = holdteams[x].id;
                        teamhiyr = holdteams[x].yr;
                        if (!PlayoffGame ())
                            return 0;

                        /* update work area and re-sort */
                        for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                            for (x = 0; x < 300; x++) {
                                if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                    teams[(lloop * 3) + divloop][x].wins++;
                                    tw = x;
                                    dw = divloop;
                                }
                                if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                    teams[(lloop * 3) + divloop][x].losses++;
                                    tl = x;
                                    dl = divloop;
                                }
                        }
                        teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                          (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                        teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                          (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                        for (loop = 0; loop < 6; loop++)
                            if (nteams[loop])
                                for (x = 0; x < (nteams[loop] - 1); x++)
                                    for (y = x + 1; y < nteams[loop]; y++)
                                        if (teams[loop][x].pct < teams[loop][y].pct) {
                                            teamh = teams[loop][x];
                                            teams[loop][x] = teams[loop][y];
                                            teams[loop][y] = teamh;
                                        }

                        ht4[x].id = win_id;
                        ht4[x].yr = win_year;
                        if (!x) {
                            holdwid[0] = win_id;
                            holdwyr[0] = win_year;
                            holdlid[0] = lose_id;
                            holdlyr[0] = lose_year;
                        }
                        else {
                            holdwid[1] = win_id;
                            holdwyr[1] = win_year;
                            holdlid[1] = lose_id;
                            holdlyr[1] = lose_year;
                        }
                    }
                    if ((int) ((float) 2 * rand () / (RAND_MAX + 1.0))) {
                        teamvi = ht4[0].id;
                        teamviyr = ht4[0].yr;
                        teamhi = ht4[2].id;
                        teamhiyr = ht4[2].yr;
                    }
                    else {
                        teamvi = ht4[2].id;
                        teamviyr = ht4[2].yr;
                        teamhi = ht4[0].id;
                        teamhiyr = ht4[0].yr;
                    }
                    if (!PlayoffGame ())
                        return 0;

                    /* update work area and re-sort */
                    for (divloop = 0; divloop < league_setup.numdivs; divloop++)
                        for (x = 0; x < 300; x++) {
                            if (teams[(lloop * 3) + divloop][x].id == win_id && teams[(lloop * 3) + divloop][x].year == win_year) {
                                teams[(lloop * 3) + divloop][x].wins++;
                                tw = x;
                                dw = divloop;
                            }
                            if (teams[(lloop * 3) + divloop][x].id == lose_id && teams[(lloop * 3) + divloop][x].year == lose_year) {
                                teams[(lloop * 3) + divloop][x].losses++;
                                tl = x;
                                dl = divloop;
                            }
                        }
                    teams[(lloop * 3) + dw][tw].pct = (float) teams[(lloop * 3) + dw][tw].wins /
                                                      (float) (teams[(lloop * 3) + dw][tw].wins + teams[(lloop * 3) + dw][tw].losses);
                    teams[(lloop * 3) + dl][tl].pct = (float) teams[(lloop * 3) + dl][tl].wins /
                                                      (float) (teams[(lloop * 3) + dl][tl].wins + teams[(lloop * 3) + dl][tl].losses);
                    for (loop = 0; loop < 6; loop++)
                        if (nteams[loop])
                            for (x = 0; x < (nteams[loop] - 1); x++)
                                for (y = x + 1; y < nteams[loop]; y++)
                                    if (teams[loop][x].pct < teams[loop][y].pct) {
                                        teamh = teams[loop][x];
                                        teams[loop][x] = teams[loop][y];
                                        teams[loop][y] = teamh;
                                    }

                    holdteams[0].id = win_id;
                    holdteams[0].yr = win_year;
                    holdteams[0].pct = teams[(lloop * 3) + dw][tw].pct;

                    /* save playoff results */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "j");
                    else
                        strcpy (&buf[0], "3");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[0]));
                    if (holdwid[0] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid[0]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[0]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[0]));
                    if (holdlid[0] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid[0]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[0]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdwyr[1]));
                    if (holdwid[1] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdwid[1]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdwid[1]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdlyr[1]));
                    if (holdlid[1] >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdlid[1]));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdlid[1]));
                    strcat (&buf[0], (char *) cnvt_int2str (4, win_year));
                    if (win_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (win_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, win_id));
                    strcat (&buf[0], (char *) cnvt_int2str (4, lose_year));
                    if (lose_id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (lose_id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, lose_id));
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
                if (hti > 4) {
                    /* more than 4 teams tied for the Division - determine the winning team via a coin flip */
                    x = (int) ((float) hti * rand () / (RAND_MAX + 1.0));
                    if (!lloop) {
                        if (!divloop) {
                            ALEDteamID = holdteams[x].id;
                            ALEDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 1) {
                            ALCDteamID = holdteams[x].id;
                            ALCDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 2) {
                            ALWDteamID = holdteams[x].id;
                            ALWDteamYR = holdteams[x].yr;
                        }
                    }
                    else {
                        if (!divloop) {
                            NLEDteamID = holdteams[x].id;
                            NLEDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 1) {
                            NLCDteamID = holdteams[x].id;
                            NLCDteamYR = holdteams[x].yr;
                        }
                        if (divloop == 2) {
                            NLWDteamID = holdteams[x].id;
                            NLWDteamYR = holdteams[x].yr;
                        }
                    }

                    /* save results of the tied teams */
                    if (league_setup.numleagues == 2 && league_setup.numdivs == 1 && !league_setup.numwc)
                        strcpy (&buf[0], "k");
                    else
                        strcpy (&buf[0], "4");
                    strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                    if (holdteams[x].id >= 900) {
                        /* user-created team */
                        strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                        strcat (&buf[0], " ");
                    }
                    else
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                    for (x = 0; x < hti; x++) {
                        strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].yr));
                        if (holdteams[x].id >= 900) {
                            /* user-created team */
                            strcat (&buf[0], GetUCTeamname (holdteams[x].id));
                            strcat (&buf[0], " ");
                        }
                        else
                            strcat (&buf[0], (char *) cnvt_int2str (4, holdteams[x].id));
                    }
                    strcat (&buf[0], "::");
                    strcpy (&dummy[0], "/var/NSB/");
                    strcat (&dummy[0], &nsbdb[user].id[0]);
                    strcat (&dummy[0], "/PlayoffResults");
                    if (!lloop)
                        strcat (&dummy[0], "AL");
                    else
                        strcat (&dummy[0], "NL");
                    /* if the file already exists then append (if the string doesn't already exist) */
                    if ((in = fopen (dummy, "r")) != NULL) {
                        fread (&buf1, sizeof buf1, 1, in);
                        fclose (in);
                        if (!strstr (&buf1[0], &buf[0])) {
                            strcat (&buf1[0], &buf[0]);
                            strcpy (&buf[0], &buf1[0]);

                            if ((out = fopen (dummy, "w")) != NULL) {
                                fwrite (&buf[0], sizeof buf, 1, out);
                                fclose (out);
                            }
                            else
                                closeup ();
                        }
                    }
                    else
                        if ((out = fopen (dummy, "w")) != NULL) {
                            fwrite (&buf[0], sizeof buf, 1, out);
                            fclose (out);
                        }
                        else
                            closeup ();
                }
            }

            /* save wild card team */
            if (league_setup.numwc == 2) {
                pteams[(lloop * 5) + 4].id = holdteams[0].id;
                pteams[(lloop * 5) + 4].yr = holdteams[0].yr;
                pteams[(lloop * 5) + 4].pct = holdteams[0].pct;
            }
            else {
                pteams[(lloop * 5) + 3].id = holdteams[0].id;
                pteams[(lloop * 5) + 3].yr = holdteams[0].yr;
                pteams[(lloop * 5) + 3].pct = holdteams[0].pct;
            }
        }

BuildPSSchedule:
    /* build post-season schedule

       the 4 (max) rounds of the post-season always occur in the same iterations of schedule[]
       i.e., round 1 - iteration 0-8
             round 2 - iteration 9-17
             round 3 - iteration 18-26
             round 4 - iteration 27-35
       the DH indicator occurs in iteration 36
       any game lines which do not occur (e.g., lines 8 and 9 in a 7-game series) will not contain any team ids nor years
    */

    /* load regular season schedule in order to get DH indicator */
    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Schedule");
    if ((in = fopen (dummy, "r")) != NULL) {
        for (x = 0; x < 244; x++)
            fgets (&schedule[x][0], 3000, in);
        fclose (in);
        dhcode = schedule[243][0] - '0';
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
        return -1;
    }
    for (x = 0; x < 36; x++) {
        strcpy (&schedule[x][0], (char *) cnvt_int2str (3, x + 1));
        strcat (&schedule[x][0], " ");
    }
    schedule[36][0] = dhcode + '0';
    schedule[36][1] = '\0';

    if (!league_setup.nummaxgames[0]) {
        /* no post-season */
        for (x = 0; x < 36; x++)
            strcpy (&schedule[x][3], "X00000000-00000000");
        goto WritePSSchedule;
    }

    if (league_setup.numwc == 2) {
        /* first round of PS with 2 WC teams */
        if (league_setup.numdivs != 2) {
            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                int z;

                /* the two WC teams play in the first round
                   the WC team with the better record has the home field advantage */
                if (pteams[3].pct > pteams[4].pct)
                    z = 1;
                else
                    z = 0;

                if (!(x % 2))  /* alternate home team */
                    if (z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].id));
                    }
                else
                    if (!z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[4].id));
                    }

                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    int z;

                    if (pteams[8].pct > pteams[9].pct)
                        z = 1;
                    else
                        z = 0;

                    if (!(x % 2))  /* alternate home team */
                        if (z) {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].id));
                        }
                    else
                        if (!z) {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[9].id));
                        }
                    strcat (&schedule[x][0], "N");
                }
            }
            for (x = 9; x < (league_setup.nummaxgames[1] + 9); x++)
                /* second round of PS with 2 WC teams and 1 or 3 divisions */
                if (league_setup.numdivs == 1) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], "00000000-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                        strcat (&schedule[x][0], "-00000000");
                    }
                    strcat (&schedule[x][0], "A");
                    if (league_setup.numleagues == 2) {
                        if (!(x % 2)) {  /* alternate home team */
                            strcat (&schedule[x][0], "00000000-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                            strcat (&schedule[x][0], "-00000000");
                        }
                        strcat (&schedule[x][0], "N");
                    }
                }
                else {
                    /* 2 divisions will never get here */
                    struct {
                        int id, yr;
                    } torder[3][2];

                    /* the division winner with the best record plays the WC winner and has the home field advantage
                       the other two division winners play with the team with the better record having the home field advantage */
                    if (pteams[0].pct >= pteams[1].pct && pteams[0].pct >= pteams[2].pct) {
                        torder[0][0].id = pteams[0].id;
                        torder[0][0].yr = pteams[0].yr;
                        if (pteams[1].pct > pteams[2].pct) {
                            torder[1][0].id = pteams[1].id;
                            torder[1][0].yr = pteams[1].yr;
                            torder[2][0].id = pteams[2].id;
                            torder[2][0].yr = pteams[2].yr;
                        }
                        else {
                            torder[1][0].id = pteams[2].id;
                            torder[1][0].yr = pteams[2].yr;
                            torder[2][0].id = pteams[1].id;
                            torder[2][0].yr = pteams[1].yr;
                        }
                    }
                    else
                        if (pteams[1].pct >= pteams[0].pct && pteams[1].pct >= pteams[2].pct) {
                            torder[0][0].id = pteams[1].id;
                            torder[0][0].yr = pteams[1].yr;
                            if (pteams[0].pct > pteams[2].pct) {
                                torder[1][0].id = pteams[0].id;
                                torder[1][0].yr = pteams[0].yr;
                                torder[2][0].id = pteams[2].id;
                                torder[2][0].yr = pteams[2].yr;
                            }
                            else {
                                torder[1][0].id = pteams[2].id;
                                torder[1][0].yr = pteams[2].yr;
                                torder[2][0].id = pteams[0].id;
                                torder[2][0].yr = pteams[0].yr;
                            }
                        }
                        else {
                            torder[0][0].id = pteams[2].id;
                            torder[0][0].yr = pteams[2].yr;
                            if (pteams[0].pct > pteams[1].pct) {
                                torder[1][0].id = pteams[0].id;
                                torder[1][0].yr = pteams[0].yr;
                                torder[2][0].id = pteams[1].id;
                                torder[2][0].yr = pteams[1].yr;
                            }
                            else {
                                torder[1][0].id = pteams[1].id;
                                torder[1][0].yr = pteams[1].yr;
                                torder[2][0].id = pteams[0].id;
                                torder[2][0].yr = pteams[0].yr;
                            }
                        }
                    if (league_setup.numleagues == 2) {
                        if (pteams[5].pct >= pteams[6].pct && pteams[5].pct >= pteams[7].pct) {
                            torder[0][1].id = pteams[5].id;
                            torder[0][1].yr = pteams[5].yr;
                            if (pteams[6].pct > pteams[7].pct) {
                                torder[1][1].id = pteams[6].id;
                                torder[1][1].yr = pteams[6].yr;
                                torder[2][1].id = pteams[7].id;
                                torder[2][1].yr = pteams[7].yr;
                            }
                            else {
                                torder[1][1].id = pteams[7].id;
                                torder[1][1].yr = pteams[7].yr;
                                torder[2][1].id = pteams[6].id;
                                torder[2][1].yr = pteams[6].yr;
                            }
                        }
                        else
                            if (pteams[6].pct >= pteams[5].pct && pteams[6].pct >= pteams[7].pct) {
                                torder[0][1].id = pteams[6].id;
                                torder[0][1].yr = pteams[6].yr;
                                if (pteams[5].pct > pteams[7].pct) {
                                    torder[1][1].id = pteams[5].id;
                                    torder[1][1].yr = pteams[5].yr;
                                    torder[2][1].id = pteams[7].id;
                                    torder[2][1].yr = pteams[7].yr;
                                }
                                else {
                                    torder[1][1].id = pteams[7].id;
                                    torder[1][1].yr = pteams[7].yr;
                                    torder[2][1].id = pteams[5].id;
                                    torder[2][1].yr = pteams[5].yr;
                                }
                            }
                            else {
                                torder[0][1].id = pteams[7].id;
                                torder[0][1].yr = pteams[7].yr;
                                if (pteams[5].pct > pteams[6].pct) {
                                    torder[1][1].id = pteams[5].id;
                                    torder[1][1].yr = pteams[5].yr;
                                    torder[2][1].id = pteams[6].id;
                                    torder[2][1].yr = pteams[6].yr;
                                }
                                else {
                                    torder[1][1].id = pteams[6].id;
                                    torder[1][1].yr = pteams[6].yr;
                                    torder[2][1].id = pteams[5].id;
                                    torder[2][1].yr = pteams[5].yr;
                                }
                            }
                    }
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], "00000000-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                        strcat (&schedule[x][0], "-00000000");
                    }
                    strcat (&schedule[x][0], "A");
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                    }
                    strcat (&schedule[x][0], "A");

                    if (league_setup.numleagues == 2) {
                        if (!(x % 2)) {  /* alternate home team */
                            strcat (&schedule[x][0], "00000000-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                            strcat (&schedule[x][0], "-00000000");
                        }
                        strcat (&schedule[x][0], "N");
                        if (!(x % 2)) {  /* alternate home team */
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                        }
                        strcat (&schedule[x][0], "N");
                    }
                }
        }
        else {
            /* in the first round the division winner with the better record plays the WC team with the worse record and
               the division winner with the worse record plays the WC team with the better record
 
               the division winner always has the home field advantage */
            int wc[4], dl[4];

            if (pteams[3].pct > pteams[4].pct) {
                wc[0] = 3;
                wc[1] = 4;
            }
            else {
                wc[0] = 4;
                wc[1] = 3;
            }
            if (pteams[0].pct > pteams[1].pct) {
                dl[0] = 0;
                dl[1] = 1;
            }
            else {
                dl[0] = 1;
                dl[1] = 0;
            }
            if (league_setup.numleagues == 2) {
                if (pteams[8].pct > pteams[9].pct) {
                    wc[2] = 8;
                    wc[3] = 9;
                }
                else {
                    wc[2] = 9;
                    wc[3] = 8;
                }
                if (pteams[5].pct > pteams[6].pct) {
                    dl[2] = 5;
                    dl[3] = 6;
                }
                else {
                    dl[2] = 6;
                    dl[3] = 5;
                }
            }

            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[0]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[0]].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[1]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[1]].id));
                    strcat (&schedule[x][0], "A");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[1]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[1]].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[0]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[0]].id));
                    strcat (&schedule[x][0], "A");
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[1]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[1]].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[0]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[0]].id));
                    strcat (&schedule[x][0], "A");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[0]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[0]].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[1]].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[1]].id));
                    strcat (&schedule[x][0], "A");
                }

                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[2]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[2]].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[3]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[3]].id));
                        strcat (&schedule[x][0], "N");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[3]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[3]].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[2]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[2]].id));
                        strcat (&schedule[x][0], "N");
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[3]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[3]].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[2]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[2]].id));
                        strcat (&schedule[x][0], "N");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[2]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[dl[2]].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[3]].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[wc[3]].id));
                        strcat (&schedule[x][0], "N");
                    }
                }
            }
        }
    }
    if (league_setup.numwc == 1) {
        if (league_setup.numdivs == 1) {
            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                }
                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                    }
                    strcat (&schedule[x][0], "N");
                }
            }
        }
        if (league_setup.numdivs == 2) {
            /* in the first round the WC team will play the division winner with the worse record */
            int y, z, y2, z2;

            if (pteams[0].pct > pteams[1].pct) {
                z = 0;
                y = 1;
            }
            else {
                z = 1;
                y = 0;
            }
            if (league_setup.numleagues == 2) {
                if (pteams[5].pct > pteams[6].pct) {
                    z2 = 5;
                    y2 = 6;
                }
                else {
                    z2 = 6;
                    y2 = 5;
                }
            }
            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                }
                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y2].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y2].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y2].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[y2].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                    }
                    strcat (&schedule[x][0], "N");
                }
            }

            for (x = 9; x < (league_setup.nummaxgames[1] + 9); x++) {
                /* second round of PS with 1 WC team and 2 divisions */
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], "00000000-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z].id));
                    strcat (&schedule[x][0], "-00000000");
                }
                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], "00000000-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z2].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z2].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z2].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[z2].id));
                        strcat (&schedule[x][0], "-00000000");
                    }
                    strcat (&schedule[x][0], "N");
                }
            }
        }
        if (league_setup.numdivs == 3) {
            /* in the first round the WC team will play the division winner with the best record */
            struct {
                int id, yr;
            } torder[3][2];

            /* the division winner with the best record plays the WC winner and has the home field advantage
               the other two division winners play with the team with the better record having the home field advantage */
            if (pteams[0].pct >= pteams[1].pct && pteams[0].pct >= pteams[2].pct) {
                torder[0][0].id = pteams[0].id;
                torder[0][0].yr = pteams[0].yr;
                if (pteams[1].pct > pteams[2].pct) {
                    torder[1][0].id = pteams[1].id;
                    torder[1][0].yr = pteams[1].yr;
                    torder[2][0].id = pteams[2].id;
                    torder[2][0].yr = pteams[2].yr;
                }
                else {
                    torder[1][0].id = pteams[2].id;
                    torder[1][0].yr = pteams[2].yr;
                    torder[2][0].id = pteams[1].id;
                    torder[2][0].yr = pteams[1].yr;
                }
            }
            else
                if (pteams[1].pct >= pteams[0].pct && pteams[1].pct >= pteams[2].pct) {
                    torder[0][0].id = pteams[1].id;
                    torder[0][0].yr = pteams[1].yr;
                    if (pteams[0].pct > pteams[2].pct) {
                        torder[1][0].id = pteams[0].id;
                        torder[1][0].yr = pteams[0].yr;
                        torder[2][0].id = pteams[2].id;
                        torder[2][0].yr = pteams[2].yr;
                    }
                    else {
                        torder[1][0].id = pteams[2].id;
                        torder[1][0].yr = pteams[2].yr;
                        torder[2][0].id = pteams[0].id;
                        torder[2][0].yr = pteams[0].yr;
                    }
                }
                else {
                    torder[0][0].id = pteams[2].id;
                    torder[0][0].yr = pteams[2].yr;
                    if (pteams[0].pct > pteams[1].pct) {
                        torder[1][0].id = pteams[0].id;
                        torder[1][0].yr = pteams[0].yr;
                        torder[2][0].id = pteams[1].id;
                        torder[2][0].yr = pteams[1].yr;
                    }
                    else {
                        torder[1][0].id = pteams[1].id;
                        torder[1][0].yr = pteams[1].yr;
                        torder[2][0].id = pteams[0].id;
                        torder[2][0].yr = pteams[0].yr;
                    }
                }
            if (league_setup.numleagues == 2) {
                if (pteams[5].pct >= pteams[6].pct && pteams[5].pct >= pteams[7].pct) {
                    torder[0][1].id = pteams[5].id;
                    torder[0][1].yr = pteams[5].yr;
                    if (pteams[6].pct > pteams[7].pct) {
                        torder[1][1].id = pteams[6].id;
                        torder[1][1].yr = pteams[6].yr;
                        torder[2][1].id = pteams[7].id;
                        torder[2][1].yr = pteams[7].yr;
                    }
                    else {
                        torder[1][1].id = pteams[7].id;
                        torder[1][1].yr = pteams[7].yr;
                        torder[2][1].id = pteams[6].id;
                        torder[2][1].yr = pteams[6].yr;
                    }
                }
                else
                    if (pteams[6].pct >= pteams[5].pct && pteams[6].pct >= pteams[7].pct) {
                        torder[0][1].id = pteams[6].id;
                        torder[0][1].yr = pteams[6].yr;
                        if (pteams[5].pct > pteams[7].pct) {
                            torder[1][1].id = pteams[5].id;
                            torder[1][1].yr = pteams[5].yr;
                            torder[2][1].id = pteams[7].id;
                            torder[2][1].yr = pteams[7].yr;
                        }
                        else {
                            torder[1][1].id = pteams[7].id;
                            torder[1][1].yr = pteams[7].yr;
                            torder[2][1].id = pteams[5].id;
                            torder[2][1].yr = pteams[5].yr;
                        }
                    }
                    else {
                        torder[0][1].id = pteams[7].id;
                        torder[0][1].yr = pteams[7].yr;
                        if (pteams[5].pct > pteams[6].pct) {
                            torder[1][1].id = pteams[5].id;
                            torder[1][1].yr = pteams[5].yr;
                            torder[2][1].id = pteams[6].id;
                            torder[2][1].yr = pteams[6].yr;
                        }
                        else {
                            torder[1][1].id = pteams[6].id;
                            torder[1][1].yr = pteams[6].yr;
                            torder[2][1].id = pteams[5].id;
                            torder[2][1].yr = pteams[5].yr;
                        }
                    }
            }

            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[3].id));
                }
                strcat (&schedule[x][0], "A");
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                }
                strcat (&schedule[x][0], "A");

                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[8].id));
                    }
                    strcat (&schedule[x][0], "N");
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                    }
                    strcat (&schedule[x][0], "N");
                }
            }
        }
    }

    if (!league_setup.numwc) {
        /* if there are no WC teams then there cannot be only 1 division with only 1 league (checked at setup time) */
        if (league_setup.numdivs == 1) {
            int z;

            /* the two league winners play in the only round
               the league winner with the better record has the home field advantage */
            if (pteams[0].pct > pteams[5].pct)
                z = 1;
            else
                z = 0;

            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2))  /* alternate home team */
                    if (z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                    }
                else
                    if (!z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                    }
                strcat (&schedule[x][0], " ");
            }
        }

        if (league_setup.numdivs == 2) {
            int z;

            /* the two division winners play in the first round
               the division winner with the better record has the home field advantage */
            if (pteams[0].pct > pteams[1].pct)
                z = 1;
            else
                z = 0;

            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2))  /* alternate home team */
                    if (z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].id));
                    }
                else
                    if (!z) {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[0].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[1].id));
                    }

                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    int z;

                    if (pteams[5].pct > pteams[6].pct)
                        z = 1;
                    else
                        z = 0;

                    if (!(x % 2))  /* alternate home team */
                        if (z) {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].id));
                        }
                    else
                        if (!z) {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                        }
                        else {
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[5].id));
                            strcat (&schedule[x][0], "-");
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].yr));
                            strcat (&schedule[x][0], (char *) cnvt_int2str (4, pteams[6].id));
                        }
                    strcat (&schedule[x][0], "N");
                }
            }
        }
        if (league_setup.numdivs == 3) {
            struct {
                int id, yr;
            } torder[3][2];

            /* the division winner with the best record gets a bye
               the other two division winners play with the team with the better record having the home field advantage */
            if (pteams[0].pct >= pteams[1].pct && pteams[0].pct >= pteams[2].pct) {
                torder[0][0].id = pteams[0].id;
                torder[0][0].yr = pteams[0].yr;
                if (pteams[1].pct > pteams[2].pct) {
                    torder[1][0].id = pteams[1].id;
                    torder[1][0].yr = pteams[1].yr;
                    torder[2][0].id = pteams[2].id;
                    torder[2][0].yr = pteams[2].yr;
                }
                else {
                    torder[1][0].id = pteams[2].id;
                    torder[1][0].yr = pteams[2].yr;
                    torder[2][0].id = pteams[1].id;
                    torder[2][0].yr = pteams[1].yr;
                }
            }
            else
                if (pteams[1].pct >= pteams[0].pct && pteams[1].pct >= pteams[2].pct) {
                    torder[0][0].id = pteams[1].id;
                    torder[0][0].yr = pteams[1].yr;
                    if (pteams[0].pct > pteams[2].pct) {
                        torder[1][0].id = pteams[0].id;
                        torder[1][0].yr = pteams[0].yr;
                        torder[2][0].id = pteams[2].id;
                        torder[2][0].yr = pteams[2].yr;
                    }
                    else {
                        torder[1][0].id = pteams[2].id;
                        torder[1][0].yr = pteams[2].yr;
                        torder[2][0].id = pteams[0].id;
                        torder[2][0].yr = pteams[0].yr;
                    }
                }
                else {
                    torder[0][0].id = pteams[2].id;
                    torder[0][0].yr = pteams[2].yr;
                    if (pteams[0].pct > pteams[1].pct) {
                        torder[1][0].id = pteams[0].id;
                        torder[1][0].yr = pteams[0].yr;
                        torder[2][0].id = pteams[1].id;
                        torder[2][0].yr = pteams[1].yr;
                    }
                    else {
                        torder[1][0].id = pteams[1].id;
                        torder[1][0].yr = pteams[1].yr;
                        torder[2][0].id = pteams[0].id;
                        torder[2][0].yr = pteams[0].yr;
                    }
                }
            if (league_setup.numleagues == 2) {
                if (pteams[5].pct >= pteams[6].pct && pteams[5].pct >= pteams[7].pct) {
                    torder[0][1].id = pteams[5].id;
                    torder[0][1].yr = pteams[5].yr;
                    if (pteams[1].pct > pteams[7].pct) {
                        torder[1][1].id = pteams[6].id;
                        torder[1][1].yr = pteams[6].yr;
                        torder[2][1].id = pteams[7].id;
                        torder[2][1].yr = pteams[7].yr;
                    }
                    else {
                        torder[1][1].id = pteams[7].id;
                        torder[1][1].yr = pteams[7].yr;
                        torder[2][1].id = pteams[6].id;
                        torder[2][1].yr = pteams[6].yr;
                    }
                }
                else
                    if (pteams[6].pct >= pteams[5].pct && pteams[6].pct >= pteams[7].pct) {
                        torder[0][1].id = pteams[6].id;
                        torder[0][1].yr = pteams[6].yr;
                        if (pteams[0].pct > pteams[7].pct) {
                            torder[1][1].id = pteams[5].id;
                            torder[1][1].yr = pteams[5].yr;
                            torder[2][1].id = pteams[7].id;
                            torder[2][1].yr = pteams[7].yr;
                        }
                        else {
                            torder[1][1].id = pteams[7].id;
                            torder[1][1].yr = pteams[7].yr;
                            torder[2][1].id = pteams[6].id;
                            torder[2][1].yr = pteams[6].yr;
                        }
                    }
                    else {
                        torder[0][1].id = pteams[7].id;
                        torder[0][1].yr = pteams[7].yr;
                        if (pteams[0].pct > pteams[6].pct) {
                            torder[1][1].id = pteams[5].id;
                            torder[1][1].yr = pteams[5].yr;
                            torder[2][1].id = pteams[6].id;
                            torder[2][1].yr = pteams[6].yr;
                        }
                        else {
                            torder[1][1].id = pteams[6].id;
                            torder[1][1].yr = pteams[6].yr;
                            torder[2][1].id = pteams[5].id;
                            torder[2][1].yr = pteams[5].yr;
                        }
                    }
            }
            for (x = 0; x < league_setup.nummaxgames[0]; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][0].id));
                    strcat (&schedule[x][0], "-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][0].id));
                }
                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[1][1].id));
                        strcat (&schedule[x][0], "-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[2][1].id));
                    }
                    strcat (&schedule[x][0], "N");
                }
            }
            /* the winning team in the first round plays the division winner with the best record in the second round */
            for (x = 9; x < league_setup.nummaxgames[1] + 9; x++) {
                if (!(x % 2)) {  /* alternate home team */
                    strcat (&schedule[x][0], "00000000-");
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                }
                else {
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].yr));
                    strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][0].id));
                    strcat (&schedule[x][0], "-00000000");
                }
                strcat (&schedule[x][0], "A");
                if (league_setup.numleagues == 2) {
                    if (!(x % 2)) {  /* alternate home team */
                        strcat (&schedule[x][0], "00000000-");
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                    }
                    else {
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].yr));
                        strcat (&schedule[x][0], (char *) cnvt_int2str (4, torder[0][1].id));
                        strcat (&schedule[x][0], "-00000000");
                    }
                    strcat (&schedule[x][0], "N");
                }
            }
        }
    }
WritePSSchedule:
    for (x = 0; x < 37; x++)
        strcat (&schedule[x][0], "\n");

    /* X out unplayed rounds in the post-season */
    for (x = 0; x < 4; x++)
        for (y = league_setup.nummaxgames[x]; y < 9; y++)
            schedule[x * 9 + y][3] = 'X';

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Schedule-PS");
    if ((out = fopen (dummy, "w")) != NULL) {
        for (x = 0; x < 37; x++)
            fwrite (&schedule[x][0], strlen (&schedule[x][0]), 1, out);
        fclose (out);
    }
    else {
        if (syslog_ent == YES)
            syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
        return -1;
    }

    if (!league_setup.nummaxgames[0]) {
        /* no post-season */
        kill_league ();
        return 0;
    }

    /* create zeroed out post-season stats for each team */
    for (x = 0; x < 10; x++) {
        char parent2[256];

        if (!pteams[x].id)
            continue;

        strcpy (&parent2[0], "/var/NSB/");
        strcat (&parent2[0], &nsbdb[user].id[0]);

        /* load the NSB user's team files rather than the Real Life files because the league and division could be different */
        strcpy (&dummy[0], "/var/NSB/");
        strcat (&dummy[0], &nsbdb[user].id[0]);
        strcat (&dummy[0], "/");
        if (pteams[x].id < 900) {
            strcat (&dummy[0], (char *) cnvt_int2str (4, pteams[x].yr));
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                 if (teaminfo[y].id == pteams[x].id)
                     strcat (&dummy[0], &teaminfo[y].filename[0]);
        }
        else
            strcat (&dummy[0], GetUCTeamname (pteams[x].id));

        if ((in = fopen (dummy, "r")) != NULL) {
            fread (&team.id, sizeof team.id, 1, in);
            fread (&team.year, sizeof team.year, 1, in);
            fread (&team.league, sizeof team.league, 1, in);
            fread (&team.division, sizeof team.division, 1, in);
            for (z = 0; z < 25; z++) {
                fread (&team.batters[z].id, sizeof team.batters[z].id, 1, in);
                fread (&team.batters[z].dob, sizeof team.batters[z].dob, 1, in);
                fread (&team.batters[z].hitting, sizeof team.batters[z].hitting, 1, in);
                for (n = 0; n < 11; n++)
                    fread (&team.batters[z].fielding[n], sizeof team.batters[z].fielding[n], 1, in);
            }
            for (z = 0; z < 11; z++) {
                fread (&team.pitchers[z].id, sizeof team.pitchers[z].id, 1, in);
                fread (&team.pitchers[z].pitching, sizeof team.pitchers[z].pitching, 1, in);
            }
            fclose (in);
        }
        else {
            if (syslog_ent == YES)
                syslog (LOG_INFO, "couldn't open %s: %s", dummy, strerror (errno));
            return -1;
        }

        clear_stats ();

        strcat (&dummy[0], "-PS");

        if ((out = fopen (dummy, "w")) != NULL) {
            fwrite (&team.id, sizeof team.id, 1, out);
            fwrite (&team.year, sizeof team.year, 1, out);
            fwrite (&team.league, sizeof team.league, 1, out);
            fwrite (&team.division, sizeof team.division, 1, out);
            for (z = 0; z < 25; z++) {
                fwrite (&team.batters[z].id, sizeof team.batters[z].id, 1, out);
                fwrite (&team.batters[z].dob, sizeof team.batters[z].dob, 1, out);
                fwrite (&team.batters[z].hitting, sizeof team.batters[z].hitting, 1, out);
                for (n = 0; n < 11; n++)
                    fwrite (&team.batters[z].fielding[n], sizeof team.batters[z].fielding[n], 1, out);
            }
            for (z = 0; z < 11; z++) {
                fwrite (&team.pitchers[z].id, sizeof team.pitchers[z].id, 1, out);
                fwrite (&team.pitchers[z].pitching, sizeof team.pitchers[z].pitching, 1, out);
            }
            fclose (out);
        }
        else {
            if (syslog_ent == YES)
                syslog (LOG_INFO, "couldn't open %s for writing: %s", dummy, strerror (errno));
            return -1;
        }
    }

    return 0;
}

int
PlayoffGame () {
    /* all playoff games are treated as part of the regular season
       however, playoff games for the division title do not count against the losing team for Wild Card consideration */
    int x, z;
    FILE *out;

    if (play_league_game () == -1) {
        connected = 0;
        return 0;
    }
    /* update team wins data */
    if (winners == 'v') {
        if (visitor.year)
            win_id = visitor.id;
        else
            win_id = visitor_season.id;
        win_year = visitor.year;
        if (home.year)
            lose_id = home.id;
        else
            lose_id = home_season.id;
        lose_year = home.year;
    }
    else {
        if (home.year)
            win_id = home.id;
        else
            win_id = home_season.id;
        win_year = home.year;
        if (visitor.year)
            lose_id = visitor.id;
        else
            lose_id = visitor_season.id;
        lose_year = visitor.year;
    }
    for (z = 0; z < 300; z++)
        if (teamwins[z].id == win_id && teamwins[z].year == win_year)
            for (x = 0; x < 300; x++)
                if (teamwins[z].opp[x].id == lose_id && teamwins[z].opp[x].year == lose_year)
                    teamwins[z].opp[x].wins++;

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Standings");
    if ((out = fopen (dummy, "w")) != NULL) {
        fwrite (&teamwins[0], sizeof teamwins, 1, out);
        fclose (out);
    }
    else
        closeup ();

    update_records (999, teamvi, teamhi, NULL);
    return 1;
}

void
update_sch_ps () {
    int x, z, sday, sgame, it1, it2, iy1, iy2, wn[4], wy[4];
    char team1[5], team2[5], team1yr[5], team2yr[5];
    struct {
        int t1[2], t2[2], y1, y2;
    } mu[4];

    for (x = 0; x < 9; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 9)
        goto second_round;

    for (x = 0; x < 4; x++)
        mu[x].t1[0] = mu[x].t1[1] = mu[x].t2[0] = mu[x].t2[1] = 0;

    for (x = 0, sgame = 12; sgame < strlen (&schedule[0][0]); sgame += 18, x++) {
        for (z = 0; z < 4; z++) {
            team1[z] = schedule[0][sgame - 4 + z];
            team1yr[z] = schedule[0][sgame - 8 + z];
            team2[z] = schedule[0][sgame + 5 + z];
            team2yr[z] = schedule[0][sgame + 1 + z];
        }
        team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
        mu[x].t1[0] = atoi (&team1[0]);
        mu[x].y1 = atoi (&team1yr[0]);
        mu[x].t2[0] = atoi (&team2[0]);
        mu[x].y2 = atoi (&team2yr[0]);
    }

    for (sday = 0; sday < 9; sday++)
        for (x = 0, sgame = 12; sgame < strlen (&schedule[sday][0]); sgame += 18, x++) {
            if (schedule[sday][sgame] == 'X' || schedule[sday][sgame] == '-')
                continue;
            for (z = 0; z < 4; z++) {
                team1[z] = schedule[sday][sgame - 4 + z];
                team1yr[z] = schedule[sday][sgame - 8 + z];
                team2[z] = schedule[sday][sgame + 5 + z];
                team2yr[z] = schedule[sday][sgame + 1 + z];
            }
            team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
            it1 = atoi (&team1[0]);
            iy1 = atoi (&team1yr[0]);
            it2 = atoi (&team2[0]);
            iy2 = atoi (&team2yr[0]);

            if (schedule[sday][sgame] == 'V')
                if (it1 == mu[x].t1[0] && iy1 == mu[x].y1)
                    mu[x].t1[1]++;
                else
                    mu[x].t2[1]++;
            else
                if (it2 == mu[x].t2[0] && iy2 == mu[x].y2)
                    mu[x].t2[1]++;
                else
                    mu[x].t1[1]++;
        }

    for (x = 0, sgame = 12; x < 4; x++, sgame += 18)
        if (mu[x].t1[1] == (league_setup.nummaxgames[0] / 2 + 1) || mu[x].t2[1] == (league_setup.nummaxgames[0] / 2 + 1))
            for (z = 0; z < 9; z++)
                if (schedule[z][sgame] == '-')
                    schedule[z][sgame] = 'X';

    for (x = 0; x < 9; x++)
        if (!index (&schedule[x][3], '-'))
            schedule[x][3] = 'X';

    for (x = 0; x < 36; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 36)
        for (x = 0; x < 4; x++) {
            if (mu[x].t1[1] == (league_setup.nummaxgames[0] / 2 + 1)) {
                champs = mu[x].t1[0];
                champsyr = mu[x].y1;
                return;
            }
            if (mu[x].t2[1] == (league_setup.nummaxgames[0] / 2 + 1)) {
                champs = mu[x].t2[0];
                champsyr = mu[x].y2;
                return;
            }
        }

    for (x = 0; x < 9; x++)
        if (schedule[x][3] != 'X')
            return;

    /* all games in the first round of post-season play are completed but no world champ crowned yet */

    for (x = 0; x < 4; x++)
        wn[x] = 0;

    for (x = 0; x < 4; x++)
        if (mu[x].t1[1] == (league_setup.nummaxgames[0] / 2 + 1)) {
            wn[x] = mu[x].t1[0];
            wy[x] = mu[x].y1;
        }
        else {
            wn[x] = mu[x].t2[0];
            wy[x] = mu[x].y2;
        }

    if (strlen (&schedule[9][0]) < 5)
        for (x = 9; x < 18; x++)
            schedule[x][4] = '\0';
    for (x = 9; x < 18; x++)
        if (schedule[x][strlen (&schedule[x][0]) - 1] == '\n')
            schedule[x][strlen (&schedule[x][0]) - 1] = '\0';

    for (x = 0; x < 4; x += 2)
        if (wn[x]) {
            char *noteam;

            for (z = 9; z < 18; z++) {
                if (schedule[z][3] == 'X')
                    continue;
                noteam = strstr (&schedule[z][0], "00000000");
                if (noteam != NULL) {
                    strncpy (noteam, cnvt_int2str (4, wy[x]), 4);
                    strncpy (noteam + 4, cnvt_int2str (4, wn[x]), 4);

                    noteam = strstr (&schedule[z][0], "00000000");
                    if (noteam != NULL) {
                        strncpy (noteam, cnvt_int2str (4, wy[x + 1]), 4);
                        strncpy (noteam + 4, cnvt_int2str (4, wn[x + 1]), 4);
                    }
                }
                else {
                    if (z % 2) {
                        strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                        strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                        strcat (&schedule[z][0], "-");
                        strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                        strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                    }
                    else {
                        strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                        strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                        strcat (&schedule[z][0], "-");
                        strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                        strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                    }
                    if (!x)
                        strcat (&schedule[z][0], "A");
                    else
                        strcat (&schedule[z][0], "N");
                }
            }
        }
    for (z = 9; z < 18; z++)
        strcat (&schedule[z][0], "\n");
second_round:
    for (x = 9; x < 18; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 18)
        goto third_round;

    for (x = 0; x < 4; x++)
        mu[x].t1[0] = mu[x].t1[1] = mu[x].t2[0] = mu[x].t2[1] = 0;

    for (x = 0, sgame = 12; sgame < strlen (&schedule[9][0]); sgame += 18, x++) {
        for (z = 0; z < 4; z++) {
            team1[z] = schedule[9][sgame - 4 + z];
            team1yr[z] = schedule[9][sgame - 8 + z];
            team2[z] = schedule[9][sgame + 5 + z];
            team2yr[z] = schedule[9][sgame + 1 + z];
        }
        team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
        mu[x].t1[0] = atoi (&team1[0]);
        mu[x].y1 = atoi (&team1yr[0]);
        mu[x].t2[0] = atoi (&team2[0]);
        mu[x].y2 = atoi (&team2yr[0]);
    }

    for (sday = 9; sday < 18; sday++)
        for (x = 0, sgame = 12; sgame < strlen (&schedule[sday][0]); sgame += 18, x++) {
            if (schedule[sday][sgame] == 'X' || schedule[sday][sgame] == '-')
                continue;
            for (z = 0; z < 4; z++) {
                team1[z] = schedule[sday][sgame - 4 + z];
                team1yr[z] = schedule[sday][sgame - 8 + z];
                team2[z] = schedule[sday][sgame + 5 + z];
                team2yr[z] = schedule[sday][sgame + 1 + z];
            }
            team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
            it1 = atoi (&team1[0]);
            iy1 = atoi (&team1yr[0]);
            it2 = atoi (&team2[0]);
            iy2 = atoi (&team2yr[0]);

            if (schedule[sday][sgame] == 'V')
                if (it1 == mu[x].t1[0] && iy1 == mu[x].y1)
                    mu[x].t1[1]++;
                else
                    mu[x].t2[1]++;
            else
                if (it2 == mu[x].t2[0] && iy2 == mu[x].y2)
                    mu[x].t2[1]++;
                else
                    mu[x].t1[1]++;
        }

    for (x = 0, sgame = 12; x < 4; x++, sgame += 18)
        if (mu[x].t1[1] == (league_setup.nummaxgames[1] / 2 + 1) || mu[x].t2[1] == (league_setup.nummaxgames[1] / 2 + 1))
            for (z = 9; z < 18; z++)
                if (schedule[z][sgame] == '-')
                    schedule[z][sgame] = 'X';

    for (x = 9; x < 18; x++)
        if (!index (&schedule[x][3], '-'))
            schedule[x][3] = 'X';

    for (x = 0; x < 36; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 36)
        for (x = 0; x < 4; x++) {
            if (mu[x].t1[1] == (league_setup.nummaxgames[1] / 2 + 1)) {
                champs = mu[x].t1[0];
                champsyr = mu[x].y1;
                return;
            }
            if (mu[x].t2[1] == (league_setup.nummaxgames[1] / 2 + 1)) {
                champs = mu[x].t2[0];
                champsyr = mu[x].y2;
                return;
            }
        }

    for (x = 9; x < 18; x++)
        if (schedule[x][3] != 'X')
            return;

    /* all games in the second round of post-season play are completed but no world champ crowned yet */

    for (x = 0; x < 4; x++)
        wn[x] = 0;

    for (x = 0; x < 4; x++)
        if (mu[x].t1[1] == (league_setup.nummaxgames[1] / 2 + 1)) {
            wn[x] = mu[x].t1[0];
            wy[x] = mu[x].y1;
        }
        else {
            wn[x] = mu[x].t2[0];
            wy[x] = mu[x].y2;
        }

    for (x = 18; x < 27; x++)
        schedule[x][4] = '\0';

    for (x = 0; x < 4; x += 2)
        if (wn[x])
            for (z = 18; z < (league_setup.nummaxgames[2] + 18); z++) {
                if (z % 2) {
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                    strcat (&schedule[z][0], "-");
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                }
                else {
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                    strcat (&schedule[z][0], "-");
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                }
                if (league_setup.numleagues == 2 && league_setup.numdivs == 3 && league_setup.numwc == 2)
                    if (!x)
                        strcat (&schedule[z][0], "A");
                    else
                        strcat (&schedule[z][0], "N");
                else
                    strcat (&schedule[z][0], " ");
            }
    for (z = 18; z < 27; z++)
        strcat (&schedule[z][0], "\n");

third_round:
    for (x = 18; x < 27; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 27)
        goto fourth_round;

    for (x = 0; x < 4; x++)
        mu[x].t1[0] = mu[x].t1[1] = mu[x].t2[0] = mu[x].t2[1] = 0;

    for (x = 0, sgame = 12; sgame < strlen (&schedule[18][0]); sgame += 18, x++) {
        for (z = 0; z < 4; z++) {
            team1[z] = schedule[18][sgame - 4 + z];
            team1yr[z] = schedule[18][sgame - 8 + z];
            team2[z] = schedule[18][sgame + 5 + z];
            team2yr[z] = schedule[18][sgame + 1 + z];
        }
        team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
        mu[x].t1[0] = atoi (&team1[0]);
        mu[x].y1 = atoi (&team1yr[0]);
        mu[x].t2[0] = atoi (&team2[0]);
        mu[x].y2 = atoi (&team2yr[0]);
    }

    for (sday = 18; sday < 27; sday++)
        for (x = 0, sgame = 12; sgame < strlen (&schedule[sday][0]); sgame += 18, x++) {
            if (schedule[sday][sgame] == 'X' || schedule[sday][sgame] == '-')
                continue;
            for (z = 0; z < 4; z++) {
                team1[z] = schedule[sday][sgame - 4 + z];
                team1yr[z] = schedule[sday][sgame - 8 + z];
                team2[z] = schedule[sday][sgame + 5 + z];
                team2yr[z] = schedule[sday][sgame + 1 + z];
            }
            team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
            it1 = atoi (&team1[0]);
            iy1 = atoi (&team1yr[0]);
            it2 = atoi (&team2[0]);
            iy2 = atoi (&team2yr[0]);

            if (schedule[sday][sgame] == 'V')
                if (it1 == mu[x].t1[0] && iy1 == mu[x].y1)
                    mu[x].t1[1]++;
                else
                    mu[x].t2[1]++;
            else
                if (it2 == mu[x].t2[0] && iy2 == mu[x].y2)
                    mu[x].t2[1]++;
                else
                    mu[x].t1[1]++;
        }

    for (x = 0, sgame = 12; x < 4; x++, sgame += 18)
        if (mu[x].t1[1] == (league_setup.nummaxgames[2] / 2 + 1) || mu[x].t2[1] == (league_setup.nummaxgames[2] / 2 + 1))
            for (z = 18; z < 27; z++)
                if (schedule[z][sgame] == '-')
                    schedule[z][sgame] = 'X';

    for (x = 18; x < 27; x++)
        if (!index (&schedule[x][3], '-'))
            schedule[x][3] = 'X';

    for (x = 0; x < 36; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 36)
        for (x = 0; x < 4; x++) {
            if (mu[x].t1[1] == (league_setup.nummaxgames[2] / 2 + 1)) {
                champs = mu[x].t1[0];
                champsyr = mu[x].y1;
                return;
            }
            if (mu[x].t2[1] == (league_setup.nummaxgames[2] / 2 + 1)) {
                champs = mu[x].t2[0];
                champsyr = mu[x].y2;
                return;
            }
        }

    for (x = 18; x < 27; x++)
        if (schedule[x][3] != 'X')
            return;

    /* all the games in the third round of post-season play are completed but no world champ crowned yet */

    /* there can't be more than four rounds of post-season play */

    for (x = 0; x < 4; x++)
        wn[x] = 0;

    for (x = 0; x < 4; x++)
        if (mu[x].t1[1] == (league_setup.nummaxgames[2] / 2 + 1)) {
            wn[x] = mu[x].t1[0];
            wy[x] = mu[x].y1;
        }
        else {
            wn[x] = mu[x].t2[0];
            wy[x] = mu[x].y2;
        }

    for (z = 27; z < 36; z++)
        schedule[z][4] = '\0';
    for (x = 0; x < 2; x += 2)
        if (wn[x])
            for (z = 27; z < (league_setup.nummaxgames[3] + 27); z++) {
                if (z % 2) {
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                    strcat (&schedule[z][0], "-");
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                }
                else {
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x + 1]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x + 1]));
                    strcat (&schedule[z][0], "-");
                    strcat (&schedule[z][0], cnvt_int2str (4, wy[x]));
                    strcat (&schedule[z][0], cnvt_int2str (4, wn[x]));
                }
                strcat (&schedule[z][0], " \n");
            }

    for (z = 27 + league_setup.nummaxgames[3]; z < 36; z++)
        strcat (&schedule[z][0], "\n");

fourth_round:
    for (x = 27; x < 36; x++)
        if (schedule[x][3] != 'X')
            break;
    if (x == 36)
        return;

    for (x = 0; x < 4; x++)
        mu[x].t1[0] = mu[x].t1[1] = mu[x].t2[0] = mu[x].t2[1] = 0;

    for (x = 0, sgame = 12; sgame < strlen (&schedule[27][0]); sgame += 18, x++) {
        for (z = 0; z < 4; z++) {
            team1[z] = schedule[27][sgame - 4 + z];
            team1yr[z] = schedule[27][sgame - 8 + z];
            team2[z] = schedule[27][sgame + 5 + z];
            team2yr[z] = schedule[27][sgame + 1 + z];
        }
        team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
        mu[x].t1[0] = atoi (&team1[0]);
        mu[x].y1 = atoi (&team1yr[0]);
        mu[x].t2[0] = atoi (&team2[0]);
        mu[x].y2 = atoi (&team2yr[0]);
    }

    for (sday = 27; sday < 36; sday++)
        for (x = 0, sgame = 12; sgame < strlen (&schedule[sday][0]); sgame += 18, x++) {
            if (schedule[sday][sgame] == 'X' || schedule[sday][sgame] == '-')
                continue;
            for (z = 0; z < 4; z++) {
                team1[z] = schedule[sday][sgame - 4 + z];
                team1yr[z] = schedule[sday][sgame - 8 + z];
                team2[z] = schedule[sday][sgame + 5 + z];
                team2yr[z] = schedule[sday][sgame + 1 + z];
            }
            team1[4] = team2[4] = team1yr[4] = team2yr[4] = '\0';
            it1 = atoi (&team1[0]);
            iy1 = atoi (&team1yr[0]);
            it2 = atoi (&team2[0]);
            iy2 = atoi (&team2yr[0]);

            if (schedule[sday][sgame] == 'V')
                if (it1 == mu[x].t1[0] && iy1 == mu[x].y1)
                    mu[x].t1[1]++;
                else
                    mu[x].t2[1]++;
            else
                if (it2 == mu[x].t2[0] && iy2 == mu[x].y2)
                    mu[x].t2[1]++;
                else
                    mu[x].t1[1]++;
        }

    for (x = 0; x < 4; x++)
        wn[x] = 0;

    for (x = 0; x < 4; x++)
        if (mu[x].t1[1] == (league_setup.nummaxgames[3] / 2 + 1)) {
            wn[x] = mu[x].t1[0];
            wy[x] = mu[x].y1;
        }
        else {
            wn[x] = mu[x].t2[0];
            wy[x] = mu[x].y2;
        }
    for (x = 0, sgame = 12; x < 4; x++, sgame += 18)
        if (mu[x].t1[1] == (league_setup.nummaxgames[3] / 2 + 1) || mu[x].t2[1] == (league_setup.nummaxgames[3] / 2 + 1))
            for (z = 27; z < 36; z++)
                if (schedule[z][sgame] == '-')
                    schedule[z][sgame] = 'X';

    for (x = 27; x < 36; x++)
        if (!index (&schedule[x][3], '-'))
            schedule[x][3] = 'X';

    for (x = 0, sgame = 12; x < 4; x++, sgame += 18) {
        if (mu[x].t1[1] == (league_setup.nummaxgames[3] / 2 + 1) && strlen (&schedule[27][0]) < 26) {
            champs = mu[x].t1[0];
            champsyr = mu[x].y1;
            for (z = 0; z < 36; z++)
                schedule[z][3] = 'X';
            return;
        }
        if (mu[x].t2[1] == (league_setup.nummaxgames[3] / 2 + 1) && strlen (&schedule[27][0]) < 26) {
            champs = mu[x].t2[0];
            champsyr = mu[x].y2;
            for (z = 0; z < 36; z++)
                schedule[z][3] = 'X';
            return;
        }
    }
}

void
sortleaders (int rev, char era) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++) {
            if (!rev) {
                if (leaders[x].stat[5] < leaders[y].stat[5]) {
                    temp = leaders[x];
                    leaders[x] = leaders[y];
                    leaders[y] = temp;
                }
            }
            else
                if (leaders[x].stat[5] > leaders[y].stat[5]) {
                    temp = leaders[x];
                    leaders[x] = leaders[y];
                    leaders[y] = temp;
                }
            if (leaders[x].stat[5] == leaders[y].stat[5]) {
                if (rev && era == 'u') {
                    if (leaders[x].stat[0] < leaders[y].stat[0]) {
                        temp = leaders[x];
                        leaders[x] = leaders[y];
                        leaders[y] = temp;
                    }
                    else
                        if (leaders[x].stat[0] == leaders[y].stat[0])
                            if (leaders[x].stat[1] < leaders[y].stat[1]) {
                                temp = leaders[x];
                                leaders[x] = leaders[y];
                                leaders[y] = temp;
                            }
                }
                else
                    if (leaders[x].stat[0] < leaders[y].stat[0]) {
                        temp = leaders[x];
                        leaders[x] = leaders[y];
                        leaders[y] = temp;
                    }
            }
        }
}

char *
GetUCTeamname (int id) {
    char parent2[256], dummy[256];
    DIR *fnames;
    struct dirent *dir;
    int x, y;
    FILE *in;

    strcpy (&parent2[0], "/var/NSB/");
    strcat (&parent2[0], &nsbdb[user].id[0]);

    if ((fnames = opendir (&parent2[0])) != NULL)
        while ((dir = readdir (fnames))) {
            /* don't process . and .. files */
            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                continue;
            /* don't process these files */
            if (!strcmp (dir->d_name, "Schedule-PS") || !strcmp (dir->d_name, "Standings") || !strcmp (dir->d_name, "Series") ||
                      !strcmp (dir->d_name, "PlayoffResultsAL") || !strcmp (dir->d_name, "PlayoffResultsNL") || !strcmp (dir->d_name, "Schedule"))
                continue;
            if (!strcmp (dir->d_name, "Lifetime") || !strcmp (dir->d_name, "Records") || !strcmp (dir->d_name, "UserTeams"))
                continue;
            if (strstr (dir->d_name, "-PS"))
                continue;
            /* don't process the LeagueSetup file */
            if (!strcmp (dir->d_name, "LeagueSetup"))
                continue;

            /* look for any teams */
            strcpy (&dummy[0], &parent2[0]);
            strcat (&dummy[0], "/");
            strcat (&dummy[0], dir->d_name);

            if ((in = fopen (dummy, "r")) != NULL) {
                fread (&dteam.id, sizeof dteam.id, 1, in);
                fread (&dteam.year, sizeof dteam.year, 1, in);
                fread (&dteam.league, sizeof dteam.league, 1, in);
                fread (&dteam.division, sizeof dteam.division, 1, in);
                for (x = 0; x < 25; x++) {
                    fread (&dteam.batters[x].id, sizeof dteam.batters[x].id, 1, in);
                    fread (&dteam.batters[x].dob, sizeof dteam.batters[x].dob, 1, in);
                    fread (&dteam.batters[x].hitting, sizeof dteam.batters[x].hitting, 1, in);
                    for (y = 0; y < 11; y++)
                        fread (&dteam.batters[x].fielding[y], sizeof dteam.batters[x].fielding[y], 1, in);
                }
                for (x = 0; x < 11; x++) {
                    fread (&dteam.pitchers[x].id, sizeof dteam.pitchers[x].id, 1, in);
                    fread (&dteam.pitchers[x].pitching, sizeof dteam.pitchers[x].pitching, 1, in);
                }
                fclose (in);
            }
            if (dteam.id == id) {
                closedir (fnames);
                return (dir->d_name);
            }
        }
    closedir (fnames);
    return (NULL);
}

