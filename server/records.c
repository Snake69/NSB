/*
    functions dealing with the Records file
*/

#include <stdio.h>
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sglobal.h"
#include "db.h"
#include "sproto.h"

extern int totgames;

void
get_records () {
    int x, y, z, zz, zzz;
    char dummy[256];
    FILE *f1;

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Records");

    /*
       read Records file, if it doesn't exist then create it
    */
    if ((f1 = fopen (dummy, "r")) != NULL) {
        fread (&records, sizeof records, 1, f1);
        fclose (f1);
    }
    else
        for (x = 0; x < 2; x++)
            for (y = 0; y < 50; y++) {
                for (z = 0; z < 20; z++) {
                    records[x].hitting[z][y].name[0] = records[x].hitting[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        records[x].hitting[z][y].stat[zzz] = 0;
                    records[x].hitting[z][y].dis = records[x].hitting[z][y].month = records[x].hitting[z][y].day = records[x].hitting[z][y].year = 0;
                }
                for (z = 0; z < 7; z++)
                    for (zz = 0; zz < 8; zz++) {
                        records[x].fielding[zz][z][y].name[0] = records[x].fielding[zz][z][y].uctname[0] = '\0';
                        for (zzz = 0; zzz < 7; zzz++)
                            records[x].fielding[zz][z][y].stat[zzz] = 0;
                        records[x].fielding[zz][z][y].day = records[x].fielding[zz][z][y].month =
                                                  records[x].fielding[zz][z][y].year = records[x].fielding[zz][z][y].dis = 0;
                    }
                for (z = 0; z < 32; z++) {
                    records[x].pitching[z][y].name[0] = records[x].pitching[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        records[x].pitching[z][y].stat[zzz] = 0;
                    records[x].pitching[z][y].day = records[x].pitching[z][y].month = records[x].pitching[z][y].year = records[x].pitching[z][y].dis = 0;
                }
                /* for ERA & OPP BA ... lowest is best */
                records[x].pitching[29][y].stat[5] = records[x].pitching[31][y].stat[5] = 9999;
            }
}

void
get_lifetime_records () {
    int x, y, z, zz, zzz;
    char dummy[256];
    FILE *f1;

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Lifetime");
    strcat (&dummy[0], "/Records");

    /*
       read Lifetime Records file
    */
    if ((f1 = fopen (dummy, "r")) != NULL) {
        fread (&lrecords, sizeof lrecords, 1, f1);
        fclose (f1);
    }
    else
        for (x = 0; x < 2; x++)
            for (y = 0; y < 50; y++) {
                for (z = 0; z < 20; z++) {
                    lrecords[x].hitting[z][y].name[0] = lrecords[x].hitting[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        lrecords[x].hitting[z][y].stat[zzz] = 0;
                    lrecords[x].hitting[z][y].dis = lrecords[x].hitting[z][y].month = lrecords[x].hitting[z][y].day = lrecords[x].hitting[z][y].year = 0;
                }
                for (z = 0; z < 7; z++)
                    for (zz = 0; zz < 8; zz++) {
                        lrecords[x].fielding[zz][z][y].name[0] = lrecords[x].fielding[zz][z][y].uctname[0] = '\0';
                        for (zzz = 0; zzz < 7; zzz++)
                            lrecords[x].fielding[zz][z][y].stat[zzz] = 0;
                        lrecords[x].fielding[zz][z][y].day = lrecords[x].fielding[zz][z][y].month =
                                                 lrecords[x].fielding[zz][z][y].year = lrecords[x].fielding[zz][z][y].dis = 0;
                    }
                for (z = 0; z < 32; z++) {
                    lrecords[x].pitching[z][y].name[0] = lrecords[x].pitching[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        lrecords[x].pitching[z][y].stat[zzz] = 0;
                    lrecords[x].pitching[z][y].day = lrecords[x].pitching[z][y].month = lrecords[x].pitching[z][y].year = lrecords[x].pitching[z][y].dis = 0;
                }
                /* for ERA & OPP BA ... lowest is best */
                lrecords[x].pitching[29][y].stat[5] = lrecords[x].pitching[31][y].stat[5] = 9999;
            }
}

void
put_records () {
    FILE *f1;
    char dummy[256];
    int gs, st, pi;

    /* before writing ensure ties are sorted properly */
    for (gs = 0; gs < 2; gs++) {
        for (st = 17; st < 20; st++)
            sorthitting (gs, st);
        for (st = 29; st < 32; st++)
            sortpitching (gs, st);
        for (pi = 0; pi < 8; pi++)
            for (st = 6; st < 7; st++)
                sortfielding (gs, pi, st);
    }

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Records");

    if ((f1 = fopen (dummy, "w")) != NULL) {
        fwrite (&records, sizeof records, 1, f1);
        fclose (f1);
    }
}

void
put_lifetime_records () {
    FILE *f1;
    char dummy[256];
    int gs, st, pi;

    /* before writing ensure ties are sorted properly */
    for (gs = 0; gs < 2; gs++) {
        for (st = 17; st < 20; st++)
            sortlhitting (gs, st);
        for (st = 29; st < 32; st++)
            sortlpitching (gs, st);
        for (pi = 0; pi < 8; pi++)
            for (st = 6; st < 7; st++)
                sortlfielding (gs, pi, st);
    }

    strcpy (&dummy[0], "/var/NSB/");
    strcat (&dummy[0], &nsbdb[user].id[0]);
    strcat (&dummy[0], "/Lifetime");
    strcat (&dummy[0], "/Records");

    if ((f1 = fopen (dummy, "w")) != NULL) {
        fwrite (&lrecords, sizeof lrecords, 1, f1);
        fclose (f1);
    }
}

void
update_records (int sday, int vid, int hid, char *tn) {
    /* visitor_cur and home_cur hold the complete stats for this game (done after each game)
       if sday is -1 then team holds the complete stats for the season (done after each season) */
    int hpf, ws, x, y, z, zz, gs, st, pi, lim;
    char teamname[50];
    time_t dt;
    struct tm dc;

    time (&dt);
    dc = *localtime (&dt);

    get_records ();

    if (sday == -1)
        lim = 1;
    else
        lim = 2;

    for (ws = 0; ws < lim; ws++) {
        if (sday != -1)
            switch (ws) {
                case 0:
                    team = visitor_cur;
                    strcpy (&teamname[0], GetUCTeamname (vid));
                    gs = 0;
                    break;
                case 1:
                    team = home_cur;
                    strcpy (&teamname[0], GetUCTeamname (hid));
                    gs = 0;
                    break;
            }
        else {
            gs = 1;
            strcpy (&teamname[0], tn);
        }

        hpf = 1;
        for (st = 0; st < 20; st++)
            for (x = 0; x < maxplayers[ws]; x++)
                for (y = 0; y < 50; y++)
                    if (cmp_rec (x, y, st, hpf, gs, pi)) {
                        for (z = 48; z >= y; z--)
                            if (!team.year) {
                                if (gs == 1 && !strcmp (&records[gs].hitting[st][z].uctname[0], &teamname[0]) &&
                                              !strcmp (&records[gs].hitting[st][z].name[0], &team.batters[x].id.name[0]))
                                    for (zz = z; zz < 49; zz++)
                                        records[gs].hitting[st][zz] = records[gs].hitting[st][zz + 1];
                                records[gs].hitting[st][z + 1] = records[gs].hitting[st][z];
                            }
                            else {
                                if (gs == 1 && records[gs].hitting[st][z].teamid == team.id && records[gs].hitting[st][z].tyear == team.year &&
                                              !strcmp (&records[gs].hitting[st][z].name[0], &team.batters[x].id.name[0]))
                                    for (zz = z; zz < 49; zz++)
                                        records[gs].hitting[st][zz] = records[gs].hitting[st][zz + 1];
                                records[gs].hitting[st][z + 1] = records[gs].hitting[st][z];
                            }
                        cp_rec (x, y, st, hpf, gs, pi);
                        records[gs].hitting[st][y].tyear = team.year;
                        records[gs].hitting[st][y].teamid = team.id;
                        strcpy (&records[gs].hitting[st][y].name[0], &team.batters[x].id.name[0]);
                        strcpy (&records[gs].hitting[st][y].uctname[0], &teamname[0]);
                        records[gs].hitting[st][y].dis = sday + 1;
                        records[gs].hitting[st][y].month = dc.tm_mon + 1;
                        records[gs].hitting[st][y].day = dc.tm_mday;
                        records[gs].hitting[st][y].year = dc.tm_year + 1900;
                        break;
                    }
                    else
                        if (!team.year) {
                            if (gs == 1 && !strcmp (&records[gs].hitting[st][y].uctname[0], &teamname[0]) &&
                                          !strcmp (&records[gs].hitting[st][y].name[0], &team.batters[x].id.name[0]))
                                break;
                        }
                        else
                            if (gs == 1 && records[gs].hitting[st][y].teamid == team.id && records[gs].hitting[st][y].tyear == team.year &&
                                    !strcmp (&records[gs].hitting[st][y].name[0], &team.batters[x].id.name[0]))
                                break;

        hpf = 2;
        for (st = 0; st < 32; st++)
            for (x = 0; x < maxpitchers[ws]; x++)
                for (y = 0; y < 50; y++)
                    if (cmp_rec (x, y, st, hpf, gs, pi)) {
                        for (z = 48; z >= y; z--)
                            if (!team.year) {
                                if (gs == 1 && !strcmp (&records[gs].pitching[st][z].uctname[0], &teamname[0]) &&
                                              !strcmp (&records[gs].pitching[st][z].name[0], &team.pitchers[x].id.name[0]))
                                    for (zz = z; zz < 49; zz++)
                                        records[gs].pitching[st][zz] = records[gs].pitching[st][zz + 1];
                                records[gs].pitching[st][z + 1] = records[gs].pitching[st][z];
                            }
                            else {
                                if (gs == 1 && records[gs].pitching[st][z].teamid == team.id && records[gs].pitching[st][z].tyear == team.year &&
                                              !strcmp (&records[gs].pitching[st][z].name[0], &team.pitchers[x].id.name[0]))
                                    for (zz = z; zz < 49; zz++)
                                        records[gs].pitching[st][zz] = records[gs].pitching[st][zz + 1];
                                records[gs].pitching[st][z + 1] = records[gs].pitching[st][z];
                            }
                        cp_rec (x, y, st, hpf, gs, pi);
                        records[gs].pitching[st][y].tyear = team.year;
                        records[gs].pitching[st][y].teamid = team.id;
                        strcpy (&records[gs].pitching[st][y].name[0], &team.pitchers[x].id.name[0]);
                        strcpy (&records[gs].pitching[st][y].uctname[0], &teamname[0]);
                        records[gs].pitching[st][y].dis = sday + 1;
                        records[gs].pitching[st][y].month = dc.tm_mon + 1;
                        records[gs].pitching[st][y].day = dc.tm_mday;
                        records[gs].pitching[st][y].year = dc.tm_year + 1900;
                        break;
                    }
                    else
                        if (!team.year) {
                            if (gs == 1 && !strcmp (&records[gs].pitching[st][y].uctname[0], &teamname[0]) &&
                                          !strcmp (&records[gs].pitching[st][y].name[0], &team.pitchers[x].id.name[0]))
                                break;
                        }
                        else
                            if (gs == 1 && records[gs].pitching[st][y].teamid == team.id && records[gs].pitching[st][y].tyear == team.year &&
                                    !strcmp (&records[gs].pitching[st][y].name[0], &team.pitchers[x].id.name[0]))
                                break;

        hpf = 3;
        for (pi = 0; pi < 8; pi++)
            for (st = 0; st < 7; st++)
                for (x = 0; x < maxplayers[ws]; x++)
                    for (y = 0; y < 50; y++)
                        if (cmp_rec (x, y, st, hpf, gs, pi)) {
                            for (z = 48; z >= y; z--)
                                if (!team.year) {
                                    if (gs == 1 && !strcmp (&records[gs].fielding[pi][st][z].uctname[0], &teamname[0]) &&
                                                  !strcmp (&records[gs].fielding[pi][st][z].name[0], &team.batters[x].id.name[0]))
                                        for (zz = z; zz < 49; zz++)
                                            records[gs].fielding[pi][st][zz] = records[gs].fielding[pi][st][zz + 1];
                                    records[gs].fielding[pi][st][z + 1] = records[gs].fielding[pi][st][z];
                                }
                                else {
                                    if (gs == 1 && records[gs].fielding[pi][st][z].teamid == team.id && records[gs].fielding[pi][st][z].tyear == team.year &&
                                                  !strcmp (&records[gs].fielding[pi][st][z].name[0], &team.batters[x].id.name[0]))
                                        for (zz = z; zz < 49; zz++)
                                            records[gs].fielding[pi][st][zz] = records[gs].fielding[pi][st][zz + 1];
                                    records[gs].fielding[pi][st][z + 1] = records[gs].fielding[pi][st][z];
                                }
                            cp_rec (x, y, st, hpf, gs, pi);
                            records[gs].fielding[pi][st][y].tyear = team.year;
                            records[gs].fielding[pi][st][y].teamid = team.id;
                            strcpy (&records[gs].fielding[pi][st][y].name[0], &team.batters[x].id.name[0]);
                            strcpy (&records[gs].fielding[pi][st][y].uctname[0], &teamname[0]);
                            records[gs].fielding[pi][st][y].dis = sday + 1;
                            records[gs].fielding[pi][st][y].month = dc.tm_mon + 1;
                            records[gs].fielding[pi][st][y].day = dc.tm_mday;
                            records[gs].fielding[pi][st][y].year = dc.tm_year + 1900;
                            break;
                        }
                        else
                            if (!team.year) {
                                if (gs == 1 && !strcmp (&records[gs].fielding[pi][st][y].uctname[0], &teamname[0]) &&
                                              !strcmp (&records[gs].fielding[pi][st][y].name[0], &team.batters[x].id.name[0]))
                                    break;
                            }
                            else
                                if (gs == 1 && records[gs].fielding[pi][st][y].teamid == team.id && records[gs].fielding[pi][st][y].tyear == team.year &&
                                              !strcmp (&records[gs].fielding[pi][st][y].name[0], &team.batters[x].id.name[0]))
                                    break;
    }
    put_records ();
}

/*
    use correct stat for devising a category leaders table
*/
int
cmp_rec (int x, int y, int st, int hpf, int gs, int pi) {
    int pos, singles, errors = 0, chances = 0, games = 0, ip = 0, stat_acc;

    if (hpf == 1) {
        if (st == 0)
            if (team.batters[x].hitting.games >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 1)
            if (team.batters[x].hitting.atbats >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 2)
            if (team.batters[x].hitting.runs >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 3)
            if (team.batters[x].hitting.hits >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 4)
            if (team.batters[x].hitting.doubles >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 5)
            if (team.batters[x].hitting.triples >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 6)
            if (team.batters[x].hitting.homers >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 7)
            if (team.batters[x].hitting.rbi >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 8)
            if (team.batters[x].hitting.bb >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 9)
            if (team.batters[x].hitting.so >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 10)
            if (team.batters[x].hitting.hbp >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 11)
            if (team.batters[x].hitting.gidp >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 12)
            if (team.batters[x].hitting.sb >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 13)
            if (team.batters[x].hitting.cs >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 14)
            if (team.batters[x].hitting.ibb >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 15)
            if (team.batters[x].hitting.sh >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 16)
            if (team.batters[x].hitting.sf >= records[gs].hitting[st][y].stat[5])
                return 1;
        if (st == 17)
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                       team.batters[x].hitting.sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames)
                if (((int) (((float) team.batters[x].hitting.hits / (float) team.batters[x].hitting.atbats) * 1000.0)) >= records[gs].hitting[st][y].stat[5])
                    return 1;
        if (st == 18) {
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                      team.batters[x].hitting.sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames) {
                singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers +
                          team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
                if (((int) ((((float) (team.batters[x].hitting.homers * 4) +
                       (float) (team.batters[x].hitting.triples * 3) + (float) (team.batters[x].hitting.doubles * 2) +
                                              (float) singles) / (float) team.batters[x].hitting.atbats) * 1000.0)) >= records[gs].hitting[st][y].stat[5])
                    return 1;
            }
        }
        if (st == 19)
            if ((team.batters[x].hitting.atbats + team.batters[x].hitting.bb + team.batters[x].hitting.hbp +
                                      team.batters[x].hitting.sf + team.batters[x].hitting.sh) >= (totgames * 3.1) && totgames)
                if (((int) (((float) team.batters[x].hitting.hits + (float) team.batters[x].hitting.bb +
                             (float) team.batters[x].hitting.hbp) / ((float) team.batters[x].hitting.atbats +
                                    (float) team.batters[x].hitting.bb + (float) team.batters[x].hitting.sf +
                           (float) team.batters[x].hitting.hbp) * 1000.0)) >= records[gs].hitting[st][y].stat[5])
                    return 1;
    }

    if (hpf == 2) {
        if (st == 0)
            if (team.pitchers[x].pitching.games >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 1)
            if (team.pitchers[x].pitching.games_started >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 2)
            if (team.pitchers[x].pitching.innings >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 3)
            if (team.pitchers[x].pitching.wins >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 4)
            if (team.pitchers[x].pitching.losses >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 5)
            if (team.pitchers[x].pitching.saves >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 6)
            if (team.pitchers[x].pitching.bfp >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 7)
            if (team.pitchers[x].pitching.hits >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 8)
            if (team.pitchers[x].pitching.doubles >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 9)
            if (team.pitchers[x].pitching.triples >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 10)
            if (team.pitchers[x].pitching.homers >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 11)
            if (team.pitchers[x].pitching.runs >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 12)
            if (team.pitchers[x].pitching.er >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 13)
            if (team.pitchers[x].pitching.rbi >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 14)
            if (team.pitchers[x].pitching.cg >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 15)
            if (team.pitchers[x].pitching.gf >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 16)
            if (team.pitchers[x].pitching.sho >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 17)
            if (team.pitchers[x].pitching.svopp >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 18)
            if (team.pitchers[x].pitching.sb >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 19)
            if (team.pitchers[x].pitching.cs >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 20)
            if (team.pitchers[x].pitching.walks >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 21)
            if (team.pitchers[x].pitching.so >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 22)
            if (team.pitchers[x].pitching.ibb >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 23)
            if (team.pitchers[x].pitching.sh >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 24)
            if (team.pitchers[x].pitching.sf >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 25)
            if (team.pitchers[x].pitching.wp >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 26)
            if (team.pitchers[x].pitching.balks >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 27)
            if (team.pitchers[x].pitching.hb >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 28)
            if (team.pitchers[x].pitching.opp_ab >= records[gs].pitching[st][y].stat[5])
                return 1;
        if (st == 29)
            if (team.pitchers[x].pitching.innings >= totgames && totgames)
                if (((int) (((float) (team.pitchers[x].pitching.er * 9.0) / ((float) team.pitchers[x].pitching.innings +
                        (float) team.pitchers[x].pitching.thirds / 3.0)) * 100.0)) <= records[gs].pitching[st][y].stat[5])
                    return 1;
        if (st == 30)
            if ((team.pitchers[x].pitching.wins + team.pitchers[x].pitching.losses) >= (totgames / 12) && totgames)
                if (((int) (((float) team.pitchers[x].pitching.wins / ((float) team.pitchers[x].pitching.wins +
                        (float) team.pitchers[x].pitching.losses)) * 1000.0)) >= records[gs].pitching[st][y].stat[5])
                    return 1;
        if (st == 31)
            if (team.pitchers[x].pitching.innings >= totgames && totgames)
                if (((int) (((float) team.pitchers[x].pitching.hits / (float) team.pitchers[x].pitching.opp_ab) *
                                                                     1000.0)) <= records[gs].pitching[st][y].stat[5])
                    return 1;
    }

    if (hpf == 3) {
        if (pi == 0)
            pos = 10;
        if (pi == 1)
            pos = 3;
        if (pi == 2)
            pos = 4;
        if (pi == 3)
            pos = 5;
        if (pi == 4)
            pos = 6;
        if (pi == 5)
            pos = 1;
        if (pi == 6)
            pos = 2;
        if (pi == 7)
            pos = 0;

        stat_acc = 0;
        do {
            if (st == 0)
                /* sometimes all 3 outfield position stats are combined into one */
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].games;
            if (st == 1)
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].po;
            if (st == 2)
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].dp;
            if (st == 3)
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].a;
            if (st == 4)
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].pb;
            if (st == 5)
                if (team.batters[x].fielding[pos].po != -1)
                    stat_acc += team.batters[x].fielding[pos].e;
            if (st == 6) {
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

        pos++;    /* pos subtracted above */
        if (st == 6) {
            if (((pos == 2 && games >= (totgames / 2)) || (pos == 1 && ip >= totgames) || (pos > 2 && games >= (totgames * 2 / 3))) && (chances || errors))
                stat_acc = (int) (((float) chances / ((float) chances + (float) errors)) * 1000.0);
            else
                stat_acc = -1;
        }

        if (stat_acc >= records[gs].fielding[pi][st][y].stat[5])
            return 1;
    }

    return 0;
}

