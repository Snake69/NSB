/* set up a season */

#include "gtknsbc.h"
#include "db.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

/* TreeItem structure */
typedef struct _TreeItem TreeItem;
struct _TreeItem {
    const gchar    *label;
    gboolean        ale;
    gboolean        alc;
    gboolean        alw;
    gboolean        nle;
    gboolean        nlc;
    gboolean        nlw;
    TreeItem       *children;
};

TreeItem toplevel[200], teams[200][500];
gint toplevelentries;
gchar yearsc[200][5], nmwork[50], nm[200][50], nmseq[5000][50];

/* columns */
enum {
    NAME_COLUMN = 0,
    ALE_COLUMN,
    ALC_COLUMN,
    ALW_COLUMN,
    NLE_COLUMN,
    NLC_COLUMN,
    NLW_COLUMN,

    VISIBLE_COLUMN,
    NUM_COLUMNS
};

GtkWidget *lwin, *canbutton, *comboPSR1, *comboPSR2, *comboPSR3, *comboPSR4;

struct {
    int teamid,
        year,
        ldiv;  /* 0 = not selected, 1 = AL East, 2 = AL Central, 3 = AL West,
                                    4 = NL East, 5 = NL Central, 6 = NL West */
} leagueinfo[5000];
gint swdhNL, swdhAL, swdhAll, swdhNo, swWC0, swWC1, swWC2, numteams;

