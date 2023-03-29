
/* create or edit a user-created team  */

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
gint toplevelentries, changesw;
gchar yearsc[200][5000], uteams[5000][50];

/* columns */
enum {
    NAME_COLUMN = 0,

    VISIBLE_COLUMN,
    NUM_COLUMNS
};

enum {
    PNAME = 0,
    HOMERUNS,
    BAVG,
    SAVG,
    OBAVG,
    SB,
    POS,
    COLUMNS
};

enum {
    PITNAME = 0,
    IP,
    ERA,
    SO,
    BB,
    GAMES,
    GSTARTED,
    SAVES,
    COLUMNS3
};

enum {
    UPNAME = 0,
    UTNAME1,
    UYEAR1,
    UHOMERUNS,
    UBAVG,
    USAVG,
    UOBAVG,
    USB,
    UPOS,
    UCOLUMNS1
};

enum {
    UPITNAME = 0,
    UTNAME2,
    UYEAR2,
    UIP,
    UERA,
    USO,
    UBB,
    UGAMES,
    UGSTARTED,
    USAVES,
    UCOLUMNS2
};

GtkListStore *store, *store3, *ustore1, *ustore2;
GtkTreeIter iter, iter3, uiter1, uiter2;
GtkWidget *boxv, *utv1, *utv2;
GtkLabel *rltnamelab;
const gchar *entry_text;
gchar rltname[256];

void
CreateTeam () {
    GtkWidget *swin, *tname, *boxh, *boxh2, *boxv2, *choices, *roster, *treeview, *treeview2, *scrolled_window, *tentry, *table,
              *tlabel, *scrolled_win, *scrolled_win3, *treeview3, *usw1, *usw2, *rembut;
    GtkLabel *tmlab;
    GtkTreeModel *model;
    GtkTreeSelection *selection, *selectionply, *selectionpit;
    gchar utname[256];
    gint response, err = 0, fatalerr = 0, moreerr = 0, poserr = 0, duperr = 0, x;

    rltname[0] = utname[0] = '\0';

    sock_puts (sock, "S63\n");  /* tell the server to send us all Real Life teams available */
    sock_gets (sock, &buffer[0], sizeof (buffer));  /* get teams */

    if (strlen (&buffer[0]) < 10) {
        /* this happens if there are no team stats */
        gchar NoStats[256] = "The Team Statistics do not exist.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "No stats.\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NoStats[0];
        outMessage (msg);

        return;
    }

    PopulateTopLevel3 ();
    PopulateChildren3 ();
    for (x = 0; x < 28; x++)
        team2.batters[x].id.name[0] = '\0';
    for (x = 0; x < 13; x++)
        team2.pitchers[x].id.name[0] = '\0';
    team2.id = team2.year = 0;
    team2.league = team2.division = ' ';
    changesw = 0;

    swin = gtk_dialog_new_with_buttons ("Create Team", NULL, GTK_DIALOG_MODAL, "Check Team", 1, "Cancel", 2, "Save Team", 3, "Load U-C Team", 4, "Rename U-C Team", 6, "Delete U-C Team", 5, "List U-C Teams", 7, NULL);
    gtk_window_set_default_size (GTK_WINDOW (swin), 750, 400);
    gtk_signal_connect (GTK_OBJECT (swin), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    boxv = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (swin)->vbox), boxv, TRUE, TRUE, 0);

    tname = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxv), tname, TRUE, TRUE, 0);

    table = gtk_table_new (2, 2, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 0);
    gtk_table_set_col_spacings (GTK_TABLE (table), 0);
    gtk_box_pack_start (GTK_BOX (boxv), table, FALSE, FALSE, 0);
    tlabel = gtk_label_new_with_mnemonic ("                                                       Your Team Name:");
    gtk_table_attach_defaults (GTK_TABLE (table), tlabel, 0, 1, 0, 1);

    tentry = gtk_entry_new ();
    gtk_signal_connect (GTK_OBJECT (tentry), "insert_text", GTK_SIGNAL_FUNC (CheckTName), NULL);
    if (strlen (&utname[0]))
        gtk_entry_set_text (GTK_ENTRY (tentry), utname);
    gtk_entry_set_max_length (GTK_ENTRY (tentry), 48);
    gtk_table_attach_defaults (GTK_TABLE (table), tentry, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (tlabel), tentry);

    boxh = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxv), boxh, TRUE, TRUE, 0);
    boxv2 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxv), boxv2, TRUE, TRUE, 0);

    treeview2 = gtk_tree_view_new ();
    setup_tree_view (treeview2);
    store = gtk_list_store_new (COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    treeview3 = gtk_tree_view_new ();
    setup_tree_view3 (treeview3);
    store3 = gtk_list_store_new (COLUMNS3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

    utv1 = gtk_tree_view_new ();
    setup_tree_uview1 (utv1);
    ustore1 = gtk_list_store_new (UCOLUMNS1, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    utv2 = gtk_tree_view_new ();
    setup_tree_uview2 (utv2);
    ustore2 = gtk_list_store_new (UCOLUMNS2, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

    /* Add the tree model to the tree view and unreference it so that the model will be destroyed along with the tree view. */
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview2), GTK_TREE_MODEL (store));
    g_object_unref (store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview3), GTK_TREE_MODEL (store3));
    g_object_unref (store3);
    gtk_tree_view_set_model (GTK_TREE_VIEW (utv1), GTK_TREE_MODEL (ustore1));
    g_object_unref (ustore1);
    gtk_tree_view_set_model (GTK_TREE_VIEW (utv2), GTK_TREE_MODEL (ustore2));
    g_object_unref (ustore2);
 
    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    scrolled_win3 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    usw1 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (usw1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    usw2 = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (usw2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    rltnamelab = g_object_new (GTK_TYPE_LABEL, "label", rltname, NULL);
    gtk_box_pack_start (GTK_BOX (boxv2), GTK_WIDGET (rltnamelab), FALSE, FALSE, 0);
    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Players - Offense", NULL);
    gtk_box_pack_start (GTK_BOX (boxv2), GTK_WIDGET (tmlab), FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (scrolled_win), treeview2);
    gtk_container_add (GTK_CONTAINER (boxv2), scrolled_win);
    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Pitchers", NULL);
    gtk_box_pack_start (GTK_BOX (boxv2), GTK_WIDGET (tmlab), FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (scrolled_win3), treeview3);
    gtk_container_add (GTK_CONTAINER (boxv2), scrolled_win3);

    selectionply = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview2));
    gtk_tree_selection_set_mode (selectionply, GTK_SELECTION_SINGLE);
    g_signal_connect (selectionply, "changed", G_CALLBACK (RLPlyPicked), NULL);
    selectionpit = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview3));
    gtk_tree_selection_set_mode (selectionpit, GTK_SELECTION_SINGLE);
    g_signal_connect (selectionpit, "changed", G_CALLBACK (RLPitPicked), NULL);

    choices = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (choices), 10);
    gtk_box_pack_start (GTK_BOX (boxh), choices, TRUE, TRUE, 0);

    roster = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (roster), 10);
    gtk_box_pack_start (GTK_BOX (boxh), roster, TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click to the left of the year to show the teams.", NULL);
    gtk_box_pack_start (GTK_BOX (choices), GTK_WIDGET (tmlab), FALSE, FALSE, 0);
    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Click on the team name to display its roster below.", NULL);
    gtk_box_pack_start (GTK_BOX (choices), GTK_WIDGET (tmlab), FALSE, FALSE, 0);

    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (choices), scrolled_window, TRUE, TRUE, 0);

    tmlab = g_object_new (GTK_TYPE_LABEL, "label", "Your Team Roster", NULL);
    gtk_box_pack_start (GTK_BOX (roster), GTK_WIDGET (tmlab), FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (usw1), utv1);
    gtk_container_add (GTK_CONTAINER (roster), usw1);
    gtk_container_add (GTK_CONTAINER (usw2), utv2);
    gtk_container_add (GTK_CONTAINER (roster), usw2);
    boxh2 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (roster), boxh2, TRUE, TRUE, 0);
    rembut = gtk_button_new_with_label ("Remove");
    g_signal_connect (G_OBJECT (rembut), "clicked", G_CALLBACK (RemovePlayer), NULL);
    gtk_box_pack_start (GTK_BOX (boxh2), rembut, TRUE, FALSE, 0);

    /* create model */
    model = create_model3 ();

    /* create tree view */
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

    add_columns3 (GTK_TREE_VIEW (treeview));

    gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed", G_CALLBACK (TeamPicked), NULL);

    gtk_widget_show_all (boxv);
