/* determine how user wants to form his season */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

GtkWidget *vbox2, *dlgFile, *exentry, *inentry;
gint toggle_ha, yearsw, randomsw, teamsw;
gint yrs[YEAR_SPREAD] /* 1901-MAX-YEAR */ ;
const gchar *inentry_text, *exentry_text;

void
SetUpLeagueChoices (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *vbox, *hbox, *comboLEAGUE, *comboNOLEAGUE, *comboNODIV, *comboNOTEAM, *comboDH, *comboWC, *comboTNAME, *comboSPSR1, *comboSPSR2,
              *comboPSR1, *comboPSR2, *comboPSR3, *comboPSR4, *comboTDH, *comboTWC, *separator, *label, *buttonh, *buttona, *buttonyr, *buttonrand,
              *buttonteam, *table, *sw;
    GSList *group, *group2;
    gchar w[5], yeara[5], work[5000], *msg[5], stn[NUMBER_OF_TEAMS][100];
    gint x, y, year;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;
    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

    if (ThreadRunning) {
        msg[0] = "You cannot set up a season while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before setting up a season.";
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

    /* check if user has a series going on the connected server */
    sock_puts (sock, "S0\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "OK")) {
        msg[0] = "There is a series for you in progress on this server. ";
        msg[1] = "There cannot be both a season and a series in progress simultaneously. ";
        msg[2] = "If you continue the current series will be lost. ";
        msg[3] = "Continue? ";
        if (!ShallWeContinue (msg)) {
            close (fdlock);
            return;
        }
    }

    /* check if user has a season going on the connected server */
    sock_puts (sock, "S7\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "OK")) {
        gchar *msg[5];
        gint x;

        for (x = 0; x < 5; x ++)
            msg[x] = NULL;

        msg[0] = "A season is currently proceding for you on this server.  ";
        msg[1] = "Do you still want to set up a new season?  ";
        msg[2] = "(If so, the current season and all stats for the current season will be lost.) ";
        if (!ShallWeContinue (msg)) {
            close (fdlock);
            return;
        }
    }

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "How to Establish Season");
    gtk_window_set_deletable (GTK_WINDOW (dlgFile), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (dlgFile), 300, 200);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), sw, TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), vbox);

    label = gtk_label_new ("Select one:");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_container_add (GTK_CONTAINER (vbox), label);

    buttonh = gtk_radio_button_new_with_label (NULL, "By Hand");
    gtk_box_pack_start (GTK_BOX (vbox), buttonh, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonh), "clicked", G_CALLBACK (toggle_method_h), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (buttonh));
    buttona = gtk_radio_button_new_with_label (group, "Auto Set-Up");
    gtk_box_pack_start (GTK_BOX (vbox), buttona, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttona), "clicked", G_CALLBACK (toggle_method_a), NULL);

    vbox2 = gtk_vbox_new (FALSE, 8);
    gtk_signal_connect (GTK_OBJECT (vbox2), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
    gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    buttonyr = gtk_radio_button_new_with_label (NULL, "By Real Life Year:  ");
    gtk_box_pack_start (GTK_BOX (hbox), buttonyr, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonyr), "clicked", G_CALLBACK (CBckyear), NULL);
    group2 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (buttonyr));
    yearsw = 1;
    teamsw = randomsw = 0;

    comboLEAGUE = gtk_combo_box_new_text ();

    for (year = 1901; year < (MAX_YEAR + 1); year++) {
        strncpy (&w[0], (char *) cnvt_int2str (year, 'l'), 4);
        w[4] = '\0';
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboLEAGUE), w);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboLEAGUE), 55);
    gtk_container_add (GTK_CONTAINER (hbox), comboLEAGUE);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    buttonteam = gtk_radio_button_new_with_label (group2, "Same Team, Multiple Years:  ");
    gtk_box_pack_start (GTK_BOX (hbox), buttonteam, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonteam), "clicked", G_CALLBACK (CBckteam), NULL);

    comboTNAME = gtk_combo_box_new_text ();

    for (x = 1; x <= NUMBER_OF_TEAMS; x++) {
        strcpy (&stn[x][0], &teaminfo[x].teamname[0]);
        if (teaminfo[x].yrspan[0] == -1)
            strcat (&stn[x][0], ", no years");
        else {
            strcat (&stn[x][0], ", ");
            strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[0], 'l'));
            strcat (&stn[x][0], " - ");
            strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[1], 'l'));
            if (teaminfo[x].yrspan[2]) {
                strcat (&stn[x][0], ", ");
                strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[2], 'l'));
                strcat (&stn[x][0], " - ");
                strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[3], 'l'));
            }
        }
    }
    /* sort team names */
    /* we're not using USER CREATED so we can use that slot for work */
    for (x = 1; x < NUMBER_OF_TEAMS; x++)
        for (y = x + 1; y < (NUMBER_OF_TEAMS + 1); y++) {
            if (strcmp (&stn[x][0], &stn[y][0]) > 0) {
                strcpy (&stn[0][0], &stn[x][0]);
                strcpy (&stn[x][0], &stn[y][0]);
                strcpy (&stn[y][0], &stn[0][0]);
            }
        }

    for (x = 1; x <= NUMBER_OF_TEAMS; x++)
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboTNAME), stn[x]);
    gtk_container_add (GTK_CONTAINER (hbox), comboTNAME);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboTNAME), NUMBER_OF_TEAMS / 2);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Exclude Year(s):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    label = gtk_label_new_with_mnemonic ("Include Year(s):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

    exentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (exentry), 100);
    gtk_table_attach_defaults (GTK_TABLE (table), exentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), exentry);
    gtk_entry_set_text (GTK_ENTRY (exentry), "1901-1919 1942-1945");
    gtk_signal_connect (GTK_OBJECT (exentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntry), NULL);

    gtk_entry_set_activates_default (GTK_ENTRY (exentry), TRUE);

    inentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (inentry), 100);
    gtk_table_attach_defaults (GTK_TABLE (table), inentry, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), inentry);
    gtk_entry_set_text (GTK_ENTRY (inentry), "1901-2018");
    gtk_signal_connect (GTK_OBJECT (inentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntry), NULL);

    gtk_entry_set_activates_default (GTK_ENTRY (inentry), TRUE);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Use DH ?");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboTDH = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboTDH), "yes");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboTDH), "no");
    gtk_container_add (GTK_CONTAINER (hbox), comboTDH);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboTDH), 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Number Wild Card Teams in Post-Season:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboTWC = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboTWC), "0");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboTWC), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboTWC), "2");
    gtk_container_add (GTK_CONTAINER (hbox), comboTWC);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboTWC), 1);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("PS, Round 1 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboSPSR1 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR1), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR1), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR1), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR1), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR1), "9");
    gtk_container_add (GTK_CONTAINER (hbox), comboSPSR1);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboSPSR1), 3);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("PS, Round 2 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboSPSR2 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "No Round");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboSPSR2), "9");
    gtk_container_add (GTK_CONTAINER (hbox), comboSPSR2);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboSPSR2), 4);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("(Notes - season will be limited to 1 league, 1 division and 50 teams; if 0 Wild Card teams the top 2 regular season teams will go to the post-season)");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    buttonrand = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonteam), "Random:  ");
    gtk_box_pack_start (GTK_BOX (hbox), buttonrand, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonrand), "clicked", G_CALLBACK (CBckrandom), NULL);

    label = gtk_label_new ("Number of Leagues");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboNOLEAGUE = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNOLEAGUE), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNOLEAGUE), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNOLEAGUE), "2");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboNOLEAGUE), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboNOLEAGUE);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new ("Number of Divisions in Each League");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboNODIV = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNODIV), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNODIV), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNODIV), "2");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNODIV), "3");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboNODIV), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboNODIV);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new ("Number of Teams in Each Division");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboNOTEAM = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboNOTEAM), "random");
    for (x = 2; x < 51; x++) {
        strncpy (&w[0], (char *) cnvt_int2str (x, 'l'), 2);
        w[2] = '\0';
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboNOTEAM), w);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboNOTEAM), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboNOTEAM);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("(Note - total number of teams will not exceed 100)");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new ("Designated Hitter");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboDH = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboDH), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboDH), "DH only with AL teams");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboDH), "DH only with NL teams");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboDH), "DH with all teams");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboDH), "no DH");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboDH), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboDH);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new ("Number of Wild Card Teams");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboWC = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboWC), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboWC), "0");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboWC), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboWC), "2");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboWC), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboWC);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new ("PS, Round 1 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    comboPSR1 = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "9");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR1), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboPSR1);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new ("PS, Round 2 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    comboPSR2 = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "9");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR2), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboPSR2);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new ("PS, Round 3 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    comboPSR3 = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "9");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR3), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboPSR3);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    label = gtk_label_new ("         ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new ("PS, Round 4 - Best of ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    comboPSR4 = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "random");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "9");
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR4), 0);
    gtk_container_add (GTK_CONTAINER (hbox), comboPSR4);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_OK, 1);
    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 0);

    toggle_ha = 0;
    gtk_widget_show_all (dlgFile);
    gtk_widget_hide (vbox2);
