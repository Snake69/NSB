
/* list the users on the currently connected server  */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

gchar userinfo[1024];

void
Users (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *dlgUsers, *box1, *box2, *hbox, *separator, *prbutton, *disbutton, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "U\n");                       /* tell the server */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        gchar UsersNotAvailable[256] = "An error was encountered when trying to retrieve the users.",
              Unconnected[256] = "You need to connect to an NSB server.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (!connected) {
            strcpy (&work[0], "Not connected.\n\n");
            Add2TextWindow (&work[0], 0);

            msg[0] = &Unconnected[0];
        }
        else {
            strcpy (&work[0], "Users not available.\n\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &UsersNotAvailable[0];
        }

        outMessage (msg);
        return;
    }

    /*
       data received from server will be in the form -

       repeat for each user:
         user NSBID (variable number of characters)
         TAB (1 character)
         user login name (variable number of characters)
         TAB (1 character)
         user site (variable number of characters)
         TAB (1 character)
         NSB login count (6 characters)
         TAB (1 character)
    */

    dlgUsers = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgUsers), "Users");
    gtk_window_set_default_size (GTK_WINDOW (dlgUsers), 800, 250);
    gtk_signal_connect (GTK_OBJECT (dlgUsers), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgUsers)->vbox), box1, TRUE, TRUE, 0);

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

    FillUserInfo ();
    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, userinfo, strlen (&userinfo[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    prbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (prbutton), "clicked", G_CALLBACK (PrintUsers), dlgUsers);
    disbutton = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroyDialog), dlgUsers);
    gtk_box_pack_start (GTK_BOX (hbox), prbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgUsers);
}

void
FillUserInfo () {
    gchar *cc, *cc1, *cc2, *ns;
    gint x;

    strcpy (&userinfo[0], "\n                               Users With Accounts on ");
    strcat (&userinfo[0], &hs[0]);
    strcat (&userinfo[0], "\n\n\n");
    strcat (&userinfo[0], "  NSB ID               add'l ID info                               Connect Count\n\n");

    for (cc = &buffer[0]; cc < (&buffer[0] + strlen (&buffer[0])); ) {
        strcat (&userinfo[0], "  ");

        /* move NSB ID */
        ns = (char *) index (cc, '\t');
        strncat (&userinfo[0], cc, (ns - cc));
        strncat (&userinfo[0], "                     ", (21 - (ns - cc)));
        cc = ns + 1;

        /* move user */
        ns = (char *) index (cc, '\t');
        strncat (&userinfo[0], cc, (ns - cc));
        strcat (&userinfo[0], "@");
        cc1 = cc;
        cc = ns + 1;

        /* move site */
        ns = (char *) index (cc, '\t');
        strncat (&userinfo[0], cc, (ns - cc));
        strncat (&userinfo[0], "                                               ", (47 - (ns - cc1 + 1)));
        cc = ns + 1;

        /* move connect count */
        ns = (char *) index (cc, '\t');
        for (cc2 = cc, x = 0; *cc2 == '0' && x < 5; x++, cc2++)
            *cc2 = ' ';
        strncat (&userinfo[0], cc, (ns - cc));
        cc = ns + 1;

        strcat (&userinfo[0], "\n");
    }
}

void
PrintUsers (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Users ...", *msg[5];
    gint x;

    print (&userinfo[0]);

    strcpy (&work[0], "Print Users on server ");
    strcat (&work[0], &hs[0]);
    strcat (&work[0], "\n\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

