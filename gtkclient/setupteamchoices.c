/* determine how user wants to create a team */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

GtkWidget *dlgFile, *vboxall, *vbox2, *checkSP, *check1CL, *sw, *formPPentry, *formSPentry, *formRPentry, *offminentry, *spitminentry, *rpitminentry,
          *comboTN;
gint toggle_o0, toggle_ha, swALL, useSP, use1CL, formulaPPchanged, formulaSPchanged, formulaRPchanged, loadPPformulasw = 0, loadSPformulasw = 0,
     loadRPformulasw = 0;
gchar ldformula[4096], ifilenamePP[512], ifilenameSP[512], ifilenameRP[512], *formentry_text, *formPPentry_text, *formSPentry_text, *formRPentry_text,
      utname[256];
GtkWidget *exentry, *inentry, *tentry;
gint yrs[YEAR_SPREAD] /* 1901-MAX-YEAR */ ;
const gchar *entry_text, *inentry_text, *exentry_text;

void
SetUpTeamChoices (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *vbox, *hbox, *separator, *label, *buttonh, *buttona, *buttonTN, *buttonALL, *table, *tlabel, *formvalbut, *formcharbut,
              *formsavbut, *formloadbut, *disbutton, *okbutton, *button0, *buttono;
    GSList *group, *group2;
    gchar *msg[5], stn[NUMBER_OF_TEAMS + 1][200];
    gint x, y;

    if (getACactive)
        /* only need to execute this dialog if it isn't already active */
        return;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;
    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

    if (ThreadRunning) {
        msg[0] = "You cannot create a team while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before creating your team.";
        outMessage (msg);
        return;
    }

    /* check if max number of user-created teams */
    sock_puts (sock, "f\n");
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }
    if (!strncmp (&buffer[0], "MAX", 3)) {
        msg[0] = "You already have 500 user-created teams.  That is the max. ";
        msg[1] = "You need to delete a team before you may create a new team. ";
        msg[2] = "To delete a team use Administration->Create, Edit, Rename or Delete Team->Create/Edit by Hand";
        outMessage (msg);
        return;
    }

    getACactive = 1;
    utname[0] = '\0';

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "How to Create Team");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    vbox = gtk_vbox_new (TRUE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlgFile)->vbox), vbox);

    vbox2 = gtk_hbox_new (TRUE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
    gtk_container_add (GTK_CONTAINER (vbox), vbox2);

    label = gtk_label_new ("Select one:");
    gtk_container_add (GTK_CONTAINER (vbox2), label);

    buttonh = gtk_radio_button_new_with_label (NULL, "Create/Edit By Hand");
    gtk_container_add (GTK_CONTAINER (vbox2), buttonh);
    g_signal_connect (G_OBJECT (buttonh), "clicked", G_CALLBACK (toggle_method_h2), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (buttonh));
    buttona = gtk_radio_button_new_with_label (group, "Auto Create New Team");
    gtk_container_add (GTK_CONTAINER (vbox2), buttona);
    g_signal_connect (G_OBJECT (buttona), "clicked", G_CALLBACK (toggle_method_a2), NULL);

    vboxall = gtk_vbox_new (TRUE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vboxall), 8);
    gtk_container_add (GTK_CONTAINER (vbox), vboxall);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (vboxall), sw);

    vbox2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), vbox2);

    table = gtk_table_new (2, 2, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 0);
    gtk_table_set_col_spacings (GTK_TABLE (table), 0);
    gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
    tlabel = gtk_label_new_with_mnemonic ("                                                       Your Team Name:");
    gtk_table_attach_defaults (GTK_TABLE (table), tlabel, 0, 1, 0, 1);

    tentry = gtk_entry_new ();
    gtk_signal_connect (GTK_OBJECT (tentry), "insert_text", GTK_SIGNAL_FUNC (CheckTName), NULL);
    if (strlen (&utname[0]))
        gtk_entry_set_text (GTK_ENTRY (tentry), utname);
    gtk_entry_set_max_length (GTK_ENTRY (tentry), 48);
    gtk_table_attach_defaults (GTK_TABLE (table), tentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (tlabel), tentry);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    table = gtk_table_new (2, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Formula to Determine Position Players:");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    formPPentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (formPPentry), 500);
    gtk_table_attach_defaults (GTK_TABLE (table), formPPentry, 1, 4, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), formPPentry);
    gtk_signal_connect (GTK_OBJECT (formPPentry), "insert_text", GTK_SIGNAL_FUNC (CheckPPEntryForm), NULL);
    gtk_entry_set_text (GTK_ENTRY (formPPentry), "(TB + SB + BB + HBP - CS) / (AB - H + CS + GIDP)");
    formulaPPchanged = 0;

    formvalbut = gtk_button_new_with_label ("Validate Formula");
    g_signal_connect (G_OBJECT (formvalbut), "clicked", G_CALLBACK (ValPPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formvalbut, 0, 1, 1, 2);
    formsavbut = gtk_button_new_with_label ("Save Formula");
    g_signal_connect (G_OBJECT (formsavbut), "clicked", G_CALLBACK (SavePPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formsavbut, 1, 2, 1, 2);
    formloadbut = gtk_button_new_with_label ("Load Formula");
    g_signal_connect (G_OBJECT (formloadbut), "clicked", G_CALLBACK (LoadPPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formloadbut, 2, 3, 1, 2);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    table = gtk_table_new (2, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Formula to Determine Starting Pitchers:");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    formSPentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (formSPentry), 500);
    gtk_table_attach_defaults (GTK_TABLE (table), formSPentry, 1, 4, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), formSPentry);
    gtk_signal_connect (GTK_OBJECT (formSPentry), "insert_text", GTK_SIGNAL_FUNC (CheckSPEntryForm), NULL);
    gtk_entry_set_text (GTK_ENTRY (formSPentry), "((5 * IP / 9) - ER) + (K / 12) + SHO + ((W * 6) - (L * 2))");
    formulaSPchanged = 0;

    formvalbut = gtk_button_new_with_label ("Validate Formula");
    g_signal_connect (G_OBJECT (formvalbut), "clicked", G_CALLBACK (ValSPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formvalbut, 0, 1, 1, 2);
    formsavbut = gtk_button_new_with_label ("Save Formula");
    g_signal_connect (G_OBJECT (formsavbut), "clicked", G_CALLBACK (SaveSPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formsavbut, 1, 2, 1, 2);
    formloadbut = gtk_button_new_with_label ("Load Formula");
    g_signal_connect (G_OBJECT (formloadbut), "clicked", G_CALLBACK (LoadSPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formloadbut, 2, 3, 1, 2);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    table = gtk_table_new (2, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Formula to Determine Relief Pitchers:");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    formRPentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (formRPentry), 500);
    gtk_table_attach_defaults (GTK_TABLE (table), formRPentry, 1, 4, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), formRPentry);
    gtk_signal_connect (GTK_OBJECT (formRPentry), "insert_text", GTK_SIGNAL_FUNC (CheckRPEntryForm), NULL);
    gtk_entry_set_text (GTK_ENTRY (formRPentry), "9 - ERA");
    formulaRPchanged = 0;

    formvalbut = gtk_button_new_with_label ("Validate Formula");
    g_signal_connect (G_OBJECT (formvalbut), "clicked", G_CALLBACK (ValRPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formvalbut, 0, 1, 1, 2);
    formsavbut = gtk_button_new_with_label ("Save Formula");
    g_signal_connect (G_OBJECT (formsavbut), "clicked", G_CALLBACK (SaveRPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formsavbut, 1, 2, 1, 2);
    formloadbut = gtk_button_new_with_label ("Load Formula");
    g_signal_connect (G_OBJECT (formloadbut), "clicked", G_CALLBACK (LoadRPFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formloadbut, 2, 3, 1, 2);

    formcharbut = gtk_button_new_with_label ("Valid Formula Characters");
    g_signal_connect (G_OBJECT (formcharbut), "clicked", G_CALLBACK (ValidChars), NULL);
    gtk_container_add (GTK_CONTAINER (vbox2), formcharbut);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    useSP = 0;
    checkSP = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), checkSP, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (checkSP), "clicked", G_CALLBACK (toggle_SP), NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSP), FALSE);
    gtk_button_set_label (GTK_BUTTON (checkSP), "Do Not Use the Same Player (From Different Years) More Than Once");

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    use1CL = 0;
    check1CL = gtk_toggle_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), check1CL, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (check1CL), "clicked", G_CALLBACK (toggle_1CL), NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check1CL), FALSE);
    gtk_button_set_label (GTK_BUTTON (check1CL), "Include Only 1 Closer-Type Pitcher on the Roster");

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    buttonALL = gtk_radio_button_new_with_label (NULL, "Use All Teams");
    gtk_box_pack_start (GTK_BOX (vbox2), buttonALL, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonALL), "clicked", G_CALLBACK (CBckALL), NULL);
    group2 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (buttonALL));

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    buttonTN = gtk_radio_button_new_with_label (group2, "Use Players Only From Team:  ");
    gtk_box_pack_start (GTK_BOX (hbox), buttonTN, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttonTN), "clicked", G_CALLBACK (CBckTN), NULL);

    comboTN = gtk_combo_box_new_text ();

    for (x = 1; x <= NUMBER_OF_TEAMS; x++) {
        strcpy (&stn[x][0], &teaminfo[x].teamname[0]);
        if (teaminfo[x].yrspan[0] == -1)
            strcat (&stn[x][0], ", no years");
        else {
            strcat (&stn[x][0], ", ");
            strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[0], 'l'));
            strcat (&stn[x][0], " - ");
            strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[1], 'l'));
            if (teaminfo[x].yrspan[2]) {
                strcat (&stn[x][0], ", ");
                strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[2], 'l'));
                strcat (&stn[x][0], " - ");
                strcat (&stn[x][0], cnvt_int2str (teaminfo[x].yrspan[3], 'l'));
            }
        }
    }
    /* sort team names */
    /* we're not using USER CREATED so we can use that slot for work */
    for (x = 1; x < NUMBER_OF_TEAMS; x++)
        for (y = x + 1; y < (NUMBER_OF_TEAMS + 1); y++) {
            if (strcmp (&stn[x][0], &stn[y][0]) > 0) {
                strcpy (&stn[0][0], &stn[x][0]);
                strcpy (&stn[x][0], &stn[y][0]);
                strcpy (&stn[y][0], &stn[0][0]);
            }
        }

    for (x = 1; x <= NUMBER_OF_TEAMS; x++)
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboTN), stn[x]);
    gtk_container_add (GTK_CONTAINER (hbox), comboTN);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboTN), NUMBER_OF_TEAMS / 2);

    toggle_o0 = swALL = 1;

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Certain Statistics are not always available, towit:");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Offense");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    label = gtk_label_new ("GIDP - not available 1901-38\nCS - not available 1901-19, 1926-50\nIBB - not available 1901-54\nSF - not available 1901-53\nK - not available 1901-12");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Pitching");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    label = gtk_label_new ("IBB - not available 1901-54\nSOPP, 2B, 3B, RBI, SB, CS, SH, SF & OPAB -\n         not available 1901-97, 1999-2017");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    button0 = gtk_radio_button_new_with_label (NULL, "Substitute 0 for Unavailable Stats");
    gtk_box_pack_start (GTK_BOX (vbox2), button0, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (button0), "clicked", G_CALLBACK (toggle_method_02), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button0));
    buttono = gtk_radio_button_new_with_label (group, "Omit Years Where Stats Are Not Available");
    gtk_box_pack_start (GTK_BOX (vbox2), buttono, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttono), "clicked", G_CALLBACK (toggle_method_o2), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Sometimes, with certain formulas, a \"divide by zero\" circumstance is encountered.");
    gtk_container_add (GTK_CONTAINER (vbox), label);
    label = gtk_label_new ("In those instances 0.00001 will be substituted for 0.");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Minimum Player Requirements to Qualify");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    table = gtk_table_new (4, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (vbox2), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("For Position Players (Plate Appearances):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    label = gtk_label_new_with_mnemonic ("For Starting Pitchers (Innings Pitched):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    label = gtk_label_new_with_mnemonic ("For Relief Pitchers (Games):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

    offminentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (offminentry), 6);
    gtk_table_attach_defaults (GTK_TABLE (table), offminentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), offminentry);
    gtk_entry_set_text (GTK_ENTRY (offminentry), "3.1");
    label = gtk_label_new_with_mnemonic ("X number of games player's team played");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
    gtk_signal_connect (GTK_OBJECT (offminentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryMIN), NULL);

    gtk_entry_set_activates_default (GTK_ENTRY (offminentry), TRUE);

    spitminentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (spitminentry), 6);
    gtk_table_attach_defaults (GTK_TABLE (table), spitminentry, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), spitminentry);
    gtk_entry_set_text (GTK_ENTRY (spitminentry), "1.0");
    label = gtk_label_new_with_mnemonic ("X number of games player's team played");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);
    gtk_signal_connect (GTK_OBJECT (spitminentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryMIN), NULL);

    rpitminentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (rpitminentry), 6);
    gtk_table_attach_defaults (GTK_TABLE (table), rpitminentry, 1, 2, 2, 3);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), rpitminentry);
    gtk_entry_set_text (GTK_ENTRY (rpitminentry), "0.25");
    label = gtk_label_new_with_mnemonic ("X number of games player's team played");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 2, 3);
    gtk_signal_connect (GTK_OBJECT (rpitminentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryMIN), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (vbox2), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Exclude Year(s):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    label = gtk_label_new_with_mnemonic ("Include Year(s):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

    exentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (exentry), 100);
    gtk_table_attach_defaults (GTK_TABLE (table), exentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), exentry);
    gtk_entry_set_text (GTK_ENTRY (exentry), "");
    gtk_signal_connect (GTK_OBJECT (exentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryBS), NULL);

    gtk_entry_set_activates_default (GTK_ENTRY (exentry), TRUE);

    inentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (inentry), 100);
    gtk_table_attach_defaults (GTK_TABLE (table), inentry, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), inentry);
    gtk_entry_set_text (GTK_ENTRY (inentry), "2017");
    gtk_signal_connect (GTK_OBJECT (inentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryBS), NULL);
    gtk_window_set_focus (GTK_WINDOW (dlgFile), inentry);

    gtk_entry_set_activates_default (GTK_ENTRY (inentry), TRUE);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, TRUE, 0);

    disbutton = gtk_button_new_with_label ("Cancel/Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroyAC), dlgFile);
    okbutton = gtk_button_new_with_label ("Go");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (CTeam), dlgFile);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlgFile)->action_area), disbutton);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlgFile)->action_area), okbutton);

    toggle_ha = 0;
    gtk_widget_show_all (dlgFile);
    gtk_widget_hide (vboxall);
    gtk_window_resize (GTK_WINDOW (dlgFile), 1, 1);
}