void
SetUpLeague () {
    GtkWidget *box1, *box2, *hbox, *rbox1, *rbox2, *separator, *label, *okbutton, *statusbut, *dhNL, *dhAL, *dhAll, *dhNo, *WC0, *WC1, *WC2, *sw, *treeview;
    GSList *group, *group2;
    GtkTreeModel *model;

    sock_puts (sock, "L\n");  /* tell the server */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }

    if (!strncmp (&buffer[0], "-2", 2)) {
        /* this happens if the directory /usr/var/NSB does not exist on the server */
        gchar NoDir[256] = "There is a problem with the server.  Try again later.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Encountered error when talking to server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);

        return;
    }

    if (!strlen (&buffer[0])) {
        /* this happens if there are no groups of teams available */
        gchar NoTeams[256] = "No groups of teams are available in order to set up a season.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No groups of teams available in order to set up a season on server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoTeams[0];
        outMessage (msg);

        return;
    }

    if (!strcmp (&buffer[0], "League Already")) {
        /* this happens if an ongoing season is already established for this user */
        gchar *msg[5];
        gint x;

        for (x = 0; x < 5; x ++)
            msg[x] = NULL;

        msg[0] = "A season is already established for you on this server.  ";
        msg[1] = "Do you still want to set up a new season?  ";
        msg[2] = "(If so, the current season and all stats for the current season will be lost.)";
        if (!ShallWeContinue (msg)) {
            sock_puts (sock, "X\n");
            return;
        }
    
        sock_puts (sock, "GO\n");
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            GotError ();
            return;
        }
    }

    /*
       data in buffer[] is formatted as such:
       YYYY (year ... will be 0000 for user-created teams)
       [space]
       team name (filename format, i.e., no embedded whitespace)
       [space]
       team name
       [space]
       .
       .
       .
       YYYY
       [space]
       team name
       [space]
       .
       .
       .
       etc for all years present on the server
    */
    PopulateTopLevel ();
    PopulateChildren ();
    swdhNL = swdhAll = swdhNo = swWC1 = swWC2 = 0;
    swdhAL = swWC0 = 1;

    /* create a window */
    lwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (lwin), "Establish a Season");
    gtk_container_set_border_width (GTK_CONTAINER (lwin), 10);
    gtk_window_set_modal (GTK_WINDOW (lwin), TRUE);
    g_signal_connect (G_OBJECT (lwin), "destroy", G_CALLBACK (killlwin), lwin);
    gtk_signal_connect (GTK_OBJECT (lwin), "delete_event", GTK_SIGNAL_FUNC (killlwin), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (lwin), box1);

    box2 = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (box2), gtk_label_new ("Select Teams to be in Your Season"), FALSE, FALSE, 0);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box2), sw, TRUE, TRUE, 0);

    /* create model */
    model = create_model ();

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    add_columns (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (sw), treeview);

    /* collapse all rows after the treeview widget has been realized */
    g_signal_connect (treeview, "realize", G_CALLBACK (gtk_tree_view_collapse_all), NULL);
    gtk_window_set_default_size (GTK_WINDOW (lwin), 750, 400);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, TRUE, TRUE, 0);

    rbox1 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (hbox), rbox1, TRUE, TRUE, 0);

    label = gtk_label_new ("Designated Hitter:");
    gtk_container_add (GTK_CONTAINER (rbox1), label);

    dhAL = gtk_radio_button_new_with_label (NULL, "AL Teams Only");
    gtk_box_pack_start (GTK_BOX (rbox1), dhAL, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhAL), "clicked", G_CALLBACK (CBdhAL), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dhAL));

    dhNL = gtk_radio_button_new_with_label (group, "NL Teams Only");
    gtk_box_pack_start (GTK_BOX (rbox1), dhNL, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhNL), "clicked", G_CALLBACK (CBdhNL), NULL);

    dhAll = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (dhAL), "All Teams");
    gtk_box_pack_start (GTK_BOX (rbox1), dhAll, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhAll), "clicked", G_CALLBACK (CBdhAll), NULL);

    dhNo = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (dhAll), "None");
    gtk_box_pack_start (GTK_BOX (rbox1), dhNo, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhNo), "clicked", G_CALLBACK (CBdhNo), NULL);

    rbox1 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (hbox), rbox1, TRUE, TRUE, 0);

    label = gtk_label_new ("Number of Wild Card Teams");
    gtk_container_add (GTK_CONTAINER (rbox1), label);
    label = gtk_label_new ("in Post-Season:");
    gtk_container_add (GTK_CONTAINER (rbox1), label);

    WC0 = gtk_radio_button_new_with_label (NULL, "None");
    gtk_box_pack_start (GTK_BOX (rbox1), WC0, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (WC0), "clicked", G_CALLBACK (CBWC0), NULL);
    group2 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (WC0));

    WC1 = gtk_radio_button_new_with_label (group2, "1");
    gtk_box_pack_start (GTK_BOX (rbox1), WC1, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (WC1), "clicked", G_CALLBACK (CBWC1), NULL);

    WC2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (WC0), "2");
    gtk_box_pack_start (GTK_BOX (rbox1), WC2, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (WC2), "clicked", G_CALLBACK (CBWC2), NULL);

    rbox1 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (hbox), rbox1, TRUE, TRUE, 0);

    label = gtk_label_new ("Post-Season By Round:");
    gtk_container_add (GTK_CONTAINER (rbox1), label);

    label = gtk_label_new ("(NOTE - if the values here are unusable");
    gtk_container_add (GTK_CONTAINER (rbox1), label);

    label = gtk_label_new ("they will be made usable)");
    gtk_container_add (GTK_CONTAINER (rbox1), label);

    rbox2 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), rbox2, TRUE, TRUE, 0);

    label = gtk_label_new ("Round 1 - Best of ");
    gtk_box_pack_start (GTK_BOX (rbox2), label, TRUE, TRUE, 0);

    comboPSR1 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR1), "9");
    gtk_container_add (GTK_CONTAINER (rbox2), comboPSR1);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR1), 0);

    rbox2 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), rbox2, TRUE, TRUE, 0);

    label = gtk_label_new ("Round 2 - Best of ");
    gtk_box_pack_start (GTK_BOX (rbox2), label, TRUE, TRUE, 0);

    comboPSR2 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "No Round");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR2), "9");
    gtk_container_add (GTK_CONTAINER (rbox2), comboPSR2);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR2), 3);

    rbox2 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), rbox2, TRUE, TRUE, 0);

    label = gtk_label_new ("Round 3 - Best of ");
    gtk_box_pack_start (GTK_BOX (rbox2), label, TRUE, TRUE, 0);

    comboPSR3 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "No Round");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR3), "9");
    gtk_container_add (GTK_CONTAINER (rbox2), comboPSR3);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR3), 4);

    rbox2 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), rbox2, TRUE, TRUE, 0);

    label = gtk_label_new ("Round 4 - Best of ");
    gtk_box_pack_start (GTK_BOX (rbox2), label, TRUE, TRUE, 0);

    comboPSR4 = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "No Round");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "1");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "3");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "5");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "7");
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboPSR4), "9");
    gtk_container_add (GTK_CONTAINER (rbox2), comboPSR4);
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboPSR4), 4);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    okbutton = gtk_button_new_with_label ("OK");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (TellServerAboutLeague), model);
    statusbut = gtk_button_new_with_label ("Show Season Creation Status");
    g_signal_connect (G_OBJECT (statusbut), "clicked", G_CALLBACK (SeeSetupStatus), model);
    canbutton = gtk_button_new_with_label ("CANCEL");
    g_signal_connect (G_OBJECT (canbutton), "clicked", G_CALLBACK (TellServerAboutLeague), model);
    gtk_box_pack_start (GTK_BOX (hbox), okbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), statusbut, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), canbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (lwin);
}

