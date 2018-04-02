
/* set the Preferences */

#include "gtknsbc.h"
#include "cglobal.h"
#include "prototypes.h"

GtkWidget *dlgDset, *label, *button1, *button2, *vbox, *separator, *checkSL, *checkIUC, *checkSND, *checkPBP, *checkPP, *checkTDIB, *checkMPP, *checkAAY,
          *vbox2, *hbox, *SECsp, *QSECsp;
GtkAdjustment *adj;
int QSEC;

void
Preferences (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    dlgDset = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (dlgDset), "Preferences");
    g_signal_connect (G_OBJECT (dlgDset), "destroy", G_CALLBACK (DestroyDialog), dlgDset);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (dlgDset), vbox);

    label = gtk_label_new ("PREFERENCES\n\n");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    separator = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (vbox), separator);

    vbox2 = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);
    gtk_container_add (GTK_CONTAINER (vbox), vbox2);

    checkSL = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkSL, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkSL), "clicked", G_CALLBACK (toggle_SL), NULL);
    checkIUC = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkIUC, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkIUC), "clicked", G_CALLBACK (toggle_IUC), NULL);
    checkSND = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkSND, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkSND), "clicked", G_CALLBACK (toggle_SND), NULL);
    checkPBP = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkPBP, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkPBP), "clicked", G_CALLBACK (toggle_PBP), NULL);
    checkPP = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkPP, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkPP), "clicked", G_CALLBACK (toggle_PP), NULL);
    checkTDIB = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkTDIB, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkTDIB), "clicked", G_CALLBACK (toggle_TDIB), NULL);
    checkMPP = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkMPP, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkMPP), "clicked", G_CALLBACK (toggle_MPP), NULL);
    checkAAY = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkAAY, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkAAY), "clicked", G_CALLBACK (toggle_AAY), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (TRUE, 4);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    adj = (GtkAdjustment *) gtk_adjustment_new ((float) preferences.Speed_sec, 0.0, 100.0, 1.0, 100.0, 0.0);
    SECsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (SECsp), FALSE);
    gtk_widget_set_size_request (SECsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (SECsp), TRUE);
    g_signal_connect (SECsp, "changed", G_CALLBACK (SpinnerChangePre1), NULL);
    g_signal_connect (SECsp, "value-changed", G_CALLBACK (SpinnerChangePre1), NULL);

    label = gtk_label_new ("Gameplay Speed (time between actions) -\n\n");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    label = gtk_label_new ("Seconds:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), SECsp, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    QSEC = preferences.Speed_nsec;

    adj = (GtkAdjustment *) gtk_adjustment_new ((float) QSEC, 0.0, 4.0, 1.0, 4.0, 0.0);
    QSECsp = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (QSECsp), FALSE);
    gtk_widget_set_size_request (QSECsp, 3, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (QSECsp), TRUE);
    g_signal_connect (QSECsp, "changed", G_CALLBACK (SpinnerChangePre2), NULL);
    g_signal_connect (QSECsp, "value-changed", G_CALLBACK (SpinnerChangePre2), NULL);

    label = gtk_label_new ("Quarter-Seconds:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), QSECsp, TRUE, TRUE, 0);

    if (preferences.ShowStartingLineups) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSL), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkSL), "Show Starting Lineups Before Any Single Game");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSL), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkSL), "Do Not Show Starting Lineups Before Any Single Game");
    }
    if (preferences.IncludeUCTeams) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkIUC), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkIUC), "Include User-Created Teams in Random Team-Selection Pools");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkIUC), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkIUC), "Do Not Include User-Created Teams in Random Pools");
    }
    if (preferences.PlaySounds) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSND), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkSND), "Play Sounds");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSND), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkSND), "Do Not Play Sounds");
    }
    if (preferences.SpeakPBP) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkPBP), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkPBP), "Speak Play-By-Play (Gameplay Speed must be 3 seconds or greater for speech to function)");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkPBP), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkPBP), "Do Not Speak Play-By-Play");
    }
    if (preferences.ShowPlayerPics) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkPP), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkPP), "Show Player Pics During Gameplay");
        gtk_widget_set_sensitive (GTK_WIDGET (checkMPP), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkPP), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkPP), "Do Not Show Player Pics During Gameplay");
        gtk_widget_set_sensitive (GTK_WIDGET (checkMPP), FALSE);
    }
    if (preferences.ShowTDIBAtBoot) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkTDIB), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkTDIB), "Show \"This Day in Baseball\" at Boot");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkTDIB), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkTDIB), "Do Not Show \"This Day in Baseball\" at Boot");
    }
    if (preferences.MovingPlayerPics) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkMPP), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkMPP), "Make Player Pics Moveable During Gameplay");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkMPP), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkMPP), "Each New Player Pic During Gameplay Will Display in the Same Location");
    }
    if (preferences.AssumeAllYears) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkAAY), TRUE);
        gtk_button_set_label (GTK_BUTTON (checkAAY), "Don't ask to \"assume all years\" when \"Include Year(s)\" field is empty");
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkAAY), FALSE);
        gtk_button_set_label (GTK_BUTTON (checkAAY), "Ask to \"assume all years\" when \"Include Year(s)\" field is empty");
    }

    separator = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (vbox), separator);

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_container_add (GTK_CONTAINER (vbox), hbox);

    button1 = gtk_button_new_with_label ("Save and Dismiss");
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    gtk_signal_connect (GTK_OBJECT (button1), "clicked", G_CALLBACK (DSfinished), button1);
    button2 = gtk_button_new_with_label ("Dismiss");
    gtk_container_add (GTK_CONTAINER (hbox), button2);
    gtk_signal_connect (GTK_OBJECT (button2), "clicked", G_CALLBACK (DSfinished), button2);

    gtk_widget_show_all (dlgDset);
}