void
CTeam (GtkWidget *widget, gpointer *pdata) {
    gchar SuccessTeam[256] = "Team successfully created.", *msg[5], work[5000],
          Dup[256] = "The Team Name already exists.  Do you wish to overwrite the team?\n\n";
    gint x, y, z, err, decpt, terr, fatalerr;
    gchar NoYrs[256] = "No years to search.", holdt[100], NoTName[256] = "You need to enter a name for your team.\n\n",
          NoYears[256] = "The Real Life team from which you are creating your team has \"no years\" available.",
          NoInYears[256] = "\nThe years to include is empty.  Assume all years (1901-2017)?\n\n",
          FourPos[256] = "The team name cannot be exactly 4 positions long.  Crazy, huh?\n\n",
          TwoDP[256] = "There can't be more than one decimal point in the minimum requirements.\n", min[3][256],
          InvPPForm[256] = "The Position Player Formula is in error.", InvSPForm[256] = "The Starting Pitcher Formula is in error.",
          InvRPForm[256] = "The Relief Pitcher Formula is in error.", BlankPPForm[256] = "The Position Player Formula is empty.",
          BlankSPForm[256] = "The Starting Pitcher Formula is empty.", BlankRPForm[256] = "The Relief Pitcher Formula is empty.";

    if (!toggle_ha) {
        gtk_widget_destroy (dlgFile);
        CreateTeam ();
        getACactive = 0;
        return;
    }

    fatalerr = terr = err = 0;
    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    inentry_text = gtk_entry_get_text (GTK_ENTRY (inentry));

    if (!strlen (&inentry_text[0]) && !preferences.AssumeAllYears) {
        msg[0] = &NoInYears[0];
        if (!ShallWeContinue (msg))
            return;
    }

    exentry_text = gtk_entry_get_text (GTK_ENTRY (exentry));

    entry_text = gtk_entry_get_text (GTK_ENTRY (tentry));
    strcpy (&utname[0], entry_text);

    if (!strlen (&utname[0])) {
        msg[fatalerr] = NoTName;
        fatalerr++;
        terr = 1;
    }
    if (strlen (&utname[0]) == 4) {
        msg[fatalerr] = FourPos;
        fatalerr++;
        terr = 1;
    }

    if (fatalerr) {
        fatalerr = 0;
        outMessage (msg);
    }

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formPPentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formPPentry));
    if (!strlen (&formPPentry_text[0])) {
        msg[0] = &BlankPPForm[0];
        outMessage (msg);
        terr = 1;
    }
    else
        if (ValidateFormula (&formPPentry_text[0], 1, 1)) {
            msg[0] = &InvPPForm[0];
            outMessage (msg);
            terr = 1;
        }

    formSPentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formSPentry));
    if (!strlen (&formSPentry_text[0])) {
        msg[0] = &BlankSPForm[0];
        outMessage (msg);
        terr = 1;
    }
    else
        if (ValidateFormula (&formSPentry_text[0], 1, 2)) {
            msg[0] = &InvSPForm[0];
            outMessage (msg);
            terr = 1;
        }

    formRPentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formRPentry));
    if (!strlen (&formRPentry_text[0])) {
        msg[0] = &BlankRPForm[0];
        outMessage (msg);
        terr = 1;
    }
    else
        if (ValidateFormula (&formRPentry_text[0], 1, 3)) {
            msg[0] = &InvRPForm[0];
            outMessage (msg);
            terr = 1;
        }

    err = ValidateText3 ();
    if (err) {
        gint errct;
        gchar ExMsg[256] = "In excluded years ...", InMsg[256] = "In included years ...",
              YrAfterHy[256] = "The year after a hyphen must be equal to or greater than the year before that same hyphen.",
              NoYr[256] = "There must be a year after a hyphen.", InitYr[256] = "A year must be the first data.",
              InvYr[256] = "A year is not valid (must be 4 positions and must be 1901-2017).";

        /* first four bits (leftmost) inentry, second four bits (rightmost) exentry
           0000 0000
              |    |
              InvYr
             |    |
             InitYr
            |    |
            NoYr
           |    |
           YrAfterHy
        */

        terr = 1;
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (err >= 16) {
            msg[0] = &InMsg[0];
            errct = 1;
            if ((err & 0x80) == 0x80) {
                msg[errct] = &YrAfterHy[0];
                errct++;
            }
            if ((err & 0x40) == 0x40) {
                msg[errct] = &NoYr[0];
                errct++;
            }
            if ((err & 0x20) == 0x20) {
                msg[errct] = &InitYr[0];
                errct++;
            }
            if ((err & 0x10) == 0x10)
                msg[errct] = &InvYr[0];

            outMessage (msg);
        }

        err &= 15;
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (err) {
            msg[0] = &ExMsg[0];
            errct = 1;
            if ((err & 0x8) == 0x8) {
                msg[errct] = &YrAfterHy[0];
                errct++;
            }
            if ((err & 0x4) == 0x4) {
                msg[errct] = &NoYr[0];
                errct++;
            }
            if ((err & 0x2) == 0x2) {
                msg[errct] = &InitYr[0];
                errct++;
            }
            if ((err & 0x1) == 0x1)
                msg[errct] = &InvYr[0];

            outMessage (msg);
        }
    }

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    for (y = x = 0; x < YEAR_SPREAD; x++)
        if (yrs[x])
            if (++y > 1)
                break;
    if (x == YEAR_SPREAD && !y) {
        terr = 1;
        msg[0] = &NoYrs[0];
        outMessage (msg);
    }

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    strcpy (&work[0], "e");
    work[1] = toggle_o0 + '0';
    work[2] = useSP + '0';
    work[3] = use1CL + '0';
    work[4] = '\0';
    strcat (&work[0], &utname[0]);
    strcat (&work[0], ",");
    strcat (&work[0], formPPentry_text);
    strcat (&work[0], ",");
    strcat (&work[0], formSPentry_text);
    strcat (&work[0], ",");
    strcat (&work[0], formRPentry_text);
    strcat (&work[0], ",");

    strcpy (&min[0][0], (gchar *) gtk_entry_get_text (GTK_ENTRY (offminentry)));
    strcpy (&min[1][0], (gchar *) gtk_entry_get_text (GTK_ENTRY (spitminentry)));
    strcpy (&min[2][0], (gchar *) gtk_entry_get_text (GTK_ENTRY (rpitminentry)));
    if (!strlen (&min[0][0]))
        strcpy (&min[0][0], "0.0");
    if (!strlen (&min[1][0]))
        strcpy (&min[1][0], "0.0");
    if (!strlen (&min[2][0]))
        strcpy (&min[2][0], "0.0");

    for (z = 0; z < 3; z++) {
        for (decpt = x = 0; x < strlen (&min[z][0]); x++)
            if (min[z][x] == '.')
                decpt++;
        if (decpt > 1) {
            msg[0] = &TwoDP[0];
            if (z == 0)
                msg[1] = "(Position Players)";
            else
                if (z == 1)
                    msg[1] = "(Starting Pitchers)";
                else
                    msg[1] = "(Relief Pitchers)";
            outMessage (msg);
            terr = 1;
        }
    }
    if (terr)
        return;

    for (z = 0; z < 3; z++) {
        strcat (&work[0], &min[z][0]);
        strcat (&work[0], " ");
    }

    if (!swALL) {
        gint tyrs[YEAR_SPREAD] /* 1901-MAX-YEAR */, y, tpnt;
        gchar *comma, tholdtn[100], mwork[256];

        strcpy (&holdt[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboTN)));

        /* did user select a "no years" team? */
        comma = index (&holdt[0], ',');
        *comma = '\0';
        strcpy (&tholdtn[0], &holdt[0]);
        *comma = ',';

        for (tpnt = 0; tpnt <= NUMBER_OF_TEAMS; tpnt++)
            if (!strcmp (&teaminfo[tpnt].teamname[0], &tholdtn[0])) {
                if (teaminfo[tpnt].yrspan[0] == -1) {
                    terr = 1;
                    msg[0] = &NoYears[0];
                    outMessage (msg);
                    return;
                }
                break;
            }

        /* ensure year range in "Include Year(s)" falls within years team is available */
        for (y = 0; y < YEAR_SPREAD; y++)
            tyrs[y] = 0;

        for (y = teaminfo[tpnt].yrspan[0]; y <= teaminfo[tpnt].yrspan[1]; y++)
            tyrs[y - 1901] = 1;     /* include year in search */

        if (teaminfo[tpnt].yrspan[2])
            for (y = teaminfo[tpnt].yrspan[2]; y <= teaminfo[tpnt].yrspan[3]; y++)
                tyrs[y - 1901] = 1;     /* include year in search */

        for (y = 0; y < YEAR_SPREAD; y++)
            if (yrs[y] && !tyrs[y])
                yrs[y] = 0; 

        for (y = 0; y < YEAR_SPREAD; y++)
            if (yrs[y])
                break;

        if (y == YEAR_SPREAD) {
            /* no years to use */
            strcpy (&mwork[0], "\"Include Year(s)\" out of range from years ");
            strcat (&mwork[0], &tholdtn[0]);
            strcat (&mwork[0], " is available (");

            strcat (&mwork[0], cnvt_int2str (teaminfo[tpnt].yrspan[0], 'l'));
            strcat (&mwork[0], "-");
            strcat (&mwork[0], cnvt_int2str (teaminfo[tpnt].yrspan[1], 'l'));
            if (teaminfo[tpnt].yrspan[2]) {
                strcat (&mwork[0], ", ");
                strcat (&mwork[0], cnvt_int2str (teaminfo[tpnt].yrspan[2], 'l'));
                strcat (&mwork[0], "-");
                strcat (&mwork[0], cnvt_int2str (teaminfo[tpnt].yrspan[3], 'l'));
            }
            strcat (&mwork[0], ").  To use all available years for this team leave \"Include Year(s)\" empty.\n\n");

            terr = 1;
            msg[0] = &mwork[0];
            outMessage (msg);
            return;
        }

        /* squeeze out spaces */
        for (x = 0; x < strlen (&holdt[0]); )    /* increment x within loop */
            if (holdt[x] == ' ')
                for (y = x; y < strlen (&holdt[0]); y++)
                    holdt[y] = holdt[y + 1];
            else
                x++;
        comma = index (&holdt[0], ',');
        *comma = '\0';
        strcat (&work[0], &holdt[0]);
    }
    else
        strcat (&work[0], "ALLT");
    strcat (&work[0], " ");

    for (x = 0; x < YEAR_SPREAD; x++)
        strcat (&work[0], cnvt_int2str (yrs[x], 'l'));

    strcat (&work[0], "\n");
    sock_puts (sock, work);
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }

    if (!strncmp (&buffer[0], "ERR", 3)) {
        gint x;
        gchar *msg[5], FileRW[256] = "There is a file read/write problem.",
              ProblemSP[256] = "There is a problem with the Starting Pitcher formula.\n\n",
              ProblemRP[256] = "There is a problem with the Relief Pitcher formula.\n\n",
              ProblemPP[256] = "There is a problem with the Position Player formula.\n\n",
              PosInForm[256] = "\n\nAt or near position ",
              NoCloser[256] = "No closer could be selected.\n",
              NoSP[256] = "Not enough Starting Pitchers could be selected.\n",
              NoRP[256] = "Not enough Relief Pitchers could be selected.\n",
              NoPP[256] = "Not enough Position Players could be selected.\n",
              Unk[256] = "The problem is unknown.", Stat[256] = "The problem is possibly with a Stat Acronym.",
              Num[256] = "The problem is possibly with a number.", Op[256] = "The problem is possibly with an operator (+ - * /).",
              Paren[256] = "The problem is possibly with a parenthesis.",
              NDR2[256] = "Examine the Relief Pitcher formula.\n", ND3[256] = "Also, try loosening the minimum qualifications.\n",
              NDS2[256] = "Examine the Starting Pitcher formula.\n",
              NDP2[256] = "Examine the Position Player formula.\n",
              ND4[256] = "If you are not using all teams to create your team check which team you're using.\n",
              ND5[256] = "Check which years you're including.\n",
              Poss1[256] = "There are certain requirements associated with auto-creating a team.\n",
              Poss2[256] = "You may have to create your team by hand to get what you want.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (strlen (&buffer[0]) == 3)
            msg[0] = FileRW;

        if (!strncmp (&buffer[0], "ERRFORMC", 8)) {
            msg[0] = NoCloser;
            msg[1] = NDR2;
            msg[2] = ND3;
            msg[3] = ND4;
            msg[4] = ND5;
        }

        if (!strncmp (&buffer[0], "ERRFORMS", 8)) {
            msg[0] = NoSP;
            msg[1] = NDS2;
            msg[2] = ND3;
            msg[3] = ND4;
            msg[4] = ND5;
        }

        if (!strncmp (&buffer[0], "ERRFORMR", 8)) {
            msg[0] = NoRP;
            msg[1] = NDR2;
            msg[2] = ND3;
            msg[3] = ND4;
            msg[4] = ND5;
        }

        if (!strncmp (&buffer[0], "ERRFORMP", 8)) {
            msg[0] = NoPP;
            msg[1] = NDP2;
            msg[2] = ND3;
            msg[3] = ND4;
            msg[4] = ND5;
        }

        if (!strncmp (&buffer[0], "ERROR", 5))
            for (x = 5; x < 15; x += 5) {
                 if (buffer[x] != '0') {
                      if (x == 5)
                          msg[0] = ProblemSP;
                      else
                          if (x == 10)
                              msg[0] = ProblemRP;
                          else
                              msg[0] = ProblemPP;
                      for (y = 1; y < 4; y++)
                          if (buffer[x + y] == '0')
                              buffer[x + y] = ' ';
                          else
                              break;

                     strncat (&PosInForm[0], &buffer[6], 4);
                     strcat (&PosInForm[0], ".");
                     msg[2] = PosInForm;
                 }

                 if (buffer[x] == '1')
                      msg[1] = Paren;
                 if (buffer[x] == '2')
                      msg[1] = Op;
                 if (buffer[x] == '3')
                      msg[1] = Num;
                 if (buffer[x] == '4')
                      msg[1] = Stat;
                 if (buffer[x] == '9')
                      msg[1] = Unk;
            }

        outMessage (msg);

        for (x = 0; x < 5; x++)
            msg[x] = NULL;
        msg[0] = Poss1;
        msg[1] = Poss2;
        outMessage (msg);

        return;
    }

    if (!strncmp (&buffer[0], "DUP", 3)) {
        /* this happens if the team name already exists on the server */
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &Dup[0];
        if (!ShallWeContinue (msg)) {
            sock_puts (sock, "CANCEL\n");
            return;
        }
        sock_puts (sock, "OK\n");
        sock_gets (sock, &buffer[0], sizeof (buffer));
    }

    if (!strncmp (&buffer[0], "NODATA", 6)) {
        gint x;
        gchar *msg[5], ND1[256] = "There is no data meeting the formula criterias.\n", ND2[256] = "Examine the formulas.\n",
              ND3[256] = "Also, try loosening the minimum qualifications.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = ND1;
        msg[1] = ND2;
        msg[2] = ND3;

        outMessage (msg);
        return;
    }

    strcpy (&work[0], "Team successfully created.\n");
    Add2TextWindow (&work[0], 0);
    msg[0] = &SuccessTeam[0];
    outMessage (msg);
}

