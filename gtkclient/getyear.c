/* get a year from user */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

GtkWidget *windowTT, *windowRES, *comboTT, *comboRES;
gchar yeara[5];
extern gchar urind, stats[];

void
GetAYear4RLTeamTotals () {
    GtkWidget *box1, *box2, *hbox, *frame, *button, *okbutton, *separator, *stock;
    gchar w[5];
    gint year;

    getayrRLTTactive = 1;

    windowTT = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (windowTT), "destroy", G_CALLBACK (DestroyGetYearRLTT), windowTT);
    gtk_window_set_title (GTK_WINDOW (windowTT), "Get a Year for Real Life Team Totals");
    gtk_container_set_border_width (GTK_CONTAINER (windowTT), 0);
    gtk_window_set_keep_above (GTK_WINDOW (windowTT), TRUE);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (windowTT), box1);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (box1), stock, FALSE, FALSE, 0);

    frame = gtk_frame_new ("Pick a Year for Real Life Team Totals");
    gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    comboTT = gtk_combo_box_new_text ();

    for (year = 1901; year < (MAX_YEAR + 1); year++) {
        strncpy (&w[0], (char *) cnvt_int2str (year, 'l'), 4);
        w[4] = '\0';
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboTT), w);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboTT), 55);
    gtk_container_add (GTK_CONTAINER (box2), comboTT);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    okbutton = gtk_button_new_with_label ("Get Team Totals");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (GetYearRLTT), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), okbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyGetYearRLTT), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

    gtk_widget_show_all (windowTT);
}

void
GetYearRLTT (GtkWidget *widget, gpointer *pdata) {
    strcpy (&yeara[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTT)));

    strcpy (&work[0], "S691");
    strcat (&work[0], &yeara[0]);
    strcat (&work[0], "\n");
    sock_puts (sock, work);  /* tell the server to send real life team totals for a specific year */
    urind = 'R';
    whichur[prtbutttpnt] = urind;
    strcpy (&teamyr[0], &yeara[0]);      /* save year */
    ShowTeamTotals (1);
}

void
DestroyGetYearRLTT (GtkWidget *widget, gpointer *pdata) {
    getayrRLTTactive = 0;
    DestroyDialog (windowTT, windowTT);
}

void
GetAYear4RLResults () {
    GtkWidget *box1, *box2, *hbox, *frame, *button, *okbutton, *separator, *stock;
    gchar w[5];
    gint year;

    getayrRLRESactive = 1;

    windowRES = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (windowRES), "destroy", G_CALLBACK (DestroyGetYearRLRES), NULL);
    gtk_window_set_title (GTK_WINDOW (windowRES), "Get a Year for Real Life Results");
    gtk_container_set_border_width (GTK_CONTAINER (windowRES), 0);
    gtk_window_set_keep_above (GTK_WINDOW (windowRES), TRUE);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (windowRES), box1);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (box1), stock, FALSE, FALSE, 0);

    frame = gtk_frame_new ("Pick a Year for Real Life Results");
    gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    comboRES = gtk_combo_box_new_text ();

    for (year = 1901; year < (MAX_YEAR + 1); year++) {
        strncpy (&w[0], (char *) cnvt_int2str (year, 'l'), 4);
        w[4] = '\0';
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboRES), w);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboRES), 55);
    gtk_container_add (GTK_CONTAINER (box2), comboRES);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    okbutton = gtk_button_new_with_label ("Get Results");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (GetYearRLRES), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), okbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyGetYearRLRES), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

    gtk_widget_show_all (windowRES);
}

void
GetYearRLRES (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *window, *box1, *box2, *table, *hbox, *button, *separator, *vscrollbar, *text, *pbutton;
    GdkFont *fixed_font;

    strcpy (&yeara[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboRES)));

    strcpy (&work[0], "S3");
    strcat (&work[0], &yeara[0]);
    strcat (&work[0], "\n");
    sock_puts (sock, work);  /* tell the server to send real life results for a specific year*/

    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get stats */

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 500, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Real Life Season Results");
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

    FillRLSeasonResults ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, stats, strlen (&stats[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    strcpy (&prtbutrlrescmd[prtbutrlrespnt][0], &work[0]);
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintStats), GINT_TO_POINTER (prtbutrlrespnt));
    prtbutrlrespnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
DestroyGetYearRLRES (GtkWidget *widget, gpointer *pdata) {
    getayrRLRESactive = 0;
    DestroyDialog (windowRES, windowRES);
}

