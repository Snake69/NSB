/* play a game */

#define GTK_ENABLE_BROKEN  /* text doesn't work without this */

#include <time.h>
#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"
#include "playgame.h"

gchar gameplaytitle[200];

gint
SetUp2PlayGame (char which) {
    GtkLabel *label;
    gchar work[500];
    gint x, y, sk;

    /* set initial gameplay speed */
    speed.tv_sec = preferences.Speed_sec;
    speed.tv_nsec = preferences.Speed_nsec;

    /* set up for speaking play-by-play */
    playbyplay[0] = '\0';
    Speaking = curbeg = 0;

    if (!netgame || (netgame && !challenger_ind))
        sk = sock;
    else
        sk = sockhvh;

    if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return 0;
    }
    if (!strcmp (&buffer[0], "Not enough teams") || !strcmp (&buffer[0], "NO LEAGUE") || !strcmp (&buffer[0], "ERROR")) {
        gchar NoTeams[256] = "There's not enough teams to play a game.",
              NoLeague[256] = "A season needs to be created first.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (!strcmp (&buffer[0], "Not enough teams")) {
            strcpy (&work[0], "Not enough teams to play a game.\n");
            msg[0] = &NoTeams[0];
        }
        if (!strcmp (&buffer[0], "NO LEAGUE")) {
            strcpy (&work[0], "Attempted to play a season game with no season established.\n");
            msg[0] = &NoLeague[0];
        }

        if (!strcmp (&buffer[0], "ERROR"))
            GotError ();
        else {
            Add2TextWindow (&work[0], 1);
            outMessage (msg);
        }

        return 0;
    }
    if (netgame && !strcmp (&buffer[0], "CANCEL")) {
        /* challengee cancelled */
        gchar *msg[5], Cancel[256] = "The challengee cancelled the game.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &Cancel[0];
        outMessage (msg);

        close (sk);
        netgame = 0;
        return 0;
    }

    if (lgame == 1) {
        gint z;

        for (x = 0; x < 4; x++)
            vteamyr[x] = buffer[x];
        vteamyr[x] = '\0';

        for (y = 0; buffer[x] != ' '; x++, y++)
            visiting_team[y] = buffer[x];
        visiting_team[y] = '\0';

        for (z = 0, y = x + 1, x++; x < (y + 4); x++, z++)
            hteamyr[z] = buffer[x];
        hteamyr[z] = '\0';

        for (y = 0; x < strlen (&buffer[0]); x++, y++)
            home_team[y] = buffer[x];
        home_team[y] = '\0';

        for (x = 0; x <= NUMBER_OF_TEAMS; x++) {
            if (!strcmp (&teaminfo[x].filename[0], &visiting_team[0]))
                VisitingTeamID = teaminfo[x].id;
            if (!strcmp (&teaminfo[x].filename[0], &home_team[0]))
                HomeTeamID = teaminfo[x].id;
        }

        if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
            GotError ();
            return 0;
        }
        dh = buffer[0] - '0';
    }
    else {
        gint z;

        if (buffer[0] != 'U') {
            for (x = 0; x < 4; x++)
                vteamyr[x] = buffer[x];
            vteamyr[x] = '\0';
        }
        else {
            x = 0;
            strcpy (&vteamyr[0], "0000");
        }

        for (y = 0; buffer[x] != ' '; x++, y++)
            visiting_team[y] = buffer[x];
        visiting_team[y] = '\0';

        if (buffer[x + 1] != 'U') {
            for (z = 0, y = x + 1, x++; x < (y + 4); x++, z++)
                hteamyr[z] = buffer[x];
            hteamyr[z] = '\0';
        }
        else {
            x++;
            strcpy (&hteamyr[0], "0000");
        }

        for (y = 0; x < strlen (&buffer[0]); x++, y++)
            home_team[y] = buffer[x];
        home_team[y] = '\0';

        if (vteamyr[0] == '0')
            VisitingTeamID = 0;
        else
            for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                if (!strcmp (&teaminfo[x].filename[0], &visiting_team[0]))
                    VisitingTeamID = teaminfo[x].id;
        if (hteamyr[0] == '0')
            HomeTeamID = 0;
        else
            for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                if (!strcmp (&teaminfo[x].filename[0], &home_team[0]))
                    HomeTeamID = teaminfo[x].id;
    }
    if (!watchsw) {
        if (netgame && challenger_ind) {
            /* the challenger in a game over a network does not know if this is a game with a DH or not since the
               challengee determines whether or not to use a DH ... the server will let the challenger know here */
            if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
                GotError ();
                return 0;
            }
            dh = buffer[0] - '0';
        }
        if (!netgame || (netgame && !challenger_ind)) {
            if (!DoLineup (0))
                /* the computer will determine the starting lineup */
                sock_puts (sock, "FORGET IT\n");
        }
        if (which == 'h' || which == 'H' || (netgame && challenger_ind))
            /* if a human is playing against another human then allow both to set their lineups */
            if (!DoLineup (1))
                /* the computer will determine the starting lineup */
                /* in a network game the challenger always manages the visitors */
                sock_puts (sk, "FORGET IT\n");
    }

    ShowLineups ();
    GetIDs ();
    if ((watchsw && (speed.tv_sec || speed.tv_nsec)) || !watchsw)
        /* if the user is watching a fast game don't play the "Play Ball!" sound */
        g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/PlayBall.wav");

    savepitchername[0] = savebattername[0] = '\0';
    pitcherpicwin = batterpicwin = NULL;
    gamewin = gtk_dialog_new ();

    SetGWinTitle ();
    gtk_window_set_default_size (GTK_WINDOW (gamewin), 600, 230);
    gtk_window_set_modal (GTK_WINDOW (gamewin), TRUE);
    gtk_signal_connect (GTK_OBJECT (gamewin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->vbox), vbox, TRUE, TRUE, 0);

    /* add two tables within the main box/window */
    grview = gtk_table_new (12, 18, FALSE);
    pbyp = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (pbyp), 0, 2);
    gtk_table_set_col_spacing (GTK_TABLE (pbyp), 0, 2);
    gtk_container_add (GTK_CONTAINER (vbox), grview);
    gtk_container_add (GTK_CONTAINER (vbox), pbyp);

    /* add some static stuff to the upper table */
    if (vteamyr[0] != '0') {
        strcpy (&work[0], &vteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], GetTeamName (VisitingTeamID));
    }
    else
        strcpy (&work[0], &visiting_team[0]);
    label = g_object_new (GTK_TYPE_LABEL, "label", &work[0], NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 1, 2, 1, 2);
    if (hteamyr[0] != '0') {
        strcpy (&work[0], &hteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], GetTeamName (HomeTeamID));
    }
    else
        strcpy (&work[0], &home_team[0]);
    label = g_object_new (GTK_TYPE_LABEL, "label", &work[0], NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 1, 2, 2, 3);
    label = g_object_new (GTK_TYPE_LABEL, "label", "R", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 12, 13, 0, 1);
    label = g_object_new (GTK_TYPE_LABEL, "label", "H", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 13, 14, 0, 1);
    label = g_object_new (GTK_TYPE_LABEL, "label", "E", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 14, 15, 0, 1);
    label = g_object_new (GTK_TYPE_LABEL, "label", "PITCHER -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 0, 1, 4, 5);
    label = g_object_new (GTK_TYPE_LABEL, "label", "BATTER -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 0, 1, 5, 6);
    label = g_object_new (GTK_TYPE_LABEL, "label", "ON DECK -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 0, 1, 7, 8);
    label = g_object_new (GTK_TYPE_LABEL, "label", "IN THE HOLE -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 0, 1, 8, 9);
    label = g_object_new (GTK_TYPE_LABEL, "label", "ON FIRST", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 16, 17, 4, 5);
    label = g_object_new (GTK_TYPE_LABEL, "label", "ON SECOND", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 16, 17, 5, 6);
    label = g_object_new (GTK_TYPE_LABEL, "label", "ON THIRD", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 16, 17, 6, 7);
    for (x = 0; x < 10; x++) {
        if (x == 9)
            innlabel[x] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        else
            innlabel[x] = g_object_new (GTK_TYPE_LABEL, "label", cnvt_int2str (x + 1, 'l'), NULL);
        gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (innlabel[x]), x + 2, x + 3, 0, 1);
        for (y = 0; y < 2; y++) {
            innrlabel[x][y] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
            gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (innrlabel[x][y]), x + 2, x + 3, y + 1, y + 2);
        }
    }
    for (x = 0; x < 2; x++) {
        totrlabel[x] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (totrlabel[x]), 12, 13, x + 1, x + 2);
        tothlabel[x] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (tothlabel[x]), 13, 14, x + 1, x + 2);
        totelabel[x] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (totelabel[x]), 14, 15, x + 1, x + 2);
    }
    pitcherlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (pitcherlabel), 1, 2, 4, 5);
    outlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (outlabel), 3, 4, 3, 4);
    outslabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (outslabel), 4, 5, 3, 4);
    label = g_object_new (GTK_TYPE_LABEL, "label", "--", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 5, 6, 3, 4);
    halfilabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (halfilabel), 6, 7, 3, 4);
    label = g_object_new (GTK_TYPE_LABEL, "label", "of", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 7, 8, 3, 4);
    label = g_object_new (GTK_TYPE_LABEL, "label", "the", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (label), 8, 9, 3, 4);
    ilabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (ilabel), 9, 10, 3, 4);
    batterlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (batterlabel), 1, 2, 5, 6);
    firstlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (firstlabel), 15, 16, 4, 5);
    secondlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (secondlabel), 15, 16, 5, 6);
    thirdlabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (thirdlabel), 15, 16, 6, 7);
    decklabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (decklabel), 1, 2, 7, 8);
    holelabel = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (holelabel), 1, 2, 8, 9);
    for (x = 0; x < 4; x++) {
        msglabel[x] = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        gtk_table_attach_defaults (GTK_TABLE (grview), GTK_WIDGET (msglabel[x]), 16, 17, x + 7, x + 8);
    }

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->vbox), separator, FALSE, TRUE, 0);

    prpbpbutton = gtk_button_new_with_label ("Print Play-By-Play");
    gtk_signal_connect (GTK_OBJECT (prpbpbutton), "clicked", G_CALLBACK (PrintPBYP), gamewin);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->action_area), prpbpbutton, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (prpbpbutton), FALSE);

    boxbutton = gtk_button_new_with_label ("Boxscore");
    gtk_signal_connect (GTK_OBJECT (boxbutton), "clicked", G_CALLBACK (DoBoxscore), gamewin);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->action_area), boxbutton, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (boxbutton), FALSE);

    speedupbutton = gtk_button_new_with_label ("Speed Up Game");
    gtk_signal_connect (GTK_OBJECT (speedupbutton), "clicked", G_CALLBACK (SpeedUpGame), gamewin);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->action_area), speedupbutton, TRUE, TRUE, 0);

    slowdownbutton = gtk_button_new_with_label ("Slow Down Game");
    gtk_signal_connect (GTK_OBJECT (slowdownbutton), "clicked", G_CALLBACK (SlowDownGame), gamewin);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->action_area), slowdownbutton, TRUE, TRUE, 0);

    canbutton = gtk_button_new_with_label ("CANCEL");
    gtk_signal_connect (GTK_OBJECT (canbutton), "clicked", G_CALLBACK (CancelGame), gamewin);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gamewin)->action_area), canbutton, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (canbutton), FALSE);

    /* create a text widget */
    pbptext = gtk_text_new (NULL, NULL);
    gtk_text_set_editable (GTK_TEXT (pbptext), FALSE);
    gtk_table_attach (GTK_TABLE (pbyp), pbptext, 0, 1, 0, 2,
                                                   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* add a vertical scrollbar to the text widget */
    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (pbptext)->vadj);
    gtk_table_attach (GTK_TABLE (pbyp), vscrollbar, 1, 2, 0, 2, GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* get the system color map and allocate the color red */
    cmap = gdk_colormap_get_system ();
    color.red = 0xffff;
    color.green = 0;
    color.blue = 0;
    if (!gdk_color_alloc (cmap, &color))
        g_error ("couldn't allocate color");

    /* realizing a widget creates a window for it, ready for us to insert some text */
    gtk_widget_realize (pbptext);

    /* show the teams playing */
    strcpy (&work[0], "\n The ");
    if (vteamyr[0] != '0') {
        strcat (&work[0], &vteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], (char *) GetTeamName (VisitingTeamID));
    }
    else
        strcat (&work[0], &visiting_team[0]);
    strcat (&work[0], "\n          at\n the ");
    if (hteamyr[0] != '0') {
        strcat (&work[0], &hteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], (char *) GetTeamName (HomeTeamID));
    }
    else
        strcat (&work[0], &home_team[0]);
    strcat (&work[0], ".\n\n");
    Add2PBPWindow (&work[0], 0);

    /* get game start time */
    time (&dt);
    dc = *localtime (&dt);
    strftime (time_hr, sizeof time_hr, "%H", &dc);
    strftime (time_min, sizeof time_min, "%M", &dc);
    strftime (time_sec, sizeof time_sec, "%S", &dc);
    dc1.hr = atoi (&time_hr[0]);
    dc1.min = atoi (&time_min[0]);
    dc1.sec = atoi (&time_sec[0]);

    gtk_window_set_deletable (GTK_WINDOW (gamewin), FALSE);
    gtk_widget_show_all (gamewin);

    return 1;
}