void
toggle_SP (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkSP))) {
        useSP = 1;
        gtk_button_set_label (GTK_BUTTON (checkSP), "Okay to Use the Same Player (From Different Years) Multiple Times");
    }
    else {
        useSP = 0;
        gtk_button_set_label (GTK_BUTTON (checkSP), "Do Not Use the Same Player (From Different Years) More Than Once");
    }
}

void
toggle_1CL (GtkWidget *widget, gpointer *pdata) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check1CL))) {
        gtk_button_set_label (GTK_BUTTON (check1CL), "Okay to Have Multiple Closer-Type Pitchers on the Roster");
        use1CL = 1;
    }
    else {
        use1CL = 0;
        gtk_button_set_label (GTK_BUTTON (check1CL), "Include Only 1 Closer-Type Pitcher on the Roster");
    }
}

void
toggle_method_h2 (GtkWidget *widget, gpointer *pdata) {
    toggle_ha = 0;
    gtk_widget_hide (vboxall);
    gtk_window_resize (GTK_WINDOW (dlgFile), 1, 1);
}

void
toggle_method_a2 (GtkWidget *widget, gpointer *pdata) {
    toggle_ha = 1;
    gtk_window_resize (GTK_WINDOW (dlgFile), 780, 2000);
    gtk_widget_show (vboxall);
}

