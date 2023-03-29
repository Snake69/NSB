
/* get a user action during gameplay */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"
#include "playgame.h"

GtkWidget *owin;
/*gchar cresp;*/

/*char*/
void
OffensiveCommand (gchar *valid) {
    /*
      Offensive commands (one of the following is returned):
          L - see lineups and make changes
          R - hit and run
          B - sacrifice bunt
          Q - squeeze play
          2 - steal second
          3 - steal third
          4 - steal home
          5 - steal second and third
          6 - steal second and home
          7 - steal third and home
          9 - steal second, third, and home
          N - normal, batter swings away
          A - computer manages from here on out
    */
    GtkWidget *box, *hbox, *but1, *but2, *but3, *but4, *but5, *but6, *but7, *but8, *but9, *but10, *but11, *but12, *but13;
    GtkLabel *lab;

    cresp = ' ';
    offdialsw = 1;    /* set to indicate we're processing this dialog */

    owin = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (owin), "Offensive Command");
    gtk_window_set_default_size (GTK_WINDOW (owin), 300, 100);
    gtk_window_set_modal (GTK_WINDOW (owin), TRUE);
    gtk_signal_connect (GTK_OBJECT (owin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (owin)->vbox), box, TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Choose what you want to do:", NULL);
    gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (lab), TRUE, TRUE, 0);

    box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (owin)->action_area), box, TRUE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but1 = gtk_button_new_with_label ("Swing Away");
    gtk_signal_connect (GTK_OBJECT (but1), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (0));
    gtk_box_pack_start (GTK_BOX (hbox), but1, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but1), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but2 = gtk_button_new_with_label ("Hit & Run");
    gtk_signal_connect (GTK_OBJECT (but2), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (1));
    gtk_box_pack_start (GTK_BOX (hbox), but2, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but2), FALSE);

    but3 = gtk_button_new_with_label ("Sacrifice Bunt");
    gtk_signal_connect (GTK_OBJECT (but3), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (2));
    gtk_box_pack_start (GTK_BOX (hbox), but3, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but3), FALSE);

    but4 = gtk_button_new_with_label ("Squeeze Play");
    gtk_signal_connect (GTK_OBJECT (but4), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (3));
    gtk_box_pack_start (GTK_BOX (hbox), but4, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but4), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but5 = gtk_button_new_with_label ("Steal Second");
    gtk_signal_connect (GTK_OBJECT (but5), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (4));
    gtk_box_pack_start (GTK_BOX (hbox), but5, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but5), FALSE);

    but6 = gtk_button_new_with_label ("Steal Third");
    gtk_signal_connect (GTK_OBJECT (but6), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (5));
    gtk_box_pack_start (GTK_BOX (hbox), but6, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but6), FALSE);

    but7 = gtk_button_new_with_label ("Steal Home");
    gtk_signal_connect (GTK_OBJECT (but7), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (6));
    gtk_box_pack_start (GTK_BOX (hbox), but7, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but7), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but8 = gtk_button_new_with_label ("Steal Second & Third");
    gtk_signal_connect (GTK_OBJECT (but8), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (7));
    gtk_box_pack_start (GTK_BOX (hbox), but8, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but8), FALSE);

    but9 = gtk_button_new_with_label ("Steal Second & Home");
    gtk_signal_connect (GTK_OBJECT (but9), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (8));
    gtk_box_pack_start (GTK_BOX (hbox), but9, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but9), FALSE);

    but10 = gtk_button_new_with_label ("Steal Third & Home");
    gtk_signal_connect (GTK_OBJECT (but10), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (9));
    gtk_box_pack_start (GTK_BOX (hbox), but10, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but10), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but11 = gtk_button_new_with_label ("Steal Second, Third & Home");
    gtk_signal_connect (GTK_OBJECT (but11), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (10));
    gtk_box_pack_start (GTK_BOX (hbox), but11, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but11), FALSE);

    but12 = gtk_button_new_with_label ("See Lineup");
    gtk_signal_connect (GTK_OBJECT (but12), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (11));
    gtk_box_pack_start (GTK_BOX (hbox), but12, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but12), FALSE);

    but13 = gtk_button_new_with_label ("Computer Takes Over");
    gtk_signal_connect (GTK_OBJECT (but13), "clicked", G_CALLBACK (ReturnOff), GINT_TO_POINTER (12));
    gtk_box_pack_start (GTK_BOX (hbox), but13, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but13), FALSE);

    if (index (valid, 'N'))
        gtk_widget_set_sensitive (GTK_WIDGET (but1), TRUE);
    if (index (valid, 'R'))
        gtk_widget_set_sensitive (GTK_WIDGET (but2), TRUE);
    if (index (valid, 'B'))
        gtk_widget_set_sensitive (GTK_WIDGET (but3), TRUE);
    if (index (valid, 'Q'))
        gtk_widget_set_sensitive (GTK_WIDGET (but4), TRUE);
    if (index (valid, '2'))
        gtk_widget_set_sensitive (GTK_WIDGET (but5), TRUE);
    if (index (valid, '3'))
        gtk_widget_set_sensitive (GTK_WIDGET (but6), TRUE);
    if (index (valid, '4'))
        gtk_widget_set_sensitive (GTK_WIDGET (but7), TRUE);
    if (index (valid, '5'))
        gtk_widget_set_sensitive (GTK_WIDGET (but8), TRUE);
    if (index (valid, '6'))
        gtk_widget_set_sensitive (GTK_WIDGET (but9), TRUE);
    if (index (valid, '7'))
        gtk_widget_set_sensitive (GTK_WIDGET (but10), TRUE);
    if (index (valid, '9'))
        gtk_widget_set_sensitive (GTK_WIDGET (but11), TRUE);
    if (index (valid, 'L'))
        gtk_widget_set_sensitive (GTK_WIDGET (but12), TRUE);
    if (index (valid, 'A'))
        gtk_widget_set_sensitive (GTK_WIDGET (but13), TRUE);

    gtk_widget_show_all (owin);
}

void
ReturnOff (GtkWidget *widget, gpointer pdata) {
    gint resp = GPOINTER_TO_INT (pdata);

    switch (resp) {
        case 0:
            cresp = 'N';
            break;
        case 1:
            cresp = 'R';
            break;
        case 2:
            cresp = 'B';
            break;
        case 3:
            cresp = 'Q';
            break;
        case 4:
            cresp = '2';
            break;
        case 5:
            cresp = '3';
            break;
        case 6:
            cresp = '4';
            break;
        case 7:
            cresp = '5';
            break;
        case 8:
            cresp = '6';
            break;
        case 9:
            cresp = '7';
            break;
        case 10:
            cresp = '9';
            break;
        case 11:
            cresp = 'L';
            break;
        default:
            cresp = 'A';
    }
    if (cresp == 'A') {
        gint z;
        gchar *msg[5];

        for (z = 0; z < 5; z++)
            msg[z] = NULL;

        msg[0] = "Are you sure you want the computer to take over managing your team?\n\n";
        if (!ShallWeContinue (msg))
            return;

        currenttype = ' ';
    }

    DestroyDialog (owin, owin);
    offdialsw = 2;     /* set to indicate we're finished with this dialog */
}

GtkWidget *swin, *batterbutton, *br1button, *br2button, *br3button, *lubutton[10];
gint batter, br[3], brind[3], batterind, newbatter, newbr[3], t, plyr[10], pos[10], bonum[10], roster[2][28], lubutind[10], newlu[10], which[2][28], dhind;
struct {
    gint player[30],
         pos[30];
} bo[2][9];
struct {
    gint pitcher[15];
} p[2];

