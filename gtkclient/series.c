/* set up to play a series between two teams */

#include "gtknsbc.h"
#include "db.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

/* TreeItem structure */
typedef struct _TreeItem TreeItem;
struct _TreeItem {
    const gchar    *label;
    TreeItem       *children;
};

TreeItem toplevel[200], teams[200][50];
gint toplevelentries;
gchar yearsc[200][13], yearv[5], yearh[5], uteams[5000][50], hometm[100], vistm[100];

/* columns */
enum {
    NAME_COLUMN = 0,

    VISIBLE_COLUMN,
    NUM_COLUMNS
};

GtkWidget *rbox1, *tbox, *visbutton, *homebutton, *dhbuttony, *dhbuttonn, *rbuttony, *rbuttonn, *comboLSERIES;
gint tdh, trand;
gchar statsss[10000];

void
SetUpSeries (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *swin, *box1, *box2, *hbox, *vbox, *vbox2, *vbox3, *label, *separator, *treeview, *scrolled_window;
    GSList *group, *group2;
    GtkLabel *tmlab;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    gint x;
    gchar *msg[5];

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot set up a series while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before setting up a series.";
        outMessage (msg);
        return;
    }

    /* check if user has a series going on the connected server */
    sock_puts (sock, "S0\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "OK")) {
        msg[0] = "There is already a series for you in progress on this server. ";
        msg[1] = "If you continue the current ongoing series will be lost. ";
        msg[2] = "Continue? ";
        if (!ShallWeContinue (msg))
            return;
    }

    /* check if user has a season going on the connected server */
    sock_puts (sock, "S7\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "OK")) {
        gchar *msg[5];
        gint x;

        for (x = 0; x < 5; x ++)
            msg[x] = NULL;

        msg[0] = "A season is currently proceding for you on this server.  There cannot be both a season and a series going at the same time. ";
        msg[1] = "Do you still want to set up a series?  ";
        msg[2] = "(If so, the current season and all stats for the current season will be lost.) ";
        if (!ShallWeContinue (msg))
            return;
    }

    sock_puts (sock, "Q\n");  /* tell the server */

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
        gchar NoTeams[256] = "No groups of teams are available in order to set up a series.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No groups of teams available in order to set up a series on server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoTeams[0];
        outMessage (msg);

        return;
    }

    tdh = trand = 1;
    VisitingTeamID = HomeTeamID = -1;
    PopulateTopLevel2S ();
    PopulateChildren2S ();

    swin = gtk_dialog_new_with_buttons ("Set Up Series", NULL, GTK_DIALOG_MODAL, "OK", 1, "Cancel", 0, NULL);
    gtk_window_set_default_size (GTK_WINDOW (swin), 750, 400);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), box1, TRUE, TRUE, 0);

    box2 = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Select One:", NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (label), FALSE, FALSE, 0);

    vbox = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), vbox, FALSE, FALSE, 0);
    rbuttony = gtk_radio_button_new_with_label (NULL, "Random Teams");
    group2 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (rbuttony));
    rbuttonn = gtk_radio_button_new_with_label (group2, "Manually Select Teams");
    gtk_box_pack_start (GTK_BOX (vbox), rbuttony, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), rbuttonn, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (rbuttony), "clicked", G_CALLBACK (CBrbuty), NULL);
    group2 = gtk_radio_button_get_group (GTK_RADIO_BUTTON (rbuttony));
    g_signal_connect (G_OBJECT (rbuttonn), "clicked", G_CALLBACK (CBrbutn), NULL);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    tbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box2), tbox, TRUE, TRUE, 0);
    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click to the left of a year to bring up the available teams\n\
                          Click on a team to select", NULL);
    gtk_box_pack_start (GTK_BOX (tbox), GTK_WIDGET (tmlab), FALSE, FALSE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (tbox), scrolled_window, TRUE, TRUE, 0);

    /* create model */
    model = create_model2S ();

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    add_columns2S (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed", G_CALLBACK (TeamSelected2S), NULL);

    rbox1 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), rbox1, FALSE, FALSE, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (rbox1), GTK_BUTTONBOX_SPREAD);

    vbox2 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), vbox2, FALSE, FALSE, 0);
    visbutton = gtk_button_new_with_label ("Team #1");
    homebutton = gtk_button_new_with_label ("Team #2");
    gtk_box_pack_start (GTK_BOX (vbox2), visbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), homebutton, TRUE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), hbox, FALSE, FALSE, 0);
    label = gtk_label_new ("Series Length:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    comboLSERIES = gtk_combo_box_new_text ();

    gtk_combo_box_append_text (GTK_COMBO_BOX (comboLSERIES), "random");
    for (x = 1; x < 1000; x += 2)
        gtk_combo_box_append_text (GTK_COMBO_BOX (comboLSERIES), (char *) cnvt_int2str (x, 'l'));
    gtk_combo_box_set_active (GTK_COMBO_BOX (comboLSERIES), 4);
    gtk_container_add (GTK_CONTAINER (hbox), comboLSERIES);
    label = gtk_label_new ("games");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    vbox3 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), vbox3, FALSE, FALSE, 0);
    dhbuttony = gtk_radio_button_new_with_label (NULL, "Use DH");
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dhbuttony));
    dhbuttonn = gtk_radio_button_new_with_label (group, "Do Not Use DH");
    gtk_box_pack_start (GTK_BOX (vbox3), dhbuttony, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox3), dhbuttonn, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhbuttony), "clicked", G_CALLBACK (CBdhbuty), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dhbuttony));
    g_signal_connect (G_OBJECT (dhbuttonn), "clicked", G_CALLBACK (CBdhbutn), NULL);

    gtk_widget_show_all (box1);
    gtk_widget_hide (tbox);

