/* various initializations */

#include <sys/stat.h>
#include <fcntl.h>
#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

/* attempt to connect to a server */
int
Connect2Server () {
    sock = make_connection ("49999", SOCK_STREAM, &hs[0], 0);
    if (sock == -1)
        return (0);
    else {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        strcpy (&sname[0], &buffer[0]);
        return (1);
    }
}

/* try to get the user's name from $LOGNAME, $USER or cuserid() */
int
GetUserInfo () {
    gchar *NSBIDLibFull = "The NSBID library on this server is full.  Try another NSB server.",
          *ServerProb = "There is a problem with this server.  Try another NSB server.",
          *NSBIDDup = "This ID is already being used on this server.  Use a different ID or try another NSB server.",
          *NSBIDInv = "This ID is an invalid NSB ID.  Use a different ID.",
          *msg[5];
    gint x;

    env_logname = getenv ("LOGNAME");
    env_user = getenv ("USER");
    if (env_logname) {
        strcpy (cname, env_logname);
    }
    else
        if (env_user)
            strcpy (cname, env_user);
        else
            cuserid (&cname[0]);

    /* send userid to server */
    strcpy (&buffer[0], cname);
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    /* if a new player send NSB id */
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (strcmp (&buffer[0], "ok")) {
get_id:
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        CreateNSBID (0);
        if (!strlen (&buffer[0]))
            return -1;

        if (strlen (&buffer[0]) == 1) {
            msg[0] = NSBIDInv;
            outMessage (msg);
            goto get_id;
        }

        sock_puts (sock, &buffer[0]);
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            msg[0] = ServerProb;
            outMessage (msg);
            goto get_id;
        }
        if (!strcmp (&buffer[0], "err")) {
            msg[0] = NSBIDLibFull;
            outMessage (msg);
            goto get_id;
        }
        if (!strcmp (&buffer[0], "dup")) {
            msg[0] = NSBIDDup;
            outMessage (msg);
            goto get_id;
        }
        if (!strcmp (&buffer[0], "inv")) {
            msg[0] = NSBIDInv;
            outMessage (msg);
            goto get_id;
        }

        strcpy (&work[0], "NSB ID accepted on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 0);
    }

    /* initialize teaminfo structure */
    populate ();

    /* get site id */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
        return (-1);
    strcpy (&sid[0], &buffer[0]);

    /* get NSB id */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
        return (-1);
    strcpy (&nsbid[0], &buffer[0]);

    /* get login count */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
        return (-1);
    strcpy (&lct[0], &buffer[0]);

    return (0);
}

void
ShowTDIB (int boot) {
    if (boot)
        if (preferences.ShowTDIBAtBoot)
            TDIB (boot, 0, 0);
}

