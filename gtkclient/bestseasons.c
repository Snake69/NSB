/* find best real life player seasons for a given time period */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

#define STACKSIZE 20
#define CALC_FAIL -2
#define CALC_SUCCESS -1
        
GtkWidget *dlgBS, *exentry, *inentry, *formentry, *spinner, *offminentry, *pitminentry, *combotype;
gint stack, offind, pitind, loadformulasw = 0, LAsw, formulachanged, yrs[YEAR_SPREAD] /* 1901 - MAX_YEAR */ ;
gchar *inentry_text, *exentry_text, *formentry_text, *minentry_text, ifilename[512], ldformula[4096];
gint SOPPsw, OPBAsw, OPABsw, GIDPsw, OBAsw, BFPsw, ERAsw, PCTsw, SHOsw, ABsw, TBsw, BAsw, SAsw, POsw, PBsw, FAsw, CGsw, ERsw, GFsw, GSsw, IPsw, WPsw,
     Asw, Esw, Bsw, Wsw, Lsw, Ssw, toggle_o0, toggle_0, bypts;

typedef enum CALC_SYMBOLS_TAG {
    OPEN,
    CLOSE,
    NUMBER,
    STAT,
    MULTIPLY,
    DIVIDE,
    ADD,
    SUBTRACT,
    INVALID
} CALC_SYMBOLS;
          
struct level {
    CALC_SYMBOLS sym;
} top[STACKSIZE];

int
push (struct level *p1) {
    if (stack < STACKSIZE) {
        top[stack].sym = p1->sym;
        stack++;
        return 1;
    }   
    return 0;
}   

int
pop (struct level *p1) {
    if (stack > 0) {
        stack--;
        p1->sym = top[stack].sym;
        return 1;
    }
    return 0;
}

int
peek (struct level *p1) {
    if (stack > 0) {
        p1->sym = top[stack - 1].sym;
        return 1;
    }
    return 0;
}

int
calc_number (int num) {
    struct level temp;
    CALC_SYMBOLS precede;

    if (stack < 1) {
        temp.sym = NUMBER;
        push (&temp);
        return CALC_SUCCESS;
    }

    peek (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            switch (precede) {
                case MULTIPLY:
                    break;
                case DIVIDE:
                    if (num == 0)
                        return CALC_FAIL;
                    break;
                case ADD:
                    break;
                case SUBTRACT:
                    break;
                default:
                    return CALC_FAIL;
                    break;
            }
            temp.sym = NUMBER;
            if (push (&temp) != 1)
                return CALC_FAIL;
            break;
        case OPEN:
            temp.sym = NUMBER;
            if (push (&temp) != 1)
                return CALC_FAIL;
            break;
        case CLOSE:
        case STAT:
        case NUMBER:
            return CALC_FAIL;
            break;
        default:
            return CALC_FAIL;
            break;
    }
    return CALC_SUCCESS;
}

int
calc_paren (CALC_SYMBOLS sym) {
    struct level temp;

    switch (sym) {
        case OPEN:
            temp.sym = sym;
            if (push (&temp) != 1)
                return CALC_FAIL;
            break;
        case CLOSE:
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return CALC_FAIL;
            if (pop (&temp) != 1)
                return CALC_FAIL;
            if (temp.sym != OPEN)
                return CALC_FAIL;
            /* treat the number or Stat Acronym as if it is new */
            if (calc_number (1) != CALC_SUCCESS)
                return CALC_FAIL;
            break;
        default:
            return CALC_FAIL;
            break;
    }
    return CALC_SUCCESS;
}

int
calc_binary_op (CALC_SYMBOLS sym) {
    struct level temp;

    if (stack < 1)
        return CALC_FAIL;

    peek (&temp);
    if (temp.sym != NUMBER && temp.sym != STAT)
        return CALC_FAIL;

    temp.sym = sym;
    if (push (&temp) != 1)
        return CALC_FAIL;

    return CALC_SUCCESS;
}

