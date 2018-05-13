/* display some statistics */

#include <dirent.h>
#include "gtknsbc.h"
#include "prototypes.h"
#include "db.h"
#include "cglobal.h"
#include "net.h"

gchar stats[500000], statscmdstr[7], urind, psorrs;
gint whichstats, wri, sgi;
struct {
    int          id, year;
    char         league, division;
    struct bttr  batters;
    struct ptchr pitchers;
} teamdata[302];
gint nteams;
gchar uctname[301][50], yeara[5] = "0000";

/* show NSB season standings */
void
NSBStandings (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "S1\n");  /* we want NSB season standings */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }

    if (!strcmp (&buffer[0], "-1")) {
        /* this happens if the user has no season established */
        gchar NoDir[256] = "You have no season.  First set up a season.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No season established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Current NSB Season Standings");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box1);

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

    FillStandings ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, stats, strlen (&stats[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintStandings), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillStandings () {
    gchar *cc, *work = NULL, *work2 = NULL, teamy[5], teamid[5], tuctname[50];
    gint x, y = 0, nteams[6], loop, al1sw, nl1sw, ALNL, tid[2], ty[2], overpssw;
    struct {
        int id, year, wins, losses, gb;
        float pct;
        char ucteamname[50];
    } teams[6][300], teamh;

    for (loop = 0; loop < 6; loop++) {
        nteams[loop] = 0;
        for (x = 0; x < 300; x++)
            teams[loop][x].id = 0;
    }

    tid[0] = tid[1] = ty[0] = ty[1] = 0;
    cc = &buffer[0];
    /* store team id, wins, & losses, calculate pct */
    for (x = 0; cc < (&buffer[0] + strlen (&buffer[0])) && *cc != ':'; x++) {
        if (*cc == 'A')
            if (*(cc + 1) == 'E')
                loop = 0;
            else
                if (*(cc + 1) == 'C')
                    loop = 1;
                else
                    loop = 2;
        else
            if (*(cc + 1) == 'E')
                loop = 3;
            else
                if (*(cc + 1) == 'C')
                    loop = 4;
                else
                    loop = 5;

        cc += 2;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teamy[0] = *cc;
        teamy[1] = *(cc + 1);
        teamy[2] = *(cc + 2);
        teamy[3] = *(cc + 3);
        teamy[4] = '\0';
        teamid[0] = *(cc + 4);
        teamid[1] = *(cc + 5);
        teamid[2] = *(cc + 6);
        teamid[3] = '\0';
        teams[loop][nteams[loop]].year = atoi (&teamy[0]);
        teams[loop][nteams[loop]].id = atoi (&teamid[0]);
        if (!teams[loop][nteams[loop]].year)
            strcpy (&teams[loop][nteams[loop]].ucteamname[0], cc + 7);
        *work = ' ';
        cc = work + 1;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teams[loop][nteams[loop]].wins = atoi (cc);
        *work = ' ';
        cc = work + 1;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teams[loop][nteams[loop]].losses = atoi (cc);
        *work = ' ';
        cc = work + 1;

        teams[loop][nteams[loop]].pct = (float) teams[loop][nteams[loop]].wins / (float) (teams[loop][nteams[loop]].wins + teams[loop][nteams[loop]].losses);
        if (isnanf (teams[loop][nteams[loop]].pct))
            teams[loop][nteams[loop]].pct = 0.0;

        nteams[loop]++;
    }

    for (loop = 0; loop < 6; loop++) {
        if (!nteams[loop])
            continue;

        /* sort by primary pct, secondary losses */
        for (x = 0; x < (nteams[loop] - 1); x++)
            for (y = x + 1; y < nteams[loop]; y++)
                if (teams[loop][x].pct < teams[loop][y].pct) {
                    teamh = teams[loop][x];
                    teams[loop][x] = teams[loop][y];
                    teams[loop][y] = teamh;
                }
                else
                    if (teams[loop][x].pct == teams[loop][y].pct && teams[loop][x].losses > teams[loop][y].losses) {
                        teamh = teams[loop][x];
                        teams[loop][x] = teams[loop][y];
                        teams[loop][y] = teamh;
                    }
        /* figure games back */
        for (x = 0; x < nteams[loop]; x++)
            teams[loop][x].gb = teams[loop][x].wins - teams[loop][x].losses;
        for (y = x = 0; x < nteams[loop]; x++)
            if (teams[loop][x].gb > y)
                y = teams[loop][x].gb;
        for (x = 0; x < nteams[loop]; x++)
            teams[loop][x].gb = abs (teams[loop][x].gb - y);
    }

    /* check for a possible reversal of teams */
    for (work = index (&buffer[0], ':') + 2; work < (&buffer[0] + strlen (&buffer[0])); ) {
        if (*work == ':' || *work == '1' || *work == '2' || *work == '3' || *work == '6')
            break;
        work += 3;
        if (*work == '5' || *work == '7') {
            strncpy (&teamy[0], work + 1, 4);
            teamy[4] = '\0';
            ty[0] = atoi (&teamy[0]);
            if (ty[0]) {
                strncpy (&teamid[0], work + 5, 4);
                teamid[4] = '\0';
                tid[0] = atoi (&teamid[0]);
                work += 9;
            }
            else {
                int s1, s2;

                work2 = work;
                work = (char *) index (work, ' ');
                *work = '\0';
                strcpy (&tuctname[0], work2 + 5);
                *work = ' ';
                work++;
                for (s1 = 0; s1 < 6; s1++)
                    for (s2 = 0; s2 < 300; s2++)
                        if (!strcmp (&teams[s1][s2].ucteamname[0], &tuctname[0]))
                            tid[0] = teams[s1][s2].id;
            }

            strncpy (&teamy[0], work, 4);
            teamy[4] = '\0';
            ty[1] = atoi (&teamy[0]);
            if (ty[1]) {
                strncpy (&teamid[0], work + 4, 4);
                teamid[4] = '\0';
                tid[1] = atoi (&teamid[0]);
            }
            else {
                int s1, s2;

                work2 = work;
                work = (char *) index (work, ' ');
                *work = '\0';
                strcpy (&tuctname[0], work2 + 4);
                *work = ' ';
                for (s1 = 0; s1 < 6; s1++)
                    for (s2 = 0; s2 < 300; s2++)
                        if (!strcmp (&teams[s1][s2].ucteamname[0], &tuctname[0]))
                            tid[1] = teams[s1][s2].id;
            }

            for (loop = 0; loop < 6; loop++) {
                if (!nteams[loop])
                    continue;
                for (x = 0; x < (nteams[loop]); x++)
                    if (teams[loop][x].year == ty[1] && teams[loop][x].id == tid[1] && teams[loop][x + 1].year == ty[0] && teams[loop][x + 1].id == tid[0]) {
                        teamh = teams[loop][x];
                        teams[loop][x] = teams[loop][x + 1];
                        teams[loop][x + 1] = teamh;
                        break;
                    }
            }
        }
        for (work = index (work, ':'); *work == ':'; work++);
    }

    for (al1sw = x = 0; x < 3; x++)
        if (nteams[x])
            al1sw++;
    for (nl1sw = 0, x = 3; x < 6; x++)
        if (nteams[x])
            nl1sw++;

    for (stats[0] = '\0', loop = 0; loop < 6; loop++) {
        if (!nteams[loop])
            continue;
        strcat (&stats[0], "\n");
        switch (loop) {
            case 0:
                if (al1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              American League");
                if (al1sw > 1)
                    strcat (&stats[0], ", Eastern Division");
                break;
            case 1:
                if (al1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              American League");
                if (al1sw > 1)
                    strcat (&stats[0], ", Central Division");
                break;
            case 2:
                if (al1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              American League");
                if (al1sw > 1)
					strcat (&stats[0], ", Western Division");
                break;
            case 3:
                if (nl1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              National League");
                if (nl1sw > 1)
                    strcat (&stats[0], ", Eastern Division");
                break;
            case 4:
                if (nl1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              National League");
                if (nl1sw > 1)
                    strcat (&stats[0], ", Central Division");
                break;
            default:
                if (nl1sw == 1)
                    strcat (&stats[0], "         ");
                strcat (&stats[0], "              National League");
                if (nl1sw > 1)
                    strcat (&stats[0], ", Western Division");
        }

        strcat (&stats[0], "\n\n     Team                           W      L      PCT   Games Back\n\n");

        for (x = 0; teams[loop][x].id != 0; x++) {
            /* move Team Year and Name */
            if (teams[loop][x].id < 900) {
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == teams[loop][x].id) {
                        strcat (&stats[0], (char *) cnvt_int2str ((teams[loop][x].year), 'l'));
                        break;
                    }
            }
            else
                strcat (&stats[0], "    ");
            strcat (&stats[0], " ");
            if (teams[loop][x].id < 900) {
                strcat (&stats[0], &teaminfo[y].teamname[0]);
                strncat (&stats[0], "                             ", (29 - strlen (&teaminfo[y].teamname[0])));
            }
            else {
                strcat (&stats[0], &teams[loop][x].ucteamname[0]);
                strncat (&stats[0], "                             ", (29 - strlen (&teams[loop][x].ucteamname[0])));
            }
            if (teams[loop][x].wins < 100)
                strcat (&stats[0], " ");
            if (teams[loop][x].wins < 10)
                strcat (&stats[0], " ");
            /* move wins and losses */
            strcat (&stats[0], (char *) cnvt_int2str ((teams[loop][x].wins), 'l'));
            strcat (&stats[0], "   ");
            if (teams[loop][x].losses < 10)
                strcat (&stats[0], "  ");
            else
                strcat (&stats[0], " ");
            if (teams[loop][x].losses < 100)
                strcat (&stats[0], " ");
            strcat (&stats[0], (char *) cnvt_int2str ((teams[loop][x].losses), 'l'));
            strcat (&stats[0], "   ");
            /* move Pct */
            strcat (&stats[0], (char *) do_average (teams[loop][x].wins, (teams[loop][x].wins + teams[loop][x].losses)));
            if ((teams[loop][x].gb / 2) < 10 && (teams[loop][x].gb / 2))
                strcat (&stats[0], "       ");
            else
                strcat (&stats[0], "      ");
            /* move Games Back */
            if (teams[loop][x].gb / 2)
                strcat (&stats[0], (char *) cnvt_int2str ((teams[loop][x].gb / 2), 'l'));

            if (teams[loop][x].gb % 2) {
                if (teams[loop][x].gb / 2)
                    strcat (&stats[0], ".5");
                else
                    strcat (&stats[0], "  .5");
            }
            if (!(teams[loop][x].gb / 2) && !(teams[loop][x].gb % 2))
                strcat (&stats[0], " -");
            strcat (&stats[0], "\n");
        }
        strcat (&stats[0], "\n");
    }

    /* process extra info */
    cc = index (&buffer[0], ':');
    cc += 2;
    if (*cc == ':')
        /* nothing to do */
        return;

    strcat (&stats[0], "\n\n\n");

Check4Playoffs:
    ALNL = 1;
    if (*cc == '4' || *cc == '5') {
        /* playoff results */
        gint x, flip;
        gchar type;

        if (*cc == '4')
            ALNL = 0;
        else
            ALNL = 1;
        cc += 3;

        while (*cc != ':') {
            type = *cc;
            cc++;

            if (type == '1') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 2-team tie for the Wild Card in the NL.\nIn a 1-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 2-team tie for the Wild Card in the AL.\nIn a 1-game playoff:\n");
            }
            if (type == '2') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 3-team tie for the Wild Card in the NL.\nIn a 2-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 3-team tie for the Wild Card in the AL.\nIn a 2-game playoff:\n");
            }
            if (type == '3') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 4-team tie for the Wild Card in the NL.\nIn a 3-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 4-team tie for the Wild Card in the AL.\nIn a 3-game playoff:\n");
            }
            if (type == '4') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There were more than 4 teams tied for the Wild Card in the NL.\n");
                else
                    strcat (&stats[0], "There were more than 4 teams tied for the Wild Card in the AL.\n");
            }
            if (type == 'a') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 3-team tie for a Division in the NL.\nIn a 2-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 3-team tie for a Division in the AL.\nIn a 2-game playoff:\n");
            }
            if (type == 'b') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 4-team tie for a Division in the NL.\nIn a 3-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 4-team tie for a Division in the AL.\nIn a 3-game playoff:\n");
            }
            if (type == 'c') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There were more than 4 teams tied for a Division in the NL.\n");
                else
                    strcat (&stats[0], "There were more than 4 teams tied for a Division in the AL.\n");
            }
            if (type == 'd') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 2-team tie for the NL.\nIn a 1-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 2-team tie for the AL.\nIn a 1-game playoff:\n");
            }
            if (type == 'e') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 3-team tie for the NL.\nIn a 2-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 3-team tie for the AL.\nIn a 2-game playoff:\n");
            }
            if (type == 'f') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 4-team tie for the NL.\nIn a 3-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 4-team tie for the AL.\nIn a 3-game playoff:\n");
            }
            if (type == 'g') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There were more than 4 teams tied for the NL.\n");
                else
                    strcat (&stats[0], "There were more than 4 teams tied for the AL.\n");
            }
            if (type == 'h') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 2-team tie for second place in the NL.\nIn a 1-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 2-team tie for second place in the AL.\nIn a 1-game playoff:\n");
            }
            if (type == 'i') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 3-team tie for second place in the NL.\nIn a 2-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 3-team tie for second place in the AL.\nIn a 2-game playoff:\n");
            }
            if (type == 'j') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 4-team tie for second place in the NL.\nIn a 3-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 4-team tie for second place in the AL.\nIn a 3-game playoff:\n");
            }
            if (type == 'k') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There were more than 4 teams tied for second place in the NL.\n");
                else
                    strcat (&stats[0], "There were more than 4 teams tied for second place in the AL.\n");
            }
            if (type == '5' || type == '6' || type == '7' || type == '8') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 2-team tie for a Division in the NL.\n");
                else
                    strcat (&stats[0], "There was a 2-team tie for a Division in the AL.\n");
                if (type == '5' || type == '6') {
                    strcat (&stats[0], "The Division was decided by their record against one another\n");
                    strcat (&stats[0], "  since the loser is the Wild Card.\n");
                }
                else {
                    strcat (&stats[0], "The Division was decided by a coin flip since they had the same record\n");
                    strcat (&stats[0], "  against one another, and since the loser is the Wild Card.\n");
                }
            }
            if (type == '9') {
                if (ALNL || (!al1sw))
                    strcat (&stats[0], "There was a 2-team tie for a Division in the NL.\nIn a 1-game playoff:\n");
                else
                    strcat (&stats[0], "There was a 2-team tie for a Division in the AL.\nIn a 1-game playoff:\n");
            }
            for (flip = 0; *cc != ':'; /* cc is incremented within the loop */ ) {
                if (flip && (type == '4' || type == 'c' || type == 'g' || type == 'k')) {
                    cc++;
                    continue;
                }
                else
                    if (*cc != '0') {
                        strncat (&stats[0], cc, 4);
                        strcat (&stats[0], " ");
                        strncpy (&teamid[0], cc + 4, 4);
                        teamid[4] = '\0';
                        x = atoi (&teamid[0]);
                        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                            if (teaminfo[y].id == x) {
                                strcat (&stats[0], &teaminfo[y].teamname[0]);
                                break;
                            }
                        cc += 8;
                    }
                    else {
                        cc += 4;                 /* point to name of user-created team */
                        work = index (cc, ' ');
                        *work = '\0';
                        strcat (&stats[0], cc);
                        *work = ' ';
                        cc = work + 1;
                    }
                if (!flip) {
                    if (type == '0' || type == '1' || type == '2' || type == '3' || type == '9' || type == 'b' || type == 'd' ||
                                          type == 'a' || type == 'e' || type == 'f' || type == 'h' || type == 'i' || type == 'j')
                        strcat (&stats[0], " beat ");
                    if (type == '7' || type == '8')
                        strcat (&stats[0], " in a coin flip won out against ");
                    if (type == '4' || type == 'c' || type == 'g' || type == 'k')
                        strcat (&stats[0], " won out in a coin flip.\n");
                    if (type == '5' || type == '6')
                        strcat (&stats[0], " won out against ");
                    flip = 1;
                }
                else {
                    strcat (&stats[0], "\n");
                    flip = 0;
                }
            }
            cc += 2;
            strcat (&stats[0], "\n");
        }
        strcat (&stats[0], "\n");
        cc += 2;
    }

    if (!ALNL)
        goto Check4Playoffs;

    if (*cc == '1') {
        int xx;
        char wcyr[4][5], wcid[4][5];

        strcat (&stats[0], "The regular season is completed.\n");
        strcat (&stats[0], "The first round of the post-season will be:\n\n");
        for (xx = 0, cc++; cc < (&buffer[0] + strlen (&buffer[0])); xx += 2) {  /* cc is incremented within loop */
            if (*cc == ' ')
                if (xx == 4 || xx == 8) {
                    strncat (&stats[0], &wcyr[(xx - 4) / 2][0], 4);
                    strncpy (&teamid[0], &wcid[(xx - 4) / 2][0], 4);
                    teamid[4] = '\0';
                    x = atoi (&teamid[0]);
                    for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                        if (teaminfo[y].id == x) {
                            strcat (&stats[0], &teaminfo[y].teamabbrev[0]);
                            break;
                        }
                    strcat (&stats[0], " or ");

                    strncat (&stats[0], &wcyr[(xx - 4) / 2 + 1][0], 4);
                    strncpy (&teamid[0], &wcid[(xx - 4) / 2 + 1][0], 4);
                    teamid[4] = '\0';
                    x = atoi (&teamid[0]);
                    for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                        if (teaminfo[y].id == x) {
                            strcat (&stats[0], &teaminfo[y].teamabbrev[0]);
                            break;
                        }

                    cc += 8;
                    if (!x)
                        cc++;
                    goto standings_vs;
                }
            if (*cc != '0') {
                strncat (&stats[0], cc, 4);
                strcat (&stats[0], " ");
                strncpy (&teamid[0], cc + 4, 4);
                teamid[4] = '\0';
                x = atoi (&teamid[0]);
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == x) {
                        strcat (&stats[0], &teaminfo[y].teamname[0]);
                        break;
                    }
                /* save team yr & id for possible processing later */
                if (!xx || xx == 2) {
                    strncpy (&wcyr[xx][0], cc, 4);
                    strncpy (&wcid[xx][0], cc + 4, 4);
                    wcyr[xx][4] = wcid[xx][4] = '\0';
                }
                cc += 8;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                work = index (cc, ' ');
                *work = '\0';
                strcat (&stats[0], cc);
                *work = ' ';
                cc = work + 1;
            }
standings_vs:
            strcat (&stats[0], " vs ");
            if (*cc != '0') {
                strncat (&stats[0], cc, 4);
                strcat (&stats[0], " ");
                strncpy (&teamid[0], cc + 4, 4);
                teamid[4] = '\0';
                x = atoi (&teamid[0]);
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == x) {
                        strcat (&stats[0], &teaminfo[y].teamname[0]);
                        break;
                    }
                /* save team yr & id for possible processing later */
                if (!xx || xx == 2) {
                    strncpy (&wcyr[xx + 1][0], cc, 4);
                    strncpy (&wcid[xx + 1][0], cc + 4, 4);
                    wcyr[xx + 1][4] = wcid[xx + 1][4] = '\0';
                }
                cc += 8;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                work = index (cc, ' ');
                *work = '\0';
                strcat (&stats[0], cc);
                *work = ' ';
                cc = work + 1;
            }
            strcat (&stats[0], "\n\n");
        }
    }
    overpssw = 0;
    if (*cc == '2')
        strcat (&stats[0], "The post-season results so far:\n\n");
    if (*cc == '3') {
        strcat (&stats[0], "The post-season is completed.\n");
        strcat (&stats[0], "The post-season results:\n\n");
        overpssw = 1;
    }
    if (*cc == '2' || *cc == '3') {
        char work[5], *w1, uctn[2][50];
        int id[2], yr[2], w[2], i, oversw = 0;

        for (cc++; cc < (&buffer[0] + strlen (&buffer[0])); /* cc is incremented within loop */) {
            if (*cc == ':') {
                strcat (&stats[0], "\n\n");
                cc++;
                continue;
            }
            if (*cc == 'X')
                break;
            strncpy (&work[0], cc, 4);
            work[4] = '\0';
            yr[0] = atoi (&work[0]);
            strncpy (&work[0], cc + 4, 4);
            work[4] = '\0';
            id[0] = atoi (&work[0]);
            if (id[0] < 900) {
                strncpy (&work[0], cc + 8, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[0] = atoi (&work[0]);
                cc += 9;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                w1 = index (cc, ' ');
                *w1 = '\0';
                strcpy (&uctn[0][0], cc);
                *w1 = ' ';
                cc = w1 + 1;
                strncpy (&work[0], cc, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[0] = atoi (&work[0]);
                cc++;
            }

            strncpy (&work[0], cc, 4);
            work[4] = '\0';
            yr[1] = atoi (&work[0]);
            strncpy (&work[0], cc + 4, 4);
            work[4] = '\0';
            id[1] = atoi (&work[0]);
            if (id[1] < 900) {
                strncpy (&work[0], cc + 8, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[1] = atoi (&work[0]);
                cc += 9;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                w1 = index (cc, ' ');
                *w1 = '\0';
                strcpy (&uctn[1][0], cc);
                *w1 = ' ';
                cc = w1 + 1;
                strncpy (&work[0], cc, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[1] = atoi (&work[0]);
                cc++;
            }

            if (w[0] >= w[1])
                i = 0;
            else
                i = 1;

            if (id[i] < 900) {
                strcat (&stats[0], (char *) cnvt_int2str (yr[i], 'l'));
                strcat (&stats[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcat (&stats[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], &uctn[i][0]);

            if (oversw) {
                strcat (&stats[0], " beat ");
                oversw = 0;
            }
            else
                if (w[0] == w[1])
                    strcat (&stats[0], " and ");
                else
                    strcat (&stats[0], " are leading ");

            if (i)
                i = 0;
            else
                i = 1;

            if (id[i] < 900) {
                strcat (&stats[0], (char *) cnvt_int2str (yr[i], 'l'));
                strcat (&stats[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcat (&stats[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], &uctn[i][0]);
            strcat (&stats[0], " ");

            if (i)
                i = 0;
            else
                i = 1;

            if (w[0] == w[1])
                strcat (&stats[0], "are tied at ");
            strcat (&stats[0], (char *) cnvt_int2str (w[i], 'l'));
            if (w[0] == w[1])
                if (w[0] == 1)
                    strcat (&stats[0], " game apiece");
                else
                    strcat (&stats[0], " games apiece");
            else
                if (w[i] == 1)
                    strcat (&stats[0], " game to ");
                else
                    strcat (&stats[0], " games to ");

            if (i)
                i = 0;
            else
                i = 1;

            if (w[0] != w[1])
                strcat (&stats[0], (char *) cnvt_int2str (w[i], 'l'));

            strcat (&stats[0], "\n\n");
        }

        if (*cc == 'X' && overpssw) {
            if (w[0] > w[1])
                i = 0;
            else
                i = 1;

            strcat (&stats[0], "\n\n");

            if (id[i] < 900) {
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcat (&stats[0], (char *) cnvt_int2str (yr[i], 'l'));
                        strcat (&stats[0], " ");
                        strcat (&stats[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], &uctn[i][0]);
            strcat (&stats[0], " WORLD CHAMPS!!\n");
        }
    }
    if (*cc == '6') {
        strcat (&stats[0], "End of the regular season\n\n");
        strcat (&stats[0], "No post-season\n");
    }
}

/* show Real Life season results */
void
RealLifeSeasonResults (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S62\n");  /* tell the server to send us the Real Life years available */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
            strcpy (&work[0], "Encountered error when talking to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &Error[0];
        }

        outMessage (msg);
        return;
    }

    if (!strncmp (&buffer[0], "-2", 2)) {
        GotError ();
        return;
    }

    if (strlen (&buffer[0]) < 3) {
        /* this happens if there are no stats */
        gchar NoStats[256] = "The Real Life season results do not exist.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);

        return;
    }

    /* get a year from the user and send data to server */
    if (!getayrRLRESactive)
        /* only need to call this function if the "get a year" window isn't already active */
        GetAYear4RLResults ();
}

void
FillRLSeasonResults () {
    gchar *cc;
    gint y;

    cc = &buffer[0];
    for (y = 0; cc < (&buffer[0] + strlen (&buffer[0])); cc++) {
        stats[y] = *cc;
        if (stats[y] == '*')  /* the server sends an * in place of a \n */
            stats[y] = '\n';
        y++;
    }
    stats[y] = '\0';

    strcat (&stats[0], "\n");
}

void
PrintStandings (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Standings ...", *msg[5];
    gint x;

    sock_puts (sock, "S1\n");  /* we need to get the NSB season standings again */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }
    FillStandings ();

    print (&stats[0]);

    strcpy (&work[0], "Print season standings.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintStats (GtkWidget *widget, gpointer cnt) {
    gchar work1[100], Printing[256] = "Printing Stats ...", *msg[5];
    gint icnt = GPOINTER_TO_INT (cnt), x;

    if (icnt > 255) {
        gchar NoPrint[256] = "Cannot print stats.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print stats.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&work1[0], &prtbutrlrescmd[icnt][0]);
    sock_puts (sock, work1);  /* tell the server to send us the Real Life results for a specific year again */
    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */

    FillRLSeasonResults ();

    print (&stats[0]);

    strcpy (&work[0], "Print Real Life season results.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

char teamtotinfo[900000];

void
PrintTeamTotals (GtkTextBuffer *buf, gpointer cnt) {
    /* the "team totals" stats need to be reformatted for printing */
    gchar work1[100], Printing[256] = "Printing Stats ...", *msg[5], tname[100];
    gint x, y, z, pib, totg, singles, tsingles, w, l, icnt = GPOINTER_TO_INT (cnt);
    float fnum1, fnum2;

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print stats.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print stats.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&work1[0], &prtbutttcmd[icnt][0]);
    urind = whichur[icnt];
    psorrs = whichpsorrs[icnt];
    strncpy (&teamyr[0], &work1[4], 4);         /* save year */
    teamyr[4] = '\0';

    sock_puts (sock, work1);  /* tell the server to send us the specific team totals again */

    GetTeamTotals ();

    strcpy (&teamtotinfo[0], "                     ");
    if (urind == 'U') {
        if (psorrs == '4') {
            strcat (&teamtotinfo[0], "User-Created Teams Statistics ");
            strcat (&teamtotinfo[0], "\n(NOTE - Some stats are not available for all years which may make some\n        of the totals look strange.) ");
        }
        else
            if (psorrs == '3')
                strcat (&teamtotinfo[0], "NSB Series Statistics ");
            else {
                strcat (&teamtotinfo[0], "NSB Season Statistics ");
                if (psorrs == '2')
                    strcat (&teamtotinfo[0], "(Post-Season)");
                else
                    strcat (&teamtotinfo[0], "(Regular Season)");
            }
    }
    else {
        strcat (&teamtotinfo[0], "Real Life Statistics (Regular Season)\n\n");
        strcat (&teamtotinfo[0], "(NOTE - These stats do not include players who may have played in real life\n");
        strcat (&teamtotinfo[0], "        but are not included in these NSB rosters.  Each team's combined\n");
        strcat (&teamtotinfo[0], "        wins and losses in the pitching stats may not equal the number of\n");
        strcat (&teamtotinfo[0], "        games the team played during the regular season.)");

        /* need RL results to get total team games */
        strcpy (&work[0], "S3");
        strcat (&work[0], &teamyr[0]);
        strcat (&work[0], "\n");
        sock_puts (sock, work);  /* tell the server to send real life results for a specific year */

        sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */
    }
    strcat (&teamtotinfo[0], "\n\n");

    strcat (&teamtotinfo[0], "Team Year & Name             G     AB      R      H     2B     3B     HR    RBI\n\n");

    /* sort team data by batting average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = (float) teamdata[x].batters.hitting.hits / (float) teamdata[x].batters.hitting.atbats;
            fnum2 = (float) teamdata[y].batters.hitting.hits / (float) teamdata[y].batters.hitting.atbats;

            if (fnum1 < fnum2) {
                teamdata[300] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[300];
            }
        }

    teamdata[301].batters.hitting.games = teamdata[301].batters.hitting.atbats = teamdata[301].batters.hitting.runs =
      teamdata[301].batters.hitting.hits = teamdata[301].batters.hitting.doubles = teamdata[301].batters.hitting.triples =
      teamdata[301].batters.hitting.homers = teamdata[301].batters.hitting.rbi = teamdata[301].batters.hitting.bb = 
      teamdata[301].batters.hitting.so = teamdata[301].batters.hitting.hbp = teamdata[301].batters.hitting.gidp =
      teamdata[301].batters.hitting.sb = teamdata[301].batters.hitting.cs = teamdata[301].batters.hitting.ibb =
      teamdata[301].batters.hitting.sh = teamdata[301].batters.hitting.sf = tsingles = 0;

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        if (urind == 'U') {
            strcat (&teamtotinfo[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
            teamdata[301].batters.hitting.games += teamdata[x].pitchers.pitching.wins;
        }
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        w = atoi (&work[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        l = atoi (&work[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&teamtotinfo[0], "    ");
                        strcat (&teamtotinfo[0], &work[0]);
                        break;
                    }
            }
            else
                strcat (&teamtotinfo[0], "       ");

        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.atbats, 'r'));
        teamdata[301].batters.hitting.atbats += teamdata[x].batters.hitting.atbats;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.runs, 'r'));
        teamdata[301].batters.hitting.runs += teamdata[x].batters.hitting.runs;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.hits, 'r'));
        teamdata[301].batters.hitting.hits += teamdata[x].batters.hitting.hits;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.doubles, 'r'));
        teamdata[301].batters.hitting.doubles += teamdata[x].batters.hitting.doubles;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.triples, 'r'));
        teamdata[301].batters.hitting.triples += teamdata[x].batters.hitting.triples;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.homers, 'r'));
        teamdata[301].batters.hitting.homers += teamdata[x].batters.hitting.homers;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.rbi, 'r'));
        teamdata[301].batters.hitting.rbi += teamdata[x].batters.hitting.rbi;

        strcat (&teamtotinfo[0], "\n");
    }

    strcat (&teamtotinfo[0], "                 TOTALS");
    if (urind == 'U')
        strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work[0])) {
                case 0:
                    strcat (&teamtotinfo[0], "       ");
                    break;
                case 1:
                    strcat (&teamtotinfo[0], "      ");
                    break;
                case 2:
                    strcat (&teamtotinfo[0], "     ");
                    break;
                case 3:
                    strcat (&teamtotinfo[0], "    ");
                    break;
                case 4:
                    strcat (&teamtotinfo[0], "   ");
                    break;
                case 5:
                    strcat (&teamtotinfo[0], "  ");
                    break;
                case 6:
                    strcat (&teamtotinfo[0], " ");
                    break;
            }

            strcat (&teamtotinfo[0], &work[0]);
        }
        else
            strcat (&teamtotinfo[0], "       ");

    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.atbats, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.runs, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.hits, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.doubles, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.triples, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.homers, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.rbi, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name             BA    SA   OBA     BB     SO    HBP   GIDP\n\n");

    for (x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 20);
            if (strlen (&teaminfo[y].teamname[0]) < 20)
                strncat (&teamtotinfo[0], "                       ", 20 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 20);
            if (strlen (&uctname[x][0]) < 20)
                strncat (&teamtotinfo[0], "                       ", 20 - strlen (&uctname[x][0]));
        }

        singles = teamdata[x].batters.hitting.hits - (teamdata[x].batters.hitting.homers +
                  teamdata[x].batters.hitting.triples + teamdata[x].batters.hitting.doubles);
        tsingles += singles;

        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].batters.hitting.hits, teamdata[x].batters.hitting.atbats));
        strcat (&teamtotinfo[0], (char *) do_average (((teamdata[x].batters.hitting.homers * 4) +
                                                 (teamdata[x].batters.hitting.triples * 3) +
                                                 (teamdata[x].batters.hitting.doubles * 2) + singles),
                                                  teamdata[x].batters.hitting.atbats));
        if (teamdata[x].batters.hitting.sf == -1)
            y = 0;
        else
            y = teamdata[x].batters.hitting.sf;
        y += teamdata[x].batters.hitting.sh;
        strcat (&teamtotinfo[0], (char *) do_average ((teamdata[x].batters.hitting.hits + teamdata[x].batters.hitting.bb + teamdata[x].batters.hitting.hbp),
                                              (teamdata[x].batters.hitting.atbats + teamdata[x].batters.hitting.bb + y + teamdata[x].batters.hitting.hbp)));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.bb, 'r'));
        teamdata[301].batters.hitting.bb += teamdata[x].batters.hitting.bb;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.so, 'r'));
        teamdata[301].batters.hitting.so += teamdata[x].batters.hitting.so;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.hbp, 'r'));
        teamdata[301].batters.hitting.hbp += teamdata[x].batters.hitting.hbp;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.gidp, 'r'));
        teamdata[301].batters.hitting.gidp += teamdata[x].batters.hitting.gidp;
        if (teamdata[301].batters.hitting.sf != -1)
            teamdata[301].batters.hitting.sf += teamdata[x].batters.hitting.sf;
        teamdata[301].batters.hitting.sh += teamdata[x].batters.hitting.sh;

        strcat (&teamtotinfo[0], "\n");
    }

    strcat (&teamtotinfo[0], "                 TOTALS  ");
    strcat (&teamtotinfo[0], (char *) do_average (teamdata[301].batters.hitting.hits, teamdata[301].batters.hitting.atbats));
    strcat (&teamtotinfo[0], (char *) do_average (((teamdata[301].batters.hitting.homers * 4) +
                                             (teamdata[301].batters.hitting.triples * 3) +
                                             (teamdata[301].batters.hitting.doubles * 2) + tsingles),
                                              teamdata[301].batters.hitting.atbats));
    strcat (&teamtotinfo[0], (char *) do_average ((teamdata[301].batters.hitting.hits + teamdata[301].batters.hitting.bb +
                                             teamdata[301].batters.hitting.hbp),
                                             (teamdata[301].batters.hitting.atbats + teamdata[301].batters.hitting.bb + teamdata[301].batters.hitting.sh +
                                             teamdata[301].batters.hitting.sf + teamdata[301].batters.hitting.hbp)));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.bb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.so, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.hbp, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.gidp, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name                   SB     CS    IBB     SH     SF\n\n");

    for (x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 25);
            if (strlen (&teaminfo[y].teamname[0]) < 25)
                strncat (&teamtotinfo[0], "                       ", 25 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 25)
                strncat (&teamtotinfo[0], "                       ", 25 - strlen (&uctname[x][0]));
        }

        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sb, 'r'));
        teamdata[301].batters.hitting.sb += teamdata[x].batters.hitting.sb;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.cs, 'r'));
        teamdata[301].batters.hitting.cs += teamdata[x].batters.hitting.cs;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.ibb, 'r'));
        teamdata[301].batters.hitting.ibb += teamdata[x].batters.hitting.ibb;
        /* sf & sh are cummed above since they're needed to compute OBA above */
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sh, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sf, 'r'));
        strcat (&teamtotinfo[0], "\n");
    }

    strcat (&teamtotinfo[0], "                 TOTALS");
    strcat (&teamtotinfo[0], "       ");
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.sb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.cs, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.ibb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.sh, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.sf, 'r'));

    strcat (&teamtotinfo[0], "\n\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name             G     IP        H      R     ER      W      L   PCT\n\n");

    /* sort team data by earned run average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = (float) (teamdata[x].pitchers.pitching.er * 9.0) / ((float) teamdata[x].pitchers.pitching.innings +
                                                   (float) teamdata[x].pitchers.pitching.thirds / 3.0);
            fnum2 = (float) (teamdata[y].pitchers.pitching.er * 9.0) / ((float) teamdata[y].pitchers.pitching.innings +
                                                   (float) teamdata[y].pitchers.pitching.thirds / 3.0);

            if (fnum1 > fnum2) {
                teamdata[300] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[300];
            }
        }

    teamdata[301].pitchers.pitching.innings = teamdata[301].pitchers.pitching.thirds = teamdata[301].pitchers.pitching.hits =
      teamdata[301].pitchers.pitching.runs = teamdata[301].pitchers.pitching.er = teamdata[301].pitchers.pitching.wins =
      teamdata[301].pitchers.pitching.losses = teamdata[301].pitchers.pitching.so = teamdata[301].pitchers.pitching.walks =
      teamdata[301].pitchers.pitching.games_started = teamdata[301].pitchers.pitching.cg = teamdata[301].pitchers.pitching.gf =
      teamdata[301].pitchers.pitching.sho = teamdata[301].pitchers.pitching.saves = teamdata[301].pitchers.pitching.svopp =
      teamdata[301].pitchers.pitching.ibb = teamdata[301].pitchers.pitching.wp = teamdata[301].pitchers.pitching.hb =
      teamdata[301].pitchers.pitching.balks = teamdata[301].pitchers.pitching.bfp = teamdata[301].pitchers.pitching.doubles =
      teamdata[301].pitchers.pitching.triples = teamdata[301].pitchers.pitching.homers = teamdata[301].pitchers.pitching.rbi =
      teamdata[301].pitchers.pitching.sb = teamdata[301].pitchers.pitching.cs = teamdata[301].pitchers.pitching.sh =
      teamdata[301].pitchers.pitching.sf = teamdata[301].pitchers.pitching.opp_ab = 0;

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        z = (int) (teamdata[x].pitchers.pitching.thirds / 3);
        teamdata[x].pitchers.pitching.innings += z;
        teamdata[x].pitchers.pitching.thirds %= 3;
        if (urind == 'U')
            strcat (&teamtotinfo[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        w = atoi (&work[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        l = atoi (&work[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&teamtotinfo[0], "    ");
                        strcat (&teamtotinfo[0], &work[0]);
                        break;
                    }
            }
            else
                strcat (&teamtotinfo[0], "       ");

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.innings, 'r'));
        if (teamdata[x].pitchers.pitching.thirds == 1 || teamdata[x].pitchers.pitching.thirds == 2) {
            strcat (&teamtotinfo[0], ".");
            strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.thirds, 'l'));
        }
        else
            strcat (&teamtotinfo[0], "  ");
        teamdata[301].pitchers.pitching.innings += teamdata[x].pitchers.pitching.innings;
        teamdata[301].pitchers.pitching.thirds += teamdata[x].pitchers.pitching.thirds;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.hits, 'r'));
        teamdata[301].pitchers.pitching.hits += teamdata[x].pitchers.pitching.hits;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.runs, 'r'));
        teamdata[301].pitchers.pitching.runs += teamdata[x].pitchers.pitching.runs;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.er, 'r'));
        teamdata[301].pitchers.pitching.er += teamdata[x].pitchers.pitching.er;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.wins, 'r'));
        teamdata[301].pitchers.pitching.wins += teamdata[x].pitchers.pitching.wins;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.losses, 'r'));
        teamdata[301].pitchers.pitching.losses += teamdata[x].pitchers.pitching.losses;
        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].pitchers.pitching.wins, (teamdata[x].pitchers.pitching.wins +
                                           teamdata[x].pitchers.pitching.losses)));
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "                 TOTALS");
    if (urind == 'U')
        strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work[0])) {
                case 0:
                    strcat (&teamtotinfo[0], "       ");
                    break;
                case 1:
                    strcat (&teamtotinfo[0], "      ");
                    break;
                case 2:
                    strcat (&teamtotinfo[0], "     ");
                    break;
                case 3:
                    strcat (&teamtotinfo[0], "    ");
                    break;
                case 4:
                    strcat (&teamtotinfo[0], "   ");
                    break;
                case 5:
                    strcat (&teamtotinfo[0], "  ");
                    break;
                case 6:
                    strcat (&teamtotinfo[0], " ");
                    break;
            }

            strcat (&teamtotinfo[0], &work[0]);
        }
        else
            strcat (&teamtotinfo[0], "       ");

    y = (int) (teamdata[301].pitchers.pitching.thirds / 3);
    teamdata[301].pitchers.pitching.innings += y;
    teamdata[301].pitchers.pitching.thirds %= 3;

    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.innings, 'r'));
    if (teamdata[301].pitchers.pitching.thirds == 1 || teamdata[301].pitchers.pitching.thirds == 2) {
        strcat (&teamtotinfo[0], ".");
        strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.thirds, 'l'));
    }
    else
        strcat (&teamtotinfo[0], "  ");
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.hits, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.runs, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.er, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.wins, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.losses, 'r'));
    strcat (&teamtotinfo[0], (char *) do_average (teamdata[301].pitchers.pitching.wins, (teamdata[301].pitchers.pitching.wins +
                                            teamdata[301].pitchers.pitching.losses)));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name            SO     BB   ERA     GS     CG     SHO     S    SOPP\n\n");

    for (x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.so, 'r'));
        teamdata[301].pitchers.pitching.so += teamdata[x].pitchers.pitching.so;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.walks, 'r'));
        teamdata[301].pitchers.pitching.walks += teamdata[x].pitchers.pitching.walks;
        strcat (&teamtotinfo[0], (char *) do_era (teamdata[x].pitchers.pitching.er * 9, teamdata[x].pitchers.pitching.innings,
                                                   teamdata[x].pitchers.pitching.thirds));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.games_started, 'r'));
        teamdata[301].pitchers.pitching.games_started += teamdata[x].pitchers.pitching.games_started;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.cg, 'r'));
        teamdata[301].pitchers.pitching.cg += teamdata[x].pitchers.pitching.cg;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sho, 'r'));
        teamdata[301].pitchers.pitching.sho += teamdata[x].pitchers.pitching.sho;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.saves, 'r'));
        teamdata[301].pitchers.pitching.saves += teamdata[x].pitchers.pitching.saves;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.svopp, 'r'));
        teamdata[301].pitchers.pitching.svopp += teamdata[x].pitchers.pitching.svopp;
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "               TOTALS  ");

    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.so, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.walks, 'r'));
    strcat (&teamtotinfo[0], (char *) do_era (teamdata[301].pitchers.pitching.er * 9, teamdata[301].pitchers.pitching.innings,
                                               teamdata[301].pitchers.pitching.thirds));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.games_started, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.cg, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.sho, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.saves, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.svopp, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name            GF    IBB     WP     HB      B    BFP     SH     SF\n\n");

    for (x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.gf, 'r'));
        teamdata[301].pitchers.pitching.gf += teamdata[x].pitchers.pitching.gf;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.ibb, 'r'));
        teamdata[301].pitchers.pitching.ibb += teamdata[x].pitchers.pitching.ibb;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.wp, 'r'));
        teamdata[301].pitchers.pitching.wp += teamdata[x].pitchers.pitching.wp;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.hb, 'r'));
        teamdata[301].pitchers.pitching.hb += teamdata[x].pitchers.pitching.hb;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.balks, 'r'));
        teamdata[301].pitchers.pitching.balks += teamdata[x].pitchers.pitching.balks;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.bfp, 'r'));
        teamdata[301].pitchers.pitching.bfp += teamdata[x].pitchers.pitching.bfp;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sh, 'r'));
        teamdata[301].pitchers.pitching.sh += teamdata[x].pitchers.pitching.sh;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sf, 'r'));
        teamdata[301].pitchers.pitching.sf += teamdata[x].pitchers.pitching.sf;
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "               TOTALS  ");

    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.gf, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.ibb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.wp, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.hb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.balks, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.bfp, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.sh, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.sf, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name            2B     3B     HR    RBI     SB     CS   OPAB  OPBA\n\n");

    for (x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.doubles, 'r'));
        teamdata[301].pitchers.pitching.doubles += teamdata[x].pitchers.pitching.doubles;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.triples, 'r'));
        teamdata[301].pitchers.pitching.triples += teamdata[x].pitchers.pitching.triples;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.homers, 'r'));
        teamdata[301].pitchers.pitching.homers += teamdata[x].pitchers.pitching.homers;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.rbi, 'r'));
        teamdata[301].pitchers.pitching.rbi += teamdata[x].pitchers.pitching.rbi;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sb, 'r'));
        teamdata[301].pitchers.pitching.sb += teamdata[x].pitchers.pitching.sb;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.cs, 'r'));
        teamdata[301].pitchers.pitching.cs += teamdata[x].pitchers.pitching.cs;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.opp_ab, 'r'));
        teamdata[301].pitchers.pitching.opp_ab += teamdata[x].pitchers.pitching.opp_ab;
        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].pitchers.pitching.hits, teamdata[x].pitchers.pitching.opp_ab));
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "               TOTALS  ");

    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.doubles, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.triples, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.homers, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.rbi, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.sb, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.cs, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].pitchers.pitching.opp_ab, 'r'));
    strcat (&teamtotinfo[0], (char *) do_average (teamdata[301].pitchers.pitching.hits, teamdata[301].pitchers.pitching.opp_ab));

    strcat (&teamtotinfo[0], "\n\n\n");
    strcat (&teamtotinfo[0], "Team Year & Name                     G     PO      A      E     PB   FA\n\n");

    teamdata[301].batters.fielding[0].po = teamdata[301].batters.fielding[0].a = teamdata[301].batters.fielding[0].e =
      teamdata[301].batters.fielding[0].pb = 0;

    /* sort team data by fielding average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = ((float) teamdata[x].batters.fielding[0].po + (float) teamdata[x].batters.fielding[0].a) /
                    ((float) teamdata[x].batters.fielding[0].po + (float) teamdata[x].batters.fielding[0].a +
                                                                  (float) teamdata[x].batters.fielding[0].e);
            fnum2 = ((float) teamdata[y].batters.fielding[0].po + (float) teamdata[y].batters.fielding[0].a) /
                    ((float) teamdata[y].batters.fielding[0].po + (float) teamdata[y].batters.fielding[0].a +
                                                                  (float) teamdata[y].batters.fielding[0].e);

            if (fnum1 < fnum2) {
                teamdata[100] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[100];
            }
        }

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&teamtotinfo[0], " ");
        }
        else
            strcat (&teamtotinfo[0], "     ");
        if (teamdata[x].year) {
            strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&teamtotinfo[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&teamtotinfo[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        strcat (&teamtotinfo[0], "        ");
        if (urind == 'U')
            strcat (&teamtotinfo[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        w = atoi (&work[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work[z] = buffer[pib];
                        work[z] = '\0';
                        l = atoi (&work[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&teamtotinfo[0], "    ");
                        strcat (&teamtotinfo[0], &work[0]);
                        break;
                    }
            }
            else
                strcat (&teamtotinfo[0], "       ");

        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[0].po, 'r'));
        teamdata[301].batters.fielding[0].po += teamdata[x].batters.fielding[0].po;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[0].a, 'r'));
        teamdata[301].batters.fielding[0].a += teamdata[x].batters.fielding[0].a;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[0].e, 'r'));
        teamdata[301].batters.fielding[0].e += teamdata[x].batters.fielding[0].e;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[0].pb, 'r'));
        teamdata[301].batters.fielding[0].pb += teamdata[x].batters.fielding[0].pb;
        strcat (&teamtotinfo[0], (char *) do_average ((teamdata[x].batters.fielding[0].po + teamdata[x].batters.fielding[0].a),
            (teamdata[x].batters.fielding[0].po + teamdata[x].batters.fielding[0].a + teamdata[x].batters.fielding[0].e)));
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "                 TOTALS        ");
    if (urind == 'U')
        strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work[0])) {
                case 0:
                    strcat (&teamtotinfo[0], "       ");
                    break;
                case 1:
                    strcat (&teamtotinfo[0], "      ");
                    break;
                case 2:
                    strcat (&teamtotinfo[0], "     ");
                    break;
                case 3:
                    strcat (&teamtotinfo[0], "    ");
                    break;
                case 4:
                    strcat (&teamtotinfo[0], "   ");
                    break;
                case 5:
                    strcat (&teamtotinfo[0], "  ");
                    break;
                case 6:
                    strcat (&teamtotinfo[0], " ");
                    break;
            }

            strcat (&teamtotinfo[0], &work[0]);
        }
        else
            strcat (&teamtotinfo[0], "       ");

    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.fielding[0].po, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.fielding[0].a, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.fielding[0].e, 'r'));
    strcat (&teamtotinfo[0], check_stats (teamdata[301].batters.fielding[0].pb, 'r'));
    strcat (&teamtotinfo[0], (char *) do_average ((teamdata[301].batters.fielding[0].po + teamdata[301].batters.fielding[0].a),
        (teamdata[301].batters.fielding[0].po + teamdata[301].batters.fielding[0].a + teamdata[301].batters.fielding[0].e)));

    strcat (&teamtotinfo[0], "\n");

    print (&teamtotinfo[0]);

    strcpy (&work[0], "Print Team Totals.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

char byteaminfo[20000];

void
PrintTeamStats (GtkWidget *widget, gpointer cnt) {
    /* the "by team" stats need to be reformatted for printing */
    gint x, y, z, newpl, totg, pib, w, l, singles = 0, ts, tab, tr, th, t2b, t3b, thr, tbi, tbb, tso, thbp, tgidp, tsb, tcs, tibb, tsf, tsh,
         tinn, tthirds, ter, tw, tl, tgs, tcg, tgf, tsho, tsopp, twp, thb, tb, tbfp, tpo, ta, te, tpb, topp_ab,
         maxplayers, maxpitchers, tmid, icnt = GPOINTER_TO_INT (cnt);
    gchar work1[100], work2[100], tname[100], Printing[256] = "Printing Stats ...", *msg[5];

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print stats.  Too many windows have been opened.", *msg[5];

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print stats.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&work1[0], &prtbuttmcmd[icnt][0]);
    urind = whichur[icnt];
    psorrs = whichpsorrs[icnt];
    if (urind == 'C')
        strcpy (&usercreatedtname[0], &prtuctm[icnt][0]);

    sock_puts (sock, work1);  /* tell the server to send us the specific team stats again */
    if (get_stats (sock, 't', 0) == -1)
        return;

    totg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    for (maxplayers = 0; maxplayers < 25; maxplayers++)
        if (team.batters[maxplayers].id.name[0] == ' ' || !strlen (&team.batters[maxplayers].id.name[0]))
            break;
    for (maxpitchers = 0; maxpitchers < 25; maxpitchers++)
        if (team.pitchers[maxpitchers].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers].id.name[0]))
            break;

    for (tmid = 0; tmid <= NUMBER_OF_TEAMS; tmid++)
        if (teaminfo[tmid].id == team.id)
            break;

    strcpy (&byteaminfo[0], "     ");
    if (urind != 'C' && team.year) {
        strcat (&byteaminfo[0], (char *) cnvt_int2str ((team.year), 'l'));
        strcat (&byteaminfo[0], " ");
    }
    if (team.year)
        strcat (&byteaminfo[0], &teaminfo[tmid].teamname[0]);
    else
        strcat (&byteaminfo[0], &usercreatedtname[0]);
    strcat (&byteaminfo[0], " - ");
    if (urind == 'S')
        strcat (&byteaminfo[0], "NSB Series Statistics\n\n");
    else
        if (urind == 'U') {
            strcat (&byteaminfo[0], "NSB Season Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&byteaminfo[0], "(Post-Season)");
            else
                strcat (&byteaminfo[0], "(Regular Season)");
            strcat (&byteaminfo[0], "\n\n");
        }
        else
            if (urind == 'L') {
                strcat (&byteaminfo[0], "NSB Lifetime Statistics ");
                if (psorrs == '6' || psorrs == '7')
                    strcat (&byteaminfo[0], "(Post-Season)");
                else
                    strcat (&byteaminfo[0], "(Regular Season)");
                strcat (&byteaminfo[0], "\n\n");
            }
            else
                if (urind == 'C')
                    strcat (&byteaminfo[0], "Real Life Statistics (User-Created Team)\n\n");
                else {
                    strcat (&byteaminfo[0], "Real Life Statistics\n\n");
                    strcat (&byteaminfo[0], "(NOTE - The totals do not include players which may have played\n");
                    strcat (&byteaminfo[0], "        in real life but are not included in this NSB roster.)\n\n");

                    /* need RL results to get total team games */
                    strcpy (&work2[0], cnvt_int2str (team.year, 'l'));
                    strcpy (&work1[0], "S3");
                    strcat (&work1[0], &work2[0]);
                    strcat (&work1[0], "\n");
                    sock_puts (sock, work1);  /* tell the server to send real life results for a specific year */

                    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */

                    /* real life data ... get total games played by the team from the Results data (wins + losses) */
                    if (team.year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                        /* get the team name */
                        strcpy (&tname[0], &teaminfo[tmid].teamname[0]);
                        /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                        if (!strncmp (&tname[0], "St ", 3)) {
                            strcpy (&work2[0], &tname[0]);
                            tname[2] = '.';
                            tname[3] = ' ';
                            strcpy (&tname[4], &work2[3]);
                        }

                        for (pib = 0; pib < strlen (&buffer[0]); pib++)
                            if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                                pib += strlen (&tname[0]);   /* move data pointer past team name */
                                for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                                /* get team wins */
                                for (z = 0; buffer[pib] != ' '; z++, pib++)
                                    work2[z] = buffer[pib];
                                work2[z] = '\0';
                                w = atoi (&work2[0]);

                                for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                                /* get team losses */
                                for (z = 0; buffer[pib] != ' '; z++, pib++)
                                    work2[z] = buffer[pib];
                                work2[z] = '\0';
                                l = atoi (&work2[0]);

                                w += l;    /* total team games */
                                totg += w;     /* accumulate total games */

                                break;
                            }
                    }
                }

    /* if the stats we're seeking are not real life then get total games from all the pitchers' W/L records
       we cannot get total games if the stats are real life and the team is user-created */
    if (urind == 'S' || urind == 'U' || urind == 'L')
        for (x = 0; x < maxpitchers; x++) {
            totg += team.pitchers[x].pitching.wins;
            totg += team.pitchers[x].pitching.losses;
        }

    strcat (&byteaminfo[0], "Player Name                  G     AB      R      H     2B     3B     HR    RBI\n\n");

    for (x = 0; x < maxplayers; x++) {
        strncat (&byteaminfo[0], &team.batters[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                       ", 23 - strlen (&team.batters[x].id.name[0]));
        strcat (&byteaminfo[0], (char *) check_stats (team.batters[x].hitting.games, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.atbats, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.runs, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.hits, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.doubles, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.triples, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.homers, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.rbi, 'r'));
        strcat (&byteaminfo[0], "\n");

        tab += team.batters[x].hitting.atbats;
        tr += team.batters[x].hitting.runs;
        th += team.batters[x].hitting.hits;
        t2b += team.batters[x].hitting.doubles;
        t3b += team.batters[x].hitting.triples;
        thr += team.batters[x].hitting.homers;
        tbi += team.batters[x].hitting.rbi;
    }

    if (urind == 'C')
        strcat (&byteaminfo[0], "  TOTALS                      ");
    else {
        strcat (&byteaminfo[0], "  TOTALS               ");
        strcat (&byteaminfo[0], check_stats (totg, 'r'));
    }
    strcat (&byteaminfo[0], check_stats (tab, 'r'));
    strcat (&byteaminfo[0], check_stats (tr, 'r'));
    strcat (&byteaminfo[0], check_stats (th, 'r'));
    strcat (&byteaminfo[0], check_stats (t2b, 'r'));
    strcat (&byteaminfo[0], check_stats (t3b, 'r'));
    strcat (&byteaminfo[0], check_stats (thr, 'r'));
    strcat (&byteaminfo[0], check_stats (tbi, 'r'));
    strcat (&byteaminfo[0], "\n\n");

    if (urind == 'C')
        strcat (&byteaminfo[0], "Player Name          Real Life Team  BA    SA   OBA     BB     SO    HBP   GIDP\n\n");
    else
        strcat (&byteaminfo[0], "Player Name                  BA    SA   OBA     BB     SO    HBP   GIDP\n\n");

    for (x = 0; x < maxplayers; x++) {
        strncat (&byteaminfo[0], &team.batters[x].id.name[0], 23);
        if (urind == 'C')
            strncat (&byteaminfo[0], "                         ", 23 - strlen (&team.batters[x].id.name[0]));
        else
            strncat (&byteaminfo[0], "                         ", 25 - strlen (&team.batters[x].id.name[0]));

        if (urind == 'C') {
            strcat (&byteaminfo[0], check_stats (team.batters[x].id.year, 'l'));
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (team.batters[x].id.teamid == teaminfo[y].id) {
                    strcat (&byteaminfo[0], &teaminfo[y].teamabbrev[0]);
                    strncat (&byteaminfo[0], "      ", 6 - strlen (&teaminfo[y].teamabbrev[0]));
                    break;
                }
        }

        singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers + team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
        strcat (&byteaminfo[0], (char *) do_average (team.batters[x].hitting.hits, team.batters[x].hitting.atbats));
        strcat (&byteaminfo[0], (char *) do_average (((team.batters[x].hitting.homers * 4) + (team.batters[x].hitting.triples * 3) +
                                                 (team.batters[x].hitting.doubles * 2) + singles), team.batters[x].hitting.atbats));
        if (team.batters[x].hitting.sf == -1)
            y = 0;
        else
            y = team.batters[x].hitting.sf;
        y += team.batters[x].hitting.sh;
        strcat (&byteaminfo[0], (char *) do_average ((team.batters[x].hitting.hits + team.batters[x].hitting.bb + team.batters[x].hitting.hbp),
                                                 (team.batters[x].hitting.atbats + team.batters[x].hitting.bb + y + team.batters[x].hitting.hbp)));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.bb, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.so, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.hbp, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.gidp, 'r'));
        strcat (&byteaminfo[0], "\n");

        ts += singles;
        tbb += team.batters[x].hitting.bb;
        if (team.batters[x].hitting.so != -1)
            /* strikeouts for the batter wasn't always a recorded stat */
            tso += team.batters[x].hitting.so;
        thbp += team.batters[x].hitting.hbp;
        if (team.batters[x].hitting.gidp != -1)
            /* grounded into DP for the batter wasn't always a recorded stat */
            tgidp += team.batters[x].hitting.gidp;
    }

    strcat (&byteaminfo[0], "  TOTALS                 ");
    if (urind == 'C')
        strcat (&byteaminfo[0], "        ");
    strcat (&byteaminfo[0], (char *) do_average (th, tab));
    strcat (&byteaminfo[0], (char *) do_average (((thr * 4) + (t3b * 3) + (t2b * 2) + ts), tab));
    strcat (&byteaminfo[0], (char *) do_average ((th + tbb + thbp), (tab + tbb + tsf + tsh + thbp)));
    strcat (&byteaminfo[0], check_stats (tbb, 'r'));
    strcat (&byteaminfo[0], check_stats (tso, 'r'));
    strcat (&byteaminfo[0], check_stats (thbp, 'r'));
    strcat (&byteaminfo[0], check_stats (tgidp, 'r'));

    strcat (&byteaminfo[0], "\n");

    strcat (&byteaminfo[0], "     ");
    if (urind != 'C' && team.year) {
        strcat (&byteaminfo[0], (char *) cnvt_int2str ((team.year), 'l'));
        strcat (&byteaminfo[0], " ");
    }
    if (team.year)
        strcat (&byteaminfo[0], &teaminfo[tmid].teamname[0]);
    else
        strcat (&byteaminfo[0], &usercreatedtname[0]);
    strcat (&byteaminfo[0], " - ");
    if (urind == 'S')
        strcat (&byteaminfo[0], "NSB Series Statistics\n\n");
    else
        if (urind == 'U') {
            strcat (&byteaminfo[0], "NSB Season Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&byteaminfo[0], "(Post-Season)");
            else
                strcat (&byteaminfo[0], "(Regular Season)");
            strcat (&byteaminfo[0], "\n\n");
        }
        else
            if (urind == 'L') {
                strcat (&byteaminfo[0], "NSB Lifetime Statistics ");
                if (psorrs == '6' || psorrs == '7')
                    strcat (&byteaminfo[0], "(Post-Season)");
                else
                    strcat (&byteaminfo[0], "(Regular Season)");
                strcat (&byteaminfo[0], "\n\n");
            }
            else
                if (urind == 'C')
                    strcat (&byteaminfo[0], "Real Life Statistics (User-Created Team)\n\n");
                else
                    strcat (&byteaminfo[0], "Real Life Statistics\n\n");

    strcat (&byteaminfo[0], "Player Name                        SB     CS    IBB     SH     SF\n\n");

    for (x = 0; x < maxplayers; x++) {
        strncat (&byteaminfo[0], &team.batters[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                             ", 29 - strlen (&team.batters[x].id.name[0]));

        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.sb, 'r'));
        strcat (&byteaminfo[0], " ");
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.cs, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.ibb, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.sh, 'r'));
        strcat (&byteaminfo[0], check_stats (team.batters[x].hitting.sf, 'r'));

        tsb += team.batters[x].hitting.sb;
        if (team.batters[x].hitting.cs != -1)
            /* caught stealing for the batter wasn't always a recorded stat */
            tcs += team.batters[x].hitting.cs;
        if (team.batters[x].hitting.ibb != -1)
            /* intentional walk for the batter wasn't always a recorded stat */
            tibb += team.batters[x].hitting.ibb;
        tsh += team.batters[x].hitting.sh;
        if (team.batters[x].hitting.sf != -1)
            /* sacrifice flies for the batter wasn't always a recorded stat */
            tsf += team.batters[x].hitting.sf;
        strcat (&byteaminfo[0], "\n");
    }

    strcat (&byteaminfo[0], "  TOTALS                      ");
    strcat (&byteaminfo[0], check_stats (tsb, 'r'));
    strcat (&byteaminfo[0], check_stats (tcs, 'r'));
    strcat (&byteaminfo[0], check_stats (tibb, 'r'));
    strcat (&byteaminfo[0], check_stats (tsh, 'r'));
    strcat (&byteaminfo[0], check_stats (tsf, 'r'));

    strcat (&byteaminfo[0], "\n");

    tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    strcat (&byteaminfo[0], "     ");
    if (urind != 'C' && team.year) {
        strcat (&byteaminfo[0], (char *) cnvt_int2str ((team.year), 'l'));
        strcat (&byteaminfo[0], " ");
    }
    if (team.year)
        strcat (&byteaminfo[0], &teaminfo[tmid].teamname[0]);
    else
        strcat (&byteaminfo[0], &usercreatedtname[0]);
    strcat (&byteaminfo[0], " - ");
    if (urind == 'S')
        strcat (&byteaminfo[0], "NSB Series Statistics\n\n");
    else
        if (urind == 'U') {
            strcat (&byteaminfo[0], "NSB Season Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&byteaminfo[0], "(Post-Season)");
            else
                strcat (&byteaminfo[0], "(Regular Season)");
            strcat (&byteaminfo[0], "\n\n");
        }
        else
            if (urind == 'L') {
                strcat (&byteaminfo[0], "NSB Lifetime Statistics ");
                if (psorrs == '6' || psorrs == '7')
                    strcat (&byteaminfo[0], "(Post-Season)");
                else
                    strcat (&byteaminfo[0], "(Regular Season)");
                strcat (&byteaminfo[0], "\n\n");
            }
            else
                if (urind == 'C')
                    strcat (&byteaminfo[0], "Real Life Statistics (User-Created Team)\n\n");
                else
                    strcat (&byteaminfo[0], "Real Life Statistics\n\n");

    strcat (&byteaminfo[0], "Pitcher Name                 G     IP        H      R     ER      W      L   PCT\n\n");

    for (x = 0; x < maxpitchers; x++) {
        if (team.pitchers[x].id.name[0] == ' ')
            break;
        strncat (&byteaminfo[0], &team.pitchers[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                       ", 23 - strlen (&team.pitchers[x].id.name[0]));
        strcat (&byteaminfo[0], (char *) check_stats (team.pitchers[x].pitching.games, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.innings, 'r'));
        if (team.pitchers[x].pitching.thirds == 1 || team.pitchers[x].pitching.thirds == 2) {
            strcat (&byteaminfo[0], ".");
            strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.thirds, 'l'));
        }
        else
            strcat (&byteaminfo[0], "  ");
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.hits, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.runs, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.er, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.wins, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.losses, 'r'));
        strcat (&byteaminfo[0], (char *) do_average (team.pitchers[x].pitching.wins, (team.pitchers[x].pitching.wins + team.pitchers[x].pitching.losses)));

        tinn += team.pitchers[x].pitching.innings;
        tthirds += team.pitchers[x].pitching.thirds;
        tr += team.pitchers[x].pitching.runs;
        th += team.pitchers[x].pitching.hits;
        ter += team.pitchers[x].pitching.er;
        tw += team.pitchers[x].pitching.wins;
        tl += team.pitchers[x].pitching.losses;
        strcat (&byteaminfo[0], "\n");
    }

    if (urind == 'C')
        strcat (&byteaminfo[0], "  TOTALS                      ");
    else {
        strcat (&byteaminfo[0], "  TOTALS               ");
        strcat (&byteaminfo[0], check_stats (totg, 'r'));
    }
    for (; tthirds > 2; tthirds -= 3)
        tinn++;
    strcat (&byteaminfo[0], check_stats (tinn, 'r'));
    if (tthirds == 1 || tthirds == 2) {
        strcat (&byteaminfo[0], ".");
        strcat (&byteaminfo[0], check_stats (tthirds, 'l'));
    }
    else
        strcat (&byteaminfo[0], "  ");
    strcat (&byteaminfo[0], check_stats (th, 'r'));
    strcat (&byteaminfo[0], check_stats (tr, 'r'));
    strcat (&byteaminfo[0], check_stats (ter, 'r'));
    strcat (&byteaminfo[0], check_stats (tw, 'r'));
    strcat (&byteaminfo[0], check_stats (tl, 'r'));
    strcat (&byteaminfo[0], (char *) do_average (tw, (tw + tl)));
    strcat (&byteaminfo[0], "\n\n");

    strcat (&byteaminfo[0], "Pitcher Name                SO     BB   ERA     GS     CG     SHO     S    SOPP\n\n");

    for (x = 0; x < maxpitchers; x++) {
        if (team.pitchers[x].id.name[0] == ' ')
            break;
        strncat (&byteaminfo[0], &team.pitchers[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                       ", 23 - strlen (&team.pitchers[x].id.name[0]));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.so, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.walks, 'r'));
        strcat (&byteaminfo[0], (char *) do_era (team.pitchers[x].pitching.er * 9, team.pitchers[x].pitching.innings, team.pitchers[x].pitching.thirds));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.games_started, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.cg, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.sho, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.saves, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.svopp, 'r'));
        strcat (&byteaminfo[0], "\n");

        ts += team.pitchers[x].pitching.saves;
        if (team.pitchers[x].pitching.svopp != -1)
            /* save opportunities for the pitcher wasn't always a recorded stat */
            tsopp += team.pitchers[x].pitching.svopp;
        tbb += team.pitchers[x].pitching.walks;
        tso += team.pitchers[x].pitching.so;
        tgs += team.pitchers[x].pitching.games_started;
        tcg += team.pitchers[x].pitching.cg;
        tsho += team.pitchers[x].pitching.sho;
    }

    strcat (&byteaminfo[0], "  TOTALS               ");
    strcat (&byteaminfo[0], check_stats (tso, 'r'));
    strcat (&byteaminfo[0], check_stats (tbb, 'r'));
    strcat (&byteaminfo[0], (char *) do_era (ter * 9, tinn, tthirds));
    strcat (&byteaminfo[0], check_stats (tgs, 'r'));
    strcat (&byteaminfo[0], check_stats (tcg, 'r'));
    strcat (&byteaminfo[0], check_stats (tsho, 'r'));
    strcat (&byteaminfo[0], check_stats (ts, 'r'));
    strcat (&byteaminfo[0], check_stats (tsopp, 'r'));
    strcat (&byteaminfo[0], "\n\n");

    strcat (&byteaminfo[0], "Pitcher Name                GF    IBB     WP     HB      B    BFP     SH     SF\n\n");

    for (x = 0; x < maxpitchers; x++) {
        if (team.pitchers[x].id.name[0] == ' ')
            break;
        strncat (&byteaminfo[0], &team.pitchers[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                       ", 23 - strlen (&team.pitchers[x].id.name[0]));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.gf, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.ibb, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.wp, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.hb, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.balks, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.bfp, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.sh, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.sf, 'r'));
        strcat (&byteaminfo[0], "\n");

        tgf += team.pitchers[x].pitching.gf;
        if (team.pitchers[x].pitching.ibb != -1)
            /* intentional walks for the pitcher wasn't always a recorded stat */
            tibb += team.pitchers[x].pitching.ibb;
        twp += team.pitchers[x].pitching.wp;
        thb += team.pitchers[x].pitching.hb;
        tb += team.pitchers[x].pitching.balks;
        if (team.pitchers[x].pitching.bfp != -1)
            /* batters facing pitcher wasn't always a recorded stat */
            tbfp += team.pitchers[x].pitching.bfp;
        if (team.pitchers[x].pitching.sh != -1)
            /* sacrifice hits allowed for the pitcher wasn't always a recorded stat */
            tsh += team.pitchers[x].pitching.sh;
        if (team.pitchers[x].pitching.sf != -1)
            /* sacrifice flies allowed for the pitcher wasn't always a recorded stat */
            tsf += team.pitchers[x].pitching.sf;
    }

    strcat (&byteaminfo[0], "  TOTALS               ");
    strcat (&byteaminfo[0], check_stats (tgf, 'r'));
    strcat (&byteaminfo[0], check_stats (tibb, 'r'));
    strcat (&byteaminfo[0], check_stats (twp, 'r'));
    strcat (&byteaminfo[0], check_stats (thb, 'r'));
    strcat (&byteaminfo[0], check_stats (tb, 'r'));
    strcat (&byteaminfo[0], check_stats (tbfp, 'r'));
    strcat (&byteaminfo[0], check_stats (tsh, 'r'));
    strcat (&byteaminfo[0], check_stats (tsf, 'r'));
    strcat (&byteaminfo[0], "\n\n");

    strcat (&byteaminfo[0], "Pitcher Name                2B     3B     HR    RBI     SB     CS   OPAB  OPBA\n\n");

    for (x = 0; x < maxpitchers; x++) {
        if (team.pitchers[x].id.name[0] == ' ')
            break;
        strncat (&byteaminfo[0], &team.pitchers[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                       ", 23 - strlen (&team.pitchers[x].id.name[0]));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.doubles, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.triples, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.homers, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.rbi, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.sb, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.cs, 'r'));
        strcat (&byteaminfo[0], check_stats (team.pitchers[x].pitching.opp_ab, 'r'));
        strcat (&byteaminfo[0], (char *) do_average (team.pitchers[x].pitching.hits, team.pitchers[x].pitching.opp_ab));
        strcat (&byteaminfo[0], "\n");

        if (team.pitchers[x].pitching.doubles != -1)
            /* doubles allowed for the pitcher wasn't always a recorded stat */
            t2b += team.pitchers[x].pitching.doubles;
        if (team.pitchers[x].pitching.triples != -1)
            /* triples allowed for the pitcher wasn't always a recorded stat */
            t3b += team.pitchers[x].pitching.triples;
        thr += team.pitchers[x].pitching.homers;
        if (team.pitchers[x].pitching.rbi != -1)
            /* RBIs allowed for the pitcher wasn't always a recorded stat */
            tbi += team.pitchers[x].pitching.rbi;
        if (team.pitchers[x].pitching.sb != -1)
            /* stolen bases allowed for the pitcher wasn't always a recorded stat */
            tsb += team.pitchers[x].pitching.sb;
        if (team.pitchers[x].pitching.cs != -1)
            /* caught stealing against for the pitcher wasn't always a recorded stat */
            tcs += team.pitchers[x].pitching.cs;
        if (team.pitchers[x].pitching.opp_ab != -1)
            /* at bats against for the pitcher wasn't always a recorded stat */
            topp_ab += team.pitchers[x].pitching.opp_ab;
    }

    strcat (&byteaminfo[0], "  TOTALS               ");
    strcat (&byteaminfo[0], check_stats (t2b, 'r'));
    strcat (&byteaminfo[0], check_stats (t3b, 'r'));
    strcat (&byteaminfo[0], check_stats (thr, 'r'));
    strcat (&byteaminfo[0], check_stats (tbi, 'r'));
    strcat (&byteaminfo[0], check_stats (tsb, 'r'));
    strcat (&byteaminfo[0], check_stats (tcs, 'r'));
    strcat (&byteaminfo[0], check_stats (topp_ab, 'r'));
    if (topp_ab)
        strcat (&byteaminfo[0], (char *) do_average (th, topp_ab));

    strcat (&byteaminfo[0], "\n");

    strcat (&byteaminfo[0], "     ");
    if (urind != 'C' && team.year) {
        strcat (&byteaminfo[0], (char *) cnvt_int2str ((team.year), 'l'));
        strcat (&byteaminfo[0], " ");
    }
    if (team.year)
        strcat (&byteaminfo[0], &teaminfo[tmid].teamname[0]);
    else
        strcat (&byteaminfo[0], &usercreatedtname[0]);
    strcat (&byteaminfo[0], " - ");
    if (urind == 'S')
        strcat (&byteaminfo[0], "NSB Series Statistics\n\n");
    else
        if (urind == 'U') {
            strcat (&byteaminfo[0], "NSB Season Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&byteaminfo[0], "(Post-Season)");
            else
                strcat (&byteaminfo[0], "(Regular Season)");
            strcat (&byteaminfo[0], "\n\n");
        }
        else
            if (urind == 'L') {
                strcat (&byteaminfo[0], "NSB Lifetime Statistics ");
                if (psorrs == '6' || psorrs == '7')
                    strcat (&byteaminfo[0], "(Post-Season)");
                else
                    strcat (&byteaminfo[0], "(Regular Season)");
                strcat (&byteaminfo[0], "\n\n");
            }
            else
                if (urind == 'C')
                    strcat (&byteaminfo[0], "Real Life Statistics (User-Created Team)\n\n");
                else
                    strcat (&byteaminfo[0], "Real Life Statistics\n\n");

    strcat (&byteaminfo[0], "Player Name           POS      G     PO     DP      A      E     PB   FA\n\n");

    for (x = 0; x < maxplayers; x++) {
        strncat (&byteaminfo[0], &team.batters[x].id.name[0], 23);
        strncat (&byteaminfo[0], "                    ", 23 - strlen (&team.batters[x].id.name[0]));

        for (newpl = y = 0; y < 11; y++)
            if (team.batters[x].fielding[y].games > 0) {
                if (newpl == 1)
                    strcat (&byteaminfo[0], "                       ");
                if (y == 10)
                    strcat (&byteaminfo[0], "OF");
                else
                    strcat (&byteaminfo[0], figure_pos (y));
                strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].games, 'r'));
                strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].po, 'r'));
                strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].dp, 'r'));
                strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].a, 'r'));
                strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].e, 'r'));
                if (y == 2)
                    strcat (&byteaminfo[0], check_stats (team.batters[x].fielding[y].pb, 'r'));
                else
                    strcat (&byteaminfo[0], "       ");
                strcat (&byteaminfo[0], (char *) do_average ((team.batters[x].fielding[y].po + team.batters[x].fielding[y].a),
                    (team.batters[x].fielding[y].po + team.batters[x].fielding[y].a + team.batters[x].fielding[y].e)));
                newpl = 1;
                strcat (&byteaminfo[0], "\n");

                if (team.batters[x].fielding[y].po != -1)
                    tpo += team.batters[x].fielding[y].po;
                if (team.batters[x].fielding[y].a != -1)
                    ta += team.batters[x].fielding[y].a;
                if (team.batters[x].fielding[y].e != -1)
                    te += team.batters[x].fielding[y].e;
                if (y == 2)
                    tpb += team.batters[x].fielding[y].pb;
            }
        if (newpl == 0)
            strcat (&byteaminfo[0], "\n");
    }

    if (urind == 'C')
        strcat (&byteaminfo[0], "  TOTALS                        ");
    else {
        strcat (&byteaminfo[0], "  TOTALS                 ");
        strcat (&byteaminfo[0], check_stats (totg, 'r'));
    }
    strcat (&byteaminfo[0], check_stats (tpo, 'r'));
    strcat (&byteaminfo[0], "       ");
    strcat (&byteaminfo[0], check_stats (ta, 'r'));
    strcat (&byteaminfo[0], check_stats (te, 'r'));
    strcat (&byteaminfo[0], check_stats (tpb, 'r'));
    strcat (&byteaminfo[0], (char *) do_average ((tpo + ta), (tpo + ta + te)));
    strcat (&byteaminfo[0], "\n");

    print (&byteaminfo[0]);

    strcpy (&work[0], "Print Team Stats.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
NSBLifetimeRS (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (6);
}

void
NSBLifetimePS (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (8);
}

void
NSBCategoryLeadersPS (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (7);
}

void
NSBCategoryLeadersRS (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (1);
}

void
NSBCategoryLeadersSeries (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (3);
}

void
RealLifeCategoryLeaders (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 0;
    CategoryLeaders (2);
}

void
NSBRecordsPersonalGame (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 1;
    CategoryLeaders (5);
}

void
NSBRecordsPersonalSeason (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = 2;
    wri = 1;
    CategoryLeaders (5);
}

void
NSBRecordsAllGame (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = 1;
    wri = 2;
    CategoryLeaders (5);
}

void
NSBRecordsAllSeason (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sgi = wri = 2;
    CategoryLeaders (5);
}

void
NSBLifetimeByTeam (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x;

    sock_puts (sock, "S61\n");  /* tell the server we want a list of our NSB Lifetime teams */

    x = sock_gets (sock, &buffer[0], sizeof (buffer));
    if (strlen (&buffer[0]) <= 2) {
        gchar NoLT[256] = "No Lifetime teams available.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No lifetime teams on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");

        msg[0] = &NoLT[0];

        Add2TextWindow (&work[0], 1);
        outMessage (msg);

        return;
    }

    if (x < 0) {
        gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
            strcpy (&work[0], "Encountered error when talking to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &Error[0];
        }

        outMessage (msg);
        return;
    }
    urind = 'L';
    Select1Team (urind);
}

