/* make a lineup */

#include <time.h>
#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

GtkWidget *teamtable, *vbox2, *plybutton[9], *posbutton[9], *ckbutton[10], *pitbutton, *teamlabel[25];
gint ckmark[10];

gint
DoStartingLineup (gchar which) {
    GtkWidget *swin, *box1, *box2, *rbox1, *lubox, *scrolled_window;
    GtkLabel *tmlab;
    gint x, y, z, singles, sf, gp;
    gchar year[5], teamname[50], ha[20], wrk[500];

    if (which == 'a') {
        team = home;
        if (hteamyr[0] != '0')
            strcpy (&year[0], &hteamyr[0]);
        else
            strcpy (&year[0], "    ");
        strcpy (&teamname[0], &home_team[0]);
        strcpy (&ha[0], "(Home Team)");
        if (lgame == 1)
            team2 = home_season;  /* needed to determine injury */
    }
    else {
        team = visitor;
        if (vteamyr[0] != '0')
            strcpy (&year[0], &vteamyr[0]);
        else
            strcpy (&year[0], "    ");
        strcpy (&teamname[0], &visiting_team[0]);
        strcpy (&ha[0], "(Visiting Team)");
        if (lgame == 1)
            team2 = visitor_season;  /* needed to determine injury */
    }

    for (x = 0; x < 10; x++) {
        lineup[x].player = lineup[x].pos = 99;
        ckmark[x] = 0;
    }

    swin = gtk_dialog_new_with_buttons ("Do Lineup", NULL, GTK_DIALOG_MODAL, "Done", 1, "Let Computer Figure Lineup", 0, NULL);
    gtk_window_set_default_size (GTK_WINDOW (swin), 500, 500);
    gtk_dialog_set_default_response (GTK_DIALOG (swin), 0);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), box1, TRUE, TRUE, 0);

    box2 = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", year, NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", teamname, NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", ha, NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click on a player to select:", NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);

    /* add a table */
    teamtable = gtk_table_new (25, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (teamtable), 2);
    gtk_table_set_col_spacings (GTK_TABLE (teamtable), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), teamtable);

    /* fill in table */
    for (x = 0; x < 28; x++) {
        if (team.batters[x].id.name[0] == ' ' || !strlen (&team.batters[x].id.name[0]))
            break;

        teamlabel[x] = gtk_button_new ();

        strcpy (&wrk[0], &team.batters[x].id.name[0]);
        strcat (&wrk[0], "; ");

        for (gp = y = 0; y < 11; y++)
            gp += team.batters[x].fielding[y].games;

        if (gp != team.batters[x].fielding[1].games) {
            /* played more positions than pitcher */
            singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers +
                      team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
            if (team.batters[x].hitting.sf == -1)
                sf = 0;
            else
                sf = team.batters[x].hitting.sf;
            strcat (&wrk[0], "HR - ");
            strcat (&wrk[0], cnvt_int2str (team.batters[x].hitting.homers, 'l'));
            strcat (&wrk[0], ", BA - ");
            strcat (&wrk[0], (char *) do_average (team.batters[x].hitting.hits, team.batters[x].hitting.atbats));
            strcat (&wrk[0], ", SA - ");
            strcat (&wrk[0], (char *) do_average (((team.batters[x].hitting.homers * 4) + (team.batters[x].hitting.triples * 3) +
                                               (team.batters[x].hitting.doubles * 2) + singles), team.batters[x].hitting.atbats));
            strcat (&wrk[0], ", OBA - ");
            strcat (&wrk[0], (char *) do_average ((team.batters[x].hitting.hits + team.batters[x].hitting.bb + team.batters[x].hitting.hbp),
                             (team.batters[x].hitting.atbats + team.batters[x].hitting.bb + sf + team.batters[x].hitting.hbp)));
            strcat (&wrk[0], ", SB - ");
            strcat (&wrk[0], cnvt_int2str (team.batters[x].hitting.sb, 'l'));
            strcat (&wrk[0], ", POS: ");
            for (z = y = 0; y < 11; y++)
                if (team.batters[x].fielding[y].games) {
                    if (z)
                        strcat (&wrk[0], " ");
                    if (y == 10)
                        strcat (&wrk[0], "OF");
                    else
                        strcat (&wrk[0], figure_pos (y));
                    strcat (&wrk[0], "-");
                    strcat (&wrk[0], cnvt_int2str (team.batters[x].fielding[y].games, 'l'));
                    z = 1;
                }
        }
        if (team.batters[x].fielding[1].games) {
            /* this player [also] pitched */
            for (y = 0; y < 13; y++)
                if (!strcmp (&team.batters[x].id.name[0], &team.pitchers[y].id.name[0]))
                    break;
            if (gp != team.batters[x].fielding[1].games)
                strcat (&wrk[0], ", ");
            strcat (&wrk[0], "IP - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
            strcat (&wrk[0], ", ERA - ");
            strcat (&wrk[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings, team.pitchers[y].pitching.thirds));
            strcat (&wrk[0], ", SO - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
            strcat (&wrk[0], ", BB - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
            strcat (&wrk[0], ", G - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));
            strcat (&wrk[0], ", GS - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.games_started, 'l'));
            strcat (&wrk[0], ", SAVES - ");
            strcat (&wrk[0], cnvt_int2str (team.pitchers[y].pitching.saves, 'l'));
        }
        gtk_button_set_label (GTK_BUTTON (teamlabel[x]), wrk);

        if (lgame == 1 && team2.batters[x].id.injury)
            gtk_widget_set_sensitive (GTK_WIDGET (teamlabel[x]), FALSE);
        gtk_table_attach_defaults (GTK_TABLE (teamtable), GTK_WIDGET (teamlabel[x]), 0, 1, x, x + 1);
        g_signal_connect (G_OBJECT (teamlabel[x]), "clicked", G_CALLBACK (CBplayerbutton), GINT_TO_POINTER (x));
    }

    rbox1 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), rbox1, FALSE, FALSE, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (rbox1), GTK_BUTTONBOX_SPREAD);

    vbox2 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), vbox2, FALSE, FALSE, 0);

    for (x = 0; x < 9; x++) {
        gchar border[2];

        border[0] = (x + 1) + '0';
        border[1] = '\0';

        lubox = gtk_hbutton_box_new ();
        gtk_box_pack_start (GTK_BOX (vbox2), lubox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (lubox), gtk_label_new (border), FALSE, FALSE, 0);
        ckbutton[x] = gtk_check_button_new ();
        gtk_box_pack_start (GTK_BOX (lubox), ckbutton[x], TRUE, TRUE, 0);
        g_signal_connect (G_OBJECT (ckbutton[x]), "clicked", G_CALLBACK (CBckbutton), GINT_TO_POINTER (x));
        plybutton[x] = gtk_button_new ();
        gtk_box_pack_start (GTK_BOX (lubox), plybutton[x], TRUE, TRUE, 0);
        posbutton[x] = gtk_button_new_with_label ("pos");
        gtk_box_pack_start (GTK_BOX (lubox), posbutton[x], TRUE, TRUE, 0);
        g_signal_connect (G_OBJECT (posbutton[x]), "button-press-event", G_CALLBACK (CBposbutton), GINT_TO_POINTER (x));
    }
    if (dh) {
        lubox = gtk_hbutton_box_new ();
        gtk_box_pack_start (GTK_BOX (vbox2), lubox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (lubox), gtk_label_new ("Starting Pitcher"), FALSE, FALSE, 0);
        ckbutton[9] = gtk_check_button_new ();
        gtk_box_pack_start (GTK_BOX (lubox), ckbutton[9], TRUE, TRUE, 0);
        g_signal_connect (G_OBJECT (ckbutton[9]), "clicked", G_CALLBACK (CBckbutton), GINT_TO_POINTER (9));
        pitbutton = gtk_button_new ();
        gtk_box_pack_start (GTK_BOX (lubox), pitbutton, TRUE, TRUE, 0);
    }

    gtk_widget_show_all (box1);
RunLineup:
    if (gtk_dialog_run (GTK_DIALOG (swin))) {
        gint pitsw, incsw, dupplysw, duppossw;
        gchar NoSP[256] = "You need to select a starting pitcher.\n\n",
              SPNotP[256] = "The player selected to pitch needs to have actually pitched some innings in real life.\n\n",
              Incomplete[256] = "You need to select a full lineup.\n\n",
              DupPly[256] = "The same player is in more than one batting order slot.\n\n",
              DupPos[256] = "The same position is assigned to more than 1 player.\n\n", *msg[5];
        gint x, y;

        pitsw = incsw = dupplysw = duppossw = 0;
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        for (x = 0; x < (9 + dh); x++)
            if (lineup[x].pos == 1)
                if (!team.batters[lineup[x].player].fielding[1].games) {
                    pitsw = 2;
                    break;
                }

        if (dh)
            if (lineup[9].pos != 1)
                pitsw = 1;

        for (x = 0; x < (9 + dh); x++) {
            if (lineup[x].player == 99)
                incsw = 1;
            for (y = x + 1; y < (9 + dh); y++) {
                if (lineup[x].player == lineup[y].player)
                    dupplysw = 1;
                if (lineup[x].pos == lineup[y].pos)
                    duppossw = 1;
            }
        }

        if (pitsw || incsw || dupplysw || duppossw) {
            x = 0;
            if (pitsw == 1)
                msg[x++] = NoSP;
            if (pitsw == 2)
                msg[x++] = SPNotP;
            if (incsw)
                msg[x++] = Incomplete;
            if (dupplysw)
                msg[x++] = DupPly;
            if (duppossw)
                msg[x] = DupPos;
            outMessage (msg);
            goto RunLineup;
        }

        DestroyDialog (swin, swin);
        return 1;
    }
    else {
        DestroyDialog (swin, swin);
        return 0;
    }
}