int
check4stat (gchar *exp, int ind) {
    int err;
    struct level temp;
    CALC_SYMBOLS precede;

    err = -1;

    if (!strncasecmp (&exp[ind], "LA", 2)) {
        LAsw++;
        ind += 2;
    }

    if (!strncasecmp (&exp[ind], "SOPP", 4) || !strncasecmp (&exp[ind], "OPAB", 4) || !strncasecmp (&exp[ind], "OPBA", 4) ||
                                                                                         !strncasecmp (&exp[ind], "GIDP", 4)) {
        if (!strncasecmp (&exp[ind], "SOPP", 4))
            pitind = SOPPsw = 1; 
        if (!strncasecmp (&exp[ind], "OPBA", 4))
            pitind = OPBAsw = 1;
        if (!strncasecmp (&exp[ind], "OPAB", 4))
            pitind = OPABsw = 1;
        if (!strncasecmp (&exp[ind], "GIDP", 4))
            offind = GIDPsw = 1;
        ind += 3;
        err = 0;
    }
    if (!strncasecmp (&exp[ind], "RBI", 3) || !strncasecmp (&exp[ind], "IBB", 3) || !strncasecmp (&exp[ind], "HBP", 3) ||
                                           !strncasecmp (&exp[ind], "OBA", 3) || !strncasecmp (&exp[ind], "BFP", 3) ||
                                           !strncasecmp (&exp[ind], "ERA", 3) || !strncasecmp (&exp[ind], "PCT", 3) || !strncasecmp (&exp[ind], "SHO", 3)) {
        if (!strncasecmp (&exp[ind], "OBA", 3))
            offind = OBAsw = 1;
        if (!strncasecmp (&exp[ind], "BFP", 3))
            pitind = BFPsw = 1;
        if (!strncasecmp (&exp[ind], "ERA", 3))
            pitind = ERAsw = 1;
        if (!strncasecmp (&exp[ind], "PCT", 3))
            pitind = PCTsw = 1;
        if (!strncasecmp (&exp[ind], "SHO", 3))
            pitind = SHOsw = 1;
        ind += 2;
        err = 0;
    }
    if (!strncasecmp (&exp[ind], "SG", 2) || !strncasecmp (&exp[ind], "DB", 2) || !strncasecmp (&exp[ind], "TP", 2) || !strncasecmp (&exp[ind], "HR", 2) ||
            !strncasecmp (&exp[ind], "BB", 2) || !strncasecmp (&exp[ind], "SB", 2) || !strncasecmp (&exp[ind], "CS", 2) ||
            !strncasecmp (&exp[ind], "SF", 2) || !strncasecmp (&exp[ind], "SH", 2) || !strncasecmp (&exp[ind], "AB", 2) ||
            !strncasecmp (&exp[ind], "TB", 2) || !strncasecmp (&exp[ind], "BA", 2) || !strncasecmp (&exp[ind], "SA", 2) ||
            !strncasecmp (&exp[ind], "PO", 2) || !strncasecmp (&exp[ind], "PB", 2) || !strncasecmp (&exp[ind], "FA", 2) ||
            !strncasecmp (&exp[ind], "CG", 2) || !strncasecmp (&exp[ind], "ER", 2) || !strncasecmp (&exp[ind], "GF", 2) ||
            !strncasecmp (&exp[ind], "GS", 2) || !strncasecmp (&exp[ind], "IP", 2) || !strncasecmp (&exp[ind], "WP", 2)) {
        if (!strncasecmp (&exp[ind], "AB", 2))
            offind = ABsw = 1;
        if (!strncasecmp (&exp[ind], "TB", 2))
            offind = TBsw = 1;
        if (!strncasecmp (&exp[ind], "BA", 2))
            offind = BAsw = 1;
        if (!strncasecmp (&exp[ind], "SA", 2))
            offind = SAsw = 1;
        if (!strncasecmp (&exp[ind], "PO", 2))
            offind = POsw = 1;
        if (!strncasecmp (&exp[ind], "PB", 2))
            offind = PBsw = 1;
        if (!strncasecmp (&exp[ind], "FA", 2))
            offind = FAsw = 1;
        if (!strncasecmp (&exp[ind], "CG", 2))
            pitind = CGsw = 1;
        if (!strncasecmp (&exp[ind], "ER", 2))
            pitind = ERsw = 1;
        if (!strncasecmp (&exp[ind], "GF", 2))
            pitind = GFsw = 1;
        if (!strncasecmp (&exp[ind], "GS", 2))
            pitind = GSsw = 1;
        if (!strncasecmp (&exp[ind], "IP", 2))
            pitind = IPsw = 1;
        if (!strncasecmp (&exp[ind], "WP", 2))
            pitind = WPsw = 1;
        ind++;
        err = 0;
    }
    if (!strncasecmp (&exp[ind], "G", 1) || !strncasecmp (&exp[ind], "H", 1) || !strncasecmp (&exp[ind], "R", 1) || !strncasecmp (&exp[ind], "K", 1) ||
                                            !strncasecmp (&exp[ind], "A", 1) || !strncasecmp (&exp[ind], "E", 1) || !strncasecmp (&exp[ind], "B", 1) ||
                                            !strncasecmp (&exp[ind], "W", 1) || !strncasecmp (&exp[ind], "L", 1) || !strncasecmp (&exp[ind], "S", 1))
        if (exp[ind + 1] == ' ' || exp[ind + 1] == '+' || exp[ind + 1] == '-' || exp[ind + 1] == '*' || exp[ind + 1] == '/' ||
                                                     exp[ind + 1] == '(' || exp[ind + 1] == ')' || (ind + 1) >= strlen (&exp[0]))
            if (!ind || (ind && (exp[ind - 1] == ' ' || exp[ind - 1] == '+' || exp[ind - 1] == '-' || exp[ind - 1] == '*' || exp[ind - 1] == '/' ||
                                                 (ind > 1 && !strncasecmp (&exp[ind - 2], "LA", 2)) || exp[ind - 1] == '(' || exp[ind - 1] == ')'))) {
                if (!strncasecmp (&exp[ind], "A", 1))
                    offind = Asw = 1;
                if (!strncasecmp (&exp[ind], "E", 1))
                    offind = Esw = 1;
                if (!strncasecmp (&exp[ind], "B", 1))
                    pitind = Bsw = 1;
                if (!strncasecmp (&exp[ind], "W", 1))
                    pitind = Wsw = 1;
                if (!strncasecmp (&exp[ind], "L", 1))
                    pitind = Lsw = 1;
                if (!strncasecmp (&exp[ind], "S", 1))
                    pitind = Ssw = 1;
                err = 0;
            }

    if (err)
        return -1;

    if (stack < 1) {
        temp.sym = STAT;
        push (&temp);
        return ind;
    }

    peek (&temp);
    precede = temp.sym;
    switch (precede) {
        case MULTIPLY:
        case DIVIDE:
        case ADD:
        case SUBTRACT:
            if (pop (&temp) != 1)
                return -1;
            if (pop (&temp) != 1)
                return -1;
            if (temp.sym != NUMBER && temp.sym != STAT)
                return -1;
            temp.sym = STAT;
            if (push (&temp) != 1)
                return -1;
            break;
        case OPEN:
            temp.sym = STAT;
            if (push (&temp) != 1)
                return -1;
            break;
        case CLOSE:
        case STAT:
        case NUMBER:
            return -1;
            break;
        default:
            return -1;
            break;
    }
    return ind;
}

