/* select two teams to play in a game */

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

GtkWidget *rbox1, *visbutton, *homebutton, *dhbuttony, *dhbuttonn;
gint tdh;

void
SelectTeams (char which) {
    GtkWidget *swin, *box1, *box2, *vbox2, *vbox3, *treeview, *scrolled_window, *exbutton;
    GSList *group;
    GtkLabel *tmlab;
    GtkTreeModel *model;
    GtkTreeSelection *selection;

    tdh = 1;
    SelectTeamsCompleted = 0;
    VisitingTeamID = HomeTeamID = -1;
    PopulateTopLevel2 ();
    PopulateChildren2 ();

    swin = gtk_dialog_new_with_buttons ("Select Teams", NULL, GTK_DIALOG_MODAL, "OK", 1, "Cancel", 0, NULL);
    gtk_window_set_default_size (GTK_WINDOW (swin), 750, 400);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), box1, TRUE, TRUE, 0);

    box2 = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click to the left of a year to bring up the available teams\n\
                          Click on a team to select", NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);

    /* create model */
    model = create_model2 ();

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    add_columns2 (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed", G_CALLBACK (TeamSelected2), NULL);

    rbox1 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), rbox1, FALSE, FALSE, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (rbox1), GTK_BUTTONBOX_SPREAD);

    exbutton = gtk_button_new_with_label ("Exchange");
    g_signal_connect (G_OBJECT (exbutton), "clicked", G_CALLBACK (ExchangeTeams), NULL);
    gtk_box_pack_start (GTK_BOX (rbox1), exbutton, TRUE, TRUE, 0);
    vbox2 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), vbox2, FALSE, FALSE, 0);
    visbutton = gtk_button_new_with_label ("Visiting Team");
    homebutton = gtk_button_new_with_label ("Home Team");
    gtk_box_pack_start (GTK_BOX (vbox2), visbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), gtk_label_new ("at"), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), homebutton, TRUE, TRUE, 0);

    vbox3 = gtk_vbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (rbox1), vbox3, FALSE, FALSE, 0);
    dhbuttony = gtk_radio_button_new_with_label (NULL, "Use DH");
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dhbuttony));
    dhbuttonn = gtk_radio_button_new_with_label (group, "Do Not Use DH");
    gtk_box_pack_start (GTK_BOX (vbox3), dhbuttony, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox3), dhbuttonn, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (dhbuttony), "clicked", G_CALLBACK (CBdhbuttony), NULL);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dhbuttony));
    g_signal_connect (G_OBJECT (dhbuttonn), "clicked", G_CALLBACK (CBdhbuttonn), NULL);

    gtk_widget_show_all (box1);

    if (gtk_dialog_run (GTK_DIALOG (swin)))
        TellServerTeams (which);

    DestroyDialog (swin, swin);
}

void
CBdhbuttony (GtkWidget *widget, gpointer *pdata) {
    tdh = 1;
}

void
CBdhbuttonn (GtkWidget *widget, gpointer *pdata) {
    tdh = 0;
}

void
ExchangeTeams (GtkWidget *widget, gpointer *pdata) {
    gchar hlabel[256], tm[100];
    gint h;

    if (VisitingTeamID == -1 || HomeTeamID == -1)
        return;

    strcpy (&hlabel[0], gtk_button_get_label (GTK_BUTTON (visbutton)));
    h = VisitingTeamID;
    strcpy (&tm[0], &vistm[0]);

    VisitingTeamID = HomeTeamID;
    strcpy (&vistm[0], &hometm[0]);
    gtk_button_set_label (GTK_BUTTON (visbutton), gtk_button_get_label (GTK_BUTTON (homebutton)));

    HomeTeamID = h;
    strcpy (&hometm[0], &tm[0]);
    gtk_button_set_label (GTK_BUTTON (homebutton), hlabel);

    strcpy (&work[0], &yearv[0]);
    strcpy (&yearv[0], &yearh[0]);
    strcpy (&yearh[0], &work[0]);

    gtk_widget_show_all (rbox1);
}

void
TeamSelected2 (GtkTreeSelection *selection) {
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
TellServerTeams (char which) {
    gint x;

    if (VisitingTeamID == -1 || HomeTeamID == -1) {
        gchar NoTeams[256] = "Need to select two teams to play.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Two teams were not selected when attempting to watch a game.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoTeams[0];
        outMessage (msg);

        sock_puts (sock, "X\n");
        return;
    }

    if (tdh) {
        dh = 1;
        buffer[0] = '1';
    }
    else {
        dh = 0;
        buffer[0] = '0';
    }

    strcpy (&buffer[1], &yearv[0]);
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

    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    if (watchsw)
        strcpy (&work[0], "Watch 1 non-season game.\n");
    else
        if (which == 'c')
            strcpy (&work[0], "Play against the computer 1 non-season game.\n");
        else
            strcpy (&work[0], "Play against another human 1 non-season game.\n");
    Add2TextWindow (&work[0], 0);

    SelectTeamsCompleted = 1;
}

void
PopulateTopLevel2 () {
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
PopulateChildren2 () {
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
create_model2 () {
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
add_columns2 (GtkTreeView *treeview) {
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