ContinueCreateTeam:
    response = gtk_dialog_run (GTK_DIALOG (swin));

    if (response == 1 || response == 3) {
        gchar NoTName[256] = "You need to enter a name for your team.\n\n", NoPit[256] = "There are no pitchers on your team.\n\n",
              NoPly[256] = "There are not enough players (at least 9) on your team.\n\n",
              LessPit[256] = "There can be up to 13 pitchers on a team.  There are only    on your team.\n\n",
              LessPly[256] = "There can be up to 28 players on a team.  There are only    on your team.\n\n",
              LessSPit[256], NoCloser[256] = "There is no true closer on your team.\n\n",
              NoPos[256] = "There is no one on your team who played ", Dup[256],
              OnePos[256] = "There is only one player on your team who can play ",
              FourPos[256] = "The team name cannot be exactly 4 positions long.  Crazy, huh?\n\n", *msg[5];
        gint x, y, numpit, pos[10];

        fatalerr = err = moreerr = 0;
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        entry_text = gtk_entry_get_text (GTK_ENTRY (tentry));
        strcpy (&utname[0], entry_text);

        if (!strlen (&utname[0])) {
            msg[fatalerr] = NoTName;
            fatalerr++;
        }
        if (strlen (&utname[0]) == 4) {
            msg[fatalerr] = FourPos;
            fatalerr++;
        }
        if (team2.pitchers[0].id.name[0] == ' ' || !strlen (&team2.pitchers[0].id.name[0])) {
            msg[fatalerr] = NoPit;
            fatalerr++;
        }
        if (team2.batters[8].id.name[0] == ' ' || !strlen (&team2.batters[8].id.name[0])) {
            msg[fatalerr] = NoPly;
            fatalerr++;
        }
        if (fatalerr)
            outMessage (msg);

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        for (x = 0; x < 13; x++)
            if (team2.pitchers[x].id.name[0] == ' ' || !strlen (&team2.pitchers[x].id.name[0]))
                break;
        if (x < 13) {
            if (x > 9)
                LessPit[58] = '1';
            LessPit[59] = (x % 10) + '0';
            msg[err] = LessPit;
            err++;
        }
        for (numpit = x = 0; x < 13; x++)
            if (team2.pitchers[x].pitching.games_started && (team2.pitchers[x].id.name[0] != ' ' && strlen (&team2.pitchers[x].id.name[0])))
                numpit++;
        if (numpit < 5) {
            strcpy (&LessSPit[0], "There ");
            if (numpit == 1)
                strcat (&LessSPit[0], "is only ");
            else
                if (!numpit)
                    strcat (&LessSPit[0], "are no ");
                else
                    strcat (&LessSPit[0], "are only ");
            if (numpit == 1) {
                LessSPit[14] = numpit + '0';
                LessSPit[15] = '\0';
            }
            else
                if (numpit) {
                    LessSPit[15] = numpit + '0';
                    LessSPit[16] = '\0';
                }
            if (numpit == 1)
                strcat (&LessSPit[0], " pitcher on your team who has ");
            else
                strcat (&LessSPit[0], " pitchers on your team who have ");
            strcat (&LessSPit[0], "started games.  There should be at least 5.\n\n");
            msg[err] = LessSPit;
            err++;
        }
        for (x = 0; x < 13; x++)
            if (team2.pitchers[x].pitching.saves > 10 && (team2.pitchers[x].id.name[0] != ' ' && strlen (&team2.pitchers[x].id.name[0])))
                break;
        if (x == 13) {
            msg[err] = NoCloser;
            err++;
        }
        for (x = 0; x < 28; x++)
            if (team2.batters[x].id.name[0] == ' ' || !strlen (&team2.batters[x].id.name[0]))
                break;
        if (x < 28) {
            if (x > 9)
                LessPly[57] = (x / 10) + '0';
            LessPly[58] = (x % 10) + '0';
            msg[err] = LessPly;
            err++;
        }
        if (err)
            outMessage (msg);

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        for (x = 0; x < 10; x++)
            pos[x] = 0;
        for (x = 0; x < 28; x++)
            for (y = 0; y < 10; y++)
                if (team2.batters[x].fielding[y].games > 0)
                    pos[y]++;
        for (poserr = x = 0; x < 10; x++)
            if (!pos[x]) {
                if (poserr)
                    strcat (&NoPos[0], ", ");
                strcat (&NoPos[0], figure_pos (x));
                poserr = 1;
            }
        if (poserr) {
            strcat (&NoPos[0], " in real life.\n\n");
            msg[moreerr] = NoPos;
            moreerr++;
        }
        for (poserr = x = 0; x < 10; x++)
            if (pos[x] == 1) {
                if (poserr)
                    strcat (&OnePos[0], ", ");
                strcat (&OnePos[0], figure_pos (x));
                poserr = 1;
            }
        if (poserr) {
            strcat (&OnePos[0], ".\n\n");
            msg[moreerr] = OnePos;
            moreerr++;
        }

        for (Dup[0] = '\0', duperr = x = 0; x < 27; x++)
            for (y = x + 1; y < 28; y++)
                if (!strcmp (&team2.batters[x].id.name[0], &team2.batters[y].id.name[0]) && team2.batters[x].id.year == team2.batters[y].id.year &&
                                (team2.batters[x].id.name[0] != ' ' && strlen (&team2.batters[x].id.name[0]))) {
                    if (duperr)
                        strcat (&Dup[0], ", ");
                    strcat (&Dup[0], &team2.batters[x].id.name[0]);
                    strcat (&Dup[0], " (");
                    strcat (&Dup[0], (char *) cnvt_int2str (team2.batters[x].id.year, 'l'));
                    strcat (&Dup[0], ")");
                    duperr++;
                    break;
                }
        if (duperr) {
            strcat (&Dup[0], " appear");
            if (duperr == 1)
                strcat (&Dup[0], "s");
            strcat (&Dup[0], " more than once on your team.\n\n");
            msg[moreerr] = Dup;
            moreerr++;
        }

        if (moreerr)
            outMessage (msg);
    }

    if (response == 1) {
        gchar OK[256] = "Everything seems copacetic.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;
        if (!fatalerr && !err && !moreerr) {
            msg[0] = OK;
            outMessage (msg);
        }
        goto ContinueCreateTeam;
    }

    if (response == 3 && fatalerr)
        goto ContinueCreateTeam;

    if (response == 3 && (err || moreerr)) {
        gint z;
        gchar *msg[5];

        for (z = 0; z < 5; z++)
            msg[z] = NULL;

        msg[0] = "In spite of the warning(s) do you want to continue and save your team?\n\n";
        if (!ShallWeContinue (msg))
            goto ContinueCreateTeam;
    }

    /* save user's team */
    if (response == 3) {
        gchar Dup[256] = "The Team Name already exists.  Do you wish to overwrite the team?\n\n",
              Succ[256] = "Your team was successfully saved.\n\n",
              Error[256] = "There is a problem with the NSB server.\n\n", *msg[5];
        gint x;

        strcpy (&buffer[0], "E");
        strcat (&buffer[0], &utname[0]);
        strcat (&buffer[0], "\n");
        sock_puts (sock, &buffer[0]);  /* send team name */
        send_stats (sock);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "DUP")) {
            /* this happens if the team name already exists on the server */
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            msg[0] = &Dup[0];
            if (!ShallWeContinue (msg)) {
                sock_puts (sock, "CANCEL\n");
                goto ContinueCreateTeam;
            }
            sock_puts (sock, "OK\n");
            sock_gets (sock, &buffer[0], sizeof (buffer));
        }
        if (!strcmp (&buffer[0], "ERR")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            msg[0] = &Error[0];
            outMessage (msg);

            goto ContinueCreateTeam;
        }
        else {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            msg[0] = &Succ[0];
            outMessage (msg);
        }
    }

    /* load a user team */
    if (response == 4) {
        if ((strlen (&team2.batters[0].id.name[0]) && team2.batters[0].id.name[0] != ' ') || (strlen (&team2.pitchers[0].id.name[0]) && team2.pitchers[0].id.name[0] != ' ')) {
            gchar AD[256] = "Current data will be overwritten and lost.  Do you wish to continue?\n\n", *msg[5];
            gint x;

            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            msg[0] = &AD[0];
            if (!ShallWeContinue (msg))
                goto ContinueCreateTeam;
        }
        /* get a team/file name */
        if (GetTeamFileName (1)) {
            gchar NoTeams[256] = "You have no user-created teams on the currently connected server.\n\n",
                  NoMatch[256] = "There is no match to the team/file name you entered.  The available team/file names are:\n\n",
                  *msg[5];
            gint x;

            strcpy (&buffer[0], "F");
            strcat (&buffer[0], &tfname[0]);
            strcat (&buffer[0], "\n");
            sock_puts (sock, &buffer[0]);  /* send team/file name */
            sock_gets (sock, &buffer[0], sizeof (buffer));
            if (!strcmp (&buffer[0], "NONE")) {
                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &NoTeams[0];
                outMessage (msg);

                goto ContinueCreateTeam;
            }
            if (!strncmp (&buffer[0], "NOMATCH", 7)) {
                gchar fname[256], *fnpnt;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                if (strlen (&buffer[0]) == 7) {
                    msg[0] = &NoTeams[0];
                    outMessage (msg);

                    goto ContinueCreateTeam;
                }

                /* add available team/file names to message */
                for (fnpnt = &buffer[7], x = 0; fnpnt < (&buffer[0] + strlen (&buffer[0])); fnpnt++, x++)
                    if (*fnpnt != ' ')
                        fname[x] = *fnpnt;
                    else {
                        fname[x] = '\0';
                        x = -1;
                        strcat (&NoMatch[0], &fname[0]);
                        strcat (&NoMatch[0], "\n");
                    }

                msg[0] = &NoMatch[0];
                outMessage (msg);

                goto ContinueCreateTeam;
            }
            if (!strncmp (&buffer[0], "OK", 2)) {
                gint ply, pit, singles, sf, y;
                gchar pos[200], bavg[10], savg[10], oba[10], tname[100];

                strcpy (&utname[0], &buffer[2]);
                gtk_entry_set_text (GTK_ENTRY (tentry), utname);
                gtk_widget_show_all (tentry);

                get_stats (sock, 'u', 0);

                /* fill in data store and display */
                for (ply = 0; team2.batters[ply].id.name[0] != ' ' && strlen (&team2.batters[ply].id.name[0]) && ply < 28; ply++) {
                    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore1), &uiter1, NULL, ply);

                    strcpy (&tname[0], (char *) GetTeamName (team2.batters[ply].id.teamid));
                    singles = team2.batters[ply].hitting.hits - (team2.batters[ply].hitting.homers + team2.batters[ply].hitting.triples + team2.batters[ply].hitting.doubles);
                    if (team2.batters[ply].hitting.sf == -1)
                        sf = 0;
                    else
                        sf = team2.batters[ply].hitting.sf;
                    for (pos[0] = '\0', y = 0; y < 13; y++)
                        if (team2.batters[ply].fielding[y].games > 0) {
                            if (strlen (&pos[0]))
                                strcat (&pos[0], ", ");
                            if (y == 10) {
                                /* don't show OF if there are games in LF, CF or RF */
                                if (team2.batters[ply].fielding[7].games <= 0 && team2.batters[ply].fielding[8].games <= 0 && team2.batters[ply].fielding[9].games <= 0)
                                    strcat (&pos[0], "OF");
                                else {
                                    /* remove the last comma */
                                    pos[strlen (&pos[0]) - 2] = '\0';
                                    continue;
                                }
                            }
                            else
                                strcat (&pos[0], figure_pos (y));
                            strcat (&pos[0], "-");
                            strcat (&pos[0], (char *) check_stats (team2.batters[ply].fielding[y].games, 'l'));
                        }
                    strcpy (&bavg[0], (char *) do_average (team2.batters[ply].hitting.hits, team2.batters[ply].hitting.atbats));
                    strcpy (&savg[0], (char *) do_average (((team2.batters[ply].hitting.homers * 4) + (team2.batters[ply].hitting.triples * 3) +
                                                           (team2.batters[ply].hitting.doubles * 2) + singles), team2.batters[ply].hitting.atbats));
                    strcpy (&oba[0], (char *) do_average ((team2.batters[ply].hitting.hits + team2.batters[ply].hitting.bb + team2.batters[ply].hitting.hbp),
                                                  (team2.batters[ply].hitting.atbats + team2.batters[ply].hitting.bb + sf + team2.batters[ply].hitting.hbp)));

                    gtk_list_store_append (ustore1, &uiter1);
                    gtk_list_store_set (ustore1, &uiter1, UPNAME, team2.batters[ply].id.name, UTNAME1, tname, UYEAR1, team2.batters[ply].id.year, UHOMERUNS, team2.batters[ply].hitting.homers,
                                        UBAVG, bavg, USAVG, savg, UOBAVG, oba, USB, team2.batters[ply].hitting.sb, UPOS, pos, -1);
                }

                for (pit = 0; team2.pitchers[pit].id.name[0] != ' ' && strlen (&team2.pitchers[pit].id.name[0]) && pit < 13; pit++) {
                    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore2), &uiter2, NULL, pit);

                    strcpy (&tname[0], (char *) GetTeamName (team2.pitchers[pit].id.teamid));
                    gtk_list_store_append (ustore2, &uiter2);
                    gtk_list_store_set (ustore2, &uiter2, UPITNAME, team2.pitchers[pit].id.name, UTNAME2, tname, UYEAR2, team2.pitchers[pit].id.year, UIP, team2.pitchers[pit].pitching.innings,
                                 UERA, (char *) do_era (team2.pitchers[pit].pitching.er * 9, team2.pitchers[pit].pitching.innings, team2.pitchers[pit].pitching.thirds),
                                        USO, team2.pitchers[pit].pitching.so, UBB, team2.pitchers[pit].pitching.walks,
                                UGAMES, team2.pitchers[pit].pitching.games, UGSTARTED, team2.pitchers[pit].pitching.games_started, USAVES, team2.pitchers[pit].pitching.saves, -1);
                }
            }

            changesw = 0;
        }

        goto ContinueCreateTeam;
    }

    /* delete a user team */
    if (response == 5) {
        /* get a team/file name */
        if (GetTeamFileName (2)) {
            gchar NoTeams[256] = "You have no user-created teams on the currently connected server.\n\n",
                  NoMatch[256] = "There is no match to the team/file name you entered.  The available team/file names are:\n\n",
                  *msg[5];
            gint x;

            strcpy (&buffer[0], "G");
            strcat (&buffer[0], &tfname[0]);
            strcat (&buffer[0], "\n");
            sock_puts (sock, &buffer[0]);  /* send team/file name */
            sock_gets (sock, &buffer[0], sizeof (buffer));
            if (!strcmp (&buffer[0], "NONE")) {
                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &NoTeams[0];
                outMessage (msg);

                goto ContinueCreateTeam;
            }
            if (!strncmp (&buffer[0], "NOMATCH", 7)) {
                gchar fname[256], *fnpnt;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                if (strlen (&buffer[0]) == 7) {
                    msg[0] = &NoTeams[0];
                    outMessage (msg);

                    goto ContinueCreateTeam;
                }

                /* add available team/file names to message */
                for (fnpnt = &buffer[7], x = 0; fnpnt < (&buffer[0] + strlen (&buffer[0])); fnpnt++, x++)
                    if (*fnpnt != ' ')
                        fname[x] = *fnpnt;
                    else {
                        fname[x] = '\0';
                        x = -1;
                        strcat (&NoMatch[0], &fname[0]);
                        strcat (&NoMatch[0], "\n");
                    }

                msg[0] = &NoMatch[0];
                outMessage (msg);
            }
            if (!strncmp (&buffer[0], "MATCH", 5)) {
                gchar RD[256] = "Team found.  If deleted it will be gone forever.  Do you really wish to delete the team?\n\n",
                      *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &RD[0];
                if (!ShallWeContinue (msg))
                    sock_puts (sock, "FORGETIT\n");
                else {
                    gchar TD[256] = "Team deleted.\n\n", SW[256] = "Something went wrong.  Team NOT deleted.", *msg[5];

                    for (x = 0; x < 5; x++)
                        msg[x] = NULL;

                    sock_puts (sock, "DOIT\n");  /* send verification */
                    sock_gets (sock, &buffer[0], sizeof (buffer));
                    if (!strncmp (&buffer[0], "OK", 2))
                        msg[0] = &TD[0];
                    else
                        msg[0] = &SW[0];

                    outMessage (msg);
                }
            }
        }

        goto ContinueCreateTeam;
    }

    /* rename a user team */
    if (response == 6) {
        /* get names */
        if (GetTeamFileName (3)) {
            gchar NoTeams[256] = "You have no user-created teams on the currently connected server.\n\n", NoMatch[256] = "There is no match to the team/file name you entered.  The available team/file names are:\n\n",
                  *msg[5];
            gint x;

            strcpy (&buffer[0], "g");
            strcat (&buffer[0], &tfname[0]);
            strcat (&buffer[0], " ");
            strcat (&buffer[0], &tfnewname[0]);
            strcat (&buffer[0], "\n");
            sock_puts (sock, &buffer[0]);  /* send names */
            sock_gets (sock, &buffer[0], sizeof (buffer));
            if (!strcmp (&buffer[0], "NONE")) {
                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &NoTeams[0];
                outMessage (msg);

                goto ContinueCreateTeam;
            }
            if (!strncmp (&buffer[0], "NOMATCH", 7)) {
                gchar fname[256], *fnpnt;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                if (strlen (&buffer[0]) == 7) {
                    msg[0] = &NoTeams[0];
                    outMessage (msg);

                    goto ContinueCreateTeam;
                }

                /* add available team/file names to message */
                for (fnpnt = &buffer[7], x = 0; fnpnt < (&buffer[0] + strlen (&buffer[0])); fnpnt++, x++)
                    if (*fnpnt != ' ')
                        fname[x] = *fnpnt;
                    else {
                        fname[x] = '\0';
                        x = -1;
                        strcat (&NoMatch[0], &fname[0]);
                        strcat (&NoMatch[0], "\n");
                    }

                msg[0] = &NoMatch[0];
                outMessage (msg);
            }
            if (!strncmp (&buffer[0], "MATCH", 5)) {
                gchar TR[256] = "Team renamed.\n\n", *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;

                msg[0] = &TR[0];
                outMessage (msg);
            }
        }

        goto ContinueCreateTeam;
    }

    /* list existing user teams */
    if (response == 7) {
        gchar NoTeams[256] = "You have no user-created teams on the currently connected server.\n\n", List[2048] = "Teams you've created:\n\n", fname[256], *fnpnt, *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&buffer[0], "u");
        strcat (&buffer[0], "\n");

        sock_puts (sock, &buffer[0]);  /* send command */
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "NONE") || strlen (&buffer[0]) < 2) {
            msg[0] = &NoTeams[0];
            outMessage (msg);

            goto ContinueCreateTeam;
        }

        /* add available team/file names to message */
        for (fnpnt = &buffer[0], x = 0; fnpnt < (&buffer[0] + strlen (&buffer[0])); fnpnt++, x++)
            if (*fnpnt != ' ')
                fname[x] = *fnpnt;
            else {
                fname[x] = '\0';
                x = -1;
                strcat (&List[0], &fname[0]);
                strcat (&List[0], "\n");
            }

        msg[0] = &List[0];
        outMessage (msg);
    }

    if (response == 2 && changesw) {
        gchar Lost[256] = "Current data will be lost.  Do you wish to continue without saving?\n\n", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &Lost[0];
        if (!ShallWeContinue (msg))
            goto ContinueCreateTeam;
    }

    DestroyDialog (swin, swin);
}