void
SeeLineup (int sock) {
    GtkWidget *vbox, *hbox, *vbox1, *vbox2, *vbox3, *vbox4, *but1, *but2, *but3, *plytable, *scrolled_window, *labp;
    GtkLabel *lab;
    GSList *group, *grouplu = NULL;
    gint ha, bcol, x, y, z, zz, used[2][28], singles, sf, gp;
    gchar buf[500], work[500], *cc;

    lineupsw = 2;    /* set to indicate we're processing this dialog */
    sock_gets (sock, &buf[0], sizeof (buf));
    /*
       data received from server will be in the form -

       home/away indicator (1 character)
       DH indicator (1 character)
       next 8 characters are:
         if batting:
           batter (2 characters)
           runner on firstbase (2 characters)
           runner on second base (2 characters)
           runner on third base (2 characters)
         if in field:
           99999999 (8 characters)
       remainder of data is variable length -
         first appears all players who have appeared in each batting order position for the visiting team in the
         form:
           player (2 characters)
           position played (2 characters)
           (if the same player in one batting order position played two different positions in the field then that
            player will appear twice in data)
           the last player/position-played pair for each batting order position will be followed by a colon (:)
           the last player/position-played pair for the ninth batting order position will be followed by a second
             colon (::)

         repeat for home team

         next appears all pitchers who have appeared for the visiting team in the form:
           pitcher (2 characters)
           the last pitcher will be followed by a colon (:)

         repeat for home team
    */

    /* init */
    for (x = 0; x < 10; x++) {
        pos[x] = 0;
        plyr[x] = bonum[x] =  99;
    }
    newbatter = newbr[0] = newbr[1] = newbr[2] = 99;
    for (x = 0; x < 10; x++)
        newlu[x] = 99;

    for (z = 0; z < 2; z++) {
        for (x = 0; x < 9; x++)
            for (y = 0; y < 30; y++)
                bo[z][x].player[y] = bo[z][x].pos[y] = 99;
        for (x = 0; x < 15; x++)
            p[z].pitcher[x] = 99;
        for (x = 0; x < 28; x++)
            roster[z][x] = which[z][x] = used[z][x] = 99;
    }

    ha = buf[0] - '0';
    dhind = buf[1] - '0';
    work[0] = buf[2];
    work[1] = buf[3];
    work[2] = '\0';
    batter = atoi (&work[0]);  /* batter will always equal 99 when user's team is in the field
                                  batter will never equal 99 when user's team is at bat */
    if (ha)
        if (batter == 99) {
            team = home;
            team2 = home_season;  /* needed to see injury */
            t = 1;
        }
        else {
            team = visitor;
            team2 = visitor_season;  /* needed to see injury */
            t = 0;
        }
    else
        if (batter == 99) {
            team = visitor;
            team2 = visitor_season;  /* needed to see injury */
            t = 0;
        }
        else {
            team = home;
            team2 = home_season;  /* needed to see injury */
            t = 1;
        }
    work[0] = buf[4];
    work[1] = buf[5];
    work[2] = '\0';
    br[0] = atoi (&work[0]);  /* who's on first */
    work[0] = buf[6];
    work[1] = buf[7];
    work[2] = '\0';
    br[1] = atoi (&work[0]);  /* who's on second */
    work[0] = buf[8];
    work[1] = buf[9];
    work[2] = '\0';
    br[2] = atoi (&work[0]);  /* who's on third */

    /* fill in batting orders */
    for (bcol = 10, z = 0; z < 2; z++, bcol++)
        for (x = 0; x < 9; x++, bcol++)
            for (y = 0; buf[bcol] != ':'; y++) {
                work[0] = buf[bcol];
                work[1] = buf[bcol + 1];
                work[2] = '\0';
                bcol += 2;
                bo[z][x].player[y] = atoi (&work[0]);

                work[0] = buf[bcol];
                work[1] = buf[bcol + 1];
                work[2] = '\0';
                bcol += 2;
                bo[z][x].pos[y] = atoi (&work[0]);
            }
    /* fill in pitchers */
    for (z = 0; z < 2; z++, bcol++)
        for (x = 0; buf[bcol] != ':'; x++) {
            work[0] = buf[bcol];
            work[1] = buf[bcol + 1];
            work[2] = '\0';
            bcol += 2;
            p[z].pitcher[x] = atoi (&work[0]);
        }

    /* designated hitter? */
    if (dhind)
        for (z = 0; z < 2; z++)
            for (zz = 0; zz < 15; zz++) {
                if (p[z].pitcher[zz] == 99)
                    continue;
                for (x = 0; x < 28; x++)
                    if (!strcmp (&team.batters[x].id.name[0], &team.pitchers[p[z].pitcher[zz]].id.name[0]))
                        break;
                for (y = 0; y < 28; y++)
                    if (used[z][y] == 99) {
                        used[z][y] = x;
                        break;
                    }
            }

    /* determine players already used */
    for (z = 0; z < 2; z++)
        for (zz = 0; zz < 9; zz++)
            for (x = 1; x < 30; x++) {
                if (bo[z][zz].player[x] == 99)
                    continue;
                if (bo[z][zz].player[x] == bo[z][zz].player[x - 1])  /* same player, different positions? */
                    continue;
                for (y = 0; y < 28; y++)
                    if (used[z][y] == 99) {
                        used[z][y] = bo[z][zz].player[x];
                        break;
                    }
            }

    /* fill in roster, "which" holds availability */
    for (z = 0; z < 2; z++) {
        for (x = 0; x < 9; x++) {
            roster[z][x] = bo[z][x].player[0];
            which[z][x] = 0;
        }
        for (x = 0, y = 9; x < 28; x++, y++)
            if (used[z][x] != 99) {
                roster[z][y] = used[z][x];
                if (dhind && y == 9)
                    which[z][y] = 0;
                else
                    which[z][y] = 1;
            }
            else
                break;
        for (y = 0; y < 28; y++)
            for (zz = 0; zz < 28; zz++)
                if (y == roster[z][zz])
                    break;
                else
                    if (roster[z][zz] == 99) {
                        roster[z][zz] = y;
                        which[z][zz] = 2;
                        break;
                    }
    }

    strcpy (&work[0], "Lineup - ");
    if (!t) {
        strcat (&work[0], &vteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], &visiting_team[0]);
    }
    else {
        strcat (&work[0], &hteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], &home_team[0]);
    }

    swin = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (swin), work);
    gtk_window_set_default_size (GTK_WINDOW (swin), 300, 100);
    gtk_window_set_modal (GTK_WINDOW (swin), TRUE);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), hbox, TRUE, TRUE, 0);

    vbox1 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
    vbox3 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox3, TRUE, TRUE, 0);
    vbox4 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox4, TRUE, TRUE, 0);
    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

    batterind = 1;
    brind[0] = brind[1] = brind[2] = 0;
    for (x = 0; x < 10; x++)
        lubutind[x] = 0;
    lubutind[8 + dhind] = 1;

    lab = g_object_new (GTK_TYPE_LABEL, "label", "LINEUP:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);

    for (x = 0; x < 9; x++) {
        work[0] = (x + 1) + '0';
        work[1] = '-';
        work[2] = ' ';
        cc = index (&team.batters[roster[t][x]].id.name[0], ',');
        work[3] = *(cc + 2);
        work[4] = ' ';
        for (z = 0, y = 5; team.batters[roster[t][x]].id.name[z] != ','; z++, y++)
            work[y] = team.batters[roster[t][x]].id.name[z];
        work[y++] = '\0';

        strcat (&work[0], "  ");
        strcat (&work[0], figure_pos (bo[t][x].pos[0]));

        /* add some stats */
        if (batter == 99 && bo[t][x].pos[0] == 1) {
            for (y = 0; y < 13; y++)
                if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[y].id.name[0]))
                    break;
            strcat (&work[0], "; IP-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
            strcat (&work[0], ", ERA-");
            strcat (&work[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings, team.pitchers[y].pitching.thirds));
            strcat (&work[0], ", SO-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
            strcat (&work[0], ", BB-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
            strcat (&work[0], ", G-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));
        }
        else {
            /* not a pitcher */
            singles = team.batters[roster[t][x]].hitting.hits - (team.batters[roster[t][x]].hitting.homers +
                      team.batters[roster[t][x]].hitting.triples + team.batters[roster[t][x]].hitting.doubles);
            if (team.batters[roster[t][x]].hitting.sf == -1)
                sf = 0;
            else
                sf = team.batters[roster[t][x]].hitting.sf;
            strcat (&work[0], "; HR-");
            strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.homers, 'l'));
            strcat (&work[0], ", BA-");
            strcat (&work[0], (char *) do_average (team.batters[roster[t][x]].hitting.hits,
                                                 team.batters[roster[t][x]].hitting.atbats));
            strcat (&work[0], ", SA-");
            strcat (&work[0], (char *) do_average (((team.batters[roster[t][x]].hitting.homers * 4) +
                                                   (team.batters[roster[t][x]].hitting.triples * 3) +
                                                   (team.batters[roster[t][x]].hitting.doubles * 2) +
                                                 singles), team.batters[roster[t][x]].hitting.atbats));
            strcat (&work[0], ", OBA-");
            strcat (&work[0], (char *) do_average ((team.batters[roster[t][x]].hitting.hits +
                                                      team.batters[roster[t][x]].hitting.bb +
                                                      team.batters[roster[t][x]].hitting.hbp),
                             (team.batters[roster[t][x]].hitting.atbats + team.batters[roster[t][x]].hitting.bb +
                                                                    sf + team.batters[roster[t][x]].hitting.hbp)));
            strcat (&work[0], ", SB-");
            strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.sb, 'l'));
        }

        if (batter != 99) {
            /* the user's team is at bat */
            lab = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
            gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);
            gtk_label_set_text (GTK_LABEL (lab), work);
        }
        else {
            /* the user's team is in the field */
            if (!x) {
                lubutton[x] = gtk_radio_button_new (NULL);
                grouplu = gtk_radio_button_get_group (GTK_RADIO_BUTTON (lubutton[x]));
            }
            else
                if (x == 1)
                    lubutton[x] = gtk_radio_button_new (grouplu);
                else
                    lubutton[x] = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (lubutton[x - 1]));

            gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lubutton[x]), TRUE, TRUE, 0);
            g_signal_connect (G_OBJECT (lubutton[x]), "clicked", G_CALLBACK (CBlubutton), GINT_TO_POINTER (x));
            gtk_button_set_label (GTK_BUTTON (lubutton[x]), work);
        }
    }

    if (dhind && batter == 99) {
        /* there is a DH and the user's team is in the field */
        strcpy (&work[0], "Pitcher - ");
        cc = index (&team.pitchers[p[t].pitcher[0]].id.name[0], ',');
        work[10] = *(cc + 2);
        work[11] = ' ';
        for (z = 0, y = 12; team.pitchers[p[t].pitcher[0]].id.name[z] != ','; z++, y++)
            work[y] = team.pitchers[p[t].pitcher[0]].id.name[z];
        work[y++] = '\0';

        /* add some stats */
        for (y = 0; y < 13; y++)
            if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[y].id.name[0]))
                break;
        strcat (&work[0], "; IP-");
        strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
        strcat (&work[0], ", ERA-");
        strcat (&work[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings, team.pitchers[y].pitching.thirds));
        strcat (&work[0], ", SO-");
        strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
        strcat (&work[0], ", BB-");
        strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
        strcat (&work[0], ", G-");
        strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));

        lubutton[9] = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (lubutton[8]));

        gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lubutton[9]), TRUE, TRUE, 0);
        g_signal_connect (G_OBJECT (lubutton[x]), "clicked", G_CALLBACK (CBlubutton), GINT_TO_POINTER (9));
        gtk_button_set_label (GTK_BUTTON (lubutton[9]), work);
    }

    for (x = 0; x < 9; x++)
        if (bo[t][x].pos[0] == 1) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lubutton[x]), TRUE);
            break;
        }
    if (x == 9)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lubutton[9]), TRUE);

    if (batter != 99) {
        /* team is at bat */
        GtkWidget *sep;

        sep = gtk_hseparator_new ();
        gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);
        sep = gtk_hseparator_new ();
        gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);
        sep = gtk_hseparator_new ();
        gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
        lab = g_object_new (GTK_TYPE_LABEL, "label", "Batter - ", NULL);
        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

        batterbutton = gtk_radio_button_new_with_label (NULL, "");
        group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (batterbutton));
        g_signal_connect (G_OBJECT (batterbutton), "clicked", G_CALLBACK (CBbatterbutton), NULL);
        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (batterbutton), TRUE, TRUE, 0);

        cc = index (&team.batters[roster[t][batter]].id.name[0], ',');
        work[0] = *(cc + 2);
        work[1] = ' ';
        for (z = 0, y = 2; team.batters[roster[t][batter]].id.name[z] != ','; z++, y++)
            work[y] = team.batters[roster[t][batter]].id.name[z];
        work[y++] = '\0';

        gtk_button_set_label (GTK_BUTTON (batterbutton), work);

        for (x = 0; x < 3; x++)
            if (br[x] != 99) {
                hbox = gtk_hbox_new (FALSE, 0);
                gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
                switch (x) {
                    case 0:
                        lab = g_object_new (GTK_TYPE_LABEL, "label", "On First - ", NULL);
                        break;
                    case 1:
                        lab = g_object_new (GTK_TYPE_LABEL, "label", "On Second - ", NULL);
                        break;
                    default:
                        lab = g_object_new (GTK_TYPE_LABEL, "label", "On Third - ", NULL);
                }
                gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (lab), TRUE, TRUE, 0);
                switch (x) {
                    case 0:
                        br1button = gtk_radio_button_new_with_label (group, "");
                        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (br1button), TRUE, TRUE, 0);
                        g_signal_connect (G_OBJECT (br1button), "clicked", G_CALLBACK (CBbr1button), NULL);
                        break;
                    case 1:
                        if (br[0] == 99)
                            br2button = gtk_radio_button_new_with_label (group, "");
                        else
                            br2button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (br1button), "");
                        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (br2button), TRUE, TRUE, 0);
                        g_signal_connect (G_OBJECT (br2button), "clicked", G_CALLBACK (CBbr2button), NULL);
                        break;
                    default:
                        if (br[0] == 99 && br[1] == 99)
                            br3button = gtk_radio_button_new_with_label (group, "");
                        if (br[0] != 99 && br[1] == 99)
                            br3button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (br1button), "");
                        if (br[1] != 99)
                            br3button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (br2button), "");
                        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (br3button), TRUE, TRUE, 0);
                        g_signal_connect (G_OBJECT (br3button), "clicked", G_CALLBACK (CBbr3button), NULL);
                }

                cc = index (&team.batters[roster[t][br[x]]].id.name[0], ',');
                work[0] = *(cc + 2);
                work[1] = ' ';
                for (z = 0, y = 2; team.batters[roster[t][br[x]]].id.name[z] != ','; z++, y++)
                    work[y] = team.batters[roster[t][br[x]]].id.name[z];
                work[y++] = '\0';

                switch (x) {
                    case 0:
                        gtk_button_set_label (GTK_BUTTON (br1button), work);
                        break;
                    case 1:
                        gtk_button_set_label (GTK_BUTTON (br2button), work);
                        break;
                    default:
                        gtk_button_set_label (GTK_BUTTON (br3button), work);
                }
            }
    }

    lab = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), vbox, TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Available Players:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);
    lab = g_object_new (GTK_TYPE_LABEL, "label", "(click on an available player)", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

    /* add a table */
    plytable = gtk_table_new (25, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (plytable), 2);
    gtk_table_set_col_spacings (GTK_TABLE (plytable), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), plytable);

    for (x = 0, y = 0; x < 28; x++)
        if (which[t][x] > 1 && !team2.batters[roster[t][x]].id.injury) {
            cc = index (&team.batters[roster[t][x]].id.name[0], ',');
            if (cc == NULL)
                break;
            work[0] = *(cc + 2);
            work[1] = ' ';
            for (z = 0, zz = 2; team.batters[roster[t][x]].id.name[z] != ','; z++, zz++)
                work[zz] = team.batters[roster[t][x]].id.name[z];
            work[zz++] = '\0';

            for (gp = zz = 0; zz < 11; zz++)
                gp += team.batters[roster[t][x]].fielding[zz].games;

            if (gp != team.batters[roster[t][x]].fielding[1].games) {
                /* played more positions than pitcher */
                singles = team.batters[roster[t][x]].hitting.hits - (team.batters[roster[t][x]].hitting.homers +
                          team.batters[roster[t][x]].hitting.triples + team.batters[roster[t][x]].hitting.doubles);
                if (team.batters[roster[t][x]].hitting.sf == -1)
                    sf = 0;
                else
                    sf = team.batters[roster[t][x]].hitting.sf;
                strcat (&work[0], "; HR-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.homers, 'l'));
                strcat (&work[0], ", BA-");
                strcat (&work[0], (char *) do_average (team.batters[roster[t][x]].hitting.hits,
                                                     team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", SA-");
                strcat (&work[0], (char *) do_average (((team.batters[roster[t][x]].hitting.homers * 4) +
                                                       (team.batters[roster[t][x]].hitting.triples * 3) +
                                                       (team.batters[roster[t][x]].hitting.doubles * 2) +
                                                     singles), team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", OBA-");
                strcat (&work[0], (char *) do_average ((team.batters[roster[t][x]].hitting.hits +
                                                          team.batters[roster[t][x]].hitting.bb +
                                                          team.batters[roster[t][x]].hitting.hbp),
                                 (team.batters[roster[t][x]].hitting.atbats + team.batters[roster[t][x]].hitting.bb +
                                                                        sf + team.batters[roster[t][x]].hitting.hbp)));
                strcat (&work[0], ", SB-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.sb, 'l'));
                strcat (&work[0], ", POS: ");
                for (z = zz = 0; zz < 11; zz++)
                    if (team.batters[roster[t][x]].fielding[zz].games) {
                        if (z)
                            strcat (&work[0], " ");
                        if (zz == 10)
                            strcat (&work[0], "OF");
                        else
                            strcat (&work[0], figure_pos (zz));
                        strcat (&work[0], "-");
                        strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].fielding[zz].games, 'l'));
                        z = 1;
                    }
            }
            if (team.batters[roster[t][x]].fielding[1].games) {
                /* this player [also] pitched */
                for (zz = 0; zz < 13; zz++)
                    if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[zz].id.name[0]))
                        break;
                strcat (&work[0], "; IP-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.innings, 'l'));
                strcat (&work[0], ", ERA-");
                strcat (&work[0], (char *) do_era (team.pitchers[zz].pitching.er * 9, team.pitchers[zz].pitching.innings, team.pitchers[zz].pitching.thirds));
                strcat (&work[0], ", SO-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.so, 'l'));
                strcat (&work[0], ", BB-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.walks, 'l'));
                strcat (&work[0], ", G-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.games, 'l'));
                strcat (&work[0], ", GS-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.games_started, 'l'));
                strcat (&work[0], ", SAVES-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.saves, 'l'));
            }

            labp = gtk_button_new_with_label (work);
            gtk_table_attach_defaults (GTK_TABLE (plytable), GTK_WIDGET (labp), 0, 1, y, y + 1);
            g_signal_connect (G_OBJECT (labp), "clicked", G_CALLBACK (CBplybutton), GINT_TO_POINTER (x));
            y++;
        }
    gtk_table_resize (GTK_TABLE (plytable), y, 1);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), vbox, TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Players Unavailable:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

    /* add a table */
    plytable = gtk_table_new (25, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (plytable), 2);
    gtk_table_set_col_spacings (GTK_TABLE (plytable), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), plytable);

    for (x = 0, y = 0; x < 28; x++)
        if (which[t][x] == 1 || team2.batters[roster[t][x]].id.injury) {
            cc = index (&team.batters[roster[t][x]].id.name[0], ',');
            if (cc == NULL)
                break;
            work[0] = *(cc + 2);
            work[1] = ' ';
            for (z = 0, zz = 2; team.batters[roster[t][x]].id.name[z] != ','; z++, zz++)
                work[zz] = team.batters[roster[t][x]].id.name[z];
            work[zz++] = '\0';

            labp = gtk_button_new_with_label (work);
            gtk_table_attach_defaults (GTK_TABLE (plytable), GTK_WIDGET (labp), 0, 1, y, y + 1);
            y++;
        }
    if (!y)
        y = 1;
    gtk_table_resize (GTK_TABLE (plytable), y, 1);

    but1 = gtk_button_new_with_label ("CANCEL");
    gtk_signal_connect (GTK_OBJECT (but1), "clicked", G_CALLBACK (CanLineup), GINT_TO_POINTER (sock));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->action_area), but1, TRUE, TRUE, 0);
    but2 = gtk_button_new_with_label ("Reset");
    gtk_signal_connect (GTK_OBJECT (but2), "clicked", G_CALLBACK (ResetLDsp), NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->action_area), but2, TRUE, TRUE, 0);
    but3 = gtk_button_new_with_label ("Make Change(s)");
    gtk_signal_connect (GTK_OBJECT (but3), "clicked", G_CALLBACK (MakeLineupChange), GINT_TO_POINTER (sock));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->action_area), but3, TRUE, TRUE, 0);

    gtk_widget_show_all (swin);
}