void
CBdhNL (GtkWidget *widget, gpointer *pdata) {
    swdhNL = 1;
    swdhAL = swdhAll = swdhNo = 0;
}

void
CBdhAL (GtkWidget *widget, gpointer *pdata) {
    swdhAL = 1;
    swdhNL = swdhAll = swdhNo = 0;
}

void
CBdhAll (GtkWidget *widget, gpointer *pdata) {
    swdhAll = 1;
    swdhNL = swdhAL = swdhNo = 0;
}

void
CBdhNo (GtkWidget *widget, gpointer *pdata) {
    swdhNo = 1;
    swdhNL = swdhAL = swdhAll = 0;
}

void
CBWC0 (GtkWidget *widget, gpointer *pdata) {
    swWC0 = 1;
    swWC1 = swWC2 = 0;
}

void
CBWC1 (GtkWidget *widget, gpointer *pdata) {
    swWC1 = 1;
    swWC0 = swWC2 = 0;
}

void
CBWC2 (GtkWidget *widget, gpointer *pdata) {
    swWC2 = 1;
    swWC0 = swWC1 = 0;
}

void
PopulateTopLevel () {
    gchar *cc, *work;
    gint x, buflen;

    for (x = 0, cc = &buffer[0], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
        work = (char *) index (cc, ' ');
        if ((work - cc) == 4) {
            /* found a year */
            strncpy (&yearsc[x][0], cc, 4);
            yearsc[x][4] = '\0';
            if (yearsc[x][0] == '0')
                toplevel[x].label = "User-Created";
            else
                toplevel[x].label = yearsc[x];
            toplevel[x].children = teams[x];
            toplevel[x].ale = toplevel[x].alc = toplevel[x].alw = toplevel[x].nle = toplevel[x].nlc = toplevel[x].nlw = FALSE;
            x++;
        }
        cc = work;
    }
    toplevel[x].label = NULL;
    toplevelentries = x;
}

void
PopulateChildren () {
    gchar *cc, *work, yearc[5];
    gint buflen, x, y, z, yr = 0, nmcnt = 0;

    for (x = -1, numteams = y = 0, cc = &buffer[0], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
        work = (char *) index (cc, ' ');
        if ((work - cc) == 4) {
            if (x >= 0)
                teams[x][y].label = NULL;
            x++;
            strncpy (&yearc[0], cc, 4);
            yearc[4] = '\0';
            yr = atoi (&yearc[0]);  /* convert year to an integer */
            y = 0;
        }
        else {
            strncpy (&nmwork[0], cc, (work - cc));
            nmwork[work - cc] = '\0';
            if (yr) {
                for (z = 0; z <= NUMBER_OF_TEAMS; z++)
                    if (!strcmp (&teaminfo[z].filename[0], &nmwork[0])) {
                        leagueinfo[numteams].year = yr;
                        leagueinfo[numteams].teamid = teaminfo[z].id;
                        leagueinfo[numteams].ldiv = 0;
                        break;
                    }                    
            }
            else
                leagueinfo[numteams].year = leagueinfo[numteams].teamid = leagueinfo[numteams].ldiv = 0;
            if (yr)
                teams[x][y].label = teaminfo[z].teamname;
            else {
                strcpy (&nm[nmcnt][0], &nmwork[0]);
                strcpy (&nmseq[numteams][0], &nmwork[0]);
                teams[x][y].label = nm[nmcnt];
                nmcnt++;
            }
            teams[x][y].ale = teams[x][y].alc = teams[x][y].alw = teams[x][y].nle = teams[x][y].nlc = teams[x][y].nlw = FALSE;
            teams[x][y].children = NULL;
            y++;
            numteams++;
        }
        cc = work;
    }
    teams[x][y].label = NULL;
    teams[x + 1][0].label = NULL;
}

