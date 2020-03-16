
/* list an injury report for the players in the user's season  */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include "db.h"

gchar injuryinfo[50000];

void
LeagueInjuryReport (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    GtkWidget *window, *box1, *box2, *hbox, *button, *pbutton, *table, *vscrollbar, *text;
    GdkFont *fixed_font;
    gchar buf[256], NoLeague[256] = "You do not have a currently active season nor series.", *msg[5],
          InjuryNotAvailable[256] = "An error was encountered when trying to retrieve the injury report.",
          Unconnected[256] = "You need to connect to an NSB server.";
    gint x;

    /* check for the existence of a season */
    sock_puts (sock, "S7\n");
    sock_gets (sock, &buf[0], sizeof (buf));
    if (!strcmp (&buf[0], "NOLEAGUE")) {
        /* check if user has a series going on the connected server */
        sock_puts (sock, "S0\n");
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "NOSERIES")) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "No season nor series established on ");
            strcat (&work[0], &hs[0]);
            strcat (&work[0], "\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoLeague[0];
            outMessage (msg);

            return;
        }
    }

    sock_puts (sock, "I\n");                       /* tell the server we want to see the season injury report */
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        if (!connected) {
            strcpy (&work[0], "Not connected.\n");
            Add2TextWindow (&work[0], 0);

            msg[0] = &Unconnected[0];
        }
        else {
            strcpy (&work[0], "Injury Report not available.\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &InjuryNotAvailable[0];
        }

        outMessage (msg);
        return;
    }

    /*
       data received from server will be in the form -

       repeat for each player:
         player name (variable number of characters)
         space (1 character)
         team year (4 characters)
         team ID (5 characters)
         teamname (field present only if user-created team (team ID >= 900) ... variable length)
         space (1 character)
         games (5 characters)
         space (1 character)
    */

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, 600, 500);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (DestroyDialog), window);
    gtk_window_set_title (GTK_WINDOW (window), "Injuries");
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

    FillInjuryInfo ();
    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, injuryinfo, strlen (&injuryinfo[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    pbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (pbutton), "clicked", G_CALLBACK (PrintInjuryReport), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), pbutton, TRUE, TRUE, 0);
    button = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (DestroyDialog), window);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_widget_show_all (window);
}

void
FillInjuryInfo () {
    gchar *cc, *ns, *cc2, work[10];
    gint x, y, teamid, teamyr, len, numlines;
    struct {
        gchar name[100], tmabbrev[20], remgames[10];
    } plist[5001];

    for (x = 0; x < 5001; x++)
        plist[x].name[0] = plist[x].tmabbrev[0] = plist[x].remgames[0] = '\0';

    strcpy (&injuryinfo[0], "\n         Injury Report for ");
    strcat (&injuryinfo[0], &nsbid[0]);
    strcat (&injuryinfo[0], " on ");
    strcat (&injuryinfo[0], &hs[0]);
    strcat (&injuryinfo[0], "\n\n PLAYER                   TEAM      Remaining Games Out\n\n");

    for (numlines = 0, len = strlen (&buffer[0]), cc = &buffer[0]; cc < (&buffer[0] + len); numlines++) { /* cc incremented in body of loop */
        gchar tnm[50];

        /* move Player Last Name */
        ns = (char *) index (cc, ' ');
        strncat (&plist[numlines].name[0], cc, (ns - cc));
        strcat (&plist[numlines].name[0], " ");
        cc = ns + 1;
        /* move Player First Name */
        for (; *ns != '1' && *ns != '2' && *ns != '0'; ns++);
        ns--;
        *ns = '\0';
        strcat (&plist[numlines].name[0], cc);
        *ns = ' ';
        cc = ns + 1;
        /* move Team year */
        strncpy (&work[0], cc, 4);
        work[4] = '\0';
        teamyr = atoi (&work[0]);
        if (teamyr)
            strncat (&plist[numlines].tmabbrev[0], cc, 4);
        cc += 4;
        /* move Team Abbrev */
        strncpy (&work[0], cc, 5);
        work[5] = '\0';
        teamid = atoi (&work[0]);

        if (teamid < 900) {
            for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                if (teaminfo[y].id == teamid) {
                    strcat (&plist[numlines].tmabbrev[0], &teaminfo[y].teamabbrev[0]);
                    break;
                }
            cc += 6;
        }
        else {
            cc += 5;
            for (y = 0; *cc != ' '; cc++, y++)
                tnm[y] = *cc;
            tnm[y] = '\0';
            if (strlen (&tnm[0]) > 14)
                strncat (&plist[numlines].tmabbrev[0], &tnm[0], 14);
            else
                strcat (&plist[numlines].tmabbrev[0], &tnm[0]);
            cc++;
        }
        /* move Games */
        ns = (char *) index (cc, ' ');
        for (cc2 = cc, x = 0; *cc2 == '0' && x < 5; x++, cc2++)
            *cc2 = ' ';
        strncat (&plist[numlines].remgames[0], cc, (ns - cc));
        cc = ns + 1;
    }

    /* sort list */
    for (x = 0; x < (numlines - 1); x++)
        for (y = x + 1; y < numlines; y++) {
            if (strcasecmp (&plist[x].tmabbrev[0], &plist[y].tmabbrev[0]) > 0) {
                plist[5000] = plist[x];
                plist[x] = plist[y];
                plist[y] = plist[5000];
            }
        }

    /* move list to output area */
    for (x = 0; x < numlines; x++) {
        strcat (&injuryinfo[0], &plist[x].name[0]);
        strncat (&injuryinfo[0], "                         ", (25 - strlen (&plist[x].name[0])));
        strcat (&injuryinfo[0], &plist[x].tmabbrev[0]);
        strncat (&injuryinfo[0], "                 ", (17 - strlen (&plist[x].tmabbrev[0])));
        strcat (&injuryinfo[0], &plist[x].remgames[0]);
        strcat (&injuryinfo[0], "\n");
    }
}

void
PrintInjuryReport (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Injury Report ...", *msg[5];
    gint x;

    print (&injuryinfo[0]);

    strcpy (&work[0], "Print Injury Report in season on ");
    strcat (&work[0], &hs[0]);
    strcat (&work[0], "\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