void
CBckTN (GtkWidget *widget, gpointer pdata) {
    swALL = 0;
}

void
CBckALL (GtkWidget *widget, gpointer pdata) {
    swALL = 1;
}

int
ValidateText3 () {
    gint x, z, yrpnt, pos, errors, yr1, yr2;
    gchar yrdata[2][1000], work[10];

    errors = 0;

    if (!strlen (&inentry_text[0])) {
        /* using all years */
        for (x = 0; x < YEAR_SPREAD; x++)
            yrs[x] = 1;

        if (!strlen (&exentry_text[0]))
            /* done ... input all years */
            return 0;

        yrpnt = 1;
    }
    else
        yrpnt = 0;

    strcpy (&yrdata[0][0], inentry_text);
    strcpy (&yrdata[1][0], exentry_text);

    for (pos = 0; yrpnt < 2; yrpnt++, pos = 0) {
nxt_group:
        if (pos >= strlen (&yrdata[yrpnt][0]))
            continue;
        /* ignore leading spaces */
        for ( ; pos < strlen (&yrdata[yrpnt][0]); pos++)
            if (yrdata[yrpnt][pos] != ' ')
                break;
        /* the first data must be a year */
        if (yrdata[yrpnt][pos] == '-') {
            if (yrpnt == 0)
                errors |= 0x20;
            else
                errors |= 0x02;
            continue;                                                /* don't do the other checks on this data in this case */
        }

        /* got a year ... maybe */
        for (x = 0; x < 5 && pos < strlen (&yrdata[yrpnt][0]); x++, pos++)
            if (yrdata[yrpnt][pos] == ' ' || yrdata[yrpnt][pos] == '-')
                break;
            else
                work[x] = yrdata[yrpnt][pos];
        /* must be a 4-position year */
        if (x != 4) {
            if (yrpnt == 0)
                errors |= 0x10;
            else
                errors |= 0x01;
            continue;                                                /* don't do the other checks on this data in this case */
        }
        work[4] = '\0';
        yr1 = atoi (&work[0]);
        if (yr1 < 1901 || yr1 > MAX_YEAR) {
            if (yrpnt == 0)
                errors |= 0x10;
            else
                errors |= 0x01;
        }
        if (yrdata[yrpnt][pos] == ' ')
            for ( ; pos < strlen (&yrdata[yrpnt][0]); pos++)
                if (yrdata[yrpnt][pos] == '-')
                    break;
                else
                    if (yrdata[yrpnt][pos] == ' ')
                        continue;
                    else {
                        /* got another year */
                        if (!errors) {
                            if (yrpnt == 0)
                                yrs[yr1 - 1901] = 1;     /* include year in search */
                            else
                                yrs[yr1 - 1901] = 0;     /* exclude year from search */
                        }
                        goto nxt_group;
                    }
        if (pos >= strlen (&yrdata[yrpnt][0])) {
            /* got another year */
            if (!errors) {
                if (yrpnt == 0)
                    yrs[yr1 - 1901] = 1;     /* include year in search */
                else
                    yrs[yr1 - 1901] = 0;     /* exclude year from search */
            }
            goto nxt_group;
        }

        /* need to get a second year */
        if (yrdata[yrpnt][pos] == '-') {
            /* ignore spaces */
            for (pos++; pos < strlen (&yrdata[yrpnt][0]); pos++)
                if (yrdata[yrpnt][pos] != ' ')
                    break;
            /* the first data encountered must be a year */
            if (yrdata[yrpnt][pos] == '-') {
                if (yrpnt == 0)
                    errors |= 0x20;
                else
                    errors |= 0x02;
                continue;                                                /* don't do the other checks on this data in this case */
            }

            /* got a year ... maybe */
            for (x = 0; x < 5 && pos < strlen (&yrdata[yrpnt][0]); x++, pos++)
                if (yrdata[yrpnt][pos] == ' ' || yrdata[yrpnt][pos] == '-')
                    break;
                else
                    work[x] = yrdata[yrpnt][pos];
            /* must be a 4-position year */
            if (x != 4) {
                if (yrpnt == 0)
                    errors |= 0x10;
                else
                    errors |= 0x01;
                continue;                                                /* don't do the other checks on this data in this case */
            }
            work[4] = '\0';
            yr2 = atoi (&work[0]);
            if (yr2 < 1901 || yr2 > MAX_YEAR) {
                if (yrpnt == 0)
                    errors |= 0x10;
                else
                    errors |= 0x01;
            }
            if (yr2 < yr1) {
                if (yrpnt == 0)
                    errors |= 0x80;
                else
                    errors |= 0x08;
                for ( ; pos < strlen (&yrdata[yrpnt][0]) && yrdata[yrpnt][pos] != ' '; pos++);
                goto nxt_group;
            }

            if (!errors)
                for (z = yr1; z <= yr2; z++)
                    if (yrpnt == 0)
                        yrs[z - 1901] = 1;     /* include year in search */
                    else
                        yrs[z - 1901] = 0;     /* exclude year from search */
            pos++;
            goto nxt_group;
        }
    }

    return errors;
}