void
NSBByTeam (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x;

    sock_puts (sock, "S6\n");  /* tell the server we want a list of our NSB teams */

    x = sock_gets (sock, &buffer[0], sizeof (buffer));
    if (strlen (&buffer[0]) <= 2) {
        gchar NoDir[256] = "You have no season.  First set up a season.", *msg[5];
        gint x;

        /* this happens if the user has no season established */
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No season established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");

        msg[0] = &NoDir[0];

        Add2TextWindow (&work[0], 1);
        outMessage (msg);

        return;
    }

    if (x < 0) {
        gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
            strcpy (&work[0], "Encountered error when talking to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &Error[0];
        }

        outMessage (msg);
        return;
    }
    urind = 'U';
    Select1Team (urind);
}

void
NSBSeriesByTeam (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x;
    gchar buf[256], NoLeague[256] = "You have no series.  First set up a series.", *msg[5];

    /* check for the existence of a series */
    sock_puts (sock, "S0\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "NOSERIES")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No series established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoLeague[0];
        outMessage (msg);

        return;
    }

    sock_puts (sock, "S6\n");  /* tell the server we want a list of our NSB teams */

    x = sock_gets (sock, &buffer[0], sizeof (buffer));

    if (x < 0) {
        gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
            strcpy (&work[0], "Encountered error when talking to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &Error[0];
        }

        outMessage (msg);
        return;
    }
    urind = 'S';
    Select1Team (urind);
}