void
ResetLDsp (GtkWidget *widget, gpointer pdata) {
    gchar work[256], *cc;
    gint x, y, z, sf, singles;

    if (batter != 99) {
        /* user's team is at bat */
        newbatter = newbr[0] = newbr[1] = newbr[2] = 99;

        cc = index (&team.batters[roster[t][batter]].id.name[0], ',');
        work[0] = *(cc + 2);
        work[1] = ' ';
        for (z = 0, y = 2; team.batters[roster[t][batter]].id.name[z] != ','; z++, y++)
            work[y] = team.batters[roster[t][batter]].id.name[z];
        work[y++] = '\0';
        gtk_button_set_label (GTK_BUTTON (batterbutton), work);

        for (x = 0; x < 3; x++)
            if (br[x] != 99) {
                cc = index (&team.batters[roster[t][br[x]]].id.name[0], ',');
                work[0] = *(cc + 2);
                work[1] = ' ';
                for (z = 0, y = 2; team.batters[roster[t][br[x]]].id.name[z] != ','; z++, y++)
                    work[y] = team.batters[roster[t][br[x]]].id.name[z];
                work[y++] = '\0';
                if (!x)
                    gtk_button_set_label (GTK_BUTTON (br1button), work);
                else
                    if (x == 1)
                        gtk_button_set_label (GTK_BUTTON (br2button), work);
                    else
                        gtk_button_set_label (GTK_BUTTON (br3button), work);
            }
    }
    else {
        for (x = 0; x < 10; x++)
            newlu[x] = 99;

        for (x = 0; x < 9; x++) {
            work[0] = (x + 1) + '0';
            work[1] = '-';
            work[2] = ' ';
            cc = index (&team.batters[roster[t][x]].id.name[0], ',');
            work[3] = *(cc + 2);
            work[4] = ' ';
            for (z = 0, y = 5; team.batters[roster[t][x]].id.name[z] != ','; z++, y++)
                work[y] = team.batters[roster[t][x]].id.name[z];
            work[y++] = '\0';
            strcat (&work[0], "  ");
            strcat (&work[0], figure_pos (bo[t][x].pos[0]));

            /* add some stats */
            if (batter == 99 && bo[t][x].pos[0] == 1) {
                for (y = 0; y < 13; y++)
                    if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[y].id.name[0]))
                        break;
                strcat (&work[0], "; IP-");
                strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
                strcat (&work[0], ", ERA-");
                strcat (&work[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings,
                                                                                       team.pitchers[y].pitching.thirds));
                strcat (&work[0], ", SO-");
                strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
                strcat (&work[0], ", BB-");
                strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
                strcat (&work[0], ", G-");
                strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));
            }
            else {
                /* not a pitcher */
                singles = team.batters[roster[t][x]].hitting.hits - (team.batters[roster[t][x]].hitting.homers +
                          team.batters[roster[t][x]].hitting.triples + team.batters[roster[t][x]].hitting.doubles);
                if (team.batters[roster[t][x]].hitting.sf == -1)
                    sf = 0;
                else
                    sf = team.batters[roster[t][x]].hitting.sf;
                strcat (&work[0], "; HR-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.homers, 'l'));
                strcat (&work[0], ", BA-");
                strcat (&work[0], (char *) do_average (team.batters[roster[t][x]].hitting.hits,
                                                     team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", SA-");
                strcat (&work[0], (char *) do_average (((team.batters[roster[t][x]].hitting.homers * 4) + (team.batters[roster[t][x]].hitting.triples * 3) +
                                                    (team.batters[roster[t][x]].hitting.doubles * 2) + singles), team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", OBA-");
                strcat (&work[0], (char *) do_average ((team.batters[roster[t][x]].hitting.hits + team.batters[roster[t][x]].hitting.bb +
                                                          team.batters[roster[t][x]].hitting.hbp),
                                 (team.batters[roster[t][x]].hitting.atbats + team.batters[roster[t][x]].hitting.bb +
                                                                        sf + team.batters[roster[t][x]].hitting.hbp)));
                strcat (&work[0], ", SB-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.sb, 'l'));
            }

            gtk_button_set_label (GTK_BUTTON (lubutton[x]), work);
        }

        if (dhind) {
            /* there is a DH */
            strcpy (&work[0], "Pitcher - ");
            cc = index (&team.pitchers[p[t].pitcher[0]].id.name[0], ',');
            work[10] = *(cc + 2);
            work[11] = ' ';
            for (z = 0, y = 12; team.pitchers[p[t].pitcher[0]].id.name[z] != ','; z++, y++)
                work[y] = team.pitchers[p[t].pitcher[0]].id.name[z];
            work[y++] = '\0';

            /* add some stats */
            for (y = 0; y < 13; y++)
                if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[y].id.name[0]))
                    break;
            strcat (&work[0], "; IP-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
            strcat (&work[0], ", ERA-");
            strcat (&work[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings, team.pitchers[y].pitching.thirds));
            strcat (&work[0], ", SO-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
            strcat (&work[0], ", BB-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
            strcat (&work[0], ", G-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));

            gtk_button_set_label (GTK_BUTTON (lubutton[9]), work);
        }
    }
}

void
MakeLineupChange (GtkWidget *widget, gpointer pdata) {
    gint y = 0, x, sock = GPOINTER_TO_INT (pdata);
    gchar buf[500];

    if (batter != 99) {
        /* user's team is at bat */
        if (newbatter != 99) {
            plyr[y] = roster[t][newbatter];
            pos[y] = 11;
            bonum[y] = batter;
            y++;
        }
        if (newbr[0] != 99) {
            plyr[y] = roster[t][newbr[0]];
            pos[y] = 12;
            bonum[y] = br[0];
            y++;
        }
        if (newbr[1] != 99) {
            plyr[y] = roster[t][newbr[1]];
            pos[y] = 12;
            bonum[y] = br[1];
            y++;
        }
        if (newbr[2] != 99) {
            plyr[y] = roster[t][newbr[2]];
            pos[y] = 12;
            bonum[y] = br[2];
            y++;
        }
    }
    else
        /* user's team is in the field */
        for (x = y = 0; x < 10; x++)
            if (newlu[x] != 99) {
                plyr[y] = roster[t][newlu[x]];
                if (dhind && x == 9)
                    pos[y] = 1;
                else
                    pos[y] = bo[t][x].pos[0];
                bonum[y] = x;
                y++;
            }

    if (bonum[0] == 99 || plyr[0] == 99 || !pos[0])
        strcpy (&buf[0], "No Change\n");
    else {
        for (y = x = 0; x < 10; x++, y += 6) {
            if (plyr[x] == 99 || bonum[x] == 99 || !pos[x])
                break;
            buf[y] = plyr[x] / 10 + '0';
            buf[y + 1] = plyr[x] % 10 + '0';
            buf[y + 2] = bonum[x] / 10 + '0';
            buf[y + 3] = bonum[x] % 10 + '0';
            buf[y + 4] = pos[x] / 10 + '0';
            buf[y + 5] = pos[x] % 10 + '0';
        }
        buf[y] = '\n';
        buf[y + 1] = '\0';
    }
    sock_puts (sock, buf);

    DestroyDialog (swin, swin);
    lineupsw = 3;    /* set to indicate we're finished with this dialog */
}

void
CanLineup (GtkWidget *widget, gpointer pdata) {
    gint sock = GPOINTER_TO_INT (pdata);
    gchar buf[50];

    strcpy (&buf[0], "No Change\n");
    sock_puts (sock, buf);

    DestroyDialog (swin, swin);
    lineupsw = 3;    /* set to indicate we're finished with this dialog */
}

void
CBplybutton (GtkWidget *widget, gpointer pdata) {
    gchar plabel[256], work[256], *cc;
    gint x = GPOINTER_TO_INT (pdata), y, z;

    strcpy (&plabel[0], gtk_button_get_label (GTK_BUTTON (widget)));
    cc = index (&plabel[0], ';');
    *cc = '\0';

    if (batter != 99) {
        /* user's team is at bat */

        /* ensure same player isn't subbed twice */
        if (newbatter != x && newbr[0] != x && newbr[1] != x && newbr[2] != x) {
            if (batterind) {
                newbatter = x;
                gtk_button_set_label (GTK_BUTTON (batterbutton), plabel);
            }
            if (brind[0]) {
                newbr[0] = x;
                gtk_button_set_label (GTK_BUTTON (br1button), plabel);
            }
            if (brind[1]) {
                newbr[1] = x;
                gtk_button_set_label (GTK_BUTTON (br2button), plabel);
            }
            if (brind[2]) {
                newbr[2] = x;
                gtk_button_set_label (GTK_BUTTON (br3button), plabel);
            }
        }
    }
    else {
        gchar RLPitch[256] = "For a player to pitch he must have pitched in real life.", *msg[5];

        for (z = 0; z < 5; z++)
            msg[z] = NULL;

        /* ensure same player isn't subbed twice */
        for (y = 0; y < 10; y++)
            if (newlu[y] == x)
                return;
        for (y = 0; y < 10; y++)
            if (lubutind[y]) {
                /* only a player who's pitched in real life can replace a pitcher */
                if (y < 9 && bo[t][y].pos[0] == 1)
                    if (!team.batters[roster[t][x]].fielding[1].games) {
                        msg[0] = RLPitch;
                        outMessage (msg);
                        return;
                    }

                if (y == 9)
                    strcpy (&work[0], "Pitcher - ");
                else {
                    work[0] = (y + 1) + '0';
                    work[1] = '-';
                    work[2] = ' ';
                    work[3] = '\0';
                }
                strcat (&work[0], &plabel[0]);
                if (y < 9) {
                    strcat (&work[0], "  ");
                    strcat (&work[0], figure_pos (bo[t][y].pos[0]));
                }
                newlu[y] = x;
                gtk_button_set_label (GTK_BUTTON (lubutton[y]), work);
            }
    }
}

void
CBbatterbutton (GtkWidget *widget, gpointer pdata) {
    batterind = 1;
    brind[0] = brind[1] = brind[2] = 0;
}

void
CBbr1button (GtkWidget *widget, gpointer pdata) {
    brind[0] = 1;
    batterind = brind[1] = brind[2] = 0;
}

void
CBbr2button (GtkWidget *widget, gpointer pdata) {
    brind[1] = 1;
    batterind = brind[0] = brind[2] = 0;
}

void
CBbr3button (GtkWidget *widget, gpointer pdata) {
    brind[2] = 1;
    batterind = brind[0] = brind[1] = 0;
}

void
CBlubutton (GtkWidget *widget, gpointer pdata) {
    gint x = GPOINTER_TO_INT (pdata), y;

    for (y = 0; y < 10; y++)
        lubutind[y] = 0;

    lubutind[x] = 1;
}

GtkWidget *dwin, *but4;
gint ckbutind[6], sk;
gchar dresp = ' ';

void
DefensiveCommand (gchar *valid, gint sock) {
    /*
      Defensive commands (multiple commands are acceptable):
          C - infield in at corners
          W - entire infield in
          H - hold runner on first base
          O - outfield in
          F - guard the first base line
          T - guard the third base line

          (any of the following commands stops the acceptance of commands and initiates the action):

          N - pitch to batter
          I - intentionally walk batter
          L - see lineups and make changes
          P - pitchout
          1 - pickoff attempt at first base
          2 - pickoff attempt at second base
          3 - pickoff attempt at third base
          A - computer manages from here on out
    */
    GtkWidget *box, *hbox, *but1, *but2, *but3, *but5, *but6, *but7, *but8, *ckbutton[6];
    GtkLabel *lab;
    gint x;

    defdialsw = 1;    /* set to indicate we're processing this dialog */
    sk = sock;

    dwin = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dwin), "Defensive Commands");
    gtk_window_set_default_size (GTK_WINDOW (dwin), 300, 100);
    gtk_window_set_modal (GTK_WINDOW (dwin), TRUE);
    gtk_signal_connect (GTK_OBJECT (dwin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dwin)->vbox), box, TRUE, TRUE, 0);

    ckbutton[0] = gtk_check_button_new_with_label ("Infield In at Corners");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[0], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[0]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (0));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[0]), FALSE);

    ckbutton[1] = gtk_check_button_new_with_label ("Entire Infield In");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[1], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[1]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (1));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[1]), FALSE);

    ckbutton[2] = gtk_check_button_new_with_label ("Hold Runner on First");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[2], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[2]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (2));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[2]), FALSE);

    ckbutton[3] = gtk_check_button_new_with_label ("Outfield In");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[3], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[3]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (3));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[3]), FALSE);

    ckbutton[4] = gtk_check_button_new_with_label ("Guard the First Base Line");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[4], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[4]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (4));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[4]), FALSE);

    ckbutton[5] = gtk_check_button_new_with_label ("Guard the Third Base Line");
    gtk_box_pack_start (GTK_BOX (box), ckbutton[5], TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (ckbutton[5]), "clicked", G_CALLBACK (CBckbutDef), GINT_TO_POINTER (5));
    gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[5]), FALSE);

    for (x = 0; x < 6; x++)
        ckbutind[x] = 0;

    if (index (valid, 'C'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[0]), TRUE);
    if (index (valid, 'W'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[1]), TRUE);
    if (index (valid, 'H'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[2]), TRUE);
    if (index (valid, 'O'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[3]), TRUE);
    if (index (valid, 'F'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[4]), TRUE);
    if (index (valid, 'T'))
        gtk_widget_set_sensitive (GTK_WIDGET (ckbutton[5]), TRUE);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Choosing one of the following initiates the action:", NULL);
    gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (lab), TRUE, TRUE, 0);

    box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dwin)->action_area), box, TRUE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but1 = gtk_button_new_with_label ("Pitch to Batter");
    gtk_signal_connect (GTK_OBJECT (but1), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (0));
    gtk_box_pack_start (GTK_BOX (hbox), but1, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but1), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but2 = gtk_button_new_with_label ("Intentionally Walk Batter");
    gtk_signal_connect (GTK_OBJECT (but2), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (1));
    gtk_box_pack_start (GTK_BOX (hbox), but2, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but2), FALSE);

    but3 = gtk_button_new_with_label ("Pitchout");
    gtk_signal_connect (GTK_OBJECT (but3), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (2));
    gtk_box_pack_start (GTK_BOX (hbox), but3, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but3), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but4 = gtk_button_new_with_label ("Pickoff Attempt at First");
    gtk_signal_connect (GTK_OBJECT (but4), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (3));
    gtk_box_pack_start (GTK_BOX (hbox), but4, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but4), FALSE);

    but5 = gtk_button_new_with_label ("Pickoff Attempt at Second");
    gtk_signal_connect (GTK_OBJECT (but5), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (4));
    gtk_box_pack_start (GTK_BOX (hbox), but5, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but5), FALSE);

    but6 = gtk_button_new_with_label ("Pickoff Attempt at Third");
    gtk_signal_connect (GTK_OBJECT (but6), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (5));
    gtk_box_pack_start (GTK_BOX (hbox), but6, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but6), FALSE);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    but7 = gtk_button_new_with_label ("See Lineup");
    gtk_signal_connect (GTK_OBJECT (but7), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (6));
    gtk_box_pack_start (GTK_BOX (hbox), but7, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but7), FALSE);

    but8 = gtk_button_new_with_label ("Computer Takes Over");
    gtk_signal_connect (GTK_OBJECT (but8), "clicked", G_CALLBACK (ReturnDef), GINT_TO_POINTER (7));
    gtk_box_pack_start (GTK_BOX (hbox), but8, TRUE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (but8), FALSE);

    if (index (valid, 'N'))
        gtk_widget_set_sensitive (GTK_WIDGET (but1), TRUE);
    if (index (valid, 'I'))
        gtk_widget_set_sensitive (GTK_WIDGET (but2), TRUE);
    if (index (valid, 'P'))
        gtk_widget_set_sensitive (GTK_WIDGET (but3), TRUE);
    if (index (valid, '2'))
        gtk_widget_set_sensitive (GTK_WIDGET (but5), TRUE);
    if (index (valid, '3'))
        gtk_widget_set_sensitive (GTK_WIDGET (but6), TRUE);
    if (index (valid, 'L'))
        gtk_widget_set_sensitive (GTK_WIDGET (but7), TRUE);
    if (index (valid, 'A'))
        gtk_widget_set_sensitive (GTK_WIDGET (but8), TRUE);

    gtk_widget_show_all (dwin);
}

