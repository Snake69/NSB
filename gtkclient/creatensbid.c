
/* create a NSB ID */

#include "gtknsbc.h"
#include "prototypes.h"
#include "net.h"
#include "cglobal.h"

GtkWidget *nsbidentry;
gchar tsid[256];
const gchar *entry_text;

void
CreateNSBID (int chgid) {
    GtkWidget *dlgCNSBID, *hbox, *stock, *table, *label;
    gchar labelmsg[512];
    gint response;

    tsid[0] = '\0';
    strcpy (&buffer[0], "\n");

    if (!chgid) {
        strcpy (&labelmsg[0], "You do not have an NSB ID on server ");
        strcat (&labelmsg[0], &hs[0]);
        strcat (&labelmsg[0], "\nEnter the NSB ID to use:");
    }
    else {
        strcpy (&labelmsg[0], "\nEnter the new NSB ID to use on ");
        strcat (&labelmsg[0], &hs[0]);
        strcat (&labelmsg[0], ":");
    }

    dlgCNSBID = gtk_dialog_new_with_buttons ("Create an NSB ID", GTK_WINDOW (mainwin), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgCNSBID)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic (labelmsg);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    nsbidentry = gtk_entry_new ();
    if (strlen (&tsid[0]))
        gtk_entry_set_text (GTK_ENTRY (nsbidentry), tsid);
    gtk_entry_set_max_length (GTK_ENTRY (nsbidentry), 50);
    gtk_table_attach_defaults (GTK_TABLE (table), nsbidentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), nsbidentry);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgCNSBID), GTK_RESPONSE_OK);
    gtk_entry_set_activates_default (GTK_ENTRY (nsbidentry), TRUE);

    gtk_widget_show_all (hbox);
    response = gtk_dialog_run (GTK_DIALOG (dlgCNSBID));

    if (response == GTK_RESPONSE_OK) {
        entry_text = gtk_entry_get_text (GTK_ENTRY (nsbidentry));
        strcpy (&tsid[0], entry_text);

        strcpy (&buffer[0], &tsid[0]);          /* save user response */
        strcat (&buffer[0], "\n");              /* Add line-feed */
    }
    else
        buffer[0] = '\0';

    DestroyDialog (dlgCNSBID, dlgCNSBID);
}