void
UserCreatedTeamTotals (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S693\n");  /* tell the server to send user-created team totals */
    strcpy (&work[0], "S693\n");
    urind = 'U';
    whichur[prtbutttpnt] = urind;
    ShowTeamTotals (4);
}

void
UserCreatedByTeam (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S64\n");  /* tell the server to send us all user-created teams available */
    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get teams */

    if (strlen (&buffer[0]) < 3) {
        /* this happens if there are no user-created teams */
        gchar NoStats[256] = "There are no user-created teams.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);

        return;
    }

    urind = 'C';
    Select1Team (urind);
}

void
RealLifeByTeam (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S63\n");  /* tell the server to send us all Real Life teams available */
    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get teams */

    if (strlen (&buffer[0]) < 10) {
        /* this happens if there are no team stats */
        gchar NoStats[256] = "The Team Statistics do not exist.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);

        return;
    }

    urind = 'R';
    Select1Team (urind);
}

void
NSBRegularSeasonTeamTotals (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S691\n");  /* tell the server to send NSB season regular season team totals */
    strcpy (&work[0], "S691\n");
    urind = 'U';
    whichur[prtbutttpnt] = urind;
    ShowTeamTotals (1);
}

void
NSBPostSeasonTeamTotals (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    sock_puts (sock, "S692\n");  /* tell the server to send NSB season post season team totals */
    strcpy (&work[0], "S692\n");
    urind = 'U';
    whichur[prtbutttpnt] = urind;
    ShowTeamTotals (2);
}

void
NSBSeriesTeamTotals (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x;
    gchar buf[256], NoLeague[256] = "You have no series.  First set up a series.", *msg[5];

    /* check for the existence of a series */
    sock_puts (sock, "S0\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "NOSERIES")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No series established on "); 
        strcat (&work[0], &hs[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1); 

        msg[0] = &NoLeague[0];
        outMessage (msg);

        return;
    }   

    sock_puts (sock, "S691\n");  /* tell the server to send NSB series team totals */
    strcpy (&work[0], "S691\n");
    urind = 'U';
    whichur[prtbutttpnt] = urind;
    ShowTeamTotals (3);
}

GtkTextBuffer *ttbuffer;