/* add some text to the scrolling play-by-play area in the game window */
void
Add2PBPWindow (char *textbuf, int red) {
    gint sbUpperLim;

    /* scrollbar near upper limit? */
    if (gtk_adjustment_get_value (GTK_TEXT (pbptext)->vadj) >= (GTK_TEXT (pbptext)->vadj->upper - (GTK_TEXT (pbptext)->vadj->page_size * 2)))
        sbUpperLim = 1;
    else
        sbUpperLim = 0;

    /* freeze the text widget, ready for multiple updates */
    gtk_text_freeze (GTK_TEXT (pbptext));

    /* put text */
    if (red)
        gtk_text_insert (GTK_TEXT (pbptext), NULL, &color, NULL, textbuf, -1);
    else
        gtk_text_insert (GTK_TEXT (pbptext), NULL, &pbptext->style->black, NULL, textbuf, -1);

    /* force scrollbar to upper limit only if it was already near the upper limit prior to the text insertion */
    if (sbUpperLim)
        gtk_adjustment_set_value (GTK_TEXT (pbptext)->vadj, GTK_TEXT (pbptext)->vadj->upper - GTK_TEXT (pbptext)->vadj->page_size);

    /* thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (pbptext));

    /* show text */
    gtk_widget_show (pbptext);

    /* save play-by-play */
    strcat (&playbyplay[0], textbuf);

    /* speak play-by-play */
    if (preferences.SpeakPBP) {
        if (!curbeg && speed.tv_sec >= 3)
            sleep (1);  /* don't overlap with other voice saying "Play Ball!" */
        g_thread_create (SpeakPlayByPlay, (gpointer) NULL, FALSE, NULL);
    }
}

/* add text to play by play window and change the dynamic game status on the fly */
int
PlayTheGame () {
    gint x, y, z, sk;

    if (netgame && challenger_ind)
        sk = sockhvh;
    else
        sk = sock;

    pitpicwinlocX = (int) gdk_screen_width () / 2 - ((int) gdk_screen_width () / 6);
    batpicwinlocX = (int) gdk_screen_width () / 2 - ((int) gdk_screen_width () / 20);
    pitpicwinlocY = batpicwinlocY = 0;

    /* if game speed cannot get any faster then desensitize the "Speed Up Game" button */
    if (!speed.tv_sec && !speed.tv_nsec)
        gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
    else
        gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), TRUE);

    if (offdialsw) {
        if (offdialsw == 2) {
            buffer1[0] = cresp;
            buffer1[1] = '\n';
            buffer1[2] = '\0';
            sock_puts (sk, buffer1);
            if (buffer1[0] == 'L')
                lineupsw = 1;
            offdialsw = 0;
            return TRUE;
        }
        else
            return TRUE;
    }

    if (defdialsw) {
        if (defdialsw == 2) {
            defdialsw = 0;
            return TRUE;
        }
        else
            return TRUE;
    }

    if (lineupsw) {
        if (lineupsw == 1) {
            SeeLineup (sk);
            return TRUE;
        }
        if (lineupsw == 2)
            return TRUE;
        if (lineupsw == 3) {
            lineupsw = 0;
            return TRUE;
        }
    }

    if (clineupsw) {
        if (clineupsw == 1) {
            ChangeLineup (sk);
            return TRUE;
        }
        if (clineupsw == 2)
            return TRUE;
        if (clineupsw == 3) {
            clineupsw = 0;
            return TRUE;
        }
    }

    if (EOSsw) {
        if (EOSsw == 1) {
            gint x;
            gchar EndSeason[256] = "Reached end of regular season.", *msg[5];

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "Completed regular season.\n");
            Add2TextWindow (&work[0], 0);

            msg[0] = &EndSeason[0];
            outEOMessage (msg);

            sock_gets (sk, &buffer[0], sizeof buffer);
            if (!strcmp (&buffer[0], "FUCKED"))
                GotEOError ();

            return TRUE;
        }
        if (EOSsw == 2)
            return TRUE;
        if (EOSsw == 3) {
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }
    }

    if (EOPSsw) {
        if (EOPSsw == 1) {
            gint x, y, teami, yeari;
            gchar teamc[10], yearc[10], EndWS[256] = "Completed post-season.  ",
                  Champs[256] = " WORLD CHAMPS !!", *msg[5], uctname[50];

            for (y = 0; y < 4; y++) {
                yearc[y] = buffer[4 + y];
                teamc[y] = buffer[8 + y];
            }
            teamc[4] = yearc[4] = '\0';
            yeari = atoi (&yearc[0]);
            teami = atoi (&teamc[0]);
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
            Add2TextWindow (&work[0], 0);
            SetLeagueUnderWay (2);

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
            outEOMessage (msg);

            sock_gets (sk, &buffer[0], sizeof buffer);
            if (!strcmp (&buffer[0], "FUCKED"))
                GotEOError ();

            return TRUE;
        }
        if (EOPSsw == 2)
            return TRUE;
        if (EOPSsw == 3) {
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }
    }

    if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
        gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
        KillPlayerPics ();
        close (fdlock);
        return FALSE;
    }
    if (!strcmp (&buffer[0], "fuckup")) {
        GotError ();
        gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
        gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
        KillPlayerPics ();
        close (fdlock);
        return FALSE;
    }

    if (!strncmp (&buffer[0], "PB", 2))
        g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/JackBuck.wav");

    if (!strncmp (&buffer[0], "BD", 2)) {
        /* the server is sending us info for the play by play */
        int zz;

        for (x = strlen (&buffer[0]), z = y = 0; y < x; y++)
             if (!strncmp (&buffer[y], "score", 5)) {
                 for (x = strlen (&buffer[0]), zz = y = 0; y < x; y++)
                     if (!strncmp (&buffer[y], "trying to", 9))
                         zz = 1;
                 if (zz)
                     break;
                 z = 1;  /* when there's a run scoring play show the text as red */
                 break;
             }
        Add2PBPWindow (&buffer[2], z);
        if (strlen (&buffer[0]) > 2)
            if (buffer[strlen (&buffer[0]) - 1] != '!')
                Add2PBPWindow (".\n", z);
            else
                Add2PBPWindow ("\n", 0);
        else
            Add2PBPWindow ("\n", 0);

        if (speed.tv_nsec == 1)
            speed.tv_nsec = 250000000;
        if (speed.tv_nsec == 2)
            speed.tv_nsec = 500000000;
        if (speed.tv_nsec == 3)
            speed.tv_nsec = 750000000;
        nanosleep (&speed, NULL);      /* delay the text display */
        /* convert back to numbers easier to work with */
        if (speed.tv_nsec == 250000000)
            speed.tv_nsec = 1;
        if (speed.tv_nsec == 500000000)
            speed.tv_nsec = 2;
        if (speed.tv_nsec == 750000000)
            speed.tv_nsec = 3;
    }

    if (!strncmp (&buffer[0], "TD", 2))
        /* the server is sending us info for the top half of the display */
        UpdateTop ();

    if (!strncmp (&buffer[0], "DC", 2))
        /* the server is looking for a command from the defense */
        DefensiveCommand (&buffer[2], sk);  /* the server is notified in DefensiveCommand() */

    if (!strncmp (&buffer[0], "OC", 2)) {
        /* the server is looking for a command from the offense */
        if (currenttype == 'h' && (currentd == '1' || currentd == '2' || currentd == '3' || currentd == 'I')) {
            /* if we're playing a human vs human game and the defensive choice is to attempt a pickoff or to issue an
               intentional walk then we don't want to look for an offensive decision and it doesn't matter
               what we send to the server as long as it's valid */
            buffer1[0] = 'N';
            buffer1[1] = '\n';
            buffer1[2] = '\0';
            sock_puts (sk, buffer1);
        }
        else
            OffensiveCommand (&buffer[2]);
    }

    if (!strncmp (&buffer[0], "LU", 2))
        /* the human player needs to make a change to his lineup */
        clineupsw = 1;

    if (!strcmp (&buffer[0], "BDGame over")) {
        /* get game finish time */
        time (&dt);
        dc = *localtime (&dt);
        strftime (time_hr, sizeof time_hr, "%H", &dc);
        strftime (time_min, sizeof time_min, "%M", &dc);
        strftime (time_sec, sizeof time_sec, "%S", &dc);
        dc2.hr = atoi (&time_hr[0]);
        dc2.min = atoi (&time_min[0]);
        dc2.sec = atoi (&time_sec[0]);

        /* returning FALSE stops the call of this function (and the playing of the game) but
           we don't want to stop until after the boxscore data is received */
    }

    if (!strncmp (&buffer[0], "BX", 2)) {
        /* data for an end-of-game boxscore */

        /* first, save the buffer area containing some boxscore data then get the complete stats for both teams */
        strcpy (&buffer1[0], &buffer[0]);
        if (get_stats (sk, 'v', 0)) {
            GotError ();
            gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
            gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }
        if (get_stats (sk, 'h', 0)) {
            GotError ();
            gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
            gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }
        if (get_stats (sk, 'a', 0)) {
            GotError ();
            gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
            gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }
        if (get_stats (sk, 'b', 0)) {
            GotError ();
            gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
            gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
            gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);
            KillPlayerPics ();
            close (fdlock);
            return FALSE;
        }

        /* figure elapsed time of game */
        if (dc2.sec < dc1.sec) {
            dc2.sec += 60;
            if (dc2.min == 0) {
                dc2.min += 60;
                if (dc2.hr == 0)
                    dc2.hr += 24;
                dc2.hr--;
            }
         dc2.min--;
        }
        x = dc2.sec - dc1.sec;

        if (dc2.min < dc1.min) {
            dc2.min += 60;
            if (dc2.hr == 0)
                dc2.hr += 24;
            dc2.hr--;
        }
        y = dc2.min - dc1.min;

        if (dc2.hr < dc1.hr)
            dc2.hr += 24;
        z = dc2.hr - dc1.hr;

        cnvt_int2str (z, 'r');
        game_time[0] = resultstr[5];
        game_time[1] = resultstr[6];
        cnvt_int2str (y, 'r');
        game_time[2] = resultstr[5];
        game_time[3] = resultstr[6];
        cnvt_int2str (x, 'r');
        game_time[4] = resultstr[5];
        game_time[5] = resultstr[6];

        gtk_button_set_label (GTK_BUTTON (canbutton), "DISMISS");
        gtk_widget_set_sensitive (GTK_WIDGET (speedupbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (slowdownbutton), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (prpbpbutton), TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (boxbutton), TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (canbutton), TRUE);

        if (lgame == 1) {
            sock_gets (sk, &buffer[0], sizeof buffer);

            if (!strcmp (&buffer[0], "EOS")) {
                EOSsw = 1;
                return TRUE;
            }

            if (!strncmp (&buffer[0], "EOPS", 4)) {
                EOPSsw = 1;
                return TRUE;
            }

            sock_gets (sk, &buffer[0], sizeof buffer);
            if (!strcmp (&buffer[0], "FUCKED"))
                GotError ();
        }

        KillPlayerPics ();
        close (fdlock);
        return FALSE;
    }
    return TRUE;
}