GtkTreeModel *
create_model () {
    GtkTreeStore *model;
    GtkTreeIter iter;
    TreeItem *years = toplevel;

    /* create tree store */
    model = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                                            G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
    /* add data to the tree store */
    while (years->label) {
        TreeItem *division = years->children;

        gtk_tree_store_append (model, &iter, NULL);
        gtk_tree_store_set (model, &iter, NAME_COLUMN, years->label, ALE_COLUMN, FALSE, ALC_COLUMN, FALSE,
                                                  ALW_COLUMN, FALSE, NLE_COLUMN, FALSE, NLC_COLUMN, FALSE,
                                                                 NLW_COLUMN, FALSE, VISIBLE_COLUMN, FALSE, -1);
        /* add children */
        while (division->label) {
            GtkTreeIter child_iter;

            gtk_tree_store_append (model, &child_iter, &iter);
            gtk_tree_store_set (model, &child_iter, NAME_COLUMN, division->label, ALE_COLUMN, division->ale,
                            ALC_COLUMN, division->alc, ALW_COLUMN, division->alw, NLE_COLUMN, division->nle,
                                 NLC_COLUMN, division->nlc, NLW_COLUMN, division->nlw, VISIBLE_COLUMN, TRUE, -1);
            division++;
        }
        years++;
    }
    return GTK_TREE_MODEL (model);
}

void
item_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data) {
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    gboolean toggle_item;
    gint *column, x;

    column = g_object_get_data (G_OBJECT (cell), "column");

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, column, &toggle_item, -1);

    /* flip the value */
    toggle_item ^= 1;

    /* set all columns in selected row to FALSE since we're treating each row of checks as radio-checks */
    for (x = ALE_COLUMN; x < (NLW_COLUMN + 1); x++)
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, x, FALSE, -1);

    /* set new value on the clicked column */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, toggle_item, -1);

    /* clean up */
    gtk_tree_path_free (path);
}

gint
killlwin (GtkWidget *widget, GdkEventConfigure *event) {
    gtk_widget_destroy (GTK_WIDGET (widget));
    sock_puts (sock, "X\n");
    return TRUE ;
}