void
ShowTeamTotals (int type) {
    GtkWidget *window, *vpaned, *view, *sw, *vbox, *hbox, *pbutton, *button;
    gint x;
    gchar NoStats[256] = "The Team Statistics do not exist.", *msg[5];

    GetTeamTotals ();
    if (!nteams) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);

        return;
    }

    psorrs = whichpsorrs[prtbutttpnt] = type + '0';

    /* set up to display stats */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 1350, 600);
    g_signal_connect (window, "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Team Totals");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    vpaned = gtk_vpaned_new ();
    gtk_container_set_border_width (GTK_CONTAINER (vpaned), 5);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

    view = gtk_text_view_new ();
    ttbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_NONE);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_add1 (GTK_PANED (vpaned), sw);
    gtk_container_add (GTK_CONTAINER (sw), view);

    FillTeamTotals (ttbuffer);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    strcpy (&prtbutttcmd[prtbutttpnt][0], &work[0]);
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintTeamTotals), GINT_TO_POINTER (prtbutttpnt));
    prtbutttpnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillTeamTotals (GtkTextBuffer *buf) {
    GtkTextIter pos;
    gint x, y, z, pib, totg, singles, tsingles, w, l;
    gchar work1[100], tname[100];
    float fnum1, fnum2;

    gtk_text_buffer_get_iter_at_offset (buf, &pos, 0);
    gtk_text_buffer_create_tag (buf, "monospace", "family", "monospace", NULL);

    strcpy (&stats[0], "                               ");
    if (urind == 'U') {
        if (psorrs == '4') {
            strcat (&stats[0], "User-Created Teams Statistics ");
            strcat (&stats[0], "\n(NOTE - Some stats are not available for all years which may make some of the totals look strange.) ");
        }
        else
            if (psorrs == '3')
                strcat (&stats[0], "NSB Series Statistics ");
            else {
                strcat (&stats[0], "NSB Season Statistics ");
                if (psorrs == '2')
                    strcat (&stats[0], "(Post-Season)");
                else
                    strcat (&stats[0], "(Regular Season)");
            }
    }
    else {
        strcat (&stats[0], "Real Life Statistics (Regular Season)\n\n");
        strcat (&stats[0], "(NOTE - These stats do not include players who may have played ");
        strcat (&stats[0], "in real life but are not included in these NSB rosters.\n");
        strcat (&stats[0], "        Each team's combined wins and losses in the pitching stats may not ");
        strcat (&stats[0], "equal the number of games the team played during the regular season.)");

        /* need RL results to get total team games */
        strcpy (&work1[0], "S3");
        strcat (&work1[0], &teamyr[0]);       /* year saved when it was acquired from user */
        strcat (&work1[0], "\n");
        sock_puts (sock, work1);  /* tell the server to send real life results for a specific year */

        sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */
    }
    strcat (&stats[0], "\n\n");

    strcat (&stats[0], "Team Year & Name             G     AB      R      H     2B     3B     HR    RBI    BA    SA   OBA     BB     SO    HBP   GIDP     SB     CS    IBB     SH     SF\n\n");

    /* sort team data by batting average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = (float) teamdata[x].batters.hitting.hits / (float) teamdata[x].batters.hitting.atbats;
            fnum2 = (float) teamdata[y].batters.hitting.hits / (float) teamdata[y].batters.hitting.atbats;

            if (fnum1 < fnum2) {
                teamdata[300] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[300];

                strcpy (&uctname[300][0], &uctname[x][0]);
                strcpy (&uctname[x][0], &uctname[y][0]);
                strcpy (&uctname[y][0], &uctname[300][0]);
            }
        }

    teamdata[301].batters.hitting.games = teamdata[301].batters.hitting.atbats = teamdata[301].batters.hitting.runs =
      teamdata[301].batters.hitting.hits = teamdata[301].batters.hitting.doubles = teamdata[301].batters.hitting.triples =
      teamdata[301].batters.hitting.homers = teamdata[301].batters.hitting.rbi = teamdata[301].batters.hitting.bb = 
      teamdata[301].batters.hitting.so = teamdata[301].batters.hitting.hbp = teamdata[301].batters.hitting.gidp =
      teamdata[301].batters.hitting.sb = teamdata[301].batters.hitting.cs = teamdata[301].batters.hitting.ibb =
      teamdata[301].batters.hitting.sh = teamdata[301].batters.hitting.sf = tsingles = 0;

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&stats[0], " ");
        }
        else
            strcat (&stats[0], "     ");
        if (teamdata[x].year) {
            strncat (&stats[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&stats[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&stats[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&stats[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        singles = teamdata[x].batters.hitting.hits - (teamdata[x].batters.hitting.homers +
                  teamdata[x].batters.hitting.triples + teamdata[x].batters.hitting.doubles);
        tsingles += singles;

        if (urind == 'U') {
            strcat (&stats[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
            teamdata[301].batters.hitting.games += teamdata[x].pitchers.pitching.wins;
        }
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work1[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work1[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        w = atoi (&work1[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        l = atoi (&work1[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work1[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&stats[0], "    ");
                        strcat (&stats[0], &work1[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], "       ");

        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.atbats, 'r'));
        teamdata[301].batters.hitting.atbats += teamdata[x].batters.hitting.atbats;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.runs, 'r'));
        teamdata[301].batters.hitting.runs += teamdata[x].batters.hitting.runs;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.hits, 'r'));
        teamdata[301].batters.hitting.hits += teamdata[x].batters.hitting.hits;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.doubles, 'r'));
        teamdata[301].batters.hitting.doubles += teamdata[x].batters.hitting.doubles;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.triples, 'r'));
        teamdata[301].batters.hitting.triples += teamdata[x].batters.hitting.triples;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.homers, 'r'));
        teamdata[301].batters.hitting.homers += teamdata[x].batters.hitting.homers;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.rbi, 'r'));
        teamdata[301].batters.hitting.rbi += teamdata[x].batters.hitting.rbi;
        strcat (&stats[0], (char *) do_average (teamdata[x].batters.hitting.hits, teamdata[x].batters.hitting.atbats));
        strcat (&stats[0], (char *) do_average (((teamdata[x].batters.hitting.homers * 4) + (teamdata[x].batters.hitting.triples * 3) +
                                                 (teamdata[x].batters.hitting.doubles * 2) + singles), teamdata[x].batters.hitting.atbats));
        if (teamdata[x].batters.hitting.sf == -1)
            y = 0;
        else
            y = teamdata[x].batters.hitting.sf;
        y += teamdata[x].batters.hitting.sh;
        strcat (&stats[0], (char *) do_average ((teamdata[x].batters.hitting.hits + teamdata[x].batters.hitting.bb + teamdata[x].batters.hitting.hbp),
                                                 (teamdata[x].batters.hitting.atbats + teamdata[x].batters.hitting.bb + y +
                                                  teamdata[x].batters.hitting.hbp)));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.bb, 'r'));
        teamdata[301].batters.hitting.bb += teamdata[x].batters.hitting.bb;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.so, 'r'));
        teamdata[301].batters.hitting.so += teamdata[x].batters.hitting.so;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.hbp, 'r'));
        teamdata[301].batters.hitting.hbp += teamdata[x].batters.hitting.hbp;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.gidp, 'r'));
        teamdata[301].batters.hitting.gidp += teamdata[x].batters.hitting.gidp;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sb, 'r'));
        teamdata[301].batters.hitting.sb += teamdata[x].batters.hitting.sb;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.cs, 'r'));
        teamdata[301].batters.hitting.cs += teamdata[x].batters.hitting.cs;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.ibb, 'r'));
        teamdata[301].batters.hitting.ibb += teamdata[x].batters.hitting.ibb;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sh, 'r'));
        teamdata[301].batters.hitting.sh += teamdata[x].batters.hitting.sh;
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sf, 'r'));
        if (teamdata[x].batters.hitting.sf != -1)
            teamdata[301].batters.hitting.sf += teamdata[x].batters.hitting.sf;
        strcat (&stats[0], "\n");
    }
    strcat (&stats[0], "                 TOTALS");
    if (urind == 'U')
        strcat (&stats[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work1[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work1[0])) {
                case 0:
                    strcat (&stats[0], "       ");
                    break;
                case 1:
                    strcat (&stats[0], "      ");
                    break;
                case 2:
                    strcat (&stats[0], "     ");
                    break;
                case 3:
                    strcat (&stats[0], "    ");
                    break;
                case 4:
                    strcat (&stats[0], "   ");
                    break;
                case 5:
                    strcat (&stats[0], "  ");
                    break;
                case 6:
                    strcat (&stats[0], " ");
                    break;
            }

            strcat (&stats[0], &work1[0]);
        }
        else
            strcat (&stats[0], "       ");

    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.atbats, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.runs, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.hits, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.doubles, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.triples, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.homers, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.rbi, 'r'));
    strcat (&stats[0], (char *) do_average (teamdata[301].batters.hitting.hits, teamdata[301].batters.hitting.atbats));
    strcat (&stats[0], (char *) do_average (((teamdata[301].batters.hitting.homers * 4) + (teamdata[301].batters.hitting.triples * 3) +
                                             (teamdata[301].batters.hitting.doubles * 2) + tsingles), teamdata[301].batters.hitting.atbats));
    strcat (&stats[0], (char *) do_average ((teamdata[301].batters.hitting.hits + teamdata[301].batters.hitting.bb + teamdata[301].batters.hitting.hbp),
                                             (teamdata[301].batters.hitting.atbats + teamdata[301].batters.hitting.bb + teamdata[301].batters.hitting.sh +
                                             teamdata[301].batters.hitting.sf + teamdata[301].batters.hitting.hbp)));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.bb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.so, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.hbp, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.gidp, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.sb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.cs, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.ibb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.sh, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.hitting.sf, 'r'));

    strcat (&stats[0], "\n\n\n");

    strcat (&stats[0], "Team Year & Name             G     IP        H      R     ER      W      L   PCT     SO     BB   ERA     GS     CG     GF    SHO      S   SOPP    IBB     WP     HB      B    BFP     2B     3B     HR    RBI     SB     CS     SH     SF   OPAB  OPBA\n\n");

    /* sort team data by earned run average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = (float) (teamdata[x].pitchers.pitching.er * 9.0) / ((float) teamdata[x].pitchers.pitching.innings +
                                                   (float) teamdata[x].pitchers.pitching.thirds / 3.0);
            fnum2 = (float) (teamdata[y].pitchers.pitching.er * 9.0) / ((float) teamdata[y].pitchers.pitching.innings +
                                                   (float) teamdata[y].pitchers.pitching.thirds / 3.0);

            if (fnum1 > fnum2) {
                teamdata[300] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[300];

                strcpy (&uctname[300][0], &uctname[x][0]);
                strcpy (&uctname[x][0], &uctname[y][0]);
                strcpy (&uctname[y][0], &uctname[300][0]);
            }
        }

    teamdata[301].pitchers.pitching.innings = teamdata[301].pitchers.pitching.thirds = teamdata[301].pitchers.pitching.hits =
      teamdata[301].pitchers.pitching.runs = teamdata[301].pitchers.pitching.er = teamdata[301].pitchers.pitching.wins =
      teamdata[301].pitchers.pitching.losses = teamdata[301].pitchers.pitching.so = teamdata[301].pitchers.pitching.walks =
      teamdata[301].pitchers.pitching.games_started = teamdata[301].pitchers.pitching.cg = teamdata[301].pitchers.pitching.gf =
      teamdata[301].pitchers.pitching.sho = teamdata[301].pitchers.pitching.saves = teamdata[301].pitchers.pitching.svopp =
      teamdata[301].pitchers.pitching.ibb = teamdata[301].pitchers.pitching.wp = teamdata[301].pitchers.pitching.hb =
      teamdata[301].pitchers.pitching.balks = teamdata[301].pitchers.pitching.bfp = teamdata[301].pitchers.pitching.doubles =
      teamdata[301].pitchers.pitching.triples = teamdata[301].pitchers.pitching.homers = teamdata[301].pitchers.pitching.rbi =
      teamdata[301].pitchers.pitching.sb = teamdata[301].pitchers.pitching.cs = teamdata[301].pitchers.pitching.sh =
      teamdata[301].pitchers.pitching.sf = teamdata[301].pitchers.pitching.opp_ab = 0;

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&stats[0], " ");
        }
        else
            strcat (&stats[0], "     ");
        if (teamdata[x].year) {
            strncat (&stats[0], &teaminfo[y].teamname[0], 18);
            if (strlen (&teaminfo[y].teamname[0]) < 18)
                strncat (&stats[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&stats[0], &uctname[x][0], 18);
            if (strlen (&uctname[x][0]) < 18)
                strncat (&stats[0], "                       ", 18 - strlen (&uctname[x][0]));
        }

        z = (int) (teamdata[x].pitchers.pitching.thirds / 3);
        teamdata[x].pitchers.pitching.innings += z;
        teamdata[x].pitchers.pitching.thirds %= 3;
        if (urind == 'U')
            strcat (&stats[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work1[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work1[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        w = atoi (&work1[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        l = atoi (&work1[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work1[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&stats[0], "    ");
                        strcat (&stats[0], &work1[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], "       ");

        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.innings, 'r'));
        if (teamdata[x].pitchers.pitching.thirds == 1 || teamdata[x].pitchers.pitching.thirds == 2) {
            strcat (&stats[0], ".");
            strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.thirds, 'l'));
        }
        else
            strcat (&stats[0], "  ");
        teamdata[301].pitchers.pitching.innings += teamdata[x].pitchers.pitching.innings;
        teamdata[301].pitchers.pitching.thirds += teamdata[x].pitchers.pitching.thirds;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.hits, 'r'));
        teamdata[301].pitchers.pitching.hits += teamdata[x].pitchers.pitching.hits;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.runs, 'r'));
        teamdata[301].pitchers.pitching.runs += teamdata[x].pitchers.pitching.runs;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.er, 'r'));
        teamdata[301].pitchers.pitching.er += teamdata[x].pitchers.pitching.er;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.wins, 'r'));
        teamdata[301].pitchers.pitching.wins += teamdata[x].pitchers.pitching.wins;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.losses, 'r'));
        teamdata[301].pitchers.pitching.losses += teamdata[x].pitchers.pitching.losses;
        strcat (&stats[0], (char *) do_average (teamdata[x].pitchers.pitching.wins, (teamdata[x].pitchers.pitching.wins +
                                           teamdata[x].pitchers.pitching.losses)));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.so, 'r'));
        teamdata[301].pitchers.pitching.so += teamdata[x].pitchers.pitching.so;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.walks, 'r'));
        teamdata[301].pitchers.pitching.walks += teamdata[x].pitchers.pitching.walks;
        strcat (&stats[0], (char *) do_era (teamdata[x].pitchers.pitching.er * 9, teamdata[x].pitchers.pitching.innings,
                                                   teamdata[x].pitchers.pitching.thirds));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.games_started, 'r'));
        teamdata[301].pitchers.pitching.games_started += teamdata[x].pitchers.pitching.games_started;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.cg, 'r'));
        teamdata[301].pitchers.pitching.cg += teamdata[x].pitchers.pitching.cg;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.gf, 'r'));
        teamdata[301].pitchers.pitching.gf += teamdata[x].pitchers.pitching.gf;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sho, 'r'));
        teamdata[301].pitchers.pitching.sho += teamdata[x].pitchers.pitching.sho;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.saves, 'r'));
        teamdata[301].pitchers.pitching.saves += teamdata[x].pitchers.pitching.saves;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.svopp, 'r'));
        teamdata[301].pitchers.pitching.svopp += teamdata[x].pitchers.pitching.svopp;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.ibb, 'r'));
        teamdata[301].pitchers.pitching.ibb += teamdata[x].pitchers.pitching.ibb;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.wp, 'r'));
        teamdata[301].pitchers.pitching.wp += teamdata[x].pitchers.pitching.wp;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.hb, 'r'));
        teamdata[301].pitchers.pitching.hb += teamdata[x].pitchers.pitching.hb;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.balks, 'r'));
        teamdata[301].pitchers.pitching.balks += teamdata[x].pitchers.pitching.balks;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.bfp, 'r'));
        teamdata[301].pitchers.pitching.bfp += teamdata[x].pitchers.pitching.bfp;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.doubles, 'r'));
        teamdata[301].pitchers.pitching.doubles += teamdata[x].pitchers.pitching.doubles;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.triples, 'r'));
        teamdata[301].pitchers.pitching.triples += teamdata[x].pitchers.pitching.triples;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.homers, 'r'));
        teamdata[301].pitchers.pitching.homers += teamdata[x].pitchers.pitching.homers;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.rbi, 'r'));
        teamdata[301].pitchers.pitching.rbi += teamdata[x].pitchers.pitching.rbi;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sb, 'r'));
        teamdata[301].pitchers.pitching.sb += teamdata[x].pitchers.pitching.sb;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.cs, 'r'));
        teamdata[301].pitchers.pitching.cs += teamdata[x].pitchers.pitching.cs;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sh, 'r'));
        teamdata[301].pitchers.pitching.sh += teamdata[x].pitchers.pitching.sh;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sf, 'r'));
        teamdata[301].pitchers.pitching.sf += teamdata[x].pitchers.pitching.sf;
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.opp_ab, 'r'));
        teamdata[301].pitchers.pitching.opp_ab += teamdata[x].pitchers.pitching.opp_ab;
        strcat (&stats[0], (char *) do_average (teamdata[x].pitchers.pitching.hits, teamdata[x].pitchers.pitching.opp_ab));
        strcat (&stats[0], "\n");
    }
    strcat (&stats[0], "                 TOTALS");
    if (urind == 'U')
        strcat (&stats[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work1[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work1[0])) {
                case 0:
                    strcat (&stats[0], "       ");
                    break;
                case 1:
                    strcat (&stats[0], "      ");
                    break;
                case 2:
                    strcat (&stats[0], "     ");
                    break;
                case 3:
                    strcat (&stats[0], "    ");
                    break;
                case 4:
                    strcat (&stats[0], "   ");
                    break;
                case 5:
                    strcat (&stats[0], "  ");
                    break;
                case 6:
                    strcat (&stats[0], " ");
                    break;
            }

            strcat (&stats[0], &work1[0]);
        }
        else
            strcat (&stats[0], "       ");

    y = (int) (teamdata[301].pitchers.pitching.thirds / 3);
    teamdata[301].pitchers.pitching.innings += y;
    teamdata[301].pitchers.pitching.thirds %= 3;

    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.innings, 'r'));
    if (teamdata[301].pitchers.pitching.thirds == 1 || teamdata[301].pitchers.pitching.thirds == 2) {
        strcat (&stats[0], ".");
        strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.thirds, 'l'));
    }
    else
        strcat (&stats[0], "  ");
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.hits, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.runs, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.er, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.wins, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.losses, 'r'));
    strcat (&stats[0], (char *) do_average (teamdata[301].pitchers.pitching.wins, (teamdata[301].pitchers.pitching.wins +
                                            teamdata[301].pitchers.pitching.losses)));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.so, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.walks, 'r'));
    strcat (&stats[0], (char *) do_era (teamdata[301].pitchers.pitching.er * 9, teamdata[301].pitchers.pitching.innings,
                                        teamdata[301].pitchers.pitching.thirds));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.games_started, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.cg, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.gf, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.sho, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.saves, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.svopp, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.ibb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.wp, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.hb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.balks, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.bfp, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.doubles, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.triples, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.homers, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.rbi, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.sb, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.cs, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.sh, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.sf, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].pitchers.pitching.opp_ab, 'r'));
    strcat (&stats[0], (char *) do_average (teamdata[301].pitchers.pitching.hits, teamdata[301].pitchers.pitching.opp_ab));

    strcat (&stats[0], "\n\n\n");

    strcat (&stats[0], "Team Year & Name                                    G     PO      A      E     PB   FA\n\n");

    /* sort team data by fielding average */
    for (x = 0; x < (nteams - 1); x++)
        for (y = x + 1; y < nteams; y++) {
            fnum1 = ((float) teamdata[x].batters.fielding[0].po + (float) teamdata[x].batters.fielding[0].a) /
                    ((float) teamdata[x].batters.fielding[0].po + (float) teamdata[x].batters.fielding[0].a + (float) teamdata[x].batters.fielding[0].e);
            fnum2 = ((float) teamdata[y].batters.fielding[0].po + (float) teamdata[y].batters.fielding[0].a) /
                    ((float) teamdata[y].batters.fielding[0].po + (float) teamdata[y].batters.fielding[0].a + (float) teamdata[y].batters.fielding[0].e);

            if (fnum1 < fnum2) {
                teamdata[300] = teamdata[x];
                teamdata[x] = teamdata[y];
                teamdata[y] = teamdata[300];

                strcpy (&uctname[300][0], &uctname[x][0]);
                strcpy (&uctname[x][0], &uctname[y][0]);
                strcpy (&uctname[y][0], &uctname[300][0]);
            }
        }

    teamdata[301].batters.fielding[0].po = teamdata[301].batters.fielding[0].a = teamdata[301].batters.fielding[0].e =
      teamdata[301].batters.fielding[0].pb = 0;

    for (totg = x = 0; x < nteams; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].id)
                break;

        if (teamdata[x].year) {
            strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].year), 'l'));
            strcat (&stats[0], " ");
        }
        else
            strcat (&stats[0], "     ");
        if (teamdata[x].year) {
            strncat (&stats[0], &teaminfo[y].teamname[0], 25);
            if (strlen (&teaminfo[y].teamname[0]) < 25)
                strncat (&stats[0], "                            ", 25 - strlen (&teaminfo[y].teamname[0]));
        }
        else {
            strncat (&stats[0], &uctname[x][0], 25);
            if (strlen (&uctname[x][0]) < 25)
                strncat (&stats[0], "                            ", 25 - strlen (&uctname[x][0]));
        }

        strcat (&stats[0], "                ");
        if (urind == 'U')
            strcat (&stats[0], (char *) check_stats (teamdata[x].pitchers.pitching.wins + teamdata[x].pitchers.pitching.losses, 'r'));
        else
            /* real life data ... get total games played by the team from the Results data (wins + losses) */
            if (teamdata[x].year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                /* get the team name */
                strcpy (&tname[0], &teaminfo[y].teamname[0]);
                /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                if (!strncmp (&tname[0], "St ", 3)) {
                    strcpy (&work1[0], &tname[0]);
                    tname[2] = '.';
                    tname[3] = ' ';
                    strcpy (&tname[4], &work1[3]);
                }

                for (pib = 0; pib < strlen (&buffer[0]); pib++)
                    if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                        pib += strlen (&tname[0]);   /* move data pointer past team name */
                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                        /* get team wins */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        w = atoi (&work1[0]);

                        for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                        /* get team losses */
                        for (z = 0; buffer[pib] != ' '; z++, pib++)
                            work1[z] = buffer[pib];
                        work1[z] = '\0';
                        l = atoi (&work1[0]);

                        w += l;    /* total team games */
                        totg += w;     /* accumulate total games */
                        strcpy (&work1[0], (char *) cnvt_int2str ((w), 'l'));

                        strcat (&stats[0], "    ");
                        strcat (&stats[0], &work1[0]);
                        break;
                    }
            }
            else
                strcat (&stats[0], "       ");

        strcat (&stats[0], check_stats (teamdata[x].batters.fielding[0].po, 'r'));
        teamdata[301].batters.fielding[0].po += teamdata[x].batters.fielding[0].po;
        strcat (&stats[0], check_stats (teamdata[x].batters.fielding[0].a, 'r'));
        teamdata[301].batters.fielding[0].a += teamdata[x].batters.fielding[0].a;
        strcat (&stats[0], check_stats (teamdata[x].batters.fielding[0].e, 'r'));
        teamdata[301].batters.fielding[0].e += teamdata[x].batters.fielding[0].e;
        strcat (&stats[0], check_stats (teamdata[x].batters.fielding[0].pb, 'r'));
        teamdata[301].batters.fielding[0].pb += teamdata[x].batters.fielding[0].pb;
        strcat (&stats[0], (char *) do_average ((teamdata[x].batters.fielding[0].po + teamdata[x].batters.fielding[0].a),
            (teamdata[x].batters.fielding[0].po + teamdata[x].batters.fielding[0].a + teamdata[x].batters.fielding[0].e)));
        strcat (&stats[0], "\n");
    }
    strcat (&stats[0], "                 TOTALS");
    strcat (&stats[0], "                       ");
    if (urind == 'U')
        strcat (&stats[0], check_stats (teamdata[301].batters.hitting.games, 'r'));
    else
        if (teamdata[0].year != 1981) {
            totg /= 2;
            strcpy (&work1[0], (char *) cnvt_int2str ((totg), 'l'));

            switch (strlen (&work1[0])) {
                case 0:
                    strcat (&stats[0], "       ");
                    break;
                case 1:
                    strcat (&stats[0], "      ");
                    break;
                case 2:
                    strcat (&stats[0], "     ");
                    break;
                case 3:
                    strcat (&stats[0], "    ");
                    break;
                case 4:
                    strcat (&stats[0], "   ");
                    break;
                case 5:
                    strcat (&stats[0], "  ");
                    break;
                case 6:
                    strcat (&stats[0], " ");
                    break;
            }

            strcat (&stats[0], &work1[0]);
        }
        else
            strcat (&stats[0], "       ");

    strcat (&stats[0], check_stats (teamdata[301].batters.fielding[0].po, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.fielding[0].a, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.fielding[0].e, 'r'));
    strcat (&stats[0], check_stats (teamdata[301].batters.fielding[0].pb, 'r'));
    strcat (&stats[0], (char *) do_average ((teamdata[301].batters.fielding[0].po + teamdata[301].batters.fielding[0].a),
        (teamdata[301].batters.fielding[0].po + teamdata[301].batters.fielding[0].a + teamdata[301].batters.fielding[0].e)));

    strcat (&stats[0], "\n");
    gtk_text_buffer_insert_with_tags_by_name (buf, &pos, stats, -1, "monospace", NULL);
}

void
RealLifeTeamTotals (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    /* get a year from the user and send data to server */
    if (!getayrRLTTactive)
        /* only need to call this function if the "get a year" window isn't already active */
        GetAYear4RLTeamTotals ();
}

const gchar *entry_text;
gchar name[256];
GtkWidget *swin, *tentry;
gint papm, papd, papy;

void
RealLifeByPlayer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *tname, *table, *boxv, *tlabel;
    gint response;

    name[0] = '\0';

    swin = gtk_dialog_new_with_buttons ("Enter Player Name", GTK_WINDOW (mainwin),
           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
    g_signal_connect (G_OBJECT (swin), "destroy", G_CALLBACK (DestroyDialog), swin);
    gtk_container_set_border_width (GTK_CONTAINER (swin), 0);

    boxv = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), boxv, TRUE, TRUE, 0);

    tname = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxv), tname, TRUE, TRUE, 0);

    table = gtk_table_new (2, 2, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 0);
    gtk_table_set_col_spacings (GTK_TABLE (table), 0);
    gtk_box_pack_start (GTK_BOX (boxv), table, FALSE, FALSE, 0);
    tlabel = gtk_label_new_with_mnemonic ("Enter the Complete Player Name:");
    gtk_table_attach_defaults (GTK_TABLE (table), tlabel, 0, 1, 0, 1);

    tentry = gtk_entry_new ();
    gtk_signal_connect (GTK_OBJECT (tentry), "insert_text", GTK_SIGNAL_FUNC (CheckPName), NULL);
    if (strlen (&name[0]))
        gtk_entry_set_text (GTK_ENTRY (tentry), name);
    gtk_entry_set_max_length (GTK_ENTRY (tentry), 48);
    gtk_table_attach_defaults (GTK_TABLE (table), tentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (tlabel), tentry);

    gtk_dialog_set_default_response (GTK_DIALOG (swin), GTK_RESPONSE_OK);
    gtk_entry_set_activates_default (GTK_ENTRY (tentry), TRUE);

    gtk_widget_show_all (swin);
    gtk_window_set_keep_above (GTK_WINDOW (swin), TRUE);
GetPlayerName:
    response = gtk_dialog_run (GTK_DIALOG (swin));

    if (response == GTK_RESPONSE_CLOSE)
        DestroyDialog (swin, swin);
    else {
        gchar NoTName[256] = "You need to enter a name.\n\n", work[512],
              Error[256] = "An error was encountered when trying to retrieve the stats.",
              NoSpace[256] = "There needs to be at least 1 space character within the player name.",
              Unconnected[256] = "You need to connect to an NSB server.", *msg[5];
        gint x, fatalerr;

        fatalerr = 0;
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        entry_text = gtk_entry_get_text (GTK_ENTRY (tentry));
        strcpy (&name[0], entry_text);

        /* remove spaces at beginning and at end of name */
        while (name[0] == ' ')
            for (x = 0; x < strlen (&name[0]); x++)
                name[x] = name[x + 1];
        while (name[strlen (&name[0]) - 1] == ' ')
            name[strlen (&name[0]) - 1] = '\0';

        if (!strlen (&name[0])) {
            msg[fatalerr] = NoTName;
            fatalerr++;
        }
        if (!index (&name[0], ' ')) {
            msg[fatalerr] = NoSpace;
            fatalerr++;
        }
        if (fatalerr)
            outMessage (msg);
        else {
            strcpy (&work[0], "S9");
            strcat (&work[0], &name[0]);
            strcat (&work[0], "\n");

            sock_puts (sock, work);  /* tell the server */
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                if (!connected) {
                    strcpy (&work[0], "Not connected.\n");
                    Add2TextWindow (&work[0], 0);

                    msg[0] = &Unconnected[0];
                }
                else {
                    strcpy (&work[0], "Encountered error when talking to server ");
                    strcat (&work[0], &hs[0]);
                    strcat (&work[0], ".\n");
                    Add2TextWindow (&work[0], 1);

                    msg[0] = &Error[0];
                }

                outMessage (msg);
                DestroyDialog (swin, swin);
            }

            sock_gets (sock, &buffer[0], sizeof (buffer));
            if (strstr (&buffer[0], "MULT ")) {
                if (!PickAPlayer ())
                    strcpy (&work[0], "X\n");
                else
                    sprintf (work, "%.2d%.2d%.4d\n", papm, papd, papy);

                sock_puts (sock, work);  /* tell the server */
                if (work[0] == 'X')
                    /* user canceled */
                    goto GetPlayerName;

                sock_gets (sock, &buffer[0], sizeof (buffer));
            }

            if (!strstr (&buffer[0], "STATS")) {
                strcpy (&work[0], "No Stats for ");
                strcat (&work[0], &name[0]);
                strcat (&work[0], " on ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &work[0];

                outMessage (msg);
            }
            else
                ShowPlayerStats (&name[0]);
        }
        goto GetPlayerName;
    }
}

void
CheckPName (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    GtkEditable *editable = GTK_EDITABLE (entry);
    int i, count = 0;
    gchar *result = g_new (gchar, length);

    for (i = 0; i < length; i++) {
        if (!isalpha (text[i]) && !isblank (text[i]) && text[i] != '.' && text[i] != '\'' && text[i] != '-')
            continue;

        result[count++] = text[i];
    }

    if (count > 0) {
        gtk_signal_handler_block_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckPName), data);
        gtk_editable_insert_text (editable, result, count, position);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckPName), data);
    }
    gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");

    g_free (result);
}

GtkWidget *scatwindow = NULL;
extern GtkWidget *exentry, *inentry;