void
toggle_method_o2 (GtkWidget *widget, gpointer *pdata) {
    toggle_o0 = 0;
}

void
toggle_method_02 (GtkWidget *widget, gpointer *pdata) {
    toggle_o0 = 1;
}

void
CheckPPEntryForm (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    formulaPPchanged = 1;
}

void
CheckSPEntryForm (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    formulaSPchanged = 1;
}

void
CheckRPEntryForm (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    formulaRPchanged = 1;
}

void
LoadPPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    FILE *iform;

    if (formulaPPchanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved Player Position formula.  Continuing will lose any changes.  Continue anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }

    dialog = gtk_file_chooser_dialog_new ("Load Player Position Formula", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *msg[5], *filename;
        gint x, err = 0;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if ((iform = fopen (filename, "r")) != NULL) {
            x = fread (&ldformula, sizeof ldformula, 1, iform);
            if (ferror (iform)) {
                /* error reading file */
                gchar ERR[256] = "Error reading file.";

                err = -1;
                msg[0] = ERR;
                outMessage (msg);
            }
            fclose (iform);
        }
        else {
            /* error opening file */
            gchar ERR[256] = "Error opening file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (strncmp (&ldformula[0], "NSBFormula", 10)) {
            /* not an NSB formula file */
            gchar ERR[256] = "Not an NSB formula file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (!err) {
            gtk_entry_set_text (GTK_ENTRY (formPPentry), ldformula + 10);
            strcpy (&ifilenamePP[0], &filename[0]);
            loadPPformulasw = 1;
            formulaPPchanged = 0;
        }
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void
LoadSPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    FILE *iform;

    if (formulaSPchanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved Starting Pitcher formula.  Continuing will lose any changes.  Continue anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }

    dialog = gtk_file_chooser_dialog_new ("Load Starting Pitcher Formula", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *msg[5], *filename;
        gint x, err = 0;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if ((iform = fopen (filename, "r")) != NULL) {
            x = fread (&ldformula, sizeof ldformula, 1, iform);
            if (ferror (iform)) {
                /* error reading file */
                gchar ERR[256] = "Error reading file.";

                err = -1;
                msg[0] = ERR;
                outMessage (msg);
            }
            fclose (iform);
        }
        else {
            /* error opening file */
            gchar ERR[256] = "Error opening file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (strncmp (&ldformula[0], "NSBFormula", 10)) {
            /* not an NSB formula file */
            gchar ERR[256] = "Not an NSB formula file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (!err) {
            gtk_entry_set_text (GTK_ENTRY (formSPentry), ldformula + 10);
            strcpy (&ifilenameSP[0], &filename[0]);
            loadSPformulasw = 1;
            formulaSPchanged = 0;
        }
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void
LoadRPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    FILE *iform;

    if (formulaRPchanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved Relief Pitcher formula.  Continuing will lose any changes.  Continue anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }

    dialog = gtk_file_chooser_dialog_new ("Load Relief Pitcher Formula", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                               GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *msg[5], *filename;
        gint x, err = 0;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if ((iform = fopen (filename, "r")) != NULL) {
            x = fread (&ldformula, sizeof ldformula, 1, iform);
            if (ferror (iform)) {
                /* error reading file */
                gchar ERR[256] = "Error reading file.";

                err = -1;
                msg[0] = ERR;
                outMessage (msg);
            }
            fclose (iform);
        }
        else {
            /* error opening file */
            gchar ERR[256] = "Error opening file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (strncmp (&ldformula[0], "NSBFormula", 10)) {
            /* not an NSB formula file */
            gchar ERR[256] = "Not an NSB formula file.";

            err = -1;
            msg[0] = ERR;
            outMessage (msg);
        }

        if (!err) {
            gtk_entry_set_text (GTK_ENTRY (formRPentry), ldformula + 10);
            strcpy (&ifilenameRP[0], &filename[0]);
            loadRPformulasw = 1;
            formulaRPchanged = 0;
        }
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void
SavePPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    gint x;
    gchar *msg[5], Nothing[256] = "There is Nothing to Save for Position Players.", InvForm[256] = "The Position Player formula is not valid.  Save anyway?";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formPPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &Nothing[0];
        outMessage (msg);
        formulaPPchanged = 0;
        return;
    }
    else
        if (ValidateFormula (&formentry_text[0], 2, 1)) {
            msg[0] = &InvForm[0];
            if (!ShallWeContinue (msg))
                return;
        }

    dialog = gtk_file_chooser_dialog_new ("Save Position Player Formula", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                                GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
    if (!loadPPformulasw) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled formula");
    }
    else
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), ifilenamePP);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename, oformula[4096], *msg[5], *ERR = "File cannot be written.";
        FILE *oform;
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        /* save the NSB formula */
        if ((oform = fopen (filename, "w")) != NULL) {
            strcpy (&oformula[0], "NSBFormula");
            strcat (&oformula[0], formentry_text);
            fwrite (&oformula, sizeof oformula, 1, oform);
            formulaPPchanged = 0;
            fclose (oform);
        }
        else {
            msg[0] = ERR;
            outMessage (msg);
        }

        g_free (filename);
    }
    gtk_widget_destroy (dialog);

}

