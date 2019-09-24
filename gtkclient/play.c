
/* play game(s) */

#include "gtknsbc.h"
#include "db.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "playgame.h"

/* watch 1 game computer versus computer */
void
PlayNSBWatch1GameComputerVsComputer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot play a game while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before playing a game.";
        outMessage (msg);
        return;
    }

    netgame = 0;
    watchsw = 1;
    Play1Game ('w');
}

/* play 1 game against the computer */
void
PlayNSB1GameAgainstComputer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot play a game while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before playing a game.";
        outMessage (msg);
        return;
    }

    netgame = watchsw = 0;
    Play1Game ('c');
}

/* play 1 game against another human on the same computer */
void
PlayNSB1GameAgainstHumanSameComputer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot play a game while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before playing a game.";
        outMessage (msg);
        return;
    }

    netgame = watchsw = 0;
    Play1Game ('h');
}

/* play 1 game against another human over a network */
void
PlayNSB1GameOverNetwork () {
    netgame = 1;
    watchsw = 0;
    Play1Game ('n');
}

/* play 1 game */
void
Play1Game (char which) {
    gint x, seldh, selrand;
    gchar wrk[10], *msg[5];

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    seldh = selrand = 0;

    if (!netgame || (netgame && !challenger_ind))
        /* if it's a game over a network then only the challengee gets to select playing a season game or not */
        if (LeagueUnderWay == 1) {
            lgame = InOutLeague ();
            if (!lgame) {
                sock_puts (sock, "X\n");  /* tell server we're canceling */
                return;
            }
        }
        else
            lgame = 2;
    else
        lgame = 2;

    if (lgame == 1) {
        x = CheckLock ();
        if (x) {
            if (x == 2)
                msg[0] = "Cannot open nor create league lock file.";
            else
                if (x == 3)
                    msg[0] = "Cannot open league lock file.";
                else {
                    msg[0] = "Another process is running games in your league.";
                    msg[1] = "Try again later.";
                }
            if (x == -1)
                close (fdlock);
            outMessage (msg);
            sock_puts (sock, "X\n");  /* tell server to cancel */
            return;
        }

        if (watchsw) {
            sock_puts (sock, "W\n");  /* tell server we want to watch the next scheduled game in our season */
            strcpy (&work[0], "Watch 1 season game.\n");
            Add2TextWindow (&work[0], 0);
        }
        else
            if (which == 'c') {
                ShowVs ();
                sock_puts (sock, "P\n");  /* tell server we want to play the next scheduled game in our season against the computer */
                ManageVH ();
                strcpy (&work[0], "Play against computer 1 game in season.\n");
                Add2TextWindow (&work[0], 0);
            }
            else {
                if (!netgame || (netgame && !challenger_ind)) {
                    /* if it's a game over a network then only the challengee will tell its child server process */
                    ShowVs ();
                    sock_puts (sock, "H\n");  /* tell server we want to play the next scheduled game in our season
                                                 against another human on the same computer */
                }
                ManageV = ManageH = 1;
                strcpy (&work[0], "Play against another human 1 game in season.\n");
                Add2TextWindow (&work[0], 0);
            }
    }
    else {
        if (!netgame || (netgame && !challenger_ind))
            /* if it's a game over a network then only the challengee gets to select the teams and whether or not to use a DH */
            if (!SelRandomTeams ()) {
                selrand = 1;
                if (!SelDesignatedHitter ()) {
                    seldh = 1;
                    dh = 1;
                }
                else
                    dh = 0;
            }

        if (watchsw)
            if (!selrand)
                sock_puts (sock, "w\n");  /* tell server we want to watch a non-season game */
            else {
                wrk[0] = 'w';                     /* non-season game */
                if (preferences.IncludeUCTeams)
                    wrk[1] = 'R';                 /* include user-created teams in randomness */
                else
                    wrk[1] = 'r';                 /* do not include user-created teams in randomness */
                if (seldh)
                    wrk[2] = '1';                 /* DH */
                else
                    wrk[2] = '0';                 /* no DH */
                wrk[3] = '\n';
                wrk[4] = '\0';
                sock_puts (sock, &wrk[0]);
            }
        else
            if (which == 'c') {
                if (!selrand)
                    sock_puts (sock, "p\n");  /* tell server we want to play a non-season game against the computer */
                else {
                    wrk[0] = 'p';                     /* non-season game against computer */
                    if (preferences.IncludeUCTeams)
                        wrk[1] = 'R';                 /* include user-created teams in randomness */
                    else
                        wrk[1] = 'r';                 /* do not include user-created teams in randomness */
                    if (seldh)
                        wrk[2] = '1';                 /* DH */
                    else
                        wrk[2] = '0';                 /* no DH */
                    wrk[3] = '\n';
                    wrk[4] = '\0';
                    sock_puts (sock, &wrk[0]);
                }
                ManageVH ();
            }
            else {
                if (!netgame || (netgame && !challenger_ind)) {
                    /* if it's a game over a network then only the challengee will tell its child server process */
                    if (!selrand)
                        sock_puts (sock, "h\n");  /* tell server we want to play a non-season game against another human */
                    else {
                        wrk[0] = 'h';                     /* non-season game against another human */
                        if (preferences.IncludeUCTeams)
                            wrk[1] = 'R';                 /* include user-created teams in randomness */
                        else
                            wrk[1] = 'r';                 /* do not include user-created teams in randomness */
                        if (seldh)
                            wrk[2] = '1';                 /* DH */
                        else
                            wrk[2] = '0';                 /* no DH */
                        wrk[3] = '\n';
                        wrk[4] = '\0';
                        sock_puts (sock, &wrk[0]);
                    }
                }
                ManageV = ManageH = 1;
            }
        if ((!netgame || (netgame && !challenger_ind)) && !selrand) {
            /* get all available teams from server (for a non-season game) */
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                GotError ();
                close (fdlock);
                return;
            }
            if (!strncmp (&buffer[0], "-2", 2)) {
                gchar NoDir[256] = "There's a problem with the server.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                strcpy (&work[0], "Problem with server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &NoDir[0];
                outMessage (msg);

                close (fdlock);
                return;
            }
            if (!strlen (&buffer[0])) {
                gchar NoTeams[256] = "No teams available.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                strcpy (&work[0], "No teams available on server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &NoTeams[0];
                outMessage (msg);

                close (fdlock);
                return;
            }
        }
    }

    currenttype = which;
    offdialsw = defdialsw = lineupsw = clineupsw = EOSsw = EOPSsw = 0;

    if (lgame == 1) {
        /* a season game */
        if (SetUp2PlayGame (which))
            g_idle_add ((GSourceFunc) PlayTheGame, NULL);
    }
    else {
        /* a non-season game */
        if (!netgame || (netgame && !challenger_ind))
            if (selrand)
                SelectTeamsCompleted = 1;
            else
                SelectTeams (which);
        else
            SelectTeamsCompleted = 1;
        if (SelectTeamsCompleted) {
            if (SetUp2PlayGame (which))
                g_idle_add ((GSourceFunc) PlayTheGame, NULL);
        }
        else
            sock_puts (sock, "X\n");
    }
}