/* prepare to show category leaders or records */
void
CategoryLeaders (gint which) {
    GtkWidget *box1, *box2, *hbox, *table, *separator, *sw, *lab,
              *butbg, *butbab, *butbr, *butbh, *butb2b, *butb3b, *butbhr, *butbbi, *butbbb, *butbso, *butbhp, *butbdp,
              *butbsb, *butbcs, *butbibb, *butbs, *butbsf, *butbba, *butbsa, *butboba,
              *butpg, *butpgs, *butpip, *butpw, *butpl, *butps, *butpbfp, *butph, *butp2b, *butp3b, *butphr, *butpr,
              *butper, *butpbi, *butpcg, *butpgf, *butpsho, *butpsopp, *butpsb, *butpcs, *butpbb, *butpso, *butpibb,
              *butpsh, *butpsf, *butpwp, *butpb, *butphb, *butpab, *butpera, *butppct, *butpba,
              *butfog, *butfopo, *butfodp, *butfoa, *butfoe, *butfoavg,
              *butf1g, *butf1po, *butf1dp, *butf1a, *butf1e, *butf1avg,
              *butf2g, *butf2po, *butf2dp, *butf2a, *butf2e, *butf2avg,
              *butf3g, *butf3po, *butf3dp, *butf3a, *butf3e, *butf3avg,
              *butfsg, *butfspo, *butfsdp, *butfsa, *butfse, *butfsavg,
              *butfpg, *butfppo, *butfpdp, *butfpa, *butfpe, *butfpavg,
              *butfcg, *butfcpo, *butfcdp, *butfca, *butfce, *butfcavg, *butfcpb, *butfdhg,
              *dbutton;
    GtkLabel *hitlabel, *pitchlabel, *fieldlabel;
    gchar Unconnected[256] = "You need to connect to an NSB server.", *msg[5];
    gint x;

    whichstats = which;

    if (scatwindow != NULL)
        DestroyDialog (scatwindow, scatwindow);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        strcpy (&work[0], "Not connected.\n");
        Add2TextWindow (&work[0], 0);

        msg[0] = &Unconnected[0];
        outMessage (msg);
        return;
    }

    if (which == 1 || which == 7) {
        /* check for the existence of a season */
        gchar buf[256], NoLeague[256] = "You have no season.  First set up a season.", *msg[5];
        gint x;

        sock_puts (sock, "S71\n");
        sock_gets (sock, &buf[0], sizeof (buf));
        if (!strcmp (&buf[0], "NOLEAGUE")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "No season established on ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoLeague[0];
            outMessage (msg);

            return;
        }
    }

    if (which == 3) {
        /* check for the existence of a series */
        gchar buf[256], NoLeague[256] = "You have no series.  First set up a series.", *msg[5];
        gint x;

        sock_puts (sock, "S0\n");
        sock_gets (sock, &buf[0], sizeof (buf));
        if (!strcmp (&buf[0], "NOSERIES")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "No series established on ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoLeague[0];
            outMessage (msg);

            return;
        }
    }

    if (which == 5) {
        /* check for the existence of records */
        gchar buf[256], NoRecords[256] = "The records you're seeking do not exist.", *msg[5];
        gint x;

        if (wri == 1)
            sock_puts (sock, "S81\n");
        else
            sock_puts (sock, "S82\n");
        sock_gets (sock, &buf[0], sizeof (buf));
        if (!strcmp (&buf[0], "NORECORDS")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            if (wri == 1)
                strcpy (&work[0], "No records for this user on ");
            else
                strcpy (&work[0], "No records for all users on ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoRecords[0];
            outMessage (msg);

            return;
        }
    }

    scatwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (scatwindow), 840, 600);
    g_signal_connect (G_OBJECT (scatwindow), "destroy", G_CALLBACK (CBscatDestroyDialog), NULL);

    if (which == 1)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Current NSB Season Stat Categories");
    if (which == 2)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Real Life Stat Categories");
    if (which == 3)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Current Series Stat Categories");
    if (which == 5)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Regular Season Records Stat Categories");
    if (which == 6)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Lifetime Regular Season Stat Categories");
    if (which == 7)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Current NSB Season Post-Season Stat Categories");
    if (which == 8)
        gtk_window_set_title (GTK_WINDOW (scatwindow), "Lifetime Post-Season Stat Categories");
    gtk_container_set_border_width (GTK_CONTAINER (scatwindow), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (scatwindow), box1);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    if (whichstats == 2) {
        sock_puts (sock, "S62\n");  /* tell the server to send us the Real Life years available */

        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
                strcpy (&work[0], "Encountered error when talking to server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &Error[0];
            }

            outMessage (msg);
            return;
        }

        if (!strncmp (&buffer[0], "-2", 2)) {
            GotError ();
            return;
        }

        if (strlen (&buffer[0]) < 10) {
            /* if there is only one year available don't bother asking the user for a year */
            strncpy (&yeara[0], &buffer[2], 4);
            yeara[4] = '\0';
        }
        else {
            /* get a year from the user */
            GtkWidget *label, *box3, *table;

            box3 = gtk_hbox_new (FALSE, 10);
            gtk_container_set_border_width (GTK_CONTAINER (box3), 10);
            gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);

            table = gtk_table_new (2, 2, FALSE);
            gtk_table_set_row_spacings (GTK_TABLE (table), 4);
            gtk_table_set_col_spacings (GTK_TABLE (table), 4);
            gtk_box_pack_start (GTK_BOX (box3), table, TRUE, TRUE, 0);
            label = gtk_label_new_with_mnemonic ("Exclude Year(s):");
            gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
            label = gtk_label_new_with_mnemonic ("Include Year(s):");
            gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

            exentry = gtk_entry_new ();
            gtk_entry_set_max_length (GTK_ENTRY (exentry), 100);
            gtk_table_attach_defaults (GTK_TABLE (table), exentry, 1, 2, 0, 1);
            gtk_label_set_mnemonic_widget (GTK_LABEL (label), exentry);
            gtk_entry_set_text (GTK_ENTRY (exentry), "");
            gtk_signal_connect (GTK_OBJECT (exentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntry), NULL);

            inentry = gtk_entry_new ();
            gtk_entry_set_max_length (GTK_ENTRY (inentry), 100);
            gtk_table_attach_defaults (GTK_TABLE (table), inentry, 1, 2, 1, 2);
            gtk_label_set_mnemonic_widget (GTK_LABEL (label), inentry);
            gtk_entry_set_text (GTK_ENTRY (inentry), "2017");
            gtk_signal_connect (GTK_OBJECT (inentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntry), NULL);
            gtk_window_set_focus (GTK_WINDOW (scatwindow), inentry);

            label = gtk_label_new ("Separate multiple years with either a space or a hyphen.");
            gtk_container_add (GTK_CONTAINER (box3), label);
        }

        lab = gtk_label_new ("NOTE - Some stats are not available for all years.");
        gtk_box_pack_start (GTK_BOX (box2), lab, FALSE, FALSE, 0);
    }

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (box2), sw);

    table = gtk_table_new (33, 5, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);
    gtk_table_set_col_spacings (GTK_TABLE (table), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), table);

    /* insert the column headers */
    hitlabel = g_object_new (GTK_TYPE_LABEL, "label", "Hitting Categories:", NULL);
    pitchlabel = g_object_new (GTK_TYPE_LABEL, "label", "Pitching Categories:", NULL);
    fieldlabel = g_object_new (GTK_TYPE_LABEL, "label", "Fielding Categories:", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (hitlabel), 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (pitchlabel), 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (fieldlabel), 2, 3, 0, 1);

    /* insert the buttons */
    butbg = gtk_button_new_with_label ("Games Played");
    butbab = gtk_button_new_with_label ("At Bats");
    butbr = gtk_button_new_with_label ("Runs Scored");
    butbh = gtk_button_new_with_label ("Hits");
    butb2b = gtk_button_new_with_label ("Doubles");
    butb3b = gtk_button_new_with_label ("Triples");
    butbhr = gtk_button_new_with_label ("Home Runs");
    butbbi = gtk_button_new_with_label ("Runs Batted In");
    butbbb = gtk_button_new_with_label ("Bases on Balls");
    butbso = gtk_button_new_with_label ("Strike Outs");
    butbhp = gtk_button_new_with_label ("Hit by Pitcher");
    butbdp = gtk_button_new_with_label ("Double Plays");
    butbsb = gtk_button_new_with_label ("Stolen Bases");
    butbcs = gtk_button_new_with_label ("Caught Stealing");
    butbibb = gtk_button_new_with_label ("Intentional Bases on Balls");
    butbs = gtk_button_new_with_label ("Sacrifices");
    butbsf = gtk_button_new_with_label ("Sacrifice Flies");
    butbba = gtk_button_new_with_label ("Batting Average");
    butbsa = gtk_button_new_with_label ("Slugging Average");
    butboba = gtk_button_new_with_label ("On-Base Percentage");

    butpg = gtk_button_new_with_label ("Games Pitched");
    butpgs = gtk_button_new_with_label ("Games Started");
    butpip = gtk_button_new_with_label ("Innings Pitched");
    butpw = gtk_button_new_with_label ("Wins");
    butpl = gtk_button_new_with_label ("Losses");
    butps = gtk_button_new_with_label ("Saves");
    butpbfp = gtk_button_new_with_label ("Batters Facing Pitcher");
    butph = gtk_button_new_with_label ("Hits");
    butp2b = gtk_button_new_with_label ("Doubles");
    butp3b = gtk_button_new_with_label ("Triples");
    butphr = gtk_button_new_with_label ("Home Runs");
    butpr = gtk_button_new_with_label ("Runs");
    butper = gtk_button_new_with_label ("Earned Runs");
    butpbi = gtk_button_new_with_label ("Runs Batted In");
    butpcg = gtk_button_new_with_label ("Complete Games");
    butpgf = gtk_button_new_with_label ("Games Finished");
    butpsho = gtk_button_new_with_label ("Shutouts");
    butpsopp = gtk_button_new_with_label ("Save Opportunities");
    butpsb = gtk_button_new_with_label ("Stolen Bases");
    butpcs = gtk_button_new_with_label ("Caught Stealing");
    butpbb = gtk_button_new_with_label ("Bases on Balls");
    butpso = gtk_button_new_with_label ("Strike Outs");
    butpibb = gtk_button_new_with_label ("Intentional Bases on Balls");
    butpsh = gtk_button_new_with_label ("Sacrifices");
    butpsf = gtk_button_new_with_label ("Sacrifice Flies");
    butpwp = gtk_button_new_with_label ("Wild Pitches");
    butpb = gtk_button_new_with_label ("Balks");
    butphb = gtk_button_new_with_label ("Hit Batters");
    butpab = gtk_button_new_with_label ("At Bats");
    butpera = gtk_button_new_with_label ("Earned Run Average");
    butppct = gtk_button_new_with_label ("Winning Percentage");
    butpba = gtk_button_new_with_label ("Batting Average Against");

    butfog = gtk_button_new_with_label ("Outfield - Games Played");
    butfopo = gtk_button_new_with_label ("Outfield - Put Outs");
    butfodp = gtk_button_new_with_label ("Outfield - Double Plays");
    butfoa = gtk_button_new_with_label ("Outfield - Assists");
    butfoe = gtk_button_new_with_label ("Outfield - Errors");
    butfoavg = gtk_button_new_with_label ("Outfield - Fielding Average");
    butf1g = gtk_button_new_with_label ("First Base - Games Played");
    butf1po = gtk_button_new_with_label ("First Base - Put Outs");
    butf1dp = gtk_button_new_with_label ("First Base - Double Plays");
    butf1a = gtk_button_new_with_label ("First Base - Assists");
    butf1e = gtk_button_new_with_label ("First Base - Errors");
    butf1avg = gtk_button_new_with_label ("First Base - Fielding Average");
    butf2g = gtk_button_new_with_label ("Second Base - Games Played");
    butf2po = gtk_button_new_with_label ("Second Base - Put Outs");
    butf2dp = gtk_button_new_with_label ("Second Base - Double Plays");
    butf2a = gtk_button_new_with_label ("Second Base - Assists");
    butf2e = gtk_button_new_with_label ("Second Base - Errors");
    butf2avg = gtk_button_new_with_label ("Second Base - Fielding Average");
    butf3g = gtk_button_new_with_label ("Third Base - Games Played");
    butf3po = gtk_button_new_with_label ("Third Base - Put Outs");
    butf3dp = gtk_button_new_with_label ("Third Base - Double Plays");
    butf3a = gtk_button_new_with_label ("Third Base - Assists");
    butf3e = gtk_button_new_with_label ("Third Base - Errors");
    butf3avg = gtk_button_new_with_label ("Third Base - Fielding Average");
    butfsg = gtk_button_new_with_label ("Shortstop - Games Played");
    butfspo = gtk_button_new_with_label ("Shortstop - Put Outs");
    butfsdp = gtk_button_new_with_label ("Shortstop - Double Plays");
    butfsa = gtk_button_new_with_label ("Shortstop - Assists");
    butfse = gtk_button_new_with_label ("Shortstop - Errors");
    butfsavg = gtk_button_new_with_label ("Shortstop - Fielding Average");
    butfpg = gtk_button_new_with_label ("Pitcher - Games Played");
    butfppo = gtk_button_new_with_label ("Pitcher - Put Outs");
    butfpdp = gtk_button_new_with_label ("Pitcher - Double Plays");
    butfpa = gtk_button_new_with_label ("Pitcher - Assists");
    butfpe = gtk_button_new_with_label ("Pitcher - Errors");
    butfpavg = gtk_button_new_with_label ("Pitcher - Fielding Average");
    butfcg = gtk_button_new_with_label ("Catcher - Games Played");
    butfcpo = gtk_button_new_with_label ("Catcher - Put Outs");
    butfcdp = gtk_button_new_with_label ("Catcher - Double Plays");
    butfca = gtk_button_new_with_label ("Catcher - Assists");
    butfce = gtk_button_new_with_label ("Catcher - Errors");
    butfcavg = gtk_button_new_with_label ("Catcher - Fielding Average");
    butfcpb = gtk_button_new_with_label ("Catcher - Passed Balls");
    butfdhg = gtk_button_new_with_label ("Designated Hitter - Games Played");

    g_signal_connect (G_OBJECT (butbg), "clicked", G_CALLBACK (CBbutbg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbg), 0, 1, 1, 2);
    g_signal_connect (G_OBJECT (butbab), "clicked", G_CALLBACK (CBbutbab), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbab), 0, 1, 2, 3);
    g_signal_connect (G_OBJECT (butbr), "clicked", G_CALLBACK (CBbutbr), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbr), 0, 1, 3, 4);
    g_signal_connect (G_OBJECT (butbh), "clicked", G_CALLBACK (CBbutbh), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbh), 0, 1, 4, 5);
    g_signal_connect (G_OBJECT (butb2b), "clicked", G_CALLBACK (CBbutb2b), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butb2b), 0, 1, 5, 6);
    g_signal_connect (G_OBJECT (butb3b), "clicked", G_CALLBACK (CBbutb3b), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butb3b), 0, 1, 6, 7);
    g_signal_connect (G_OBJECT (butbhr), "clicked", G_CALLBACK (CBbutbhr), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbhr), 0, 1, 7, 8);
    g_signal_connect (G_OBJECT (butbbi), "clicked", G_CALLBACK (CBbutbbi), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbbi), 0, 1, 8, 9);
    g_signal_connect (G_OBJECT (butbbb), "clicked", G_CALLBACK (CBbutbbb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbbb), 0, 1, 9, 10);
    g_signal_connect (G_OBJECT (butbso), "clicked", G_CALLBACK (CBbutbso), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbso), 0, 1, 10, 11);
    g_signal_connect (G_OBJECT (butbhp), "clicked", G_CALLBACK (CBbutbhp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbhp), 0, 1, 11, 12);
    g_signal_connect (G_OBJECT (butbdp), "clicked", G_CALLBACK (CBbutbdp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbdp), 0, 1, 12, 13);
    g_signal_connect (G_OBJECT (butbsb), "clicked", G_CALLBACK (CBbutbsb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbsb), 0, 1, 13, 14);
    g_signal_connect (G_OBJECT (butbcs), "clicked", G_CALLBACK (CBbutbcs), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbcs), 0, 1, 14, 15);
    g_signal_connect (G_OBJECT (butbibb), "clicked", G_CALLBACK (CBbutbibb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbibb), 0, 1, 15, 16);
    g_signal_connect (G_OBJECT (butbs), "clicked", G_CALLBACK (CBbutbs), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbs), 0, 1, 16, 17);
    g_signal_connect (G_OBJECT (butbsf), "clicked", G_CALLBACK (CBbutbsf), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbsf), 0, 1, 17, 18);
    g_signal_connect (G_OBJECT (butbba), "clicked", G_CALLBACK (CBbutbba), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbba), 0, 1, 18, 19);
    g_signal_connect (G_OBJECT (butbsa), "clicked", G_CALLBACK (CBbutbsa), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butbsa), 0, 1, 19, 20);
    g_signal_connect (G_OBJECT (butboba), "clicked", G_CALLBACK (CBbutboba), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butboba), 0, 1, 20, 21);

    g_signal_connect (G_OBJECT (butpg), "clicked", G_CALLBACK (CBbutpg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpg), 1, 2, 1, 2);
    g_signal_connect (G_OBJECT (butpgs), "clicked", G_CALLBACK (CBbutpgs), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpgs), 1, 2, 2, 3);
    g_signal_connect (G_OBJECT (butpip), "clicked", G_CALLBACK (CBbutpip), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpip), 1, 2, 3, 4);
    g_signal_connect (G_OBJECT (butpw), "clicked", G_CALLBACK (CBbutpw), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpw), 1, 2, 4, 5);
    g_signal_connect (G_OBJECT (butpl), "clicked", G_CALLBACK (CBbutpl), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpl), 1, 2, 5, 6);
    g_signal_connect (G_OBJECT (butps), "clicked", G_CALLBACK (CBbutps), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butps), 1, 2, 6, 7);
    g_signal_connect (G_OBJECT (butpbfp), "clicked", G_CALLBACK (CBbutpbfp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpbfp), 1, 2, 7, 8);
    g_signal_connect (G_OBJECT (butph), "clicked", G_CALLBACK (CBbutph), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butph), 1, 2, 8, 9);
    g_signal_connect (G_OBJECT (butp2b), "clicked", G_CALLBACK (CBbutp2b), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butp2b), 1, 2, 9, 10);
    g_signal_connect (G_OBJECT (butp3b), "clicked", G_CALLBACK (CBbutp3b), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butp3b), 1, 2, 10, 11);
    g_signal_connect (G_OBJECT (butphr), "clicked", G_CALLBACK (CBbutphr), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butphr), 1, 2, 11, 12);
    g_signal_connect (G_OBJECT (butpr), "clicked", G_CALLBACK (CBbutpr), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpr), 1, 2, 12, 13);
    g_signal_connect (G_OBJECT (butper), "clicked", G_CALLBACK (CBbutper), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butper), 1, 2, 13, 14);
    g_signal_connect (G_OBJECT (butpbi), "clicked", G_CALLBACK (CBbutpbi), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpbi), 1, 2, 14, 15);
    g_signal_connect (G_OBJECT (butpcg), "clicked", G_CALLBACK (CBbutpcg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpcg), 1, 2, 15, 16);
    g_signal_connect (G_OBJECT (butpgf), "clicked", G_CALLBACK (CBbutpgf), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpgf), 1, 2, 16, 17);
    g_signal_connect (G_OBJECT (butpsho), "clicked", G_CALLBACK (CBbutpsho), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpsho), 1, 2, 17, 18);
    g_signal_connect (G_OBJECT (butpsopp), "clicked", G_CALLBACK (CBbutpsopp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpsopp), 1, 2, 18, 19);
    g_signal_connect (G_OBJECT (butpsb), "clicked", G_CALLBACK (CBbutpsb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpsb), 1, 2, 19, 20);
    g_signal_connect (G_OBJECT (butpcs), "clicked", G_CALLBACK (CBbutpcs), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpcs), 1, 2, 20, 21);
    g_signal_connect (G_OBJECT (butpbb), "clicked", G_CALLBACK (CBbutpbb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpbb), 1, 2, 21, 22);
    g_signal_connect (G_OBJECT (butpso), "clicked", G_CALLBACK (CBbutpso), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpso), 1, 2, 22, 23);
    g_signal_connect (G_OBJECT (butpibb), "clicked", G_CALLBACK (CBbutpibb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpibb), 1, 2, 23, 24);
    g_signal_connect (G_OBJECT (butpsh), "clicked", G_CALLBACK (CBbutpsh), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpsh), 1, 2, 24, 25);
    g_signal_connect (G_OBJECT (butpsf), "clicked", G_CALLBACK (CBbutpsf), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpsf), 1, 2, 25, 26);
    g_signal_connect (G_OBJECT (butpwp), "clicked", G_CALLBACK (CBbutpwp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpwp), 1, 2, 26, 27);
    g_signal_connect (G_OBJECT (butpb), "clicked", G_CALLBACK (CBbutpb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpb), 1, 2, 27, 28);
    g_signal_connect (G_OBJECT (butphb), "clicked", G_CALLBACK (CBbutphb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butphb), 1, 2, 28, 29);
    g_signal_connect (G_OBJECT (butpab), "clicked", G_CALLBACK (CBbutpab), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpab), 1, 2, 29, 30);
    g_signal_connect (G_OBJECT (butpera), "clicked", G_CALLBACK (CBbutpera), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpera), 1, 2, 30, 31);
    g_signal_connect (G_OBJECT (butppct), "clicked", G_CALLBACK (CBbutppct), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butppct), 1, 2, 31, 32);
    g_signal_connect (G_OBJECT (butpba), "clicked", G_CALLBACK (CBbutpba), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butpba), 1, 2, 32, 33);

    g_signal_connect (G_OBJECT (butfog), "clicked", G_CALLBACK (CBbutfog), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfog), 2, 3, 1, 2);
    g_signal_connect (G_OBJECT (butfopo), "clicked", G_CALLBACK (CBbutfopo), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfopo), 2, 3, 2, 3);
    g_signal_connect (G_OBJECT (butfodp), "clicked", G_CALLBACK (CBbutfodp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfodp), 2, 3, 3, 4);
    g_signal_connect (G_OBJECT (butfoa), "clicked", G_CALLBACK (CBbutfoa), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfoa), 2, 3, 4, 5);
    g_signal_connect (G_OBJECT (butfoe), "clicked", G_CALLBACK (CBbutfoe), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfoe), 2, 3, 5, 6);
    g_signal_connect (G_OBJECT (butfoavg), "clicked", G_CALLBACK (CBbutfoavg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfoavg), 2, 3, 6, 7);
    g_signal_connect (G_OBJECT (butf1g), "clicked", G_CALLBACK (CBbutf1g), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1g), 2, 3, 8, 9);
    g_signal_connect (G_OBJECT (butf1po), "clicked", G_CALLBACK (CBbutf1po), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1po), 2, 3, 9, 10);
    g_signal_connect (G_OBJECT (butf1dp), "clicked", G_CALLBACK (CBbutf1dp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1dp), 2, 3, 10, 11);
    g_signal_connect (G_OBJECT (butf1a), "clicked", G_CALLBACK (CBbutf1a), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1a), 2, 3, 11, 12);
    g_signal_connect (G_OBJECT (butf1e), "clicked", G_CALLBACK (CBbutf1e), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1e), 2, 3, 12, 13);
    g_signal_connect (G_OBJECT (butf1avg), "clicked", G_CALLBACK (CBbutf1avg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf1avg), 2, 3, 13, 14);
    g_signal_connect (G_OBJECT (butf2g), "clicked", G_CALLBACK (CBbutf2g), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2g), 2, 3, 15, 16);
    g_signal_connect (G_OBJECT (butf2po), "clicked", G_CALLBACK (CBbutf2po), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2po), 2, 3, 16, 17);
    g_signal_connect (G_OBJECT (butf2dp), "clicked", G_CALLBACK (CBbutf2dp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2dp), 2, 3, 17, 18);
    g_signal_connect (G_OBJECT (butf2a), "clicked", G_CALLBACK (CBbutf2a), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2a), 2, 3, 18, 19);
    g_signal_connect (G_OBJECT (butf2e), "clicked", G_CALLBACK (CBbutf2e), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2e), 2, 3, 19, 20);
    g_signal_connect (G_OBJECT (butf2avg), "clicked", G_CALLBACK (CBbutf2avg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf2avg), 2, 3, 20, 21);
    g_signal_connect (G_OBJECT (butf3g), "clicked", G_CALLBACK (CBbutf3g), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3g), 2, 3, 22, 23);
    g_signal_connect (G_OBJECT (butf3po), "clicked", G_CALLBACK (CBbutf3po), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3po), 2, 3, 23, 24);
    g_signal_connect (G_OBJECT (butf3dp), "clicked", G_CALLBACK (CBbutf3dp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3dp), 2, 3, 24, 25);
    g_signal_connect (G_OBJECT (butf3a), "clicked", G_CALLBACK (CBbutf3a), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3a), 2, 3, 25, 26);
    g_signal_connect (G_OBJECT (butf3e), "clicked", G_CALLBACK (CBbutf3e), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3e), 2, 3, 26, 27);
    g_signal_connect (G_OBJECT (butf3avg), "clicked", G_CALLBACK (CBbutf3avg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butf3avg), 2, 3, 27, 28);

    g_signal_connect (G_OBJECT (butfsg), "clicked", G_CALLBACK (CBbutfsg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfsg), 3, 4, 1, 2);
    g_signal_connect (G_OBJECT (butfspo), "clicked", G_CALLBACK (CBbutfspo), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfspo), 3, 4, 2, 3);
    g_signal_connect (G_OBJECT (butfsdp), "clicked", G_CALLBACK (CBbutfsdp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfsdp), 3, 4, 3, 4);
    g_signal_connect (G_OBJECT (butfsa), "clicked", G_CALLBACK (CBbutfsa), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfsa), 3, 4, 4, 5);
    g_signal_connect (G_OBJECT (butfse), "clicked", G_CALLBACK (CBbutfse), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfse), 3, 4, 5, 6);
    g_signal_connect (G_OBJECT (butfsavg), "clicked", G_CALLBACK (CBbutfsavg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfsavg), 3, 4, 6, 7);

    g_signal_connect (G_OBJECT (butfpg), "clicked", G_CALLBACK (CBbutfpg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfpg), 3, 4, 8, 9);
    g_signal_connect (G_OBJECT (butfppo), "clicked", G_CALLBACK (CBbutfppo), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfppo), 3, 4, 9, 10);
    g_signal_connect (G_OBJECT (butfpdp), "clicked", G_CALLBACK (CBbutfpdp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfpdp), 3, 4, 10, 11);
    g_signal_connect (G_OBJECT (butfpa), "clicked", G_CALLBACK (CBbutfpa), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfpa), 3, 4, 11, 12);
    g_signal_connect (G_OBJECT (butfpe), "clicked", G_CALLBACK (CBbutfpe), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfpe), 3, 4, 12, 13);
    g_signal_connect (G_OBJECT (butfpavg), "clicked", G_CALLBACK (CBbutfpavg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfpavg), 3, 4, 13, 14);

    g_signal_connect (G_OBJECT (butfcg), "clicked", G_CALLBACK (CBbutfcg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfcg), 3, 4, 15, 16);
    g_signal_connect (G_OBJECT (butfcpo), "clicked", G_CALLBACK (CBbutfcpo), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfcpo), 3, 4, 16, 17);
    g_signal_connect (G_OBJECT (butfcdp), "clicked", G_CALLBACK (CBbutfcdp), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfcdp), 3, 4, 17, 18);
    g_signal_connect (G_OBJECT (butfca), "clicked", G_CALLBACK (CBbutfca), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfca), 3, 4, 18, 19);
    g_signal_connect (G_OBJECT (butfce), "clicked", G_CALLBACK (CBbutfce), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfce), 3, 4, 19, 20);
    g_signal_connect (G_OBJECT (butfcpb), "clicked", G_CALLBACK (CBbutfcpb), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfcpb), 3, 4, 20, 21);
    g_signal_connect (G_OBJECT (butfcavg), "clicked", G_CALLBACK (CBbutfcavg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfcavg), 3, 4, 21, 22);

    g_signal_connect (G_OBJECT (butfdhg), "clicked", G_CALLBACK (CBbutfdhg), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (butfdhg), 3, 4, 23, 24);

    if (which == 5 && sgi == 1 && (wri == 1 || wri == 2)) {
        gtk_widget_set_sensitive (GTK_WIDGET (butbg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butbba), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butbsa), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butboba), FALSE);

        gtk_widget_set_sensitive (GTK_WIDGET (butpg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpgs), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpw), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpl), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butps), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpcg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpgf), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpsho), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpsopp), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpera), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butppct), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butpba), FALSE);

        gtk_widget_set_sensitive (GTK_WIDGET (butfog), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfoavg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf1g), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf1avg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf2g), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf2avg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf3g), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butf3avg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfsg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfsavg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfpg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfpavg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfcg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfcavg), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (butfdhg), FALSE);
    }

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    dbutton = gtk_button_new_with_label ("DISMISS");
    g_signal_connect (G_OBJECT (dbutton), "clicked", G_CALLBACK (CBscatDestroyDialog), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), dbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (scatwindow);
}

void
CBscatDestroyDialog (GtkWidget *widget, gpointer *pdata) {
    DestroyDialog (scatwindow, scatwindow);
    scatwindow = NULL;
}

void
CBbutbg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "11 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbab (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "12 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbr (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "13 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbh (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "14 00\n");
    ShowCategoryLeaders ();
}

void
CBbutb2b (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "15 00\n");
    ShowCategoryLeaders ();
}

void
CBbutb3b (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "16 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbhr (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "17 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbbi (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "18 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbbb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "19 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbso (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1a 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbhp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1b 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbdp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1c 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbsb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1d 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbcs (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1e 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbibb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1f 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbs (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1g 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbsf (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1h 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbba (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1i 00\n");
    ShowCategoryLeaders ();
}

void
CBbutbsa (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1j 00\n");
    ShowCategoryLeaders ();
}

void
CBbutboba (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "1k 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "21 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpgs (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "22 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpip (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "23 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpw (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "24 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpl (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "25 00\n");
    ShowCategoryLeaders ();
}

void
CBbutps (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "26 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpbfp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "27 00\n");
    ShowCategoryLeaders ();
}

void
CBbutph (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "28 00\n");
    ShowCategoryLeaders ();
}

void
CBbutp2b (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "29 00\n");
    ShowCategoryLeaders ();
}

void
CBbutp3b (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2a 00\n");
    ShowCategoryLeaders ();
}

void
CBbutphr (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2b 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpr (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2c 00\n");
    ShowCategoryLeaders ();
}

void
CBbutper (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2d 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpbi (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2e 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpcg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2f 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpgf (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2g 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpsho (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2h 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpsopp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2i 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpsb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2j 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpcs (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2k 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpbb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2l 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpso (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2m 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpibb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2n 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpsh (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2o 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpsf (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2p 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpwp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2q 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2r 00\n");
    ShowCategoryLeaders ();
}

void
CBbutphb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2s 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpab (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2t 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpera (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2u 00\n");
    ShowCategoryLeaders ();
}

void
CBbutppct (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2v 00\n");
    ShowCategoryLeaders ();
}

void
CBbutpba (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "2w 00\n");
    ShowCategoryLeaders ();
}

void
CBbutfog (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31100\n");
    ShowCategoryLeaders ();
}

void
CBbutfopo (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32100\n");
    ShowCategoryLeaders ();
}

void
CBbutfodp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33100\n");
    ShowCategoryLeaders ();
}

void
CBbutfoa (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34100\n");
    ShowCategoryLeaders ();
}

void
CBbutfoe (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36100\n");
    ShowCategoryLeaders ();
}

void
CBbutfoavg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37100\n");
    ShowCategoryLeaders ();
}

void
CBbutf1g (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31200\n");
    ShowCategoryLeaders ();
}

void
CBbutf1po (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32200\n");
    ShowCategoryLeaders ();
}

void
CBbutf1dp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33200\n");
    ShowCategoryLeaders ();
}

void
CBbutf1a (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34200\n");
    ShowCategoryLeaders ();
}

void
CBbutf1e (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36200\n");
    ShowCategoryLeaders ();
}

void
CBbutf1avg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37200\n");
    ShowCategoryLeaders ();
}

void
CBbutf2g (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31300\n");
    ShowCategoryLeaders ();
}

void
CBbutf2po (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32300\n");
    ShowCategoryLeaders ();
}

void
CBbutf2dp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33300\n");
    ShowCategoryLeaders ();
}

void
CBbutf2a (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34300\n");
    ShowCategoryLeaders ();
}

void
CBbutf2e (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36300\n");
    ShowCategoryLeaders ();
}

void
CBbutf2avg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37300\n");
    ShowCategoryLeaders ();
}

void
CBbutf3g (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31400\n");
    ShowCategoryLeaders ();
}

void
CBbutf3po (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32400\n");
    ShowCategoryLeaders ();
}

void
CBbutf3dp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33400\n");
    ShowCategoryLeaders ();
}

void
CBbutf3a (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34400\n");
    ShowCategoryLeaders ();
}

void
CBbutf3e (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36400\n");
    ShowCategoryLeaders ();
}

void
CBbutf3avg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37400\n");
    ShowCategoryLeaders ();
}

void
CBbutfsg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31500\n");
    ShowCategoryLeaders ();
}

void
CBbutfspo (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32500\n");
    ShowCategoryLeaders ();
}

void
CBbutfsdp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33500\n");
    ShowCategoryLeaders ();
}

void
CBbutfsa (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34500\n");
    ShowCategoryLeaders ();
}

void
CBbutfse (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36500\n");
    ShowCategoryLeaders ();
}

void
CBbutfsavg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37500\n");
    ShowCategoryLeaders ();
}

void
CBbutfpg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31600\n");
    ShowCategoryLeaders ();
}

void
CBbutfppo (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32600\n");
    ShowCategoryLeaders ();
}

void
CBbutfpdp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33600\n");
    ShowCategoryLeaders ();
}

void
CBbutfpa (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34600\n");
    ShowCategoryLeaders ();
}

void
CBbutfpe (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36600\n");
    ShowCategoryLeaders ();
}

void
CBbutfpavg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37600\n");
    ShowCategoryLeaders ();
}

void
CBbutfcg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31700\n");
    ShowCategoryLeaders ();
}

void
CBbutfcpo (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "32700\n");
    ShowCategoryLeaders ();
}

void
CBbutfcdp (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "33700\n");
    ShowCategoryLeaders ();
}

void
CBbutfca (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "34700\n");
    ShowCategoryLeaders ();
}

void
CBbutfce (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "36700\n");
    ShowCategoryLeaders ();
}

void
CBbutfcavg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "37700\n");
    ShowCategoryLeaders ();
}

void
CBbutfcpb (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "35700\n");
    ShowCategoryLeaders ();
}

void
CBbutfdhg (GtkWidget *widget, gpointer *pdata) {
    strcpy (&statscmdstr[0], "31800\n");
    ShowCategoryLeaders ();
}

extern gint yrs[];
extern gchar *inentry_text, *exentry_text;