/* player in roster clicked - if a checkbox is checked put the player into that slot, if no checkbox is checked put
   the player into the first open slot, if a pitcher is clicked (games at pitcher more than half total games) force him
   into either slot 9 or, if DH, the Starting Pitcher slot */
void
CBplayerbutton (GtkWidget *widget, gpointer pdata) {
    gint x, gp, ipdata = GPOINTER_TO_INT (pdata);

    if (lgame == 1 && team2.batters[ipdata].id.injury)
        return;

    for (gp = x = 0; x < 10; x++)
        gp += team.batters[ipdata].fielding[x].games;

    if (team.batters[ipdata].fielding[1].games > (gp / 2)) {
        /* the player clicked is a pitcher */
        if (dh) {
            lineup[9].player = ipdata;
            lineup[9].pos = 1;
            gtk_button_set_label (GTK_BUTTON (pitbutton), team.batters[ipdata].id.name);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ckbutton[9]), FALSE);
            ckmark[9] = 0;
        }
        else {
            lineup[8].player = ipdata;
            lineup[8].pos = 1;
            gtk_button_set_label (GTK_BUTTON (posbutton[8]), figure_pos (lineup[8].pos));
            gtk_button_set_label (GTK_BUTTON (plybutton[8]), team.batters[ipdata].id.name);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ckbutton[8]), FALSE);
            ckmark[8] = 0;
        }
        return;
    }

    for (x = 0; x < (9 + dh); x++)
        if (ckmark[x])
            break;

    if (x < (9 + dh)) {
        if (x == 9)
            gtk_button_set_label (GTK_BUTTON (pitbutton), team.batters[ipdata].id.name);
        else
            gtk_button_set_label (GTK_BUTTON (plybutton[x]), team.batters[ipdata].id.name);
        lineup[x].player = ipdata;
        if (x == 9)
            lineup[9].pos = 1;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ckbutton[x]), FALSE);
        ckmark[x] = 0;
    }
    else
        for (x = 0; x < (9 + dh); x++)
            if (lineup[x].player == 99) {
                if (x == 9)
                    gtk_button_set_label (GTK_BUTTON (pitbutton), team.batters[ipdata].id.name);
                else
                    gtk_button_set_label (GTK_BUTTON (plybutton[x]), team.batters[ipdata].id.name);
                if (x == 9)
                    lineup[9].pos = 1;
                lineup[x].player = ipdata;
                break;
            }
}