void
Connect2NSBServer (int boot) {
    gint cansw = 0;

    if (!strlen (&hs[0]))
        strcpy (&hs[0], "localhost");

    /* try to connect to server running NSB */
    connected = Connect2Server ();

    /* get some user info from the server */
    if (connected)
        if (GetUserInfo ()) {
            sname[0] = '\0';
            close (sock);
            cansw = 1;
            connected = 0;
        }

    DisplayTopHalf (boot);

    /* add some text into the scrolling text window in the bottom half of the main window */
    if (connected) {
        strcpy (&work[0], "Connected to NSB server running on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], " as NSB ID ");
        strcat (&work[0], &nsbid[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 0);
        /* play a connect sound */
        g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/hit.wav");
        /* check if user has a season going on the connected server */
        sock_puts (sock, "S7\n");
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "OK"))
            SetLeagueUnderWay (1);
        else {
            /* check if user has a season which has completed */
            sock_puts (sock, "S71\n");
            sock_gets (sock, &buffer[0], sizeof (buffer));
            if (!strcmp (&buffer[0], "OK"))
                SetLeagueUnderWay (2);
            else
                SetLeagueUnderWay (0);
        }
        /* check if user has a series going on the connected server */
        sock_puts (sock, "S0\n");
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "OK"))
            SetSeriesUnderWay (1);
        else
            if (!strcmp (&buffer[0], "EOS"))
                SetSeriesUnderWay (2);
            else
                SetSeriesUnderWay (0);
    }
    else {
        /* add some text into the scrolling text window in the bottom half of the main window */
        SetLeagueUnderWay (0);
        SetSeriesUnderWay (0);
        if (!cansw) { 
            strcpy (&work[0], "Cannot make a connection to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], "\n");
            if (strcmp (&hs[0], "localhost")) {
                strcat (&work[0], "Maybe the server hardware itself is down or the\n");
                strcat (&work[0], "baseball server software is not running there.\n");
            }
            else
                strcat (&work[0], "Maybe the baseball server software is not running.\n");
            Add2TextWindow (&work[0], 1);
        }
        else {
            strcpy (&work[0], "Connection to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], " killed.\n");
            Add2TextWindow (&work[0], 0);
        }
    }

    /* update main window display */
    gtk_widget_show_all (mainwin);
}

/* add some dynamic status info into the top half of the main window */
void
DisplayTopHalf (int boot) {
    if (!boot) {
        gchar null[5];

        /* clear text already in top half of window */
        null[0] = '\0';
        gtk_label_set_text (nsbidlabel, null);
        gtk_label_set_text (serverlabel, null);
        gtk_label_set_text (connectcntlabel, null);
    }

    if (connected) {
        strcpy (&work[0], &nsbid[0]);
        strcat (&work[0], " <");
        strcat (&work[0], &cname[0]);
        if (strlen (&sid[0]) > 0) {
            strcat (&work[0], "@");
            strcat (&work[0], &sid[0]);
        }
        strcat (&work[0], ">");
        nsbidlabel = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);

        strcpy (&work[0], "NSB Server - ");
        strcat (&work[0], &hs[0]);
        serverlabel = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);

        strcpy (&work[0], "Connection #");
        strcat (&work[0], &lct[0]);
        connectcntlabel = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);

        gtk_table_attach_defaults (GTK_TABLE (statustable), GTK_WIDGET (nsbidlabel), 0, 1, 0, 1);
        gtk_table_attach_defaults (GTK_TABLE (statustable), GTK_WIDGET (serverlabel), 1, 2, 0, 1);
        gtk_table_attach_defaults (GTK_TABLE (statustable), GTK_WIDGET (connectcntlabel), 2, 3, 0, 1);
    }
    else {
        strcpy (&work[0], "NSB Server - ");
        strcat (&work[0], "not connected");
        serverlabel = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);

        gtk_table_attach_defaults (GTK_TABLE (statustable), GTK_WIDGET (serverlabel), 1, 2, 0, 1);
    }

    gtk_widget_show_all (mainwin);
}