/* show stats category leaders */
void
ShowCategoryLeaders () {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table,
              *vscrollbar, *text;
    GdkFont *fixed_font;
    gint x;
    gchar w[300];

    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;
    statscmdstr[3] = sgi + '0';
    statscmdstr[4] = wri + '0';

    strcpy (&work[0], "S");
    if (whichstats == 2)
        strcat (&work[0], "2");  /* Real Life category leaders */
    if (whichstats == 1 || whichstats == 3)
        strcat (&work[0], "4");  /* Current NSB Season category leaders */
    if (whichstats == 5)
        strcat (&work[0], "5");  /* records */
    if (whichstats == 6)
        strcat (&work[0], "a");  /* Lifetime regular season category leaders */
    if (whichstats == 7)
        strcat (&work[0], "b");  /* Current season post-season category leaders */
    if (whichstats == 8)
        strcat (&work[0], "c");  /* Lifetime post-season category leaders */

    w[0] = '\0';
    if (whichstats == 2) {
        gint x, y, err;
        gchar *msg[5], NoYrs[256] = "No years to search.", NoInYears[256] = "\nThe years to include is empty.  Assume all years (1901-2017)?\n\n";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        inentry_text = (char *) gtk_entry_get_text (GTK_ENTRY (inentry));

        if (!strlen (&inentry_text[0]) && !preferences.AssumeAllYears) {
            msg[0] = &NoInYears[0];
            if (!ShallWeContinue (msg))
                return;
        }

        exentry_text = (char *) gtk_entry_get_text (GTK_ENTRY (exentry));

        err = ValidateText ();
        if (err) {
            gint errct;
            gchar ExMsg[256] = "In excluded years ...", InMsg[256] = "In included years ...",
                  YrAfterHy[256] = "The year after a hyphen must be equal to or greater than the year before that same hyphen.",
                  NoYr[256] = "There must be a year after a hyphen.", InitYr[256] = "A year must be the first data.",
                  InvYr[256] = "A year is not valid (must be 4 positions and must be 1901-2017).";

            /* first four bits (leftmost) inentry, second four bits (rightmost) exentry
               0000 0000
                  |    |
                  InvYr
                 |    |
                 InitYr
                |    |
                NoYr
               |    |
               YrAfterHy
            */

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            if (err >= 16) {
                msg[0] = &InMsg[0];
                errct = 1;
                if ((err & 0x80) == 0x80) {
                    msg[errct] = &YrAfterHy[0];
                    errct++;
                }
                if ((err & 0x40) == 0x40) {
                    msg[errct] = &NoYr[0];
                    errct++;
                }
                if ((err & 0x20) == 0x20) {
                    msg[errct] = &InitYr[0];
                    errct++;
                }
                if ((err & 0x10) == 0x10)
                    msg[errct] = &InvYr[0];

                outMessage (msg);
            }

            err &= 15;
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            if (err) {
                msg[0] = &ExMsg[0];
                errct = 1;
                if ((err & 0x8) == 0x8) {
                    msg[errct] = &YrAfterHy[0];
                    errct++;
                }
                if ((err & 0x4) == 0x4) {
                    msg[errct] = &NoYr[0];
                    errct++;
                }
                if ((err & 0x2) == 0x2) {
                    msg[errct] = &InitYr[0];
                    errct++;
                }
                if ((err & 0x1) == 0x1)
                    msg[errct] = &InvYr[0];

                outMessage (msg);
            }
            return;
        }

        for (x = 0; x < YEAR_SPREAD; x++)
            if (yrs[x])
                break;
        if (x == YEAR_SPREAD) {
            msg[0] = &NoYrs[0];
            outMessage (msg);
            return;
        }

        /* find if multiple years have been selected */
        for (y = x = 0; x < YEAR_SPREAD; x++)
            if (yrs[x])
                y++;
        if (y > 1)
            RLCatLeadersMY = 1;
        else
            RLCatLeadersMY = 0;

        for (x = 0; x < YEAR_SPREAD; x++)
            strcat (&w[0], cnvt_int2str (yrs[x], 'l'));
    }

    if (whichstats == 2)
        strcat (&work[0], "0000");
    else
        strcat (&work[0], &yeara[0]);
    strcat (&work[0], statscmdstr);   /* statscmdstr contains a \n in the last position */
    if (whichstats == 2) {
        work[strlen (&work[0]) - 1] = '\0';     /* get rid of the newline */
        strcat (&work[0], &w[0]);
        strcat (&work[0], "\n");
    }

    sock_puts (sock, work);  /* tell the server */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        gchar Error[256] = "An error was encountered when trying to retrieve the stats.",
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
            strcpy (&work[0], "Encountered error when talking to server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &Error[0];
        }

        outMessage (msg);
        return;
    }
    if (!strncmp (&buffer[0], "-1", 2)) {
        /* this happens if there are no stats */
        gchar NoStats[256] = "The Statistics do not exist.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);
        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 400);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    if (whichstats == 1)
        gtk_window_set_title (GTK_WINDOW (window), "Current NSB Season Regular Season Category Leaders");
    if (whichstats == 2)
        gtk_window_set_title (GTK_WINDOW (window), "Real Life Category Leaders");
    if (whichstats == 3)
        gtk_window_set_title (GTK_WINDOW (window), "Current Series Category Leaders");
    if (whichstats == 5)
        gtk_window_set_title (GTK_WINDOW (window), "Records");
    if (whichstats == 6)
        gtk_window_set_title (GTK_WINDOW (window), "Lifetime Regular Season Category Leaders");
    if (whichstats == 7)
        gtk_window_set_title (GTK_WINDOW (window), "Current NSB Season Post-Season Category Leaders");
    if (whichstats == 8)
        gtk_window_set_title (GTK_WINDOW (window), "Lifetime Post-Season Category Leaders");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box1);

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

    FillCategoryLeaders ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, stats, strlen (&stats[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    strcpy (&prtbutcatcmd[prtbutcatpnt][0], &work[0]);
    whichcatl[prtbutcatpnt] = whichstats;
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintCatL), GINT_TO_POINTER (prtbutcatpnt));
    prtbutcatpnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillCategoryLeaders () {
    gint x, y, z, teamid, type2, manysw = 0;
    gchar buf[300], hdr[100], year[5], w[100], w2[100], *cc, *cc1, *work, charid[3], cat, pos;
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50],
             info1[6],
             info2[6],
             info3[6],
             info4[6],
             info5[6],
             info6[6],
             stat[6],
             date[11],
             dis[4],
             user[50];
    } list[50];

    for (x = 0; x < 50; x++) {
        strcpy (&list[x].stat[0], "    0");
        strcpy (&list[x].info1[0], "    0");
        strcpy (&list[x].info2[0], "    0");
        strcpy (&list[x].info3[0], "    0");
        strcpy (&list[x].info4[0], "    0");
        strcpy (&list[x].info5[0], "    0");
    }

    type2 = statscmdstr[0] - '0';
    cat = statscmdstr[1];
    pos = statscmdstr[2];

    strcpy (&hdr[0], "         Name, Team                       ");

    buf[0] = '\0';
    switch (whichstats) {
        case 1:
            strcat (&buf[0], "NSB Season ");
            break;
        case 2:
            strcat (&buf[0], "Real Life ");
            break;
        case 3:
            strcat (&buf[0], "NSB Series ");
            break;
        case 5:
            strcat (&buf[0], "NSB Records ");
            break;
        case 6:
            strcat (&buf[0], "NSB Lifetime ");
            break;
        case 7:
            strcat (&buf[0], "NSB Season PS ");
            break;
        default:
            strcat (&buf[0], "NSB Lifetime PS ");
    }

    switch (type2) {
        case 1:
            strcat (&buf[0], "Hitting ");
            break;
        case 2:
            strcat (&buf[0], "Pitching ");
            break;
        default:
            strcat (&buf[0], "Fielding ");
    }

    if (whichstats == 5)
        strcat (&buf[0], "Record for ");
    else
        strcat (&buf[0], "Leaders in ");

    switch (type2) {
        case 1:
            if (cat == '1') {
                strcat (&buf[0], "Games Played");
                strcat (&hdr[0], " G ");
            }
            if (cat == '2') {
                strcat (&buf[0], "At Bats");
                strcat (&hdr[0], " AB");
            }
            if (cat == '3') {
                strcat (&buf[0], "Runs Scored");
                strcat (&hdr[0], " R ");
            }
            if (cat == '4') {
                strcat (&buf[0], "Hits");
                strcat (&hdr[0], " H ");
            }
            if (cat == '5') {
                strcat (&buf[0], "Doubles");
                strcat (&hdr[0], " 2B");
            }
            if (cat == '6') {
                strcat (&buf[0], "Triples");
                strcat (&hdr[0], " 3B");
            }
            if (cat == '7') {
                strcat (&buf[0], "Home Runs");
                strcat (&hdr[0], " HR");
            }
            if (cat == '8') {
                strcat (&buf[0], "Runs Batted In");
                strcat (&hdr[0], " BI");
            }
            if (cat == '9') {
                strcat (&buf[0], "Bases on balls");
                strcat (&hdr[0], " BB");
            }
            if (cat == 'a') {
                strcat (&buf[0], "Strike Outs");
                strcat (&hdr[0], " K ");
            }
            if (cat == 'b') {
                strcat (&buf[0], "Hit by Pitches");
                strcat (&hdr[0], "HBP");
            }
            if (cat == 'c') {
                strcat (&buf[0], "Grounded into DPs");
                strcat (&hdr[0], "GDP");
            }
            if (cat == 'd') {
                strcat (&buf[0], "Stolen Bases");
                strcat (&hdr[0], " SB");
            }
            if (cat == 'e') {
                strcat (&buf[0], "Caught Stealing");
                strcat (&hdr[0], " CS");
            }
            if (cat == 'f') {
                strcat (&buf[0], "Intentional Walks");
                strcat (&hdr[0], "IBB");
            }
            if (cat == 'g') {
                strcat (&buf[0], "Sacrifice Hits");
                strcat (&hdr[0], " SH");
            }
            if (cat == 'h') {
                strcat (&buf[0], "Sacrifice Flies");
                strcat (&hdr[0], " SF");
            }
            if (cat == 'i') {
                strcat (&buf[0], "Batting Average");
                strcat (&hdr[0], " AB     H    BA");
            }
            if (cat == 'j') {
                strcat (&buf[0], "Slugging Average");
                strcat (&hdr[0], " AB     H    2B    3B    HR    SA");
            }
            if (cat == 'k') {
                strcat (&buf[0], "On Base Average");
                strcat (&hdr[0], " PA     H    BB    HB   OBA");
            }
            break;
        case 2:
            if (cat == '1') {
                strcat (&buf[0], "Games Pitched");
                strcat (&hdr[0], " G ");
            }
            if (cat == '2') {
                strcat (&buf[0], "Games Started");
                strcat (&hdr[0], " GS");
            }
            if (cat == '3') {
                strcat (&buf[0], "Innings Pitched");
                strcat (&hdr[0], " IP");
            }
            if (cat == '4') {
                strcat (&buf[0], "Wins");
                strcat (&hdr[0], " W ");
            }
            if (cat == '5') {
                strcat (&buf[0], "Losses");
                strcat (&hdr[0], " L ");
            }
            if (cat == '6') {
                strcat (&buf[0], "Saves");
                strcat (&hdr[0], " S ");
            }
            if (cat == '7') {
                strcat (&buf[0], "Batters Facing Pitcher");
                strcat (&hdr[0], "BFP");
            }
            if (cat == '8') {
                strcat (&buf[0], "Hits Allowed");
                strcat (&hdr[0], " H ");
            }
            if (cat == '9') {
                strcat (&buf[0], "Doubles Allowed");
                strcat (&hdr[0], " 2B");
            }
            if (cat == 'a') {
                strcat (&buf[0], "Triples Allowed");
                strcat (&hdr[0], " 3B");
            }
            if (cat == 'b') {
                strcat (&buf[0], "Home Runs Allowed");
                strcat (&hdr[0], " HR");
            }
            if (cat == 'c') {
                strcat (&buf[0], "Runs Allowed");
                strcat (&hdr[0], " R ");
            }
            if (cat == 'd') {
                strcat (&buf[0], "Earned Runs Allowed");
                strcat (&hdr[0], " ER");
            }
            if (cat == 'e') {
                strcat (&buf[0], "RBIs Allowed");
                strcat (&hdr[0], " BI");
            }
            if (cat == 'f') {
                strcat (&buf[0], "Complete Games");
                strcat (&hdr[0], " CG");
            }
            if (cat == 'g') {
                strcat (&buf[0], "Games Finished");
                strcat (&hdr[0], " GF");
            }
            if (cat == 'h') {
                strcat (&buf[0], "Shut Outs");
                strcat (&hdr[0], "SHO");
            }
            if (cat == 'i') {
                strcat (&buf[0], "Save Opportunities");
                strcat (&hdr[0], "SOP");
            }
            if (cat == 'j') {
                strcat (&buf[0], "Stolen Bases Against");
                strcat (&hdr[0], " SB");
            }
            if (cat == 'k') {
                strcat (&buf[0], "Caught Stealing Against");
                strcat (&hdr[0], " CS");
            }
            if (cat == 'l') {
                strcat (&buf[0], "Bases on Balls");
                strcat (&hdr[0], " BB");
            }
            if (cat == 'm') {
                strcat (&buf[0], "Strike Outs");
                strcat (&hdr[0], " K ");
            }
            if (cat == 'n') {
                strcat (&buf[0], "Intentional Walks");
                strcat (&hdr[0], "IBB");
            }
            if (cat == 'o') {
                strcat (&buf[0], "Sacrifice Hits Allowed");
                strcat (&hdr[0], " SH");
            }
            if (cat == 'p') {
                strcat (&buf[0], "Sacrifice Flies Allowed");
                strcat (&hdr[0], " SF");
            }
            if (cat == 'q') {
                strcat (&buf[0], "Wild Pitches");
                strcat (&hdr[0], " WP");
            }
            if (cat == 'r') {
                strcat (&buf[0], "Balks");
                strcat (&hdr[0], " B ");
            }
            if (cat == 's') {
                strcat (&buf[0], "Hit Batters");
                strcat (&hdr[0], " HB");
            }
            if (cat == 't') {
                strcat (&buf[0], "Opponents' At Bats");
                strcat (&hdr[0], "OAB");
            }
            if (cat == 'u') {
                strcat (&buf[0], "Earned Run Average");
                strcat (&hdr[0], " IP      ER   ERA");
            }
            if (cat == 'v') {
                strcat (&buf[0], "Won/Loss Percentage");
                strcat (&hdr[0], "  W     L   PCT");
            }
            if (cat == 'w') {
                strcat (&buf[0], "Opponents' Batting Average");
                strcat (&hdr[0], "OAB     H   OBA");
            }
            break;
        default:
            if (cat == '1') {
                strcat (&buf[0], "Games Played");
                strcat (&hdr[0], " G ");
            }
            if (cat == '2') {
                strcat (&buf[0], "Put Outs");
                strcat (&hdr[0], " PO");
            }
            if (cat == '3') {
                strcat (&buf[0], "Double Plays");
                strcat (&hdr[0], " DP");
            }
            if (cat == '4') {
                strcat (&buf[0], "Assists");
                strcat (&hdr[0], " A ");
            }
            if (cat == '5') {
                strcat (&buf[0], "Passed Balls");
                strcat (&hdr[0], " PB");
            }
            if (cat == '6') {
                strcat (&buf[0], "Errors");
                strcat (&hdr[0], " E ");
            }
            if (cat == '7') {
                strcat (&buf[0], "Fielding Avg");
                strcat (&hdr[0], " TC     E    FA");
            }

            strcat (&buf[0], " for ");

            if (pos == '1')
                strcat (&buf[0], "OF");
            if (pos == '2')
                strcat (&buf[0], "1B");
            if (pos == '3')
                strcat (&buf[0], "2B");
            if (pos == '4')
                strcat (&buf[0], "3B");
            if (pos == '5')
                strcat (&buf[0], "SS");
            if (pos == '6')
                strcat (&buf[0], "P");
            if (pos == '7')
                strcat (&buf[0], "C");
            if (pos == '8')
                strcat (&buf[0], "DH");
    }
    if (whichstats == 5) {
        if (wri == 1)
            strcat (&buf[0], " (");
        if (wri == 2)
            strcat (&buf[0], " (All Users - ");

        if (sgi == 1) {
            strcat (&buf[0], "Game)");
            strcat (&buf[0], "\n(DIS = Day in Schedule, 998 & 999 = playoff game)");
        }
        else
            strcat (&buf[0], "Season)");
    }
    if (whichstats == 2) {
        gint x, yron;

        strcat (&buf[0], "\n");
        strcat (&buf[0], "Year(s) - ");

        for (yron = x = 0; x < YEAR_SPREAD; x++)
            if (yrs[x])
                if (!yron) {
                    strcat (&buf[0], cnvt_int2str (1901 + x, 'l'));
                    yron = 1;
                }
                else
                    yron = 2;
            else {
                if (yron == 1)
                    strcat (&buf[0], " ");
                if (yron == 2) {
                    strcat (&buf[0], "-");
                    strcat (&buf[0], cnvt_int2str (1901 + x - 1, 'l'));
                    strcat (&buf[0], " ");
                }
                yron = 0;
            }
        if (yron == 2) {
            strcat (&buf[0], "-");
            strcat (&buf[0], cnvt_int2str (1901 + x - 1, 'l'));
        }
    }

    if (whichstats == 5) {
        strcat (&hdr[0], "     Date");
        if (sgi == 1)
            strcat (&hdr[0], "     DIS");
        else
            strcat (&hdr[0], "   ");
        if (wri == 2)
            strcat (&hdr[0], " UserID");
    }

    for (x = 0, cc = &buffer[0]; x < 50 && cc < (&buffer[0] + strlen (&buffer[0])); x++) {
        work = (char *) index (cc, '?');
        *work = '\0';

        if (*cc == ' ')
            /* a blank name signifies no more stats */
            break;

        /* reverse name */
        strcpy (&w2[0], cc);
        strcpy (&w[0], &w2[index (&w2[0], ',') - &w2[0] + 2]);
        strcat (&w[0], " ");
        strncat (&w[0], &w2[0], (index (&w2[0], ',') - &w2[0]));
        strcpy (&list[x].name[0], &w[0]);

        *work = '?';
        cc = work + 2;
        strncpy (&year[0], cc, 4);
        year[4] = '\0';
        cc += 4;

        charid[0] = *cc;
        cc++;
        charid[1] = *cc;
        charid[2] = '\0';
        cc++;
        teamid = atoi (&charid[0]);

        strcpy (&list[x].tyr[0], &year[0]);

        if (year[0] != '0') {
            /* move Team Abbreviation */
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (teaminfo[y].id == teamid) {
                    strcpy (&list[x].tabbr[0], &teaminfo[y].teamabbrev[0]);
                    break;
                }
            list[x].uctname[0] = '\0';
        }
        else {
            cc++;
            work = index (cc, ' ');
            *work = '\0';
            strcpy (&list[x].uctname[0], cc);
            *work = ' ';
            cc = work + 1;
        }

        strncpy (&list[x].info1[0], cc, 5);
        list[x].info1[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info1[y] == '0')
                list[x].info1[y] = ' ';
            else
                break;
        cc += 5;
        strncpy (&list[x].info2[0], cc, 5);
        list[x].info2[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info2[y] == '0')
                list[x].info2[y] = ' ';
            else
                break;
        cc += 5;
        strncpy (&list[x].info3[0], cc, 5);
        list[x].info3[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info3[y] == '0')
                list[x].info3[y] = ' ';
            else
                break;
        cc += 5;
        strncpy (&list[x].info4[0], cc, 5);
        list[x].info4[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info4[y] == '0')
                list[x].info4[y] = ' ';
            else
                break;
        cc += 5;
        strncpy (&list[x].info5[0], cc, 5);
        list[x].info5[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info5[y] == '0')
                list[x].info5[y] = ' ';
            else
                break;
        cc += 5;

        for (cc1 = cc, y = 0; *cc1 == '0' && y < 4; y++, cc1++)
            *cc1 = ' ';
        if ((type2 == 1 && (cat == 'i' || cat == 'j' || cat == 'k')) || (type2 == 2 && (cat == 'v' || cat == 'w')) || (type2 == 3 && cat == '7')) {
            if (*(cc + 1) != ' ')
                *cc = *(cc + 1);
            *(cc + 1) = '.';
            if (*(cc + 2) == ' ')
                *(cc + 2) = '0';
            if (*(cc + 3) == ' ')
                *(cc + 3) = '0';
        }
        if (type2 == 2 && cat == 'u') {
            if (*(cc + 2) != ' ') {
                *cc = *(cc + 1);
                *(cc + 1) = *(cc + 2);
            }
            *(cc + 2) = '.';
            if (*(cc + 1) == ' ')
                *(cc + 1) = '0';
            if (*(cc + 3) == ' ')
                *(cc + 3) = '0';
        }
        strncpy (&list[x].stat[0], cc, 5);
        list[x].stat[5] = '\0';
        cc += 5;

        strncpy (&list[x].info6[0], cc, 5);
        list[x].info6[5] = '\0';
        for (y = 0; y < 4; y++)
            if (list[x].info6[y] == '0')
                list[x].info6[y] = ' ';
            else
                break;
        cc += 5;

        if (whichstats == 5) {
            strncpy (&list[x].date[0], cc, 2);
            list[x].date[2] = '\0';
            strcat (&list[x].date[0], "/");
            cc += 2;
            strncat (&list[x].date[0], cc, 2);
            list[x].date[5] = '\0';
            strcat (&list[x].date[0], "/");
            cc += 2;
            strncat (&list[x].date[0], cc, 4);
            list[x].date[10] = '\0';
            cc += 4;
            if (sgi == 1) {
                strncpy (&list[x].dis[0], cc, 3);
                list[x].dis[3] = '\0';
            }
            cc += 3;
            if (wri == 2) {
                work = (char *) index (cc, '?');
                *work = '\0';
                strcpy (&list[x].user[0], cc);
                *work = '?';
                cc = work + 2;
            }
        }
    }

    if (type2 == 1)
        if (cat == 'i' || cat == 'j' || cat == 'k') {
            if (whichstats == 1 || whichstats == 6 || whichstats == 8 || (whichstats == 2 && RLCatLeadersMY))
                strcat (&buf[0], "\nMinimum plate appearances - 3.1 X number of games player's team played");
            else {
                strcat (&buf[0], "\nMinimum plate appearances - ");
                strcat (&buf[0], &list[0].info6[0]);
            }
        }
    if (type2 == 2) {
        if (cat == 'u' || cat == 'w') {
            if (whichstats == 1 || whichstats == 6 || whichstats == 8 || (whichstats == 2 && RLCatLeadersMY))
                strcat (&buf[0], "\nMinimum innings pitched - number of games player's team played");
            else {
                strcat (&buf[0], "\nMinimum innings pitched - ");
                strcat (&buf[0], &list[0].info6[0]);
            }
        }
        if (cat == 'v') {
            if (whichstats == 1 || whichstats == 6 || whichstats == 8 || (whichstats == 2 && RLCatLeadersMY))
                strcat (&buf[0], "\nMinimum number of decisions - number of games player's team played / 12");
            else {
                strcat (&buf[0], "\nMinimum number of decisions - ");
                strcat (&buf[0], &list[0].info6[0]);
            }
        }
    }
    if (type2 == 3)
        if (cat == '7') {
            strcat (&buf[0], "\nMinimum number of ");
            if (pos == '6')
                strcat (&buf[0], "innings pitched");
            else
                strcat (&buf[0], "games played");
            if (whichstats != 6 && whichstats != 8 && (whichstats == 2 && !RLCatLeadersMY)) {
                strcat (&buf[0], " - ");
                strcat (&buf[0], &list[0].info6[0]);
            }
            else
                if (pos == '6')
                    strcat (&buf[0], " - number of games player's team played");
                else
                    if (pos == '7')
                        strcat (&buf[0], " - number of games player's team played / 2");
                    else
                        strcat (&buf[0], " - number of games player's team played / 3 X 2");
        }

    strcpy (&stats[0], &buf[0]);
    strcat (&stats[0], "\n\n");
    strcat (&stats[0], &hdr[0]);
    strcat (&stats[0], "\n\n");

    x--;
    if (x > 24) {
        if (!strcmp (&list[24].stat[0], &list[25].stat[0]))
            if (!strcmp (&list[24].stat[0], &list[49].stat[0]))
                for (x = 23; x >= 0; x--) {
                    if (!strcmp (&list[24].stat[0], &list[x].stat[0]))
                        continue;
                    if (x == 23) {
                        x = 24;
                        break;
                    }
                    x++;
                    manysw = 1;
                    break;
                }
            else
                for (x = 25; x < 50; x++)
                    if (!strcmp (&list[24].stat[0], &list[x].stat[0]))
                        continue;
                    else {
                        x--;
                        break;
                    }
        else
            x = 24;
    }

    if (x < 0) {
        x = 0;
        manysw = 1;
    }

    for (y = 0; y <= x; y++) {
        buf[0] = '\0';
        strcat (&buf[0], (char *) cnvt_int2str ((y + 1), 'l'));
        strcat (&buf[0], "  ");

        if (x == y && manysw) {
            strcat (&buf[0], "Many tied with                      ");
            if (y < 9)
                strcat (&buf[0], " ");
            if ((type2 == 1 && (cat == 'i' || cat == 'j' || cat == 'k')) || (type2 == 2 && (cat == 'u' || cat == 'w' || cat == 'v')) ||
                       (type2 == 3 && cat == '7'))
                strcat (&buf[0], "            ");
            if (type2 == 1 && cat == 'k')
                strcat (&buf[0], "           ");

            strcat (&buf[0], &list[y].stat[0]);
            strcat (&stats[0], &buf[0]);
            strcat (&stats[0], "\n");
            break;
        }

        strcat (&buf[0], &list[y].name[0]);
        strcat (&buf[0], ", ");
        if (!strlen (&list[y].uctname[0])) {
            strcat (&buf[0], &list[y].tyr[0]);
            strcat (&buf[0], &list[y].tabbr[0]);
        }
        else
            strcat (&buf[0], &list[y].uctname[0]);

        z = strlen (&buf[0]);
        strncat (&buf[0], "                                        ", 40 - z);

        if (type2 == 1) {
            if (cat == 'i') {
                strcat (&buf[0], &list[y].info1[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info2[0]);
                strcat (&buf[0], " ");
            }
            if (cat == 'j') {
                strcat (&buf[0], &list[y].info1[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info2[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info3[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info4[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info5[0]);
                strcat (&buf[0], " ");
            }
            if (cat == 'k') {
                strcat (&buf[0], &list[y].info1[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info2[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info3[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info4[0]);
                strcat (&buf[0], " ");
            }
        }
        if (type2 == 2) {
            if (cat == 'v' || cat == 'w') {
                strcat (&buf[0], &list[y].info1[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info2[0]);
                strcat (&buf[0], " ");
            }
            if (cat == 'u') {
                strcat (&buf[0], &list[y].info1[0]);
                if (strcmp (&list[y].info2[0], "    0")) {
                    strcat (&buf[0], ".");
                    strcat (&buf[0], &list[y].info2[4]);
                }
                else
                    strcat (&buf[0], "  ");

                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info3[0]);
                strcat (&buf[0], " ");
            }
        }
        if (type2 == 3)
            if (cat == '7') {
                strcat (&buf[0], &list[y].info1[0]);
                strcat (&buf[0], " ");
                strcat (&buf[0], &list[y].info2[0]);
                strcat (&buf[0], " ");
            }

        strcat (&buf[0], &list[y].stat[0]);

        if (type2 == 2 && cat == '3')
            if (list[y].info1[4] != '0') {
                strcat (&buf[0], ".");
                strncat (&buf[0], &list[y].info1[4], 1);
            }

        if (whichstats == 5) {
            strcat (&buf[0], "  ");
            strcat (&buf[0], &list[y].date[0]);
            strcat (&buf[0], "  ");
            if (sgi == 1) {
                strcat (&buf[0], &list[y].dis[0]);
                strcat (&buf[0], " ");
            }
            if (wri == 2)
                strcat (&buf[0], &list[y].user[0]);
        }
        strcat (&stats[0], &buf[0]);
        strcat (&stats[0], "\n");
    }
    strcat (&stats[0], "\n");
}

void
PrintCatL (GtkWidget *widget, gpointer cnt) {
    gint x, y, icnt = GPOINTER_TO_INT (cnt);
    gchar work1[1024], Printing[256] = "Printing Category Leaders ...", *msg[5];

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print stats.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print stats.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&work1[0], &prtbutcatcmd[icnt][0]);
    whichstats = whichcatl[icnt];
    statscmdstr[0] = work1[6];
    statscmdstr[1] = work1[7];
    statscmdstr[2] = work1[8];
    strncpy (&yeara[0], &work1[2], 4);
    yeara[4] = '\0';
    sgi = work1[8] - '0';
    wri = work1[9] - '0';
    if (whichstats == 2)
        for (x = 0, y = 11; x < YEAR_SPREAD; x++, y++)
            yrs[x] = work1[y] - '0';

    sock_puts (sock, work1);  /* tell the server to send us specific category leaders or records again */
    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */

    FillCategoryLeaders ();

    print (&stats[0]);

    strcpy (&work[0], "Print category leaders/records.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

GtkTextBuffer *tbuffer;

void
ShowTeamStats () {
    GtkWidget *window, *vpaned, *view, *sw, *vbox, *hbox, *pbutton, *button;

    psorrs = whichpsorrs[prtbuttmpnt] = buffer[2];
    if (get_stats (sock, 't', 0) == -1)
        return;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 1350, 600);
    g_signal_connect (window, "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Team Stats");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    vpaned = gtk_vpaned_new ();
    gtk_container_set_border_width (GTK_CONTAINER (vpaned), 5);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

    view = gtk_text_view_new ();
    tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_NONE);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_add1 (GTK_PANED (vpaned), sw);
    gtk_container_add (GTK_CONTAINER (sw), view);

    FillTeamStats (tbuffer);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintTeamStats), GINT_TO_POINTER (prtbuttmpnt));
    prtbuttmpnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillTeamStats (GtkTextBuffer *buf) {
    GtkTextIter pos;
    gint x, y, z, pib, totg, w, l, newpl, singles, ts, tab, tr, th, t2b, t3b, thr, tbi, tbb, tso, thbp, tgidp, tsb, tcs, tibb, tsf, tsh,
         tinn, tthirds, ter, tw, tl, tgs, tcg, tgf, tsho, tsopp, twp, thb, tb, tbfp, tpo, ta, te, tpb, topp_ab,
         maxplayers, maxpitchers;
    gchar work1[100], work2[100], tname[100];

    totg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    gtk_text_buffer_get_iter_at_offset (buf, &pos, 0);
    gtk_text_buffer_create_tag (buf, "monospace", "family", "monospace", NULL);

    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
        if (teaminfo[x].id == team.id)
            break;

    for (maxplayers = 0; maxplayers < 25; maxplayers++)
        if (team.batters[maxplayers].id.name[0] == ' ' || !strlen (&team.batters[maxplayers].id.name[0]))
            break;
    for (maxpitchers = 0; maxpitchers < 25; maxpitchers++)
        if (team.pitchers[maxpitchers].id.name[0] == ' ' || !strlen (&team.pitchers[maxpitchers].id.name[0]))
            break;

    strcpy (&stats[0], "                               ");
    if (urind != 'C' && team.year) {
        strcat (&stats[0], (char *) cnvt_int2str ((team.year), 'l'));
        strcat (&stats[0], " ");
    }
    if (team.year)
        strcat (&stats[0], &teaminfo[x].teamname[0]);
    else
        strcat (&stats[0], &usercreatedtname[0]);
    strcat (&stats[0], " - ");
    if (urind == 'S')
        strcat (&stats[0], "NSB Series Statistics\n\n");
    else
        if (urind == 'U') {
            strcat (&stats[0], "NSB Season Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&stats[0], "(Post-Season)");
            else
                strcat (&stats[0], "(Regular Season)");
            strcat (&stats[0], "\n\n");
        }
        else
            if (urind == 'L') {
                strcat (&stats[0], "NSB Lifetime Statistics ");
            if (psorrs == '6' || psorrs == '7')
                strcat (&stats[0], "(Post-Season)");
            else
                strcat (&stats[0], "(Regular Season)");
                strcat (&stats[0], "\n\n");
            }
            else
                if (urind == 'C')
                    strcat (&stats[0], "Real Life Statistics (User-Created Team)\n\n");
                else {
                    strcat (&stats[0], "Real Life Statistics\n\n");
                    strcat (&stats[0], "(NOTE - The totals do not include players which may have played ");
                    strcat (&stats[0], "in real life but are not included in this NSB roster.)\n\n");

                    /* need RL results to get total team games */
                    strcpy (&work2[0], cnvt_int2str (team.year, 'l'));
                    strcpy (&work1[0], "S3");
                    strcat (&work1[0], &work2[0]);
                    strcat (&work1[0], "\n");
                    sock_puts (sock, work1);  /* tell the server to send real life results for a specific year */

                    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */

                    /* real life data ... get total games played by the team from the Results data (wins + losses) */
                    if (team.year != 1981) {  /* the 1981 Results data is in a different format than all other years */
                        /* get the team name */
                        strcpy (&tname[0], &teaminfo[x].teamname[0]);
                        /* the Cardinals and Browns show as "St." in Results files but as "St " in teamnames */
                        if (!strncmp (&tname[0], "St ", 3)) {
                            strcpy (&work2[0], &tname[0]);
                            tname[2] = '.';
                            tname[3] = ' ';
                            strcpy (&tname[4], &work2[3]);
                        }

                        for (pib = 0; pib < strlen (&buffer[0]); pib++)
                            if (!strncmp (&buffer[pib], &tname[0], strlen (&tname[0]))) {
                                pib += strlen (&tname[0]);   /* move data pointer past team name */
                                for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team wins) */
                                /* get team wins */
                                for (z = 0; buffer[pib] != ' '; z++, pib++)
                                    work2[z] = buffer[pib];
                                work2[z] = '\0';
                                w = atoi (&work2[0]);

                                for ( ; buffer[pib] == ' '; pib++);      /* go to the next non-whitespace (first position of team losses) */
                                /* get team losses */
                                for (z = 0; buffer[pib] != ' '; z++, pib++)
                                    work2[z] = buffer[pib];
                                work2[z] = '\0';
                                l = atoi (&work2[0]);

                                w += l;    /* total team games */
                                totg += w;     /* accumulate total games */

                                break;
                            }
                    }
                }

    /* if the stats we're seeking are not real life then get total games from all the pitchers' W/L records
       we cannot get total games if the stats are real life and the team is user-created */
    if (urind == 'S' || urind == 'U' || urind == 'L')
        for (x = 0; x < maxpitchers; x++) {
            totg += team.pitchers[x].pitching.wins;
            totg += team.pitchers[x].pitching.losses;
        }

    if (urind == 'C')
        strcat (&stats[0], "Player Name                  Real Life Team                  G     AB      R      H     2B     3B     HR    RBI    BA    SA   OBA     BB     SO    HBP   GIDP     SB     CS    IBB     SH     SF\n\n");
    else
        strcat (&stats[0], "Player Name                  G     AB      R      H     2B     3B     HR    RBI    BA    SA   OBA     BB     SO    HBP   GIDP     SB     CS    IBB     SH     SF\n\n");

    for (x = 0; x < maxplayers; x++) {
        singles = team.batters[x].hitting.hits - (team.batters[x].hitting.homers + team.batters[x].hitting.triples + team.batters[x].hitting.doubles);
        strncat (&stats[0], &team.batters[x].id.name[0], 23);
        strncat (&stats[0], "                       ", 23 - strlen (&team.batters[x].id.name[0]));

        if (urind == 'C') {
            strcat (&stats[0], "   ");
            strcat (&stats[0], check_stats (team.batters[x].id.year, 'l'));
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (team.batters[x].id.teamid == teaminfo[y].id) {
                    strcat (&stats[0], &teaminfo[y].filename[0]);
                    strncat (&stats[0], "                         ", 25 - strlen (&teaminfo[y].filename[0]));
                    break;
                }
        }

        strcat (&stats[0], (char *) check_stats (team.batters[x].hitting.games, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.atbats, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.runs, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.hits, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.doubles, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.triples, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.homers, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.rbi, 'r'));
        strcat (&stats[0], (char *) do_average (team.batters[x].hitting.hits, team.batters[x].hitting.atbats));
        strcat (&stats[0], (char *) do_average (((team.batters[x].hitting.homers * 4) + (team.batters[x].hitting.triples * 3) +
                                                 (team.batters[x].hitting.doubles * 2) + singles), team.batters[x].hitting.atbats));
        if (team.batters[x].hitting.sf == -1)
            y = 0;
        else
            y = team.batters[x].hitting.sf;
        y += team.batters[x].hitting.sh;
        strcat (&stats[0], (char *) do_average ((team.batters[x].hitting.hits + team.batters[x].hitting.bb + team.batters[x].hitting.hbp),
                                                 (team.batters[x].hitting.atbats + team.batters[x].hitting.bb + y + team.batters[x].hitting.hbp)));
        strcat (&stats[0], check_stats (team.batters[x].hitting.bb, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.so, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.hbp, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.gidp, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.sb, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.cs, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.ibb, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.sh, 'r'));
        strcat (&stats[0], check_stats (team.batters[x].hitting.sf, 'r'));
        strcat (&stats[0], "\n");

        tab += team.batters[x].hitting.atbats;
        tr += team.batters[x].hitting.runs;
        th += team.batters[x].hitting.hits;
        t2b += team.batters[x].hitting.doubles;
        t3b += team.batters[x].hitting.triples;
        thr += team.batters[x].hitting.homers;
        tbi += team.batters[x].hitting.rbi;
        tbb += team.batters[x].hitting.bb;
        if (team.batters[x].hitting.so != -1)
            /* strikeouts for the batter wasn't always a recorded stat */
            tso += team.batters[x].hitting.so;
        thbp += team.batters[x].hitting.hbp;
        if (team.batters[x].hitting.gidp != -1)
            /* grounded into DP for the batter wasn't always a recorded stat */
            tgidp += team.batters[x].hitting.gidp;
        tsb += team.batters[x].hitting.sb;
        if (team.batters[x].hitting.cs != -1)
            /* caught stealing for the batter wasn't always a recorded stat */
            tcs += team.batters[x].hitting.cs;
        if (team.batters[x].hitting.ibb != -1)
            /* intentional walk for the batter wasn't always a recorded stat */
            tibb += team.batters[x].hitting.ibb;
        tsh += team.batters[x].hitting.sh;
        if (team.batters[x].hitting.sf != -1)
            /* sacrifice flies for the batter wasn't always a recorded stat */
            tsf += team.batters[x].hitting.sf;
        ts += singles;
    }

    if (urind == 'C')
        strcat (&stats[0], "  TOTALS                                                      ");
    else {
        strcat (&stats[0], "  TOTALS               ");
        strcat (&stats[0], check_stats (totg, 'r'));
    }
    strcat (&stats[0], check_stats (tab, 'r'));
    strcat (&stats[0], check_stats (tr, 'r'));
    strcat (&stats[0], check_stats (th, 'r'));
    strcat (&stats[0], check_stats (t2b, 'r'));
    strcat (&stats[0], check_stats (t3b, 'r'));
    strcat (&stats[0], check_stats (thr, 'r'));
    strcat (&stats[0], check_stats (tbi, 'r'));
    strcat (&stats[0], (char *) do_average (th, tab));
    strcat (&stats[0], (char *) do_average (((thr * 4) + (t3b * 3) + (t2b * 2) + ts), tab));
    strcat (&stats[0], (char *) do_average ((th + tbb + thbp), (tab + tbb + tsf + tsh + thbp)));
    strcat (&stats[0], check_stats (tbb, 'r'));
    strcat (&stats[0], check_stats (tso, 'r'));
    strcat (&stats[0], check_stats (thbp, 'r'));
    strcat (&stats[0], check_stats (tgidp, 'r'));
    strcat (&stats[0], check_stats (tsb, 'r'));
    strcat (&stats[0], check_stats (tcs, 'r'));
    strcat (&stats[0], check_stats (tibb, 'r'));
    strcat (&stats[0], check_stats (tsh, 'r'));
    strcat (&stats[0], check_stats (tsf, 'r'));

    strcat (&stats[0], "\n\n\n\n");

    tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    if (urind == 'C')
        strcat (&stats[0], "Pitcher Name                 Real Life Team                  G     IP        H      R     ER      W      L   PCT     SO     BB   ERA     GS     CG     GF    SHO      S   SOPP    IBB     WP     HB      B    BFP     2B     3B     HR    RBI     SB     CS     SH     SF   OPAB  OPBA\n\n");
    else
        strcat (&stats[0], "Pitcher Name                 G     IP        H      R     ER      W      L   PCT     SO     BB   ERA     GS     CG     GF    SHO      S   SOPP    IBB     WP     HB      B    BFP     2B     3B     HR    RBI     SB     CS     SH     SF   OPAB  OPBA\n\n");

    for (x = 0; x < maxpitchers; x++) {
        if (team.pitchers[x].id.name[0] == ' ')
            break;
        strncat (&stats[0], &team.pitchers[x].id.name[0], 23);
        strncat (&stats[0], "                       ", 23 - strlen (&team.pitchers[x].id.name[0]));

        if (urind == 'C') {
            strcat (&stats[0], "   ");
            strcat (&stats[0], check_stats (team.pitchers[x].id.year, 'l'));
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (team.pitchers[x].id.teamid == teaminfo[y].id) {
                    strcat (&stats[0], &teaminfo[y].filename[0]);
                    strncat (&stats[0], "                         ", 25 - strlen (&teaminfo[y].filename[0]));
                    break;
                }
        }

        strcat (&stats[0], (char *) check_stats (team.pitchers[x].pitching.games, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.innings, 'r'));
        if (team.pitchers[x].pitching.thirds == 1 || team.pitchers[x].pitching.thirds == 2) {
            strcat (&stats[0], ".");
            strcat (&stats[0], check_stats (team.pitchers[x].pitching.thirds, 'l'));
        }
        else
            strcat (&stats[0], "  ");
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.hits, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.runs, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.er, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.wins, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.losses, 'r'));
        strcat (&stats[0], (char *) do_average (team.pitchers[x].pitching.wins, (team.pitchers[x].pitching.wins + team.pitchers[x].pitching.losses)));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.so, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.walks, 'r'));
        strcat (&stats[0], (char *) do_era (team.pitchers[x].pitching.er * 9, team.pitchers[x].pitching.innings, team.pitchers[x].pitching.thirds));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.games_started, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.cg, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.gf, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.sho, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.saves, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.svopp, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.ibb, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.wp, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.hb, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.balks, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.bfp, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.doubles, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.triples, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.homers, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.rbi, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.sb, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.cs, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.sh, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.sf, 'r'));
        strcat (&stats[0], check_stats (team.pitchers[x].pitching.opp_ab, 'r'));
        strcat (&stats[0], (char *) do_average (team.pitchers[x].pitching.hits, team.pitchers[x].pitching.opp_ab));
        strcat (&stats[0], "\n");

        tinn += team.pitchers[x].pitching.innings;
        tthirds += team.pitchers[x].pitching.thirds;
        tr += team.pitchers[x].pitching.runs;
        th += team.pitchers[x].pitching.hits;
        ter += team.pitchers[x].pitching.er;
        tw += team.pitchers[x].pitching.wins;
        tl += team.pitchers[x].pitching.losses;
        tbb += team.pitchers[x].pitching.walks;
        tso += team.pitchers[x].pitching.so;
        tgs += team.pitchers[x].pitching.games_started;
        tcg += team.pitchers[x].pitching.cg;
        tgf += team.pitchers[x].pitching.gf;
        tsho += team.pitchers[x].pitching.sho;
        ts += team.pitchers[x].pitching.saves;
        if (team.pitchers[x].pitching.svopp != -1)
            /* save opportunities for the pitcher wasn't always a recorded stat */
            tsopp += team.pitchers[x].pitching.svopp;
        if (team.pitchers[x].pitching.ibb != -1)
            /* intentional walks for the pitcher wasn't always a recorded stat */
            tibb += team.pitchers[x].pitching.ibb;
        twp += team.pitchers[x].pitching.wp;
        thb += team.pitchers[x].pitching.hb;
        tb += team.pitchers[x].pitching.balks;
        if (team.pitchers[x].pitching.bfp != -1)
            /* batters facing pitcher wasn't always a recorded stat */
            tbfp += team.pitchers[x].pitching.bfp;
        if (team.pitchers[x].pitching.doubles != -1)
            /* doubles allowed for the pitcher wasn't always a recorded stat */
            t2b += team.pitchers[x].pitching.doubles;
        if (team.pitchers[x].pitching.triples != -1)
            /* triples allowed for the pitcher wasn't always a recorded stat */
            t3b += team.pitchers[x].pitching.triples;
        thr += team.pitchers[x].pitching.homers;
        if (team.pitchers[x].pitching.rbi != -1)
            /* RBIs allowed for the pitcher wasn't always a recorded stat */
            tbi += team.pitchers[x].pitching.rbi;
        if (team.pitchers[x].pitching.sb != -1)
            /* stolen bases allowed for the pitcher wasn't always a recorded stat */
            tsb += team.pitchers[x].pitching.sb;
        if (team.pitchers[x].pitching.cs != -1)
            /* caught stealing against for the pitcher wasn't always a recorded stat */
            tcs += team.pitchers[x].pitching.cs;
        if (team.pitchers[x].pitching.sh != -1)
            /* sacrifice hits allowed for the pitcher wasn't always a recorded stat */
            tsh += team.pitchers[x].pitching.sh;
        if (team.pitchers[x].pitching.sf != -1)
            /* sacrifice flies allowed for the pitcher wasn't always a recorded stat */
            tsf += team.pitchers[x].pitching.sf;
        if (team.pitchers[x].pitching.opp_ab != -1)
            /* at bats against for the pitcher wasn't always a recorded stat */
            topp_ab += team.pitchers[x].pitching.opp_ab;
    }

    if (urind == 'C')
        strcat (&stats[0], "  TOTALS                                                      ");
    else {
        strcat (&stats[0], "  TOTALS               ");
        strcat (&stats[0], check_stats (totg, 'r'));
    }
    for (; tthirds > 2; tthirds -= 3)
        tinn++;
    strcat (&stats[0], check_stats (tinn, 'r'));
    if (tthirds == 1 || tthirds == 2) {
        strcat (&stats[0], ".");
        strcat (&stats[0], check_stats (tthirds, 'l'));
    }
    else
        strcat (&stats[0], "  ");
    strcat (&stats[0], check_stats (th, 'r'));
    strcat (&stats[0], check_stats (tr, 'r'));
    strcat (&stats[0], check_stats (ter, 'r'));
    strcat (&stats[0], check_stats (tw, 'r'));
    strcat (&stats[0], check_stats (tl, 'r'));
    strcat (&stats[0], (char *) do_average (tw, (tw + tl)));
    strcat (&stats[0], check_stats (tso, 'r'));
    strcat (&stats[0], check_stats (tbb, 'r'));
    strcat (&stats[0], (char *) do_era (ter * 9, tinn, tthirds));
    strcat (&stats[0], check_stats (tgs, 'r'));
    strcat (&stats[0], check_stats (tcg, 'r'));
    strcat (&stats[0], check_stats (tgf, 'r'));
    strcat (&stats[0], check_stats (tsho, 'r'));
    strcat (&stats[0], check_stats (ts, 'r'));
    strcat (&stats[0], check_stats (tsopp, 'r'));
    strcat (&stats[0], check_stats (tibb, 'r'));
    strcat (&stats[0], check_stats (twp, 'r'));
    strcat (&stats[0], check_stats (thb, 'r'));
    strcat (&stats[0], check_stats (tb, 'r'));
    strcat (&stats[0], check_stats (tbfp, 'r'));
    strcat (&stats[0], check_stats (t2b, 'r'));
    strcat (&stats[0], check_stats (t3b, 'r'));
    strcat (&stats[0], check_stats (thr, 'r'));
    strcat (&stats[0], check_stats (tbi, 'r'));
    strcat (&stats[0], check_stats (tsb, 'r'));
    strcat (&stats[0], check_stats (tcs, 'r'));
    strcat (&stats[0], check_stats (tsh, 'r'));
    strcat (&stats[0], check_stats (tsf, 'r'));
    strcat (&stats[0], check_stats (topp_ab, 'r'));
    if (topp_ab)
        strcat (&stats[0], (char *) do_average (th, topp_ab));

    strcat (&stats[0], "\n\n\n\n");

    if (urind == 'C')
        strcat (&stats[0], "Player Name                  Real Life Team           POS      G     PO     DP      A      E     PB   FA\n\n");
    else
        strcat (&stats[0], "Player Name           POS      G     PO     DP      A      E     PB   FA\n\n");

    for (x = 0; x < maxplayers; x++) {
        strncat (&stats[0], &team.batters[x].id.name[0], 23);
        strncat (&stats[0], "                    ", 23 - strlen (&team.batters[x].id.name[0]));

        if (urind == 'C') {
            strcat (&stats[0], "   ");
            strcat (&stats[0], check_stats (team.batters[x].id.year, 'l'));
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (team.batters[x].id.teamid == teaminfo[y].id) {
                    strcat (&stats[0], &teaminfo[y].filename[0]);
                    strncat (&stats[0], "                         ", 25 - strlen (&teaminfo[y].filename[0]));
                    break;
                }
        }

        for (newpl = y = 0; y < 11; y++)
            if (team.batters[x].fielding[y].games > 0) {
                if (newpl == 1) {
                    if (urind == 'C')
                        strcat (&stats[0], "                                                       ");
                    else
                        strcat (&stats[0], "                       ");
                }
                if (y == 10)
                    strcat (&stats[0], "OF");
                else
                    strcat (&stats[0], figure_pos (y));
                strcat (&stats[0], check_stats (team.batters[x].fielding[y].games, 'r'));
                strcat (&stats[0], check_stats (team.batters[x].fielding[y].po, 'r'));
                strcat (&stats[0], check_stats (team.batters[x].fielding[y].dp, 'r'));
                strcat (&stats[0], check_stats (team.batters[x].fielding[y].a, 'r'));
                strcat (&stats[0], check_stats (team.batters[x].fielding[y].e, 'r'));
                if (y == 2)
                    strcat (&stats[0], check_stats (team.batters[x].fielding[y].pb, 'r'));
                else
                    strcat (&stats[0], "       ");
                strcat (&stats[0], (char *) do_average ((team.batters[x].fielding[y].po + team.batters[x].fielding[y].a),
                    (team.batters[x].fielding[y].po + team.batters[x].fielding[y].a + team.batters[x].fielding[y].e)));
                newpl = 1;
                strcat (&stats[0], "\n");

                if (team.batters[x].fielding[y].po != -1)
                    tpo += team.batters[x].fielding[y].po;
                if (team.batters[x].fielding[y].a != -1)
                    ta += team.batters[x].fielding[y].a;
                if (team.batters[x].fielding[y].e != -1)
                    te += team.batters[x].fielding[y].e;
                if (y == 2)
                    tpb += team.batters[x].fielding[y].pb;
            }
        if (newpl == 0)
            strcat (&stats[0], "\n");
    }
    if (urind == 'C')
        strcat (&stats[0], "  TOTALS                                                        ");
    else {
        strcat (&stats[0], "  TOTALS                 ");
        strcat (&stats[0], check_stats (totg, 'r'));
    }
    strcat (&stats[0], check_stats (tpo, 'r'));
    strcat (&stats[0], "       ");
    strcat (&stats[0], check_stats (ta, 'r'));
    strcat (&stats[0], check_stats (te, 'r'));
    strcat (&stats[0], check_stats (tpb, 'r'));
    strcat (&stats[0], (char *) do_average ((tpo + ta), (tpo + ta + te)));

    strcat (&stats[0], "\n\n");

    gtk_text_buffer_insert_with_tags_by_name (buf, &pos, stats, -1, "monospace", NULL);
}