GetTwoTeams:
    if (!gtk_dialog_run (GTK_DIALOG (swin))) {
        sock_puts (sock, "X\n");
        DestroyDialog (swin, swin);
    }
    else {
        gint x;
        gchar twrk[100], SuccessSeries[256] = "Series successfully established on server.", *msg[5];

        for (x = 0; x < 5; x ++)
            msg[x] = NULL;

        if (!trand) {
            if (VisitingTeamID == -1 || HomeTeamID == -1) {
                gchar NoTeams[256] = "Need to select two teams to play.", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &NoTeams[0];
                outMessage (msg);

                goto GetTwoTeams;
            }
            else
                if (!strcmp (&vistm[0], &hometm[0])) {
                    gchar DupTeams[256] = "A team cannot be played against itself in a series.", *msg[5];
                    gint x;

                    for (x = 0; x < 5; x++)
                        msg[x] = NULL;

                    msg[0] = &DupTeams[0];
                    outMessage (msg);

                    goto GetTwoTeams;
                }
        }

        if (tdh) {
            dh = 3;
            buffer[0] = '3';
        }
        else {
            dh = 0;
            buffer[0] = '0';
        }

        strcpy (&twrk[0], gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboLSERIES)));
        if (twrk[0] == 'r')
            strcpy (&buffer[1], "RND ");
        else {
            if (strlen (&twrk[0]) == 1) {
                buffer[3] = twrk[0];
                buffer[1] = buffer[2] = '0';
            }
            if (strlen (&twrk[0]) == 2) {
                buffer[3] = twrk[1];
                buffer[2] = twrk[0];
                buffer[1] = '0';
            }
            if (strlen (&twrk[0]) == 3)
                strcpy (&buffer[1], &twrk[0]);
            buffer[4] = ' ';
        }

        if (trand) {
            buffer[5] = 'R';
            buffer[6] = '\0';
        }
        else {
            strcpy (&buffer[5], &yearv[0]);
            strcpy (&vteamyr[0], &yearv[0]);
            if (vteamyr[0] != '0') {
                for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                    if (VisitingTeamID == teaminfo[x].id)
                        strcat (&buffer[0], &teaminfo[x].filename[0]);
            }
            else
                strcat (&buffer[0], &vistm[0]);

            strcat (&buffer[0], " ");
            strcat (&buffer[0], &yearh[0]);
            strcpy (&hteamyr[0], &yearh[0]);

            if (hteamyr[0] != '0') {
                for (x = 0; x <= NUMBER_OF_TEAMS; x++)
                    if (HomeTeamID == teaminfo[x].id)
                        strcat (&buffer[0], &teaminfo[x].filename[0]);
            }
            else
                strcat (&buffer[0], &hometm[0]);
        }

        strcat (&buffer[0], "\n");
        sock_puts (sock, &buffer[0]);

        strcpy (&work[0], "Establishing a series.\n");
        Add2TextWindow (&work[0], 0);

        SetLeagueUnderWay (0);
        SetSeriesUnderWay (1);
        msg[0] = &SuccessSeries[0];
        outMessage (msg);

        DestroyDialog (swin, swin);
    }
}