void
SaveSPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    gint x;
    gchar *msg[5], Nothing[256] = "There is Nothing to Save for Starting Pitchers.",
          InvForm[256] = "The Starting Pitcher formula is not valid.  Save anyway?";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formSPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &Nothing[0];
        outMessage (msg);
        formulaSPchanged = 0;
        return;
    }
    else
        if (ValidateFormula (&formentry_text[0], 2, 2)) {
            msg[0] = &InvForm[0];
            if (!ShallWeContinue (msg))
                return;
        }

    dialog = gtk_file_chooser_dialog_new ("Save Starting Pitcher Formula", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                                 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
    if (!loadSPformulasw) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled formula");
    }
    else
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), ifilenameSP);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename, oformula[4096], *msg[5], *ERR = "File cannot be written.";
        FILE *oform;
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        /* save the NSB formula */
        if ((oform = fopen (filename, "w")) != NULL) {
            strcpy (&oformula[0], "NSBFormula");
            strcat (&oformula[0], formentry_text);
            fwrite (&oformula, sizeof oformula, 1, oform);
            formulaSPchanged = 0;
            fclose (oform);
        }
        else {
            msg[0] = ERR;
            outMessage (msg);
        }

        g_free (filename);
    }
    gtk_widget_destroy (dialog);

}

