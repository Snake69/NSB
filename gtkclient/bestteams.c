/* find best real life teams for a given time period */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

GtkWidget *dlgBT, *WPCTRSsp, *WPCTPSsp, *RSADRSsp, *RSALAsp, *GA2PTRSsp, *RSADPSsp, *totbut, *exentry, *inentry, *comboPSgames;
gint total, WinPCTRS, WinPCTPS, RDiffRS, RAvgRS, GDiffRS, RDiffPS, RDiffPSG,
     yrs[YEAR_SPREAD] /* 1901 - MAX_YEAR */ ;
const gchar *inentry_text, *exentry_text;

void
BestRLTeams (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *box1, *box2, *sw, *hbox, *label, *separator, *findbutton, *disbutton, *table;
    GtkAdjustment *adj;
    gint x;

    if (getBTactive)
        /* only need to execute this function if it isn't already active */
        return;

    getBTactive = 1;

    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;
    total = 100;

    dlgBT = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgBT), "Evaluate Team Seasons");
    gtk_window_set_default_size (GTK_WINDOW (dlgBT), 630, 400);
    gtk_signal_connect (GTK_OBJECT (dlgBT), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgBT)->vbox), sw, TRUE, TRUE, 0);

    box1 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 10);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), box1);

    label = gtk_label_new ("Evaluate Team Seasons\n\n");
    gtk_container_add (GTK_CONTAINER (box1), label);

    separator = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (box1), separator);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Factors and Strengths");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Winning percentage - Regular Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (50.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    WPCTRSsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (WPCTRSsp), FALSE);
    gtk_widget_set_size_request (WPCTRSsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (WPCTRSsp), TRUE);
    g_signal_connect (WPCTRSsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (WPCTRSsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), WPCTRSsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Winning percentage - Post Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (5.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    WPCTPSsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (WPCTPSsp), FALSE);
    gtk_widget_set_size_request (WPCTPSsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (WPCTPSsp), TRUE);
    g_signal_connect (WPCTPSsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (WPCTPSsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), WPCTPSsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Runs Scored/Allowed Disparity - Regular Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (20.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    RSADRSsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (RSADRSsp), FALSE);
    gtk_widget_set_size_request (RSADRSsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (RSADRSsp), TRUE);
    g_signal_connect (RSADRSsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (RSADRSsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), RSADRSsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Runs Scored Above League Average - Regular Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (10.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    RSALAsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (RSALAsp), FALSE);
    gtk_widget_set_size_request (RSALAsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (RSALAsp), TRUE);
    g_signal_connect (RSALAsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (RSALAsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), RSALAsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Games Ahead of 2nd Place Team - Regular Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (10.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    GA2PTRSsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (GA2PTRSsp), FALSE);
    gtk_widget_set_size_request (GA2PTRSsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (GA2PTRSsp), TRUE);
    g_signal_connect (GA2PTRSsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (GA2PTRSsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), GA2PTRSsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Runs Scored/Allowed Disparity - Post Season:");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    adj = (GtkAdjustment *) gtk_adjustment_new (5.0, 0.0, 100.0, 1.0, 100.0, 0.0);
    RSADPSsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (RSADPSsp), FALSE);
    gtk_widget_set_size_request (RSADPSsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (RSADPSsp), TRUE);
    g_signal_connect (RSADPSsp, "changed", G_CALLBACK (SpinnerChange), NULL);
    g_signal_connect (RSADPSsp, "value-changed", G_CALLBACK (SpinnerChange), NULL);
    gtk_container_add (GTK_CONTAINER (hbox), RSADPSsp);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("For Post Season Run Disparity, take into account number of Post Season games?");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboPSgames = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSgames), "yes");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSgames), "no");
    gtk_container_add (GTK_CONTAINER (hbox), comboPSgames);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSgames), 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Total (must be 100): ");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    totbut = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), totbut, TRUE, TRUE, 0);
    gtk_button_set_label (GTK_BUTTON (totbut), cnvt_int2str (total, 'l'));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);
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

    label = gtk_label_new ("(NOTE - 1981 is NEVER included in the search criteria.)");
    gtk_container_add (GTK_CONTAINER (box2), label);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    disbutton = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroyBT), dlgBT);
    findbutton = gtk_button_new_with_label ("Go");
    g_signal_connect (G_OBJECT (findbutton), "clicked", G_CALLBACK (FindBT), dlgBT);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), findbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgBT);
}

void
DestroyBT (GtkWidget *widget, gpointer *pdata) {
    getBTactive = 0;
    DestroyDialog (dlgBT, dlgBT);
}