void
ScoreSeasons (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *box1, *box2, *hbox, *vbox, *label, *separator, *findoff, *findpit, *disbutton, *formvalbut, *formsavbut, *formloadbut, *formcharbut, *table,
              *buttono, *button0, *button0s, *button0n, *sw;
    GSList *group, *group1;
    GtkAdjustment *adj;
    gchar *msg[5], *ERR = "You need to be connected to a NSB server in order to execute this function.", labtxt[] = "Default formula for finding the best offensive seasons:\n(TB + SB + BB + HBP - CS) / (AB - H + CS + GIDP)\n\nDefault formula for finding the best pitching seasons:\n((5 * IP / 9) - ER) + (K / 12) + (S * 3) + SHO + ((W * 6) - (L * 2))\n";
    gint x;

    group = group1 = '\0';  /* to avoid a compile warning */

    if (getBSactive)
        /* only need to execute this dialog if it isn't already active */
        return;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = ERR;
        outMessage (msg);
        return;
    }

    getBSactive = toggle_o0 = toggle_0 = 1;
    bypts = 0;

    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

    dlgBS = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgBS), "Evaluate Player Seasons");
    gtk_window_set_default_size (GTK_WINDOW (dlgBS), 703, 845);
    gtk_signal_connect (GTK_OBJECT (dlgBS), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgBS)->vbox), box1, TRUE, TRUE, 0);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (box1), sw);

    box2 = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), box2);

    label = gtk_label_new ("Evaluate Player Seasons\n\n");
    gtk_container_add (GTK_CONTAINER (box2), label);

    separator = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (box2), separator);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (labtxt);
    gtk_container_add (GTK_CONTAINER (hbox), label);

    table = gtk_table_new (2, 4, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("Current Formula:");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    formentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (formentry), 500);
    gtk_table_attach_defaults (GTK_TABLE (table), formentry, 1, 4, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), formentry);
    gtk_signal_connect (GTK_OBJECT (formentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryForm), NULL);
    gtk_entry_set_text (GTK_ENTRY (formentry), "Default (specified above)");
    formulachanged = 0;

    formvalbut = gtk_button_new_with_label ("Validate Formula");
    g_signal_connect (G_OBJECT (formvalbut), "clicked", G_CALLBACK (ValFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formvalbut, 0, 1, 1, 2);
    formcharbut = gtk_button_new_with_label ("Valid Formula Characters");
    g_signal_connect (G_OBJECT (formcharbut), "clicked", G_CALLBACK (ValidChars), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formcharbut, 1, 2, 1, 2);
    formsavbut = gtk_button_new_with_label ("Save Formula");
    g_signal_connect (G_OBJECT (formsavbut), "clicked", G_CALLBACK (SaveFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formsavbut, 2, 3, 1, 2);
    formloadbut = gtk_button_new_with_label ("Load Formula");
    g_signal_connect (G_OBJECT (formloadbut), "clicked", G_CALLBACK (LoadFormula), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), formloadbut, 3, 4, 1, 2);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), vbox, FALSE, FALSE, 0);

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

    label = gtk_label_new ("IBB - not available 1901-54\nSOPP, 2B, 3B, RBI, SB, CS, SH, SF & OPAB -\n         not available 1901-97, 1999-2019");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    button0 = gtk_radio_button_new_with_label (NULL, "Substitute 0 for Unavailable Stats");
    gtk_box_pack_start (GTK_BOX (box2), button0, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (button0), "clicked", G_CALLBACK (toggle_method_0), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button0));
    buttono = gtk_radio_button_new_with_label (group, "Omit Years Where Stats Are Not Available");
    gtk_box_pack_start (GTK_BOX (box2), buttono, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (buttono), "clicked", G_CALLBACK (toggle_method_o), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), vbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Sometimes, with certain formulas, a \"divide by zero\" circumstance is encountered.");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    label = gtk_label_new ("What to do when that happens?");
    gtk_container_add (GTK_CONTAINER (vbox), label);

    button0s = gtk_radio_button_new_with_label (NULL, "Substitute 0 With 0.00001");
    gtk_box_pack_start (GTK_BOX (box2), button0s, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (button0s), "clicked", G_CALLBACK (toggle_method_0s), NULL);
    group1 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button0s));
    button0n = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button0s), "Do Nothing");
    gtk_box_pack_start (GTK_BOX (box2), button0n, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (button0n), "clicked", G_CALLBACK (toggle_method_0n), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Length of Returned List (500 max): ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    adj = (GtkAdjustment *) gtk_adjustment_new (100.0, 1.0, 500.0, 25.0, 100.0, 0.0);
    spinner = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
    gtk_widget_set_size_request (spinner, 35, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_container_add (GTK_CONTAINER (hbox), spinner);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("List Results: ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    combotype = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (combotype), "Highest to Lowest");
    gtk_combo_box_append_text (GTK_COMBO_BOX (combotype), "Lowest to Highest");
    gtk_combo_box_set_active (GTK_COMBO_BOX (combotype), 0);
    gtk_container_add (GTK_CONTAINER (hbox), combotype);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Minimum Player Requirements to Qualify");
    gtk_container_add (GTK_CONTAINER (hbox), label);

    table = gtk_table_new (3, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic ("For Offense (Plate Appearances):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    label = gtk_label_new_with_mnemonic ("For Pitching (Innings Pitched):");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

    offminentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (offminentry), 6);
    gtk_table_attach_defaults (GTK_TABLE (table), offminentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), offminentry);
    gtk_entry_set_text (GTK_ENTRY (offminentry), "3.1");
    label = gtk_label_new_with_mnemonic ("X number of games player's team played");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
    gtk_signal_connect (GTK_OBJECT (offminentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryMIN), NULL);

    gtk_entry_set_activates_default (GTK_ENTRY (offminentry), TRUE);

    pitminentry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (pitminentry), 6);
    gtk_table_attach_defaults (GTK_TABLE (table), pitminentry, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), pitminentry);
    gtk_entry_set_text (GTK_ENTRY (pitminentry), "1.0");
    label = gtk_label_new_with_mnemonic ("X number of games player's team played");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);
    gtk_signal_connect (GTK_OBJECT (pitminentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryMIN), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);
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
    gtk_entry_set_text (GTK_ENTRY (inentry), "2019");
    gtk_signal_connect (GTK_OBJECT (inentry), "insert_text", GTK_SIGNAL_FUNC (CheckEntryBS), NULL);
    gtk_window_set_focus (GTK_WINDOW (dlgBS), inentry);

    gtk_entry_set_activates_default (GTK_ENTRY (inentry), TRUE);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    disbutton = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroyBS), dlgBS);
    findoff = gtk_button_new_with_label ("Evaluate Offensive Seasons");
    g_signal_connect (G_OBJECT (findoff), "clicked", G_CALLBACK (FindOff), dlgBS);
    findpit = gtk_button_new_with_label ("Evaluate Pitching Seasons");
    g_signal_connect (G_OBJECT (findpit), "clicked", G_CALLBACK (FindPit), dlgBS);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), findoff, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), findpit, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgBS);
}