void
cp_rec (int x, int y, int st, int hpf, int gs, int pi) {
    int pos, singles, errors = 0, chances = 0;
    float pct;

    if (hpf == 1) {
        if (st == 0)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.games;
        if (st == 1)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.atbats;
        if (st == 2)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.runs;
        if (st == 3)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.hits;
        if (st == 4)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.doubles;
        if (st == 5)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.triples;
        if (st == 6)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.homers;
        if (st == 7)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.rbi;
        if (st == 8)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.bb;
        if (st == 9)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.so;
        if (st == 10)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.hbp;
        if (st == 11)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.gidp;
        if (st == 12)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.sb;
        if (st == 13)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.cs;
        if (st == 14)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.ibb;
        if (st == 15)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.sh;
        if (st == 16)
            records[gs].hitting[st][y].stat[5] = team.batters[x].hitting.sf;
        if (st == 17) {
            pct = (float) team.batters[x].hitting.hits / (float) team.batters[x].hitting.atbats + 0.0005;              /* round up */
            records[gs].hitting[st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].hitting[st][y].stat[0] = team.batters[x].hitting.atbats;
            records[gs].hitting[st][y].stat[1] = team.batters[x].hitting.hits;
            records[gs].hitting[st][y].stat[6] = totgames * 3.1;
        }
        if (st == 18) {
            singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers + team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
            pct = (((float) (team.batters[x].hitting.homers * 4) +
                                (float) (team.batters[x].hitting.triples * 3) + (float) (team.batters[x].hitting.doubles * 2) +
                                                            (float) singles) / (float) team.batters[x].hitting.atbats) + 0.0005;    /* round up */
            records[gs].hitting[st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].hitting[st][y].stat[0] = team.batters[x].hitting.atbats;
            records[gs].hitting[st][y].stat[1] = team.batters[x].hitting.hits;
            records[gs].hitting[st][y].stat[2] = team.batters[x].hitting.doubles;
            records[gs].hitting[st][y].stat[3] = team.batters[x].hitting.triples;
            records[gs].hitting[st][y].stat[4] = team.batters[x].hitting.homers;
            records[gs].hitting[st][y].stat[6] = totgames * 3.1;
        }
        if (st == 19) {
            pct = ((float) team.batters[x].hitting.hits + (float) team.batters[x].hitting.bb + (float) team.batters[x].hitting.hbp) /
                                         ((float) team.batters[x].hitting.atbats + (float) team.batters[x].hitting.bb +
                                          (float) team.batters[x].hitting.sf + (float) team.batters[x].hitting.sh +
                                                                      (float) team.batters[x].hitting.hbp) + 0.0005;   /* round up */
            records[gs].hitting[st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].hitting[st][y].stat[0] = team.batters[x].hitting.atbats + team.batters[x].hitting.bb +
                                         team.batters[x].hitting.sf + team.batters[x].hitting.sh + team.batters[x].hitting.hbp;
            records[gs].hitting[st][y].stat[1] = team.batters[x].hitting.hits;
            records[gs].hitting[st][y].stat[2] = team.batters[x].hitting.bb;
            records[gs].hitting[st][y].stat[3] = team.batters[x].hitting.hbp;
            records[gs].hitting[st][y].stat[6] = totgames * 3.1;
        }
    }
    if (hpf == 2) {
        if (st == 0)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.games;
        if (st == 1)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.games_started;
        if (st == 2)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.innings;
        if (st == 3)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.wins;
        if (st == 4)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.losses;
        if (st == 5)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.saves;
        if (st == 6)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.bfp;
        if (st == 7)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.hits;
        if (st == 8)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.doubles;
        if (st == 9)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.triples;
        if (st == 10)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.homers;
        if (st == 11)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.runs;
        if (st == 12)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.er;
        if (st == 13)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.rbi;
        if (st == 14)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.cg;
        if (st == 15)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.gf;
        if (st == 16)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.sho;
        if (st == 17)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.svopp;
        if (st == 18)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.sb;
        if (st == 19)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.cs;
        if (st == 20)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.walks;
        if (st == 21)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.so;
        if (st == 22)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.ibb;
        if (st == 23)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.sh;
        if (st == 24)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.sf;
        if (st == 25)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.wp;
        if (st == 26)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.balks;
        if (st == 27)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.hb;
        if (st == 28)
            records[gs].pitching[st][y].stat[5] = team.pitchers[x].pitching.opp_ab;
        if (st == 29) {
            pct = ((float) (team.pitchers[x].pitching.er * 9.0) / ((float) team.pitchers[x].pitching.innings +
                                                               (float) team.pitchers[x].pitching.thirds / 3.0)) + 0.005;   /* round up */
            records[gs].pitching[st][y].stat[5] = (int) (pct * 100.0);
            records[gs].pitching[st][y].stat[0] = team.pitchers[x].pitching.innings;
            records[gs].pitching[st][y].stat[1] = team.pitchers[x].pitching.thirds;
            records[gs].pitching[st][y].stat[2] = team.pitchers[x].pitching.er;
            records[gs].pitching[st][y].stat[6] = totgames;
        }
        if (st == 30) {
            pct = ((float) team.pitchers[x].pitching.wins / ((float) team.pitchers[x].pitching.wins +
                                                                    (float) team.pitchers[x].pitching.losses)) + 0.0005;   /* round up */
            records[gs].pitching[st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].pitching[st][y].stat[0] = team.pitchers[x].pitching.wins;
            records[gs].pitching[st][y].stat[1] = team.pitchers[x].pitching.losses;
            records[gs].pitching[st][y].stat[6] = totgames / 12;
        }
        if (st == 31) {
            pct = ((float) team.pitchers[x].pitching.hits / (float) team.pitchers[x].pitching.opp_ab) + 0.0005;   /* round up */
            records[gs].pitching[st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].pitching[st][y].stat[0] = team.pitchers[x].pitching.opp_ab;
            records[gs].pitching[st][y].stat[1] = team.pitchers[x].pitching.hits;
            records[gs].pitching[st][y].stat[6] = totgames;
        }
    }
    if (hpf == 3) {
        if (pi == 0)
            pos = 10;
        if (pi == 1)
            pos = 3;
        if (pi == 2)
            pos = 4;
        if (pi == 3)
            pos = 5;
        if (pi == 4)
            pos = 6;
        if (pi == 5)
            pos = 1;
        if (pi == 6)
            pos = 2;
        if (pi == 7)
            pos = 0;

        records[gs].fielding[pi][st][y].stat[5] = 0;
        /* this do routine is only to accumulate all outfield positions */
        do {
            if (st == 0)
                /* sometimes all 3 outfield position stats are combined into one */
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].games;
            if (st == 1)
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].po;
            if (st == 2)
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].dp;
            if (st == 3)
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].a;
            if (st == 4)
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].pb;
            if (st == 5)
                if (team.batters[x].fielding[pos].po != -1)
                    records[gs].fielding[pi][st][y].stat[5] += team.batters[x].fielding[pos].e;
            if (st == 6)
                if (team.batters[x].fielding[pos].po != -1) {
                    errors += team.batters[x].fielding[pos].e;
                    chances += (team.batters[x].fielding[pos].po + team.batters[x].fielding[pos].a);
                }
            pos--;
        } while (pos > 6);
        if (st == 6) {
            pct = ((float) chances / ((float) chances + (float) errors)) + 0.0005;    /* round up */
            records[gs].fielding[pi][st][y].stat[5] = (int) (pct * 1000.0);
            records[gs].fielding[pi][st][y].stat[0] = chances + errors;
            records[gs].fielding[pi][st][y].stat[1] = errors;

            if ((pos + 1) == 1)
                records[gs].fielding[pi][st][y].stat[6] = totgames;
            else
                if ((pos + 1) == 2)
                    records[gs].fielding[pi][st][y].stat[6] = totgames / 2;
                else 
                    records[gs].fielding[pi][st][y].stat[6] = totgames * 2 / 3;
        }
    }
}