void
CBdhbuty (GtkWidget *widget, gpointer *pdata) {
    tdh = 1;
}

void
CBdhbutn (GtkWidget *widget, gpointer *pdata) {
    tdh = 0;
}

void
CBrbuty (GtkWidget *widget, gpointer *pdata) {
    gtk_widget_hide (tbox);
    gtk_button_set_label (GTK_BUTTON (visbutton), "Team #1");
    gtk_button_set_label (GTK_BUTTON (homebutton), "Team #2");
    trand = 1;
}

void
CBrbutn (GtkWidget *widget, gpointer *pdata) {
    gtk_widget_show (tbox);
    trand = 0;
}

void
TeamSelected2S (GtkTreeSelection *selection) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gint sub, row_count, x = 0;
    gchar *teamname, path_str[10] = " ";
    gboolean valid;

    if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
        return;
    gtk_tree_model_get (model, &iter, NAME_COLUMN, &teamname, -1);

    if (strlen (teamname) == 4)
        /* user clicked on a year */
        return;
    if (!strcmp (teamname, "User-Created"))
        /* user clicked User-Created heading */
        return;

    for (sub = row_count = 0; sub < toplevelentries; sub++, row_count = 0) {
        /* walk through the root level of data (years) */
        sprintf (&path_str[0], "%d:0", sub);
        path = gtk_tree_path_new_from_string (&path_str[0]);

        valid = gtk_tree_model_get_iter (model, &iter, path);
        while (valid) {
            /* walk through the sub-level (teams), looking for the selected row */
            if (gtk_tree_selection_iter_is_selected (selection, &iter) == TRUE)
                /* get out of the for() loop */
                goto GetOutFor;

            row_count++;
            valid = gtk_tree_model_iter_next (model, &iter);
        }
    }
GetOutFor:
    /* get team ID */
    if (yearsc[sub][0] != 'U')
        for (x = 0; x <= NUMBER_OF_TEAMS; x++)
            if (!strcmp (&teaminfo[x].teamname[0], teams[sub][row_count].label))
                break;

    if (VisitingTeamID == -1) {
        if (yearsc[sub][0] != 'U') {
            VisitingTeamID = teaminfo[x].id;
            strcpy (&yearv[0], &yearsc[sub][0]);
            strcpy (&vistm[0], &yearv[0]);
            strcat (&vistm[0], &teaminfo[x].filename[0]);
        }
        else {
            VisitingTeamID = 0;
            strcpy (&yearv[0], "0000");
            strcpy (&vistm[0], &uteams[row_count][0]);
        }
        gtk_button_set_label (GTK_BUTTON (visbutton), vistm);
    }
    else
        if (HomeTeamID == -1) {
            if (yearsc[sub][0] != 'U') {
                HomeTeamID = teaminfo[x].id;
                strcpy (&yearh[0], &yearsc[sub][0]);
                strcpy (&hometm[0], &yearh[0]);
                strcat (&hometm[0], &teaminfo[x].filename[0]);
            }
            else {
                HomeTeamID = 0;
                strcpy (&yearh[0], "0000");
                strcpy (&hometm[0], &uteams[row_count][0]);
            }
            gtk_button_set_label (GTK_BUTTON (homebutton), hometm);
        }
        else {
            HomeTeamID = VisitingTeamID;
            strcpy (&yearh[0], &yearv[0]);
            strcpy (&hometm[0], &vistm[0]);
            gtk_button_set_label (GTK_BUTTON (homebutton), gtk_button_get_label (GTK_BUTTON (visbutton)));
            if (yearsc[sub][0] != 'U') {
                VisitingTeamID = teaminfo[x].id;
                strcpy (&yearv[0], &yearsc[sub][0]);
                strcpy (&vistm[0], &yearv[0]);
                strcat (&vistm[0], &teaminfo[x].filename[0]);
            }
            else {
                VisitingTeamID = 0;
                strcpy (&yearv[0], "0000");
                strcpy (&vistm[0], &uteams[row_count][0]);
            }
            gtk_button_set_label (GTK_BUTTON (visbutton), vistm);
        }
    /* unselect row so that it can be selected again right away */
    gtk_tree_selection_unselect_iter (selection, &iter);
    gtk_widget_show_all (rbox1);
}