void
ValFormula (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar *msg[5], UsingDefault[256] = "Using Default Formula.";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formentry));

    if (!strlen (&formentry_text[0]) ||
                       (!strncmp (&formentry_text[0], "Default (specified above)", strlen (&formentry_text[0])) && strlen (&formentry_text[0]) > 1)) {
        msg[0] = &UsingDefault[0];
        outMessage (msg);
        formulachanged = 0;
    }
    else
        ValidateFormula (&formentry_text[0], 0, 0);
}

void
ValidChars (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    FILE *formch;
    gint noformch;

    /* load the file showing valid formula characters if it exists */
    if ((formch = fopen ("/usr/local/share/NSB/FormulaChars.txt", "r")) != NULL) {
        noformch = fread (&formchtext, sizeof formchtext, 1, formch);
        if (!ferror (formch))
            noformch = 0;
        else
            /* error reading file */
            noformch = 1;
        fclose (formch);
    }
    else
        /* error opening file */
        noformch = 1;

    if (noformch) {
        gchar NoFormChAvailable[256] = "Valid formula characters info is not available", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "The valid formula characters information is not available.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoFormChAvailable[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 700, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Valid Formula Characters");
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

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, formchtext, strlen (&formchtext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintFormCh), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
PrintFormCh (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Valid Formula Characters ...", *msg[5];
    gint x;

    print (&formchtext[0]);

    strcpy (&work[0], "Print valid formula characters info.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

void
SaveFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    gint x;
    gchar *msg[5], Nothing[256] = "There is Nothing to Save.", InvForm[256] = "The formula is not valid.  Save anyway?";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formentry));

    if (!strlen (&formentry_text[0]) ||
                       (!strncmp (&formentry_text[0], "Default (specified above)", strlen (&formentry_text[0])) && strlen (&formentry_text[0]) > 1)) {
        msg[0] = &Nothing[0];
        outMessage (msg);
        formulachanged = 0;
        return;
    }
    else
        if (ValidateFormula (&formentry_text[0], 2, 0)) {
            msg[0] = &InvForm[0];
            if (!ShallWeContinue (msg))
                return;
        }

    dialog = gtk_file_chooser_dialog_new ("Save Formula", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
                                          GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
    if (!loadformulasw) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled formula");
    }
    else
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), ifilename);

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
            formulachanged = 0;
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
LoadFormula (GtkWidget *widget, gpointer *pdata) {
    GtkWidget *dialog;
    FILE *iform;

    if (formulachanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved formula.  Continuing will lose any changes.  Continue anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }

    dialog = gtk_file_chooser_dialog_new ("Load Formula", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
                                          GTK_RESPONSE_ACCEPT, NULL);
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
            gtk_entry_set_text (GTK_ENTRY (formentry), ldformula + 10);
            strcpy (&ifilename[0], &filename[0]);
            loadformulasw = 1;
            formulachanged = 0;
        }
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void
DestroyBS (GtkWidget *widget, gpointer *pdata) {
    if (formulachanged) {
        gint x;
        gchar *msg[5], UnsavedFormula[256] = "There is an unsaved formula.  Exiting will lose any changes.  Exit anyway?";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &UnsavedFormula[0];
        if (!ShallWeContinue (msg))
            return;
    }
    getBSactive = 0;
    DestroyDialog (dlgBS, dlgBS);
}

void
FindOff (GtkWidget *widget, gpointer *pdata) {
    strcpy (&buffer[0], "v");
    FindBS ();
}

void
FindPit (GtkWidget *widget, gpointer *pdata) {
    strcpy (&buffer[0], "V");
    FindBS ();
}