void
FindBT (GtkWidget *widget, gpointer *pdata) {
    gint x, err;
    gchar *msg[5], TotalNot100[256] = "Total must equal 100.", NoYrs[256] = "No years to search.",
          NoInYears[256] = "\nThe years to include is empty.  Assume all years (1901-2018)?\n\n";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (total != 100) {
        msg[0] = &TotalNot100[0];
        outMessage (msg);
        return;
    }

    inentry_text = gtk_entry_get_text (GTK_ENTRY (inentry));

    if (!strlen (&inentry_text[0]) && !preferences.AssumeAllYears) {
        msg[0] = &NoInYears[0];
        if (!ShallWeContinue (msg))
            return;
    }

    exentry_text = gtk_entry_get_text (GTK_ENTRY (exentry));

    err = ValidateText ();
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

    WinPCTRS = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (WPCTRSsp));
    WinPCTPS = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (WPCTPSsp));
    RDiffRS =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSADRSsp));
    RAvgRS =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSALAsp));
    GDiffRS =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GA2PTRSsp));
    RDiffPS =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSADPSsp));
    if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSgames)), "yes"))
        RDiffPSG = 1;
    else
        RDiffPSG = 0;

    strcpy (&buffer[0], "B");
    strcat (&buffer[0], cnvt_int2str (WinPCTRS, 'l'));
    strcat (&buffer[0], " ");
    strcat (&buffer[0], cnvt_int2str (WinPCTPS, 'l'));
    strcat (&buffer[0], " ");
    strcat (&buffer[0], cnvt_int2str (RDiffRS, 'l'));
    strcat (&buffer[0], " ");
    strcat (&buffer[0], cnvt_int2str (RAvgRS, 'l'));
    strcat (&buffer[0], " ");
    strcat (&buffer[0], cnvt_int2str (GDiffRS, 'l'));
    strcat (&buffer[0], " ");
    strcat (&buffer[0], cnvt_int2str (RDiffPS, 'l'));
    strcat (&buffer[0], " ");

    if (RDiffPSG)
        strcat (&buffer[0], "1");
    else
        strcat (&buffer[0], "0");
    strcat (&buffer[0], " ");

    for (x = 0; x < YEAR_SPREAD; x++)
        strcat (&buffer[0], cnvt_int2str (yrs[x], 'l'));

    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    ShowBestTeams ();
}

void
SpinnerChange () {
    total = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (WPCTRSsp)) +
            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (WPCTPSsp)) +
            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSADRSsp)) +
            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSALAsp)) +
            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (RSADPSsp)) +   
            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GA2PTRSsp));    
    gtk_button_set_label (GTK_BUTTON (totbut), cnvt_int2str (total, 'l'));
}

int
ValidateText () {
    gint x, z, yrpnt, pos, errors, yr1, yr2;
    gchar yrdata[2][1000], work[10];

    errors = 0;
    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

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

/* allow only numbers, spaces, and hyphens to be entered */
void
CheckEntry (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    GtkEditable *editable = GTK_EDITABLE (entry);
    int i, count = 0;
    gchar *result = g_new (gchar, length);

    for (i = 0; i < length; i++) {
        if (!isdigit (text[i]) && text[i] != ' ' && text[i] != '-')
            continue;
        result[count++] = text[i];
    }

    if (count > 0) {
        gtk_signal_handler_block_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntry), data);
        gtk_editable_insert_text (editable, result, count, position);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntry), data);
    }
    gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");

    g_free (result);
}

gchar buf[5000];

