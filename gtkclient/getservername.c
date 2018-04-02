
/* get the name of the NSB server to connect to */

#include "gtknsbc.h"
#include "prototypes.h"
#include "net.h"
#include "cglobal.h"

GtkWidget *serverentry;
gchar server[256];
const gchar *entry_text;

int
GetServerName () {
    GtkWidget *dlgServerID, *hbox, *stock, *label, *table;
    gchar labelmsg[512];
    gint response, x;

    server[0] = '\0';
    strcpy (&labelmsg[0], "Enter the name of the server to which to connect:\n(default - localhost)");

    dlgServerID = gtk_dialog_new_with_buttons ("Connect to NSB Server", GTK_WINDOW (mainwin),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgServerID)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic (labelmsg);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    serverentry = gtk_entry_new ();
    if (strlen (&server[0]))
        gtk_entry_set_text (GTK_ENTRY (serverentry), server);
    gtk_entry_set_max_length (GTK_ENTRY (serverentry), 50);
    gtk_table_attach_defaults (GTK_TABLE (table), serverentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), serverentry);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgServerID), GTK_RESPONSE_OK);
    gtk_entry_set_activates_default (GTK_ENTRY (serverentry), TRUE);

    gtk_widget_show_all (hbox);
    response = gtk_dialog_run (GTK_DIALOG (dlgServerID));

    if (response == GTK_RESPONSE_OK) {
        entry_text = gtk_entry_get_text (GTK_ENTRY (serverentry));
        strcpy (&server[0], entry_text);

        strcpy (&hs[0], &server[0]);          /* save user response */
        x = 1;
    }
    else {
        hs[0] = '\0';
        x = 0;
    }

    DestroyDialog (dlgServerID, dlgServerID);
    return (x);
}

