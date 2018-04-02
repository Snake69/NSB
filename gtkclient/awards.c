/* display some real life award winners */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "db.h"
#include "net.h"

void
MVP (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *mvp;
    gint nomvp;

    /* load the data if it exists */
    if ((mvp = fopen ("/usr/local/share/NSB/MVP.txt", "r")) != NULL) {
        nomvp = fread (&mvptext, sizeof mvptext, 1, mvp);
        if (!ferror (mvp))
            nomvp = 0;
        else
            /* error reading file */
            nomvp = 1;
        fclose (mvp);
    }
    else
        /* error opening data file */
        nomvp = 1;

    if (nomvp) {
        gchar NoMVPAvailable[256] = "MVP data is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "MVP data is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoMVPAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 780, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "MVPs");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, mvptext, strlen (&mvptext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintMVP), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
PrintMVP (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing MVPs ...", *msg[5];
    gint x;

    print (&mvptext[0]);

    strcpy (&work[0], "Print MVPs.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintCyYoung (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Cy Youngs ...", *msg[5];
    gint x;

    print (&cytext[0]);

    strcpy (&work[0], "Print Cy Youngs.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintGoldGlove (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Gold Gloves ...", *msg[5];
    gint x;

    print (&ggtext[0]);

    strcpy (&work[0], "Print Gold Gloves.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintSilverSlugger (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Silver Sluggers ...", *msg[5];
    gint x;

    print (&sstext[0]);

    strcpy (&work[0], "Print Silver Sluggers.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintRookie (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Rookies of the Year ...", *msg[5];
    gint x;

    print (&rookietext[0]);

    strcpy (&work[0], "Print Rookies of the Year.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
CyYoung (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *cy;
    gint nocy;

    /* load the data if it exists */
    if ((cy = fopen ("/usr/local/share/NSB/CyYoung.txt", "r")) != NULL) {
        nocy = fread (&cytext, sizeof cytext, 1, cy);
        if (!ferror (cy))
            nocy = 0;
        else
            /* error reading file */
            nocy = 1;
        fclose (cy);
    }
    else
        /* error opening data file */
        nocy = 1;

    if (nocy) {
        gchar NoCYAvailable[256] = "Cy Young data is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Cy Young data is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoCYAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 780, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Cy Youngs");
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
    text = gtk_text_new (NULL, NULL); gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, cytext, strlen (&cytext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintCyYoung), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
GoldGlove (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *gg;
    gint nogg;

    /* load the data if it exists */
    if ((gg = fopen ("/usr/local/share/NSB/GoldGlove.txt", "r")) != NULL) {
        nogg = fread (&ggtext, sizeof ggtext, 1, gg);
        if (!ferror (gg))
            nogg = 0;
        else
            /* error reading file */
            nogg = 1;
        fclose (gg);
    }
    else
        /* error opening data file */
        nogg = 1;

    if (nogg) {
        gchar NoGGAvailable[256] = "Gold Glove data is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Gold Glove data is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoGGAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 780, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Gold Gloves");
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
    text = gtk_text_new (NULL, NULL); gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, ggtext, strlen (&ggtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintGoldGlove), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
SilverSlugger (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *ss;
    gint noss;

    /* load the data if it exists */
    if ((ss = fopen ("/usr/local/share/NSB/SilverSlugger.txt", "r")) != NULL) {
        noss = fread (&sstext, sizeof sstext, 1, ss);
        if (!ferror (ss))
            noss = 0;
        else
            /* error reading file */
            noss = 1;
        fclose (ss);
    }
    else
        /* error opening data file */
        noss = 1;

    if (noss) {
        gchar NoSSAvailable[256] = "Silver Slugger data is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Silver Slugger data is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoSSAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 500, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Silver Sluggers");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, sstext, strlen (&sstext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintSilverSlugger), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
Rookie (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *rk;
    gint nork;

    /* load the data if it exists */
    if ((rk = fopen ("/usr/local/share/NSB/Rookie.txt", "r")) != NULL) {
        nork = fread (&rookietext, sizeof rookietext, 1, rk);
        if (!ferror (rk))
            nork = 0;
        else
            /* error reading file */
            nork = 1;
        fclose (rk);
    }
    else
        /* error opening data file */
        nork = 1;

    if (nork) {
        gchar NoRKAvailable[256] = "Rookie of the Year data is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Rookie of the Year data is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoRKAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 780, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Rookies of the Year");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, rookietext, strlen (&rookietext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintRookie), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

gchar buf[5000];

void
NSBMVP (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    /* show MVP voting for current season (regular season must have completed) */
    gchar NotEOS[256] = "MVP selection does not occur until after the regular season ends.", *msg[5];
    gint x;
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "AM\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "-1")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Regular season not yet completed.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotEOS[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "MVP - Current NSB Season");
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

    FillNSBMVP ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, NSBMVPtext, strlen (&NSBMVPtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintNSBMVP), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillNSBMVP () {
    gint x, y, teamid, clen;
    gchar hdr[200], year[5], w[100], w2[100], *cc, *work, charid[3];
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50],
             score[20];
    } list[20];

    for (x = 0; x < 20; x++)
        list[x].name[0] = '\0';

    strcpy (&hdr[0], "                     MVP - Current NSB Season\n\n");
    strcat (&hdr[0], "       Name, Team                                    Score ");

    for (x = 0, cc = &buf[0]; x < 20 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        work = (char *) index (cc, '?');
        *work = '\0';

        if (*cc == ' ')
            /* a blank name signifies no more data */
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

        strncpy (&list[x].score[0], cc, 10);
        list[x].score[10] = '\0';
        cc += 10;
    }

    strcpy (&NSBMVPtext[0], &hdr[0]);
    strcat (&NSBMVPtext[0], "\n\n");
    for (clen = x = 0; x < 20; x++, clen = 0) {
        if (!strlen (&list[x].name[0]))
            break;

        strcat (&NSBMVPtext[0], " ");
        strcat (&NSBMVPtext[0], &list[x].name[0]);
        strcat (&NSBMVPtext[0], ", ");
        clen += strlen (&list[x].name[0]);
        clen += 3;
        if (list[x].tyr[0] != '0') {
            strcat (&NSBMVPtext[0], &list[x].tyr[0]);
            strcat (&NSBMVPtext[0], " ");
            strcat (&NSBMVPtext[0], &list[x].tabbr[0]);
            clen += strlen (&list[x].tabbr[0]);
            clen += 5;
        }
        else {
            strcat (&NSBMVPtext[0], &list[x].uctname[0]);
            clen += strlen (&list[x].uctname[0]);
        }
        strncat (&NSBMVPtext[0], "                                                 ", (50 - clen));
        strcat (&NSBMVPtext[0], &list[x].score[0]);
        strcat (&NSBMVPtext[0], "\n");
    }
    strcat (&NSBMVPtext[0], "\n\n");
}

void
NSBCyYoung (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    /* show Cy Young voting for current season (regular season must have completed) */
    gchar NotEOS[256] = "Cy Young selection does not occur until after the regular season ends.", *msg[5];
    gint x;
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "AC\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "-1")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Regular season not yet completed.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotEOS[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Cy Young - Current NSB Season");
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

    FillNSBCyYoung ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, NSBCyYoungtext, strlen (&NSBCyYoungtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintNSBCyYoung), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
NSBGoldGlove (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    /* show Gold Glove winners for current season (regular season must have completed) */
    gchar NotEOS[256] = "Gold Glove selections do not occur until after the regular season ends.", *msg[5];
    gint x;
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "AG\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "-1")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Regular season not yet completed.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotEOS[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Gold Gloves - Current NSB Season");
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

    FillNSBGoldGlove ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, NSBGoldGlovetext, strlen (&NSBGoldGlovetext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintNSBGoldGlove), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillNSBCyYoung () {
    gint x, y, teamid, clen;
    gchar hdr[200], year[5], w[100], w2[100], *cc, *work, charid[3];
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50],
             score[20];
    } list[20];

    for (x = 0; x < 20; x++)
        list[x].name[0] = '\0';

    strcpy (&hdr[0], "                   Cy Young - Current NSB Season\n\n");
    strcat (&hdr[0], "       Name, Team                                    Score ");

    for (x = 0, cc = &buf[0]; x < 20 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        work = (char *) index (cc, '?');
        *work = '\0';

        if (*cc == ' ')
            /* a blank name signifies no more data */
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

        strncpy (&list[x].score[0], cc, 10);
        list[x].score[10] = '\0';
        cc += 10;
    }

    strcpy (&NSBCyYoungtext[0], &hdr[0]);
    strcat (&NSBCyYoungtext[0], "\n\n");
    for (clen = x = 0; x < 20; x++, clen = 0) {
        if (!strlen (&list[x].name[0]))
            break;

        strcat (&NSBCyYoungtext[0], " ");
        strcat (&NSBCyYoungtext[0], &list[x].name[0]);
        strcat (&NSBCyYoungtext[0], ", ");
        clen += strlen (&list[x].name[0]);
        clen += 3;
        if (list[x].tyr[0] != '0') {
            strcat (&NSBCyYoungtext[0], &list[x].tyr[0]);
            strcat (&NSBCyYoungtext[0], " ");
            strcat (&NSBCyYoungtext[0], &list[x].tabbr[0]);
            clen += strlen (&list[x].tabbr[0]);
            clen += 5;
        }
        else {
            strcat (&NSBCyYoungtext[0], &list[x].uctname[0]);
            clen += strlen (&list[x].uctname[0]);
        }
        strncat (&NSBCyYoungtext[0], "                                                 ", (50 - clen));
        strcat (&NSBCyYoungtext[0], &list[x].score[0]);
        strcat (&NSBCyYoungtext[0], "\n");
    }
    strcat (&NSBCyYoungtext[0], "\n\n");
}

void
FillNSBGoldGlove () {
    gint x, y, teamid, clen;
    gchar hdr[200], year[5], w[100], w2[100], *cc, *work, charid[3];
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50];
    } list[9];

    strcpy (&hdr[0], "                Gold Gloves - Current NSB Season\n\n");
    strcat (&hdr[0], "       Name, Team                               Position ");

    for (x = 0, cc = &buf[0]; x < 9 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        work = (char *) index (cc, '?');
        *work = '\0';

        if (*cc == ' ')
            /* a blank name signifies no more data */
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

        cc += 10;
    }

    strcpy (&NSBGoldGlovetext[0], &hdr[0]);
    strcat (&NSBGoldGlovetext[0], "\n\n");
    for (clen = x = 0; x < 9; x++, clen = 0) {
        strcat (&NSBGoldGlovetext[0], " ");
        strcat (&NSBGoldGlovetext[0], &list[x].name[0]);
        strcat (&NSBGoldGlovetext[0], ", ");
        clen += strlen (&list[x].name[0]);
        clen += 3;
        if (list[x].tyr[0] != '0') {
            strcat (&NSBGoldGlovetext[0], &list[x].tyr[0]);
            strcat (&NSBGoldGlovetext[0], " ");
            strcat (&NSBGoldGlovetext[0], &list[x].tabbr[0]);
            clen += strlen (&list[x].tabbr[0]);
            clen += 5;
        }
        else {
            strcat (&NSBGoldGlovetext[0], &list[x].uctname[0]);
            clen += strlen (&list[x].uctname[0]);
        }
        strncat (&NSBGoldGlovetext[0], "                                                 ", (50 - clen));
        strcat (&NSBGoldGlovetext[0], (char *) figure_pos (x + 1));
        strcat (&NSBGoldGlovetext[0], "\n");
    }
    strcat (&NSBGoldGlovetext[0], "\n\n");
}

void
FillNSBSilverSlugger () {
    gint x, y, teamid, clen;
    gchar hdr[200], year[5], w[100], w2[100], *cc, *work, charid[3];
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50];
    } list[10];

    strcpy (&hdr[0], "              Silver Sluggers - Current NSB Season\n\n");
    strcat (&hdr[0], "       Name, Team                               Position ");

    for (x = 0, cc = &buf[0]; x < 10 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        work = (char *) index (cc, '?');
        *work = '\0';

        if (*cc == ' ')
            /* a blank name signifies no more data */
            break;
        if (!strncmp (cc, "NONAME", 6)) {
            strcpy (&list[x].name[0], "NONAME");
            goto SkipReverse;
        }

        /* reverse name */
        strcpy (&w2[0], cc);
        strcpy (&w[0], &w2[index (&w2[0], ',') - &w2[0] + 2]);
        strcat (&w[0], " ");
        strncat (&w[0], &w2[0], (index (&w2[0], ',') - &w2[0]));
        strcpy (&list[x].name[0], &w[0]);
SkipReverse:
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

        cc += 10;
    }

    strcpy (&NSBSilverSluggertext[0], &hdr[0]);
    strcat (&NSBSilverSluggertext[0], "\n\n");
    for (clen = x = 0; x < 10; x++, clen = 0) {
        if (!strncmp (&list[x].name[0], "NONAME", 6))
            continue;

        strcat (&NSBSilverSluggertext[0], " ");
        strcat (&NSBSilverSluggertext[0], &list[x].name[0]);
        strcat (&NSBSilverSluggertext[0], ", ");
        clen += strlen (&list[x].name[0]);
        clen += 3;
        if (list[x].tyr[0] != '0') {
            strcat (&NSBSilverSluggertext[0], &list[x].tyr[0]);
            strcat (&NSBSilverSluggertext[0], " ");
            strcat (&NSBSilverSluggertext[0], &list[x].tabbr[0]);
            clen += strlen (&list[x].tabbr[0]);
            clen += 5;
        }
        else {
            strcat (&NSBSilverSluggertext[0], &list[x].uctname[0]);
            clen += strlen (&list[x].uctname[0]);
        }
        strncat (&NSBSilverSluggertext[0], "                                                 ", (50 - clen));
        strcat (&NSBSilverSluggertext[0], (char *) figure_pos (x));
        strcat (&NSBSilverSluggertext[0], "\n");
    }
    strcat (&NSBSilverSluggertext[0], "\n\n");
}

void
NSBSilverSlugger (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    /* show Silver Slugger winners for current season (regular season must have completed) */
    gchar NotEOS[256] = "Silver Slugger selections do not occur until after the regular season ends.", *msg[5];
    gint x;
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "AS\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "-1")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Regular season not yet completed.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotEOS[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Silver Sluggers - Current NSB Season");
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

    FillNSBSilverSlugger ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, NSBSilverSluggertext, strlen (&NSBSilverSluggertext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintNSBSilverSlugger), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
PrintNSBMVP (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Current NSB Season MVP ...", *msg[5];
    gint x;

    print (&NSBMVPtext[0]);

    strcpy (&work[0], "Print Current NSB Season MVP.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintNSBCyYoung (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Current NSB Season Cy Young ...", *msg[5];
    gint x;

    print (&NSBCyYoungtext[0]);

    strcpy (&work[0], "Print Current NSB Season Cy Young.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintNSBGoldGlove (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Current NSB Season Gold Gloves ...", *msg[5];
    gint x;

    print (&NSBGoldGlovetext[0]);

    strcpy (&work[0], "Print Current NSB Season Gold Gloves.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
PrintNSBSilverSlugger (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Current NSB Season Silver Sluggers ...", *msg[5];
    gint x;

    print (&NSBSilverSluggertext[0]);

    strcpy (&work[0], "Print Current NSB Season Silver Sluggers.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

