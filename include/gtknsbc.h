#ifndef GTKNSBC
#define GTKNSBC

#define GTK_ENABLE_BROKEN  /* text doesn't work without this */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkwidget.h>

#define WINSIZEX 640
#define WINSIZEY 480

#define APPTITLE "GTK NetStatsBaseball Client"
#define APPVERSION "V0.9.9.7"
#define APPAUTHOR "Marshall Lake"

gchar hs[256];  /* name of NSB server */
gchar work[256];   /* work area */
gchar helptext[70000];  /* holds help info */
gchar abbrtext[50000];  /* holds abbreviations info */
gchar gpstext[50000];  /* holds good player season info */
gchar formchtext[50000];  /* holds valid formula characters info */
gchar tdibtext[100000];  /* holds This Day in Baseball info */
gchar mvptext[50000];  /* holds MVP info */
gchar hoftext[50000];  /* holds Hall of Fame info */
gchar nacctext[50000];  /* holds Notable Accomplishments info */
gchar NSBMVPtext[50000];  /* holds NSB MVP info */
gchar NSBCyYoungtext[50000];  /* holds NSB Cy Young info */
gchar NSBGoldGlovetext[50000];  /* holds NSB Gold Glove info */
gchar NSBSilverSluggertext[50000];  /* holds NSB Silver Slugger info */
gchar BestTeamstext[50000];  /* holds Best Real Life Teams info */
gchar BestSeasonstext[50000];  /* holds Best Real Life Player Seasons info */
gchar cytext[50000];  /* holds Cy Young info */
gchar ggtext[50000];  /* holds Gold Glove info */
gchar sstext[50000];  /* holds Silver Slugger info */
gchar rookietext[50000];  /* holds Rookie of the Year info */
gint connected;  /* 1 = connected to a NSB server, 0 = not connected */
gint nohelp;  /* 0 = help info available, 1 = no help info available */
gint noabbr;  /* 0 = abbreviations info available, 1 = no abbreviations info available */
gint SelectTeamsCompleted;  /* 0 = selecting the teams for a game did not complete successfully
                               1 = selecting the teams for a game completed successfully */
gint lgame;  /* 1 = league game, 2 = non-league game */
gint watchsw;  /* 1 = watching the game, 0 = playing against the computer */
gint LeagueUnderWay;   /* 0 = no league, 1 = league under way, 2 = league completed */
gint SeriesUnderWay;   /* 0 = no series, 1 = series under way, 2 = series completed */
gint VisitingTeamID, HomeTeamID;
gint ManageV, ManageH;   /* 1 = user managing visiting team (ManageV) or home team (ManageH), 0 - computer managing */
gint getayrRLTTactive;    /* 1 = the GetAYear window for Real Life Team Totals is active, 0 = not */
gint getayrRLRESactive;    /* 1 = the GetAYear window for Real Life Results is active, 0 = not */
gint getmdTDIBactive;    /* 1 = the TDIB window for getting a month & day is active, 0 = not */
gint getBSactive;    /* 1 = the Best Seasons window is active, 0 = not */
gint getBTactive;    /* 1 = the Best Teams window is active, 0 = not */
gint getACactive;    /* 1 = the Create Team window is active, 0 = not */
GtkWidget *mainwin, *text, *statustable;
GtkItemFactory *item_factory;
GtkLabel *nsbidlabel, *serverlabel, *connectcntlabel;
GdkColormap *cmap;
GdkColor color;

time_t dt;
struct tm dc;
struct timespec speed;
struct {
    int hr, min, sec;
} dc1, dc2;
char time_hr[3];
char time_min[3];
char time_sec[3];

struct autoplayparms {
    int active, linescore, boxscore, standings, injury;
    struct {
        struct {
            int games, atbats, runs, hits, doubles, triples, homers, rbis, bb, so, hbp, dp, sb, cs, ibb, sh, sf, ba, sa, oba;
        } hitting;
        struct {
            int games, gs, ip, wins, losses, sv, bfp, hits, db, tp, hr, runs, er, rbi, cg, gf, sho, svopp, sb, cs, bb, so, ibb, sh, sf, wp, b, hb, ab, era,
                pct, oppba;
        } pitching;
        struct {
            struct {
                int games, po, dp, a, e, pb, pct;
            } pos[8];
              /* pos[0] - OF, pos[1] - 1B, pos[2] - 2B, pos[3] - 3B, pos[4] - SS, pos[5] - P, pos[6] - C, pos[7] - DH */
        } fielding;
    } catl;
} autoplay;

struct {
    gint nplyr;
    struct {
        gchar pname[50], bmonth[3], bday[3], byear[5];
        GdkPixbuf *pic;
    } plyr[25];
} ppics[2];     /* instance 0 = home team, instance 1 = visitor */

#endif