void
GetTeamTotals () {
    for (nteams = 0; ; nteams++) {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (atoi (&buffer[0]) == -1)
            /* end of data */
            break;

        /* get stats */
        teamdata[nteams].id = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].year = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].league = buffer[0];
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].division = buffer[0];

        if (!teamdata[nteams].year) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            strcpy (&uctname[nteams][0], &buffer[0]);
        }

        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.atbats = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.bb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.hbp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.gidp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.hitting.sf = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].po = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].dp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].a = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].pb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].batters.fielding[0].e = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.games_started = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.innings = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.thirds = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.wins = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.losses = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.saves = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.bfp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.er = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.cg = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.gf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.sho = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.svopp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.walks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.sf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.wp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.balks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.hb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        teamdata[nteams].pitchers.pitching.opp_ab = atoi (&buffer[0]);
    }
}

GtkTextBuffer *rlpbuffer;

void
ShowPlayerStats (char *pname) {
    GtkWidget *window, *vpaned, *view, *sw, *vbox, *hbox, *pbutton, *button, *picwin, *label;
    gint nbat, npit, bufpnt, x, y, gothit;
    gchar work[100], borp, nmsanssp[100], fname[300], fullpath[300];
    DIR *fnames;
    struct dirent *dir;

    nbat = npit = bufpnt = 0;

    do {
        if (!strncmp (&buffer[bufpnt], "BSTATS", 6))
            borp = 'B';
        else
            borp = 'P';
        bufpnt += 7;
        strncpy (&work[0], &buffer[bufpnt], 4);
        work[4] = '\0';
        teamdata[nbat].batters.id.year = teamdata[npit].pitchers.id.year = atoi (&work[0]);
        bufpnt += 5;
        strncpy (&work[0], &buffer[bufpnt], 2);
        work[2] = '\0';

        teamdata[nbat].batters.id.teamid = teamdata[npit].pitchers.id.teamid = atoi (&work[0]);
        bufpnt += 3;
        teamdata[nbat].batters.id.batslr = teamdata[npit].pitchers.id.batslr = buffer[bufpnt] - '0';
        teamdata[nbat].batters.id.throwslr = teamdata[npit].pitchers.id.throwslr = buffer[bufpnt + 2] - '0';
        bufpnt += 3;
        strncpy (&work[0], &buffer[bufpnt], 2);
        work[2] = '\0';
        teamdata[nbat].batters.dob.month = atoi (&work[0]);
        strncpy (&work[0], &buffer[bufpnt + 2], 2);
        work[2] = '\0';
        teamdata[nbat].batters.dob.day = atoi (&work[0]);
        strncpy (&work[0], &buffer[bufpnt + 4], 4);
        work[4] = '\0';
        teamdata[nbat].batters.dob.year = atoi (&work[0]);

        if (borp == 'B') {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.atbats = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.runs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.hits = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.doubles = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.triples = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.homers = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.rbi = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.bb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.so = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.hbp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.gidp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.cs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.ibb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sh = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sf = atoi (&buffer[0]);
 
            for (x = 0; x < 11; x++) {
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].games = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].po = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].dp = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].a = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].pb = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].e = atoi (&buffer[0]);
            }

            nbat++;
        }
        else {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.games_started = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.innings = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.thirds = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.wins = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.losses = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.saves = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.bfp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.hits = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.doubles = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.triples = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.homers = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.runs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.er = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.rbi = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.cg = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.gf = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sho = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.svopp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.cs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.walks = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.so = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.ibb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sh = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sf = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.wp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.balks = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.hb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.opp_ab = atoi (&buffer[0]);

            npit++;
        }

        sock_gets (sock, &buffer[0], sizeof (buffer));
        bufpnt = 0;
    } while (strncmp (&buffer[0], "-1", 2));

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 1350, 600);
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_widget_destroyed), &window);
    gtk_window_set_title (GTK_WINDOW (window), "Player Stats");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    vpaned = gtk_vpaned_new ();
    gtk_container_set_border_width (GTK_CONTAINER (vpaned), 5);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

    view = gtk_text_view_new ();
    rlpbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_NONE);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_add1 (GTK_PANED (vpaned), sw);
    gtk_container_add (GTK_CONTAINER (sw), view);

    FillRLPlayerStats (rlpbuffer, pname, nbat, npit);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    strcpy (&prtbutrlpcmd[prtbutrlppnt][0], "S9");
    strcat (&prtbutrlpcmd[prtbutrlppnt][0], pname);
    strcat (&prtbutrlpcmd[prtbutrlppnt][0], "\n");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintRLPlayerStats), GINT_TO_POINTER (prtbutrlppnt));
    prtbutrlppnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);

    /* fill in filename for player pic */
    for (y = x = 0; x <= strlen (&pname[0]); x++)
        if (pname[x] != ' ') {
            nmsanssp[y] = pname[x];
            y++;
        }
    strcpy (&fname[0], &nmsanssp[0]);
    strcat (&fname[0], "-");
    if (teamdata[0].batters.dob.year > 1800 && teamdata[0].batters.dob.year < 3000) {
        if (teamdata[0].batters.dob.month) {
            if (teamdata[0].batters.dob.month < 10)
                strcat (&fname[0], "0");
            strcat (&fname[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.month), 'l'));
            if (teamdata[0].batters.dob.day) {
                if (teamdata[0].batters.dob.day < 10)
                    strcat (&fname[0], "0");
                strcat (&fname[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.day), 'l'));
            }
            else
                strcat (&fname[0], "00");
        }
        else
            strcat (&fname[0], "0000");
        strcat (&fname[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.year), 'l'));
    }
    else
        strcat (&fname[0], "000000");
    strcat (&fname[0], ".jpg");

    /* make sure the player pic exists, search case-insensitive */
    strcpy (&fullpath[0], "/usr/local/share/NSB/PlayerPics");

    gothit = 0;
    if ((fnames = opendir (&fullpath[0])) != NULL) {
        while ((dir = readdir (fnames))) {
            if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, ".."))
                continue;

            if (!strcasecmp (dir->d_name, &fname[0])) {
                gothit = 1;
                strcat (&fullpath[0], "/");
                strcat (&fullpath[0], dir->d_name);
                break;
            }
        }
        closedir (fnames);
    }

    if (gothit) {
        picwin = gtk_dialog_new ();
        gtk_window_set_title (GTK_WINDOW (picwin), "Player Pic");
        gtk_window_set_default_size (GTK_WINDOW (picwin), 200, 130);
        gtk_window_set_transient_for (GTK_WINDOW (picwin), GTK_WINDOW (window));
        gtk_window_set_destroy_with_parent (GTK_WINDOW (picwin), TRUE);
        g_signal_connect (picwin, "destroy", G_CALLBACK (gtk_widget_destroyed), &picwin);

        strcpy (&work[0], &pname[0]);

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (picwin)->vbox), vbox, TRUE, TRUE, 0);
        label = g_object_new (GTK_TYPE_LABEL, "label", &work[0], NULL);
        gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (vbox), gtk_image_new_from_file (fullpath));
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (picwin)->vbox), vbox, TRUE, TRUE, 0);

        gtk_widget_show_all (picwin);
    }
}

void
FillRLPlayerStats (GtkTextBuffer *buf, char *name, int nbat, int npit) {
    GtkTextIter pos;
    gint x, y, newyr, singles, ts, tg, tab, tr, th, t2b, t3b, thr, tbi, tbb, tso, thbp, tgidp, tsb, tcs, tibb, tsf, tsh, tdp,
         tinn, tthirds, ter, tw, tl, tgs, tcg, tgf, tsho, tsopp, twp, thb, tb, tbfp, tpo, ta, te, tpb, topp_ab;

    tg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb = tdp =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    gtk_text_buffer_get_iter_at_offset (buf, &pos, 0);
    gtk_text_buffer_create_tag (buf, "monospace", "family", "monospace", NULL);

    strcpy (&stats[0], name);
    strcat (&stats[0], "\n");
    if (teamdata[0].batters.dob.year > 1800 && teamdata[0].batters.dob.year < 3000) {
        strcat (&stats[0], "Born - ");
        if (teamdata[0].batters.dob.month) {
            strcat (&stats[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.month), 'l'));
            strcat (&stats[0], "/");
            strcat (&stats[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.day), 'l'));
            strcat (&stats[0], "/");
        }
        strcat (&stats[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.year), 'l'));
        strcat (&stats[0], "\n");
    }
    strcat (&stats[0], "Bats - ");
    if (!teamdata[0].batters.id.batslr)
        strcat (&stats[0], "right");
    else
        if (teamdata[0].batters.id.batslr == 1)
            strcat (&stats[0], "left");
        else
            strcat (&stats[0], "both");
    strcat (&stats[0], ", Throws - ");
    if (!teamdata[0].batters.id.throwslr)
        strcat (&stats[0], "right");
    else
        strcat (&stats[0], "left");
    strcat (&stats[0], "\n\n");

    strcat (&stats[0], "(NOTE - These stats (including totals) do not include any years where the player may not have played ");
    strcat (&stats[0], "enough games in real life to be included on an NSB roster for that year.)\n\n");

    strcat (&stats[0], "Year Team                    G     AB      R      H     2B     3B     HR    RBI    BA    SA   OBA     BB     SO    HBP   GIDP     SB     CS    IBB     SH     SF\n\n");

    for (x = 0; x < nbat; x++) {
        singles = teamdata[x].batters.hitting.hits - (teamdata[x].batters.hitting.homers +
                  teamdata[x].batters.hitting.triples + teamdata[x].batters.hitting.doubles);

        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&stats[0], " ");
        strncat (&stats[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&stats[0], "                          ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&stats[0], (char *) check_stats (teamdata[x].batters.hitting.games, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.atbats, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.runs, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.hits, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.doubles, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.triples, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.homers, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.rbi, 'r'));
        strcat (&stats[0], (char *) do_average (teamdata[x].batters.hitting.hits, teamdata[x].batters.hitting.atbats));
        strcat (&stats[0], (char *) do_average (((teamdata[x].batters.hitting.homers * 4) + (teamdata[x].batters.hitting.triples * 3) +
                                                 (teamdata[x].batters.hitting.doubles * 2) + singles), teamdata[x].batters.hitting.atbats));
        if (teamdata[x].batters.hitting.sf == -1)
            y = 0;
        else
            y = teamdata[x].batters.hitting.sf;
        y += teamdata[x].batters.hitting.sh;
        strcat (&stats[0], (char *) do_average ((teamdata[x].batters.hitting.hits + teamdata[x].batters.hitting.bb + teamdata[x].batters.hitting.hbp),
                                               (teamdata[x].batters.hitting.atbats + teamdata[x].batters.hitting.bb + y + teamdata[x].batters.hitting.hbp)));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.bb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.so, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.hbp, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.gidp, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.cs, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.ibb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sh, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].batters.hitting.sf, 'r'));
        strcat (&stats[0], "\n");

        tg += teamdata[x].batters.hitting.games;
        tab += teamdata[x].batters.hitting.atbats;
        tr += teamdata[x].batters.hitting.runs;
        th += teamdata[x].batters.hitting.hits;
        t2b += teamdata[x].batters.hitting.doubles;
        t3b += teamdata[x].batters.hitting.triples;
        thr += teamdata[x].batters.hitting.homers;
        tbi += teamdata[x].batters.hitting.rbi;
        tbb += teamdata[x].batters.hitting.bb;
        if (teamdata[x].batters.hitting.so != -1)
            /* strikeouts for the batter wasn't always a recorded stat */
            tso += teamdata[x].batters.hitting.so;
        thbp += teamdata[x].batters.hitting.hbp;
        if (teamdata[x].batters.hitting.gidp != -1)
            /* grounded into DP for the batter wasn't always a recorded stat */
            tgidp += teamdata[x].batters.hitting.gidp;
        tsb += teamdata[x].batters.hitting.sb;
        if (teamdata[x].batters.hitting.cs != -1)
            /* caught stealing for the batter wasn't always a recorded stat */
            tcs += teamdata[x].batters.hitting.cs;
        if (teamdata[x].batters.hitting.ibb != -1)
            /* intentional walk for the batter wasn't always a recorded stat */
            tibb += teamdata[x].batters.hitting.ibb;
        tsh += teamdata[x].batters.hitting.sh;
        if (teamdata[x].batters.hitting.sf != -1)
            /* sacrifice flies for the batter wasn't always a recorded stat */
            tsf += teamdata[x].batters.hitting.sf;
        ts += singles;
    }

    strcat (&stats[0], "  TOTALS               ");
    strcat (&stats[0], check_stats (tg, 'r'));
    strcat (&stats[0], check_stats (tab, 'r'));
    strcat (&stats[0], check_stats (tr, 'r'));
    strcat (&stats[0], check_stats (th, 'r'));
    strcat (&stats[0], check_stats (t2b, 'r'));
    strcat (&stats[0], check_stats (t3b, 'r'));
    strcat (&stats[0], check_stats (thr, 'r'));
    strcat (&stats[0], check_stats (tbi, 'r'));
    strcat (&stats[0], (char *) do_average (th, tab));
    strcat (&stats[0], (char *) do_average (((thr * 4) + (t3b * 3) + (t2b * 2) + ts), tab));
    strcat (&stats[0], (char *) do_average ((th + tbb + thbp), (tab + tbb + tsf + tsh + thbp)));
    strcat (&stats[0], check_stats (tbb, 'r'));
    strcat (&stats[0], check_stats (tso, 'r'));
    strcat (&stats[0], check_stats (thbp, 'r'));
    strcat (&stats[0], check_stats (tgidp, 'r'));
    strcat (&stats[0], check_stats (tsb, 'r'));
    strcat (&stats[0], check_stats (tcs, 'r'));
    strcat (&stats[0], check_stats (tibb, 'r'));
    strcat (&stats[0], check_stats (tsh, 'r'));
    strcat (&stats[0], check_stats (tsf, 'r'));

    strcat (&stats[0], "\n\n\n\n");

    tg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb = tdp =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    if (npit)
        strcat (&stats[0], "Year Team                    G     IP        H      R     ER      W      L   PCT     SO     BB   ERA     GS     CG     GF    SHO      S   SOPP    IBB     WP     HB      B    BFP     2B     3B     HR    RBI     SB     CS     SH     SF   OPAB  OPBA\n\n");

    for (x = 0; x < npit; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].pitchers.id.teamid)
                break;

        strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].pitchers.id.year), 'l'));
        strcat (&stats[0], " ");
        strncat (&stats[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&stats[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&stats[0], (char *) check_stats (teamdata[x].pitchers.pitching.games, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.innings, 'r'));
        if (teamdata[x].pitchers.pitching.thirds == 1 || teamdata[x].pitchers.pitching.thirds == 2) {
            strcat (&stats[0], ".");
            strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.thirds, 'l'));
        }
        else
            strcat (&stats[0], "  ");
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.hits, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.runs, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.er, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.wins, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.losses, 'r'));
        strcat (&stats[0], (char *) do_average (teamdata[x].pitchers.pitching.wins, (teamdata[x].pitchers.pitching.wins +
                                           teamdata[x].pitchers.pitching.losses)));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.so, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.walks, 'r'));
        strcat (&stats[0], (char *) do_era (teamdata[x].pitchers.pitching.er * 9, teamdata[x].pitchers.pitching.innings,
                                                   teamdata[x].pitchers.pitching.thirds));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.games_started, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.cg, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.gf, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sho, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.saves, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.svopp, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.ibb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.wp, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.hb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.balks, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.bfp, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.doubles, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.triples, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.homers, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.rbi, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sb, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.cs, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sh, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.sf, 'r'));
        strcat (&stats[0], check_stats (teamdata[x].pitchers.pitching.opp_ab, 'r'));
        strcat (&stats[0], (char *) do_average (teamdata[x].pitchers.pitching.hits, teamdata[x].pitchers.pitching.opp_ab));
        strcat (&stats[0], "\n");

        tg += teamdata[x].pitchers.pitching.games;
        tinn += teamdata[x].pitchers.pitching.innings;
        tthirds += teamdata[x].pitchers.pitching.thirds;
        tr += teamdata[x].pitchers.pitching.runs;
        th += teamdata[x].pitchers.pitching.hits;
        ter += teamdata[x].pitchers.pitching.er;
        tw += teamdata[x].pitchers.pitching.wins;
        tl += teamdata[x].pitchers.pitching.losses;
        tbb += teamdata[x].pitchers.pitching.walks;
        tso += teamdata[x].pitchers.pitching.so;
        tgs += teamdata[x].pitchers.pitching.games_started;
        tcg += teamdata[x].pitchers.pitching.cg;
        tgf += teamdata[x].pitchers.pitching.gf;
        tsho += teamdata[x].pitchers.pitching.sho;
        ts += teamdata[x].pitchers.pitching.saves;
        if (teamdata[x].pitchers.pitching.svopp != -1)
            /* save opportunities for the pitcher wasn't always a recorded stat */
            tsopp += teamdata[x].pitchers.pitching.svopp;
        if (teamdata[x].pitchers.pitching.ibb != -1)
            /* intentional walks for the pitcher wasn't always a recorded stat */
            tibb += teamdata[x].pitchers.pitching.ibb;
        twp += teamdata[x].pitchers.pitching.wp;
        thb += teamdata[x].pitchers.pitching.hb;
        tb += teamdata[x].pitchers.pitching.balks;
        if (teamdata[x].pitchers.pitching.bfp != -1)
            /* batters facing pitcher wasn't always a recorded stat */
            tbfp += teamdata[x].pitchers.pitching.bfp;
        if (teamdata[x].pitchers.pitching.doubles != -1)
            /* doubles allowed for the pitcher wasn't always a recorded stat */
            t2b += teamdata[x].pitchers.pitching.doubles;
        if (teamdata[x].pitchers.pitching.triples != -1)
            /* triples allowed for the pitcher wasn't always a recorded stat */
            t3b += teamdata[x].pitchers.pitching.triples;
        thr += teamdata[x].pitchers.pitching.homers;
        if (teamdata[x].pitchers.pitching.rbi != -1)
            /* RBIs allowed for the pitcher wasn't always a recorded stat */
            tbi += teamdata[x].pitchers.pitching.rbi;
        if (teamdata[x].pitchers.pitching.sb != -1)
            /* stolen bases allowed for the pitcher wasn't always a recorded stat */
            tsb += teamdata[x].pitchers.pitching.sb;
        if (teamdata[x].pitchers.pitching.cs != -1)
            /* caught stealing against for the pitcher wasn't always a recorded stat */
            tcs += teamdata[x].pitchers.pitching.cs;
        if (teamdata[x].pitchers.pitching.sh != -1)
            /* sacrifice hits allowed for the pitcher wasn't always a recorded stat */
            tsh += teamdata[x].pitchers.pitching.sh;
        if (teamdata[x].pitchers.pitching.sf != -1)
            /* sacrifice flies allowed for the pitcher wasn't always a recorded stat */
            tsf += teamdata[x].pitchers.pitching.sf;
        if (teamdata[x].pitchers.pitching.opp_ab != -1)
            /* at bats against for the pitcher wasn't always a recorded stat */
            topp_ab += teamdata[x].pitchers.pitching.opp_ab;
    }

    if (npit) {
        strcat (&stats[0], "  TOTALS               ");
        strcat (&stats[0], check_stats (tg, 'r'));
        for (; tthirds > 2; tthirds -= 3)
            tinn++;
        strcat (&stats[0], check_stats (tinn, 'r'));
        if (tthirds == 1 || tthirds == 2) {
            strcat (&stats[0], ".");
            strcat (&stats[0], check_stats (tthirds, 'l'));
        }
        else
            strcat (&stats[0], "  ");
        strcat (&stats[0], check_stats (th, 'r'));
        strcat (&stats[0], check_stats (tr, 'r'));
        strcat (&stats[0], check_stats (ter, 'r'));
        strcat (&stats[0], check_stats (tw, 'r'));
        strcat (&stats[0], check_stats (tl, 'r'));
        strcat (&stats[0], (char *) do_average (tw, (tw + tl)));
        strcat (&stats[0], check_stats (tso, 'r'));
        strcat (&stats[0], check_stats (tbb, 'r'));
        strcat (&stats[0], (char *) do_era (ter * 9, tinn, tthirds));
        strcat (&stats[0], check_stats (tgs, 'r'));
        strcat (&stats[0], check_stats (tcg, 'r'));
        strcat (&stats[0], check_stats (tgf, 'r'));
        strcat (&stats[0], check_stats (tsho, 'r'));
        strcat (&stats[0], check_stats (ts, 'r'));
        strcat (&stats[0], check_stats (tsopp, 'r'));
        strcat (&stats[0], check_stats (tibb, 'r'));
        strcat (&stats[0], check_stats (twp, 'r'));
        strcat (&stats[0], check_stats (thb, 'r'));
        strcat (&stats[0], check_stats (tb, 'r'));
        strcat (&stats[0], check_stats (tbfp, 'r'));
        strcat (&stats[0], check_stats (t2b, 'r'));
        strcat (&stats[0], check_stats (t3b, 'r'));
        strcat (&stats[0], check_stats (thr, 'r'));
        strcat (&stats[0], check_stats (tbi, 'r'));
        strcat (&stats[0], check_stats (tsb, 'r'));
        strcat (&stats[0], check_stats (tcs, 'r'));
        strcat (&stats[0], check_stats (tsh, 'r'));
        strcat (&stats[0], check_stats (tsf, 'r'));
        strcat (&stats[0], check_stats (topp_ab, 'r'));
        if (topp_ab)
            strcat (&stats[0], (char *) do_average (th, topp_ab));

        strcat (&stats[0], "\n\n\n\n");
    }

    tg = 0;
    strcat (&stats[0], "Year Team             POS      G     PO     DP      A      E     PB   FA\n\n");

    for (x = 0; x < nbat; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&stats[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&stats[0], " ");
        strncat (&stats[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&stats[0], "                          ", 18 - strlen (&teaminfo[y].teamname[0]));

        for (newyr = y = 0; y < 11; y++)
            if (teamdata[x].batters.fielding[y].games > 0) {
                if (newyr == 1)
                    strcat (&stats[0], "                       ");
                if (y == 10)
                    strcat (&stats[0], "OF");
                else
                    strcat (&stats[0], figure_pos (y));
                strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].games, 'r'));
                strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].po, 'r'));
                strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].dp, 'r'));
                strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].a, 'r'));
                strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].e, 'r'));
                if (y == 2)
                    strcat (&stats[0], check_stats (teamdata[x].batters.fielding[y].pb, 'r'));
                else
                    strcat (&stats[0], "       ");
                strcat (&stats[0], (char *) do_average ((teamdata[x].batters.fielding[y].po + teamdata[x].batters.fielding[y].a),
                    (teamdata[x].batters.fielding[y].po + teamdata[x].batters.fielding[y].a + teamdata[x].batters.fielding[y].e)));
                newyr = 1;
                strcat (&stats[0], "\n");

                if (teamdata[x].batters.fielding[y].po != -1)
                    tg += teamdata[x].batters.fielding[y].games;
                if (teamdata[x].batters.fielding[y].po != -1)
                    tpo += teamdata[x].batters.fielding[y].po;
                if (teamdata[x].batters.fielding[y].dp != -1)
                    tdp += teamdata[x].batters.fielding[y].dp;
                if (teamdata[x].batters.fielding[y].a != -1)
                    ta += teamdata[x].batters.fielding[y].a;
                if (teamdata[x].batters.fielding[y].e != -1)
                    te += teamdata[x].batters.fielding[y].e;
                if (y == 2)
                    tpb += teamdata[x].batters.fielding[y].pb;
            }
        if (!newyr)
            strcat (&stats[0], "\n");
    }

    strcat (&stats[0], "  TOTALS                 ");
    strcat (&stats[0], check_stats (tg, 'r'));
    strcat (&stats[0], check_stats (tpo, 'r'));
    strcat (&stats[0], check_stats (tdp, 'r'));
    strcat (&stats[0], check_stats (ta, 'r'));
    strcat (&stats[0], check_stats (te, 'r'));
    strcat (&stats[0], check_stats (tpb, 'r'));
    strcat (&stats[0], (char *) do_average ((tpo + ta), (tpo + ta + te)));

    strcat (&stats[0], "\n\n");

    gtk_text_buffer_insert_with_tags_by_name (buf, &pos, stats, -1, "monospace", NULL);
}

