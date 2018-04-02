
/* the "About GTK Client for NSB" dialog */

#include "gtknsbc.h"
#include "prototypes.h"

void
AboutGTKNSBClient (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    char labeltext[500];
    GtkWidget *dlgFile, *label, *button;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "About GTK Client for NSB");

    strcpy (&labeltext[0], "  \n  ");
    strcat (&labeltext[0], APPTITLE);
    strcat (&labeltext[0], "  \n  ");
    strcat (&labeltext[0], APPVERSION);
    strcat (&labeltext[0], "  \n\n");
    strcat (&labeltext[0], "  \n  A GTK client to talk to a NetStatsBaseball (NSB) server  \n");

    label = gtk_label_new (labeltext);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("DISMISS");
    gtk_signal_connect (GTK_OBJECT (button), "clicked", (GtkSignalFunc) DestroyDialog, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_show_all (dlgFile);
}