void
ReturnDef (GtkWidget *widget, gpointer pdata) {
    gint resp = GPOINTER_TO_INT (pdata), x, y;

    switch (resp) {
        case 0:
            dresp = 'N';
            break;
        case 1:
            dresp = 'I';
            break;
        case 2:
            dresp = 'P';
            break;
        case 3:
            dresp = '1';
            break;
        case 4:
            dresp = '2';
            break;
        case 5:
            dresp = '3';
            break;
        case 6:
            dresp = 'L';
            break;
        default:
            dresp = 'A';
    }

    for (y = x = 0; x < 6; x++)
        if (ckbutind[x]) {
            switch (x) {
                case 0:
                    buffer1[y] = 'C';
                    break;
                case 1:
                    buffer1[y] = 'W';
                    break;
                case 2:
                    buffer1[y] = 'H';
                    break;
                case 3:
                    buffer1[y] = 'O';
                    break;
                case 4:
                    if (ckbutind[5])
                        buffer1[y] = 'B';
                    else
                        buffer1[y] = 'F';
                    x = 5;  /* so that we don't process ckbutind[5] */
                    break;
                default:
                    buffer1[y] = 'T';
            }
            y++;
        }

    buffer1[y] = dresp;
    buffer1[y + 1] = '\n';
    buffer1[y + 2] = '\0';
    currentd = dresp;
    /* if the user chooses to let the computer manage from here on out then it's no longer a human vs human game */
    if (dresp == 'A') {
        gint z;
        gchar *msg[5];

        for (z = 0; z < 5; z++)
            msg[z] = NULL;

        msg[0] = "Are you sure you want the computer to take over managing your team?\n\n";
        if (!ShallWeContinue (msg))
            return;

        currenttype = ' ';
    }

    sock_puts (sk, buffer1);
    if (index (&buffer1[0], 'L'))
        lineupsw = 1;
    else
        lineupsw = 0;

    DestroyDialog (dwin, dwin);
    defdialsw = 2;     /* set to indicate we're finished with this dialog */
}

