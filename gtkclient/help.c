
/* display help */

#include "gtknsbc.h"
#include "prototypes.h"

void
Help (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    if (nohelp) {
        gchar NoHelpAvailable[256] = "No help available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Help information is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoHelpAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 780, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Help");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, helptext, strlen (&helptext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintHelp), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
PrintHelp (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Help ...", *msg[5];
    gint x;

    print (&helptext[0]);

    strcpy (&work[0], "Print help.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
Abbrev (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    if (noabbr) {
        gchar NoAbbrAvailable[256] = "No abbreviations list available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Abbreviations information is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoAbbrAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 400, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Abbreviations");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, abbrtext, strlen (&abbrtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintAbbrev), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);

}

void
PrintAbbrev (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Abbreviations ...", *msg[5];
    gint x;

    print (&abbrtext[0]);

    strcpy (&work[0], "Print abbreviations.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