void
SaveRPFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    gint x;
    gchar *msg[5], Nothing[256] = "There is Nothing to Save for Relief Pitcher.", InvForm[256] = "The Relief Pitcher formula is not valid.  Save anyway?";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formRPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &Nothing[0];
        outMessage (msg);
        formulaRPchanged = 0;
        return;
    }
    else
        if (ValidateFormula (&formentry_text[0], 2, 3)) {
            msg[0] = &InvForm[0];
            if (!ShallWeContinue (msg))
                return;
        }

    dialog = gtk_file_chooser_dialog_new ("Save Relief Pitcher Formula", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                               GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
    if (!loadRPformulasw) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled formula");
    }
    else
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), ifilenameRP);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename, oformula[4096], *msg[5], *ERR = "File cannot be written.";
        FILE *oform;
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        /* save the NSB formula */
        if ((oform = fopen (filename, "w")) != NULL) {
            strcpy (&oformula[0], "NSBFormula");
            strcat (&oformula[0], formentry_text);
            fwrite (&oformula, sizeof oformula, 1, oform);
            formulaRPchanged = 0;
            fclose (oform);
        }
        else {
            msg[0] = ERR;
            outMessage (msg);
        }

        g_free (filename);
    }
    gtk_widget_destroy (dialog);

}