void
CancelGame (GtkWidget *widget, gpointer *pdata) {
    DestroyDialog (gamewin, gamewin);
}

void
SlowDownGame (GtkWidget *widget, gpointer *pdata) {
    speed.tv_nsec++;
    if (speed.tv_nsec > 3) {
        speed.tv_nsec = 0;
        speed.tv_sec++;
    }

    SetGWinTitle ();
}

void
SpeedUpGame (GtkWidget *widget, gpointer *pdata) {
    if (--speed.tv_nsec < 0) {
        if (--speed.tv_sec < 0)
            speed.tv_sec = speed.tv_nsec = 0;
        else
            speed.tv_nsec = 3;
    }

    SetGWinTitle ();
}

gchar rpbp[100000];

void
PrintPBYP (GtkWidget *widget, gpointer *pdata) {
    /* the play by play needs to be reformatted slightly for printing */
    gchar *cc = &playbyplay[0], Printing[256] = "Printing Play by Play ...", *msg[5];
    gint pbpend = strlen (&playbyplay[0]), x;

    rpbp[0] = '\0';
    while (cc < (&playbyplay[0] + pbpend)) {
        gchar *wcc;

        for (wcc = cc; *wcc != '\n' && *wcc != '\0'; wcc++);
        if ((wcc - cc) > 75) {
            for (wcc = cc + 75; *wcc != ' ' && wcc > cc; wcc--);
            for (; *wcc == ' ' && wcc > cc; wcc--);
            wcc++;
        }
        strncat (&rpbp[0], cc, (wcc - cc));
        strcat (&rpbp[0], "\n");
        for (cc = wcc + 1; *cc == ' '; cc++);
    }

    print (&rpbp[0]);

    strcpy (&work[0], "Print Play by Play.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

/* update the status info at the top of the game play display */
void
UpdateTop () {
    gint half_inning, innings, innplushalf, x, y, inn, bcol, dcol, line, truns[2], iwork;
    gchar work[50], w[50];

    truns[0] = truns[1] = 0;

    work[0] = buffer[2];
    work[1] = buffer[3];
    work[2] = '\0';
    half_inning = atoi (&work[0]);

    innings = half_inning / 2;
    innplushalf = half_inning / 2 + (half_inning % 2);

    /* ensure the header of the line score shows the right inning count in the case of extra innings */
    if (innplushalf > 9)
        for (x = 10, y = innplushalf; y > (innplushalf - 10); x--, y--)
            gtk_label_set_text (GTK_LABEL (innlabel[x - 1]), cnvt_int2str (y, 'l'));

    for (x = 1, inn = 0, bcol = 4, dcol = 1; inn <= innings; x++) {
        /* determine line on display to which to output for line score */
        if (((x - 1) % 2) == 0)
            line = 1;
        else
            line = 2;

        work[0] = buffer[bcol];
        work[1] = buffer[bcol + 1];
        work[2] = '\0';
        iwork = atoi (&work[0]);

        truns[line - 1] += iwork;           /* accumulate runs */

        /* if game goes into extra innings output only last ten innings */
        if (inn < (innings - 9)) {
            bcol += 2;
            if (line == 2)
                inn++;
            continue;
        }

        if (inn == innings && innings >= 8 && !iwork && line == 2 && truns[1] > truns[0]) {
            work[0] = 'x';
            work[1] = '\0';
        }
        else
            if (iwork < 10) {
                work[0] = buffer[bcol + 1];
                work[1] = '\0';
            }
            else {
                work[0] = buffer[bcol];
                work[1] = buffer[bcol + 1];
                work[2] = '\0';
            }
        gtk_label_set_text (GTK_LABEL (innrlabel[dcol - 1][line - 1]), work);
        bcol += 2;
        if (line == 2) {
            inn++;
            dcol++;
        }
    }

    /* move total runs to display */
    gtk_label_set_text (GTK_LABEL (totrlabel[0]), cnvt_int2str (truns[0], 'l'));
    gtk_label_set_text (GTK_LABEL (totrlabel[1]), cnvt_int2str (truns[1], 'l'));

    /* move total hits to display */
    for (x = 0; x < 2; x++, bcol += 2) {
        if (buffer[bcol] > '0') {
            work[0] = buffer[bcol];
            work[1] = buffer[bcol + 1];
            work[2] = '\0';
        }
        else {
            work[0] = buffer[bcol + 1];
            work[1] = '\0';
        }
        gtk_label_set_text (GTK_LABEL (tothlabel[x]), work);
    }

    /* move total errors to display */
    for (x = 0; x < 2; x++, bcol += 2) {
        if (buffer[bcol] > '0') {
            work[0] = buffer[bcol];
            work[1] = buffer[bcol + 1];
            work[2] = '\0';
        }
        else {
            work[0] = buffer[bcol + 1];
            work[1] = '\0';
        }
        gtk_label_set_text (GTK_LABEL (totelabel[x]), work);
    }

    /* move pitcher name */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    /* reverse name & save */
    strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
    strcat (&w[0], " ");
    strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
    bcol++;
    if (strcmp (&savepitchername[0], &w[0])) {
        strcpy (&savepitchername[0], &w[0]);
        /* move pitcher date of birth */
        for (x = 0; x < 8; x++)
            savepitcherdob[x] = buffer[bcol + x];
        savepitcherdob[8] = '\0';
        ShowPlayerPic (0);
    }
    gtk_label_set_text (GTK_LABEL (pitcherlabel), w);

    bcol += 8;

    /* move number of outs */
    work[0] = buffer[bcol];
    work[1] = '\0';
    gtk_label_set_text (GTK_LABEL (outlabel), work);
    if (buffer[bcol] != '1')
        gtk_label_set_text (GTK_LABEL (outslabel), "OUTS");
    else
        gtk_label_set_text (GTK_LABEL (outslabel), "OUT");

    /* display half-inning being played */
    if (half_inning < 18) {
        work[0] = ((half_inning / 2) + 1) + '0';
        work[1] = '\0';
    }
    else {
        work[0] = (((half_inning / 2) + 1) / 10) + '0';
        work[1] = (((half_inning / 2) + 1) % 10) + '0';
        work[2] = '\0';
    }
    if (half_inning < 2)
        strcat (&work[0], "st");
    else
        if (half_inning < 4)
            strcat (&work[0], "nd");
        else
            if (half_inning < 6)
                strcat (&work[0], "rd");
            else
                strcat (&work[0], "th");
    if (!(half_inning % 2))
        gtk_label_set_text (GTK_LABEL (halfilabel), "TOP");
    else
        gtk_label_set_text (GTK_LABEL (halfilabel), "BOT");
    gtk_label_set_text (GTK_LABEL (ilabel), work);

    bcol++;

    /* move batter name */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    /* reverse name */
    strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
    strcat (&w[0], " ");
    strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
    bcol++;
    if (strcmp (&savebattername[0], &w[0])) {
        strcpy (&savebattername[0], &w[0]);
        /* move batter date of birth */
        for (x = 0; x < 8; x++)
            savebatterdob[x] = buffer[bcol + x];
        savebatterdob[8] = '\0';
        ShowPlayerPic (1);
    }
    gtk_label_set_text (GTK_LABEL (batterlabel), w);

    bcol += 8;

    /* move name of the runner on first base */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    if (strlen (&work[0])) {
        /* reverse name */
        strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
        strcat (&w[0], " ");
        strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
        gtk_label_set_text (GTK_LABEL (firstlabel), w);
    }
    else
        gtk_label_set_text (GTK_LABEL (firstlabel), work);

    bcol++;

    /* move name of the runner on second base */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    if (strlen (&work[0])) {
        /* reverse name */
        strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
        strcat (&w[0], " ");
        strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
        gtk_label_set_text (GTK_LABEL (secondlabel), w);
    }
    else
        gtk_label_set_text (GTK_LABEL (secondlabel), work);

    bcol++;

    /* move name of the runner on third base */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    if (strlen (&work[0])) {
        /* reverse name */
        strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
        strcat (&w[0], " ");
        strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
        gtk_label_set_text (GTK_LABEL (thirdlabel), w);
    }
    else
        gtk_label_set_text (GTK_LABEL (thirdlabel), work);

    bcol++;

    /* move name of the player on deck */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    /* reverse name */
    strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
    strcat (&w[0], " ");
    strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
    gtk_label_set_text (GTK_LABEL (decklabel), w);

    bcol++;

    /* move name of the player in the hole */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++)
        work[x] = buffer[bcol];
    work[x] = '\0';
    /* reverse name */
    strcpy (&w[0], &work[index (&work[0], ',') - &work[0] + 2]);
    strcat (&w[0], " ");
    strncat (&w[0], &work[0], (index (&work[0], ',') - &work[0]));
    gtk_label_set_text (GTK_LABEL (holelabel), w);

    /* move status lines */
    for (bcol++, y = 0; y < 4; y++, bcol++) {
        for (x = 0; buffer[bcol] != ':'; x++, bcol++)
            work[x] = buffer[bcol];
        work[x] = '\0';
        gtk_label_set_text (GTK_LABEL (msglabel[y]), work);
    }
}

gchar boxinfo[8192];

void
DoBoxscore (GtkWidget *widget, gpointer *pdata) {
/*
   output an end-of-game boxscore
   some info to display will be in the global variable called buffer1[1024]
   all the individual player data will be in the home_cur and visitor_cur structures

   the format of the buffer1 record from the server looks like this:
   pos 1 - 2 = BX
   pos 3 - 4 = total innings in game
   pos 5 - 6 = number of runs scored by visiting team in top of 1st inning
   pos 7 - 8 = number of runs scored by home team in bottom of 1st inning
   pos 9 - 10 = number of runs scored by visiting team in top of 2nd inning
     etc (variable depending upon value of pos 3 - 4 (if the bottom of the
     last inning wasn't played then two blanks will be present in those positions))
   total left on base for the visiting team (2 pos)
   total left on base for the home team (2 pos)
   position 1 in batting order for visiting team ... numeric pointer to
     player (2 pos) and position played (2 pos)
     (these four positions can be repeated several times ... a colon marks end of entry for each batting order position)
   .
   .
   .
   position 9 in batting order for visiting team ... numeric pointer to player (2 pos) and position played (2 pos)
   position 1 in batting order for home team ... numeric pointer to
     player (2 pos) and position played (2 pos)
     (these four positions can be repeated several times ... a colon marks end of entry for each batting order position)
   .
   .
   .
   position 9 in batting order for home team ... numeric pointer to player (2 pos) and position played (2 pos)
   numeric pointer (2 pos) for each pitcher who appeared for visiting team followed by a colon
   numeric pointer (2 pos) for each pitcher who appeared for home team followed by a colon
*/
    GtkWidget *box1, *box2, *hbox, *separator, *prbutton, *disbutton, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    gtk_window_set_modal (GTK_WINDOW (gamewin), FALSE);

    dlgBox = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgBox), "Boxscore");
    gtk_window_set_default_size (GTK_WINDOW (dlgBox), 780, 500);
    gtk_window_set_modal (GTK_WINDOW (dlgBox), TRUE);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgBox)->vbox), box1, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (dlgBox), "delete_event", GTK_SIGNAL_FUNC (delete_dlg), 0);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
    gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
    gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);

    /* Create the GtkText widget */
    text = gtk_text_new (NULL, NULL);
    gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* Add a vertical scrollbar to the GtkText widget */
    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
    gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1, GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* Load a fixed font */
    fixed_font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

    /* Realizing a widget creates a window for it, ready for us to insert some text */
    gtk_widget_realize (text);

    /* Freeze the text widget, ready for multiple updates */
    gtk_text_freeze (GTK_TEXT (text));

    FillBoxInfo ();
    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, boxinfo, strlen (&boxinfo[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    prbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (prbutton), "clicked", G_CALLBACK (PrintBox), dlgBox);
    disbutton = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroydlgBox), dlgBox);
    gtk_box_pack_start (GTK_BOX (hbox), prbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgBox);
}