/* some initialization for the NSB client */
void
nsb_init () {
    FILE *rc;
    struct stat statbuf;
    gint x, fin;
    gchar *Line1 = "The format of the rc file (user preferences) has changed since the last time it was saved.  ",
          *Line2 = "If something other than the default settings are desired then click\nGame->Preferences ",
          *Line3 = "to make the necessary changes.  Make sure to save your changes if you would like to use the new settings the ",
          *Line4 = "next time NSB is executed.  You will continue to get this alert at each running until the preferences are saved.",
          *msg[5], path2ap[1024];

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    /* seed random */
    srand (time (NULL));

    prtbutrlrespnt = prtbutcatpnt = prtbuttmpnt = prtbutttpnt = prtbutrlppnt = prtbuttdibpnt = prtbutBSpnt = getayrRLTTactive = getayrRLRESactive =
      getmdTDIBactive = getBSactive = getACactive = getBTactive = prtbutBTpnt = 0;

    /* load the HELP file if it exists */
    if ((rc = fopen ("/usr/local/share/NSB/user-manual", "r")) != NULL) {
        x = fread (&helptext, sizeof helptext, 1, rc);
        if (!ferror (rc))
            nohelp = 0;
        else
            /* error reading HELP file */
            nohelp = 1;
        fclose (rc);
    }
    else
        /* error opening HELP file */
        nohelp = 1;

    /* load the ABBREVIATIONS file if it exists */
    if ((rc = fopen ("/usr/local/share/NSB/abbreviations", "r")) != NULL) {
        x = fread (&abbrtext, sizeof abbrtext, 1, rc);
        if (!ferror (rc))
            noabbr = 0;
        else
            /* error reading ABBREVIATIONS file */
            noabbr = 1;
        fclose (rc);
    }
    else
        /* error opening ABBREVIATIONS file */
        noabbr = 1;

    DefSetPath[0] = '\0';
    strcpy (&DefSetPath[0], getenv ("HOME"));
    strcat (&DefSetPath[0], "/.nsbrc");

    /* check if rc file is latest format */
    if ((fin = open (DefSetPath, O_RDONLY)) >= 0) {
        if ((fstat (fin, &statbuf)) >= 0)
            if (statbuf.st_size != 40) {
                close (fin);
                preferences.ShowStartingLineups = 1;
                preferences.IncludeUCTeams = 1;
                preferences.PlaySounds = 1;
                preferences.SpeakPBP = 0;
                preferences.ShowPlayerPics = 1;
                preferences.ShowTDIBAtBoot = 1;
                preferences.MovingPlayerPics = 0;
                preferences.AssumeAllYears = 0;
                preferences.Speed_sec = 1;
                preferences.Speed_nsec = 0;

                msg[0] = Line1;
                msg[1] = Line2;
                msg[2] = Line3;
                msg[3] = Line4;
                outMessage (msg);

                return;
            }
        close (fin);
    }

    /* load the NSB rc file if it exists */
    if ((rc = fopen (DefSetPath, "r")) != NULL) {
        x = fread (&preferences, sizeof preferences, 1, rc);
        fclose (rc);
    }
    else {
        preferences.ShowStartingLineups = 1;
        preferences.IncludeUCTeams = 1;
        preferences.PlaySounds = 1;
        preferences.SpeakPBP = 0;
        preferences.ShowPlayerPics = 1;
        preferences.ShowTDIBAtBoot = 1;
        preferences.MovingPlayerPics = 0;
        preferences.AssumeAllYears = 0;
        preferences.Speed_sec = 1;
        preferences.Speed_nsec = 0;
    }

    /* load the NSB AUTOPLAY rc file if it exists */
    path2ap[0] = '\0';
    strcpy (&path2ap[0], getenv ("HOME"));
    strcat (&path2ap[0], "/.NSBautoplayrc");

    if ((rc = fopen (path2ap, "r")) != NULL) {
        x = fread (&autoplay, sizeof autoplay, 1, rc);
        fclose (rc);
    }
    else {
        autoplay.active = autoplay.linescore = autoplay.boxscore = autoplay.standings = autoplay.injury = 0;
        autoplay.catl.hitting.games = autoplay.catl.hitting.atbats = autoplay.catl.hitting.runs = autoplay.catl.hitting.hits = autoplay.catl.hitting.doubles =
          autoplay.catl.hitting.triples = autoplay.catl.hitting.homers = autoplay.catl.hitting.rbis = autoplay.catl.hitting.bb = autoplay.catl.hitting.so =
          autoplay.catl.hitting.hbp = autoplay.catl.hitting.dp = autoplay.catl.hitting.sb = autoplay.catl.hitting.cs = autoplay.catl.hitting.ibb =
          autoplay.catl.hitting.sh = autoplay.catl.hitting.sf = autoplay.catl.hitting.ba = autoplay.catl.hitting.sa = autoplay.catl.hitting.oba = 0;
        autoplay.catl.pitching.games = autoplay.catl.pitching.gs = autoplay.catl.pitching.ip = autoplay.catl.pitching.wins = autoplay.catl.pitching.losses =
          autoplay.catl.pitching.sv = autoplay.catl.pitching.bfp = autoplay.catl.pitching.hits = autoplay.catl.pitching.db = autoplay.catl.pitching.tp =
          autoplay.catl.pitching.hr = autoplay.catl.pitching.runs = autoplay.catl.pitching.er = autoplay.catl.pitching.rbi = autoplay.catl.pitching.cg =
          autoplay.catl.pitching.gf = autoplay.catl.pitching.sho = autoplay.catl.pitching.svopp = autoplay.catl.pitching.sb = autoplay.catl.pitching.cs =
          autoplay.catl.pitching.bb = autoplay.catl.pitching.so = autoplay.catl.pitching.ibb = autoplay.catl.pitching.sh = autoplay.catl.pitching.sf =
          autoplay.catl.pitching.wp = autoplay.catl.pitching.b = autoplay.catl.pitching.hb = autoplay.catl.pitching.ab = autoplay.catl.pitching.era =
          autoplay.catl.pitching.pct = autoplay.catl.pitching.oppba = 0;
        for (x = 0; x < 7; x++)
            autoplay.catl.fielding.pos[x].games = autoplay.catl.fielding.pos[x].po = autoplay.catl.fielding.pos[x].dp = autoplay.catl.fielding.pos[x].a =
              autoplay.catl.fielding.pos[x].e = autoplay.catl.fielding.pos[x].pct = 0;
        autoplay.catl.fielding.pos[7].games = autoplay.catl.fielding.pos[6].pb = 0;
    }
}