void
FindBS () {
    gint x, err, usestdform, decpt, listlen;
    gchar *msg[5], NoYrs[256] = "No years to search.", InvForm[256] = "The formula is not valid.  Make it so.",
          TwoDP[256] = "There can't be more than one decimal point in the minimum requirements.",
          NoInYears[256] = "\nThe years to include is empty.  Assume all years (1901-2019)?\n\n";

    usestdform = 0;
    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (buffer[0] == 'V')
        minentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (pitminentry));
    else
        minentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (offminentry));
    if (!strlen (&minentry_text[0]))
        strcpy (&minentry_text[0], "0.0");

    for (decpt = x = 0; x < strlen (&minentry_text[0]); x++)
        if (minentry_text[x] == '.')
            decpt++;
    if (decpt > 1) {
        msg[0] = &TwoDP[0];
        outMessage (msg);
        return;
    }

    formentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (formentry));

    if (!strlen (&formentry_text[0]) ||
                       (!strncmp (&formentry_text[0], "Default (specified above)", strlen (&formentry_text[0])) && strlen (&formentry_text[0]) > 1)) {
        formulachanged = 0;
        usestdform = 1;
    }
    else
        if (ValidateFormula (&formentry_text[0], 1, 0) || (buffer[0] == 'V' && offind) || (buffer[0] == 'v' && pitind)) {
            msg[0] = &InvForm[0];
            outMessage (msg);
            return;
        }

    inentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (inentry));

    if (!strlen (&inentry_text[0]) && !preferences.AssumeAllYears) {
        msg[0] = &NoInYears[0];
        if (!ShallWeContinue (msg))
            return;
    }

    exentry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (exentry));

    err = ValidateTextBS ();
    if (err) {
        gint errct;
        gchar ExMsg[256] = "In excluded years ...", InMsg[256] = "In included years ...",
              YrAfterHy[256] = "The year after a hyphen must be equal to or greater than the year before that same hyphen.",
              NoYr[256] = "There must be a year after a hyphen.", InitYr[256] = "A year must be the first data.",
              InvYr[256] = "A year is not valid (must be 4 positions and must be 1901-2019).";

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
        return;
    }

    for (x = 0; x < YEAR_SPREAD; x++)
        if (yrs[x])
            break;
    if (x == YEAR_SPREAD) {
        msg[0] = &NoYrs[0];
        outMessage (msg);
        return;
    }

    /* need to pass these to server (even though the server doesn't use value in buffer[1])
       they may be used for printing later */
    buffer[1] =  toggle_o0 + '0';
    buffer[2] =  toggle_0 + '0';
    buffer[3] =  ',';
    buffer[4] =  '\0';

    listlen = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
    strcat (&buffer[0], cnvt_int2str (listlen, 'l'));
    strcat (&buffer[0], ",");

    x = gtk_combo_box_get_active (GTK_COMBO_BOX (combotype));
    strcat (&buffer[0], cnvt_int2str (x, 'l'));

    strcat (&buffer[0], minentry_text);
    strcat (&buffer[0], ",");

    if (usestdform)
        if (buffer[0] == 'v')
            strcat (&buffer[0], "(TB + SB + BB + HBP - CS) / (AB - H + CS + GIDP),");
        else
            strcat (&buffer[0], "((5 * IP / 9) - ER) + (K / 12) + (S * 3) + SHO + ((W * 6) - (L * 2)),");
    else {
        strcat (&buffer[0], formentry_text);
        strcat (&buffer[0], ",");
    }

    if (!toggle_o0) {
        /* omit years with unavailable stats */
        if (usestdform && buffer[0] == 'v')
            /* if using standard offense formula omit years up to and including 1950 (because of GIDP & CS) */
            for (x = 0; x < 50; x++)
                yrs[x] = 0;
        if (strcasestr (formentry_text, "IBB"))
            /* IBB is unavailable for both offense and pitching 1901 through 1954 */
            for (x = 0; x < 54; x++)
                yrs[x] = 0;
        if (!usestdform) {
            if (buffer[0] == 'v') {
                if (strcasestr (formentry_text, "GIDP"))
                    /* GIDP is unavailable for offense 1901 through 1938 */
                    for (x = 0; x < 38; x++)
                        yrs[x] = 0;
                if (strcasestr (formentry_text, "SF"))
                    /* SF is unavailable for offense 1901 through 1953 */
                    for (x = 0; x < 53; x++)
                        yrs[x] = 0;
                if (strcasestr (formentry_text, "K"))
                    /* K is unavailable for offense 1901 through 1912 */
                    for (x = 0; x < 12; x++)
                        yrs[x] = 0;
                if (strcasestr (formentry_text, "CS")) {
                    /* CS is unavailable for offense 1901 through 1919 and 1926 through 1950 */
                    for (x = 0; x < 19; x++)
                        yrs[x] = 0;
                    for (x = 26; x < 50; x++)
                        yrs[x] = 0;
                }
            }
            else {
                gchar *pnt;

                if (strcasestr (formentry_text, "SOPP") || strcasestr (formentry_text, "DB") || strcasestr (formentry_text, "TP") ||
                    strcasestr (formentry_text, "RBI") || strcasestr (formentry_text, "SB") || strcasestr (formentry_text, "CS") ||
                    strcasestr (formentry_text, "SF") || strcasestr (formentry_text, "OPBA")) {
                    /* SOPP, DB, TP, RBI, SB, CS, SF, & OPBA are unavailable for pitching for all years except 1998 */
                    for (x = 0; x < 97; x++)
                        yrs[x] = 0;
                    for (x = 98; x < YEAR_SPREAD; x++)
                        yrs[x] = 0;
                }
                /* make sure SH isn't really SHO */
                pnt = strcasestr (&formentry_text[0], (char *) "SH");
                if (pnt && *(pnt + 2) != 'O' && *(pnt + 2) !='o') {
                    for (x = 0; x < 97; x++)
                        yrs[x] = 0;
                    for (x = 98; x < YEAR_SPREAD; x++)
                        yrs[x] = 0;
                }
            }
        }
    }

    for (x = 0; x < YEAR_SPREAD; x++)
        strcat (&buffer[0], cnvt_int2str (yrs[x], 'l'));

    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    ShowBestSeasons ();
}

int
ValidateTextBS () {
    gint x, z, yrpnt, pos, errors, yr1, yr2;
    gchar yrdata[2][1000], work[10];

    errors = 0;
    for (x = 0; x < YEAR_SPREAD; x++)
        yrs[x] = 0;

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
CheckEntryForm (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    formulachanged = 1;
}

void
CheckEntryBS (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    GtkEditable *editable = GTK_EDITABLE (entry);
    int i, count = 0;
    gchar *result = g_new (gchar, length);

    for (i = 0; i < length; i++) {
        if (!isdigit (text[i]) && text[i] != ' ' && text[i] != '-')
            continue;
        result[count++] = text[i];
    }

    if (count > 0) {
        gtk_signal_handler_block_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntryBS), data);
        gtk_editable_insert_text (editable, result, count, position);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntryBS), data);
    }
    gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");

    g_free (result);
}

void
CheckEntryMIN (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    GtkEditable *editable = GTK_EDITABLE (entry);
    int i, count = 0;
    gchar *result = g_new (gchar, length);

    for (i = 0; i < length; i++) {
        if (!isdigit (text[i]) && text[i] != '.')
            continue;
        result[count++] = text[i];
    }

    if (count > 0) {
        gtk_signal_handler_block_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntryMIN), data);
        gtk_editable_insert_text (editable, result, count, position);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckEntryMIN), data);
    }
    gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");

    g_free (result);
}