RunSetUp:
    x = gtk_dialog_run (GTK_DIALOG (dlgFile));

    if (x) {
        if (!toggle_ha) {
            gtk_widget_destroy (dlgFile);
            SetUpLeague ();
        }
        else {
            gchar SuccessLeague[256] = "Season successfully established on server.", *msg[5];

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            if (yearsw) {
                /* tell server to set up a season based upon a specific real life year */
                strcpy (&yeara[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboLEAGUE)));

                strcpy (&work[0], "M");
                strcat (&work[0], &yeara[0]);
                strcat (&work[0], "\n");
            }
            if (teamsw) {
                gint x, y, err;
                gchar *msg[5], NoYrs[256] = "No years to search.", holdt[100], *comma,
                      OneYr[256] = "\nA team with this name happened in only 1 year.  There needs to be at least 2 teams to form a league.\n\n",
                      NoInYears[256] = "\nThe years to include is empty.  Assume all years (1901-2018)?\n\n";

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                inentry_text = gtk_entry_get_text (GTK_ENTRY (inentry));

                if (!strlen (&inentry_text[0]) && !preferences.AssumeAllYears) {
                    msg[0] = &NoInYears[0];
                    if (!ShallWeContinue (msg))
                        goto RunSetUp;
                }

                exentry_text = gtk_entry_get_text (GTK_ENTRY (exentry));

                err = ValidateText2 ();
                if (err) {
                    gint errct;
                    gchar ExMsg[256] = "In excluded years ...", InMsg[256] = "In included years ...",
                          YrAfterHy[256] = "The year after a hyphen must be equal to or greater than the year before that same hyphen.",
                          NoYr[256] = "There must be a year after a hyphen.", InitYr[256] = "A year must be the first data.",
                          InvYr[256] = "A year is not valid (must be 4 positions and must be 1901-2018).";

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
                    goto RunSetUp;
                }

                for (y = x = 0; x < YEAR_SPREAD; x++)
                    if (yrs[x])
                        if (++y > 1)
                            break;
                if (x == YEAR_SPREAD) {
                    if (y)
                        msg[0] = &OneYr[0];
                    else
                        msg[0] = &NoYrs[0];
                    outMessage (msg);
                    goto RunSetUp;
                }

                strcpy (&work[0], "MT");
                strcpy (&holdt[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTNAME)));
                /* find the comma and remove it and everything after */
                comma = (char *) index (&holdt[0], ',');
                *comma = '\0';

                /* squeeze out spaces */
                for (x = 0; x < strlen (&holdt[0]); )    /* increment x within loop */
                    if (holdt[x] == ' ')
                        for (y = x; y < strlen (&holdt[0]); y++)
                            holdt[y] = holdt[y + 1];
                    else
                        x++;
                strcat (&work[0], &holdt[0]);
                strcat (&work[0], " ");
                if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTWC)), "0"))
                    strcat (&work[0], "0");
                else
                    if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTWC)), "1"))
                        strcat (&work[0], "1");
                    else
                        strcat (&work[0], "2");
                if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTDH)), "yes"))
                    strcat (&work[0], "1");
                else
                    strcat (&work[0], "0");
                strcat (&work[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboSPSR1)));
                if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboSPSR2)), "No Round"))
                    strcat (&work[0], "0");
                else
                    strcat (&work[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboSPSR2)));
                for (x = 0; x < YEAR_SPREAD; x++)
                    strcat (&work[0], cnvt_int2str (yrs[x], 'l'));

                strcat (&work[0], "\n");
            }
            if (randomsw) {
                gint numl, numd, numt, dhi, wci, psr1, psr2, psr3, psr4;
                gchar twrk[100];

                /* tell server to set up a random season with some limited direction by user */
                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboNOLEAGUE)));
                if (twrk[0] == '1')
                    numl = 1;
                else
                    if (twrk[0] == '2')
                        numl = 2;
                    else
                        numl = (int) ((float) 2 * rand () / (RAND_MAX + 1.0)) + 1;

                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboNODIV)));
                if (twrk[0] == '1')
                    numd = 1;
                else
                    if (twrk[0] == '2')
                        numd = 2;
                    else
                        if (twrk[0] == '3')
                            numd = 3;
                        else
                            numd = (int) ((float) 3 * rand () / (RAND_MAX + 1.0)) + 1;

                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboNOTEAM)));
                if (twrk[0] == 'r')
                    numt = (int) ((float) 50 * rand () / (RAND_MAX + 1.0)) + 2;                /* 2 - 51 */
                else
                    numt = atoi (&twrk[0]);
                if (numt > 50)
                    /* just in case 51 is the randomized result above */
                    numt = 50;
                /* the total number of teams in the league cannot exceed 100 */
                if ((numl * numd * numt) > 100)
                    numt = 100 / (numl * numd);

                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboDH)));
                if (twrk[0] == 'n')
                    dhi = 0;
                if (strstr (&twrk[0], "AL"))
                    dhi = 1;
                if (strstr (&twrk[0], "all teams"))
                    dhi = 3;
                if (twrk[0] == 'r') {
                    if (numl == 1)
                        dhi = (int) ((float) 2 * rand () / (RAND_MAX + 1.0));
                    else
                        dhi = (int) ((float) 4 * rand () / (RAND_MAX + 1.0));
                }
                if (strstr (&twrk[0], "NL")) {
                    if (numl == 1)
                        dhi = 0;
                    else
                        dhi = 2;
                }

                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboWC)));
                if (twrk[0] == '0')
                    wci = 0;
                if (twrk[0] == '1')
                    wci = 1;
                if (twrk[0] == '2')
                    wci = 2;
                if (twrk[0] == 'r')
                    wci = (int) ((float) 3 * rand () / (RAND_MAX + 1.0));

                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR1)));
                if (twrk[0] == 'r') {
                    psr1 = (int) ((float) 9 * rand () / (RAND_MAX + 1.0));
                    /* ensure result is an odd number (1, 3, 5, 7, 9 valid values) */
                    if (!(psr1 % 2))
                        psr1++;
                }
                else
                    psr1 = twrk[0] - '0';
                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR2)));
                if (twrk[0] == 'r') {
                    psr2 = (int) ((float) 9 * rand () / (RAND_MAX + 1.0));
                    /* ensure result is an odd number (1, 3, 5, 7, 9 valid values) */
                    if (!(psr2 % 2))
                        psr2++;
                }
                else
                    psr2 = twrk[0] - '0';
                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR3)));
                if (twrk[0] == 'r') {
                    psr3 = (int) ((float) 9 * rand () / (RAND_MAX + 1.0));
                    /* ensure result is an odd number (1, 3, 5, 7, 9 valid values) */
                    if (!(psr3 % 2))
                        psr3++;
                }
                else
                    psr3 = twrk[0] - '0';
                strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR4)));
                if (twrk[0] == 'r') {
                    psr4 = (int) ((float) 9 * rand () / (RAND_MAX + 1.0));
                    /* ensure result is an odd number (1, 3, 5, 7, 9 valid values) */
                    if (!(psr4 % 2))
                        psr4++;
                }
                else
                    psr4 = twrk[0] - '0';

                strcpy (&work[0], "MR");
                strncat (&work[0], (char *) cnvt_int2str (numl, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (numd, 'l'), 1);
                if (numt > 9)
                    strncat (&work[0], (char *) cnvt_int2str (numt, 'l'), 2);
                else {
                    strcat (&work[0], "0");
                    strncat (&work[0], (char *) cnvt_int2str (numt, 'l'), 1);
                }
                strncat (&work[0], (char *) cnvt_int2str (wci, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (dhi, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (psr1, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (psr2, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (psr3, 'l'), 1);
                strncat (&work[0], (char *) cnvt_int2str (psr4, 'l'), 1);
                strcat (&work[0], "\n");
                if (work[4] == ' ')
                    work[4] = '0';
            }
            gtk_widget_destroy (dlgFile);

            sock_puts (sock, work);
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                close (fdlock);
                GotError ();
                return;
            }

            if (!strncmp (&buffer[0], "-2", 2)) {
                /* this happens if the directory /usr/var/NSB does not exist on the server */
                gchar NoDir[256] = "There is a problem with the server.  Try again later.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                strcpy (&work[0], "Encountered error when talking to server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &NoDir[0];
                outMessage (msg);

                close (fdlock);
                return;
            }

            if (!strlen (&buffer[0])) {
                /* this happens if there are no groups of teams available */
                gchar NoTeams[256] = "No teams are available in order to establish a season.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                strcpy (&work[0], "No teams available in order to establish a season on server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &NoTeams[0];
                outMessage (msg);

                close (fdlock);
                return;
            }

            if (!strncmp (&buffer[0], "-4", 2)) {
                /* this happens if there are not enough teams to establish a league */
                gchar NoTeams[256] = "There are not enough teams to establish a league.  Minimum is two teams.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                strcpy (&work[0], "Not enough teams available to establish a season on server ");
                strcat (&work[0], &hs[0]);
                strcat (&work[0], ".\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &NoTeams[0];
                outMessage (msg);

                close (fdlock);
                return;
            }

            strcpy (&work[0], "Season established on server ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], ".\n");
            Add2TextWindow (&work[0], 0);
            SetLeagueUnderWay (1);
            SetSeriesUnderWay (0);
            msg[0] = &SuccessLeague[0];
            outMessage (msg);
        }
    }
    else
        gtk_widget_destroy (dlgFile);
    close (fdlock);
}

void
toggle_method_h (GtkWidget *widget, gpointer *pdata) {
    toggle_ha = 0;
    gtk_widget_hide (vbox2);
}

void
toggle_method_a (GtkWidget *widget, gpointer *pdata) {
    toggle_ha = 1;
    gtk_window_resize (GTK_WINDOW (dlgFile), 1000, 2000);
    gtk_widget_show (vbox2);
}

void
CBckyear (GtkWidget *widget, gpointer pdata) {
    randomsw = teamsw = 0;
    yearsw = 1;
}

void
CBckrandom (GtkWidget *widget, gpointer pdata) {
    randomsw = 1;
    yearsw = teamsw = 0;
}

void
CBckteam (GtkWidget *widget, gpointer pdata) {
    teamsw = 1;
    randomsw = yearsw = 0;
}

int
ValidateText2 () {
    gint x, z, yrpnt, pos, errors, yr1, yr2;
    gchar yrdata[2][1000], work[10];

    errors = 0;

    if (!strlen (&inentry_text[0])) {
        /* using all years */
        for (x = 0; x < YEAR_SPREAD; x++)
            yrs[x] = 1;

        if (!strlen (&exentry_text[0]))
            /* done ... input all years */
            return 0;

        yrpnt = 1;
    }
    else
        yrpnt = 0;

    strcpy (&yrdata[0][0], inentry_text);
    strcpy (&yrdata[1][0], exentry_text);

    for (pos = 0; yrpnt < 2; yrpnt++, pos = 0) {
nxt_group:
        if (pos >= strlen (&yrdata[yrpnt][0]))
            continue;
        /* ignore leading spaces */
        for ( ; pos < strlen (&yrdata[yrpnt][0]); pos++)
            if (yrdata[yrpnt][pos] != ' ')
                break;
        /* the first data must be a year */
        if (yrdata[yrpnt][pos] == '-') {
            if (yrpnt == 0)
                errors |= 0x20;
            else
                errors |= 0x02;
            continue;                                                /* don't do the other checks on this data in this case */
        }

        /* got a year ... maybe */
        for (x = 0; x < 5 && pos < strlen (&yrdata[yrpnt][0]); x++, pos++)
            if (yrdata[yrpnt][pos] == ' ' || yrdata[yrpnt][pos] == '-')
                break;
            else
                work[x] = yrdata[yrpnt][pos];
        /* must be a 4-position year */
        if (x != 4) {
            if (yrpnt == 0)
                errors |= 0x10;
            else
                errors |= 0x01;
            continue;                                                /* don't do the other checks on this data in this case */
        }
        work[4] = '\0';
        yr1 = atoi (&work[0]);
        if (yr1 < 1901 || yr1 > MAX_YEAR) {
            if (yrpnt == 0)
                errors |= 0x10;
            else
                errors |= 0x01;
        }
        if (yrdata[yrpnt][pos] == ' ')
            for ( ; pos < strlen (&yrdata[yrpnt][0]); pos++)
                if (yrdata[yrpnt][pos] == '-')
                    break;
                else
                    if (yrdata[yrpnt][pos] == ' ')
                        continue;
                    else {
                        /* got another year */
                        if (!errors) {
                            if (yrpnt == 0)
                                yrs[yr1 - 1901] = 1;     /* include year in search */
                            else
                                yrs[yr1 - 1901] = 0;     /* exclude year from search */
                        }
                        goto nxt_group;
                    }
        if (pos >= strlen (&yrdata[yrpnt][0])) {
            /* got another year */
            if (!errors) {
                if (yrpnt == 0)
                    yrs[yr1 - 1901] = 1;     /* include year in search */
                else
                    yrs[yr1 - 1901] = 0;     /* exclude year from search */
            }
            goto nxt_group;
        }

        /* need to get a second year */
        if (yrdata[yrpnt][pos] == '-') {
            /* ignore spaces */
            for (pos++; pos < strlen (&yrdata[yrpnt][0]); pos++)
                if (yrdata[yrpnt][pos] != ' ')
                    break;
            /* the first data encountered must be a year */
            if (yrdata[yrpnt][pos] == '-') {
                if (yrpnt == 0)
                    errors |= 0x20;
                else
                    errors |= 0x02;
                continue;                                                /* don't do the other checks on this data in this case */
            }

            /* got a year ... maybe */
            for (x = 0; x < 5 && pos < strlen (&yrdata[yrpnt][0]); x++, pos++)
                if (yrdata[yrpnt][pos] == ' ' || yrdata[yrpnt][pos] == '-')
                    break;
                else
                    work[x] = yrdata[yrpnt][pos];
            /* must be a 4-position year */
            if (x != 4) {
                if (yrpnt == 0)
                    errors |= 0x10;
                else
                    errors |= 0x01;
                continue;                                                /* don't do the other checks on this data in this case */
            }
            work[4] = '\0';
            yr2 = atoi (&work[0]);
            if (yr2 < 1901 || yr2 > MAX_YEAR) {
                if (yrpnt == 0)
                    errors |= 0x10;
                else
                    errors |= 0x01;
            }
            if (yr2 < yr1) {
                if (yrpnt == 0)
                    errors |= 0x80;
                else
                    errors |= 0x08;
                for ( ; pos < strlen (&yrdata[yrpnt][0]) && yrdata[yrpnt][pos] != ' '; pos++);
                goto nxt_group;
            }

            if (!errors)
                for (z = yr1; z <= yr2; z++)
                    if (yrpnt == 0)
                        yrs[z - 1901] = 1;     /* include year in search */
                    else
                        yrs[z - 1901] = 0;     /* exclude year from search */
            pos++;
            goto nxt_group;
        }
    }

    return errors;
}