void
PopulateTopLevel2S () {
    gchar *cc, *work;
    gint x, buflen;

    for (x = 0, cc = &buffer[0], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
        work = (char *) index (cc, ' ');
        if ((work - cc) == 4) {
            /* found a year */
            if (*cc == '0')
                strcpy (&yearsc[x][0], "User-Created");
            else {
                strncpy (&yearsc[x][0], cc, 4);
                yearsc[x][4] = '\0';
            }
            toplevel[x].label = yearsc[x];
            toplevel[x].children = teams[x];
            x++;
        }
        cc = work;
    }
    toplevel[x].label = NULL;
    toplevelentries = x;
}

void
PopulateChildren2S () {
    gchar *cc, *work, nm[50];
    gint buflen, x, y, z, yr = 0, tc;

    for (x = -1, tc = y = 0, cc = &buffer[0], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
        work = (char *) index (cc, ' ');
        if ((work - cc) == 4) {
            if (x >= 0)
                teams[x][y].label = NULL;
            x++;
            y = 0;
            strncpy (&nm[0], cc, 4);
            nm[4] = '\0';
            yr = atoi (&nm[0]);
        }
        else {
            strncpy (&nm[0], cc, (work - cc));
            nm[work - cc] = '\0';
            if (yr) {
                for (z = 0; z <= NUMBER_OF_TEAMS; z++)
                    if (!strcmp (&teaminfo[z].filename[0], &nm[0]))
                        break;
                teams[x][y].label = teaminfo[z].teamname;
            }
            else {
                strcpy (&uteams[tc][0], &nm[0]);
                teams[x][y].label = uteams[tc];
                tc++;
            }
            teams[x][y].children = NULL;
            y++;
        }
        cc = work;
    }
    teams[x][y].label = NULL;
    teams[x + 1][0].label = NULL;
}

GtkTreeModel *
create_model2S () {
    GtkTreeStore *model;
    GtkTreeIter iter;
    TreeItem *years = toplevel;

    /* create tree store */
    model = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN);
    /* add data to the tree store */
    while (years->label) {
        TreeItem *division = years->children;

        gtk_tree_store_append (model, &iter, NULL);
        gtk_tree_store_set (model, &iter, NAME_COLUMN, years->label, VISIBLE_COLUMN, FALSE, -1);
        /* add children */
        while (division->label) {
            GtkTreeIter child_iter;

            gtk_tree_store_append (model, &child_iter, &iter);
            gtk_tree_store_set (model, &child_iter, NAME_COLUMN, division->label, VISIBLE_COLUMN, TRUE, -1);
            division++;
        }
        years++;
    }
    return GTK_TREE_MODEL (model);
}

void
add_columns2S (GtkTreeView *treeview) {
    gint col_offset;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* column for years and team names */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Year->Teams", renderer,
                                                                                           "text", NAME_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
}