void
update_lifetime_records () {
    int x, y, z, zzz, gs, st, pi;

    for (gs = 0; gs < 2; gs++) {
        for (st = 0; st < 20; st++)
            for (x = 0; x < 50; x++)
                for (y = 0; y < 50; y++) {
                    for (zzz = 0; zzz < 50; zzz++)
                        if (!lrecords[gs].hitting[st][zzz].tyear) {
                            if (!strcmp (&records[gs].hitting[st][x].name[0], &lrecords[gs].hitting[st][zzz].name[0]) &&
                                !strcmp (&records[gs].hitting[st][x].uctname[0], &lrecords[gs].hitting[st][zzz].uctname[0]) &&
                                                records[gs].hitting[st][x].stat[5] == lrecords[gs].hitting[st][zzz].stat[5] &&
                                                        records[gs].hitting[st][x].day == lrecords[gs].hitting[st][zzz].day &&
                                                    records[gs].hitting[st][x].month == lrecords[gs].hitting[st][zzz].month &&
                                                      records[gs].hitting[st][x].year == lrecords[gs].hitting[st][zzz].year &&
                                                           records[gs].hitting[st][x].dis == lrecords[gs].hitting[st][zzz].dis)
                                zzz = 999;
                        }
                        else
                            if (!strcmp (&records[gs].hitting[st][x].name[0], &lrecords[gs].hitting[st][zzz].name[0]) &&
                                              records[gs].hitting[st][x].tyear == lrecords[gs].hitting[st][zzz].tyear &&
                                            records[gs].hitting[st][x].teamid == lrecords[gs].hitting[st][zzz].teamid &&
                                          records[gs].hitting[st][x].stat[5] == lrecords[gs].hitting[st][zzz].stat[5] &&
                                                  records[gs].hitting[st][x].day == lrecords[gs].hitting[st][zzz].day &&
                                              records[gs].hitting[st][x].month == lrecords[gs].hitting[st][zzz].month &&
                                                records[gs].hitting[st][x].year == lrecords[gs].hitting[st][zzz].year &&
                                                     records[gs].hitting[st][x].dis == lrecords[gs].hitting[st][zzz].dis)
                                zzz = 999;
                    if (zzz > 998)
                        break;
                    if (records[gs].hitting[st][x].stat[5] >= lrecords[gs].hitting[st][y].stat[5]) {
                        for (z = 48; z >= y; z--)
                            lrecords[gs].hitting[st][z + 1] = lrecords[gs].hitting[st][z];
                        lrecords[gs].hitting[st][y] = records[gs].hitting[st][x];
                        break;
                    }
                }

        for (st = 0; st < 32; st++)
            for (x = 0; x < 50; x++)
                for (y = 0; y < 50; y++) {
                    for (zzz = 0; zzz < 50; zzz++)
                        if (!lrecords[gs].pitching[st][zzz].tyear) {
                            if (!strcmp (&records[gs].pitching[st][x].name[0], &lrecords[gs].pitching[st][zzz].name[0]) &&
                                !strcmp (&records[gs].pitching[st][x].uctname[0], &lrecords[gs].pitching[st][zzz].uctname[0]) &&
                                                records[gs].pitching[st][x].stat[5] == lrecords[gs].pitching[st][zzz].stat[5] &&
                                                        records[gs].pitching[st][x].day == lrecords[gs].pitching[st][zzz].day &&
                                                    records[gs].pitching[st][x].month == lrecords[gs].pitching[st][zzz].month &&
                                                      records[gs].pitching[st][x].year == lrecords[gs].pitching[st][zzz].year &&
                                                           records[gs].pitching[st][x].dis == lrecords[gs].pitching[st][zzz].dis)
                                zzz = 999;
                        }
                        else
                            if (!strcmp (&records[gs].pitching[st][x].name[0], &lrecords[gs].pitching[st][zzz].name[0]) &&
                                              records[gs].pitching[st][x].tyear == lrecords[gs].pitching[st][zzz].tyear &&
                                            records[gs].pitching[st][x].teamid == lrecords[gs].pitching[st][zzz].teamid &&
                                          records[gs].pitching[st][x].stat[5] == lrecords[gs].pitching[st][zzz].stat[5] &&
                                                  records[gs].pitching[st][x].day == lrecords[gs].pitching[st][zzz].day &&
                                              records[gs].pitching[st][x].month == lrecords[gs].pitching[st][zzz].month &&
                                                records[gs].pitching[st][x].year == lrecords[gs].pitching[st][zzz].year &&
                                                     records[gs].pitching[st][x].dis == lrecords[gs].pitching[st][zzz].dis)
                                zzz = 999;
                    if (zzz > 998)
                        break;
                    if (st == 29 || st == 31) {
                        if (records[gs].pitching[st][x].stat[5] <= lrecords[gs].pitching[st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                lrecords[gs].pitching[st][z + 1] = lrecords[gs].pitching[st][z];
                            lrecords[gs].pitching[st][y] = records[gs].pitching[st][x];
                            break;
                        }
                    }
                    else
                        if (records[gs].pitching[st][x].stat[5] >= lrecords[gs].pitching[st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                lrecords[gs].pitching[st][z + 1] = lrecords[gs].pitching[st][z];
                            lrecords[gs].pitching[st][y] = records[gs].pitching[st][x];
                            break;
                        }
                }

        for (pi = 0; pi < 8; pi++)
            for (st = 0; st < 7; st++)
                for (x = 0; x < 50; x++)
                    for (y = 0; y < 50; y++) {
                        for (zzz = 0; zzz < 50; zzz++)
                            if (!lrecords[gs].fielding[pi][st][zzz].tyear) {
                                if (!strcmp (&records[gs].fielding[pi][st][x].name[0],
                                                                               &lrecords[gs].fielding[pi][st][zzz].name[0]) &&
                                       !strcmp (&records[gs].fielding[pi][st][x].uctname[0],
                                                                            &lrecords[gs].fielding[pi][st][zzz].uctname[0]) &&
                                      records[gs].fielding[pi][st][x].stat[5] == lrecords[gs].fielding[pi][st][zzz].stat[5] &&
                                              records[gs].fielding[pi][st][x].day == lrecords[gs].fielding[pi][st][zzz].day &&
                                          records[gs].fielding[pi][st][x].month == lrecords[gs].fielding[pi][st][zzz].month &&
                                            records[gs].fielding[pi][st][x].year == lrecords[gs].fielding[pi][st][zzz].year &&
                                                 records[gs].fielding[pi][st][x].dis == lrecords[gs].fielding[pi][st][zzz].dis)
                                    zzz = 999;
                            }
                            else
                                if (!strcmp (&records[gs].fielding[pi][st][x].name[0],
                                                                               &lrecords[gs].fielding[pi][st][zzz].name[0]) &&
                                          records[gs].fielding[pi][st][x].tyear == lrecords[gs].fielding[pi][st][zzz].tyear &&
                                        records[gs].fielding[pi][st][x].teamid == lrecords[gs].fielding[pi][st][zzz].teamid &&
                                      records[gs].fielding[pi][st][x].stat[5] == lrecords[gs].fielding[pi][st][zzz].stat[5] &&
                                              records[gs].fielding[pi][st][x].day == lrecords[gs].fielding[pi][st][zzz].day &&
                                          records[gs].fielding[pi][st][x].month == lrecords[gs].fielding[pi][st][zzz].month &&
                                            records[gs].fielding[pi][st][x].year == lrecords[gs].fielding[pi][st][zzz].year &&
                                                 records[gs].fielding[pi][st][x].dis == lrecords[gs].fielding[pi][st][zzz].dis)
                                    zzz = 999;
                        if (zzz > 998)
                            break;
                        if (records[gs].fielding[pi][st][x].stat[5] >= lrecords[gs].fielding[pi][st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                lrecords[gs].fielding[pi][st][z + 1] = lrecords[gs].fielding[pi][st][z];
                            lrecords[gs].fielding[pi][st][y] = records[gs].fielding[pi][st][x];
                            break;
                        }
                    }
    }
}

void
get_server_records () {
    int x, y, z, zz, zzz;
    char dummy[256];
    FILE *f1;

    strcpy (&dummy[0], "/var/NSB/Records");

    /*
       read Server Records file, if it doesn't exist then create it
    */
    if ((f1 = fopen (dummy, "r")) != NULL) {
        fread (&srecords, sizeof srecords, 1, f1);
        fclose (f1);
    }
    else
        for (x = 0; x < 2; x++)
            for (y = 0; y < 50; y++) {
                for (z = 0; z < 20; z++) {
                    srecords[x].hitting[z][y].name[0] = srecords[x].hitting[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        srecords[x].hitting[z][y].stat[zzz] = 0;
                    srecords[x].hitting[z][y].dis = srecords[x].hitting[z][y].month = srecords[x].hitting[z][y].day =
                                                                                     srecords[x].hitting[z][y].year = 0;
                }
                for (z = 0; z < 7; z++)
                    for (zz = 0; zz < 8; zz++) {
                        srecords[x].fielding[zz][z][y].name[0] = srecords[x].fielding[zz][z][y].uctname[0] = '\0';
                        for (zzz = 0; zzz < 7; zzz++)
                            srecords[x].fielding[zz][z][y].stat[zzz] = 0;
                        srecords[x].fielding[zz][z][y].day = srecords[x].fielding[zz][z][y].month =
                         srecords[x].fielding[zz][z][y].year = srecords[x].fielding[zz][z][y].dis = 0;
                    }
                for (z = 0; z < 32; z++) {
                    srecords[x].pitching[z][y].name[0] = srecords[x].pitching[z][y].uctname[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        srecords[x].pitching[z][y].stat[zzz] = 0;
                    srecords[x].pitching[z][y].day = srecords[x].pitching[z][y].month =
                     srecords[x].pitching[z][y].year = srecords[x].pitching[z][y].dis = 0;
                }
                /* for ERA & OPP BA ... lowest is best */
                srecords[x].pitching[29][y].stat[5] = srecords[x].pitching[31][y].stat[5] = 9999;
            }
}

void
update_server_records () {
    int x, y, z, zz, zzz, gs, st, pi, rf;
    char dummy[256];

    strcpy (&dummy[0], "/var/NSB/Records");

    /*
       read Server Records file, if it doesn't exist then create it
    */
    if ((rf = open (dummy, O_RDWR)) != -1) {
        lseek (rf, 0, SEEK_SET);
        read (rf, &srecords, sizeof srecords);
        lockf (rf, F_LOCK, 0);
    }
    else {
        rf = open (dummy, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        lseek (rf, 0, SEEK_SET);
        lockf (rf, F_LOCK, 0);

        for (x = 0; x < 2; x++)
            for (y = 0; y < 50; y++) {
                for (z = 0; z < 20; z++) {
                    srecords[x].hitting[z][y].name[0] = srecords[x].hitting[z][y].id[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        srecords[x].hitting[z][y].stat[zzz] = 0;
                    srecords[x].hitting[z][y].dis = srecords[x].hitting[z][y].month = srecords[x].hitting[z][y].day =
                                                                                     srecords[x].hitting[z][y].year = 0;
                }
                for (z = 0; z < 7; z++)
                    for (zz = 0; zz < 8; zz++) {
                        srecords[x].fielding[zz][z][y].name[0] = srecords[x].fielding[zz][z][y].id[0] = '\0';
                        for (zzz = 0; zzz < 7; zzz++)
                            srecords[x].fielding[zz][z][y].stat[zzz] = 0;
                        srecords[x].fielding[zz][z][y].day = srecords[x].fielding[zz][z][y].month =
                         srecords[x].fielding[zz][z][y].year = srecords[x].fielding[zz][z][y].dis = 0;
                    }
                for (z = 0; z < 32; z++) {
                    srecords[x].pitching[z][y].name[0] = srecords[x].pitching[z][y].id[0] = '\0';
                    for (zzz = 0; zzz < 7; zzz++)
                        srecords[x].pitching[z][y].stat[zzz] = 0;
                    srecords[x].pitching[z][y].day = srecords[x].pitching[z][y].month = srecords[x].pitching[z][y].year =
                                                                                         srecords[x].pitching[z][y].dis = 0;
                }
                /* for ERA & OPP BA ... lowest is best */
                srecords[x].pitching[29][y].stat[5] = srecords[x].pitching[31][y].stat[5] = 9999;
            }
    }

    /* update */
    for (gs = 0; gs < 2; gs++) {
        for (st = 0; st < 20; st++)
            for (x = 0; x < 50; x++)
                for (y = 0; y < 50; y++) {
                    for (zzz = 0; zzz < 50; zzz++)
                        if (!srecords[gs].hitting[st][zzz].tyear) {
                            if (!strcmp (&lrecords[gs].hitting[st][x].name[0], &srecords[gs].hitting[st][zzz].name[0]) &&
                                                             !strcmp (&srecords[gs].hitting[st][zzz].id[0], &nsbdb[user].id[0]) &&
                                   !strcmp (&lrecords[gs].hitting[st][x].uctname[0], &srecords[gs].hitting[st][zzz].uctname[0]) &&
                                                   lrecords[gs].hitting[st][x].stat[5] == srecords[gs].hitting[st][zzz].stat[5] &&
                                                           lrecords[gs].hitting[st][x].day == srecords[gs].hitting[st][zzz].day &&
                                                       lrecords[gs].hitting[st][x].month == srecords[gs].hitting[st][zzz].month &&
                                                         lrecords[gs].hitting[st][x].year == srecords[gs].hitting[st][zzz].year &&
                                                              lrecords[gs].hitting[st][x].dis == srecords[gs].hitting[st][zzz].dis)
                                zzz = 999;
                        }
                        else
                            if (!strcmp (&lrecords[gs].hitting[st][x].name[0], &srecords[gs].hitting[st][zzz].name[0]) &&
                                                    !strcmp (&srecords[gs].hitting[st][zzz].id[0], &nsbdb[user].id[0]) &&
                                              lrecords[gs].hitting[st][x].tyear == srecords[gs].hitting[st][zzz].tyear &&
                                            lrecords[gs].hitting[st][x].teamid == srecords[gs].hitting[st][zzz].teamid &&
                                          lrecords[gs].hitting[st][x].stat[5] == srecords[gs].hitting[st][zzz].stat[5] &&
                                                  lrecords[gs].hitting[st][x].day == srecords[gs].hitting[st][zzz].day &&
                                              lrecords[gs].hitting[st][x].month == srecords[gs].hitting[st][zzz].month &&
                                                lrecords[gs].hitting[st][x].year == srecords[gs].hitting[st][zzz].year &&
                                                     lrecords[gs].hitting[st][x].dis == srecords[gs].hitting[st][zzz].dis)
                                zzz = 999;
                    if (zzz > 998)
                        break;
                    if (lrecords[gs].hitting[st][x].stat[5] >= srecords[gs].hitting[st][y].stat[5]) {
                        for (z = 48; z >= y; z--)
                            srecords[gs].hitting[st][z + 1] = srecords[gs].hitting[st][z];
                        srecords[gs].hitting[st][y] = lrecords[gs].hitting[st][x];
                        strcpy (&srecords[gs].hitting[st][y].id[0], &nsbdb[user].id[0]);
                        break;
                    }
                }

        for (st = 0; st < 32; st++)
            for (x = 0; x < 50; x++)
                for (y = 0; y < 50; y++) {
                    for (zzz = 0; zzz < 50; zzz++)
                        if (!srecords[gs].pitching[st][zzz].tyear) {
                            if (!strcmp (&lrecords[gs].pitching[st][x].name[0], &srecords[gs].pitching[st][zzz].name[0]) &&
                                                            !strcmp (&srecords[gs].pitching[st][zzz].id[0], &nsbdb[user].id[0]) &&
                                 !strcmp (&lrecords[gs].pitching[st][x].uctname[0], &srecords[gs].pitching[st][zzz].uctname[0]) &&
                                                 lrecords[gs].pitching[st][x].stat[5] == srecords[gs].pitching[st][zzz].stat[5] &&
                                                         lrecords[gs].pitching[st][x].day == srecords[gs].pitching[st][zzz].day &&
                                                     lrecords[gs].pitching[st][x].month == srecords[gs].pitching[st][zzz].month &&
                                                       lrecords[gs].pitching[st][x].year == srecords[gs].pitching[st][zzz].year &&
                                                            lrecords[gs].pitching[st][x].dis == srecords[gs].pitching[st][zzz].dis)
                                zzz = 999;
                        }
                        else
                            if (!strcmp (&lrecords[gs].pitching[st][x].name[0], &srecords[gs].pitching[st][zzz].name[0]) &&
                                                     !strcmp (&srecords[gs].pitching[st][zzz].id[0], &nsbdb[user].id[0]) &&
                                              lrecords[gs].pitching[st][x].tyear == srecords[gs].pitching[st][zzz].tyear &&
                                            lrecords[gs].pitching[st][x].teamid == srecords[gs].pitching[st][zzz].teamid &&
                                          lrecords[gs].pitching[st][x].stat[5] == srecords[gs].pitching[st][zzz].stat[5] &&
                                                  lrecords[gs].pitching[st][x].day == srecords[gs].pitching[st][zzz].day &&
                                              lrecords[gs].pitching[st][x].month == srecords[gs].pitching[st][zzz].month &&
                                                lrecords[gs].pitching[st][x].year == srecords[gs].pitching[st][zzz].year &&
                                                     lrecords[gs].pitching[st][x].dis == srecords[gs].pitching[st][zzz].dis)
                                zzz = 999;
                    if (zzz > 998)
                        break;
                    if (st == 29 || st == 31) {
                        if (lrecords[gs].pitching[st][x].stat[5] <= srecords[gs].pitching[st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                srecords[gs].pitching[st][z + 1] = srecords[gs].pitching[st][z];
                            srecords[gs].pitching[st][y] = lrecords[gs].pitching[st][x];
                            strcpy (&srecords[gs].pitching[st][y].id[0], &nsbdb[user].id[0]);
                            break;
                        }
                    }
                    else
                        if (lrecords[gs].pitching[st][x].stat[5] >= srecords[gs].pitching[st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                srecords[gs].pitching[st][z + 1] = srecords[gs].pitching[st][z];
                            srecords[gs].pitching[st][y] = lrecords[gs].pitching[st][x];
                            strcpy (&srecords[gs].pitching[st][y].id[0], &nsbdb[user].id[0]);
                            break;
                        }
                }

        for (pi = 0; pi < 8; pi++)
            for (st = 0; st < 7; st++)
                for (x = 0; x < 50; x++)
                    for (y = 0; y < 50; y++) {
                        for (zzz = 0; zzz < 50; zzz++)
                            if (!srecords[gs].fielding[pi][st][zzz].tyear) {
                                if (!strcmp (&lrecords[gs].fielding[pi][st][x].name[0],
                                                                            &srecords[gs].fielding[pi][st][zzz].name[0]) &&
                                                 !strcmp (&srecords[gs].fielding[pi][st][zzz].id[0], &nsbdb[user].id[0]) &&
                                                 !strcmp (&lrecords[gs].fielding[pi][st][x].uctname[0],
                                                                         &srecords[gs].fielding[pi][st][zzz].uctname[0]) &&
                                  lrecords[gs].fielding[pi][st][x].stat[5] == srecords[gs].fielding[pi][st][zzz].stat[5] &&
                                          lrecords[gs].fielding[pi][st][x].day == srecords[gs].fielding[pi][st][zzz].day &&
                                      lrecords[gs].fielding[pi][st][x].month == srecords[gs].fielding[pi][st][zzz].month &&
                                        lrecords[gs].fielding[pi][st][x].year == srecords[gs].fielding[pi][st][zzz].year &&
                                             lrecords[gs].fielding[pi][st][x].dis == srecords[gs].fielding[pi][st][zzz].dis)
                                    zzz = 999;
                            }
                            else
                                if (!strcmp (&lrecords[gs].fielding[pi][st][x].name[0],
                                                                             &srecords[gs].fielding[pi][st][zzz].name[0]) &&
                                                  !strcmp (&srecords[gs].fielding[pi][st][zzz].id[0], &nsbdb[user].id[0]) &&
                                       lrecords[gs].fielding[pi][st][x].tyear == srecords[gs].fielding[pi][st][zzz].tyear &&
                                     lrecords[gs].fielding[pi][st][x].teamid == srecords[gs].fielding[pi][st][zzz].teamid &&
                                   lrecords[gs].fielding[pi][st][x].stat[5] == srecords[gs].fielding[pi][st][zzz].stat[5] &&
                                           lrecords[gs].fielding[pi][st][x].day == srecords[gs].fielding[pi][st][zzz].day &&
                                       lrecords[gs].fielding[pi][st][x].month == srecords[gs].fielding[pi][st][zzz].month &&
                                         lrecords[gs].fielding[pi][st][x].year == srecords[gs].fielding[pi][st][zzz].year &&
                                              lrecords[gs].fielding[pi][st][x].dis == srecords[gs].fielding[pi][st][zzz].dis)
                                    zzz = 999;
                        if (zzz > 998)
                            break;
                        if (lrecords[gs].fielding[pi][st][x].stat[5] >= srecords[gs].fielding[pi][st][y].stat[5]) {
                            for (z = 48; z >= y; z--)
                                srecords[gs].fielding[pi][st][z + 1] = srecords[gs].fielding[pi][st][z];
                            srecords[gs].fielding[pi][st][y] = lrecords[gs].fielding[pi][st][x];
                            strcpy (&srecords[gs].fielding[pi][st][y].id[0], &nsbdb[user].id[0]);
                            break;
                        }
                    }
    }
    /* before writing ensure ties are sorted properly */
    for (gs = 0; gs < 2; gs++) {
        for (st = 17; st < 20; st++)
            sortshitting (gs, st);
        for (st = 29; st < 32; st++)
            sortspitching (gs, st);
        for (pi = 0; pi < 8; pi++)
            for (st = 6; st < 7; st++)
                sortsfielding (gs, pi, st);
    }

    lseek (rf, 0, SEEK_SET);
    write (rf, &srecords, sizeof srecords);
    fsync (rf);
    close (rf);
}

void
sorthitting (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (records[gs].hitting[st][x].stat[5] == records[gs].hitting[st][y].stat[5])
                if (records[gs].hitting[st][x].stat[0] < records[gs].hitting[st][y].stat[0]) {
                    temp = records[gs].hitting[st][x];
                    records[gs].hitting[st][x] = records[gs].hitting[st][y];
                    records[gs].hitting[st][y] = temp;
                }
}

void
sortpitching (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (records[gs].pitching[st][x].stat[5] == records[gs].pitching[st][y].stat[5]) {
                if (records[gs].pitching[st][x].stat[0] < records[gs].pitching[st][y].stat[0]) {
                    temp = records[gs].pitching[st][x];
                    records[gs].pitching[st][x] = records[gs].pitching[st][y];
                    records[gs].pitching[st][y] = temp;
                }
                else
                    if (records[gs].pitching[st][x].stat[0] == records[gs].pitching[st][y].stat[0])
                        if (st == 29 && records[gs].pitching[st][x].stat[1] < records[gs].pitching[st][y].stat[1]) {
                            temp = records[gs].pitching[st][x];
                            records[gs].pitching[st][x] = records[gs].pitching[st][y];
                            records[gs].pitching[st][y] = temp;
                        }
            }
}

void
sortfielding (int gs, int pi, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (records[gs].fielding[pi][st][x].stat[5] == records[gs].fielding[pi][st][y].stat[5])
                if (records[gs].fielding[pi][st][x].stat[0] < records[gs].fielding[pi][st][y].stat[0]) {
                    temp = records[gs].fielding[pi][st][x];
                    records[gs].fielding[pi][st][x] = records[gs].fielding[pi][st][y];
                    records[gs].fielding[pi][st][y] = temp;
                }
}

void
sortlhitting (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (lrecords[gs].hitting[st][x].stat[5] == lrecords[gs].hitting[st][y].stat[5])
                if (lrecords[gs].hitting[st][x].stat[0] < lrecords[gs].hitting[st][y].stat[0]) {
                    temp = lrecords[gs].hitting[st][x];
                    lrecords[gs].hitting[st][x] = lrecords[gs].hitting[st][y];
                    lrecords[gs].hitting[st][y] = temp;
                }
}

void
sortlpitching (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (lrecords[gs].pitching[st][x].stat[5] == lrecords[gs].pitching[st][y].stat[5]) {
                if (lrecords[gs].pitching[st][x].stat[0] < lrecords[gs].pitching[st][y].stat[0]) {
                    temp = lrecords[gs].pitching[st][x];
                    lrecords[gs].pitching[st][x] = lrecords[gs].pitching[st][y];
                    lrecords[gs].pitching[st][y] = temp;
                }
                else
                    if (lrecords[gs].pitching[st][x].stat[0] == lrecords[gs].pitching[st][y].stat[0])
                        if (st == 29 && lrecords[gs].pitching[st][x].stat[1] < lrecords[gs].pitching[st][y].stat[1]) {
                            temp = lrecords[gs].pitching[st][x];
                            lrecords[gs].pitching[st][x] = lrecords[gs].pitching[st][y];
                            lrecords[gs].pitching[st][y] = temp;
                        }
            }
}

void
sortlfielding (int gs, int pi, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (lrecords[gs].fielding[pi][st][x].stat[5] == lrecords[gs].fielding[pi][st][y].stat[5])
                if (lrecords[gs].fielding[pi][st][x].stat[0] < lrecords[gs].fielding[pi][st][y].stat[0]) {
                    temp = lrecords[gs].fielding[pi][st][x];
                    lrecords[gs].fielding[pi][st][x] = lrecords[gs].fielding[pi][st][y];
                    lrecords[gs].fielding[pi][st][y] = temp;
                }
}

void
sortshitting (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (srecords[gs].hitting[st][x].stat[5] == srecords[gs].hitting[st][y].stat[5])
                if (srecords[gs].hitting[st][x].stat[0] < srecords[gs].hitting[st][y].stat[0]) {
                    temp = srecords[gs].hitting[st][x];
                    srecords[gs].hitting[st][x] = srecords[gs].hitting[st][y];
                    srecords[gs].hitting[st][y] = temp;
                }
}

void
sortspitching (int gs, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (srecords[gs].pitching[st][x].stat[5] == srecords[gs].pitching[st][y].stat[5]) {
                if (srecords[gs].pitching[st][x].stat[0] < srecords[gs].pitching[st][y].stat[0]) {
                    temp = srecords[gs].pitching[st][x];
                    srecords[gs].pitching[st][x] = srecords[gs].pitching[st][y];
                    srecords[gs].pitching[st][y] = temp;
                }
                else
                    if (srecords[gs].pitching[st][x].stat[0] == srecords[gs].pitching[st][y].stat[0])
                        if (st == 29 && srecords[gs].pitching[st][x].stat[1] < srecords[gs].pitching[st][y].stat[1]) {
                            temp = srecords[gs].pitching[st][x];
                            srecords[gs].pitching[st][x] = srecords[gs].pitching[st][y];
                            srecords[gs].pitching[st][y] = temp;
                        }
            }
}

void
sortsfielding (int gs, int pi, int st) {
    int x, y;
    struct data temp;

    for (x = 0; x < 49; x++)
        for (y = x + 1; y < 50; y++)
            if (srecords[gs].fielding[pi][st][x].stat[5] == srecords[gs].fielding[pi][st][y].stat[5])
                if (srecords[gs].fielding[pi][st][x].stat[0] < srecords[gs].fielding[pi][st][y].stat[0]) {
                    temp = srecords[gs].fielding[pi][st][x];
                    srecords[gs].fielding[pi][st][x] = srecords[gs].fielding[pi][st][y];
                    srecords[gs].fielding[pi][st][y] = temp;
                }
}