void
CBckbutDef (GtkWidget *widget, gpointer pdata) {
    gint x = GPOINTER_TO_INT (pdata);

    if (ckbutind[x])
        ckbutind[x] = 0;
    else
        ckbutind[x] = 1;

    if (ckbutind[2])
        /* but4 (throw over to first) will be made sensitive only when the "Hold Runner on First" box is checkmarked */
        gtk_widget_set_sensitive (GTK_WIDGET (but4), TRUE);
    else
        gtk_widget_set_sensitive (GTK_WIDGET (but4), FALSE);
}

gint csel, cplyr, cbonum, cpos;
GtkWidget *cwin;
GtkLabel *labsub;
struct {
    int player[30],
        pos[30];
} cbo[9];

void
ChangeLineup (gint sock) {
    GtkWidget *vbox, *hbox, *vbox1, *vbox2, *vbox3, *vbox4, *but2, *but3, *plytable, *scrolled_window, *labp, *sep;
    GtkLabel *lab;
    gint bcol, x, y, z, zz, used[28], ha, singles, sf, gp;
    gchar work[500], work1[100], *cc, inji;
    struct {
        int pitcher[15];
    } p;

    /* init */
    t = cpos = 0;
    csel = cplyr = cbonum =  99;

    for (x = 0; x < 9; x++)
        for (y = 0; y < 30; y++)
            cbo[x].player[y] = cbo[x].pos[y] = 99;
    for (x = 0; x < 15; x++)
        p.pitcher[x] = 99;
    for (x = 0; x < 28; x++)
        roster[0][x] = which[0][x] = used[x] = 99;

    work[0] = buffer[2];
    work[1] = buffer[3];
    work[2] = '\0';
    cbonum = atoi (&work[0]);
    inji = buffer[4];  /* 0 = not an injury, 1 = injury */
    ha = buffer[5] - '0';

    if (ha) {
        team = home;
        team2 = home_season;  /* needed to see injury */
    }
    else {
        team = visitor;
        team2 = visitor_season;  /* needed to see injury */
    }

    clineupsw = 2;    /* set to indicate we're processing this dialog */

    /* fill in batting order */
    for (bcol = 6, x = 0; x < 9; x++, bcol++)
        for (y = 0; buffer[bcol] != ':'; y++) {
            work[0] = buffer[bcol];
            work[1] = buffer[bcol + 1];
            work[2] = '\0';
            bcol += 2;
            cbo[x].player[y] = atoi (&work[0]);

            work[0] = buffer[bcol];
            work[1] = buffer[bcol + 1];
            work[2] = '\0';
            bcol += 2;
            cbo[x].pos[y] = atoi (&work[0]);
        }
    /* fill in pitchers */
    for (x = 0; buffer[bcol] != ':'; x++, bcol++) {
        work[0] = buffer[bcol];
        work[1] = buffer[bcol + 1];
        work[2] = '\0';
        bcol += 2;
        p.pitcher[x] = atoi (&work[0]);
    }

    /* designated hitter? */
    if (dhind)
        for (zz = 0; zz < 15; zz++) {
            if (p.pitcher[zz] == 99)
                continue;
            for (x = 0; x < 28; x++)
                if (!strcmp (&team.batters[x].id.name[0], &team.pitchers[p.pitcher[zz]].id.name[0]))
                    break;
            for (y = 0; y < 28; y++)
                if (used[y] == 99) {
                    used[y] = x;
                    break;
                }
        }

    /* determine players already used */
    for (zz = 0; zz < 9; zz++)
        for (x = 1; x < 30; x++) {
            if (cbo[zz].player[x] == 99)
                continue;
            if (cbo[zz].player[x] == cbo[zz].player[x - 1])  /* same player, different positions? */
                continue;
            for (y = 0; y < 28; y++)
                if (used[y] == 99) {
                    used[y] = cbo[zz].player[x];
                    break;
                }
        }

    /* fill in roster, which holds availability */
    for (x = 0; x < 9; x++) {
        roster[0][x] = cbo[x].player[0];
        which[0][x] = 0;
    }
    for (x = 0, y = 9; x < 28; x++, y++)
        if (used[x] != 99) {
            roster[0][y] = used[x];
            if (dhind && y == 9)
                which[0][y] = 0;
            else
                which[0][y] = 1;
        }
        else
            break;
    for (y = 0; y < 28; y++)
        for (zz = 0; zz < 28; zz++)
            if (y == roster[0][zz])
                break;
            else
                if (roster[0][zz] == 99) {
                    roster[0][zz] = y;
                    which[0][zz] = 2;
                    break;
                }

    strcpy (&work[0], "Change Lineup - ");
    if (!t) {
        strcat (&work[0], &vteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], &visiting_team[0]);
    }
    else {
        strcat (&work[0], &hteamyr[0]);
        strcat (&work[0], " ");
        strcat (&work[0], &home_team[0]);
    }

    cwin = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (cwin), work);
    gtk_window_set_default_size (GTK_WINDOW (cwin), 300, 100);
    gtk_window_set_modal (GTK_WINDOW (cwin), TRUE);
    gtk_signal_connect (GTK_OBJECT (cwin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cwin)->vbox), hbox, TRUE, TRUE, 0);

    vbox1 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
    vbox3 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox3, TRUE, TRUE, 0);
    vbox4 = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), vbox4, TRUE, TRUE, 0);
    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "LINEUP:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);

    for (x = 0; x < 9; x++) {
        work[0] = (x + 1) + '0';
        work[1] = '-';
        work[2] = ' ';
        cc = index (&team.batters[roster[t][x]].id.name[0], ',');
        work[3] = *(cc + 2);
        work[4] = ' ';
        for (z = 0, y = 5; team.batters[roster[t][x]].id.name[z] != ','; z++, y++)
            work[y] = team.batters[roster[t][x]].id.name[z];
        work[y++] = '\0';

        strcat (&work[0], "  ");
        strcat (&work[0], figure_pos (cbo[x].pos[0]));

        /* add some stats */
        if (cbo[x].pos[0] == 1) {
            for (y = 0; y < 13; y++)
                if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[y].id.name[0]))
                    break;
            strcat (&work[0], "; IP-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.innings, 'l'));
            strcat (&work[0], ", ERA-");
            strcat (&work[0], (char *) do_era (team.pitchers[y].pitching.er * 9, team.pitchers[y].pitching.innings, team.pitchers[y].pitching.thirds));
            strcat (&work[0], ", SO-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.so, 'l'));
            strcat (&work[0], ", BB-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.walks, 'l'));
            strcat (&work[0], ", G-");
            strcat (&work[0], cnvt_int2str (team.pitchers[y].pitching.games, 'l'));
        }
        else {
            /* not a pitcher */
            singles = team.batters[roster[t][x]].hitting.hits - (team.batters[roster[t][x]].hitting.homers +
                      team.batters[roster[t][x]].hitting.triples + team.batters[roster[t][x]].hitting.doubles);
            if (team.batters[roster[t][x]].hitting.sf == -1)
                sf = 0;
            else
                sf = team.batters[roster[t][x]].hitting.sf;
            strcat (&work[0], "; HR-");
            strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.homers, 'l'));
            strcat (&work[0], ", BA-");
            strcat (&work[0], (char *) do_average (team.batters[roster[t][x]].hitting.hits,
                                                 team.batters[roster[t][x]].hitting.atbats));
            strcat (&work[0], ", SA-");
            strcat (&work[0], (char *) do_average (((team.batters[roster[t][x]].hitting.homers * 4) +
                                                   (team.batters[roster[t][x]].hitting.triples * 3) +
                                                   (team.batters[roster[t][x]].hitting.doubles * 2) +
                                                 singles), team.batters[roster[t][x]].hitting.atbats));
            strcat (&work[0], ", OBA-");
            strcat (&work[0], (char *) do_average ((team.batters[roster[t][x]].hitting.hits +
                                                      team.batters[roster[t][x]].hitting.bb +
                                                      team.batters[roster[t][x]].hitting.hbp),
                             (team.batters[roster[t][x]].hitting.atbats + team.batters[roster[t][x]].hitting.bb +
                                                                    sf + team.batters[roster[t][x]].hitting.hbp)));
            strcat (&work[0], ", SB-");
            strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.sb, 'l'));
        }

        lab = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
        gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);
        gtk_label_set_text (GTK_LABEL (lab), work);
    }

    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);
    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), sep, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
    lab = g_object_new (GTK_TYPE_LABEL, "label", "Sub - ", NULL);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (lab), TRUE, TRUE, 0);
    labsub = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (labsub), TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (lab), TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cwin)->vbox), vbox, TRUE, TRUE, 0);

    if (inji == '1') {
        cc = index (&team.batters[cbo[cbonum].player[0]].id.name[0], ',');
        work1[0] = *(cc + 2);
        work1[1] = ' ';
        for (z = 0, y = 2; team.batters[cbo[cbonum].player[0]].id.name[z] != ','; z++, y++)
            work1[y] = team.batters[cbo[cbonum].player[0]].id.name[z];
        work1[y++] = '\0';
        strcpy (&work[0], &work1[0]);
        strcat (&work[0], " is injured.");
        lab = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);
        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);
        strcpy (&work[0], "Select a Player to Replace Him");
    }
    else {
        for (zz = 1; zz < 30; zz++)
            if (cbo[cbonum].pos[zz] != 10 && cbo[cbonum].pos[zz] != 11)
                break;
        strcpy (&work[0], "Select a Player to Replace ");
        cc = index (&team.batters[cbo[cbonum].player[zz]].id.name[0], ',');
        work1[0] = *(cc + 2);
        work1[1] = ' ';
        for (z = 0, y = 2; team.batters[cbo[cbonum].player[zz]].id.name[z] != ','; z++, y++)
            work1[y] = team.batters[cbo[cbonum].player[zz]].id.name[z];
        work1[y++] = '\0';
        strcat (&work[0], &work1[0]);

        strcat (&work[0], " at ");
        strcat (&work[0], figure_pos (cbo[cbonum].pos[zz]));
    }

    lab = g_object_new (GTK_TYPE_LABEL, "label", work, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Available Players:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);
    lab = g_object_new (GTK_TYPE_LABEL, "label", "(click on an available player or click the \"Leave\" button below)", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

    /* add a table */
    plytable = gtk_table_new (25, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (plytable), 2);
    gtk_table_set_col_spacings (GTK_TABLE (plytable), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), plytable);

    for (x = 0, y = 0; x < 28; x++)
        if (which[t][x] > 1 && !team2.batters[roster[t][x]].id.injury) {
            cc = index (&team.batters[roster[t][x]].id.name[0], ',');
            if (cc == NULL)
                break;
            work[0] = *(cc + 2);
            work[1] = ' ';
            for (z = 0, zz = 2; team.batters[roster[t][x]].id.name[z] != ','; z++, zz++)
                work[zz] = team.batters[roster[t][x]].id.name[z];
            work[zz++] = '\0';

            for (gp = zz = 0; zz < 11; zz++)
                gp += team.batters[roster[t][x]].fielding[zz].games;

            if (gp != team.batters[roster[t][x]].fielding[1].games) {
                /* played more positions than pitcher */
                singles = team.batters[roster[t][x]].hitting.hits - (team.batters[roster[t][x]].hitting.homers +
                          team.batters[roster[t][x]].hitting.triples + team.batters[roster[t][x]].hitting.doubles);
                if (team.batters[roster[t][x]].hitting.sf == -1)
                    sf = 0;
                else
                    sf = team.batters[roster[t][x]].hitting.sf;
                strcat (&work[0], "; HR-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.homers, 'l'));
                strcat (&work[0], ", BA-");
                strcat (&work[0], (char *) do_average (team.batters[roster[t][x]].hitting.hits,
                                                     team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", SA-");
                strcat (&work[0], (char *) do_average (((team.batters[roster[t][x]].hitting.homers * 4) +
                                                       (team.batters[roster[t][x]].hitting.triples * 3) +
                                                       (team.batters[roster[t][x]].hitting.doubles * 2) +
                                                     singles), team.batters[roster[t][x]].hitting.atbats));
                strcat (&work[0], ", OBA-");
                strcat (&work[0], (char *) do_average ((team.batters[roster[t][x]].hitting.hits +
                                                          team.batters[roster[t][x]].hitting.bb +
                                                          team.batters[roster[t][x]].hitting.hbp),
                                 (team.batters[roster[t][x]].hitting.atbats + team.batters[roster[t][x]].hitting.bb +
                                                                        sf + team.batters[roster[t][x]].hitting.hbp)));
                strcat (&work[0], ", SB-");
                strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].hitting.sb, 'l'));
                strcat (&work[0], ", POS: ");
                for (z = zz = 0; zz < 11; zz++)
                    if (team.batters[roster[t][x]].fielding[zz].games) {
                        if (z)
                            strcat (&work[0], " ");
                        if (zz == 10)
                            strcat (&work[0], "OF");
                        else
                            strcat (&work[0], figure_pos (zz));
                        strcat (&work[0], "-");
                        strcat (&work[0], cnvt_int2str (team.batters[roster[t][x]].fielding[zz].games, 'l'));
                        z = 1;
                    }
            }

            if (team.batters[roster[t][x]].fielding[1].games) {
                /* this player [also] pitched */
                for (zz = 0; zz < 13; zz++)
                    if (!strcmp (&team.batters[roster[t][x]].id.name[0], &team.pitchers[zz].id.name[0]))
                        break;
                strcat (&work[0], "; IP-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.innings, 'l'));
                strcat (&work[0], ", ERA-");
                strcat (&work[0], (char *) do_era (team.pitchers[zz].pitching.er * 9, team.pitchers[zz].pitching.innings, team.pitchers[zz].pitching.thirds));
                strcat (&work[0], ", SO-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.so, 'l'));
                strcat (&work[0], ", BB-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.walks, 'l'));
                strcat (&work[0], ", G-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.games, 'l'));
                strcat (&work[0], ", GS-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.games_started, 'l'));
                strcat (&work[0], ", SAVES-");
                strcat (&work[0], cnvt_int2str (team.pitchers[zz].pitching.saves, 'l'));
            }

            labp = gtk_button_new_with_label (work);
            gtk_table_attach_defaults (GTK_TABLE (plytable), GTK_WIDGET (labp), 0, 1, y, y + 1);
            g_signal_connect (G_OBJECT (labp), "clicked", G_CALLBACK (CBcplybutton), GINT_TO_POINTER (x));
            y++;
        }
    gtk_table_resize (GTK_TABLE (plytable), y, 1);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cwin)->vbox), vbox, TRUE, TRUE, 0);

    lab = g_object_new (GTK_TYPE_LABEL, "label", "Players Unavailable:", NULL);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (lab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

    /* add a table */
    plytable = gtk_table_new (25, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (plytable), 2);
    gtk_table_set_col_spacings (GTK_TABLE (plytable), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), plytable);

    for (x = 0, y = 0; x < 28; x++)
        if (which[t][x] == 1 || team2.batters[roster[t][x]].id.injury) {
            cc = index (&team.batters[roster[t][x]].id.name[0], ',');
            if (cc == NULL)
                break;
            work[0] = *(cc + 2);
            work[1] = ' ';
            for (z = 0, zz = 2; team.batters[roster[t][x]].id.name[z] != ','; z++, zz++)
                work[zz] = team.batters[roster[t][x]].id.name[z];
            work[zz++] = '\0';

            labp = gtk_button_new_with_label (work);
            gtk_table_attach_defaults (GTK_TABLE (plytable), GTK_WIDGET (labp), 0, 1, y, y + 1);
            y++;
        }
    if (!y)
        y = 1;
    gtk_table_resize (GTK_TABLE (plytable), y, 1);

    strcpy (&work[0], "Leave ");
    cc = index (&team.batters[cbo[cbonum].player[0]].id.name[0], ',');
    work1[0] = *(cc + 2);
    work1[1] = ' ';
    for (z = 0, y = 2; team.batters[cbo[cbonum].player[0]].id.name[z] != ','; z++, y++)
        work1[y] = team.batters[cbo[cbonum].player[0]].id.name[z];
    work1[y++] = '\0';
    strcat (&work[0], &work1[0]);
    but2 = gtk_button_new_with_label (work);
    gtk_signal_connect (GTK_OBJECT (but2), "clicked", G_CALLBACK (RLDsp), GINT_TO_POINTER (sock));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cwin)->action_area), but2, TRUE, TRUE, 0);
    but3 = gtk_button_new_with_label ("Make Change");
    gtk_signal_connect (GTK_OBJECT (but3), "clicked", G_CALLBACK (MakeLChange), GINT_TO_POINTER (sock));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cwin)->action_area), but3, TRUE, TRUE, 0);

    if (inji == '1')
        gtk_widget_set_sensitive (GTK_WIDGET (but2), FALSE);

    gtk_widget_show_all (cwin);
}