/* checkbox clicked - if another box is already checked then swap the two slots */
void
CBckbutton (GtkWidget *widget, gpointer pdata) {
    gint chk, x, swp1, swp2;

    chk = GPOINTER_TO_INT (pdata);

    for (x = 0; x < (9 + dh); x++)
        if (ckmark[x] && x != chk)
            break;

    if (x == (9 + dh))
        if (ckmark[chk])
            ckmark[chk] = 0;
        else
            ckmark[chk] = 1;
    else {
        swp1 = lineup[x].player;
        swp2 = lineup[x].pos;
        lineup[x].player = lineup[chk].player;
        lineup[x].pos = lineup[chk].pos;
        lineup[chk].player = swp1;
        lineup[chk].pos = swp2;
        if (x == 9)
            lineup[x].pos = 1;

        if (lineup[x].player == 99) {
            if (x < 9)
                gtk_button_set_label (GTK_BUTTON (plybutton[x]), "");
        }
        else
            if (x < 9)
                gtk_button_set_label (GTK_BUTTON (plybutton[x]), team.batters[lineup[x].player].id.name);
            else
                gtk_button_set_label (GTK_BUTTON (pitbutton), team.batters[lineup[x].player].id.name);
        if (lineup[x].pos == 99) {
            if (x < 9)
                gtk_button_set_label (GTK_BUTTON (posbutton[x]), "pos");
        }
        else
            if (x < 9)
                gtk_button_set_label (GTK_BUTTON (posbutton[x]), figure_pos (lineup[x].pos));
        if (lineup[chk].player == 99)
            gtk_button_set_label (GTK_BUTTON (plybutton[chk]), "");
        else
            gtk_button_set_label (GTK_BUTTON (plybutton[chk]), team.batters[lineup[chk].player].id.name);
        if (lineup[chk].pos == 99)
            gtk_button_set_label (GTK_BUTTON (posbutton[chk]), "pos");
        else
            gtk_button_set_label (GTK_BUTTON (posbutton[chk]), figure_pos (lineup[chk].pos));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ckbutton[x]), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ckbutton[chk]), FALSE);
        ckmark[x] = ckmark[chk] = 0;
    }
}

