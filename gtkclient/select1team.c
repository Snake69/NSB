/* provide the user with a way to select 1 team */

#include "gtknsbc.h"
#include "db.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"

/* TreeItem structure */
typedef struct _TreeItems1 TreeItems1;
struct _TreeItems1 {
    const gchar    *label;
    TreeItems1     *children;
};

TreeItems1 toplevels1[200], teams[200][5000];
gint toplevelentries;
gchar yearsc[200][5000], uteams[5000][50];

/* columns */
enum {
    NAME_COLUMN = 0,

    VISIBLE_COLUMN,
    NUM_COLUMNS
};

GtkWidget *swin = NULL;
gchar urind;

void
Select1Team (char turind) {
    GtkWidget *box1, *box2, *button, *treeview, *scrolled_window;
    GtkLabel *tmlab;
    GtkTreeModel *model;
    GtkTreeSelection *selection;

    /* more than one "Select 1 Team" dialog displayed at the same time screws things up */
    if (swin != NULL)
        DestroyDialog (swin, swin);

    urind = turind;

    PopulateTopLevel1 ();
    PopulateChildren1 ();

    swin = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (swin), "Select 1 Team");

    gtk_window_set_default_size (GTK_WINDOW (swin), 750, 400);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), box1, TRUE, TRUE, 0);

    box2 = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
    gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click on the team to select:", NULL);
    gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET (tmlab), TRUE, TRUE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);

    /* create model */
    model = create_model1 ();

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);

    add_columns1 (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    g_signal_connect (selection, "changed", G_CALLBACK (TeamSelected), NULL);

    /* expand the rows if user-created teams, collapse otherwise */
    if (urind == 'U' || urind == 'L' || urind == 'C' || urind == 'S')
        g_signal_connect (treeview, "realize", G_CALLBACK (gtk_tree_view_expand_all), NULL);

    button = gtk_button_new_with_label ("DISMISS");
    gtk_signal_connect (GTK_OBJECT (button), "clicked", G_CALLBACK (DestroySwin), NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->action_area), button, TRUE, TRUE, 0);

    gtk_widget_show_all (swin);
}

void
DestroySwin (GtkWidget *widget, gpointer *pdata) {
    DestroyDialog (swin, swin);
    swin = NULL;
}

void
TeamSelected (GtkTreeSelection *selection) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint sub, row_count, x;
    gchar cid[4], tnw[50], *teamname, path_str[10] = " ";
    gboolean valid;

    if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
        return;

    gtk_tree_model_get (model, &iter, NAME_COLUMN, &teamname, -1);

    if (strlen (teamname) == 4 || !strcmp (teamname, "Current Season") || !strcmp (teamname, "Lifetime Stats") || !strcmp (teamname, "Current Series") ||
                                                                                                                        !strcmp (teamname, "User-Created"))
        /* user clicked on a toplevel line */
        return;

    for (sub = row_count = 0; sub < toplevelentries; sub++, row_count = 0) {
        /* walk through the root level of data (years with Real Life teams) */
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
    if (urind == 'U' || urind == 'L' || urind == 'C' || urind == 'S') {
        strcpy (&tnw[0], teams[sub][row_count].label + 4);
        if (strstr (teams[sub][row_count].label, "-PS"))
            tnw[strlen (&tnw[0]) - 3] = '\0';
    }
    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
        if (urind == 'U' || urind == 'L' || urind == 'C' || urind == 'S') {
            if (!strcmp (&teaminfo[x].filename[0], &tnw[0]))
                break;
        }
        else
            if (!strcmp (&teaminfo[x].teamname[0], teams[sub][row_count].label))
                break;

    if (teaminfo[x].id < 10) {
        cid[0] = '0';
        cid[1] = teaminfo[x].id + '0';
        cid[2] = '\0';
    }
    else
        strcpy (&cid[0], (char *) cnvt_int2str (teaminfo[x].id, 'l'));

    /* send to server */
    strcpy (&buffer[0], "S6");
    if (urind == 'U' || urind == 'L' || urind == 'S') {
        if (strstr (teams[sub][row_count].label, "-PS"))
            if (urind == 'U' || urind == 'S')
                strcat (&buffer[0], "6");
            else
                strcat (&buffer[0], "7");
        else
            if (urind == 'U' || urind == 'S')
                strcat (&buffer[0], "4");
            else
                strcat (&buffer[0], "5");
        x = atoi (&cid[0]);
        if (x && x < 900)
            strncat (&buffer[0], teams[sub][row_count].label, 4);
        else
            strcat (&buffer[0], teams[sub][row_count].label);
    }
    else
        if (urind == 'C')
            strcat (&buffer[0], "8");
        else {
            strcat (&buffer[0], "2");
            strcat (&buffer[0], &yearsc[sub][0]);
        }
    if (urind == 'C') {
        strcat (&buffer[0], teams[sub][row_count].label);
        strcpy (&usercreatedtname[0], teams[sub][row_count].label);
        strcpy (&prtuctm[prtbuttmpnt][0], teams[sub][row_count].label);
    }
    else
        if (x && x < 900)
            strcat (&buffer[0], &cid[0]);
    strcat (&buffer[0], "\n");

    sock_puts (sock, &buffer[0]);
    if (!x || x >= 900) {
        strcpy (&usercreatedtname[0], teams[sub][row_count].label);
        strcpy (&prtuctm[prtbuttmpnt][0], teams[sub][row_count].label);
    }

    strcpy (&prtbuttmcmd[prtbuttmpnt][0], &buffer[0]);
    whichur[prtbuttmpnt] = urind;
    ShowTeamStats ();
}