void
RLDsp (GtkWidget *widget, gpointer pdata) {
    gint z, sock = GPOINTER_TO_INT (pdata);
    gchar buf[50];
    gchar RLPitch[256] = "For a player to pitch he must have pitched in real life.", *msg[5];

    for (z = 0; z < 5; z++)
        msg[z] = NULL;

    /* only a player who's pitched in real life can replace a pitcher */
    if (cbo[cbonum].pos[1] == 1)
        if (!team.batters[cbo[cbonum].player[0]].fielding[1].games) {
            msg[0] = RLPitch;
            outMessage (msg);
            return;
        }

    cplyr = cbo[cbonum].player[0];
    cpos = cbo[cbonum].pos[1];

    buf[0] = cplyr / 10 + '0';
    buf[1] = cplyr % 10 + '0';
    buf[2] = cbonum / 10 + '0';
    buf[3] = cbonum % 10 + '0';
    buf[4] = cpos / 10 + '0';
    buf[5] = cpos % 10 + '0';

    buf[6] = '\n';
    buf[7] = '\0';

    sock_puts (sock, buf);

    DestroyDialog (cwin, cwin);
    clineupsw = 3;    /* set to indicate we're finished with this dialog */
}

void
MakeLChange (GtkWidget *widget, gpointer pdata) {
    gint sock = GPOINTER_TO_INT (pdata);
    gchar buf[50];

    if (csel == 99)
        /* the user did not select a substitute */
        return;

    cplyr = roster[0][csel];
    if (buffer[4] == '1')
        cpos = cbo[cbonum].pos[0];
    else
        cpos = cbo[cbonum].pos[1];

    buf[0] = cplyr / 10 + '0';
    buf[1] = cplyr % 10 + '0';
    buf[2] = cbonum / 10 + '0';
    buf[3] = cbonum % 10 + '0';
    buf[4] = cpos / 10 + '0';
    buf[5] = cpos % 10 + '0';

    buf[6] = '\n';
    buf[7] = '\0';

    sock_puts (sock, buf);

    DestroyDialog (cwin, cwin);
    clineupsw = 3;    /* set to indicate we're finished with this dialog */
}

void
CBcplybutton (GtkWidget *widget, gpointer pdata) {
    gint z, ctemp = GPOINTER_TO_INT (pdata);
    gchar RLPitch[256] = "For a player to pitch he must have pitched in real life.", *msg[5], plabel[256], *cc;

    for (z = 0; z < 5; z++)
        msg[z] = NULL;

    /* only a player who's pitched in real life can replace a pitcher */
    if (cbo[cbonum].pos[1] == 1)
        if (!team.batters[ctemp].fielding[1].games) {
            msg[0] = RLPitch;
            outMessage (msg);
            return;
        }

    strcpy (&plabel[0], gtk_button_get_label (GTK_BUTTON (widget)));
    cc = index (&plabel[0], ';');
    *cc = '\0';

    csel = ctemp;
    gtk_label_set_text (labsub, plabel);
}

gint
donot_delete_event (GtkWidget *widget, GdkEventConfigure *event) {
    return TRUE ;
}