void
ValPPFormula (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar *msg[5], NoFormula[256] = "No Position Player Formula to Save.", *formentry_text;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formPPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &NoFormula[0];
        outMessage (msg);
        formulaPPchanged = 0;
    }
    else
        ValidateFormula (&formentry_text[0], 0, 1);
}

void
ValSPFormula (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar *msg[5], NoFormula[256] = "No Starting Pitcher Formula to Save.", *formentry_text;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formSPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &NoFormula[0];
        outMessage (msg);
        formulaSPchanged = 0;
    }
    else
        ValidateFormula (&formentry_text[0], 0, 2);
}

void
ValRPFormula (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar *msg[5], NoFormula[256] = "No Relief Pitcher Formula to Save.", *formentry_text;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formRPentry));

    if (!strlen (&formentry_text[0])) {
        msg[0] = &NoFormula[0];
        outMessage (msg);
        formulaRPchanged = 0;
    }
    else
        ValidateFormula (&formentry_text[0], 0, 3);
}

void
DestroyAC (GtkWidget *widget, gpointer *pdata) {
    if (formulaPPchanged || formulaSPchanged || formulaRPchanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved formula.  Exiting will lose any changes.  Exit anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }
    getACactive = 0;
    DestroyDialog (dlgFile, dlgFile);
}