void
CheckTName (GtkEntry *entry, const gchar *text, gint length, gint *position, gpointer data) {
    GtkEditable *editable = GTK_EDITABLE (entry);
    int i, count = 0;
    gchar *result = g_new (gchar, length);

    for (i = 0; i < length; i++) {
        if (!isalpha (text[i]) && !isdigit (text[i]))
            continue;
        result[count++] = text[i];
    }

    if (count > 0) {
        gtk_signal_handler_block_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckTName), data);
        gtk_editable_insert_text (editable, result, count, position);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable), GTK_SIGNAL_FUNC (CheckTName), data);
    }
    gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");

    g_free (result);
}

void
TeamPicked (GtkTreeSelection *selection) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint sub, row_count, x, i;
    gchar cid[3], *teamname, path_str[10] = " ";
    gboolean valid;

    if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
        return;
    gtk_tree_model_get (model, &iter, NAME_COLUMN, &teamname, -1);

    if (strlen (teamname) == 4)
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
    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
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
    strcat (&buffer[0], "2");
    strcat (&buffer[0], &yearsc[sub][0]);
    strcat (&buffer[0], &cid[0]);
    strcat (&buffer[0], "\n");

    sock_puts (sock, &buffer[0]);

    if (get_stats (sock, 't', 0) == -1)
        return;

    /* display stats */
    strcpy (&rltname[0], &yearsc[sub][0]);
    strcat (&rltname[0], " ");
    strcat (&rltname[0], &teaminfo[x].teamname[0]);
    gtk_label_set_label (rltnamelab, rltname);

    gtk_list_store_clear (store);
    i = 0;
    while (team.batters[i].id.name[0] != ' ' && strlen (&team.batters[i].id.name[0])) {
        gint singles, sf, y;
        gchar pos[200], bavg[10], savg[10], oba[10];

        singles = team.batters[i].hitting.hits - (team.batters[i].hitting.homers + team.batters[i].hitting.triples + team.batters[i].hitting.doubles);
        if (team.batters[i].hitting.sf == -1)
            sf = 0;
        else
            sf = team.batters[i].hitting.sf;
        for (pos[0] = '\0', y = 0; y < 11; y++)
            if (team.batters[i].fielding[y].games > 0) {
                if (strlen (&pos[0]))
                    strcat (&pos[0], ", ");
                if (y == 10) {
                    /* don't show OF if there are games in LF, CF or RF */
                    if (team2.batters[i].fielding[7].games <= 0 && team2.batters[i].fielding[8].games <= 0 && team2.batters[i].fielding[9].games <= 0)
                        strcat (&pos[0], "OF");
                    else {
                        /* remove the last comma */
                        pos[strlen (&pos[0]) - 2] = '\0';
                        continue;
                    }
                }
                else
                    strcat (&pos[0], figure_pos (y));
                strcat (&pos[0], "-");
                strcat (&pos[0], (char *) check_stats (team.batters[i].fielding[y].games, 'l'));
            }
        strcpy (&bavg[0], (char *) do_average (team.batters[i].hitting.hits, team.batters[i].hitting.atbats));
        strcpy (&savg[0], (char *) do_average (((team.batters[i].hitting.homers * 4) + (team.batters[i].hitting.triples * 3) + (team.batters[i].hitting.doubles * 2) + singles), team.batters[i].hitting.atbats));
        strcpy (&oba[0], (char *) do_average ((team.batters[i].hitting.hits + team.batters[i].hitting.bb + team.batters[i].hitting.hbp),
                             (team.batters[i].hitting.atbats + team.batters[i].hitting.bb + sf + team.batters[i].hitting.hbp)));

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, PNAME, team.batters[i].id.name, HOMERUNS, team.batters[i].hitting.homers, BAVG, bavg, SAVG, savg, OBAVG, oba, SB, team.batters[i].hitting.sb, POS, pos, -1);
        i++;
        if (i == 28)
            break;
    }

    i = 0;
    gtk_list_store_clear (store3);
    while (team.pitchers[i].id.name[0] != ' ' && strlen (&team.pitchers[i].id.name[0])) {
        gtk_list_store_append (store3, &iter3);
        gtk_list_store_set (store3, &iter3, PITNAME, team.pitchers[i].id.name, IP, team.pitchers[i].pitching.innings,
                            ERA, (char *) do_era (team.pitchers[i].pitching.er * 9, team.pitchers[i].pitching.innings, team.pitchers[i].pitching.thirds),
                            SO, team.pitchers[i].pitching.so, BB, team.pitchers[i].pitching.walks, GAMES, team.pitchers[i].pitching.games, GSTARTED,
                            team.pitchers[i].pitching.games_started, SAVES, team.pitchers[i].pitching.saves, -1);
        i++;
        if (i == 13)
            break;
    }
}