void
PrintRLPlayerStats (GtkWidget *widget, gpointer cnt) {
    /* the player stats need to be reformatted for printing */
    gchar work1[100], name[100], borp, Printing[256] = "Printing Stats ...", *msg[5];
    gint x, y, npit, nbat, bufpnt, singles, newyr, ts, tg, tab, tr, th, t2b, t3b, thr, tbi, tbb, tso, thbp, tgidp, tsb, tcs, icnt = GPOINTER_TO_INT (cnt),
         tibb, tsf, tsh, tdp, tinn, tthirds, ter, tw, tl, tgs, tcg, tgf, tsho, tsopp, twp, thb, tb, tbfp, tpo, ta, te, tpb, topp_ab;

    tg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb = tdp =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print stats.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print stats.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&work1[0], &prtbutrlpcmd[icnt][0]);

    sock_puts (sock, work1);  /* tell the server to send us the player stats again */
    strcpy (&name[0], &work1[2]);
    name[strlen (&name[0]) - 1] = '\0';
    sock_gets (sock, &buffer[0], sizeof (buffer));
    sock_gets (sock, &buffer[0], sizeof (buffer));

    nbat = npit = bufpnt = 0;

    do {
        if (!strncmp (&buffer[bufpnt], "BSTATS", 6))
            borp = 'B';
        else
            borp = 'P';
        bufpnt += 7;
        strncpy (&work[0], &buffer[bufpnt], 4);
        work[4] = '\0';
        teamdata[nbat].batters.id.year = teamdata[npit].pitchers.id.year = atoi (&work[0]);
        bufpnt += 5;
        strncpy (&work[0], &buffer[bufpnt], 2);
        work[2] = '\0';

        teamdata[nbat].batters.id.teamid = teamdata[npit].pitchers.id.teamid = atoi (&work[0]);
        bufpnt += 3;
        teamdata[nbat].batters.id.batslr = teamdata[npit].pitchers.id.batslr = buffer[bufpnt] - '0';
        teamdata[nbat].batters.id.throwslr = teamdata[npit].pitchers.id.throwslr = buffer[bufpnt + 2] - '0';

        if (borp == 'B') {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.atbats = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.runs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.hits = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.doubles = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.triples = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.homers = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.rbi = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.bb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.so = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.hbp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.gidp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.cs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.ibb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sh = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[nbat].batters.hitting.sf = atoi (&buffer[0]);
 
            for (x = 0; x < 11; x++) {
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].games = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].po = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].dp = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].a = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].pb = atoi (&buffer[0]);
                sock_gets (sock, &buffer[0], sizeof (buffer));
                teamdata[nbat].batters.fielding[x].e = atoi (&buffer[0]);
            }

            nbat++;
        }
        else {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.games_started = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.innings = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.thirds = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.wins = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.losses = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.saves = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.bfp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.hits = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.doubles = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.triples = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.homers = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.runs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.er = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.rbi = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.cg = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.gf = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sho = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.svopp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.cs = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.walks = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.so = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.ibb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sh = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.sf = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.wp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.balks = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.hb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            teamdata[npit].pitchers.pitching.opp_ab = atoi (&buffer[0]);

            npit++;
        }

        sock_gets (sock, &buffer[0], sizeof (buffer));
        bufpnt = 0;
    } while (strncmp (&buffer[0], "-1", 2));

    strcpy (&teamtotinfo[0], name);
    strcat (&teamtotinfo[0], "\n");
    if (teamdata[0].batters.dob.year > 1800 && teamdata[0].batters.dob.year < 3000) {
        strcat (&teamtotinfo[0], "Born - ");
        if (teamdata[0].batters.dob.month) {
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.month), 'l'));
            strcat (&teamtotinfo[0], "/");
            strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.day), 'l'));
            strcat (&teamtotinfo[0], "/");
        }
        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[0].batters.dob.year), 'l'));
        strcat (&teamtotinfo[0], "\n");
    }
    strcat (&teamtotinfo[0], "Bats - ");
    if (!teamdata[0].batters.id.batslr)
        strcat (&teamtotinfo[0], "right");
    else
        if (teamdata[0].batters.id.batslr == 1)
            strcat (&teamtotinfo[0], "left");
        else
            strcat (&teamtotinfo[0], "both");
    strcat (&teamtotinfo[0], ", Throws - ");
    if (!teamdata[0].pitchers.id.throwslr)
        strcat (&teamtotinfo[0], "right");
    else
        strcat (&teamtotinfo[0], "left");
    strcat (&teamtotinfo[0], "\n\n");

    strcat (&teamtotinfo[0], "(NOTE - These stats (including totals) do not include any years where the player\n");
    strcat (&teamtotinfo[0], "may not have played enough games in real life to be included on the NSB\n");
    strcat (&teamtotinfo[0], "roster for that year.)\n\n");

    strcat (&teamtotinfo[0], "Year Team                    G     AB      R      H     2B     3B     HR    RBI\n\n");

    for (x = 0; x < nbat; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.games, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.atbats, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.runs, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.hits, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.doubles, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.triples, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.homers, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.rbi, 'r'));

        strcat (&teamtotinfo[0], "\n");

        tg += teamdata[x].batters.hitting.games;
        tab += teamdata[x].batters.hitting.atbats;
        tr += teamdata[x].batters.hitting.runs;
        th += teamdata[x].batters.hitting.hits;
        t2b += teamdata[x].batters.hitting.doubles;
        t3b += teamdata[x].batters.hitting.triples;
        thr += teamdata[x].batters.hitting.homers;
        tbi += teamdata[x].batters.hitting.rbi;
    }

    strcat (&teamtotinfo[0], "  TOTALS               ");
    strcat (&teamtotinfo[0], check_stats (tg, 'r'));
    strcat (&teamtotinfo[0], check_stats (tab, 'r'));
    strcat (&teamtotinfo[0], check_stats (tr, 'r'));
    strcat (&teamtotinfo[0], check_stats (th, 'r'));
    strcat (&teamtotinfo[0], check_stats (t2b, 'r'));
    strcat (&teamtotinfo[0], check_stats (t3b, 'r'));
    strcat (&teamtotinfo[0], check_stats (thr, 'r'));
    strcat (&teamtotinfo[0], check_stats (tbi, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Year Team                    BA    SA   OBA     BB     SO    HBP   GIDP\n\n");

    for (x = 0; x < nbat; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 20);
        if (strlen (&teaminfo[y].teamname[0]) < 20)
            strncat (&teamtotinfo[0], "                       ", 20 - strlen (&teaminfo[y].teamname[0]));

        singles = teamdata[x].batters.hitting.hits - (teamdata[x].batters.hitting.homers +
                  teamdata[x].batters.hitting.triples + teamdata[x].batters.hitting.doubles);

        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].batters.hitting.hits, teamdata[x].batters.hitting.atbats));
        strcat (&teamtotinfo[0], (char *) do_average (((teamdata[x].batters.hitting.homers * 4) +
                                                 (teamdata[x].batters.hitting.triples * 3) + (teamdata[x].batters.hitting.doubles * 2) + singles),
                                                  teamdata[x].batters.hitting.atbats));
        if (teamdata[x].batters.hitting.sf == -1)
            y = 0;
        else
            y = teamdata[x].batters.hitting.sf;
        y += teamdata[x].batters.hitting.sh;
        strcat (&teamtotinfo[0], (char *) do_average ((teamdata[x].batters.hitting.hits + teamdata[x].batters.hitting.bb +
                                                 teamdata[x].batters.hitting.hbp), (teamdata[x].batters.hitting.atbats + teamdata[x].batters.hitting.bb + y +
                                                  teamdata[x].batters.hitting.hbp)));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.bb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.so, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.hbp, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.gidp, 'r'));

        strcat (&teamtotinfo[0], "\n");

        tbb += teamdata[x].batters.hitting.bb;
        if (teamdata[x].batters.hitting.so != -1)
            /* strikeouts for the batter wasn't always a recorded stat */
            tso += teamdata[x].batters.hitting.so;
        thbp += teamdata[x].batters.hitting.hbp;
        if (teamdata[x].batters.hitting.gidp != -1)
            /* grounded into DP for the batter wasn't always a recorded stat */
            tgidp += teamdata[x].batters.hitting.gidp;
        ts += singles;
    }

    strcat (&teamtotinfo[0], "  TOTALS                 ");
    strcat (&teamtotinfo[0], (char *) do_average (th, tab));
    strcat (&teamtotinfo[0], (char *) do_average (((thr * 4) + (t3b * 3) + (t2b * 2) + ts), tab));
    strcat (&teamtotinfo[0], (char *) do_average ((th + tbb + thbp), (tab + tbb + tsf + tsh + thbp)));
    strcat (&teamtotinfo[0], check_stats (tbb, 'r'));
    strcat (&teamtotinfo[0], check_stats (tso, 'r'));
    strcat (&teamtotinfo[0], check_stats (thbp, 'r'));
    strcat (&teamtotinfo[0], check_stats (tgidp, 'r'));

    strcat (&teamtotinfo[0], "\n\n");
    strcat (&teamtotinfo[0], "Year Team                          SB     CS    IBB     SH     SF\n\n");

    for (x = 0; x < nbat; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 25);
        if (strlen (&teaminfo[y].teamname[0]) < 25)
            strncat (&teamtotinfo[0], "                       ", 25 - strlen (&teaminfo[y].teamname[0]));

        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.cs, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.ibb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sh, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.hitting.sf, 'r'));
        strcat (&teamtotinfo[0], "\n");

        tsb += teamdata[x].batters.hitting.sb;
        if (teamdata[x].batters.hitting.cs != -1)
            /* caught stealing for the batter wasn't always a recorded stat */
            tcs += teamdata[x].batters.hitting.cs;
        if (teamdata[x].batters.hitting.ibb != -1)
            /* intentional walk for the batter wasn't always a recorded stat */
            tibb += teamdata[x].batters.hitting.ibb;
        tsh += teamdata[x].batters.hitting.sh;
        if (teamdata[x].batters.hitting.sf != -1)
            /* sacrifice flies for the batter wasn't always a recorded stat */
            tsf += teamdata[x].batters.hitting.sf;
    }

    strcat (&teamtotinfo[0], "  TOTALS                      ");
    strcat (&teamtotinfo[0], check_stats (tsb, 'r'));
    strcat (&teamtotinfo[0], check_stats (tcs, 'r'));
    strcat (&teamtotinfo[0], check_stats (tibb, 'r'));
    strcat (&teamtotinfo[0], check_stats (tsh, 'r'));
    strcat (&teamtotinfo[0], check_stats (tsf, 'r'));

    strcat (&teamtotinfo[0], "\n\n\n");

    tg = tab = tr = th = t2b = t3b = thr = tbi = tbb = tso = thbp = tgidp = tpo = tpb = tdp =
          tsb = tcs = tibb = tsf = tsh = ts = tinn = tthirds = ter = ta = te = topp_ab =
          tw = tl = tgs = tcg = tgf = tsho = tsopp = twp = thb = tb = tbfp = 0;
    if (npit)
        strcat (&teamtotinfo[0], "Year Team                    G     IP        H      R     ER      W      L   PCT\n\n");

    for (x = 0; x < npit; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        y = (int) (teamdata[x].pitchers.pitching.thirds / 3);
        teamdata[x].pitchers.pitching.innings += y;
        teamdata[x].pitchers.pitching.thirds %= 3;
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.games, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.innings, 'r'));
        if (teamdata[x].pitchers.pitching.thirds == 1 || teamdata[x].pitchers.pitching.thirds == 2) {
            strcat (&teamtotinfo[0], ".");
            strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.thirds, 'l'));
        }
        else
            strcat (&teamtotinfo[0], "  ");
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.hits, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.runs, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.er, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.wins, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.losses, 'r'));
        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].pitchers.pitching.wins, (teamdata[x].pitchers.pitching.wins +
                                           teamdata[x].pitchers.pitching.losses)));
        strcat (&teamtotinfo[0], "\n");

        tg += teamdata[x].pitchers.pitching.games;
        tinn += teamdata[x].pitchers.pitching.innings;
        tthirds += teamdata[x].pitchers.pitching.thirds;
        tr += teamdata[x].pitchers.pitching.runs;
        th += teamdata[x].pitchers.pitching.hits;
        ter += teamdata[x].pitchers.pitching.er;
        tw += teamdata[x].pitchers.pitching.wins;
        tl += teamdata[x].pitchers.pitching.losses;
    }

    if (npit) {
        strcat (&teamtotinfo[0], "  TOTALS               ");
        strcat (&teamtotinfo[0], check_stats (tg, 'r'));
        for (; tthirds > 2; tthirds -= 3)
            tinn++;
        strcat (&teamtotinfo[0], check_stats (tinn, 'r'));
        if (tthirds == 1 || tthirds == 2) {
            strcat (&teamtotinfo[0], ".");
            strcat (&teamtotinfo[0], check_stats (tthirds, 'l'));
        }
        else
            strcat (&teamtotinfo[0], "  ");
        strcat (&teamtotinfo[0], check_stats (th, 'r'));
        strcat (&teamtotinfo[0], check_stats (tr, 'r'));
        strcat (&teamtotinfo[0], check_stats (ter, 'r'));
        strcat (&teamtotinfo[0], check_stats (tw, 'r'));
        strcat (&teamtotinfo[0], check_stats (tl, 'r'));
        strcat (&teamtotinfo[0], (char *) do_average (tw, (tw + tl)));

        strcat (&teamtotinfo[0], "\n\n");
    }

    if (npit) {
        strcat (&teamtotinfo[0], "\n\n");
        strcat (&teamtotinfo[0], "Year Team                   SO     BB   ERA     GS     CG     SHO     S    SOPP\n\n");
    }

    for (x = 0; x < npit; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.so, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.walks, 'r'));
        strcat (&teamtotinfo[0], (char *) do_era (teamdata[x].pitchers.pitching.er * 9, teamdata[x].pitchers.pitching.innings,
                                                   teamdata[x].pitchers.pitching.thirds));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.games_started, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.cg, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sho, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.saves, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.svopp, 'r'));
        strcat (&teamtotinfo[0], "\n");

        tso += teamdata[x].pitchers.pitching.so;
        tbb += teamdata[x].pitchers.pitching.walks;
        tgs += teamdata[x].pitchers.pitching.games_started;
        tcg += teamdata[x].pitchers.pitching.cg;
        tsho += teamdata[x].pitchers.pitching.sho;
        ts += teamdata[x].pitchers.pitching.saves;
        if (teamdata[x].pitchers.pitching.svopp != -1)
            /* save opportunities for the pitcher wasn't always a recorded stat */
            tsopp += teamdata[x].pitchers.pitching.svopp;
    }

    if (npit) {
        strcat (&teamtotinfo[0], "  TOTALS               ");
        strcat (&teamtotinfo[0], check_stats (tso, 'r'));
        strcat (&teamtotinfo[0], check_stats (tbb, 'r'));
        strcat (&teamtotinfo[0], (char *) do_era (ter * 9, tinn, tthirds));
        strcat (&teamtotinfo[0], check_stats (tgs, 'r'));
        strcat (&teamtotinfo[0], check_stats (tcg, 'r'));
        strcat (&teamtotinfo[0], check_stats (tsho, 'r'));
        strcat (&teamtotinfo[0], check_stats (ts, 'r'));
        strcat (&teamtotinfo[0], check_stats (tsopp, 'r'));

        strcat (&teamtotinfo[0], "\n\n");
    }

    if (npit) {
        strcat (&teamtotinfo[0], "\n\n");
        strcat (&teamtotinfo[0], "Year Team                   GF    IBB     WP     HB      B    BFP     SH     SF\n\n");
    }

    for (x = 0; x < npit; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.gf, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.ibb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.wp, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.hb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.balks, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.bfp, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sh, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sf, 'r'));
        strcat (&teamtotinfo[0], "\n");

        tgf += teamdata[x].pitchers.pitching.gf;
        if (teamdata[x].pitchers.pitching.ibb != -1)
            /* intentional walks for the pitcher wasn't always a recorded stat */
            tibb += teamdata[x].pitchers.pitching.ibb;
        twp += teamdata[x].pitchers.pitching.wp;
        thb += teamdata[x].pitchers.pitching.hb;
        tb += teamdata[x].pitchers.pitching.balks;
        if (teamdata[x].pitchers.pitching.bfp != -1)
            /* batters facing pitcher wasn't always a recorded stat */
            tbfp += teamdata[x].pitchers.pitching.bfp;
        if (teamdata[x].pitchers.pitching.sh != -1)
            /* sacrifice hits allowed for the pitcher wasn't always a recorded stat */
            tsh += teamdata[x].pitchers.pitching.sh;
        if (teamdata[x].pitchers.pitching.sf != -1)
            /* sacrifice flies allowed for the pitcher wasn't always a recorded stat */
            tsf += teamdata[x].pitchers.pitching.sf;
    }

    if (npit) {
        strcat (&teamtotinfo[0], "  TOTALS               ");
        strcat (&teamtotinfo[0], check_stats (tgf, 'r'));
        strcat (&teamtotinfo[0], check_stats (tibb, 'r'));
        strcat (&teamtotinfo[0], check_stats (twp, 'r'));
        strcat (&teamtotinfo[0], check_stats (thb, 'r'));
        strcat (&teamtotinfo[0], check_stats (tb, 'r'));
        strcat (&teamtotinfo[0], check_stats (tbfp, 'r'));
        strcat (&teamtotinfo[0], check_stats (tcs, 'r'));
        strcat (&teamtotinfo[0], check_stats (tsh, 'r'));
        strcat (&teamtotinfo[0], check_stats (tsf, 'r'));

        strcat (&teamtotinfo[0], "\n\n");
    }

    if (npit) {
        strcat (&teamtotinfo[0], "\n\n");
        strcat (&teamtotinfo[0], "Year Team                   2B     3B     HR    RBI     SB     CS   OPAB  OPBA\n\n");
    }

    for (x = 0; x < npit; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 18);
        if (strlen (&teaminfo[y].teamname[0]) < 18)
            strncat (&teamtotinfo[0], "                       ", 18 - strlen (&teaminfo[y].teamname[0]));

        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.doubles, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.triples, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.homers, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.rbi, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.sb, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.cs, 'r'));
        strcat (&teamtotinfo[0], check_stats (teamdata[x].pitchers.pitching.opp_ab, 'r'));
        strcat (&teamtotinfo[0], (char *) do_average (teamdata[x].pitchers.pitching.hits, teamdata[x].pitchers.pitching.opp_ab));
        strcat (&teamtotinfo[0], "\n");

        if (teamdata[x].pitchers.pitching.doubles != -1)
            /* doubles allowed for the pitcher wasn't always a recorded stat */
            t2b += teamdata[x].pitchers.pitching.doubles;
        if (teamdata[x].pitchers.pitching.triples != -1)
            /* triples allowed for the pitcher wasn't always a recorded stat */
            t3b += teamdata[x].pitchers.pitching.triples;
        thr += teamdata[x].pitchers.pitching.homers;
        if (teamdata[x].pitchers.pitching.rbi != -1)
            /* RBIs allowed for the pitcher wasn't always a recorded stat */
            tbi += teamdata[x].pitchers.pitching.rbi;
        if (teamdata[x].pitchers.pitching.sb != -1)
            /* stolen bases allowed for the pitcher wasn't always a recorded stat */
            tsb += teamdata[x].pitchers.pitching.sb;
        if (teamdata[x].pitchers.pitching.cs != -1)
            /* caught stealing against for the pitcher wasn't always a recorded stat */
            tcs += teamdata[x].pitchers.pitching.cs;
        if (teamdata[x].pitchers.pitching.opp_ab != -1)
            /* at bats against for the pitcher wasn't always a recorded stat */
            topp_ab += teamdata[x].pitchers.pitching.opp_ab;
    }

    if (npit) {
        strcat (&teamtotinfo[0], "  TOTALS               ");
        strcat (&teamtotinfo[0], check_stats (t2b, 'r'));
        strcat (&teamtotinfo[0], check_stats (t3b, 'r'));
        strcat (&teamtotinfo[0], check_stats (thr, 'r'));
        strcat (&teamtotinfo[0], check_stats (tbi, 'r'));
        strcat (&teamtotinfo[0], check_stats (tsb, 'r'));
        strcat (&teamtotinfo[0], check_stats (tcs, 'r'));
        strcat (&teamtotinfo[0], check_stats (topp_ab, 'r'));
        if (topp_ab)
            strcat (&teamtotinfo[0], (char *) do_average (th, topp_ab));

        strcat (&teamtotinfo[0], "\n\n\n\n");
    }

    tg = 0;
    strcat (&teamtotinfo[0], "Year Team                    POS      G     PO     DP      A      E     PB   FA\n\n");

    for (x = 0; x < nbat; x++) {
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamdata[x].batters.id.teamid)
                break;

        strcat (&teamtotinfo[0], (char *) cnvt_int2str ((teamdata[x].batters.id.year), 'l'));
        strcat (&teamtotinfo[0], " ");
        strncat (&teamtotinfo[0], &teaminfo[y].teamname[0], 25);
        if (strlen (&teaminfo[y].teamname[0]) < 25)
            strncat (&teamtotinfo[0], "                              ", 25 - strlen (&teaminfo[y].teamname[0]));

        for (newyr = y = 0; y < 11; y++)
            if (teamdata[x].batters.fielding[y].games > 0) {
                if (newyr == 1)
                    strcat (&teamtotinfo[0], "                              ");
                if (y == 10)
                    strcat (&teamtotinfo[0], "OF");
                else
                    strcat (&teamtotinfo[0], figure_pos (y));

                strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].games, 'r'));
                strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].po, 'r'));
                strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].dp, 'r'));
                strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].a, 'r'));
                strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].e, 'r'));
                if (y == 2)
                    strcat (&teamtotinfo[0], check_stats (teamdata[x].batters.fielding[y].pb, 'r'));
                else
                    strcat (&teamtotinfo[0], "       ");
                strcat (&teamtotinfo[0], (char *) do_average ((teamdata[x].batters.fielding[y].po + teamdata[x].batters.fielding[y].a),
                          (teamdata[x].batters.fielding[y].po + teamdata[x].batters.fielding[y].a + teamdata[x].batters.fielding[y].e)));
                newyr = 1;
                strcat (&teamtotinfo[0], "\n");

                if (teamdata[x].batters.fielding[y].po != -1)
                    tg += teamdata[x].batters.fielding[y].games;
                if (teamdata[x].batters.fielding[y].po != -1)
                    tpo += teamdata[x].batters.fielding[y].po;
                if (teamdata[x].batters.fielding[y].dp != -1)
                    tdp += teamdata[x].batters.fielding[y].dp;
                if (teamdata[x].batters.fielding[y].a != -1)
                    ta += teamdata[x].batters.fielding[y].a;
                if (teamdata[x].batters.fielding[y].e != -1)
                    te += teamdata[x].batters.fielding[y].e;
                if (y == 2)
                    tpb += teamdata[x].batters.fielding[y].pb;
            }
        if (!newyr)
            strcat (&teamtotinfo[0], "\n");
    }

    strcat (&teamtotinfo[0], "  TOTALS                        ");
    strcat (&teamtotinfo[0], check_stats (tg, 'r'));
    strcat (&teamtotinfo[0], check_stats (tpo, 'r'));
    strcat (&teamtotinfo[0], check_stats (tdp, 'r'));
    strcat (&teamtotinfo[0], check_stats (ta, 'r'));
    strcat (&teamtotinfo[0], check_stats (te, 'r'));
    strcat (&teamtotinfo[0], check_stats (tpb, 'r'));
    strcat (&teamtotinfo[0], (char *) do_average ((tpo + ta), (tpo + ta + te)));

    strcat (&teamtotinfo[0], "\n\n");

    print (&teamtotinfo[0]);

    strcpy (&work[0], "Print Player Stats.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

struct {
    int m, d, y;
} dob[50];

/* user picks which player for which to get stats */
int
PickAPlayer () {
    gint x, dobct;
    gchar labeltext[500], dobm[3], dobd[3], doby[5];
    GtkWidget *dlgFile, *label = NULL;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Pick a Player");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    strcpy (&labeltext[0], "\nThere are multiple players with the name you entered.\nClick on the player you want.\n\n");
    label = gtk_label_new (labeltext);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    for (dobct = 0, x = 5; x < strlen (&buffer[0]); dobct++, x += 9) {
        strncpy (&dobm[0], &buffer[x], 2);
        dobm[2] = '\0';
        strncpy (&dobd[0], &buffer[x + 2], 2);
        dobd[2] = '\0';
        strncpy (&doby[0], &buffer[x + 4], 4);
        doby[4] = '\0';

        dob[dobct].m = atoi (&dobm[0]);
        dob[dobct].d = atoi (&dobd[0]);
        dob[dobct].y = atoi (&doby[0]);

        strcpy (&labeltext[0], &name[0]);
        strcat (&labeltext[0], "\nDate of Birth - ");
        strcat (&labeltext[0], &dobm[0]);
        strcat (&labeltext[0], "/");
        strcat (&labeltext[0], &dobd[0]);
        strcat (&labeltext[0], "/");
        strcat (&labeltext[0], &doby[0]);

        gtk_dialog_add_button (GTK_DIALOG (dlgFile), labeltext, dobct);
    }

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, -1);

    gtk_widget_show_all (dlgFile);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    if (x == -1)
        return 0;
    else {
        papm = dob[x].m;
        papd = dob[x].d;
        papy = dob[x].y;
        return 1;
    }
}