void
PrintBox (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Boxscore ...", *msg[5];
    gint x;

    print (&boxinfo[0]);

    strcpy (&work[0], "Print Boxscore.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
DestroydlgBox (GtkWidget *widget, gpointer *pdata) {
    DestroyDialog (dlgBox, dlgBox);
    gtk_window_set_modal (GTK_WINDOW (gamewin), TRUE);
}

gint
delete_dlg (GtkWidget *widget, GdkEventConfigure *event) {
    gtk_widget_destroy (GTK_WIDGET (widget));
    gtk_window_set_modal (GTK_WINDOW (gamewin), TRUE);
    return TRUE;
}

void
FillBoxInfo () {
    gint innings, x, y, z, inn, bcol, iwork, previ, truns[2], thits[2], addto, terrs[2], pos, indent = 0, singles_g,
        singles_s, ttb, prevm, line;
    gchar work[40][500], work1[5], workcur[100], initial;

    strcpy (&boxinfo[0], "\n");

    /* accumulate runs and determine who won game */
    for (x = truns[0] = truns[1] = 0; x < 25; x++) {
        truns[0] += visitor_cur.batters[x].hitting.runs;
        truns[1] += home_cur.batters[x].hitting.runs;
    }
    if (truns[0] > truns[1]) {
        if (vteamyr[0] != '0') {
            strcat (&boxinfo[0], &vteamyr[0]);
            strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], (char *) GetTeamName (VisitingTeamID));
        }
        else
            strcat (&boxinfo[0], &visiting_team[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) cnvt_int2str (truns[0], 'l'));
        strcat (&boxinfo[0], ", ");
        if (hteamyr[0] != '0') {
            strcat (&boxinfo[0], &hteamyr[0]);
            strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], (char *) GetTeamName (HomeTeamID));
        }
        else
            strcat (&boxinfo[0], &home_team[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) cnvt_int2str (truns[1], 'l'));
    }
    else {
        if (hteamyr[0] != '0') {
            strcat (&boxinfo[0], &hteamyr[0]);
            strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], (char *) GetTeamName (HomeTeamID));
        }
        else
            strcat (&boxinfo[0], &home_team[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) cnvt_int2str (truns[1], 'l'));
        strcat (&boxinfo[0], ", ");
        if (vteamyr[0] != '0') {
            strcat (&boxinfo[0], &vteamyr[0]);
            strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], (char *) GetTeamName (VisitingTeamID));
        }
        else
            strcat (&boxinfo[0], &visiting_team[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) cnvt_int2str (truns[0], 'l'));
    }
    strcat (&boxinfo[0], "\n\n");

    if (vteamyr[0] != '0') {
        strcat (&boxinfo[0], &vteamyr[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) GetTeamName (VisitingTeamID));
    }
    else
        strcat (&boxinfo[0], &visiting_team[0]);
    strcat (&boxinfo[0], " at ");
    if (hteamyr[0] != '0') {
        strcat (&boxinfo[0], &hteamyr[0]);
        strcat (&boxinfo[0], " ");
        strcat (&boxinfo[0], (char *) GetTeamName (HomeTeamID));
    }
    else
        strcat (&boxinfo[0], &home_team[0]);
    strcat (&boxinfo[0], " ");
    time (&dt);
    dc = *localtime (&dt);
    strcat (&boxinfo[0], (char *) cnvt_int2str ((dc.tm_mon + 1), 'l'));
    strcat (&boxinfo[0], "/");
    strcat (&boxinfo[0], (char *) cnvt_int2str (dc.tm_mday, 'l'));
    strcat (&boxinfo[0], "/");
    strcat (&boxinfo[0], (char *) cnvt_int2str ((dc.tm_year + 1900), 'l'));
    strcat (&boxinfo[0], " -- Final\n\n");

    /* the line score */
    strcpy (&work[0][0], "                                  1  2  3  4  5  6  7  8  9 ");
    work1[0] = buffer1[2];
    work1[1] = buffer1[3];
    work1[2] = '\0';
    innings = atoi (&work1[0]);
    if (innings > 9)
        strcat (&work[0][0], "10");

    if (vteamyr[0] != '0') {
        strcpy (&work[1][0], &vteamyr[0]);
        strcat (&work[1][0], " ");
        strcat (&work[1][0], (char *) GetTeamName (VisitingTeamID));
    }
    else {
        strcpy (&work[1][0], "     ");
        strcat (&work[1][0], &visiting_team[0]);
    }
    if (strlen (&work[1][0]) > 32)
        work[1][32] = '\0';
    else
        strncat (&work[1][0], "                      ", 32 - strlen (&work[1][0]));

    if (hteamyr[0] != '0') {
        strcpy (&work[2][0], &hteamyr[0]);
        strcat (&work[2][0], " ");
        strcat (&work[2][0], (char *) GetTeamName (HomeTeamID));
    }
    else {
        strcpy (&work[2][0], "     ");
        strcat (&work[2][0], &home_team[0]);
    }
    if (strlen (&work[2][0]) > 32)
        work[2][32] = '\0';
    else
        strncat (&work[2][0], "                      ", 32 - strlen (&work[2][0]));

    for (x = inn = 1, bcol = 4, line = 1; inn <= innings; x++) {
        /* determine line on display to output to */
        if (((x - 1) % 2) == 0)
            addto = 0;
        else
            addto = 1;
        /* if a game goes more than 100 innings we're screwed here ! */
        if (((inn % 10) == 1) && inn != 1 && addto == 0) {
            line += 4;
            work[line - 2][0] = work[line - 1][0] = work[line][0] = work[line + 1][0] = '\0';
            strcat (&work[line - 2][0], "\n");
            strcat (&work[line - 1][0], "                                 ");
            strcat (&work[line][0], "                                ");
            strcat (&work[line + 1][0], "                                ");
            for (y = inn; y <= innings && y < (inn + 10); y++) {
                strcat (&work[line - 1][0], (char *) cnvt_int2str (y, 'l'));
                strcat (&work[line - 1][0], " ");
            }
        }

        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        iwork = atoi (&work1[0]);

        /* check if we played the bottom half of the last inning */
        if (inn == innings && innings >= 9 && !iwork && addto == 1 && truns[1] > truns[0]) {
            strcat (&work[line + addto][0], "  x");
            inn++;
	        bcol += 2;
            continue;
        }

        work1[0] = ' ';
        work1[2] = buffer1[bcol + 1];
        work1[3] = '\0';
        if (iwork < 10)
            work1[1] = ' ';
        else
            work1[1] = buffer1[bcol];
        strcat (&work[line + addto][0], &work1[0]);

        bcol += 2;
        if (addto == 1)
            inn++;
    }
    if (innings > 10 && (innings % 10))
        for (x = innings % 10; x < 10; x++) {
            strcat (&work[line - 1][0], "   ");
            strcat (&work[line][0], "   ");
            strcat (&work[line + 1][0], "   ");
        }
    if (innings < 10) {
        strcat (&work[line][0], "   ");
        strcat (&work[line + 1][0], "   ");
        strcat (&work[line - 1][0], "  ");
    }
    /* account for a a game less than 9 innings (currently that would be only a forfeit) */
    while (inn < 10) {
        strcat (&work[line][0], "   ");
        strcat (&work[line + 1][0], "   ");

        inn++;
    }

    /* move total runs */
    if (innings < 11)
        strcat (&work[line - 1][0], " ");
    strcat (&work[line - 1][0], " R  H  E  LOB");
    if (truns[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (truns[0], 'l'));
    if (truns[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (truns[1], 'l'));

    /* move total hits */
    for (x = thits[0] = thits[1] = 0; x < 25; x++) {
        thits[0] += visitor_cur.batters[x].hitting.hits;
        thits[1] += home_cur.batters[x].hitting.hits;
    }
    if (thits[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (thits[0], 'l'));
    if (thits[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (thits[1], 'l'));

    /* move total errors */
    for (x = terrs[0] = terrs[1] = 0; x < 25; x++)
        for (y = 0; y < 11; y++) {
            terrs[0] += visitor_cur.batters[x].fielding[y].e;
            terrs[1] += home_cur.batters[x].fielding[y].e;
        }
    if (terrs[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (terrs[0], 'l'));
    if (terrs[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (terrs[1], 'l'));

    /* move total left-on-base */
    strcat (&work[line][0], "   ");
    work1[2] = '\0';
    work1[1] = buffer1[bcol + 1];
    if (buffer1[bcol] != '0')
        work1[0] = buffer1[bcol];
    else
        work1[0] = ' ';
    strcat (&work[line][0], &work1[0]);
    strcat (&work[line + 1][0], "   ");
    work1[1] = buffer1[bcol + 3];
    if (buffer1[bcol + 2] != '0')
        work1[0] = buffer1[bcol + 2];
    else
        work1[0] = ' ';
    strcat (&work[line + 1][0], &work1[0]);
    bcol += 4;

    for (x = 0; x < (line + 2); x++)
        strcat (&work[x][0], "\n");
    strcat (&work[x][0], "\n");
    for (x = 0; x < (line + 2); x++)
        strcat (&boxinfo[0], &work[x][0]);
    strcat (&boxinfo[0], "\n\n");

    /* visiting team offensive stats */
    if (vteamyr[0] != '0') {
        strcpy (&work[0][0], &vteamyr[0]);
        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) GetTeamName (VisitingTeamID));
    }
    else
        strcpy (&work[0][0], &visiting_team[0]);
    if (strlen (&work[0][0]) > 35)
        work[0][35] = '\0';
    else
        strncat (&work[0][0], "                                   ", 35 - strlen (&work[0][0]));

    strcat (&work[0][0], " AB  R  H BI TB W  K   AVG   OBP   SLG\n");
    strcat (&boxinfo[0], &work[0][0]);

    ttb = prevm = 0;
    previ = 99;
    while (buffer1[bcol] != ':') {
        /* get player pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        if (iwork == previ) {
            prevm = 1;
            goto dup1;
        }
        else {
            prevm = 0;
            previ = iwork;
        }

        /* move player last name and first initial */
        for (y = 0; visitor_cur.batters[iwork].id.name[y] != ','; y++)
            work[0][y] = visitor_cur.batters[iwork].id.name[y];
        work[0][y++] = ',';
        work[0][y++] = ' ';
	    work[0][y] = '\0';
        initial = visitor_cur.batters[iwork].id.name[y];
        work1[0] = initial;
        work1[1] = '\0';
        strcat (&work[0][0], &work1[0]);
        strcat (&work[0][0], "  ");
dup1:
        /* move player position */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
	    pos = atoi (&work1[0]);
        switch (pos) {
            case 0:
               strcpy (&work[1][0], "dh");
               break;
            case 1:
               strcpy (&work[1][0], "p ");
               break;
            case 2:
               strcpy (&work[1][0], "c ");
               break;
            case 3:
               strcpy (&work[1][0], "1b");
               break;
            case 4:
               strcpy (&work[1][0], "2b");
               break;
            case 5:
               strcpy (&work[1][0], "3b");
               break;
            case 6:
               strcpy (&work[1][0], "ss");
               break;
            case 7:
               strcpy (&work[1][0], "lf");
               break;
            case 8:
               strcpy (&work[1][0], "cf");
               break;
            case 9:
               strcpy (&work[1][0], "rf");
               break;
            case 10:
               strcpy (&work[1][0], "ph");
               break;
            case 11:
               strcpy (&work[1][0], "pr");
        }
        if (prevm) {
            for (z = 1; z < strlen (&work[0][0]); z++)
                if (!strncmp (&work[0][z], "  ", 2)) {
                    z += 2;
                    for ( ; z < strlen (&work[0][0]); z++)
                        if (work[0][z] == ' ')
                            break;
                    work[0][z] = '-';
                    work[0][z + 1] = work[1][0];
                    work[0][z + 2] = work[1][1];
                    break;
                }
            goto dup2;
        }

        strcat (&work[0][0], &work[1][0]);
        if (strlen (&work[0][0]) > 36)
            work[0][36] = '\0';
        else
            strncat (&work[0][0], "                                     ", 36 - strlen (&work[0][0]) - indent);

        singles_g = visitor_cur.batters[iwork].hitting.hits -
                    (visitor_cur.batters[iwork].hitting.homers +
                     visitor_cur.batters[iwork].hitting.triples +
                     visitor_cur.batters[iwork].hitting.doubles);
        singles_s = visitor_season.batters[iwork].hitting.hits -
                    (visitor_season.batters[iwork].hitting.homers +
                     visitor_season.batters[iwork].hitting.triples +
                     visitor_season.batters[iwork].hitting.doubles);

        /* move individual stats */
        if (visitor_cur.batters[iwork].hitting.atbats < 10)
            strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.atbats, 'l'));
        if (visitor_cur.batters[iwork].hitting.runs > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.runs, 'l'));
        if (visitor_cur.batters[iwork].hitting.hits > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.hits, 'l'));
        if (visitor_cur.batters[iwork].hitting.rbi > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.rbi, 'l'));
        if (((visitor_cur.batters[iwork].hitting.homers * 4) +
             (visitor_cur.batters[iwork].hitting.triples * 3) +
             (visitor_cur.batters[iwork].hitting.doubles * 2) + singles_g) > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (((visitor_cur.batters[iwork].hitting.homers * 4) +
                    (visitor_cur.batters[iwork].hitting.triples * 3) + (visitor_cur.batters[iwork].hitting.doubles * 2) + singles_g), 'l'));
        ttb += ((visitor_cur.batters[iwork].hitting.homers * 4) + (visitor_cur.batters[iwork].hitting.triples * 3) +
                (visitor_cur.batters[iwork].hitting.doubles * 2) + singles_g);

        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.bb, 'l'));
        strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.so, 'l'));

        strcat (&work[0][0], (char *) do_average (visitor_season.batters[iwork].hitting.hits, visitor_season.batters[iwork].hitting.atbats));
        strcat (&work[0][0], (char *) do_average ((visitor_season.batters[iwork].hitting.hits +
                         visitor_season.batters[iwork].hitting.bb + visitor_season.batters[iwork].hitting.hbp),
                    (visitor_season.batters[iwork].hitting.atbats + visitor_season.batters[iwork].hitting.bb +
                         visitor_season.batters[iwork].hitting.sf + visitor_season.batters[iwork].hitting.hbp)));
        strcat (&work[0][0], (char *) do_average (((visitor_season.batters[iwork].hitting.homers * 4) +
               (visitor_season.batters[iwork].hitting.triples * 3) + (visitor_season.batters[iwork].hitting.doubles * 2) +
                singles_s), visitor_season.batters[iwork].hitting.atbats));