/* computer to select 2 teams at random? */
int
SelRandomTeams () {
    gint x;
    GtkWidget *dlgFile, *label;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Select Teams at Random?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    label = gtk_label_new ("Select One:");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Randomly Select Two Teams", 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "I'll Select the Teams", 1);

    gtk_widget_show (label);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    return (x);
}

/* use a designated hitter? */
int
SelDesignatedHitter () {
    gint x;
    GtkWidget *dlgFile, *label;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Use Designated Hitter?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    label = gtk_label_new ("Select One:");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Use a Designated Hitter", 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Do NOT Use a Designated Hitter", 1);

    gtk_widget_show (label);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    return (x);
}

/* play season game or non-season game? */
int
InOutLeague () {
    gint x;
    GtkWidget *dlgFile, *label;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Season Game or Non-Season Game?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    label = gtk_label_new ("Select One:");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Season Game", 1);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Non-Season Game", 2);

    gtk_widget_show (label);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    return (x);
}

/* manage the visiting team or the home team? */
void
ManageVH () {
    gint x;
    GtkWidget *dlgFile, *label;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Manage Visitors or Home Team?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    label = gtk_label_new ("Select One:");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Manage Visiting Team", 1);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Manage Home Team", 2);

    gtk_widget_show (label);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    if (x == 1) {
        ManageV = 1;
        ManageH = 0;
        sock_puts (sock, "V\n");
    }
    else {
        ManageV = 0;
        ManageH = 1;
        sock_puts (sock, "H\n");
    }
}

/* play games in a season */
gint gord;  /* 0 = games, 1 = days */