void
RLPlyPicked (GtkTreeSelection *selection) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint *i, singles, sf, y, ply, pit = 0, z, pitstats = 0;
    gchar pos[200], bavg[10], savg[10], oba[10], name[100];

    if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
        return;

    path = gtk_tree_model_get_path (model, &iter) ;
    i = gtk_tree_path_get_indices (path);

    for (ply = 0; ply < 28; ply++)
        if (team2.batters[ply].id.name[0] == ' ' || !strlen (&team2.batters[ply].id.name[0]))
            break;
    if (ply == 28) {
        gchar NoRoom[256] = "The roster of the team being created is full.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &NoRoom[0];
        outMessage (msg);

        return;
    }
    for (z = 0; z < 13; z++)
        if (!strcmp (&team.batters[i[0]].id.name[0], &team.pitchers[z].id.name[0])) {
            for (pit = 0; pit < 13; pit++)
                if (team2.pitchers[pit].id.name[0] == ' ' || !strlen (&team2.pitchers[pit].id.name[0])) {
                    team2.pitchers[pit] = team.pitchers[z];
                    pitstats = 1;
                    break;
                }
            if (pitstats)
                break;
            if (pit == 13) {
                gchar NoRoom[256] = "The player you chose has pitching stats.  There is no more room for pitchers on your team.",
                      *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;
 
                msg[0] = &NoRoom[0];
                outMessage (msg);

                return;
            }
        }
    if (z == 13)
        pitstats = 0;
    team2.batters[ply] = team.batters[i[0]];
    changesw = 1;

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore1), &uiter1, NULL, ply);

    strcpy (&name[0], &rltname[5]);
    singles = team2.batters[ply].hitting.hits - (team2.batters[ply].hitting.homers +
              team2.batters[ply].hitting.triples + team2.batters[ply].hitting.doubles);
    if (team2.batters[ply].hitting.sf == -1)
        sf = 0;
    else
        sf = team2.batters[ply].hitting.sf;
    for (pos[0] = '\0', y = 0; y < 11; y++)
        if (team2.batters[ply].fielding[y].games > 0) {
            if (strlen (&pos[0]))
                strcat (&pos[0], ", ");
            if (y == 10) {
                /* don't show OF if there are games in LF, CF or RF */
                if (team2.batters[ply].fielding[7].games <= 0 && team2.batters[ply].fielding[8].games <= 0 && team2.batters[ply].fielding[9].games <= 0)
                    strcat (&pos[0], "OF");
                else {
                    /* remove the last comma */
                    pos[strlen (&pos[0]) - 2] = '\0';
                    continue;
                }
            }
            else
                strcat (&pos[0], figure_pos (y));
            strcat (&pos[0], "-");
            strcat (&pos[0], (char *) check_stats (team2.batters[ply].fielding[y].games, 'l'));
        }
    strcpy (&bavg[0], (char *) do_average (team2.batters[ply].hitting.hits, team2.batters[ply].hitting.atbats));
    strcpy (&savg[0], (char *) do_average (((team2.batters[ply].hitting.homers * 4) + (team2.batters[ply].hitting.triples * 3) +
                                 (team2.batters[ply].hitting.doubles * 2) + singles), team2.batters[ply].hitting.atbats));
    strcpy (&oba[0], (char *) do_average ((team2.batters[ply].hitting.hits + team2.batters[ply].hitting.bb + team2.batters[ply].hitting.hbp),
                      (team2.batters[ply].hitting.atbats + team2.batters[ply].hitting.bb + sf + team2.batters[ply].hitting.hbp)));

    gtk_list_store_append (ustore1, &uiter1);
    gtk_list_store_set (ustore1, &uiter1, UPNAME, team2.batters[ply].id.name, UTNAME1, name, UYEAR1, team2.batters[ply].id.year, UHOMERUNS, team2.batters[ply].hitting.homers, UBAVG, bavg, USAVG, savg,
                        UOBAVG, oba, USB, team2.batters[ply].hitting.sb, UPOS, pos, -1);

    if (pitstats) {
        strcpy (&name[0], &rltname[5]);

        gtk_list_store_append (ustore2, &uiter2);
        gtk_list_store_set (ustore2, &uiter2, UPITNAME, team2.pitchers[pit].id.name, UTNAME2, name, UYEAR2, team2.pitchers[pit].id.year, UIP, team2.pitchers[pit].pitching.innings,
                            UERA, (char *) do_era (team2.pitchers[pit].pitching.er * 9, team2.pitchers[pit].pitching.innings, team2.pitchers[pit].pitching.thirds),
                            USO, team2.pitchers[pit].pitching.so, UBB, team2.pitchers[pit].pitching.walks, UGAMES, team2.pitchers[pit].pitching.games, UGSTARTED, team2.pitchers[pit].pitching.games_started,
                            USAVES, team2.pitchers[pit].pitching.saves, -1);
    }
}