gchar buf[500000];

void
ShowBestSeasons () {
    /* show best real life teams */
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_gets (sock, &buf[0], sizeof (buf));

    if (!strncmp (&buf[0], "ERROR", 5)) {
        gint x;
        gchar *msg[5], Problem[256] = "There is a problem with the formula.\n\n", PosInForm[256] = "\n\nAt or near position ",
              Unk[256] = "The problem is unknown.", Stat[256] = "The problem is possibly with a Stat Acronym.",
              Num[256] = "The problem is possibly with a number.", Op[256] = "The problem is possibly with an operator (+ - * /).",
              Paren[256] = "The problem is possibly with a parenthesis.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = Problem;

        if (buf[5] == '1')
             msg[1] = Paren;
        if (buf[5] == '2')
             msg[1] = Op;
        if (buf[5] == '3')
             msg[1] = Num;
        if (buf[5] == '4')
             msg[1] = Stat;
        if (buf[5] == '9')
             msg[1] = Unk;

        for (x = 6; x < 9; x++)
            if (buf[x] == '0')
                buf[x] = ' ';
            else
                break;

        strncat (&PosInForm[0], &buf[6], 4);
        strcat (&PosInForm[0], ".");
        msg[2] = PosInForm;
        outMessage (msg);
        return;
    }

    if (!strncmp (&buf[0], "ERR00", 5)) {
        gint x;
        gchar *msg[5], Zero1[256] = "The formula is trying to divide by 0 at some point.\n", Zero2[256] = "Examine the formula.\n",
              Zero3[256] = "It's possible if the minimum requirement to qualify were increased slightly the problem will go away.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = Zero1;
        msg[1] = Zero2;
        msg[2] = Zero3;

        outMessage (msg);
        return;
    }

    if (!strncmp (&buf[0], "NODATA", 6)) {
        gint x;
        gchar *msg[5], ND1[256] = "There is no data meeting the formula criteria.\n", ND2[256] = "Examine the formula.\n",
              ND3[256] = "Also, try loosening the minimum qualifications.";

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = ND1;
        msg[1] = ND2;
        msg[2] = ND3;

        outMessage (msg);
        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 750, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Evaluate Player Seasons Results");
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

    FillBestSeasons (0);

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, BestSeasonstext, strlen (&BestSeasonstext[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    strcpy (&prtbutBScmd[prtbutBSpnt][0], &buffer[0]);
    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintBestSeasons), GINT_TO_POINTER (prtbutBSpnt));
    prtbutBSpnt++;
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillBestSeasons (int pind) {
    gint x, y, z, len, teamid, yron;
    gfloat tpf;
    gchar year[5], *cc, *cc2, *ccb, *ctemp, charid[3], breakup[200], tpc[5];

    strcpy (&BestSeasonstext[0], "             Evaluate Player Seasons Results");
    if (buffer[0] == 'v')
        strcat (&BestSeasonstext[0], ", Offense");
    else
        strcat (&BestSeasonstext[0], ", Pitching");
    strcat (&BestSeasonstext[0], "\n\n");

    if (buffer[1] == '0')
        strcat (&BestSeasonstext[0], "Omitting years with unavailable stats.\n");
    else
        strcat (&BestSeasonstext[0], "Substituting 0 for unavailable stats.\n");

    if (buffer[2] == '1')
        if (pind)
            strcat (&BestSeasonstext[0], "Substituting 0 with 0.00001 in those records where a divide-by-0 error is\nencountered.\n");
        else
            strcat (&BestSeasonstext[0], "Substituting 0 with 0.00001 in those records where a divide-by-0 error is encountered.\n");
    else
        strcat (&BestSeasonstext[0], "Doing nothing when a divide-by-0 error is encountered.\n");

    strcat (&BestSeasonstext[0], "Minimum Requirements - ");
    cc2 = index (&buffer[4], ',');
    cc2 += 2;
    cc = index (cc2, ',');
    *cc = '\0';

    strcpy (&tpc[0], cc2);
    tpf = atof (&tpc[0]);

    if (tpf) { 
        strcat (&BestSeasonstext[0], cc2);
        if (buffer[0] == 'v')
            if (pind)
                strcat (&BestSeasonstext[0], " plate appearances times the number of games\nplayer's team played.");
            else
                strcat (&BestSeasonstext[0], " plate appearances times the number of games player's team played.");
        else
            if (pind)
                strcat (&BestSeasonstext[0], " innings pitched times the number of games\nplayer's team played.");
            else
                strcat (&BestSeasonstext[0], " innings pitched times the number of games player's team played.");
    }
    else
        strcat (&BestSeasonstext[0], "NONE");

    strcat (&BestSeasonstext[0], "\n\n");
    *cc = ',';
    cc++;

    strcat (&BestSeasonstext[0], "Formula - ");
    cc2 = index (cc, ',');
    *cc2 = '\0';
    strcpy (&breakup[0], cc);
    *cc2 = ',';
    if (pind)
        if (strlen (&breakup[0]) > 65) {
            for (x = 60; x > 50; x--) {
                if (breakup[x] == ' ')
                    break;
                if (breakup[x] == '+' || breakup[x] == '-' || breakup[x] == '*' || breakup[x] == '/' || breakup[x] == ')') {
                    x++;
                    break;
                }
            }
            if (breakup[x] != ' ')
                for (y = strlen (&breakup[0]); y > x; y--)
                    breakup[y] = breakup[y - 1];
            breakup[x] = '\n';
        }

    strcat (&BestSeasonstext[0], &breakup[0]);
    strcat (&BestSeasonstext[0], "\n\n");

    strcat (&BestSeasonstext[0], "Year(s) - ");

    for (yron = x = 0; x < YEAR_SPREAD; x++)
        if (yrs[x])
            if (!yron) {
                strcat (&BestSeasonstext[0], cnvt_int2str (1901 + x, 'l'));
                yron = 1;
            }
            else
                yron = 2;
        else {
            if (yron == 1)
                strcat (&BestSeasonstext[0], " ");
            if (yron == 2) {
                strcat (&BestSeasonstext[0], "-");
                strcat (&BestSeasonstext[0], cnvt_int2str (1901 + x - 1, 'l'));
                strcat (&BestSeasonstext[0], " ");
            }
            yron = 0;
        }
    if (yron == 2) {
        strcat (&BestSeasonstext[0], "-");
        strcat (&BestSeasonstext[0], cnvt_int2str (1901 + x - 1, 'l'));
    }
    strcat (&BestSeasonstext[0], "\n\n");

    if (buffer[0] == 'v')
        strcat (&BestSeasonstext[0], "     Player                           Year Team          Score\n");
    else
        strcat (&BestSeasonstext[0], "     Player                           Year Team           Score\n");

    for (x = 0, cc = &buf[0]; x < 500 && cc < (&buf[0] + strlen (&buf[0])); x++) {
        strcat (&BestSeasonstext[0], cnvt_int2str ((x + 1), 'l'));
        strcat (&BestSeasonstext[0], ".");
        if (x > 98)
            strcat (&BestSeasonstext[0], " ");
        else
            if (x > 8)
                strcat (&BestSeasonstext[0], "  ");
            else
                strcat (&BestSeasonstext[0], "   ");

        strncpy (&year[0], cc, 4);
        year[4] = '\0';
        cc += 5;

        charid[0] = *cc;
        cc++;
        if (*cc != ' ') {
            charid[1] = *cc;
            charid[2] = '\0';
            cc += 2;
        }
        else {
            charid[1] = '\0';
            cc++;
        }
        teamid = atoi (&charid[0]);

        ctemp = cc;
        do {
            ccb = (char *) index (ctemp, ' ');
            ctemp = ccb + 1;
            if (*ctemp == '-')
                break;
        } while (*ctemp > '9' || *ctemp < '0');
        *ccb = '\0';
        len = strlen (cc);

        strcat (&BestSeasonstext[0], cc);
        *ccb = ' ';
        cc = ccb + 1;

        for (y = len; y < 33; y++)
            strcat (&BestSeasonstext[0], " ");

        strcat (&BestSeasonstext[0], &year[0]);
        strcat (&BestSeasonstext[0], " ");

        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
            if (teaminfo[y].id == teamid)
                break;
        strcat (&BestSeasonstext[0], &teaminfo[y].teamabbrev[0]);
        z = strlen (&teaminfo[y].teamabbrev[0]);
        strncat (&BestSeasonstext[0], "           ", 10 - z);

        ccb = (char *) index (cc, ' ');
        *ccb = '\0';

        strcat (&BestSeasonstext[0], cc);
        strcat (&BestSeasonstext[0], "\n");
        *ccb = ' ';
        cc = ccb + 1;
    }
}

void
PrintBestSeasons (GtkWidget *widget, gpointer cnt) {
    gchar Printing[256] = "Printing Evaluate Player Seasons Results ...", *msg[5], *pbuff;
    gint x, icnt = GPOINTER_TO_INT (cnt);
    gchar work1[1024];

    if (icnt > 4095) {
        gchar NoPrint[256] = "Cannot print Best Player Seasons.  Too many windows have been opened.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work1[0], "Too many windows have been opened to print Best Player Seasons.\n");

        msg[0] = &NoPrint[0];

        Add2TextWindow (&work1[0], 1);
        outMessage (msg);

        return;
    }

    /* re-get stuff to print */
    strcpy (&buffer[0], &prtbutBScmd[icnt][0]);
    sock_puts (sock, &buffer[0]);
    sock_gets (sock, &buf[0], sizeof (buf));

    pbuff = index (&buffer[4], ',') + 1;
    for (x = 0; x < YEAR_SPREAD; x++, pbuff++)
        yrs[x] = *pbuff - '0';

    FillBestSeasons (1);
    print (&BestSeasonstext[0]);

    strcpy (&work[0], "Print Evaluate Player Seasons Results.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

int
ValidateFormula (gchar *formentry_text, int ind, int type) {
    gint x, err, index, tindex, errpos = 0, number, onestat;
    gchar *msg[5], ParOOB[256] = "There's an error associated with the parentheses in the formula.  Possibly the parentheses are out of balance.",
          Terr[256], OpErr[256] = "There's an error associated with an operator (+ - * /).", PosInForm[256] = "\n\nAt or near position ",
          UnkErr[256] = "There's an unknown error in the formula.", NumErr[256] = "There's an error associated with a number (0-9).",
          SAErr[256] = "There's an error associated with a Stat Acronym.", OneStat[256] = "There needs to be at least one Stat Acronym in the formula.",
          OffOnly[256] = "The following Stat Acronyms are for an offensive formula only:  ",
          PitOnly[256] = "The following Stat Acronyms are for a pitching formula only:  ",
          LAOneStat[256] = "There needs to be at least one non-LA Stat Acronym in the formula when there is a LA Stat Acronym.";
    struct level temp;

    pitind = offind = err = stack = index = onestat = LAsw = 0;
    SOPPsw = OPBAsw = OPABsw = GIDPsw = OBAsw = BFPsw = ERAsw = PCTsw = SHOsw = ABsw = TBsw = BAsw = SAsw = POsw = PBsw = FAsw = CGsw = ERsw = GFsw =
                                                                                             GSsw = IPsw = WPsw = Asw = Esw = Bsw = Wsw = Lsw = Ssw = 0;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    for (x = 0; x < STACKSIZE; x++)
        top[x].sym = INVALID;
    temp.sym = INVALID;

    while (index < strlen (&formentry_text[0])) {
        switch (formentry_text[index]) {
            case '(':
                if (calc_paren (OPEN) == CALC_FAIL) {
                    errpos = index;
                    err = 1;
                }
                break;
            case ')':
                if (calc_paren (CLOSE) == CALC_FAIL) {
                    errpos = index;
                    err = 2;
                }
                break;
            case '*':
                if (calc_binary_op (MULTIPLY) == CALC_FAIL) {
                    errpos = index;
                    err = 3;
                }
                break;
            case '/':
                if (calc_binary_op (DIVIDE) == CALC_FAIL) {
                    errpos = index;
                    err = 4;
                }
                break;
            case '+':
                if (calc_binary_op (ADD) == CALC_FAIL) {
                    errpos = index;
                    err = 5;
                }
                break;
            case '-':
                if (calc_binary_op (SUBTRACT) == CALC_FAIL) {
                    errpos = index;
                    err = 6;
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                number = (formentry_text[index] - 0x30);
                while ((formentry_text[index + 1] >= 0x30) && (formentry_text[index + 1] <= 0x39)) {
                    index++;
                    number = (number * 10) + (formentry_text[index] - 0x30);
                }
                if (calc_number (number) == CALC_FAIL) {
                    errpos = index;
                    err = 7;
                }
                break;    
            case ' ':
                break;
            default:
                tindex = check4stat (formentry_text, index);
                if (tindex == -1) {
                    errpos = index;
                    err = 8;
                }
                else {
                    index = tindex;
                    onestat++;
                }
                break;
        }
        if (err)
            break;
        index++;
    }
    if (!err) {
        if (stack != 1) {
            errpos = index;
            err = 9;
        }

        if (pop (&temp) != 1) {
            errpos = index;
            err = 10;
        }
        if (temp.sym != NUMBER && temp.sym != STAT) {
            errpos = index;
            err = 11;
        }
    }

    if (err) {
        if (!errpos)
            errpos = 1;
        strcat (&PosInForm[0], (char *) cnvt_int2str (errpos, 'l'));
        strcat (&PosInForm[0], ".");
        msg[1] = PosInForm;
    }

    if (err == 1 || err == 2) {
        msg[0] = ParOOB;
        outMessage (msg);
    }

    if (err > 2 && err < 7) {
        msg[0] = OpErr;
        outMessage (msg);
    }

    if (err == 7) {
        msg[0] = NumErr;
        outMessage (msg);
    }

    if (err == 8) {
        msg[0] = SAErr;
        outMessage (msg);
    }

    if (err > 8) {
        msg[0] = UnkErr;
        outMessage (msg);
    }

    if (!err && !onestat) {
        err = 1;
        msg[0] = OneStat;
        outMessage (msg);
    }

    if (!err && LAsw && (onestat <= LAsw)) {
        err = 1;
        msg[0] = LAOneStat;
        outMessage (msg);
    }

    if ((!type && ind == 1 && offind && buffer[0] == 'V') || (type > 1 && ind == 1 && offind)) {
        strcpy (&Terr[0], &OffOnly[0]);
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (FAsw)
            strcat (&Terr[0], "FA ");
        if (PBsw)
            strcat (&Terr[0], "PB ");
        if (POsw)
            strcat (&Terr[0], "PO ");
        if (Esw)
            strcat (&Terr[0], "E ");
        if (Asw)
            strcat (&Terr[0], "A ");
        if (GIDPsw)
            strcat (&Terr[0], "GIDP ");
        if (OBAsw)
            strcat (&Terr[0], "OBA ");
        if (SAsw)
            strcat (&Terr[0], "SA ");
        if (BAsw)
            strcat (&Terr[0], "BA ");
        if (TBsw)
            strcat (&Terr[0], "TB ");
        if (ABsw)
            strcat (&Terr[0], "AB ");

        msg[0] = Terr;
        outMessage (msg);
    }

    if ((!type && ind == 1 && pitind && buffer[0] == 'v') || (type == 1 && ind == 1 && pitind)) {
        strcpy (&Terr[0], &PitOnly[0]);
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (WPsw)
            strcat (&Terr[0], "WP ");
        if (SHOsw)
            strcat (&Terr[0], "SHO ");
        if (SOPPsw)
            strcat (&Terr[0], "SOPP ");
        if (Ssw)
            strcat (&Terr[0], "S ");
        if (PCTsw)
            strcat (&Terr[0], "PCT ");
        if (OPBAsw)
            strcat (&Terr[0], "OPBA ");
        if (OPABsw)
            strcat (&Terr[0], "OPAB ");
        if (Lsw)
            strcat (&Terr[0], "L ");
        if (Wsw)
            strcat (&Terr[0], "W ");
        if (IPsw)
            strcat (&Terr[0], "IP ");
        if (GSsw)
            strcat (&Terr[0], "GS ");
        if (GFsw)
            strcat (&Terr[0], "GF ");
        if (ERAsw)
            strcat (&Terr[0], "ERA ");
        if (ERsw)
            strcat (&Terr[0], "ER ");
        if (CGsw)
            strcat (&Terr[0], "CG ");
        if (BFPsw)
            strcat (&Terr[0], "BFP ");
        if (Bsw)
            strcat (&Terr[0], "B ");

        msg[0] = Terr;
        outMessage (msg);
    }

    if (!err && !ind) {
        gchar OK[256] = "The formula seems okay ... so far.";

        msg[0] = OK;
        outMessage (msg);
    }

    return err;
}

void
toggle_method_o (GtkWidget *widget, gpointer *pdata) {
    toggle_o0 = 0;
}

void
toggle_method_0 (GtkWidget *widget, gpointer *pdata) {
    toggle_o0 = 1;
}

void
toggle_method_0s (GtkWidget *widget, gpointer *pdata) {
    toggle_0 = 1;
}

void
toggle_method_0n (GtkWidget *widget, gpointer *pdata) {
    toggle_0 = 2;
}