dup2:
        /* check for end of players in this batting order position */
        if (buffer1[bcol] == ':') {
            for (y = 0; y < indent; y++)
                strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], &work[0][0]);
            strcat (&boxinfo[0], "\n");

            bcol++;
            indent = 0;
        }
        else {
            work1[0] = buffer1[bcol];
            work1[1] = buffer1[bcol + 1];
            work1[2] = '\0';
            iwork = atoi (&work1[0]);
            if (iwork != previ) {
                for (y = 0; y < indent; y++)
                    strcat (&boxinfo[0], " ");
                strcat (&boxinfo[0], &work[0][0]);
                strcat (&boxinfo[0], "\n");
            }
            indent = 2;
        }
    }
    strcat (&boxinfo[0], "\n");

    /* put out totals */
    strcat (&boxinfo[0], "TOTALS                              ");
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.atbats;
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.runs;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.hits;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.rbi;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    if (ttb > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (ttb, 'l'));
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.bb;
    strcat (&boxinfo[0], " ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    for (x = z = 0; x < 25; x++)
        z += visitor_cur.batters[x].hitting.so;
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));

    strcat (&boxinfo[0], "\n\n\n");

    /* home team offensive stats */
    if (hteamyr[0] != '0') {
        strcpy (&work[0][0], &hteamyr[0]);
        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) GetTeamName (HomeTeamID));
    }
    else
        strcpy (&work[0][0], &home_team[0]);
    if (strlen (&work[0][0]) > 35)
        work[0][35] = '\0';
    else
        strncat (&work[0][0], "                                   ", 35 - strlen (&work[0][0]));

    strcat (&work[0][0], " AB  R  H BI TB W  K   AVG   OBP   SLG\n");
    strcat (&boxinfo[0], &work[0][0]);

    ttb = prevm = 0;
    previ = 99;
    bcol++;
    while (buffer1[bcol] != ':') {
        /* get player pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        if (iwork == previ) {
            prevm = 1;
            goto dup3;
        }
        else {
            prevm = 0;
            previ = iwork;
        }

        /* move player last name and first initial */
        for (y = 0; home_cur.batters[iwork].id.name[y] != ','; y++)
            work[0][y] = home_cur.batters[iwork].id.name[y];
        work[0][y++] = ',';
        work[0][y++] = ' ';
	    work[0][y] = '\0';
        initial = home_cur.batters[iwork].id.name[y];
        work1[0] = initial;
        work1[1] = '\0';
        strcat (&work[0][0], &work1[0]);
        strcat (&work[0][0], " ");
        strcat (&work[0][0], " ");
dup3:
        /* move player position */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
	    pos = atoi (&work1[0]);
        switch (pos) {
            case 0:
               work[1][0] = 'd';
               work[1][1] = 'h';
               break;
            case 1:
               work[1][0] = 'p';
               work[1][1] = ' ';
               break;
            case 2:
               work[1][0] = 'c';
               work[1][1] = ' ';
               break;
            case 3:
               work[1][0] = '1';
               work[1][1] = 'b';
               break;
            case 4:
               work[1][0] = '2';
               work[1][1] = 'b';
               break;
            case 5:
               work[1][0] = '3';
               work[1][1] = 'b';
               break;
            case 6:
               work[1][0] = 's';
               work[1][1] = 's';
               break;
            case 7:
               work[1][0] = 'l';
               work[1][1] = 'f';
               break;
            case 8:
               work[1][0] = 'c';
               work[1][1] = 'f';
               break;
            case 9:
               work[1][0] = 'r';
               work[1][1] = 'f';
               break;
            case 10:
               work[1][0] = 'p';
               work[1][1] = 'h';
               break;
            case 11:
               work[1][0] = 'p';
               work[1][1] = 'r';
        }
        work[1][2] = '\0';
        if (prevm) {
            for (z = 1; z < strlen (&work[0][0]); z++)
                if (!strncmp (&work[0][z], "  ", 2)) {
                    z += 2;
                    for ( ; z < strlen (&work[0][0]); z++)
                        if (work[0][z] == ' ')
                            break;
                    work[0][z] = '-';
                    work[0][z + 1] = work[1][0];
                    work[0][z + 2] = work[1][1];
                    break;
                }
            goto dup4;
        }

        strcat (&work[0][0], &work[1][0]);
        if (strlen (&work[0][0]) > 36)
            work[0][36] = '\0';
        else
            strncat (&work[0][0], "                                     ", 36 - strlen (&work[0][0]) - indent);

        singles_g = home_cur.batters[iwork].hitting.hits -
                    (home_cur.batters[iwork].hitting.homers +
                     home_cur.batters[iwork].hitting.triples +
                     home_cur.batters[iwork].hitting.doubles);
        singles_s = home_season.batters[iwork].hitting.hits -
                    (home_season.batters[iwork].hitting.homers +
                     home_season.batters[iwork].hitting.triples +
                     home_season.batters[iwork].hitting.doubles);

        /* move individual stats */
        if (home_cur.batters[iwork].hitting.atbats < 10)
            strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.atbats, 'l'));
        if (home_cur.batters[iwork].hitting.runs > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.runs, 'l'));
        if (home_cur.batters[iwork].hitting.hits > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.hits, 'l'));
        if (home_cur.batters[iwork].hitting.rbi > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.rbi, 'l'));
        if (((home_cur.batters[iwork].hitting.homers * 4) + (home_cur.batters[iwork].hitting.triples * 3) +
             (home_cur.batters[iwork].hitting.doubles * 2) + singles_g) > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (((home_cur.batters[iwork].hitting.homers * 4) +
                    (home_cur.batters[iwork].hitting.triples * 3) + (home_cur.batters[iwork].hitting.doubles * 2) + singles_g), 'l'));
        ttb += ((home_cur.batters[iwork].hitting.homers * 4) + (home_cur.batters[iwork].hitting.triples * 3) +
                (home_cur.batters[iwork].hitting.doubles * 2) + singles_g);

        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.bb, 'l'));
        strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.so, 'l'));

        strcat (&work[0][0], (char *) do_average (home_season.batters[iwork].hitting.hits, home_season.batters[iwork].hitting.atbats));
        strcat (&work[0][0], (char *) do_average ((home_season.batters[iwork].hitting.hits +
                         home_season.batters[iwork].hitting.bb + home_season.batters[iwork].hitting.hbp),
                    (home_season.batters[iwork].hitting.atbats + home_season.batters[iwork].hitting.bb +
                         home_season.batters[iwork].hitting.sf + home_season.batters[iwork].hitting.hbp)));
        strcat (&work[0][0], (char *) do_average (((home_season.batters[iwork].hitting.homers * 4) +
               (home_season.batters[iwork].hitting.triples * 3) + (home_season.batters[iwork].hitting.doubles * 2) +
                singles_s), home_season.batters[iwork].hitting.atbats));