void
DSfinished (GtkWidget *widget, gpointer data) {
    FILE *rc;
    gint x;
    gchar *msg[5], *msg2 = "cannot be written.";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (data == button1) {
        /* save the NSB rc file */
        if ((rc = fopen (DefSetPath, "w")) != NULL) {
            fwrite (&preferences, sizeof preferences, 1, rc);
            fclose (rc);
        }
        else {
            msg[0] = &DefSetPath[0];
            msg[1] = msg2;
            outMessage (msg);
        }
    }
    DestroyDialog (dlgDset, dlgDset);
}

void
toggle_SL (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkSL))) {
        preferences.ShowStartingLineups = 1;
        gtk_button_set_label (GTK_BUTTON (checkSL), "Show Starting Lineups Before Any Single Game");
    }
    else {
        preferences.ShowStartingLineups = 0;
        gtk_button_set_label (GTK_BUTTON (checkSL), "Do Not Show Starting Lineups Before Any Single Game");
    }
}

void
toggle_IUC (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkIUC))) {
        preferences.IncludeUCTeams = 1;
        gtk_button_set_label (GTK_BUTTON (checkIUC), "Include User-Created Teams in Random Team-Selection Pools");
    }
    else {
        preferences.IncludeUCTeams = 0;
        gtk_button_set_label (GTK_BUTTON (checkIUC), "Do Not Include User-Created Teams in Random Pools");
    }
}

void
toggle_SND (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkSND))) {
        preferences.PlaySounds = 1;
        gtk_button_set_label (GTK_BUTTON (checkSND), "Play Sounds");
    }
    else {
        preferences.PlaySounds = 0;
        gtk_button_set_label (GTK_BUTTON (checkSND), "Do Not Play Sounds");
    }
}

void
toggle_PBP (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkPBP))) {
        preferences.SpeakPBP = 1;
        gtk_button_set_label (GTK_BUTTON (checkPBP), "Speak Play-By-Play (Gameplay Speed must be 3 seconds or greater for speech to function)");
    }
    else {
        preferences.SpeakPBP = 0;
        gtk_button_set_label (GTK_BUTTON (checkPBP), "Do Not Speak Play-By-Play");
    }
}

void
toggle_PP (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkPP))) {
        preferences.ShowPlayerPics = 1;
        gtk_button_set_label (GTK_BUTTON (checkPP), "Show Player Pics During Gameplay");
        gtk_widget_set_sensitive (GTK_WIDGET (checkMPP), TRUE);
    }
    else {
        preferences.ShowPlayerPics = 0;
        gtk_button_set_label (GTK_BUTTON (checkPP), "Do Not Show Player Pics During Gameplay");
        gtk_widget_set_sensitive (GTK_WIDGET (checkMPP), FALSE);
    }
}

void
toggle_TDIB (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkTDIB))) {
        preferences.ShowTDIBAtBoot = 1;
        gtk_button_set_label (GTK_BUTTON (checkTDIB), "Show \"This Day in Baseball\" at Boot");
    }
    else {
        preferences.ShowTDIBAtBoot = 0;
        gtk_button_set_label (GTK_BUTTON (checkTDIB), "Do Not Show \"This Day in Baseball\" at Boot");
    }
}

void
toggle_MPP (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkMPP))) {
        preferences.MovingPlayerPics = 1;
        gtk_button_set_label (GTK_BUTTON (checkMPP), "Make Player Pics Moveable During Gameplay");
    }
    else {
        preferences.MovingPlayerPics = 0;
        gtk_button_set_label (GTK_BUTTON (checkMPP), "Each New Player Pic During Gameplay Will Display in the Same Location");
    }
}

void
toggle_AAY (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkAAY))) {
        preferences.AssumeAllYears = 1;
        gtk_button_set_label (GTK_BUTTON (checkAAY), "Don't ask to \"assume all years\" when \"Include Year(s)\" field is empty");
    }
    else {
        preferences.AssumeAllYears = 0;
        gtk_button_set_label (GTK_BUTTON (checkAAY), "Ask to \"assume all years\" when \"Include Year(s)\" field is empty");
    }
}

void
SpinnerChangePre1 () {
    preferences.Speed_sec = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (SECsp));
}

void
SpinnerChangePre2 () {
    QSEC = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (QSECsp));
    if (QSEC == 4) {
        QSEC = 0;
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (QSECsp), 0);

        if (preferences.Speed_sec < 100)
            preferences.Speed_sec++;
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (SECsp), preferences.Speed_sec);
    }
    preferences.Speed_nsec = QSEC;
}