/* position indicator clicked - cycle through various positions */
void
CBposbutton (GtkWidget *widget, GdkEventButton *event, gpointer pdata) {
    gint x;

    x = GPOINTER_TO_INT (pdata);

    /* lineup[x].pos is initialized at 99 */
    if (event->button == 1) {   /* left mouse button clicked */
        if (dh)
            if (lineup[x].pos > 8)
                lineup[x].pos = 0;
            else {
                lineup[x].pos++;
                /* skip P position */
                if (lineup[x].pos == 1)
                    lineup[x].pos = 2;
            }
        else
            if (lineup[x].pos > 8)
                lineup[x].pos = 1;
            else
                lineup[x].pos++;
    }

    if (event->button == 3) {   /* right mouse button clicked */
        if (dh)
            if (!lineup[x].pos || lineup[x].pos == 99)
                lineup[x].pos = 9;
            else {
                lineup[x].pos--;
                /* skip P position */
                if (lineup[x].pos == 1)
                    lineup[x].pos = 0;
            }
        else
            if (lineup[x].pos == 1 || lineup[x].pos == 99)
                lineup[x].pos = 9;
            else
                lineup[x].pos--;
    }

    gtk_button_set_label (GTK_BUTTON (posbutton[x]), figure_pos (lineup[x].pos));
}