void
PlayNSBPortionOfLeague (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint howmany, x, err = 0;
    gchar Playing[256] = "Playing games.  Click CLOSE and possibly wait depending upon the number of games being played.",
          NoDir[256] = "You have no season.  First set up a season.", GamesOK[256] = "Completed playing games.",
          EndSeason[256] = "Reached end of regular season.",
          EndWS[256] = "Completed post-season.  ", Champs[256] = " WORLD CHAMPS !!", *msg[5];

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot play a portion of a season while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before playing.";
        outMessage (msg);
        return;
    }

    x = CheckLock ();
    if (x) {
        if (x == 2)
            msg[0] = "Cannot open nor create league lock file.";
        else
            if (x == 3)
                msg[0] = "Cannot open league lock file.";
            else {
                msg[0] = "Another process is running games in your league.";
                msg[1] = "Try again later.";
            }
        if (x == -1)
            close (fdlock);
        outMessage (msg);
        return;
    }

    netgame = 0;

    /* check for the presence of a season */
    sock_puts (sock, "S7\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "NOLEAGUE")) {
        strcpy (&work[0], "No season established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);
        close (fdlock);
        return;
    }

    howmany = HowMuchLeague ();
    if (!howmany) {
        /* CANCEL */
        close (fdlock);
        return;
    }

    strcpy (&buffer1[0], "l");
    if (gord)
        buffer1[1] = 'd';
    else
        buffer1[1] = 'g';
    buffer1[2] = '\0';
    strcat (&buffer1[0], (char *) cnvt_int2str (howmany, 'l'));
    strcat (&buffer1[0], "\n");
    sock_puts (sock, buffer1);

    msg[0] = &Playing[0];
    outMessage (msg);

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        err = 1;
    }
    if (!strcmp (&buffer[0], "NO LEAGUE")) {
        /* this happens if the user has no season established */
        strcpy (&work[0], "No season established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);
        close (fdlock);
        return;
    }
    if (!strcmp (&buffer[0], "ERROR") || !strcmp (&buffer[0], "CANNOT PLAY")) {
        GotError ();
        err = 1;
    }

    if (!err) {
        strcpy (&work[0], "Played games in a season on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 0);
    }

    if (!strcmp (&buffer[0], "OK") && !err) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &GamesOK[0];
        outMessage (msg);
    }
    if (!strcmp (&buffer[0], "EOS")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Completed regular season.\n");
        Add2TextWindow (&work[0], 0);

        msg[0] = &EndSeason[0];
        outMessage (msg);
    }

    if (!strncmp (&buffer[0], "EOPS", 4)) {
        gint x, y, teami, yeari;
        gchar teamc[10], yearc[10], uctname[50];

        for (y = 0; y < 4; y++) {
            yearc[y] = buffer[4 + y];
            teamc[y] = buffer[8 + y];
        }
        teamc[4] = yearc[4] = '\0';
        teami = atoi (&teamc[0]);
        yeari = atoi (&yearc[0]);
        if (yeari) {
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (teaminfo[y].id == teami)
                    break;
        }
        else
            strcpy (&uctname[0], &buffer[12]);

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Completed post-season.\n");
        SetLeagueUnderWay (2);
        Add2TextWindow (&work[0], 0);

        msg[0] = &EndWS[0];
        if (yeari) {
            msg[1] = &yearc[0];
            msg[2] = &teaminfo[y].teamname[0];
            msg[3] = &Champs[0];
            g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/bat.wav");
        }
        else {
            msg[1] = &uctname[0];
            msg[2] = &Champs[0];
            g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/bat.wav");
        }
        outMessage (msg);
    }

    if (!err) {
        sock_gets (sock, &buffer[0], sizeof buffer);
        if (!strcmp (&buffer[0], "FUCKED"))
            GotError ();
    }
    close (fdlock);
}

/* determine number of games/days to play in a season */
gint
HowMuchLeague () {
    gint x, y, z;
    GtkWidget *dlgFile, *hbox, *label, *spinner, *combotype;
    GtkAdjustment *adj;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Number of Games/Days to Play in a Season");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete-event", G_CALLBACK (gtk_widget_destroy), dlgFile);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Play ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    adj = (GtkAdjustment *) gtk_adjustment_new (100.0, 1.0, 999.0, 25.0, 100.0, 0.0);
    spinner = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
    gtk_widget_set_size_request (spinner, 65, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_container_add (GTK_CONTAINER (hbox), spinner);

    combotype = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (combotype), "days");
    gtk_combo_box_append_text (GTK_COMBO_BOX (combotype), "games");
    gtk_combo_box_set_active (GTK_COMBO_BOX (combotype), 0);
    gtk_container_add (GTK_CONTAINER (hbox), combotype);

    label = gtk_label_new (" in your season.");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_OK, 1);
    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 1);

    gtk_widget_show_all (hbox);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));

    y = gtk_combo_box_get_active (GTK_COMBO_BOX (combotype));
    z = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
    gtk_widget_destroy (dlgFile);

    if (x) {
        /* OK */
        if (!y)
            gord = 1;
        else
            gord = 0;
        return (z);
    }
    else
        /* CANCEL */
        return (x);
}

void
ShowVs () {
    gint x;
    gchar bufin[256], next[256] = "The next game in your season is the ", *msg[5];

    sock_puts (sock, "N\n");
    sock_gets (sock, &bufin[0], sizeof (bufin));

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    strcat (&next[0], &bufin[0]);

    msg[0] = &next[0];

    outMessage (msg);
}