void
ShowBestTeams () {
    /* show best real life teams */
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_gets (sock, &buf[0], sizeof (buf));

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 750, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Evaluate Team Seasons");
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
    gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                      GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* Add a vertical scrollbar to the GtkText widget */
    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
    gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1, GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* Load a fixed font */
    fixed_font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

    /* Realizing a widget creates a window for it, ready for us to insert some text */
    gtk_widget_realize (text);

    /* Freeze the text widget, ready for multiple updates */
    gtk_text_freeze (GTK_TEXT (text));

    FillBestTeams ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, BestTeamstext, strlen (&BestTeamstext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    strcpy (&prtbutBTcmd[prtbutBTpnt][0], &buffer[0]);
    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintBestTeams), GINT_TO_POINTER (prtbutBTpnt));
    prtbutBTpnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillBestTeams () {
    gint x, y, z, teamid, yron;
    gchar year[5], *cc, *ccb, charid[3];

    strcpy (&BestTeamstext[0], "                       Evaluate Team Seasons\n\n");
    strcat (&BestTeamstext[0], "Regular Season Won/Loss Pct Strength - ");
    strcat (&BestTeamstext[0], cnvt_int2str (WinPCTRS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Post Season Won/Loss Pct Strength - ");
    strcat (&BestTeamstext[0], cnvt_int2str (WinPCTPS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Regular Season Run Differential Strength - ");
    strcat (&BestTeamstext[0], cnvt_int2str (RDiffRS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Regular Season Runs Above Average Strength - ");
    strcat (&BestTeamstext[0], cnvt_int2str (RAvgRS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Regular Season Games-Ahead Strength (only applies to first place teams) - ");
    strcat (&BestTeamstext[0], cnvt_int2str (GDiffRS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Post Season Run Differential Strength - ");
    strcat (&BestTeamstext[0], cnvt_int2str (RDiffPS, 'l'));
    strcat (&BestTeamstext[0], "\n");
    if (RDiffPSG)
        strcat (&BestTeamstext[0], "Take into account Number of Post Season games played");
    else
        strcat (&BestTeamstext[0], "DO NOT take into account Number of Post Season games played");
    strcat (&BestTeamstext[0], "\n");
    strcat (&BestTeamstext[0], "Year(s) - ");

    for (yron = x = 0; x < YEAR_SPREAD; x++)
        if (yrs[x])
            if (!yron) {
                strcat (&BestTeamstext[0], cnvt_int2str (1901 + x, 'l'));
                yron = 1;
            }
            else
                yron = 2;
        else {
            if (yron == 1)
                strcat (&BestTeamstext[0], " ");
            if (yron == 2) {
                strcat (&BestTeamstext[0], "-");
                strcat (&BestTeamstext[0], cnvt_int2str (1901 + x - 1, 'l'));
                strcat (&BestTeamstext[0], " ");
            }
            yron = 0;
        }
    if (yron == 2) {
        strcat (&BestTeamstext[0], "-");
        strcat (&BestTeamstext[0], cnvt_int2str (1901 + x - 1, 'l'));
    }
    strcat (&BestTeamstext[0], "\n\n");

    strcat (&BestTeamstext[0], "(NOTE - 1981 is NEVER included in the search criteria.)\n\n");

    strcat (&BestTeamstext[0], "     Year Team                                             Score\n");

    for (x = 0, cc = &buf[0]; x < 100 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        strcat (&BestTeamstext[0], cnvt_int2str ((x + 1), 'l'));
        strcat (&BestTeamstext[0], ".");
        if (x == 99)
            strcat (&BestTeamstext[0], " ");
        else
            if (x > 8)
                strcat (&BestTeamstext[0], "  ");
            else
                strcat (&BestTeamstext[0], "   ");

        strncpy (&year[0], cc, 4);
        year[4] = '\0';
        cc += 5;

        charid[0] = *cc;
        cc++;
        if (*cc != ' ') {
            charid[1] = *cc;
            charid[2] = '\0';
            cc += 2;
        }
        else {
            charid[1] = '\0';
            cc++;
        }
        teamid = atoi (&charid[0]);

        strcat (&BestTeamstext[0], &year[0]);
        strcat (&BestTeamstext[0], " ");

        /* move Team Name */
        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamid) {
                strcat (&BestTeamstext[0], &teaminfo[y].teamname[0]);
                break;
            }
        z = strlen (&teaminfo[y].teamname[0]);
        strncat (&BestTeamstext[0], "                                            ", 45 - z);

        ccb = (char *) index (cc, ' ');
        *ccb = '\0';

        strcat (&BestTeamstext[0], cc);
        strcat (&BestTeamstext[0], "\n");
        *ccb = ' ';
        cc = ccb + 1;
    }
}

void
PrintBestTeams (GtkWidget *widget, gpointer cnt) {
    gchar Printing[256] = "Printing Evaluate Team Seasons ...", *msg[5], work1[100], *pos1, *pos2;
    gint x, icnt = GPOINTER_TO_INT (cnt);

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print Evaluate Team Seasons.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print Evaluate Team Seasons.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    /* re-get stuff to print */
    strcpy (&buffer[0], &prtbutBTcmd[icnt][0]);
    sock_puts (sock, &buffer[0]);
    sock_gets (sock, &buf[0], sizeof (buf));

    pos1 = (char *) index (&buffer[0], ' ');
    *pos1 = '\0';
    WinPCTRS = atoi (&buffer[1]);
    *pos1 = ' ';
    pos1++;

    pos2 = (char *) index (pos1, ' ');
    *pos2 = '\0';
    WinPCTPS = atoi (pos1);
    *pos2 = ' ';
    pos1 = pos2 + 1;

    pos2 = (char *) index (pos1, ' ');
    *pos2 = '\0';
    RDiffRS = atoi (pos1);
    *pos2 = ' ';
    pos1 = pos2 + 1;

    pos2 = (char *) index (pos1, ' ');
    *pos2 = '\0';
    RAvgRS = atoi (pos1);
    *pos2 = ' ';
    pos1 = pos2 + 1;

    pos2 = (char *) index (pos1, ' ');
    *pos2 = '\0';
    GDiffRS = atoi (pos1);
    *pos2 = ' ';
    pos1 = pos2 + 1;

    pos2 = (char *) index (pos1, ' ');
    *pos2 = '\0';
    RDiffPS = atoi (pos1);
    *pos2 = ' ';

    /* point to the first position of the year indicators */
    pos1 = pos2 + 1;

    for (x = 0; x < YEAR_SPREAD; pos1++, x++)
        yrs[x] = *pos1 - '0';

    FillBestTeams ();
    print (&BestTeamstext[0]);

    strcpy (&work[0], "Print Evaluate Team Seasons.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