void
RLPitPicked (GtkTreeSelection *selection) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint *i, singles, sf, y, ply = 0, pit = 0, z, plystats = 0;
    gchar pos[200], bavg[10], savg[10], oba[10], name[100];

    if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
        return;

    path = gtk_tree_model_get_path (model, &iter) ;
    i = gtk_tree_path_get_indices (path);

    for (pit = 0; pit < 13; pit++)
        if (team2.pitchers[pit].id.name[0] == ' ' || !strlen (&team2.pitchers[pit].id.name[0]))
            break;
    if (pit == 13) {
        gchar NoRoom[256] = "There is no more room for pitchers on your roster.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        msg[0] = &NoRoom[0];
        outMessage (msg);

        return;
    }
    for (z = 0; z < 28; z++)
        if (!strcmp (&team.pitchers[i[0]].id.name[0], &team.batters[z].id.name[0])) {
            for (ply = 0; ply < 28; ply++)
                if (team2.batters[ply].id.name[0] == ' ' || !strlen (&team2.batters[ply].id.name[0])) {
                    team2.batters[ply] = team.batters[z];
                    plystats = 1;
                    break;
                }
            if (plystats)
                break;
            if (ply == 28) {
                gchar NoRoom[256] = "There is no more room for players on your team.  The roster is full.",
                      *msg[5];
                gint x;

                for (x = 0; x < 5; x++)
                    msg[x] = NULL;
 
                msg[0] = &NoRoom[0];
                outMessage (msg);

                return;
            }
        }
    if (z == 28)
        plystats = 0;
    team2.pitchers[pit] = team.pitchers[i[0]];
    changesw = 1;

    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore2), &uiter2, NULL, pit);

    strcpy (&name[0], &rltname[5]);

    gtk_list_store_append (ustore2, &uiter2);
    gtk_list_store_set (ustore2, &uiter2, UPITNAME, team2.pitchers[pit].id.name, UTNAME2, name, UYEAR2, team2.pitchers[pit].id.year, UIP, team2.pitchers[pit].pitching.innings,
                        UERA, (char *) do_era (team2.pitchers[pit].pitching.er * 9, team2.pitchers[pit].pitching.innings, team2.pitchers[pit].pitching.thirds),
                        USO, team2.pitchers[pit].pitching.so, UBB, team2.pitchers[pit].pitching.walks, UGAMES, team2.pitchers[pit].pitching.games, UGSTARTED, team2.pitchers[pit].pitching.games_started,
                        USAVES, team2.pitchers[pit].pitching.saves, -1);

    if (plystats) {
        strcpy (&name[0], &rltname[5]);

        singles = team2.batters[ply].hitting.hits - (team2.batters[ply].hitting.homers + team2.batters[ply].hitting.triples + team2.batters[ply].hitting.doubles);
        if (team2.batters[ply].hitting.sf == -1)
            sf = 0;
        else
            sf = team2.batters[ply].hitting.sf;
        for (pos[0] = '\0', y = 0; y < 11; y++)
            if (team2.batters[ply].fielding[y].games > 0) {
                if (strlen (&pos[0]))
                    strcat (&pos[0], ", ");
                if (y == 10) {
                    /* don't show OF if there are games in LF, CF or RF */
                    if (team2.batters[ply].fielding[7].games <= 0 && team2.batters[ply].fielding[8].games <= 0 && team2.batters[ply].fielding[9].games <= 0)
                        strcat (&pos[0], "OF");
                    else {
                        /* remove the last comma */
                        pos[strlen (&pos[0]) - 2] = '\0';
                        continue;
                    }
                }
                else
                    strcat (&pos[0], figure_pos (y));
                strcat (&pos[0], "-");
                strcat (&pos[0], (char *) check_stats (team2.batters[ply].fielding[y].games, 'l'));
            }
        strcpy (&bavg[0], (char *) do_average (team2.batters[ply].hitting.hits, team2.batters[ply].hitting.atbats));
        strcpy (&savg[0], (char *) do_average (((team2.batters[ply].hitting.homers * 4) + (team2.batters[ply].hitting.triples * 3) +
                                     (team2.batters[ply].hitting.doubles * 2) + singles), team2.batters[ply].hitting.atbats));
        strcpy (&oba[0], (char *) do_average ((team2.batters[ply].hitting.hits + team2.batters[ply].hitting.bb + team2.batters[ply].hitting.hbp),
                      (team2.batters[ply].hitting.atbats + team2.batters[ply].hitting.bb + sf + team2.batters[ply].hitting.hbp)));

        gtk_list_store_append (ustore1, &uiter1);
        gtk_list_store_set (ustore1, &uiter1, UPNAME, team2.batters[ply].id.name, UTNAME1, name,
                       UYEAR1, team2.batters[ply].id.year, UHOMERUNS, team2.batters[ply].hitting.homers, UBAVG, bavg, USAVG, savg,
                            UOBAVG, oba, USB, team2.batters[ply].hitting.sb, UPOS, pos, -1);
    }
}

