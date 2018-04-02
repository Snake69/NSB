
/* get the name of the user-created team to load */

#include "gtknsbc.h"
#include "prototypes.h"
#include "net.h"
#include "cglobal.h"

GtkWidget *tfnameentry, *tfnewnameentry;
gchar teamfname[256], teamnewfname[256];
const gchar *entry_text;

int
GetTeamFileName (int which) {
    GtkWidget *dlgTFName, *hbox, *stock, *label, *table;
    gchar labelmsg[512], labelnewmsg[512];
    gint response, x;

    teamfname[0] = teamnewfname[0] = '\0';
    if (which == 1) {
        strcpy (&labelmsg[0], "Enter the name of your user-created team to load:");
        dlgTFName = gtk_dialog_new_with_buttons ("Get Team Filename to Load", GTK_WINDOW (mainwin),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    }
    else
        if (which == 2) {
            strcpy (&labelmsg[0], "Enter the name of your user-created team to delete:");
            dlgTFName = gtk_dialog_new_with_buttons ("Get Team Filename to Delete", GTK_WINDOW (mainwin),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        }
        else {
            strcpy (&labelmsg[0], "Enter the name of your user-created team to rename:");
            strcpy (&labelnewmsg[0], "Enter the new name:");
            dlgTFName = gtk_dialog_new_with_buttons ("Get Team Filename to Rename", GTK_WINDOW (mainwin),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        }

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgTFName)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    table = gtk_table_new (3, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic (labelmsg);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    if (which == 3) {
        label = gtk_label_new_with_mnemonic (labelnewmsg);
        gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    }

    tfnameentry = gtk_entry_new ();
    if (strlen (&teamfname[0]))
        gtk_entry_set_text (GTK_ENTRY (tfnameentry), teamfname);
    gtk_entry_set_max_length (GTK_ENTRY (tfnameentry), 50);
    gtk_table_attach_defaults (GTK_TABLE (table), tfnameentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), tfnameentry);
    if (which == 3) {
        tfnewnameentry = gtk_entry_new ();
        gtk_signal_connect (GTK_OBJECT (tfnewnameentry), "insert_text", GTK_SIGNAL_FUNC (CheckTName), NULL);
        if (strlen (&teamnewfname[0]))
            gtk_entry_set_text (GTK_ENTRY (tfnewnameentry), teamnewfname);
        gtk_entry_set_max_length (GTK_ENTRY (tfnewnameentry), 48);
        gtk_table_attach_defaults (GTK_TABLE (table), tfnewnameentry, 1, 2, 1, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), tfnewnameentry);
    }

    gtk_dialog_set_default_response (GTK_DIALOG (dlgTFName), GTK_RESPONSE_OK);
    gtk_entry_set_activates_default (GTK_ENTRY (tfnameentry), TRUE);

    gtk_widget_show_all (hbox);
    response = gtk_dialog_run (GTK_DIALOG (dlgTFName));

    if (response == GTK_RESPONSE_OK) {
        entry_text = gtk_entry_get_text (GTK_ENTRY (tfnameentry));
        if (strlen (entry_text)) {
            strcpy (&teamfname[0], entry_text);
            strcpy (&tfname[0], &teamfname[0]);          /* save user response */
            x = 1;
        }
        else {
            tfname[0] = '\0';
            x = 0;
            goto GetOut;
        }
        if (which == 3) {
            entry_text = gtk_entry_get_text (GTK_ENTRY (tfnewnameentry));
            if (strlen (entry_text)) {
                strcpy (&teamnewfname[0], entry_text);
                strcpy (&tfnewname[0], &teamnewfname[0]);          /* save user response */
                x = 1;
            }
            else {
                tfnewname[0] = '\0';
                x = 0;
            }
        }
    }
    else {
        tfname[0] = '\0';
        x = 0;
    }
GetOut:
    DestroyDialog (dlgTFName, dlgTFName);
    return (x);
}