dup4:
        /* check for end of players in this batting order position */
        if (buffer1[bcol] == ':') {
            for (y = 0; y < indent; y++)
                strcat (&boxinfo[0], " ");
            strcat (&boxinfo[0], &work[0][0]);
            strcat (&boxinfo[0], "\n");

            bcol++;
            indent = 0;
        }
        else {
            work1[0] = buffer1[bcol];
            work1[1] = buffer1[bcol + 1];
            work1[2] = '\0';
            iwork = atoi (&work1[0]);
            if (iwork != previ) {
                for (y = 0; y < indent; y++)
                    strcat (&boxinfo[0], " ");
                strcat (&boxinfo[0], &work[0][0]);
                strcat (&boxinfo[0], "\n");
            }
            indent = 2;
        }
    }
    strcat (&boxinfo[0], "\n");

    /* put out totals */
    strcat (&boxinfo[0], "TOTALS                              ");
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.atbats;
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.runs;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.hits;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.rbi;
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    if (ttb > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (ttb, 'l'));
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.bb;
    strcat (&boxinfo[0], " ");
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));
    if (z > 9)
        strcat (&boxinfo[0], " ");
    else
        strcat (&boxinfo[0], "  ");
    for (x = z = 0; x < 25; x++)
        z += home_cur.batters[x].hitting.so;
    strcat (&boxinfo[0], (char *) cnvt_int2str (z, 'l'));

    strcat (&boxinfo[0], "\n\n");

    put_details (2);
    put_details (3);
    put_details (4);
    put_details (5);
    put_details (6);
    put_details (7);
    put_details (8);
    put_details (9);

    strcat (&boxinfo[0], "\n");

    if (vteamyr[0] != '0') {
        strcpy (&workcur[0], &vteamyr[0]);
        strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) GetTeamName (VisitingTeamID));
    }
    else
        strcpy (&workcur[0], &visiting_team[0]);
    if (strlen (&workcur[0]) > 34)
        workcur[34] = '\0';
    else
        strncat (&workcur[0], "                                   ", 34 - strlen (&workcur[0]));
    strcat (&boxinfo[0], &workcur[0]);

    strcat (&boxinfo[0], "  IP   H  R ER BB  K HR BF    ERA  OpBA\n");

    bcol++;
    while (buffer1[bcol] != ':') {
        /* get pitcher pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        /* move pitcher last name and first initial */
        for (y = 0; visitor_cur.pitchers[iwork].id.name[y] != ','; y++)
            work[0][y] = visitor_cur.pitchers[iwork].id.name[y];
	    work[0][y++] = '\0';
        initial = visitor_cur.pitchers[iwork].id.name[y + 1];
        workcur[0] = initial;
        strcpy (&workcur[1], " ");
        strcat (&workcur[0], &work[0][0]);
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.wins != 0)
            strcat (&workcur[0], "(W, ");
        if (visitor_cur.pitchers[iwork].pitching.losses != 0)
            strcat (&workcur[0], "(L, ");
        if (visitor_cur.pitchers[iwork].pitching.wins != 0 || visitor_cur.pitchers[iwork].pitching.losses != 0) {
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.wins, 'l'));
            strcat (&workcur[0], "-");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.losses, 'l'));
            strcat (&workcur[0], ")");
        }
        if (visitor_cur.pitchers[iwork].pitching.saves != 0) {
            strcat (&workcur[0], "(S, ");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.saves, 'l'));
            strcat (&workcur[0], ")");
        }

        if (strlen (&workcur[0]) > 35)
            workcur[35] = '\0';
        else
            strncat (&workcur[0], "                                     ", 35 - strlen (&workcur[0]));

        /* move individual stats */
        if (visitor_cur.pitchers[iwork].pitching.innings < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.innings, 'l'));
        if (visitor_cur.pitchers[iwork].pitching.thirds > 0) {
            strcat (&workcur[0], ".");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.thirds, 'l'));
        }
        else
            strcat (&workcur[0], "  ");
        if (visitor_cur.pitchers[iwork].pitching.hits < 10)
            strcat (&workcur[0], "  ");
        else
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.hits, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.runs < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.runs, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.er < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.er, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.walks < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.walks, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.so < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.so, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.homers < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.homers, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.bfp < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.bfp, 'l'));
        strcat (&workcur[0], " ");

        strcat (&workcur[0], (char *) do_era (visitor_season.pitchers[iwork].pitching.er * 9,
                 visitor_season.pitchers[iwork].pitching.innings, visitor_season.pitchers[iwork].pitching.thirds));
        strcat (&workcur[0], (char *) do_average (visitor_season.pitchers[iwork].pitching.hits,
                 visitor_season.pitchers[iwork].pitching.opp_ab));
        strcat (&boxinfo[0], &workcur[0]);
        strcat (&boxinfo[0], "\n");
    }

    strcat (&boxinfo[0], "\n");

    if (hteamyr[0] != '0') {
        strcpy (&workcur[0], &hteamyr[0]);
        strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) GetTeamName (HomeTeamID));
    }
    else
        strcpy (&workcur[0], &home_team[0]);
    if (strlen (&workcur[0]) > 34)
        workcur[34] = '\0';
    else
        strncat (&workcur[0], "                                   ", 34 - strlen (&workcur[0]));

    strcat (&workcur[0], "  IP   H  R ER BB  K HR BF    ERA  OpBA\n");
    strcat (&boxinfo[0], &workcur[0]);

    bcol++;
    while (buffer1[bcol] != ':') {
        /* get pitcher pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        /* move pitcher last name and first initial */
        for (y = 0; home_cur.pitchers[iwork].id.name[y] != ','; y++)
            work[0][y] = home_cur.pitchers[iwork].id.name[y];
	    work[0][y++] = '\0';
        initial = home_cur.pitchers[iwork].id.name[y + 1];
        workcur[0] = initial;
        strcpy (&workcur[1], " ");
        strcat (&workcur[0], &work[0][0]);
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.wins != 0)
            strcat (&workcur[0], "(W, ");
        if (home_cur.pitchers[iwork].pitching.losses != 0)
            strcat (&workcur[0], "(L, ");
        if (home_cur.pitchers[iwork].pitching.wins != 0 || home_cur.pitchers[iwork].pitching.losses != 0) {
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.wins, 'l'));
            strcat (&workcur[0], "-");
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.losses, 'l'));
            strcat (&workcur[0], ")");
        }
        if (home_cur.pitchers[iwork].pitching.saves != 0) {
            strcat (&workcur[0], "(S, ");
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.saves, 'l'));
            strcat (&workcur[0], ")");
        }

        if (strlen (&workcur[0]) > 35)
            workcur[35] = '\0';
        else
            strncat (&workcur[0], "                                     ", 35 - strlen (&workcur[0]));

        /* move individual stats */
        if (home_cur.pitchers[iwork].pitching.innings < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.innings, 'l'));
        if (home_cur.pitchers[iwork].pitching.thirds > 0) {
            strcat (&workcur[0], ".");
            strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.thirds, 'l'));
        }
        else
            strcat (&workcur[0], "  ");
        if (home_cur.pitchers[iwork].pitching.hits < 10)
            strcat (&workcur[0], "  ");
        else
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.hits, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.runs < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.runs, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.er < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.er, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.walks < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.walks, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.so < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.so, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.homers < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.homers, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.bfp < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.bfp, 'l'));
        strcat (&workcur[0], " ");

        strcat (&workcur[0], (char *) do_era (home_season.pitchers[iwork].pitching.er * 9,
                 home_season.pitchers[iwork].pitching.innings, home_season.pitchers[iwork].pitching.thirds));
        strcat (&workcur[0], (char *) do_average (home_season.pitchers[iwork].pitching.hits, home_season.pitchers[iwork].pitching.opp_ab));
        strcat (&boxinfo[0], &workcur[0]);
        strcat (&boxinfo[0], "\n");
    }
    strcat (&boxinfo[0], "\n\n");

    put_details (10);
    put_details (11);
    put_details (12);
    put_details (13);
    put_details (14);
}

char *
do_average (int dividend, int divisor) {
    float z;
    char work[10];

    if ((dividend < 0 || divisor < 0) || (dividend == 0 && divisor == 0)) {
        strcpy (&resultstr[0], "   -  ");
        return (&resultstr[0]);
    }

    z = ((float) dividend / (float) divisor);
    sprintf (&work[0], "%.3f", z);
    if (work[0] == '0')
        work[0] = ' ';
    if (!strcmp (&work[0], "nan"))
        strcpy (&work[0], ".000");
    if (!strcmp (&work[0], "-nan"))
        strcpy (&work[0], "  - ");

    strncpy (&resultstr[0], "      ", (6 - strlen (&work[0])));
    resultstr[6 - strlen (&work[0])] = '\0';
    strcat (&resultstr[0], &work[0]);

    return (&resultstr[0]);
}

char *
do_era (int dividend, int divisor, int thirds) {
    float z;
    char work[10];

    z = ((float) dividend / ((float) divisor + (float) thirds / 3.0));
    sprintf (&work[0], "%.2f", z);

    strncpy (&resultstr[0], "      ", (6 - strlen (&work[0])));
    resultstr[6 - strlen (&work[0])] = '\0';
    if (!strcmp (&work[0], "nan") || !strcmp (&work[0], "-nan"))
        strcpy (&work[0], " -");

    strcat (&resultstr[0], &work[0]);

    return (&resultstr[0]);
}

