/* display This Day in Baseball */

#include <time.h>
#include "gtknsbc.h"
#include "cglobal.h"
#include "prototypes.h"

GtkWidget *windowTDIB, *combom, *combod;
gchar *mns[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void
TDIBFromMenu (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *box1, *box2, *hbox, *frame, *button, *okbutton, *separator, *stock;
    gint x;

    /* set initial selection display to today's month and day */
    time (&dt);
    dc = *localtime (&dt);

    getmdTDIBactive = 1;

    windowTDIB = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (windowTDIB), "destroy", G_CALLBACK (DestroyDialog), windowTDIB);
    gtk_window_set_title (GTK_WINDOW (windowTDIB), "Get a Month & Day for This Day in Baseball");
    gtk_container_set_border_width (GTK_CONTAINER (windowTDIB), 0);
    gtk_window_set_keep_above (GTK_WINDOW (windowTDIB), TRUE);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (windowTDIB), box1);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (box1), stock, FALSE, FALSE, 0);

    frame = gtk_frame_new ("Pick a Month and a Day");
    gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    combom = gtk_combo_box_new_text ();

    for (x = 0; x < 12; x++)
        gtk_combo_box_append_text (GTK_COMBO_BOX (combom), mns[x]);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combom), dc.tm_mon);
    gtk_container_add (GTK_CONTAINER (box2), combom);

    combod = gtk_combo_box_new_text ();

    for (x = 0; x < 31; x++)
        gtk_combo_box_append_text (GTK_COMBO_BOX (combod), cnvt_int2str (x + 1, 'l'));

    gtk_combo_box_set_active (GTK_COMBO_BOX (combod), dc.tm_mday - 1);
    gtk_container_add (GTK_CONTAINER (box2), combod);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    okbutton = gtk_button_new_with_label ("Get This Day in Baseball");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (GetTDIB), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), okbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyTDIB), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

    gtk_widget_show_all (windowTDIB);
}

void
GetTDIB (GtkWidget *widget, gpointer *pdata) {
    gint m, d;

    m = gtk_combo_box_get_active (GTK_COMBO_BOX (combom)) + 1;
    d = gtk_combo_box_get_active (GTK_COMBO_BOX (combod)) + 1;

    TDIB (0, m, d);
}

void
DestroyTDIB (GtkWidget *widget, gpointer *pdata) {
    getmdTDIBactive = 0;
    DestroyDialog (windowTDIB, windowTDIB);
}

void
TDIB (int boot, int m, int d) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *tdib;
    gint b, notdib, len;
    gchar fname[200], month[3], day[3];

    strcpy (&fname[0], "/usr/local/share/NSB/TDIB/");

    if (boot) {
        /* if at boot use today's month and day */
        time (&dt);
        dc = *localtime (&dt);
        strcpy (&month[0], (char *) cnvt_int2str ((dc.tm_mon + 1), 'l'));
        strcpy (&day[0], (char *) cnvt_int2str (dc.tm_mday, 'l'));
    }
    else {
        /* if called from menu use supplied month and day */
        strcpy (&month[0], (char *) cnvt_int2str (m, 'l'));
        strcpy (&day[0], (char *) cnvt_int2str (d, 'l'));
    }

    if (strlen (&month[0]) == 1) {
        month[2] = '\0';
        month[1] = month[0];
        month[0] = '0';
    }
    if (strlen (&day[0]) == 1) {
        day[2] = '\0';
        day[1] = day[0];
        day[0] = '0';
    }
    strcat (&fname[0], &month[0]);
    strcat (&fname[0], "/");
    strcat (&fname[0], &day[0]);

    /* clear data area */
    for (b = 0; b < 50000; b++)
        tdibtext[b] = '\0';

    strcpy (&tdibtext[0], "\n                            This Day in Baseball\n\n");
    strcat (&tdibtext[0], "(NOTE - These items are taken from <www.nationalpastime.com>,\n<www.baseballlibrary.com> et al.)\n\n");

    len = strlen (&tdibtext[0]);

    /* load the correct data file if it exists */
    if ((tdib = fopen (fname, "r")) != NULL) {
        b = fread (&tdibtext[len], sizeof tdibtext, 1, tdib);
        if (!ferror (tdib))
            notdib = 0;
        else
            /* error reading This Day in Baseball file */
            notdib = 1;
        fclose (tdib);
    }
    else
        /* error opening This Day in Baseball file */
        notdib = 1;

    if (notdib) {
        gchar NoTDIBAvailable[256] = "This Day in Baseball data not available for ", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "This Day in Baseball data not available for ");
        strcat (&work[0], &month[0]);
        strcat (&work[0], "/");
        strcat (&work[0], &day[0]);
        strcat (&work[0], ".\n");
        Add2TextWindow (&work[0], 1);

        strcat (&NoTDIBAvailable[0], &month[0]);
        strcat (&NoTDIBAvailable[0], "/");
        strcat (&NoTDIBAvailable[0], &day[0]);
        strcat (&NoTDIBAvailable[0], ".\n");
        msg[0] = &NoTDIBAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 760, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "This Day in Baseball");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, tdibtext, strlen (&tdibtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    strcpy (&prtbuttdibcmd[prtbuttdibpnt][0], &month[0]);
    strcat (&prtbuttdibcmd[prtbuttdibpnt][0], "/");
    strcat (&prtbuttdibcmd[prtbuttdibpnt][0], &day[0]);
    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintTDIB), GINT_TO_POINTER (prtbuttdibpnt));
    prtbuttdibpnt++;

    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
PrintTDIB (GtkWidget *widget, gpointer cnt) {
    gchar fname[200], work1[100], Printing[256] = "Printing This Day in Baseball ...", *msg[5];
    gint b, x, len, notdib, icnt = GPOINTER_TO_INT (cnt);
    FILE *tdib;

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print TDIB.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print TDIB.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    strcpy (&fname[0], "/usr/local/share/NSB/TDIB/");
    strcat (&fname[0], &prtbuttdibcmd[icnt][0]);

    /* clear data area */
    for (b = 0; b < 50000; b++)
        tdibtext[b] = '\0';

    strcpy (&tdibtext[0], "\n                            This Day in Baseball\n\n");
    strcat (&tdibtext[0], "(NOTE - Most of these items are taken from <www.nationalpastime.com> and\n<www.baseballlibrary.com>.)\n\n");

    len = strlen (&tdibtext[0]);

    /* load the correct data file */
    if ((tdib = fopen (fname, "r")) != NULL) {
        b = fread (&tdibtext[len], sizeof tdibtext, 1, tdib);
        if (!ferror (tdib))
            notdib = 0;
        else
            /* error reading This Day in Baseball file */
            notdib = 1;
        fclose (tdib);
    }
    else
        /* error opening This Day in Baseball file */
        notdib = 1;

    if (notdib) {
        gchar NoTDIBAvailable[256] = "Something is wrong.  Cannot print TDIB.\n", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Something is wrong.  Cannot print TDIB.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoTDIBAvailable[0];
        outMessage (msg);

        return;
    }

    print (&tdibtext[0]);

    strcpy (&work[0], "Printing This Day in Baseball for ");
    strcat (&work[0], &prtbuttdibcmd[icnt][0]);
    strcat (&work[0], ".\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