void
SeeSetupStatus (GtkWidget *widget, gpointer data) {
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkWidget *dlgSU, *box1, *box2, *hbox, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    gint sub, t, x, y, row_count;
    gchar path_str[10] = " ", leaguestatus[500000];
    gboolean valid, ale, alc, alw, nle, nlc, nlw;

    row_count = 0;
    for (x = 0; x < numteams; x++)
        leagueinfo[x].ldiv = 0;

    for (sub = 0; sub < toplevelentries; sub++) {
        /* walk through the root level of data (years) */
        sprintf (&path_str[0], "%d:0", sub);
        path = gtk_tree_path_new_from_string (&path_str[0]);

        valid = gtk_tree_model_get_iter (model, &iter, path);
        while (valid) {
            /* walk through the sub-level (teams), reading each row */
            gtk_tree_model_get (model, &iter, ALE_COLUMN, &ale, ALC_COLUMN, &alc, ALW_COLUMN, &alw, NLE_COLUMN, &nle, NLC_COLUMN, &nlc, NLW_COLUMN, &nlw, -1);
            if (ale)
                leagueinfo[row_count].ldiv = 1;
            if (alc)
                leagueinfo[row_count].ldiv = 2;
            if (alw)
                leagueinfo[row_count].ldiv = 3;
            if (nle)
                leagueinfo[row_count].ldiv = 4;
            if (nlc)
                leagueinfo[row_count].ldiv = 5;
            if (nlw)
                leagueinfo[row_count].ldiv = 6;

            row_count++;
            valid = gtk_tree_model_iter_next (model, &iter);
        }
    }

    dlgSU = gtk_dialog_new_with_buttons ("Season Setup Status", NULL, GTK_DIALOG_MODAL, "Dismiss", 0, NULL);
    gtk_window_set_default_size (GTK_WINDOW (dlgSU), 550, 600);
    gtk_signal_connect (GTK_OBJECT (dlgSU), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgSU)->vbox), box1, TRUE, TRUE, 0);

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

    strcpy (&leaguestatus[0], "\n                  Season So Far\n\n\n");

    if (swdhAL)
        strcat (&leaguestatus[0], "Designated Hitter used for AL teams only");
    else
        if (swdhNL)
            strcat (&leaguestatus[0], "Designated Hitter used for NL teams only");
        else
            if (swdhAll)
                strcat (&leaguestatus[0], "Designated Hitter used for all teams");
            else
                strcat (&leaguestatus[0], "No Designated Hitter being used");
    strcat (&leaguestatus[0], "\n");
    if (swWC0)
        strcat (&leaguestatus[0], "No Wild Card teams in post-season");
    else
        if (swWC1)
            strcat (&leaguestatus[0], "1 Wild Card team in post-season");
        else
            strcat (&leaguestatus[0], "2 Wild Card teams in post-season");
    strcat (&leaguestatus[0], "\n");

    strcat (&leaguestatus[0], "Post-Season, Round 1 - Best of ");
    strcat (&leaguestatus[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR1)));
    strcat (&leaguestatus[0], "\n");
    strcat (&leaguestatus[0], "Post-Season, Round 2 - ");
    if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR2)), "No Round"))
        strcat (&leaguestatus[0], "No Round");
    else {
        strcat (&leaguestatus[0], "Best of ");
        strcat (&leaguestatus[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR2)));
    }
    strcat (&leaguestatus[0], "\n");
    strcat (&leaguestatus[0], "Post-Season, Round 3 - ");
    if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR3)), "No Round"))
        strcat (&leaguestatus[0], "No Round");
    else {
        strcat (&leaguestatus[0], "Best of ");
        strcat (&leaguestatus[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR3)));
    }
    strcat (&leaguestatus[0], "\n");
    strcat (&leaguestatus[0], "Post-Season, Round 4 - ");
    if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR4)), "No Round"))
        strcat (&leaguestatus[0], "No Round");
    else {
        strcat (&leaguestatus[0], "Best of ");
        strcat (&leaguestatus[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR4)));
    }

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "AL East\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 1) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "AL Central\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 2) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "AL West\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 3) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "NL East\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 4) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "NL Central\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 5) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    strcat (&leaguestatus[0], "NL West\n\n");
    for (t = x = 0; x < numteams; x++)
        if (leagueinfo[x].ldiv == 6) {
            if (leagueinfo[x].year) {
                strcat (&leaguestatus[0], (char *) cnvt_int2str ((leagueinfo[x].year), 'l'));
                strcat (&leaguestatus[0], " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == leagueinfo[x].teamid) {
                        strcat (&leaguestatus[0], &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (&leaguestatus[0], &nmseq[x][0]);
            strcat (&leaguestatus[0], "\n");
            t = 1;
        }
    if (!t)
        strcat (&leaguestatus[0], "No Teams");

    strcat (&leaguestatus[0], "\n\n\n");

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, leaguestatus, strlen (&leaguestatus[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);
    gtk_widget_show_all (box1);

    gtk_dialog_run (GTK_DIALOG (dlgSU));
    DestroyDialog (dlgSU, dlgSU);
}

void
TellServerAboutLeague (GtkWidget *widget, gpointer data) {
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint sub, pos, x, row_count, alsw, nlsw, nodiv[2];
    gchar path_str[10] = " ", SuccessLeague[256] = "Season successfully established on server.", *msg[5];
    gboolean valid, ale, alc, alw, nle, nlc, nlw, tteams[6];

    for (x = 0; x < 5; x++)
        msg[x] = NULL;
    for (x = 0; x < 6; x++)
        tteams[x] = 0;
    row_count = 0;
    for (x = 0; x < numteams; x++)
        leagueinfo[x].ldiv = 0;

    if (widget == canbutton) {
        sock_puts (sock, "X\n");
        SetLeagueUnderWay (0);
        DestroyDialog (GTK_WIDGET (lwin), GTK_WIDGET (lwin));
    }
    else {
        for (sub = 0; sub < toplevelentries; sub++) {
            /* walk through the root level of data (years) */
            sprintf (&path_str[0], "%d:0", sub);
            path = gtk_tree_path_new_from_string (&path_str[0]);

            valid = gtk_tree_model_get_iter (model, &iter, path);
            while (valid) {
                /* walk through the sub-level (teams), reading each row */
                gtk_tree_model_get (model, &iter, ALE_COLUMN, &ale, ALC_COLUMN, &alc, ALW_COLUMN, &alw,
                                                  NLE_COLUMN, &nle, NLC_COLUMN, &nlc, NLW_COLUMN, &nlw, -1);
                if (ale)
                    leagueinfo[row_count].ldiv = 1;
                if (alc)
                    leagueinfo[row_count].ldiv = 2;
                if (alw)
                    leagueinfo[row_count].ldiv = 3;
                if (nle)
                    leagueinfo[row_count].ldiv = 4;
                if (nlc)
                    leagueinfo[row_count].ldiv = 5;
                if (nlw)
                    leagueinfo[row_count].ldiv = 6;

                row_count++;
                valid = gtk_tree_model_iter_next (model, &iter);
            }
        }

        for (alsw = nlsw = x = 0; x < numteams; x++) {
            if (leagueinfo[x].ldiv > 0 && leagueinfo[x].ldiv < 4)
                alsw = YES;
            if (leagueinfo[x].ldiv > 3 && leagueinfo[x].ldiv < 7)
                nlsw = YES;
            if (leagueinfo[x].ldiv)
                tteams[leagueinfo[x].ldiv - 1]++;
        }
        if (!alsw && !nlsw) {
            gchar NoTeams[256] = "No teams were selected for the season.", *msg[5];
            gint x;

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "No Teams selected when attempting to set up a season.\n\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoTeams[0];
            outMessage (msg);

            return;
        }
        else {
            gchar Lim1Div[256] = "There can be no more than 50 teams in any one division.",
                  LimLeague[256] = "There can be no more than 100 teams in the entire season.",
                  SameNoDiv[256] = "Each league must contain the same number of divisions.",
                  EastDiv[256] = "With 2 divisions some teams must be in the East.",
                  MinDiv[256] = "Each established division must contain at least 2 teams.", *msg[5];
            gint x;

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            for (x = 0; x < 6; x++)
                if (tteams[x] > 50)
                    break;
            if (x < 6) {
                strcpy (&work[0], "There can be no more than 50 teams in any one division when setting up a season.\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &Lim1Div[0];
                outMessage (msg);

                return;
            }
            if ((tteams[0] + tteams[1] + tteams[2] + tteams[3] + tteams[4] + tteams[5]) > 100) {
                strcpy (&work[0], "There can be no more than 100 teams in an entire season.\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &LimLeague[0];
                outMessage (msg);

                return;
            }
            if (tteams[0] == 1 || tteams[1] == 1 || tteams[2] == 1 || tteams[3] == 1 || tteams[4] == 1 || tteams[5] == 1) {
                strcpy (&work[0], "Each established division must contain at least 2 teams.\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &MinDiv[0];
                outMessage (msg);

                return;
            }
            for (nodiv[0] = x = 0; x < 3; x++)
                if (tteams[x])
                    nodiv[0]++;
            for (nodiv[1] = 0, x = 3; x < 6; x++)
                if (tteams[x])
                    nodiv[1]++;
            if ((nodiv[0] == 2 && !tteams[0]) || (nodiv[1] == 2 && !tteams[3])) {
                strcpy (&work[0], "With 2 divisions some teams must be in the East.\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &EastDiv[0];
                outMessage (msg);

                return;
            }
            if (nodiv[0] && nodiv[1] && nodiv[0] != nodiv[1]) {
                strcpy (&work[0], "Each league must contain the same number of divisions.\n\n");
                Add2TextWindow (&work[0], 1);

                msg[0] = &SameNoDiv[0];
                outMessage (msg);

                return;
            }
        }

        if (alsw && nlsw)
            if (swdhAL)
                buffer[0] = '1';   /* AL only */
            else
                if (swdhNL)
                    buffer[0] = '2';   /* NL only */
                else
                    if (swdhAll)
                        buffer[0] = '3';   /* DH in both AL & NL */
                    else
                        buffer[0] = '0';       /* no DH */
        else
            if (alsw)
                if (swdhAL || swdhAll)
                    buffer[0] = '1';   /* AL only */
                else
                    buffer[0] = '0';       /* no DH */
            else
                if (swdhNL || swdhAll)
                    buffer[0] = '2';   /* NL only */
                else
                    buffer[0] = '0';       /* no DH */

        /* # of leagues */
        if (alsw && nlsw)
            buffer[1] = '2';
        else
            buffer[1] = '1';
        /* # of divisions per league */
        if (nodiv[0])
            buffer[2] = nodiv[0] + '0';
        else
            buffer[2] = nodiv[1] + '0';
        /* # of wild card teams in post-season per league */
        if ((alsw + nlsw) == 1 && (nodiv[0] + nodiv[1]) == 1 && swWC0) {
            /* if there is 1 league and 1 division then there must be at least 1 WC team */
            swWC0 = 0;
            swWC1 = 1;
        }
        if (swWC0)
            buffer[3] = '0';
        else
            if (swWC1)
                buffer[3] = '1';
            else
                buffer[3] = '2';
        buffer[4] = '\0';
        /* # of max games per round in post-season - values will be made sane by server if they are insane */
        strcat (&buffer[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR1)));
        if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR2)), "No Round"))
            strcat (&buffer[0], "0");
        else
            strcat (&buffer[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR2)));
        if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR3)), "No Round"))
            strcat (&buffer[0], "0");
        else
            strcat (&buffer[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR3)));
        if (!strcmp (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR4)), "No Round"))
            strcat (&buffer[0], "0");
        else
            strcat (&buffer[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboPSR4)));

        for (x = 0, pos = 8; x < numteams; x++)
            buffer[pos++] = leagueinfo[x].ldiv + '0';
        buffer[pos++] = '\n';
        buffer[pos] = '\0';
        sock_puts (sock, &buffer[0]);

        strcpy (&work[0], "Season established on server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 0);
        SetLeagueUnderWay (1);
        SetSeriesUnderWay (0);
        DestroyDialog (GTK_WIDGET (lwin), GTK_WIDGET (lwin));
        msg[0] = &SuccessLeague[0];
        outMessage (msg);
    }
}

void
add_columns (GtkTreeView *treeview) {
    gint col_offset;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    /* column for years and team names */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Year->Teams", renderer, "text", NAME_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* AL-East column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) ALE_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "AL-East", renderer,
                                                           "visible", VISIBLE_COLUMN, "active", ALE_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* AL-Central column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) ALC_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "AL-Central", renderer,
                                                              "visible", VISIBLE_COLUMN, "active", ALC_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* AL-West column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) ALW_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "AL-West", renderer,
                                                           "visible", VISIBLE_COLUMN, "active", ALW_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* NL-East column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) NLE_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "NL-East", renderer,
                                                           "visible", VISIBLE_COLUMN, "active", NLE_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* NL-Central column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) NLC_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "NL-Central", renderer,
                                                              "visible", VISIBLE_COLUMN, "active", NLC_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

    /* NL-West column */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", (gint *) NLW_COLUMN);
    g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "NL-West", renderer,
                                                           "visible", VISIBLE_COLUMN, "active", NLW_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 75);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
}