void
put_details (gint which) {
    gint w, x, y, z, limit, perrs[4][25];
    gchar work[80], work1[5], workcur[100];

    for (x = 0; x < 4; x++)
        for (y = 0; y < 25; y++)
            perrs[x][y] = 0;

    for (x = y = z = 0; x < 25; x++)
        switch (which) {
            case 2:
                y += visitor_cur.batters[x].hitting.doubles;
                z += home_cur.batters[x].hitting.doubles;
                break;
            case 3:
                y += visitor_cur.batters[x].hitting.triples;
                z += home_cur.batters[x].hitting.triples;
                break;
            case 4:
                y += visitor_cur.batters[x].hitting.homers;
                z += home_cur.batters[x].hitting.homers;
                break;
            case 5:
                y += visitor_cur.batters[x].hitting.sf;
                z += home_cur.batters[x].hitting.sf;
                break;
            case 6:
                y += visitor_cur.batters[x].hitting.sh;
                z += home_cur.batters[x].hitting.sh;
                break;
            case 7:
                y += visitor_cur.batters[x].hitting.gidp;
                z += home_cur.batters[x].hitting.gidp;
                break;
            case 8:
                y += visitor_cur.batters[x].hitting.sb;
                z += home_cur.batters[x].hitting.sb;
                break;
            case 9:
                y += visitor_cur.batters[x].hitting.cs;
                z += home_cur.batters[x].hitting.cs;
                break;
            case 10:
                for (w = 0; w < 11; w++) {
                    perrs[0][x] += visitor_cur.batters[x].fielding[w].e;
                    perrs[2][x] += visitor_season.batters[x].fielding[w].e;
                    y += visitor_cur.batters[x].fielding[w].e;
                    perrs[1][x] += home_cur.batters[x].fielding[w].e;
                    perrs[3][x] += home_season.batters[x].fielding[w].e;
                    z += home_cur.batters[x].fielding[w].e;
                }
                break;
            case 11:
                y += visitor_cur.batters[x].hitting.hbp;
                z += home_cur.batters[x].hitting.hbp;
                break;
            case 13:
                y += visitor_cur.batters[x].fielding[2].pb;
                z += home_cur.batters[x].fielding[2].pb;
        }

    if (which == 12)
        for (x = 0; x < 11; x++) {
            y += visitor_cur.pitchers[x].pitching.wp;
            z += home_cur.pitchers[x].pitching.wp;
        }

    if (which == 14)
        y = 1;         /* so we don't return prematurely */

    if ((y + z) == 0)
        return;

    switch (which) {
        case 2:
            strcat (&boxinfo[0], "DOUBLES\n");
            break;
        case 3:
            strcat (&boxinfo[0], "TRIPLES\n");
            break;
        case 4:
            strcat (&boxinfo[0], "HOME RUNS\n");
            break;
        case 5:
            strcat (&boxinfo[0], "SACRIFICE FLIES\n");
            break;
        case 6:
            strcat (&boxinfo[0], "SACRIFICES\n");
            break;
        case 7:
            strcat (&boxinfo[0], "GROUNDED INTO DOUBLE PLAYS\n");
            break;
        case 8:
            strcat (&boxinfo[0], "STOLEN BASES\n");
            break;
        case 9:
            strcat (&boxinfo[0], "CAUGHT STEALING\n");
            break;
        case 10:
            strcat (&boxinfo[0], "ERRORS\n");
            break;
        case 11:
            strcat (&boxinfo[0], "HIT BY PITCH\n");
            break;
        case 12:
            strcat (&boxinfo[0], "WILD PITCHES\n");
            break;
        case 13:
            strcat (&boxinfo[0], "PASSED BALLS\n");
            break;
        case 14:
            strcat (&boxinfo[0], "TIME\n");
    } 
    strcpy (&workcur[0], "  ");

    if (which == 14) {
        strncat (&workcur[0], &game_time[0], 2);
	    if (game_time[1] != ' ') {
            strcat (&workcur[0], ":");
            if (game_time[2] == ' ')
                game_time[2] = '0';
        }
        strncat (&workcur[0], &game_time[2], 2);
        strcat (&workcur[0], ":");
        if (game_time[4] == ' ')
            game_time[4] = '0';
        strncat (&workcur[0], &game_time[4], 2);
        strcat (&boxinfo[0], &workcur[0]);
        strcat (&boxinfo[0], "\n");
        return;
    }

    if (vteamyr[0] != '0') {
        strcat (&workcur[0], &vteamyr[0]);
        strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) GetTeamName (VisitingTeamID));
    }
    else
        strcat (&workcur[0], &visiting_team[0]);
    strcat (&workcur[0], " ");
    strcat (&workcur[0], (char *) cnvt_int2str (y, 'l'));
    if (y != 0) {
        strcat (&workcur[0], ":  ");
        if (which == 12)
            limit = 11;
        else
            limit = 25;
        for (x = 0; x < limit; x++) {
            if ((which == 2 && visitor_cur.batters[x].hitting.doubles != 0) || (which == 3 && visitor_cur.batters[x].hitting.triples != 0) ||
                      (which == 4 && visitor_cur.batters[x].hitting.homers != 0) || (which == 5 && visitor_cur.batters[x].hitting.sf != 0) ||
                      (which == 6 && visitor_cur.batters[x].hitting.sh != 0) || (which == 7 && visitor_cur.batters[x].hitting.gidp != 0) ||
                      (which == 8 && visitor_cur.batters[x].hitting.sb != 0) || (which == 9 && visitor_cur.batters[x].hitting.cs != 0) ||
                      (which == 10 && perrs[0][x] != 0) || (which == 11 && visitor_cur.batters[x].hitting.hbp != 0) ||
                      (which == 12 && visitor_cur.pitchers[x].pitching.wp != 0) ||
                      (which == 13 && visitor_cur.batters[x].fielding[2].pb != 0)) {
                if (which == 12)
                    for (y = 0; visitor_cur.pitchers[x].id.name[y] != ','; y++)
                        work[y] = visitor_cur.pitchers[x].id.name[y];
                else
                    for (y = 0; visitor_cur.batters[x].id.name[y] != ','; y++)
                        work[y] = visitor_cur.batters[x].id.name[y];
                work[y++] = ',';
                work[y] = '\0';
                if (which == 12)
                    work1[0] = visitor_cur.pitchers[x].id.name[y + 1];
                else
                    work1[0] = visitor_cur.batters[x].id.name[y + 1];
                work1[1] = '\0';
		        if ((strlen (&workcur[0]) + strlen (&work[0]) + strlen (&work1[0])) > 70) {
                    strcat (&boxinfo[0], &workcur[0]);
                    strcat (&boxinfo[0], "\n");
                    if (vteamyr[0] != '0') {
                        strncpy (&workcur[0], "                                                  ", (strlen (GetTeamName (VisitingTeamID)) + 12));
                        workcur[strlen (GetTeamName (VisitingTeamID)) + 12] = '\0';
                    }
                    else {
                        strncpy (&workcur[0], "                                                  ", (strlen (&visiting_team[0]) + 7));
                        workcur[strlen (&visiting_team[0]) + 7] = '\0';
                    }
                }
                strcat (&workcur[0], &work1[0]);
                strcat (&workcur[0], " ");
                strcat (&workcur[0], &work[0]);
                strcat (&workcur[0], " ");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[0][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], " (");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[2][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], "), ");
            }
        }
        workcur[strlen (&workcur[0]) - 2] = '.';
    }
    else
        strcat (&workcur[0], ".");

    strcat (&boxinfo[0], &workcur[0]);
    strcat (&boxinfo[0], "\n");

    strcpy (&workcur[0], "  ");
    if (hteamyr[0] != '0') {
        strcat (&workcur[0], &hteamyr[0]);
        strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) GetTeamName (HomeTeamID));
    }
    else
        strcat (&workcur[0], &home_team[0]);
    strcat (&workcur[0], " ");
    strcat (&workcur[0], (char *) cnvt_int2str (z, 'l'));
    if (z != 0) {
        strcat (&workcur[0], ":  ");
        if (which == 12)
            limit = 11;
        else
            limit = 25;
        for (x = 0; x < limit; x++) {
            if ((which == 2 && home_cur.batters[x].hitting.doubles != 0) ||
                                                          (which == 3 && home_cur.batters[x].hitting.triples != 0) ||
                                                          (which == 4 && home_cur.batters[x].hitting.homers != 0) ||
                                                          (which == 5 && home_cur.batters[x].hitting.sf != 0) ||
                                                          (which == 6 && home_cur.batters[x].hitting.sh != 0) ||
                                                          (which == 7 && home_cur.batters[x].hitting.gidp != 0) ||
                                                          (which == 8 && home_cur.batters[x].hitting.sb != 0) ||
                                                          (which == 9 && home_cur.batters[x].hitting.cs != 0) ||
                     (which == 10 && perrs[1][x] != 0) || (which == 11 && home_cur.batters[x].hitting.hbp != 0) ||
                                                          (which == 12 && home_cur.pitchers[x].pitching.wp != 0) ||
                                                          (which == 13 && home_cur.batters[x].fielding[2].pb != 0)) {
                if (which == 12)
                    for (y = 0; home_cur.pitchers[x].id.name[y] != ','; y++)
                        work[y] = home_cur.pitchers[x].id.name[y];
                else
                    for (y = 0; home_cur.batters[x].id.name[y] != ','; y++)
                        work[y] = home_cur.batters[x].id.name[y];
                work[y++] = ',';
                work[y] = '\0';
                if (which == 12)
                    work1[0] = home_cur.pitchers[x].id.name[y + 1];
                else
                    work1[0] = home_cur.batters[x].id.name[y + 1];
                work1[1] = '\0';
                if ((strlen (&workcur[0]) + strlen (&work[0]) + strlen (&work1[0])) > 70) {
                    strcat (&boxinfo[0], &workcur[0]);
                    strcat (&boxinfo[0], "\n");
                    if (hteamyr[0] != '0') {
                        strncpy (&workcur[0], "                                                  ", (strlen (GetTeamName (HomeTeamID)) + 12));
                        workcur[strlen (GetTeamName (HomeTeamID)) + 12] = '\0';
                    }
                    else {
                        strncpy (&workcur[0], "                                                  ", (strlen (&home_team[0]) + 7));
                        workcur[strlen (&home_team[0]) + 7] = '\0';
                    }
                }
                strcat (&workcur[0], &work1[0]);
                strcat (&workcur[0], " ");
                strcat (&workcur[0], &work[0]);
                strcat (&workcur[0], " ");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[1][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (home_cur.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], " (");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[3][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (home_season.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], "), ");
            }
        }
        workcur[strlen (&workcur[0]) - 2] = '.';
    }
    else
        strcat (&workcur[0], ".");

    strcat (&boxinfo[0], &workcur[0]);
    strcat (&boxinfo[0], "\n\n");
}