void
PopulateTopLevel1 () {
    gchar *cc, *work;
    gint x, buflen;

    if (urind == 'U' || urind == 'L' || urind == 'C' || urind == 'S') {
        if (urind == 'S')
            strcpy (&yearsc[0][0], "Current Series");
        else
            if (urind == 'U')
                strcpy (&yearsc[0][0], "Current Season");
            else
                if (urind == 'L')
                    strcpy (&yearsc[0][0], "Lifetime Stats");
                else
                    strcpy (&yearsc[0][0], "User-Created");
        toplevels1[0].label = yearsc[0];
        toplevels1[0].children = teams[0];

        x = 1;
    }
    else
        for (x = 0, cc = &buffer[2], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
            work = (char *) index (cc, ' ');
            if ((work - cc) == 4) {
                /* found a year */
                strncpy (&yearsc[x][0], cc, 4);
                yearsc[x][4] = '\0';
                toplevels1[x].label = yearsc[x];
                toplevels1[x].children = teams[x];
                x++;
            }
            cc = work;
        }
    toplevels1[x].label = NULL;
    toplevelentries = x;
}

void
PopulateChildren1 () {
    gchar *cc, *work, nm[50];
    gint buflen, x, y, z, tc;

    for (x = -1, tc = y = 0, cc = &buffer[2], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
        work = (char *) index (cc, ' ');
        if ((work - cc) == 4) {
            if (x >= 0)
                teams[x][y].label = NULL;
            x++;
            y = 0;
        }
        else {
            strncpy (&nm[0], cc, (work - cc));
            nm[work - cc] = '\0';
            if (urind == 'U' || urind == 'L' || urind == 'C' || urind == 'S') {
                x = 0;
                strcpy (&uteams[tc][0], &nm[0]);
                teams[x][y].label = uteams[tc];
                tc++;
            }
            else {
                for (z = 0; z <= NUMBER_OF_TEAMS; z++)
                    if (!strcmp (&teaminfo[z].filename[0], &nm[0]))
                        break;
                teams[x][y].label = teaminfo[z].teamname;
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
create_model1 () {
    GtkTreeStore *model;
    GtkTreeIter iter;
    TreeItems1 *years = toplevels1;

    /* create tree store */
    model = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN);
    /* add data to the tree store */
    while (years->label) {
        TreeItems1 *division = years->children;

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
add_columns1 (GtkTreeView *treeview) {
    gint col_offset;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* column for years/types and team names */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    if (urind == 'S')
        col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Current Series->Teams", renderer, "text", NAME_COLUMN, NULL);
    else
        if (urind == 'U')
            col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
                                       "Current Season->Teams", renderer, "text", NAME_COLUMN, NULL);
        else
            if (urind == 'L')
                col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
                                                 "Lifetime->Teams", renderer, "text", NAME_COLUMN, NULL);
            else
                if (urind == 'C')
                    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
                                                     "User-Created->Teams", renderer, "text", NAME_COLUMN, NULL);
                else
                    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Year->Teams",
                                                                                       renderer, "text", NAME_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
}

