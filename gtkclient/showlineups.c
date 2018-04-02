
/* show lineups  */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

gchar lineups[4096];

void
ShowLineups () {
    GtkWidget *dlgLU, *box1, *box2, *hbox, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    gint sk;

    if (netgame && challenger_ind)
        sk = sockhvh;
    else
        sk = sock;

    if (netgame && challenger_ind)
        /* need to do a dummy sock_gets */
        sock_gets (sk, &buffer[0], sizeof (buffer));

    if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
        gchar NotAvailable[256] = "An error was encountered when talking to the server.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Error talking to server.\n\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotAvailable[0];

        outMessage (msg);
        return;
    }
    /*
       data received from server will be in the form -

       repeat for each team:
         hitter #1 (variable number of characters)
         2 spaces
         position indicator (1 character)
         2 spaces
         repeat for hitters # 2 - 9
         if DH
             pitcher (variable number of characters)
             2 spaces
    */

    if (!preferences.ShowStartingLineups)
        /* don't show the starting lineups */
        return;

    dlgLU = gtk_dialog_new_with_buttons ("Starting Lineups", NULL, GTK_DIALOG_MODAL, "Print", 0, "Play Ball!", 1, NULL);
    gtk_window_set_default_size (GTK_WINDOW (dlgLU), 550, 600);
    gtk_signal_connect (GTK_OBJECT (dlgLU), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgLU)->vbox), box1, TRUE, TRUE, 0);

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

    FillLUInfo ();
    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, lineups, strlen (&lineups[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);
    gtk_widget_show_all (box1);
RunLineupDsp:
    if (gtk_dialog_run (GTK_DIALOG (dlgLU)))
        DestroyDialog (dlgLU, dlgLU);
    else {
        gchar Printing[256] = "Printing Starting Lineups ...", *msg[5];
        gint x;

        print (&lineups[0]);

        strcpy (&work[0], "Print Starting Lineups.\n\n");
        Add2TextWindow (&work[0], 0);

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &Printing[0];

        outMessage (msg);

        goto RunLineupDsp;
    }
}

void
FillLUInfo () {
    gchar *cc, dhind;
    gint x, pos;

    dhind = buffer[0];

    strcpy (&lineups[0], "\n                  Starting Lineups\n\n\n");

    strcat (&lineups[0], &vteamyr[0]);
    strcat (&lineups[0], " ");
    strcat (&lineups[0], &visiting_team[0]);
    strcat (&lineups[0], "\n\n");

    for (cc = &buffer[1], x = 0; x < 9; x++) {
        while (1) {
            if (!strncmp (cc, "  ", 2))
                break;
            strncat (&lineups[0], cc, 1);
            cc++;
        }
        strcat (&lineups[0], " - ");
        cc += 2;
        pos = *cc - '0';
        strcat (&lineups[0], figure_pos (pos));
        strcat (&lineups[0], "\n");
        cc += 3;
    }

    if (dhind == '1') {
        strcat (&lineups[0], "\n");
        while (1) {
            if (!strncmp (cc, "  ", 2))
                break;
            strncat (&lineups[0], cc, 1);
            cc++;
        }
        strcat (&lineups[0], " -  P\n");
        cc += 2;
    }

    strcat (&lineups[0], "\n\n");

    strcat (&lineups[0], &hteamyr[0]);
    strcat (&lineups[0], " ");
    strcat (&lineups[0], &home_team[0]);
    strcat (&lineups[0], "\n\n");

    for (x = 0; x < 9; x++) {
        while (1) {
            if (!strncmp (cc, "  ", 2))
                break;
            strncat (&lineups[0], cc, 1);
            cc++;
        }
        strcat (&lineups[0], " - ");
        cc += 2;
        pos = *cc - '0';
        strcat (&lineups[0], figure_pos (pos));
        strcat (&lineups[0], "\n");
        cc += 3;
    }

    if (dhind == '1') {
        strcat (&lineups[0], "\n");
        while (1) {
            if (!strncmp (cc, "  ", 2))
                break;
            strncat (&lineups[0], cc, 1);
            cc++;
        }
        strcat (&lineups[0], " -  P");
    }

    strcat (&lineups[0], "\n\n");
}