void
PlaySeries (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x;
    gchar  NoDir[256] = "You have no series.  First establish a series.", GamesOK[256] = "Completed playing games.",
           EndSeries[256] = "Reached end of series.", *msg[5];
    GtkWidget *dlgFile, *hbox, *label, *spinner;
    GtkAdjustment *adj;


    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (ThreadRunning) {
        msg[0] = "You cannot play games in a series while waiting for a network game challenge. ";
        msg[1] = "Remove your ID from the Waiting Pool via Waiting Pool->Remove Name ";
        msg[2] = "before playing.";
        outMessage (msg);
        return;
    }

    /* check for the presence of a series */
    sock_puts (sock, "S0\n");
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "NOSERIES")) {
        strcpy (&work[0], "No series established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);
        return;
    }

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Number of Games to Play in Series");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Play ");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    adj = (GtkAdjustment *) gtk_adjustment_new (5.0, 1.0, 999.0, 5.0, 100.0, 0.0);
    spinner = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
    gtk_widget_set_size_request (spinner, 55, -1);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_container_add (GTK_CONTAINER (hbox), spinner);

    label = gtk_label_new ("games in your series.");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, 0);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_OK, 1);
    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 1);

    gtk_widget_show_all (hbox);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 2);

    if (gtk_dialog_run (GTK_DIALOG (dlgFile))) {
        gint games, err;

        games = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
        gtk_widget_destroy (dlgFile);

        buffer[0] = 'k';

        if (games < 10) {
            buffer[1] = buffer[2] = '0';
            buffer[3] = games + '0';
        }
        else
            if (games < 100) {
                buffer[1] = '0';
                strcpy (&buffer[2], (char *) cnvt_int2str (games, 'l'));
            }
            else
                strcpy (&buffer[1], (char *) cnvt_int2str (games, 'l'));

        buffer[4] = '\n';
        buffer[5] = '\0';
        sock_puts (sock, &buffer[0]);

        err = 0;
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            GotError ();
            err = 1;
        }
        if (!strcmp (&buffer[0], "ERROR") || !strcmp (&buffer[0], "CANNOT PLAY")) {
            GotError ();
            err = 1;
        }

        if (!err) {
            strcpy (&work[0], "Played games in a series on ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], "\n");
            Add2TextWindow (&work[0], 0);
        }

        if (!strcmp (&buffer[0], "OK") && !err) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            msg[0] = &GamesOK[0];
            outMessage (msg);
        }
        if (!strcmp (&buffer[0], "EOS")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "Completed series.\n");
            Add2TextWindow (&work[0], 0);

            SetSeriesUnderWay (2);
            msg[0] = &EndSeries[0];
            outMessage (msg);
        }
    }
    else
        gtk_widget_destroy (dlgFile);
}