gint
DoLineup (int secondtime) {
    gint x;

    if (ManageH && !secondtime) {
        get_stats (sock, 'a', 0);
        get_stats (sock, 'c', 0);
        if (DoStartingLineup ('a')) {
            buffer1[0] = '\0';
            for (x = 0; x < 10; x++)
                if (lineup[x].player != 99) {
                    strcat (&buffer1[0], cnvt_int2str (lineup[x].player, 'l'));
                    strcat (&buffer1[0], " ");
                    strcat (&buffer1[0], cnvt_int2str (lineup[x].pos, 'l'));
                    strcat (&buffer1[0], " ");
                }
                else
                    break;
            strcat (&buffer1[0], "\n");
            sock_puts (sock, buffer1);
        }
        else
            return 0;

        return 1;
    }
    if (ManageV) {
        int sk;

        if (netgame && challenger_ind)
            sk = sockhvh;
        else
            sk = sock;

        get_stats (sk, 'b', 0);
        get_stats (sk, 'd', 0);
        if (DoStartingLineup ('b')) {
            buffer1[0] = '\0';
            for (x = 0; x < 10; x++)
                if (lineup[x].player != 99) {
                    strcat (&buffer1[0], cnvt_int2str (lineup[x].player, 'l'));
                    strcat (&buffer1[0], " ");
                    strcat (&buffer1[0], cnvt_int2str (lineup[x].pos, 'l'));
                    strcat (&buffer1[0], " ");
                }
                else
                    break;
            strcat (&buffer1[0], "\n");
            sock_puts (sk, buffer1);
        }
        else
            return 0;

        return 1;
    }
    return 1;
}

GtkWidget *dlgFile;

void
outEOMessage (gchar *message[5]) {
    gint x;
    gchar labeltext[500];
    GtkWidget *hbox, *stock, *label, *separator, *but1;

    if (EOSsw == 1)
        EOSsw = 2;
    if (EOPSsw == 1)
        EOPSsw = 2;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "NSB Message");
    gtk_window_set_default_size (GTK_WINDOW (dlgFile), 300, 100);
    gtk_window_set_modal (GTK_WINDOW (dlgFile), TRUE);
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    for (x = 0; x < 5; x++)
        if (message[x] != NULL) {
            strcpy (&labeltext[0], message[x]);
            label = gtk_label_new (labeltext);
            gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
            gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

            separator = gtk_hseparator_new ();
            gtk_container_add (GTK_CONTAINER (hbox), separator);
        }

    but1 = gtk_button_new_with_label ("OK");
    gtk_signal_connect (GTK_OBJECT (but1), "clicked", G_CALLBACK (EOMsgOK), NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), but1, TRUE, TRUE, 0);
    gtk_widget_show_all (dlgFile);
}

void
EOMsgOK (GtkWidget *widget, gpointer pdata) {
    if (EOSsw == 2)
        EOSsw = 3;
    if (EOPSsw == 2)
        EOPSsw = 3;
    DestroyDialog (dlgFile, dlgFile);
}

void
GotEOError () {
    gchar Error[256] = "An error was encountered when talking to the server.",
          Unconnected[256] = "You need to connect to an NSB server.", *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        strcpy (&work[0], "Not connected.\n");
        Add2TextWindow (&work[0], 0);

        msg[0] = &Unconnected[0];
    }
    else {
        strcpy (&work[0], "Encountered an error when talking to server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &Error[0];
    }
    outEOMessage (msg);
}

void
ShowPlayerPic (int pitply) {
    GtkWidget *picbox, *vbox2, *label;
    gint x, y;
    gchar work[500];

    /* if user is watching a fast game don't display pics */
    if ((watchsw && !speed.tv_sec && !speed.tv_nsec)) {
        /* ensure the picture windows are closed */
        if (pitcherpicwin != NULL) {
            if (preferences.MovingPlayerPics)
                gtk_window_get_position (GTK_WINDOW (pitcherpicwin), &pitpicwinlocX, &pitpicwinlocY);
            DestroyDialog (pitcherpicwin, pitcherpicwin);
            pitcherpicwin = NULL;
        }
        if (batterpicwin != NULL) {
            if (preferences.MovingPlayerPics)
                gtk_window_get_position (GTK_WINDOW (batterpicwin), &batpicwinlocX, &batpicwinlocY);
            DestroyDialog (batterpicwin, batterpicwin);
            batterpicwin = NULL;
        }
        return;
    }
    /* if user opts not to show pics then don't */
    if (!preferences.ShowPlayerPics)
        return;

    if (!pitply) {
        for (x = 0; x < 2; x++)
            for (y = 0; y < ppics[x].nplyr; y++)
                if (!strcmp (&savepitchername[0], &ppics[x].plyr[y].pname[0]) && !strncmp (&savepitcherdob[0], &ppics[x].plyr[y].bmonth[0], 2) &&
                       !strncmp (&savepitcherdob[2], &ppics[x].plyr[y].bday[0], 2) && !strncmp (&savepitcherdob[4], &ppics[x].plyr[y].byear[0], 4))
                    goto breaksearch1;
breaksearch1:
        if (pitcherpicwin != NULL) {
            if (preferences.MovingPlayerPics)
                gtk_window_get_position (GTK_WINDOW (pitcherpicwin), &pitpicwinlocX, &pitpicwinlocY);
            DestroyDialog (pitcherpicwin, pitcherpicwin);
            pitcherpicwin = NULL;
        }
        if ((x == 2 && y == ppics[1].nplyr) || ppics[x].plyr[y].pic == NULL)
            /* no pic */
            return;
    }
    else {
        for (x = 0; x < 2; x++)
            for (y = 0; y < ppics[x].nplyr; y++)
                if (!strcmp (&savebattername[0], &ppics[x].plyr[y].pname[0]) && !strncmp (&savebatterdob[0], &ppics[x].plyr[y].bmonth[0], 2) &&
                       !strncmp (&savebatterdob[2], &ppics[x].plyr[y].bday[0], 2) && !strncmp (&savebatterdob[4], &ppics[x].plyr[y].byear[0], 4))
                    goto breaksearch2;
breaksearch2:
        if (batterpicwin != NULL) {
            if (preferences.MovingPlayerPics)
                gtk_window_get_position (GTK_WINDOW (batterpicwin), &batpicwinlocX, &batpicwinlocY);
            DestroyDialog (batterpicwin, batterpicwin);
            batterpicwin = NULL;
        }
        if ((x == 2 && y == ppics[1].nplyr) || ppics[x].plyr[y].pic == NULL)
            /* no pic */
            return;
    }

    if (!pitply) {
        if (pitcherpicwin == NULL) {
            pitcherpicwin = gtk_dialog_new ();
            gtk_window_set_title (GTK_WINDOW (pitcherpicwin), "Pitcher Pic");
            gtk_window_set_default_size (GTK_WINDOW (pitcherpicwin), 200, 130);
            gtk_window_move (GTK_WINDOW (pitcherpicwin), pitpicwinlocX, pitpicwinlocY);
            gtk_signal_connect (GTK_OBJECT (pitcherpicwin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

            strcpy (&work[0], "Pitching - ");
            strcat (&work[0], &savepitchername[0]);

            vbox2 = gtk_vbox_new (FALSE, 0);
            gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
            gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pitcherpicwin)->vbox), vbox2, TRUE, TRUE, 0);
            label = g_object_new (GTK_TYPE_LABEL, "label", &work[0], NULL);
            gtk_box_pack_start (GTK_BOX (vbox2), label, TRUE, TRUE, 0);

            vbox2 = gtk_vbox_new (FALSE, 0);
            gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
            gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pitcherpicwin)->vbox), vbox2, TRUE, TRUE, 0);

            picbox = gtk_vbox_new (FALSE, 0);
            gtk_box_pack_start (GTK_BOX (vbox2), picbox, TRUE, TRUE, 0);
            gtk_container_add (GTK_CONTAINER (picbox), gtk_image_new_from_pixbuf (ppics[x].plyr[y].pic));

            gtk_widget_show_all (pitcherpicwin);
        }
    }
    else
        if (batterpicwin == NULL) {
            batterpicwin = gtk_dialog_new ();
            gtk_window_set_title (GTK_WINDOW (batterpicwin), "Batter Pic");
            gtk_window_set_default_size (GTK_WINDOW (batterpicwin), 200, 130);
            gtk_window_move (GTK_WINDOW (batterpicwin), batpicwinlocX, batpicwinlocY);
            gtk_signal_connect (GTK_OBJECT (batterpicwin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

            strcpy (&work[0], "Batting - ");
            strcat (&work[0], &savebattername[0]);

            vbox2 = gtk_vbox_new (FALSE, 0);
            gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
            gtk_box_pack_start (GTK_BOX (GTK_DIALOG (batterpicwin)->vbox), vbox2, TRUE, TRUE, 0);
            label = g_object_new (GTK_TYPE_LABEL, "label", &work[0], NULL);
            gtk_box_pack_start (GTK_BOX (vbox2), label, TRUE, TRUE, 0);

            vbox2 = gtk_vbox_new (FALSE, 0);
            gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
            gtk_box_pack_start (GTK_BOX (GTK_DIALOG (batterpicwin)->vbox), vbox2, TRUE, TRUE, 0);

            picbox = gtk_vbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (picbox), gtk_image_new_from_pixbuf (ppics[x].plyr[y].pic));
            gtk_box_pack_start (GTK_BOX (vbox2), picbox, TRUE, TRUE, 0);

            gtk_widget_show_all (batterpicwin);
        }
}

void
KillPlayerPics () {
    if (pitcherpicwin != NULL) {
        if (preferences.MovingPlayerPics)
            gtk_window_get_position (GTK_WINDOW (pitcherpicwin), &pitpicwinlocX, &pitpicwinlocY);
        DestroyDialog (pitcherpicwin, pitcherpicwin);
        pitcherpicwin = NULL;
    }
    
    if (batterpicwin != NULL) {
        if (preferences.MovingPlayerPics)
            gtk_window_get_position (GTK_WINDOW (batterpicwin), &batpicwinlocX, &batpicwinlocY);
        DestroyDialog (batterpicwin, batterpicwin);
        batterpicwin = NULL;
    }
}

void
SetGWinTitle () {
    strcpy (&gameplaytitle[0], "Playing a Game, gamespeed - ");
    if (speed.tv_sec)
        strcat (&gameplaytitle[0], (char *) cnvt_int2str (speed.tv_sec, 'l'));
    if (speed.tv_nsec) {
        if (speed.tv_sec)
            strcat (&gameplaytitle[0], " and ");
        if (speed.tv_nsec == 1)
            strcat (&gameplaytitle[0], "1/4");
        if (speed.tv_nsec == 2)
            strcat (&gameplaytitle[0], "1/2");
        if (speed.tv_nsec == 3)
            strcat (&gameplaytitle[0], "3/4");
    }
    if (!speed.tv_sec && !speed.tv_nsec)
        strcat (&gameplaytitle[0], "0");
    if ((speed.tv_sec == 1 && !speed.tv_nsec) || (!speed.tv_sec && speed.tv_nsec))
        strcat (&gameplaytitle[0], " second between actions");
    else
        strcat (&gameplaytitle[0], " seconds between actions");

    gtk_window_set_title (GTK_WINDOW (gamewin), gameplaytitle);
}