void
PopulateTopLevel3 () {
    gchar *cc, *work;
    gint x, buflen;

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
PopulateChildren3 () {
    gchar *cc, *work, nm[50];
    gint buflen, x, y, z;

    for (x = -1, y = 0, cc = &buffer[2], buflen = strlen (&buffer[0]); cc < (&buffer[0] + buflen); cc++) {
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
            for (z = 0; z <= NUMBER_OF_TEAMS; z++)
                if (!strcmp (&teaminfo[z].filename[0], &nm[0]))
                    break;
            teams[x][y].label = teaminfo[z].teamname;
            teams[x][y].children = NULL;
            y++;
        }
        cc = work;
    }
    teams[x][y].label = NULL;
    teams[x + 1][0].label = NULL;
}

GtkTreeModel *
create_model3 () {
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
add_columns3 (GtkTreeView *treeview) {
    gint col_offset;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* column for years/types and team names */
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "xalign", 0.0, NULL);
    col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Year->Teams", renderer, "text", NAME_COLUMN, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
    gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
}

void
setup_tree_view (GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", PNAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
 
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Homers", renderer, "text", HOMERUNS, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Batting Avg", renderer, "text", BAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Slugging Avg", renderer, "text", SAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("On-Base Avg", renderer, "text", OBAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Steals", renderer, "text", SB, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Position-Games", renderer, "text", POS, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}

void
setup_tree_view3 (GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", PITNAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
 
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Innings", renderer, "text", IP, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("ERA", renderer, "text", ERA, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Strike Outs", renderer, "text", SO, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Walks", renderer, "text", BB, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Games", renderer, "text", GAMES, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Games Started", renderer, "text", GSTARTED, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Saves", renderer, "text", SAVES, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}

void
setup_tree_uview1 (GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", UPNAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
 
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Team Name", renderer, "text", UTNAME1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Year", renderer, "text", UYEAR1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Homers", renderer, "text", UHOMERUNS, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Batting Avg", renderer, "text", UBAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Slugging Avg", renderer, "text", USAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("On-Base Avg", renderer, "text", UOBAVG, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Steals", renderer, "text", USB, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Position-Games", renderer, "text", UPOS, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}

void
setup_tree_uview2 (GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", UPITNAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
 
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Team Name", renderer, "text", UTNAME2, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Year", renderer, "text", UYEAR2, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Innings", renderer, "text", UIP, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("ERA", renderer, "text", UERA, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Strike Outs", renderer, "text", USO, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Walks", renderer, "text", UBB, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Games", renderer, "text", UGAMES, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Games Started", renderer, "text", UGSTARTED, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Saves", renderer, "text", USAVES, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}

/* remove first (and only the first) highlighted entry ... do players first then pitchers */
void
RemovePlayer (GtkWidget *widget, gpointer *pdata) {
    GtkTreeIter iter, iter2;
    GtkTreeSelection *sel;
    gchar NoSel[256] = "Highlight a player/pitcher by clicking on that row before clicking REMOVE.", *msg[5];
    gint row_count, x, y, resp = 1;
    gboolean valid;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    /* walk through players */
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (utv1));
    row_count = 0;
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ustore1), &iter);
    while (valid) {
        /* walk through the store (user's player roster) looking for a selected row */
        if (gtk_tree_selection_iter_is_selected (sel, &iter) == TRUE)
            break;;

        row_count++;
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (ustore1), &iter);
    }

    if (valid) {
        /* row selected */
        gtk_list_store_remove (ustore1, &iter);

        /* if entry in pitchers matches name remove that, too */
        for (x = 0; x < 13; x++)
            if (!strcmp (&team2.batters[row_count].id.name[0], &team2.pitchers[x].id.name[0])) {
                gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore2), &iter2, NULL, x);
                gtk_list_store_remove (ustore2, &iter2);
                break;
            }

        /* remove db entry(s) and move up following entries */
        for (; row_count < 27; row_count++)
            team2.batters[row_count] = team2.batters[row_count + 1];
        team2.batters[27].id.name[0] = ' ';
        if (x < 13) {
            for (; x < 12; x++)
                team2.pitchers[x] = team2.pitchers[x + 1];
            team2.pitchers[12].id.name[0] = ' ';
        }

        changesw = 1;
        return;
    }

    /* walk through pitchers */
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (utv2));
    row_count = 0;
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ustore2), &iter);
    while (valid) {
        /* walk through the store (user's pitcher roster) looking for a selected row */
        if (gtk_tree_selection_iter_is_selected (sel, &iter) == TRUE)
            break;;

        row_count++;
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (ustore2), &iter);
    }

    if (valid) {
        /* row selected */
        gtk_list_store_remove (ustore2, &iter);

        /* if entry in players matches name remove that, too */
        for (x = 0; x < 28; x++)
            if (!strcmp (&team2.pitchers[row_count].id.name[0], &team2.batters[x].id.name[0])) {
                /* check if this player did more than just pitch, and if he did ask the user if he wants to remove just the pitching stats or everything */
                for (y = 0; y < 10; y++) {
                    if (y == 1)
                        continue;
                    if (team2.batters[x].fielding[y].games > 0) {
                        gchar labeltext[500] = "Do you want to remove just this player's pitching stats or all his stats?";
                        GtkWidget *dlgFile, *label = NULL;

                        dlgFile = gtk_dialog_new ();
                        gtk_window_set_title (GTK_WINDOW (dlgFile), "Remove Pitching or Everything?");
                        gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

                        label = gtk_label_new (labeltext);
                        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
                        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

                        gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Remove Just Pitching Stats", 0);
                        gtk_dialog_add_button (GTK_DIALOG (dlgFile), "Remove All Stats", 1);

                        gtk_widget_show (label);

                        gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), 1);

                        resp = gtk_dialog_run (GTK_DIALOG (dlgFile));
                        gtk_widget_destroy (dlgFile);

                        break;
                    }
                }

                if (resp) {
                    gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore1), &iter2, NULL, x);
                    gtk_list_store_remove (ustore1, &iter2);
                }
                break;
            }

        /* remove db entry(s) and move up following entries */
        for (; row_count < 12; row_count++)
            team2.pitchers[row_count] = team2.pitchers[row_count + 1];
        team2.pitchers[12].id.name[0] = ' ';
        if (resp) {
            if (x < 28) {
                for (; x < 27; x++)
                    team2.batters[x] = team2.batters[x + 1];
                team2.batters[27].id.name[0] = ' ';
            }
        }
        else {
            /* we're just removing the pitching stats --- also remove fielding stats for the pitcher position */
            gchar pos[200];

            team2.batters[x].fielding[1].games = team2.batters[x].fielding[1].po = team2.batters[x].fielding[1].dp = team2.batters[x].fielding[1].a = 
              team2.batters[x].fielding[1].e = team2.batters[x].fielding[1].pb = 0;

            /* need to redisplay the data showing games at each position */
            for (pos[0] = '\0', y = 0; y < 11; y++)
                if (team2.batters[x].fielding[y].games > 0) {
                    if (strlen (&pos[0]))
                        strcat (&pos[0], ", ");
                    if (y == 10) {
                        /* don't show OF if there are games in LF, CF or RF */
                        if (team2.batters[x].fielding[7].games <= 0 && team2.batters[x].fielding[8].games <= 0 && team2.batters[x].fielding[9].games <= 0)
                            strcat (&pos[0], "OF");
                        else {
                            /* remove the last comma */
                            pos[strlen (&pos[0]) - 2] = '\0';
                            continue;
                        }
                    }
                    else
                        strcat (&pos[0], figure_pos (y));
                    strcat (&pos[0], "-");
                    strcat (&pos[0], (char *) check_stats (team2.batters[x].fielding[y].games, 'l'));
                }

            gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ustore1), &iter2, NULL, x);
            gtk_list_store_set (ustore1, &iter2, UPOS, pos, -1);
        }

        changesw = 1;
        return;
    }

    /* if we get this far there was no row selected */
    msg[0] = &NoSel[0];
    outMessage (msg);
}