void
SeriesStatus (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *separator, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    sock_puts (sock, "SA\n");  /* we want series status */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }

    if (!strlen (&buffer[0])) {
        /* this happens if the user has no series established */
        gchar NoDir[256] = "You have no series.  First establish a series.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No series established on ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoDir[0];
        outMessage (msg);

        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 650, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Current Series Status");
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

    FillSeriesStatus ();

    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, statsss, strlen (&statsss[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintSeriesStatus), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillSeriesStatus () {
    gint winsi[2], x, seriesleni;
    gchar tyr[2][5], tname[2][100], serieslen[4], wins[2][4], *pnt, *pnt2;

    /* grab info */
    /* buffer[0] = DH indicator ... which we're not using */
    strncpy (&serieslen[0], &buffer[1], 3);
    serieslen[3] = '\0';
    seriesleni = atoi (&serieslen[0]);
    strncpy (&tyr[0][0], &buffer[4], 4);
    tyr[0][4] = '\0';
    pnt = strchr (&buffer[4], ' ');
    *pnt = '\0';
    strcpy (&tname[0][0], &buffer[8]);
    *pnt = ' ';
    pnt++;
    strncpy (&tyr[1][0], pnt, 4);
    tyr[1][4] = '\0';
    pnt += 4;
    pnt2 = strchr (pnt, ' ');
    *pnt2 = '\0';
    strcpy (&tname[1][0], pnt);
    *pnt2 = ' ';
    pnt2++;
    strncpy (&wins[0][0], pnt2, 3);
    strncpy (&wins[1][0], pnt2 + 3, 3);
    wins[0][3] = wins[1][3] = '\0';
    winsi[0] = atoi (&wins[0][0]);
    winsi[1] = atoi (&wins[1][0]);

    /* format info */
    strcpy (&statsss[0], "\n                 Series Status\n\n     ");
    if (tyr[0][0] != '0')
        strcat (&statsss[0], &tyr[0][0]);
    strcat (&statsss[0], " ");
    strcat (&statsss[0], &tname[0][0]);
    strcat (&statsss[0], " versus ");
    if (tyr[1][0] != '0')
        strcat (&statsss[0], &tyr[1][0]);
    strcat (&statsss[0], " ");
    strcat (&statsss[0], &tname[1][0]);
    strcat (&statsss[0], "\n\n               Best of ");
    if (serieslen[0] == '0') {
        x = 1;
        if (serieslen[1] == '0')
            x = 2;
    }
    else
        x = 0;
    strcat (&statsss[0], &serieslen[x]);
    strcat (&statsss[0], " Games\n\n");
    if (!winsi[0] && !winsi[1])
        strcat (&statsss[0], "       No Games Have Been Played Yet.\n\n         ");
    else
        if (winsi[0] == winsi[1]) {
            if (tyr[0][0] != '0')
                strcat (&statsss[0], &tyr[0][0]);
            strcat (&statsss[0], " ");
            strcat (&statsss[0], &tname[0][0]);
            strcat (&statsss[0], " and ");
            if (tyr[1][0] != '0')
                strcat (&statsss[0], &tyr[1][0]);
            strcat (&statsss[0], " ");
            strcat (&statsss[0], &tname[1][0]);
            strcat (&statsss[0], " are tied with ");
            if (winsi[0] < 10)
                x = 2;
            else
                if (winsi[0] < 100)
                    x = 1;
                else
                    x = 0;
            strcat (&statsss[0], &wins[0][x]);
            if (winsi[0] == 1)
                strcat (&statsss[0], " game each\n\n             ");
            else
                strcat (&statsss[0], " games each\n\n             ");
        }
        else
            if (winsi[0] > winsi[1]) {
                if (tyr[0][0] != '0')
                    strcat (&statsss[0], &tyr[0][0]);
                strcat (&statsss[0], " ");
                strcat (&statsss[0], &tname[0][0]);
                if (winsi[0] == seriesleni / 2 + 1)
                    strcat (&statsss[0], " beat ");
                else
                    strcat (&statsss[0], " leading ");
                if (tyr[1][0] != '0')
                    strcat (&statsss[0], &tyr[1][0]);
                strcat (&statsss[0], " ");
                strcat (&statsss[0], &tname[1][0]);
                strcat (&statsss[0], " ");
                if (winsi[0] < 10)
                    x = 2;
                else
                    if (winsi[0] < 100)
                        x = 1;
                    else
                        x = 0;
                strcat (&statsss[0], &wins[0][x]);
                if (winsi[0] == 1)
                    strcat (&statsss[0], " game to ");
                else
                    strcat (&statsss[0], " games to ");
                if (winsi[1] < 10)
                    x = 2;
                else
                    if (winsi[1] < 100)
                        x = 1;
                    else
                        x = 0;
                strcat (&statsss[0], &wins[1][x]);
                strcat (&statsss[0], ".\n\n             ");
            }
            else {
                if (tyr[1][0] != '0')
                    strcat (&statsss[0], &tyr[1][0]);
                strcat (&statsss[0], " ");
                strcat (&statsss[0], &tname[1][0]);
                if (winsi[1] == seriesleni / 2 + 1)
                    strcat (&statsss[0], " beat ");
                else
                    strcat (&statsss[0], " leading ");
                if (tyr[0][0] != '0')
                    strcat (&statsss[0], &tyr[0][0]);
                strcat (&statsss[0], " ");
                strcat (&statsss[0], &tname[0][0]);
                strcat (&statsss[0], " ");
                if (winsi[1] < 10)
                    x = 2;
                else
                    if (winsi[1] < 100)
                        x = 1;
                    else
                        x = 0;
                strcat (&statsss[0], &wins[1][x]);
                if (winsi[1] == 1)
                    strcat (&statsss[0], " game to ");
                else
                    strcat (&statsss[0], " games to ");
                if (winsi[0] < 10)
                    x = 2;
                else
                    if (winsi[0] < 100)
                        x = 1;
                    else
                        x = 0;
                strcat (&statsss[0], &wins[0][x]);
                strcat (&statsss[0], ".\n\n             ");
            }

    x = seriesleni / 2 + 1;
    if (winsi[0] != x && winsi[1] != x) {
        strcat (&statsss[0], (char *) cnvt_int2str (x, 'l'));
        strcat (&statsss[0], " Wins Needed to Win Series.\n");
    }
}

void
PrintSeriesStatus (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Series Status ...", *msg[5];
    gint x;

    sock_puts (sock, "SA\n");  /* we need to get the NSB series status again */

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        GotError ();
        return;
    }
    FillSeriesStatus ();

    print (&statsss[0]);

    strcpy (&work[0], "Print series status.\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

