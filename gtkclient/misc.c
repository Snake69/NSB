
/* miscellaneous routines */

#include "gtknsbc.h"
#include "prototypes.h"
#include "db.h"
#include "cglobal.h"
#include "net.h"
#include <syslog.h>
#include <time.h>
#include <paths.h>
#include <fcntl.h>

/* add some text to the scrolling text area in the main window */
void
Add2TextWindow (char *textbuf, int error) {
    gint sbUpperLim;

    /* scrollbar near upper limit? */
    if (gtk_adjustment_get_value (GTK_TEXT (text)->vadj) >= (GTK_TEXT (text)->vadj->upper - (GTK_TEXT (text)->vadj->page_size * 2)))
        sbUpperLim = 1;
    else
        sbUpperLim = 0;

    /* freeze the text widget, ready for multiple updates */
    gtk_text_freeze (GTK_TEXT (text));

    if (error)
        gtk_text_insert (GTK_TEXT (text), NULL, &color, NULL, "ERROR - ", -1);
    else
        gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL, "INFO - ", -1);

    gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL, textbuf, -1);

    /* force scrollbar to upper limit only if it is already near the upper limit prior to inserting the text */
    if (sbUpperLim)
        gtk_adjustment_set_value (GTK_TEXT (text)->vadj, GTK_TEXT (text)->vadj->upper - GTK_TEXT (text)->vadj->page_size);

    /* thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    /* show text */
    gtk_widget_show (text);
}

/* ready a standard error message to be output */
void
GotError () {
    gchar Error[256] = "An error was encountered when talking to the server.",
          Unconnected[256] = "You need to connect to an NSB server.", *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        strcpy (&work[0], "Not connected.\n");
        Add2TextWindow (&work[0], 0);

        msg[0] = &Unconnected[0];
    }
    else {
        strcpy (&work[0], "Encountered an error when talking to server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 1);
        msg[0] = &Error[0];
    }

    outMessage (msg);
}

/* output a message or messages in a dialog to the user */
void
outMessage (gchar *message[5]) {
    gint x;
    gchar labeltext[5000];
    GtkWidget *dlgFile, *hbox, *stock, *label, *separator;

    dlgFile = gtk_dialog_new_with_buttons ("NSB Message", NULL, GTK_DIALOG_MODAL, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    labeltext[0] = '\0';
    for (x = 0; x < 5; x++)
        if (message[x] != NULL)
            strcat (&labeltext[0], message[x]);
        else
            break;

    label = gtk_label_new (labeltext);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    separator = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (hbox), separator);

    gtk_widget_show_all (hbox);
    gtk_dialog_run (GTK_DIALOG (dlgFile));

    DestroyDialog (dlgFile, dlgFile);
}

/* display a dialog which calls for a user response */
int
ShallWeContinue (gchar *message[5]) {
    gint x;
    gchar labeltext[500];
    GtkWidget *dlgFile, *label = NULL;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Shall We Continue?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    for (labeltext[0] = '\0', x = 0; x < 5; x++)
        if (message[x] != NULL) {
            strcat (&labeltext[0], message[x]);
            label = gtk_label_new (labeltext);
            gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
            gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);
        }

    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button (GTK_DIALOG (dlgFile), GTK_STOCK_YES, GTK_RESPONSE_YES);

    gtk_widget_show (label);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgFile), GTK_RESPONSE_CANCEL);

    x = gtk_dialog_run (GTK_DIALOG (dlgFile));
    gtk_widget_destroy (dlgFile);

    return (x == GTK_RESPONSE_YES);
}

/* do some hard copy */
void
print (char *toprint) {
    gint x, done = 0;
    gchar buf[77], *cc;
    FILE *lpr;

    lpr = popen ("lpr", "w");

    buf[76] = '\0';                /* to stop printer overruns */

    cc = toprint;
    while (!done) {
        for (x = 0; x < 76; x++) {
            buf[x] = *cc;
            cc++;
            if (buf[x] == '\n') {
                buf[x + 1] = '\0';
                break;
            }
            if (buf[x] == '\0') {
                done = 1;
                break;
            }
        }
        fprintf (lpr, "%s", buf);
    }
    fprintf (lpr, "%s", buf);

    pclose (lpr);
}

/* drop leading zeros in a string and resize string appropriately */
void
dropleadingzeros (char *str) {
    int x, y;

    for (y = strlen (str); str[0] == '0'; y--)
        for (x = 0; x < y; x++)
            str[x] = str[x + 1];

    if (strlen (str) == 0)
        strcpy (str, "0");
}

/* convert leading zeros in a string to spaces, do not resize string */
void
leadingzerostospaces (char *str) {
    int y;

    for (y = 0; str[y] == '0' && y < strlen (str); y++)
        str[y] = ' ';
}

/* convert int (max length 7 positions) to string */
char *
cnvt_int2str (int num, char justify) {
    gint div, x;

    for (div = 1000000, x = 0; x < 7; x++) {
        resultstr[x] = (num / div) + '0';
        num = num - ((num / div) * div);
        div = div / 10;
    }
    resultstr[7] = '\0';
    if (justify == 'r')
        leadingzerostospaces (&resultstr[0]);
    else
        dropleadingzeros (&resultstr[0]);
    return (&resultstr[0]);
}

/* if stat is -1 then return a dash, otherwise return integer converted to string */
char *
check_stats (int num, char justify) {
    if (num < 0)
        return ("     - ");
    else
        return (cnvt_int2str (num, justify));
}

/* get a number and return a character playing position */
char *
figure_pos (gint pos) {
    switch (pos) {
        case 0:
            return ("DH");
            break;
        case 1:
            return (" P");
            break;
        case 2:
            return (" C");
            break;
        case 3:
            return ("1B");
            break;
        case 4:
            return ("2B");
            break;
        case 5:
            return ("3B");
            break;
        case 6:
            return ("SS");
            break;
        case 7:
            return ("LF");
            break;
        case 8:
            return ("CF");
            break;
        case 9:
            return ("RF");
            break;
        case 10:
            return ("PH");
            break;
        case 11:
            return ("PR");
            break;
        default:
            return ("OF");
    }
}

/* convert a team ID to it's alphanumeric team name */
char *
GetTeamName (gint ID) {
    gint x;
    gchar *error = ("ERROR");

    for (x = 0; x <= NUMBER_OF_TEAMS; x++)
        if (teaminfo[x].id == ID)
            return (&teaminfo[x].teamname[0]);

    /* will never get here unless there's a problem */
    return (error);
}

/* get complete team structure from server */
int
get_stats (int sock, gchar which, int autoind) {
    gint x, y;

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        if (!autoind)
            GotError ();
        return -1;
    }
    if (!strlen (&buffer[0])) {
        /* this happens if there are no statistics available */
        gchar NoStats[256] = "No statistics are available.", *msg[5];
        gint x;

        if (!autoind) {
            for (x = 0; x < 5; x++)
                msg[x] = NULL;

            strcpy (&work[0], "No stats available after a game was played!\n");
            Add2TextWindow (&work[0], 1);

            msg[0] = &NoStats[0];
            outMessage (msg);
        }

        return -1;
    }

    team.id = atoi (&buffer[0]);
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.year = atoi (&buffer[0]);
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.league = buffer[0];
    sock_gets (sock, &buffer[0], sizeof (buffer));
    team.division = buffer[0];

    for (x = 0; x < 28; x++) {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        strcpy (&team.batters[x].id.name[0], &buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.teamid = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.batslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.throwslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.year = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.injury = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.starts_rest = atoi (&buffer[0]);
        for (y = 0; y < 4; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].id.ip_last4g[y] = atoi (&buffer[0]);
        }
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].id.inn_target = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].dob.month = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].dob.day = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].dob.year = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.atbats = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.bb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.hbp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.gidp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.sf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.batters[x].hitting.statsind = atoi (&buffer[0]);

        for (y = 0; y < 11; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].games = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].po = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].dp = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].a = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].pb = atoi (&buffer[0]);
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.batters[x].fielding[y].e = atoi (&buffer[0]);
        }
    }
    for (x = 0; x < 13; x++) {
        sock_gets (sock, &buffer[0], sizeof (buffer));
        strcpy (&team.pitchers[x].id.name[0], &buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.teamid = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.batslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.throwslr = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.year = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.injury = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.starts_rest = atoi (&buffer[0]);
        for (y = 0; y < 4; y++) {
            sock_gets (sock, &buffer[0], sizeof (buffer));
            team.pitchers[x].id.ip_last4g[y] = atoi (&buffer[0]);
        }
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].id.inn_target = atoi (&buffer[0]);

        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.games = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.games_started = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.innings = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.thirds = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.wins = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.losses = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.saves = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.bfp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.hits = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.doubles = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.triples = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.homers = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.runs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.er = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.rbi = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.cg = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.gf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sho = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.svopp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.cs = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.walks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.so = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.ibb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sh = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.sf = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.wp = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.balks = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.hb = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.opp_ab = atoi (&buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        team.pitchers[x].pitching.statsind = atoi (&buffer[0]);
    }

    if (which == 'v')
        visitor_cur = team;
    if (which == 'h')
        home_cur = team;
    if (which == 'b')
        visitor_season = team;
    if (which == 'a')
        home_season = team;
    if (which == 'd')
        visitor = team;
    if (which == 'c')
        home = team;
    if (which == 'u')
        team2 = team;

    return 0;
}

/* play audio */
void *
play_snd (void *arg) {
    gchar cmd[1024];

    if (!preferences.PlaySounds)
        /* don't play any sounds */
        return (NULL);

    strcpy (&cmd[0], "play ");
    strcat (&cmd[0], arg);
    strcat (&cmd[0], " > /dev/null 2>&1");
    if (system (cmd))
        syslog (LOG_INFO, "install sox for audio playback");

    return (NULL);
}

/* speak play-by-play */
void *
SpeakPlayByPlay () {
    gchar text[100000], cmd[200000];

    if (Speaking)
        /* already speaking */
        return (NULL);

    if (curbeg < strlen (&playbyplay[0])) {
        Speaking = 1;
        strcpy (&text[0], &playbyplay[curbeg]);
        curbeg = strlen (&playbyplay[0]);
        if (speed.tv_sec < 3) {
            /* if the play-by-play text is displaying too fast don't speak
               this check is after updating curbeg (point to start speaking) so that speaking doesn't try to catch up with the text that has already passed
                 by in case the user changes the display speed while playing the game */
            Speaking = 0;
            return (NULL);
        }
        strcpy (&cmd[0], "espeak -s 180 ");
        strcat (&cmd[0], "\"");
        strcat (&cmd[0], &text[0]);
        strcat (&cmd[0], "\"");
        strcat (&cmd[0], " > /dev/null 2>&1");
        if (system (cmd)) {
            syslog (LOG_INFO, "install espeak to speak play-by-play");
            /* ensure processing doesn't get here more than once by not setting the Speaking variable to 0 */
            return (NULL);
        }
    }

    Speaking = 0;
    return (NULL);
}

/* sensitize/desensitize menu selections */
void
SetLeagueUnderWay (int setting) {
    LeagueUnderWay = setting;

    /* first sensitize all menu selections which may change */
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Connect to a Server"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Disconnect from Current Server"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Computer"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Human/Same Computer"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Human/Over Network"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Watch One Game Computer vs Computer"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Portion of Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Game(s) in a Series"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Injury Report"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Establish Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Establish a Series"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Users With Accounts on Connected Server"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Change My NSB ID on Connected Server"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Xtras/Evaluate Team Seasons"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Create, Edit, Rename or Delete Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Delete My NSB ID on Connected Server"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Standings"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Category Leaders/Regular Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Category Leaders/Post-Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/By Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Team Totals/Regular Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Team Totals/Post-Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Most Valuable Player"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Cy Young"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Gold Gloves"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Silver Sluggers"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                           "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Game Records"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                         "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Season Records"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                   "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Game Records"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                 "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Season Records"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/Lifetime Leaders/Regular Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/Lifetime Leaders/Post-Season"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/By Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Season Results"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Category Leaders"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/By Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/By Player"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Team Totals"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/User-Created Teams/By Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/User-Created Teams/Team Totals"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Series Status"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Category Leaders"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/By Team"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Team Totals"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Who is Waiting"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Add Name"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Remove Name"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Request to Play"), TRUE);

    /* next desensitize those menu selections dependent upon being connected to (or disconnected from) a server */
    if (connected)
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Connect to a Server"), FALSE);
    else {
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Disconnect from Current Server"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Computer"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Human/Same Computer"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/One Game Against Human/Over Network"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Watch One Game Computer vs Computer"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Change My NSB ID on Connected Server"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Delete My NSB ID on Connected Server"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Xtras/Evaluate Team Seasons"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Create, Edit, Rename or Delete Team"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Establish Season"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Establish a Series"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Injury Report"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Users With Accounts on Connected Server"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                         "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Game Records"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                       "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Season Records"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                                 "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Game Records"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory,
                               "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Season Records"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/Lifetime Leaders/Regular Season"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/Lifetime Leaders/Post-Season"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/All NSB Seasons/By Team"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Season Results"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Category Leaders"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/By Team"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/By Player"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Real Life/Team Totals"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/User-Created Teams/By Team"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/User-Created Teams/Team Totals"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Who is Waiting"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Add Name"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Remove Name"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Request to Play"), FALSE);
    }

    /* desensitize those menu selections dependent upon the server running network play */
    if (poolmngr) {
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Who is Waiting"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Add Name"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Remove Name"), FALSE);
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Waiting Pool/Request to Play"), FALSE);
    }

    /* finally desensitize those menu selections dependent upon whether or not the user has a season going */
    switch (LeagueUnderWay) {   /* LeagueUnderWay will be 0 if connected is 0 */
        case 0:
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Injury Report"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Standings"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Category Leaders/Regular Season"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Category Leaders/Post-Season"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/By Team"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Team Totals/Regular Season"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Team Totals/Post-Season"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Most Valuable Player"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Cy Young"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Gold Gloves"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current NSB Season/Awards/Silver Sluggers"), FALSE);
        case 2:
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Portion of Season"), FALSE);
    }
}

void
SetSeriesUnderWay (int setting) {
    SeriesUnderWay = setting;

    /* desensitize those menu selections dependent upon whether or not the user has a series going */
    switch (SeriesUnderWay) {   /* SeriesUnderWay will be 0 if connected is 0 */
        case 0:
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Series Status"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Category Leaders"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/By Team"), FALSE);
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Statistics/Current Series/Team Totals"), FALSE);
        case 2:
            gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Game/Play/Game(s) in a Series"), FALSE);
    }
    if (SeriesUnderWay == 1)
        gtk_widget_set_sensitive (gtk_item_factory_get_widget (item_factory, "/Administration/Injury Report"), TRUE);
}

gchar wpinfo[1024];

/* display the waiting pool */
void
DisplayWP () {
    GtkWidget *dlgWP, *box1, *box2, *hbox, *separator, *prbutton, *disbutton, *table, *vscrollbar, *text;
    GdkFont *fixed_font;

    dlgWP = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgWP), "Users Waiting to Play a Game");
    gtk_window_set_default_size (GTK_WINDOW (dlgWP), 600, 230);
    gtk_signal_connect (GTK_OBJECT (dlgWP), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgWP)->vbox), box1, TRUE, TRUE, 0);

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

    FillWPInfo ();
    gtk_text_insert (GTK_TEXT (text), fixed_font, NULL, NULL, wpinfo, strlen (&wpinfo[0]));

    /* Thaw the text widget, allowing the updates to become visible */
    gtk_text_thaw (GTK_TEXT (text));

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);

    prbutton = gtk_button_new_with_label ("Print");
    g_signal_connect (G_OBJECT (prbutton), "clicked", G_CALLBACK (PrintWP), dlgWP);
    disbutton = gtk_button_new_with_label ("Dismiss");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (DestroyDialog), dlgWP);
    gtk_box_pack_start (GTK_BOX (hbox), prbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgWP);
}

void
FillWPInfo () {
    gchar *cc, *ns;

    strcpy (&wpinfo[0], "\n         Users Waiting to Play a Game on ");
    strcat (&wpinfo[0], &hs[0]);
    strcat (&wpinfo[0], "\n\n\n");
    strcat (&wpinfo[0], "  NSB ID\n\n");

    for (cc = &buffer[0]; cc < (&buffer[0] + strlen (&buffer[0])); ) {
        strcat (&wpinfo[0], "  ");

        /* move NSB ID */
        ns = (char *) index (cc, ' ');
        strncat (&wpinfo[0], cc, (ns - cc));
        cc = ns + 1;

        strcat (&wpinfo[0], "\n");
    }
}

void
PrintWP (GtkWidget *widget, gpointer *pdata) {
    gchar Printing[256] = "Printing Waiting Pool ...", *msg[5];
    gint x;

    print (&wpinfo[0]);

    strcpy (&work[0], "Print waiting pool on ");
    strcat (&work[0], &hs[0]);
    strcat (&work[0], "\n\n");
    Add2TextWindow (&work[0], 0);

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    msg[0] = &Printing[0];

    outMessage (msg);
}

int
GetRequestee () {
    GtkWidget *dlgRequestee, *hbox, *stock, *label, *table, *requestee;
    gchar labelmsg[512], ReqID[256];
    gint response, x;
    const gchar *entry_text;

    ReqID[0] = '\0';
    strcpy (&labelmsg[0], "Enter the NSBID of the user (see Waiting Pool->Who is Waiting) with whom you would like to play a game:");

    dlgRequestee = gtk_dialog_new_with_buttons ("Request to Play a Game", GTK_WINDOW (mainwin),
                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgRequestee)->vbox), hbox, FALSE, FALSE, 0);

    stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
    label = gtk_label_new_with_mnemonic (labelmsg);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

    requestee = gtk_entry_new ();
    if (strlen (&ReqID[0]))
        gtk_entry_set_text (GTK_ENTRY (requestee), ReqID);
    gtk_entry_set_max_length (GTK_ENTRY (requestee), 30);
    gtk_table_attach_defaults (GTK_TABLE (table), requestee, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), requestee);

    gtk_dialog_set_default_response (GTK_DIALOG (dlgRequestee), GTK_RESPONSE_OK);
    gtk_entry_set_activates_default (GTK_ENTRY (requestee), TRUE);

    gtk_widget_show_all (hbox);
    response = gtk_dialog_run (GTK_DIALOG (dlgRequestee));

    if (response == GTK_RESPONSE_OK) {
        entry_text = gtk_entry_get_text (GTK_ENTRY (requestee));
        strcpy (&ReqID[0], entry_text);

        strcpy (&RequesteeID[0], &ReqID[0]);          /* save user response */
        x = 1;
    }
    else
        x = 0;

    DestroyDialog (dlgRequestee, dlgRequestee);
    return (x);
}

/* send entire team structure to server */
void
send_stats (int sock) {
    int x, y;

    sprintf (buffer1, "%d\n%d\n%c\n%c\n", team2.id, team2.year, team2.league, team2.division);
    /* send stats to server */
    sock_puts (sock, &buffer1[0]);

    for (x = 0; x < 28; x++) {
        sprintf (buffer1, "%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team2.batters[x].id.name, team2.batters[x].id.teamid,
                 team2.batters[x].id.batslr, team2.batters[x].id.throwslr, team2.batters[x].id.year, team2.batters[x].id.injury,
                 team2.batters[x].id.starts_rest, team2.batters[x].id.ip_last4g[0],
                 team2.batters[x].id.ip_last4g[1], team2.batters[x].id.ip_last4g[2],
                 team2.batters[x].id.ip_last4g[3], team2.batters[x].id.inn_target);
        /* send stats to server */
        sock_puts (sock, &buffer1[0]);

        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.batters[x].hitting.games, team2.batters[x].hitting.atbats,
                 team2.batters[x].hitting.runs, team2.batters[x].hitting.hits, team2.batters[x].hitting.doubles,
                 team2.batters[x].hitting.triples);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.batters[x].hitting.homers, team2.batters[x].hitting.rbi,
                 team2.batters[x].hitting.bb, team2.batters[x].hitting.so, team2.batters[x].hitting.hbp,
                 team2.batters[x].hitting.gidp);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.batters[x].hitting.sb, team2.batters[x].hitting.cs,
                 team2.batters[x].hitting.ibb, team2.batters[x].hitting.sh, team2.batters[x].hitting.sf,
                 team2.batters[x].hitting.statsind);
        sock_puts (sock, &buffer1[0]);
        for (y = 0; y < 11; y++) {
            sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.batters[x].fielding[y].games, team2.batters[x].fielding[y].po,
                     team2.batters[x].fielding[y].dp, team2.batters[x].fielding[y].a, team2.batters[x].fielding[y].pb,
                     team2.batters[x].fielding[y].e);
            sock_puts (sock, &buffer1[0]);
        }
    }
    for (x = 0; x < 13; x++) {
        sprintf (buffer1, "%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].id.name,
                 team2.pitchers[x].id.teamid, team2.pitchers[x].id.batslr, team2.pitchers[x].id.throwslr,
                 team2.pitchers[x].id.year, team2.pitchers[x].id.injury, team2.pitchers[x].id.starts_rest,
                 team2.pitchers[x].id.ip_last4g[0], team2.pitchers[x].id.ip_last4g[1], team2.pitchers[x].id.ip_last4g[2],
                 team2.pitchers[x].id.ip_last4g[3], team2.pitchers[x].id.inn_target);
        /* send stats to server */
        sock_puts (sock, &buffer1[0]);

        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].pitching.games,
                 team2.pitchers[x].pitching.games_started, team2.pitchers[x].pitching.innings, team2.pitchers[x].pitching.thirds,
                 team2.pitchers[x].pitching.wins, team2.pitchers[x].pitching.losses, team2.pitchers[x].pitching.saves);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].pitching.bfp, team2.pitchers[x].pitching.hits,
                 team2.pitchers[x].pitching.doubles, team2.pitchers[x].pitching.triples, team2.pitchers[x].pitching.homers,
                 team2.pitchers[x].pitching.runs);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].pitching.er, team2.pitchers[x].pitching.rbi,
                 team2.pitchers[x].pitching.cg, team2.pitchers[x].pitching.gf, team2.pitchers[x].pitching.sho,
                 team2.pitchers[x].pitching.svopp);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].pitching.sb, team2.pitchers[x].pitching.cs,
                 team2.pitchers[x].pitching.walks, team2.pitchers[x].pitching.so, team2.pitchers[x].pitching.ibb,
                 team2.pitchers[x].pitching.sh);
        sock_puts (sock, &buffer1[0]);
        sprintf (buffer1, "%d\n%d\n%d\n%d\n%d\n%d\n", team2.pitchers[x].pitching.sf, team2.pitchers[x].pitching.wp,
                 team2.pitchers[x].pitching.balks, team2.pitchers[x].pitching.hb, team2.pitchers[x].pitching.opp_ab,
                 team2.pitchers[x].pitching.statsind);
        sock_puts (sock, &buffer1[0]);
    }
}

void
GetIDs () {
    gint x, sk, noplyh, noplyv, hv, xx, yy;
    gchar fname[1000], work[50], nmsanssp[50], *cc, *tcc;
    GError *err = NULL;

    if (netgame && challenger_ind)
        sk = sockhvh;
    else
        sk = sock;

    if (sock_gets (sk, &buffer[0], sizeof (buffer)) < 0) {
        gchar NotAvailable[256] = "An error was encountered when talking to the server.", *msg[5];
        gint x;

        for (x = 0; x < 5; x++)
            msg[x] = NULL;

        strcpy (&work[0], "Error talking to server.\n\n");
        Add2TextWindow (&work[0], 1);

        msg[0] = &NotAvailable[0];

        outMessage (msg);
        return;
    }

    /*
       data received from server will be in the form -

       repeat twice (for each team):
         number of players on home team (2 characters)
         number of players on visiting team (2 characters)
         player name #1 on home team (variable number of characters)
         : (field delimiter)
         date of birth - mmddyyyy (8 characters)
         repeat for players # 2 - # of players indicated by field 1
         player name #1 on visiting team (variable number of characters)
         : (field delimiter)
         date of birth - mmddyyyy (8 characters)
         repeat for players # 2 - # of players indicated by field 2
    */

    if (!preferences.ShowPlayerPics)
        /* the user doesn't want to show the player pics so forget about setting it up */
        return;

    strncpy (&work[0], &buffer[0], 2);
    work[2] = '\0';
    noplyh = atoi (&work[0]);
    strncpy (&work[0], &buffer[2], 2);
    work[2] = '\0';
    noplyv = atoi (&work[0]);

    ppics[0].nplyr = noplyh;
    ppics[1].nplyr = noplyv;

    cc = &buffer[4];
    for (hv = 0; hv < 2; hv++)
        for (x = 0; x < ppics[hv].nplyr; x++) {
            tcc = index (cc, ':');
            *tcc = '\0';
            strcpy (&ppics[hv].plyr[x].pname[0], cc);
            *tcc = ':';
            cc = tcc + 1;
            strncpy (&ppics[hv].plyr[x].bmonth[0], cc, 2);
            ppics[hv].plyr[x].bmonth[2] = '\0';
            strncpy (&ppics[hv].plyr[x].bday[0], cc + 2, 2);
            ppics[hv].plyr[x].bday[2] = '\0';
            strncpy (&ppics[hv].plyr[x].byear[0], cc + 4, 4);
            ppics[hv].plyr[x].byear[4] = '\0';
            cc += 8;
        }

    /* reverse names */
    for (hv = 0; hv < 2; hv++)
        for (x = 0; x < ppics[hv].nplyr; x++) {
            strcpy (&work[0], &ppics[hv].plyr[x].pname[index (&ppics[hv].plyr[x].pname[0], ',') - &ppics[hv].plyr[x].pname[0] + 2]);
            strcat (&work[0], " ");
            strncat (&work[0], &ppics[hv].plyr[x].pname[0], (index (&ppics[hv].plyr[x].pname[0], ',') - &ppics[hv].plyr[x].pname[0]));
            strcpy (&ppics[hv].plyr[x].pname[0], &work[0]);
        }

    /* load pics */
    for (hv = 0; hv < 2; hv++)
        for (x = 0; x < ppics[hv].nplyr; x++) {
            strcpy (&fname[0], "/usr/local/share/NSB/PlayerPics/");

            /* remove space(s) from name */
            for (yy = xx = 0; xx <= strlen (&ppics[hv].plyr[x].pname[0]); xx++)
                if (ppics[hv].plyr[x].pname[xx] != ' ') {
                    nmsanssp[yy] = ppics[hv].plyr[x].pname[xx];
                    yy++;
                }

            strcat (&fname[0], &nmsanssp[0]);
            strcat (&fname[0], "-");
            strcat (&fname[0], &ppics[hv].plyr[x].bmonth[0]);
            strcat (&fname[0], &ppics[hv].plyr[x].bday[0]);
            strcat (&fname[0], &ppics[hv].plyr[x].byear[0]);
            strcat (&fname[0], ".jpg");
            err = NULL;

            ppics[hv].plyr[x].pic = gdk_pixbuf_new_from_file (fname, &err);
        }
}

FILE *sfp;
gchar *sp, buf[500000], entirevteam[100], entirehteam[100];
struct autoplayparms autoplay;

void
NSBAutoPlay (int argc, char *argv[]) {
    FILE *rc;
    gint sock = -1, i, x, y, z, pvect[2], IDnotverified, stayput, error = 0;
    gchar nsbid[100], NSBID[100], nsbserver[100], host[256], path2autoplay[1024], work[10], teamc[10], yearc[10];

    if (argc < 4) {
        syslog (LOG_ERR, "(NSB League Autoplay) nsbid and/or nsbserver missing, usage - %s LEAGUEAUTOPLAY [nsbid] [nsbserver]", argv[0]);
        return;
    }

    if (gethostname (host, 255) < 0) {
        syslog (LOG_ERR, "(NSB League Autoplay) can't get our own hostname");
        return;
    }

    /* save nsbid */
    strcpy (&nsbid[0], argv[2]);
    /* save nsbserver */
    strcpy (&nsbserver[0], argv[3]);

    path2autoplay[0] = '\0';
    strcpy (&path2autoplay[0], getenv ("HOME"));
    strcat (&path2autoplay[0], "/.NSBautoplayrc");

    /* set up to send email */
    if (pipe (pvect) < 0) {
        syslog (LOG_ERR, "(NSB League Autoplay) pipe: %s", strerror (errno));
        return;
    }
    i = fork ();
    if (i < 0) {
        syslog (LOG_ERR, "(NSB League Autoplay) fork: %s", strerror (errno));
        return;
    }
    if (i == 0) {
        /* this is the child process from the fork() */
        dup2 (pvect[0], 0);   /* make return info available on standard input */
        close (pvect[0]);
        close (pvect[1]);
        execl (_PATH_SENDMAIL, "sendmail", "-fNSBLeagueAutoPlay", "-FNSBLeagueAutoPlay", getenv ("USER"), NULL);
        /* a successful execl will not return */
        syslog (LOG_ERR, "(NSB League Autoplay) can't exec %s: %s", _PATH_SENDMAIL, strerror (errno));
        return;
    }
    /* here we're in the parent process */
    close (pvect[0]);
    sfp = fdopen (pvect[1], "w");
    /* fill in return email header */
    fprintf (sfp,"User-Agent: NSBLeagueAutoPlay\n");
    fprintf (sfp, "To: %s\n", getenv ("USER"));
    fprintf (sfp, "Subject: NSB LeagueAutoPlay Results\n");

    /* begin filling in return email body */
    sp = buf;
    strcpy (buf, "\n");

    x = CheckLock ();
    if (x) {
        if (x == 2) {
            strcat (buf, "Cannot open nor create league lock file for user ");
            strcat (buf, getenv ("USER"));
            strcat (buf, ".\nLeague games not run.\n");
        }
        else
            if (x == 3) {
                strcat (buf, "Cannot open league lock file for user .");
                strcat (buf, getenv ("USER"));
                strcat (buf, ".\nLeague games not run.\n");
            }
            else {
                strcat (buf, "Another process is running games in the league for user ");
                strcat (buf, getenv ("USER"));
                strcat (buf, ".\nTry again later.\n");
            }
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* load the user's autoplay set-up file if it exists */
    if ((rc = fopen (path2autoplay, "r")) != NULL) {
        x = fread (&autoplay, sizeof autoplay, 1, rc);
        fclose (rc);
    }
    else {
        strcat (buf, "The .NSBautoplayrc file does not exist for user ");
        strcat (buf, getenv ("USER"));
        strcat (buf, ".\nCreate the .NSBautoplayrc file using Administration->Season Autoplay->Set-up in the NSB client program.\n");
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* check the active indicator */
    if (!autoplay.active) {
        strcat (buf, "Autoplay is currently suspended for user ");
        strcat (buf, getenv ("USER"));
        strcat (buf, ".\nActivate Autoplay using Administration->Season Autoplay->Activation in the NSB client program.\n");
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* make connection to NSB server */
    sock = make_connection ("49999", SOCK_STREAM, &nsbserver[0], 0);
    if (sock == -1) {
        strcat (buf, "(NSB League Autoplay) Could not make a connection with NSB server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n");
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* a dummy get */
    sock_gets (sock, &buffer[0], sizeof (buffer));

    /* here the server expects our userid
       but since the server does not have a direct connect to the source
       it can't pick up the hostname so we need to send that, too */
    strcpy (&buffer[0], "XXaBaseballXX");  /* tell server this is a special case */

    strcat (&buffer[0], getenv ("USER"));
    strcat (&buffer[0], "@");
    if (!strcmp (&nsbserver[0], "localhost") || !strcmp (&nsbserver[0], &host[0]))
        strcat (&buffer[0], "localhost");
    else
        strcat (&buffer[0], &host[0]);
    strcat (&buffer[0], "\n");
    sock_puts (sock, &buffer[0]);

    /* check that user exists on server */
    sock_gets (sock, &buffer[0], sizeof (buffer));
    /* here the server will look for a match on userid and domain */
    if (strcmp (&buffer[0], "ok")) {
        strcat (buf, "(NSB League Autoplay) NSBID ");
        strcat (buf, &nsbid[0]);
        strcat (buf, " not valid on NSB server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n");
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* dummy gets */
    sock_gets (sock, &buffer[0], sizeof (buffer));

    /* server sends us the NSBID here, compare to what the user supplied */
    sock_gets (sock, &NSBID[0], sizeof (NSBID));
    IDnotverified = 0;
    if (strcmp (&nsbid[0], &NSBID[0])) {
        strcat (buf, "(NSB League Autoplay) NSBID ");
        strcat (buf, &nsbid[0]);
        strcat (buf, " not correct for NSB server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n");
        fputs (sp, sfp);
        IDnotverified = 1;
    }
    /* dummy gets */
    sock_gets (sock, &buffer[0], sizeof (buffer));

    /* connection to server complete now */

    if (IDnotverified)
        goto GETOUT;

    /* check for the existence of a league */
    sock_puts (sock, "S7\n");
    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
        strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n");
        fputs (sp, sfp);
        goto GETOUT;
    }
    if (!strcmp (&buffer[0], "NOLEAGUE")) {
        strcat (buf, "(NSB League Autoplay) There is not a current league for NSBID ");
        strcat (buf, &nsbid[0]);
        strcat (buf, " on NSB server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n");
        fputs (sp, sfp);
        goto GETOUT;
    }

    /* play 1 day in league */
    strcpy (&work[0], "ld1\n");
    sock_puts (sock, &work[0]);

    populate ();     /* initialize teaminfo structure */
    stayput = 1;

    strcat (buf, "Results for ");
    strcat (buf, &nsbid[0]);
    strcat (buf, " on NSB server ");
    strcat (buf, &nsbserver[0]);
    strcat (buf, " ...\n\n");
    fputs (sp, sfp);

    /* game play loop */
    while (stayput) {
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
            stayput = 0;
        }

        if (!strcmp (&buffer[0], "NO LEAGUE")) {
            strcat (buf, "(NSB League Autoplay) There is not a current league for NSBID ");
            strcat (buf, &nsbid[0]);
            strcat (buf, " on NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
            stayput = 0;
        }

        if (!strcmp (&buffer[0], "ERROR") || !strcmp (&buffer[0], "CANNOT PLAY")) {
            strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
            stayput = 0;
        }

        if (!strcmp (&buffer[0], "OK"))
            stayput = 0;

        if (!strcmp (&buffer[0], "EOS")) {
            strcat (buf, "\nReached end of regular season.\n\n");
            fputs (sp, sfp);
            while (strcmp (&buffer[0], "OK"))
                sock_gets (sock, &buffer[0], sizeof (buffer));
            stayput = 0;
        }

        if (!strncmp (&buffer[0], "EOPS", 4)) {
            gint y, teami, yeari;
            gchar uctname[50];

            for (y = 0; y < 4; y++) {
                yearc[y] = buffer[4 + y];
                teamc[y] = buffer[8 + y];
            }
            teamc[4] = yearc[4] = '\0';
            teami = atoi (&teamc[0]);
            yeari = atoi (&yearc[0]);
            if (yeari) {
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == teami)
                        break;
            }
            else
                strcpy (&uctname[0], &buffer[12]);

            strcat (buf, "\nCompleted post-season.\n\n");

            if (yeari) {
                strcat (buf, &yearc[0]);
                strcat (buf, " ");
                strcat (buf, &teaminfo[y].teamname[0]);
            }
            else
                strcat (buf, &uctname[0]);
            strcat (buf, " World Champs !!\n");

            fputs (sp, sfp);
            stayput = 0;
        }

        if (!strcmp (&buffer[0], "fuckup")) {
            strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
            stayput = 0;
        }

        if (!strncmp (&buffer[0], "TEAMS", 5)) {
            /* save team years and names */
            for (y = 0, x = 5; x < 9; x++, y++)
                vteamyr[y] = buffer[x];
            vteamyr[y] = '\0';
 
            for (y = 0; buffer[x] != ' '; x++, y++)
                visiting_team[y] = buffer[x];
            visiting_team[y] = '\0';

            for (z = 0, y = x + 1, x++; x < (y + 4); x++, z++)
                hteamyr[z] = buffer[x];
            hteamyr[z] = '\0';

            for (y = 0; x < strlen (&buffer[0]); x++, y++)
                home_team[y] = buffer[x];
            home_team[y] = '\0';

            for (x = 0; x <= NUMBER_OF_TEAMS; x++) {
                if (!strcmp (&teaminfo[x].filename[0], &visiting_team[0]))
                    VisitingTeamID = teaminfo[x].id;
                if (!strcmp (&teaminfo[x].filename[0], &home_team[0]))
                    HomeTeamID = teaminfo[x].id;
            }

            if (vteamyr[0] != '0') {
                strcpy (&entirevteam[0], &vteamyr[0]);
                strcat (&entirevteam[0], " ");
                strcat (&entirevteam[0], (char *) GetTeamName (VisitingTeamID));
            }
            else
                strcpy (&entirevteam[0], &visiting_team[0]);
            if (hteamyr[0] != '0') {
                strcpy (&entirehteam[0], &hteamyr[0]);
                strcat (&entirehteam[0], " ");
                strcat (&entirehteam[0], (char *) GetTeamName (HomeTeamID));
            }
            else
                strcpy (&entirehteam[0], &home_team[0]);
        }

        if (!strncmp (&buffer[0], "BX", 2) && (autoplay.linescore || autoplay.boxscore)) {
            /* output an end-of-game boxscore
               first, save the buffer area containing some boxscore data then get the complete stats for both teams */
            strcpy (&buffer1[0], &buffer[0]);
            if (get_stats (sock, 'v', 1) == -1) {
                strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
                strcat (buf, &nsbserver[0]);
                strcat (buf, ".\n");
                fputs (sp, sfp);
                error = 1;
                break;
            }
            if (get_stats (sock, 'h', 1) == 1) {
                strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
                strcat (buf, &nsbserver[0]);
                strcat (buf, ".\n");
                fputs (sp, sfp);
                error = 1;
                break;
            }
            if (get_stats (sock, 'a', 1) == 1) {
                strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
                strcat (buf, &nsbserver[0]);
                strcat (buf, ".\n");
                fputs (sp, sfp);
                error = 1;
                break;
            }
            if (get_stats (sock, 'b', 1) == 1) {
                strcat (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
                strcat (buf, &nsbserver[0]);
                strcat (buf, ".\n");
                fputs (sp, sfp);
                error = 1;
                break;
            }

            do_boxscore ();
        }
    }

    strcpy (buf, "\n\n");
    fputs (sp, sfp);

    /* dummy gets */
    sock_gets (sock, &buffer[0], sizeof (buffer));

    if (autoplay.standings && !error) {
        /* get standings from server */
        sock_puts (sock, "S1\n");
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            strcpy (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
        }
        if (!strcmp (&buffer[0], "-1")) {
            strcpy (buf, "(NSB League Autoplay) There is not a current league for NSBID ");
            strcat (buf, &nsbid[0]);
            strcat (buf, " on NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
        }

        if (!error)
            DoAutoStandings ();
    }

    strcpy (buf, "\n\n");
    fputs (sp, sfp);

    if (autoplay.injury && !error) {
        sock_puts (sock, "I\n");                       /* ask server for an injury report */
        if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
            strcpy (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
        }

        if (!error)
            DoAutoIR ();
    }

    if (!error)
        if (DoAutoCatLeaders (sock)) {
            strcpy (buf, "(NSB League Autoplay) Something went wrong while communicating with NSB server ");
            strcat (buf, &nsbserver[0]);
            strcat (buf, ".\n");
            fputs (sp, sfp);
            error = 1;
        }

    if (!error) {
        strcpy (buf, "\n\nCompleted 1 day in the schedule of the league for ");
        strcat (buf, &nsbid[0]);
        strcat (buf, " on server ");
        strcat (buf, &nsbserver[0]);
        strcat (buf, ".\n\n");
        fputs (sp, sfp);
    }

GETOUT:
    /* disconnect from server */
    close (sock);
    close (fdlock);

    /* finish email */
    strcpy (buf, "\n\n");
    fputs (sp, sfp);
    fclose (sfp);
}

void
do_boxscore () {
    int innings, x, y, z, inn, bcol, iwork, previ, truns[2], thits[2], addto, terrs[2], pos, indent = 0, singles_g, singles_s, ttb, prevm, line;
    char work[40][500], work1[5], workcur[100], initial;

    strcpy (buf, "\n");

    /* accumulate runs and determine who won game */
    for (x = truns[0] = truns[1] = 0; x < 28; x++) {
        truns[0] += visitor_cur.batters[x].hitting.runs;
        truns[1] += home_cur.batters[x].hitting.runs;
    }

    if (truns[0] > truns[1]) {
        strcat (buf, &entirevteam[0]);
        strcat (buf, " ");
        strcat (buf, (char *) cnvt_int2str (truns[0], 'l'));
        strcat (buf, ", ");
        strcat (buf, &entirehteam[0]);
        strcat (buf, " ");
        strcat (buf, (char *) cnvt_int2str (truns[1], 'l'));
    }
    else {
        strcat (buf, &entirehteam[0]);
        strcat (buf, " ");
        strcat (buf, (char *) cnvt_int2str (truns[1], 'l'));
        strcat (buf, ", ");
        strcat (buf, &entirevteam[0]);
        strcat (buf, " ");
        strcat (buf, (char *) cnvt_int2str (truns[0], 'l'));
    }
    strcat (buf, "\n\n");

    strcat (buf, &entirevteam[0]);
    strcat (buf, " at ");
    strcat (buf, &entirehteam[0]);
    strcat (buf, " ");
    time (&dt);
    dc = *localtime (&dt);
    strcat (buf, (char *) cnvt_int2str ((dc.tm_mon + 1), 'l'));
    strcat (buf, "/");
    strcat (buf, (char *) cnvt_int2str (dc.tm_mday, 'l'));
    strcat (buf, "/");
    strcat (buf, (char *) cnvt_int2str ((dc.tm_year + 1900), 'l'));
    strcat (buf, " -- Final\n\n");

    /* the line score */
    strcpy (&work[0][0], "                                  1  2  3  4  5  6  7  8  9 ");
    work1[0] = buffer1[2];
    work1[1] = buffer1[3];
    work1[2] = '\0';
    innings = atoi (&work1[0]);
    if (innings > 9)
        strcat (&work[0][0], "10");

    strcpy (&work[1][0], &entirevteam[0]);
    if (strlen (&work[1][0]) > 33)
        work[1][32] = '\0';
    else
        strncat (&work[1][0], "                                ", 32 - strlen (&work[1][0]));

    strcpy (&work[2][0], &entirehteam[0]);
    if (strlen (&work[2][0]) > 33)
        work[2][32] = '\0';
    else
        strncat (&work[2][0], "                                ", 32 - strlen (&work[2][0]));

    work1[0] = buffer1[2];
    work1[1] = buffer1[3];
    work1[2] = '\0';
    innings = atoi (&work1[0]);

    for (x = inn = 1, bcol = 4, line = 1; inn <= innings; x++) {
        /* determine line on display to output to */
        if (((x - 1) % 2) == 0)
            addto = 0;
        else
            addto = 1;
        /* if a game goes more than 100 innings we're screwed here ! */
        if (((inn % 10) == 1) && inn != 1 && addto == 0) {
            line += 4;
            work[line - 2][0] = work[line - 1][0] = work[line][0] = work[line + 1][0] = '\0';
            strcat (&work[line - 2][0], "\n");
            strcat (&work[line - 1][0], "                                 ");
            strcat (&work[line][0], "                                ");
            strcat (&work[line + 1][0], "                                ");
            for (y = inn; y <= innings && y < (inn + 10); y++) {
                strcat (&work[line - 1][0], (char *) cnvt_int2str (y, 'l'));
                strcat (&work[line - 1][0], " ");
            }
        }

        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        iwork = atoi (&work1[0]);

        /* check if we played the bottom half of the last inning */
        if (inn == innings && innings >= 9 && !iwork && addto == 1 && truns[1] > truns[0]) {
            strcat (&work[line + addto][0], "  x");
            inn++;
            bcol += 2;
            continue;
        }

        work1[0] = ' ';
        work1[2] = buffer1[bcol + 1];
        work1[3] = '\0';
        if (iwork < 10)
            work1[1] = ' ';
        else
            work1[1] = buffer1[bcol];
        strcat (&work[line + addto][0], &work1[0]);

        bcol += 2;
        if (addto == 1)
            inn++;
    }
    if (innings > 10 && (innings % 10))
        for (x = innings % 10; x < 10; x++) {
            strcat (&work[line - 1][0], "   ");
            strcat (&work[line][0], "   ");
            strcat (&work[line + 1][0], "   ");
        }
    if (innings < 10) {
        strcat (&work[line][0], "   ");
        strcat (&work[line + 1][0], "   ");
        strcat (&work[line - 1][0], "  ");
    }

    /* move total runs */
    if (innings < 11)
        strcat (&work[line - 1][0], " ");
    strcat (&work[line - 1][0], " R  H  E  LOB");
    if (truns[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (truns[0], 'l'));
    if (truns[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (truns[1], 'l'));

    /* move total hits */
    for (x = thits[0] = thits[1] = 0; x < 28; x++) {
        thits[0] += visitor_cur.batters[x].hitting.hits;
        thits[1] += home_cur.batters[x].hitting.hits;
    }
    if (thits[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (thits[0], 'l'));
    if (thits[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (thits[1], 'l'));

    /* move total errors */
    for (x = terrs[0] = terrs[1] = 0; x < 28; x++)
        for (y = 0; y < 11; y++) {
            terrs[0] += visitor_cur.batters[x].fielding[y].e;
            terrs[1] += home_cur.batters[x].fielding[y].e;
        }
    if (terrs[0] > 9)
        strcat (&work[line][0], " ");
    else
        strcat (&work[line][0], "  ");
    strcat (&work[line][0], (char *) cnvt_int2str (terrs[0], 'l'));
    if (terrs[1] > 9)
        strcat (&work[line + 1][0], " ");
    else
        strcat (&work[line + 1][0], "  ");
    strcat (&work[line + 1][0], (char *) cnvt_int2str (terrs[1], 'l'));

    /* move total left-on-base */
    strcat (&work[line][0], "   ");
    work1[2] = '\0';
    work1[1] = buffer1[bcol + 1];
    if (buffer1[bcol] != '0')
        work1[0] = buffer1[bcol];
    else
        work1[0] = ' ';
    strcat (&work[line][0], &work1[0]);
    strcat (&work[line + 1][0], "   ");
    work1[1] = buffer1[bcol + 3];
    if (buffer1[bcol + 2] != '0')
        work1[0] = buffer1[bcol + 2];
    else
        work1[0] = ' ';
    strcat (&work[line + 1][0], &work1[0]);
    bcol += 4;

    for (x = 0; x < (line + 2); x++)
        strcat (&work[x][0], "\n");
    strcat (&work[x][0], "\n");
    for (x = 0; x < (line + 2); x++)
        strcat (buf, &work[x][0]);
    strcat (buf, "\n\n");

    /* end of line score */

    if (!autoplay.boxscore) {
        line = 7;
        goto print_box;
    }

    /* visiting team offensive stats */
    strcpy (&work[0][0], &entirevteam[0]);
    if (strlen (&work[0][0]) > 35)
        work[0][35] = '\0';
    else
        strncat (&work[0][0], "                                   ", 35 - strlen (&work[0][0]));

    strcat (&work[0][0], " AB  R  H BI TB W  K   AVG   OBP   SLG\n");
    strcat (buf, &work[0][0]);

    ttb = prevm = 0;
    previ = 99;
    while (buffer1[bcol] != ':') {
        /* get player pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        if (iwork == previ) {
            prevm = 1;
            goto dup1;
        }
        else {
            prevm = 0;
            previ = iwork;
        }

        /* move player last name and first initial */
        for (y = 0; visitor_cur.batters[iwork].id.name[y] != ','; y++)
            work[0][y] = visitor_cur.batters[iwork].id.name[y];
        work[0][y++] = ',';
        work[0][y++] = ' ';
        work[0][y] = '\0';
        initial = visitor_cur.batters[iwork].id.name[y];
        work1[0] = initial;
        work1[1] = '\0';
        strcat (&work[0][0], &work1[0]);
        strcat (&work[0][0], "  ");
dup1:
        /* move player position */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        pos = atoi (&work1[0]);

        switch (pos) {
            case 0:
               strcpy (&work[1][0], "dh");
               break;
            case 1:
               strcpy (&work[1][0], "p ");
               break;
            case 2:
               strcpy (&work[1][0], "c ");
               break;
            case 3:
               strcpy (&work[1][0], "1b");
               break;
            case 4:
               strcpy (&work[1][0], "2b");
               break;
            case 5:
               strcpy (&work[1][0], "3b");
               break;
            case 6:
               strcpy (&work[1][0], "ss");
               break;
            case 7:
               strcpy (&work[1][0], "lf");
               break;
            case 8:
               strcpy (&work[1][0], "cf");
               break;
            case 9:
               strcpy (&work[1][0], "rf");
               break;
            case 10:
               strcpy (&work[1][0], "ph");
               break;
            case 11:
               strcpy (&work[1][0], "pr");
        }
        if (prevm) {
            for (z = 1; z < strlen (&work[0][0]); z++)
                if (!strncmp (&work[0][z], "  ", 2)) {
                    z += 2;
                    for ( ; z < strlen (&work[0][0]); z++)
                        if (work[0][z] == ' ')
                            break;
                    work[0][z] = '-';
                    work[0][z + 1] = work[1][0];
                    work[0][z + 2] = work[1][1];
                    break;
                }
            goto dup2;
        }

        strcat (&work[0][0], &work[1][0]);
        if (strlen (&work[0][0]) > 36)
            work[0][36] = '\0';
        else
            strncat (&work[0][0], "                                     ", 36 - strlen (&work[0][0]) - indent);

        singles_g = visitor_cur.batters[iwork].hitting.hits -
                    (visitor_cur.batters[iwork].hitting.homers +
                     visitor_cur.batters[iwork].hitting.triples +
                     visitor_cur.batters[iwork].hitting.doubles);
        singles_s = visitor_season.batters[iwork].hitting.hits -
                    (visitor_season.batters[iwork].hitting.homers +
                     visitor_season.batters[iwork].hitting.triples +
                     visitor_season.batters[iwork].hitting.doubles);

        /* move individual stats */
        if (visitor_cur.batters[iwork].hitting.atbats < 10)
            strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.atbats, 'l'));
        if (visitor_cur.batters[iwork].hitting.runs > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.runs, 'l'));
        if (visitor_cur.batters[iwork].hitting.hits > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.hits, 'l'));
        if (visitor_cur.batters[iwork].hitting.rbi > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.rbi, 'l'));
        if (((visitor_cur.batters[iwork].hitting.homers * 4) +
             (visitor_cur.batters[iwork].hitting.triples * 3) +
             (visitor_cur.batters[iwork].hitting.doubles * 2) + singles_g) > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (((visitor_cur.batters[iwork].hitting.homers * 4) +
                    (visitor_cur.batters[iwork].hitting.triples * 3) + (visitor_cur.batters[iwork].hitting.doubles * 2) +
                     singles_g), 'l'));
        ttb += ((visitor_cur.batters[iwork].hitting.homers * 4) + (visitor_cur.batters[iwork].hitting.triples * 3) +
                (visitor_cur.batters[iwork].hitting.doubles * 2) + singles_g);

        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.bb, 'l'));
        strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (visitor_cur.batters[iwork].hitting.so, 'l'));

        strcat (&work[0][0], (char *) do_average (visitor_season.batters[iwork].hitting.hits,
                      visitor_season.batters[iwork].hitting.atbats));
        strcat (&work[0][0], (char *) do_average ((visitor_season.batters[iwork].hitting.hits +
                         visitor_season.batters[iwork].hitting.bb + visitor_season.batters[iwork].hitting.hbp),
                    (visitor_season.batters[iwork].hitting.atbats + visitor_season.batters[iwork].hitting.bb +
                         visitor_season.batters[iwork].hitting.sf + visitor_season.batters[iwork].hitting.hbp)));
        strcat (&work[0][0], (char *) do_average (((visitor_season.batters[iwork].hitting.homers * 4) +
               (visitor_season.batters[iwork].hitting.triples * 3) + (visitor_season.batters[iwork].hitting.doubles * 2) +
                singles_s), visitor_season.batters[iwork].hitting.atbats));
dup2:
        /* check for end of players in this batting order position */
        if (buffer1[bcol] == ':') {
            for (y = 0; y < indent; y++)
                strcat (buf, " ");
            strcat (buf, &work[0][0]);
            strcat (buf, "\n");

            bcol++;
            indent = 0;
        }
        else {
            work1[0] = buffer1[bcol];
            work1[1] = buffer1[bcol + 1];
            work1[2] = '\0';
            iwork = atoi (&work1[0]);
            if (iwork != previ) {
                for (y = 0; y < indent; y++)
                    strcat (buf, " ");
                strcat (buf, &work[0][0]);
                strcat (buf, "\n");
            }
            indent = 2;
        }
    }
    strcat (buf, "\n");

    /* put out totals */
    strcat (buf, "TOTALS                              ");
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.atbats;
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.runs;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.hits;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.rbi;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    if (ttb > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (ttb, 'l'));
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.bb;
    strcat (buf, " ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    for (x = z = 0; x < 28; x++)
        z += visitor_cur.batters[x].hitting.so;
    strcat (buf, (char *) cnvt_int2str (z, 'l'));

    strcat (buf, "\n\n\n");

    /* home team offensive stats */
    strcpy (&work[0][0], &entirehteam[0]);
    if (strlen (&work[0][0]) > 35)
        work[0][35] = '\0';
    else
        strncat (&work[0][0], "                                   ", 35 - strlen (&work[0][0]));

    strcat (&work[0][0], " AB  R  H BI TB W  K   AVG   OBP   SLG\n");
    strcat (buf, &work[0][0]);

    ttb = prevm = 0;
    previ = 99;
    bcol++;
    while (buffer1[bcol] != ':') {
        /* get player pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        if (iwork == previ) {
            prevm = 1;
            goto dup3;
        }
        else {
            prevm = 0;
            previ = iwork;
        }

        /* move player last name and first initial */
        for (y = 0; home_cur.batters[iwork].id.name[y] != ','; y++)
            work[0][y] = home_cur.batters[iwork].id.name[y];
        work[0][y++] = ',';
        work[0][y++] = ' ';
        work[0][y] = '\0';
        initial = home_cur.batters[iwork].id.name[y];
        work1[0] = initial;
        work1[1] = '\0';
        strcat (&work[0][0], &work1[0]);
        strcat (&work[0][0], " ");
        strcat (&work[0][0], " ");
dup3:
        /* move player position */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        pos = atoi (&work1[0]);
        switch (pos) {
            case 0:
               work[1][0] = 'd';
               work[1][1] = 'h';
               break;
            case 1:
               work[1][0] = 'p';
               work[1][1] = ' ';
               break;
            case 2:
               work[1][0] = 'c';
               work[1][1] = ' ';
               break;
            case 3:
               work[1][0] = '1';
               work[1][1] = 'b';
               break;
            case 4:
               work[1][0] = '2';
               work[1][1] = 'b';
               break;
            case 5:
               work[1][0] = '3';
               work[1][1] = 'b';
               break;
            case 6:
               work[1][0] = 's';
               work[1][1] = 's';
               break;
            case 7:
               work[1][0] = 'l';
               work[1][1] = 'f';
               break;
            case 8:
               work[1][0] = 'c';
               work[1][1] = 'f';
               break;
            case 9:
               work[1][0] = 'r';
               work[1][1] = 'f';
               break;
            case 10:
               work[1][0] = 'p';
               work[1][1] = 'h';
               break;
            case 11:
               work[1][0] = 'p';
               work[1][1] = 'r';
        }
        work[1][2] = '\0';
        if (prevm) {
            for (z = 1; z < strlen (&work[0][0]); z++)
                if (!strncmp (&work[0][z], "  ", 2)) {
                    z += 2;
                    for ( ; z < strlen (&work[0][0]); z++)
                        if (work[0][z] == ' ')
                            break;
                    work[0][z] = '-';
                    work[0][z + 1] = work[1][0];
                    work[0][z + 2] = work[1][1];
                    break;
                }
            goto dup4;
        }

        strcat (&work[0][0], &work[1][0]);
        if (strlen (&work[0][0]) > 36)
            work[0][36] = '\0';
        else
            strncat (&work[0][0], "                                     ", 36 - strlen (&work[0][0]) - indent);

        singles_g = home_cur.batters[iwork].hitting.hits -
                    (home_cur.batters[iwork].hitting.homers +
                     home_cur.batters[iwork].hitting.triples +
                     home_cur.batters[iwork].hitting.doubles);
        singles_s = home_season.batters[iwork].hitting.hits -
                    (home_season.batters[iwork].hitting.homers +
                     home_season.batters[iwork].hitting.triples +
                     home_season.batters[iwork].hitting.doubles);

        /* move individual stats */
        if (home_cur.batters[iwork].hitting.atbats < 10)
            strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.atbats, 'l'));
        if (home_cur.batters[iwork].hitting.runs > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.runs, 'l'));
        if (home_cur.batters[iwork].hitting.hits > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.hits, 'l'));
        if (home_cur.batters[iwork].hitting.rbi > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.rbi, 'l'));
        if (((home_cur.batters[iwork].hitting.homers * 4) +
             (home_cur.batters[iwork].hitting.triples * 3) +
             (home_cur.batters[iwork].hitting.doubles * 2) + singles_g) > 9)
            strcat (&work[0][0], " ");
        else
            strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (((home_cur.batters[iwork].hitting.homers * 4) +
                    (home_cur.batters[iwork].hitting.triples * 3) + (home_cur.batters[iwork].hitting.doubles * 2) +
                     singles_g), 'l'));
        ttb += ((home_cur.batters[iwork].hitting.homers * 4) + (home_cur.batters[iwork].hitting.triples * 3) +
                (home_cur.batters[iwork].hitting.doubles * 2) + singles_g);

        strcat (&work[0][0], " ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.bb, 'l'));
        strcat (&work[0][0], "  ");
        strcat (&work[0][0], (char *) cnvt_int2str (home_cur.batters[iwork].hitting.so, 'l'));

        strcat (&work[0][0], (char *) do_average (home_season.batters[iwork].hitting.hits,
                      home_season.batters[iwork].hitting.atbats));
        strcat (&work[0][0], (char *) do_average ((home_season.batters[iwork].hitting.hits +
                         home_season.batters[iwork].hitting.bb + home_season.batters[iwork].hitting.hbp),
                    (home_season.batters[iwork].hitting.atbats + home_season.batters[iwork].hitting.bb +
                         home_season.batters[iwork].hitting.sf + home_season.batters[iwork].hitting.hbp)));
        strcat (&work[0][0], (char *) do_average (((home_season.batters[iwork].hitting.homers * 4) +
               (home_season.batters[iwork].hitting.triples * 3) + (home_season.batters[iwork].hitting.doubles * 2) +
                singles_s), home_season.batters[iwork].hitting.atbats));
dup4:
        /* check for end of players in this batting order position */
        if (buffer1[bcol] == ':') {
            for (y = 0; y < indent; y++)
                strcat (buf, " ");
            strcat (buf, &work[0][0]);
            strcat (buf, "\n");

            bcol++;
            indent = 0;
        }
        else {
            work1[0] = buffer1[bcol];
            work1[1] = buffer1[bcol + 1];
            work1[2] = '\0';
            iwork = atoi (&work1[0]);
            if (iwork != previ) {
                for (y = 0; y < indent; y++)
                    strcat (buf, " ");
                strcat (buf, &work[0][0]);
                strcat (buf, "\n");
            }
            indent = 2;
        }
    }
    strcat (buf, "\n");

    /* put out totals */
    strcat (buf, "TOTALS                              ");
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.atbats;
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.runs;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.hits;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.rbi;
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    if (ttb > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    strcat (buf, (char *) cnvt_int2str (ttb, 'l'));
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.bb;
    strcat (buf, " ");
    strcat (buf, (char *) cnvt_int2str (z, 'l'));
    if (z > 9)
        strcat (buf, " ");
    else
        strcat (buf, "  ");
    for (x = z = 0; x < 28; x++)
        z += home_cur.batters[x].hitting.so;
    strcat (buf, (char *) cnvt_int2str (z, 'l'));

    strcat (buf, "\n\n");

    put_details_auto (2);
    put_details_auto (3);
    put_details_auto (4);
    put_details_auto (5);
    put_details_auto (6);
    put_details_auto (7);
    put_details_auto (8);
    put_details_auto (9);

    strcat (buf, "\n");

    strcpy (&workcur[0], &entirevteam[0]);
    if (strlen (&workcur[0]) > 34)
        workcur[34] = '\0';
    else
        strncat (&workcur[0], "                                   ", 34 - strlen (&workcur[0]));
    strcat (buf, &workcur[0]);

    strcat (buf, "  IP   H  R ER BB  K HR BF    ERA  OpBA\n");

    bcol++;
    while (buffer1[bcol] != ':') {
        /* get pitcher pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        /* move pitcher last name and first initial */
        for (y = 0; visitor_cur.pitchers[iwork].id.name[y] != ','; y++)
            work[0][y] = visitor_cur.pitchers[iwork].id.name[y];
        work[0][y++] = '\0';
        initial = visitor_cur.pitchers[iwork].id.name[y + 1];
        workcur[0] = initial;
        strcpy (&workcur[1], " ");
        strcat (&workcur[0], &work[0][0]);
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.wins != 0)
            strcat (&workcur[0], "(W, ");
        if (visitor_cur.pitchers[iwork].pitching.losses != 0)
            strcat (&workcur[0], "(L, ");
        if (visitor_cur.pitchers[iwork].pitching.wins != 0 || visitor_cur.pitchers[iwork].pitching.losses != 0) {
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.wins, 'l'));
            strcat (&workcur[0], "-");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.losses, 'l'));
            strcat (&workcur[0], ")");
        }
        if (visitor_cur.pitchers[iwork].pitching.saves != 0) {
            strcat (&workcur[0], "(S, ");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_season.pitchers[iwork].pitching.saves, 'l'));
            strcat (&workcur[0], ")");
        }

        if (strlen (&workcur[0]) > 35)
            workcur[35] = '\0';
        else
            strncat (&workcur[0], "                                     ", 35 - strlen (&workcur[0]));

        /* move individual stats */
        if (visitor_cur.pitchers[iwork].pitching.innings < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.innings, 'l'));
        if (visitor_cur.pitchers[iwork].pitching.thirds > 0) {
            strcat (&workcur[0], ".");
            strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.thirds, 'l'));
        }
        else
            strcat (&workcur[0], "  ");
        if (visitor_cur.pitchers[iwork].pitching.hits < 10)
            strcat (&workcur[0], "  ");
        else
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.hits, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.runs < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.runs, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.er < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.er, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.walks < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.walks, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.so < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.so, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.homers < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.homers, 'l'));
        strcat (&workcur[0], " ");
        if (visitor_cur.pitchers[iwork].pitching.bfp < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (visitor_cur.pitchers[iwork].pitching.bfp, 'l'));
        strcat (&workcur[0], " ");

        strcat (&workcur[0], (char *) do_era (visitor_season.pitchers[iwork].pitching.er * 9,
                 visitor_season.pitchers[iwork].pitching.innings, visitor_season.pitchers[iwork].pitching.thirds));
        strcat (&workcur[0], (char *) do_average (visitor_season.pitchers[iwork].pitching.hits,
                 visitor_season.pitchers[iwork].pitching.opp_ab));
        strcat (buf, &workcur[0]);
        strcat (buf, "\n");
    }

    strcat (buf, "\n");

    strcpy (&workcur[0], &entirehteam[0]);
    if (strlen (&workcur[0]) > 34)
        workcur[34] = '\0';
    else
        strncat (&workcur[0], "                                   ", 34 - strlen (&workcur[0]));

    strcat (&workcur[0], "  IP   H  R ER BB  K HR BF    ERA  OpBA\n");
    strcat (buf, &workcur[0]);

    bcol++;
    while (buffer1[bcol] != ':') {
        /* get pitcher pointer */
        work1[0] = buffer1[bcol];
        work1[1] = buffer1[bcol + 1];
        work1[2] = '\0';
        bcol += 2;
        iwork = atoi (&work1[0]);
        /* move pitcher last name and first initial */
        for (y = 0; home_cur.pitchers[iwork].id.name[y] != ','; y++)
            work[0][y] = home_cur.pitchers[iwork].id.name[y];
        work[0][y++] = '\0';
        initial = home_cur.pitchers[iwork].id.name[y + 1];
        workcur[0] = initial;
        strcpy (&workcur[1], " ");
        strcat (&workcur[0], &work[0][0]);
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.wins != 0)
            strcat (&workcur[0], "(W, ");
        if (home_cur.pitchers[iwork].pitching.losses != 0)
            strcat (&workcur[0], "(L, ");
        if (home_cur.pitchers[iwork].pitching.wins != 0 || home_cur.pitchers[iwork].pitching.losses != 0) {
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.wins, 'l'));
            strcat (&workcur[0], "-");
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.losses, 'l'));
            strcat (&workcur[0], ")");
        }
        if (home_cur.pitchers[iwork].pitching.saves != 0) {
            strcat (&workcur[0], "(S, ");
            strcat (&workcur[0], (char *) cnvt_int2str (home_season.pitchers[iwork].pitching.saves, 'l'));
            strcat (&workcur[0], ")");
        }

        if (strlen (&workcur[0]) > 35)
            workcur[35] = '\0';
        else
            strncat (&workcur[0], "                                     ", 35 - strlen (&workcur[0]));

        /* move individual stats */
        if (home_cur.pitchers[iwork].pitching.innings < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.innings, 'l'));
        if (home_cur.pitchers[iwork].pitching.thirds > 0) {
            strcat (&workcur[0], ".");
            strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.thirds, 'l'));
        }
        else
            strcat (&workcur[0], "  ");
        if (home_cur.pitchers[iwork].pitching.hits < 10)
            strcat (&workcur[0], "  ");
        else
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.hits, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.runs < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.runs, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.er < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.er, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.walks < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.walks, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.so < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.so, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.homers < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.homers, 'l'));
        strcat (&workcur[0], " ");
        if (home_cur.pitchers[iwork].pitching.bfp < 10)
            strcat (&workcur[0], " ");
        strcat (&workcur[0], (char *) cnvt_int2str (home_cur.pitchers[iwork].pitching.bfp, 'l'));
        strcat (&workcur[0], " ");

        strcat (&workcur[0], (char *) do_era (home_season.pitchers[iwork].pitching.er * 9,
                 home_season.pitchers[iwork].pitching.innings, home_season.pitchers[iwork].pitching.thirds));
        strcat (&workcur[0], (char *) do_average (home_season.pitchers[iwork].pitching.hits,
                 home_season.pitchers[iwork].pitching.opp_ab));
        strcat (buf, &workcur[0]);
        strcat (buf, "\n");
    }
    strcat (buf, "\n\n");

    put_details_auto (10);
    put_details_auto (11);
    put_details_auto (12);
    put_details_auto (13);

print_box:
    strcat (buf, "\n");
    fputs (sp, sfp);
    strcpy (buf, "\n");
    fputs (sp, sfp);
}

void
put_details_auto (int which) {
    int w, x, y, z, limit, perrs[4][28];
    char work[80], work1[5], workcur[100];

    for (x = 0; x < 4; x++)
        for (y = 0; y < 28; y++)
            perrs[x][y] = 0;

    for (x = y = z = 0; x < 28; x++)
        switch (which) {
            case 2:
                y += visitor_cur.batters[x].hitting.doubles;
                z += home_cur.batters[x].hitting.doubles;
                break;
            case 3:
                y += visitor_cur.batters[x].hitting.triples;
                z += home_cur.batters[x].hitting.triples;
                break;
            case 4:
                y += visitor_cur.batters[x].hitting.homers;
                z += home_cur.batters[x].hitting.homers;
                break;
            case 5:
                y += visitor_cur.batters[x].hitting.sf;
                z += home_cur.batters[x].hitting.sf;
                break;
            case 6:
                y += visitor_cur.batters[x].hitting.sh;
                z += home_cur.batters[x].hitting.sh;
                break;
            case 7:
                y += visitor_cur.batters[x].hitting.gidp;
                z += home_cur.batters[x].hitting.gidp;
                break;
            case 8:
                y += visitor_cur.batters[x].hitting.sb;
                z += home_cur.batters[x].hitting.sb;
                break;
            case 9:
                y += visitor_cur.batters[x].hitting.cs;
                z += home_cur.batters[x].hitting.cs;
                break;
            case 10:
                for (w = 0; w < 11; w++) {
                    perrs[0][x] += visitor_cur.batters[x].fielding[w].e;
                    perrs[2][x] += visitor_season.batters[x].fielding[w].e;
                    y += visitor_cur.batters[x].fielding[w].e;
                    perrs[1][x] += home_cur.batters[x].fielding[w].e;
                    perrs[3][x] += home_season.batters[x].fielding[w].e;
                    z += home_cur.batters[x].fielding[w].e;
                }
                break;
            case 11:
                y += visitor_cur.batters[x].hitting.hbp;
                z += home_cur.batters[x].hitting.hbp;
                break;
            case 13:
                y += visitor_cur.batters[x].fielding[2].pb;
                z += home_cur.batters[x].fielding[2].pb;
        }

    if (which == 12)
        for (x = 0; x < 13; x++) {
            y += visitor_cur.pitchers[x].pitching.wp;
            z += home_cur.pitchers[x].pitching.wp;
        }

    if ((y + z) == 0)
        return;

    switch (which) {
        case 2:
            strcat (buf, "DOUBLES\n");
            break;
        case 3:
            strcat (buf, "TRIPLES\n");
            break;
        case 4:
            strcat (buf, "HOME RUNS\n");
            break;
        case 5:
            strcat (buf, "SACRIFICE FLIES\n");
            break;
        case 6:
            strcat (buf, "SACRIFICES\n");
            break;
        case 7:
            strcat (buf, "GROUNDED INTO DOUBLE PLAYS\n");
            break;
        case 8:
            strcat (buf, "STOLEN BASES\n");
            break;
        case 9:
            strcat (buf, "CAUGHT STEALING\n");
            break;
        case 10:
            strcat (buf, "ERRORS\n");
            break;
        case 11:
            strcat (buf, "HIT BY PITCH\n");
            break;
        case 12:
            strcat (buf, "WILD PITCHES\n");
            break;
        case 13:
            strcat (buf, "PASSED BALLS\n");
    } 
    strcpy (&workcur[0], "  ");
    strcat (&workcur[0], &entirevteam[0]);
    strcat (&workcur[0], " ");
    strcat (&workcur[0], (char *) cnvt_int2str (y, 'l'));
    if (y != 0) {
        strcat (&workcur[0], ":  ");
        if (which == 12)
            limit = 13;
        else
            limit = 28;
        for (x = 0; x < limit; x++) {
            if ((which == 2 && visitor_cur.batters[x].hitting.doubles != 0) ||
                      (which == 3 && visitor_cur.batters[x].hitting.triples != 0) ||
                      (which == 4 && visitor_cur.batters[x].hitting.homers != 0) ||
                      (which == 5 && visitor_cur.batters[x].hitting.sf != 0) ||
                      (which == 6 && visitor_cur.batters[x].hitting.sh != 0) ||
                      (which == 7 && visitor_cur.batters[x].hitting.gidp != 0) ||
                      (which == 8 && visitor_cur.batters[x].hitting.sb != 0) ||
                      (which == 9 && visitor_cur.batters[x].hitting.cs != 0) ||
                      (which == 10 && perrs[0][x] != 0) || (which == 11 && visitor_cur.batters[x].hitting.hbp != 0) ||
                      (which == 12 && visitor_cur.pitchers[x].pitching.wp != 0) ||
                      (which == 13 && visitor_cur.batters[x].fielding[2].pb != 0)) {
                if (which == 12)
                    for (y = 0; visitor_cur.pitchers[x].id.name[y] != ','; y++)
                        work[y] = visitor_cur.pitchers[x].id.name[y];
                else
                    for (y = 0; visitor_cur.batters[x].id.name[y] != ','; y++)
                        work[y] = visitor_cur.batters[x].id.name[y];
                work[y++] = ',';
                work[y] = '\0';
                if (which == 12)
                    work1[0] = visitor_cur.pitchers[x].id.name[y + 1];
                else
                    work1[0] = visitor_cur.batters[x].id.name[y + 1];
                work1[1] = '\0';
                if ((strlen (&workcur[0]) + strlen (&work[0]) + strlen (&work1[0])) > 70) {
                    strcat (buf, &workcur[0]);
                    strcat (buf, "\n");
                    if (vteamyr[0] != '0') {
                        strncpy (&workcur[0], "                                                  ", (strlen (GetTeamName (VisitingTeamID)) + 12));
                        workcur[strlen (GetTeamName (VisitingTeamID)) + 12] = '\0';
                    }
                    else {
                        strncpy (&workcur[0], "                                                  ", (strlen (&visiting_team[0]) + 7));
                        workcur[strlen (&visiting_team[0]) + 7] = '\0';
                    }
                }
                strcat (&workcur[0], &work1[0]);
                strcat (&workcur[0], " ");
                strcat (&workcur[0], &work[0]);
                strcat (&workcur[0], " ");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[0][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (visitor_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], " (");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[2][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (visitor_season.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], "), ");
            }
        }
        workcur[strlen (&workcur[0]) - 2] = '.';
    }
    else
        strcat (&workcur[0], ".");

    strcat (buf, &workcur[0]);
    strcat (buf, "\n");

    strcpy (&workcur[0], "  ");
    strcat (&workcur[0], &entirehteam[0]);
    strcat (&workcur[0], " ");
    strcat (&workcur[0], (char *) cnvt_int2str (z, 'l'));
    if (z != 0) {
        strcat (&workcur[0], ":  ");
        if (which == 12)
            limit = 13;
        else
            limit = 28;
        for (x = 0; x < limit; x++) {
            if ((which == 2 && home_cur.batters[x].hitting.doubles != 0) ||
                      (which == 3 && home_cur.batters[x].hitting.triples != 0) ||
                      (which == 4 && home_cur.batters[x].hitting.homers != 0) ||
                      (which == 5 && home_cur.batters[x].hitting.sf != 0) ||
                      (which == 6 && home_cur.batters[x].hitting.sh != 0) ||
                      (which == 7 && home_cur.batters[x].hitting.gidp != 0) ||
                      (which == 8 && home_cur.batters[x].hitting.sb != 0) ||
                      (which == 9 && home_cur.batters[x].hitting.cs != 0) ||
                      (which == 10 && perrs[1][x] != 0) || (which == 11 && home_cur.batters[x].hitting.hbp != 0) ||
                      (which == 12 && home_cur.pitchers[x].pitching.wp != 0) ||
                      (which == 13 && home_cur.batters[x].fielding[2].pb != 0)) {
                if (which == 12)
                    for (y = 0; home_cur.pitchers[x].id.name[y] != ','; y++)
                        work[y] = home_cur.pitchers[x].id.name[y];
                else
                    for (y = 0; home_cur.batters[x].id.name[y] != ','; y++)
                        work[y] = home_cur.batters[x].id.name[y];
                work[y++] = ',';
                work[y] = '\0';
                if (which == 12)
                    work1[0] = home_cur.pitchers[x].id.name[y + 1];
                else
                    work1[0] = home_cur.batters[x].id.name[y + 1];
                work1[1] = '\0';
                if ((strlen (&workcur[0]) + strlen (&work[0]) + strlen (&work1[0])) > 70) {
                    strcat (buf, &workcur[0]);
                    strcat (buf, "\n");
                    if (hteamyr[0] != '0') {
                        strncpy (&workcur[0], "                                                  ", (strlen (GetTeamName (HomeTeamID)) + 12));
                        workcur[strlen (GetTeamName (HomeTeamID)) + 12] = '\0';
                    }
                    else {
                        strncpy (&workcur[0], "                                                  ", (strlen (&home_team[0]) + 7));
                        workcur[strlen (&home_team[0]) + 7] = '\0';
                    }
                }
                strcat (&workcur[0], &work1[0]);
                strcat (&workcur[0], " ");
                strcat (&workcur[0], &work[0]);
                strcat (&workcur[0], " ");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[1][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (home_cur.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], " (");
                switch (which) {
                    case 2:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.doubles, 'l'));
                        break;
                    case 3:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.triples, 'l'));
                        break;
                    case 4:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.homers, 'l'));
                        break;
                    case 5:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sf, 'l'));
                        break;
                    case 6:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sh, 'l'));
                        break;
                    case 7:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.gidp, 'l'));
                        break;
                    case 8:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.sb, 'l'));
                        break;
                    case 9:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.cs, 'l'));
                        break;
                    case 10:
                        strcat (&workcur[0], cnvt_int2str (perrs[3][x], 'l'));
                        break;
                    case 11:
                        strcat (&workcur[0], cnvt_int2str (home_season.batters[x].hitting.hbp, 'l'));
                        break;
                    case 12:
                        strcat (&workcur[0], cnvt_int2str (home_season.pitchers[x].pitching.wp, 'l'));
                        break;
                    case 13:
                        strcat (&workcur[0], cnvt_int2str (home_cur.batters[x].fielding[2].pb, 'l'));
                }
                strcat (&workcur[0], "), ");
            }
        }
        workcur[strlen (&workcur[0]) - 2] = '.';
    }
    else
        strcat (&workcur[0], ".");

    strcat (buf, &workcur[0]);
    strcat (buf, "\n\n");
}

void
DoAutoStandings () {
    char *cc, *work, teamy[5], teamid[5], *work2 = NULL, tuctname[50];
    int x, y = 0, nteams[6], loop, al1sw, nl1sw, ALNL, overpssw, ty[2], tid[2];
    struct {
        int id, year, wins, losses, gb;
        float pct;
        char ucteamname[50];
    } teams[6][50], teamh;

    tid[0] = tid[1] = ty[0] = ty[1] = 0;
    for (loop = 0; loop < 6; loop++) {
        nteams[loop] = 0;
        for (x = 0; x < 50; x++)
            teams[loop][x].id = 0;
    }
    cc = &buffer[0];
    /* store team id, wins, and losses calculate pct */
    for (x = 0; cc < (&buffer[0] + strlen (&buffer[0])) && *cc != ':'; x++) {
        if (*cc == 'A')
            if (*(cc + 1) == 'E')
                loop = 0;
            else
                if (*(cc + 1) == 'C')
                    loop = 1;
                else
                    loop = 2;
        else
            if (*(cc + 1) == 'E')
                loop = 3;
            else
                if (*(cc + 1) == 'C')
                    loop = 4;
                else
                    loop = 5;

        cc += 2;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teamy[0] = *cc;
        teamy[1] = *(cc + 1);
        teamy[2] = *(cc + 2);
        teamy[3] = *(cc + 3);
        teamy[4] = '\0';
        teamid[0] = *(cc + 4);
        teamid[1] = *(cc + 5);
        teamid[2] = *(cc + 6);
        teamid[3] = '\0';
        teams[loop][nteams[loop]].year = atoi (&teamy[0]);
        teams[loop][nteams[loop]].id = atoi (&teamid[0]);
        if (!teams[loop][nteams[loop]].year)
            strcpy (&teams[loop][nteams[loop]].ucteamname[0], cc + 7);
        *work = ' ';
        cc = work + 1;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teams[loop][nteams[loop]].wins = atoi (cc);
        *work = ' ';
        cc = work + 1;
        work = (char *) index (cc, ' ');
        *work = '\0';
        teams[loop][nteams[loop]].losses = atoi (cc);
        *work = ' ';
        cc = work + 1;

        teams[loop][nteams[loop]].pct = (float) teams[loop][nteams[loop]].wins / (float) (teams[loop][nteams[loop]].wins + teams[loop][nteams[loop]].losses);
        if (isnanf (teams[loop][nteams[loop]].pct))
            teams[loop][nteams[loop]].pct = 0.0;

        nteams[loop]++;
    }

    for (loop = 0; loop < 6; loop++) {
        if (!nteams[loop])
            continue;

        /* sort by primary pct, secondary losses */
        for (x = 0; x < (nteams[loop] - 1); x++)
            for (y = x + 1; y < nteams[loop]; y++)
                if (teams[loop][x].pct < teams[loop][y].pct) {
                    teamh = teams[loop][x];
                    teams[loop][x] = teams[loop][y];
                    teams[loop][y] = teamh;
                }
                else
                    if (teams[loop][x].pct == teams[loop][y].pct && teams[loop][x].losses > teams[loop][y].losses) {
                        teamh = teams[loop][x];
                        teams[loop][x] = teams[loop][y];
                        teams[loop][y] = teamh;
                    }
        /* figure games back */
        for (x = 0; x < nteams[loop]; x++)
            teams[loop][x].gb = teams[loop][x].wins - teams[loop][x].losses;
        for (y = x = 0; x < nteams[loop]; x++)
            if (teams[loop][x].gb > y)
                y = teams[loop][x].gb;
        for (x = 0; x < nteams[loop]; x++)
            teams[loop][x].gb = abs (teams[loop][x].gb - y);
    }

    /* check for a possible reversal of teams */
    for (work = index (&buffer[0], ':') + 2; work < (&buffer[0] + strlen (&buffer[0])); ) {
        if (*work == ':' || *work == '1' || *work == '2' || *work == '3' || *work == '6')
            break;
        work += 3;
        if (*work == '5' || *work == '7') {
            strncpy (&teamy[0], work + 1, 4);
            teamy[4] = '\0';
            ty[0] = atoi (&teamy[0]);
            if (ty[0]) {
                strncpy (&teamid[0], work + 5, 4);
                teamid[4] = '\0';
                tid[0] = atoi (&teamid[0]);
                work += 9;
            }
            else {
                int s1, s2;

                work2 = work;
                work = (char *) index (work, ' ');
                *work = '\0';
                strcpy (&tuctname[0], work2 + 5);
                *work = ' ';
                work++;
                for (s1 = 0; s1 < 6; s1++)
                    for (s2 = 0; s2 < 300; s2++)
                        if (!strcmp (&teams[s1][s2].ucteamname[0], &tuctname[0]))
                            tid[0] = teams[s1][s2].id;
            }

            strncpy (&teamy[0], work, 4);
            teamy[4] = '\0';
            ty[1] = atoi (&teamy[0]);
            if (ty[1]) {
                strncpy (&teamid[0], work + 4, 4);
                teamid[4] = '\0';
                tid[1] = atoi (&teamid[0]);
            }
            else {
                int s1, s2;

                work2 = work;
                work = (char *) index (work, ' ');
                *work = '\0';
                strcpy (&tuctname[0], work2 + 4);
                *work = ' ';
                for (s1 = 0; s1 < 6; s1++)
                    for (s2 = 0; s2 < 300; s2++)
                        if (!strcmp (&teams[s1][s2].ucteamname[0], &tuctname[0]))
                            tid[1] = teams[s1][s2].id;
            }

            for (loop = 0; loop < 6; loop++) {
                if (!nteams[loop])
                    continue;
                for (x = 0; x < (nteams[loop]); x++)
                    if (teams[loop][x].year == ty[1] && teams[loop][x].id == tid[1] && teams[loop][x + 1].year == ty[0] && teams[loop][x + 1].id == tid[0]) {
                        teamh = teams[loop][x];
                        teams[loop][x] = teams[loop][x + 1];
                        teams[loop][x + 1] = teamh;
                        break;
                    }
            }
        }
        for (work = index (work, ':'); *work == ':'; work++);
    }

    for (al1sw = x = 0; x < 3; x++)
        if (nteams[x])
            al1sw++;
    for (nl1sw = 0, x = 3; x < 6; x++)
        if (nteams[x])
            nl1sw++;

    strcpy (buf, "Current NSB Standings:\n\n");

    for (loop = 0; loop < 6; loop++) {
        if (!nteams[loop])
            continue;
        strcat (buf, "\n");
        switch (loop) {
            case 0:
                if (al1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              American League");
                if (al1sw > 1)
                    strcat (buf, ", Eastern Division");
                break;
            case 1:
                if (al1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              American League");
                if (al1sw > 1)
                    strcat (buf, ", Central Division");
                break;
            case 2:
                if (al1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              American League");
                if (al1sw > 1)
					strcat (buf, ", Western Division");
                break;
            case 3:
                if (nl1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              National League");
                if (nl1sw > 1)
                    strcat (buf, ", Eastern Division");
                break;
            case 4:
                if (nl1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              National League");
                if (nl1sw > 1)
                    strcat (buf, ", Central Division");
                break;
            default:
                if (nl1sw == 1)
                    strcat (buf, "         ");
                strcat (buf, "              National League");
                if (nl1sw > 1)
                    strcat (buf, ", Western Division");
        }
        fputs (sp, sfp);

        strcpy (buf, "\n\n     Team                           W      L      PCT   Games Back\n\n");
        fputs (sp, sfp);

        for (x = 0; teams[loop][x].id != 0; x++) {
            /* move Team Year and Name */
            if (teams[loop][x].id < 900) {
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == teams[loop][x].id) {
                        strcpy (buf, (char *) cnvt_int2str ((teams[loop][x].year), 'l'));
                        break;
                    }
            }
            else
                strcpy (buf, "    ");
            strcat (buf, " ");
            if (teams[loop][x].id < 900) {
                strcat (buf, &teaminfo[y].teamname[0]);
                strncat (buf, "                             ", (29 - strlen (&teaminfo[y].teamname[0])));
            }
            else {
                strcat (buf, &teams[loop][x].ucteamname[0]);
                strncat (buf, "                             ", (29 - strlen (&teams[loop][x].ucteamname[0])));
            }

            if (teams[loop][x].wins < 100)
                strcat (buf, " ");
            if (teams[loop][x].wins < 10)
                strcat (buf, " ");
            /* move wins and losses */
            strcat (buf, (char *) cnvt_int2str ((teams[loop][x].wins), 'l'));
            strcat (buf, "   ");
            if (teams[loop][x].losses < 10)
                strcat (buf, "  ");
            else
                strcat (buf, " ");
            if (teams[loop][x].losses < 100)
                strcat (buf, " ");
            strcat (buf, (char *) cnvt_int2str ((teams[loop][x].losses), 'l'));
            strcat (buf, "   ");
            /* move Pct */
            strcat (buf, (char *) do_average (teams[loop][x].wins, (teams[loop][x].wins + teams[loop][x].losses)));
            if ((teams[loop][x].gb / 2) < 10 && (teams[loop][x].gb / 2))
                strcat (buf, "       ");
            else
                strcat (buf, "      ");
            /* move Games Back */
            if (teams[loop][x].gb / 2)
                strcat (buf, (char *) cnvt_int2str ((teams[loop][x].gb / 2), 'l'));
            if (teams[loop][x].gb % 2) {
                if (teams[loop][x].gb / 2)
                    strcat (buf, ".5");
                else
                    strcat (buf, "  .5");
            }
            if (!(teams[loop][x].gb / 2) && !(teams[loop][x].gb % 2))
                strcat (buf, " -");
            strcat (buf, "\n");
            fputs (sp, sfp);
            buf[0] = '\0';
        }
        strcpy (buf, "\n");
        fputs (sp, sfp);
    }

    /* process extra info */
    cc = index (&buffer[0], ':');
    cc += 2;
    if (*cc == '0')
        /* nothing to do */
        return;

    strcpy (buf, "\n\n");
    fputs (sp, sfp);

    strcpy (buf, "\n");
Check4Playoffs:
    ALNL = 1;
    if (*cc == '4' || *cc == '5') {
        /* playoff results */
        gint x, flip;
        gchar type;

        if (*cc == '4')
            ALNL = 0;
        else
            ALNL = 1;
        cc += 3;

        while (*cc != ':') {
            type = *cc;
            cc++;

            if (type == '1') {
                if (!ALNL)
                    strcat (buf, "There was a 2-team tie for the Wild Card in the AL.\nIn a 1-game playoff:\n");
                else
                    strcat (buf, "There was a 2-team tie for the Wild Card in the NL.\nIn a 1-game playoff:\n");
            }
            if (type == '2') {
                if (!ALNL)
                    strcat (buf, "There was a 3-team tie for the Wild Card in the AL.\nIn a 2-game playoff:\n");
                else
                    strcat (buf, "There was a 3-team tie for the Wild Card in the NL.\nIn a 2-game playoff:\n");
            }
            if (type == '3') {
                if (!ALNL)
                    strcat (buf, "There was a 4-team tie for the Wild Card in the AL.\nIn a 3-game playoff:\n");
                else
                    strcat (buf, "There was a 4-team tie for the Wild Card in the NL.\nIn a 3-game playoff:\n");
            }
            if (type == '4') {
                if (!ALNL)
                    strcat (buf, "There were more than 4 teams tied for the Wild Card in the AL.\n");
                else
                    strcat (buf, "There were more than 4 teams tied for the Wild Card in the NL.\n");
            }
            if (type == 'a') {
                if (!ALNL)
                    strcat (buf, "There was a 3-team tie for a Division in the AL.\nIn a 2-game playoff:\n");
                else
                    strcat (buf, "There was a 3-team tie for a Division in the NL.\nIn a 2-game playoff:\n");
            }
            if (type == 'b') {
                if (!ALNL)
                    strcat (buf, "There was a 4-team tie for a Division in the AL.\nIn a 3-game playoff:\n");
                else
                    strcat (buf, "There was a 4-team tie for a Division in the NL.\nIn a 3-game playoff:\n");
            }
            if (type == 'c') {
                if (!ALNL)
                    strcat (buf, "There were more than 4 teams tied for a Division in the AL.\n");
                else
                    strcat (buf, "There were more than 4 teams tied for a Division in the NL.\n");
            }
            if (type == 'd') {
                if (!ALNL)
                    strcat (buf, "There was a 2-team tie for the AL.\nIn a 1-game playoff:\n");
                else
                    strcat (buf, "There was a 2-team tie for the NL.\nIn a 1-game playoff:\n");
            }
            if (type == 'e') {
                if (!ALNL)
                    strcat (buf, "There was a 3-team tie for the AL.\nIn a 2-game playoff:\n");
                else
                    strcat (buf, "There was a 3-team tie for the NL.\nIn a 2-game playoff:\n");
            }
            if (type == 'f') {
                if (!ALNL)
                    strcat (buf, "There was a 4-team tie for the AL.\nIn a 3-game playoff:\n");
                else
                    strcat (buf, "There was a 4-team tie for the NL.\nIn a 3-game playoff:\n");
            }
            if (type == 'g') {
                if (!ALNL)
                    strcat (buf, "There were more than 4 teams tied for the AL.\n");
                else
                    strcat (buf, "There were more than 4 teams tied for the NL.\n");
            }
            if (type == 'h') {
                if (!ALNL)
                    strcat (buf, "There was a 2-team tie for second place in the AL.\nIn a 1-game playoff:\n");
                else
                    strcat (buf, "There was a 2-team tie for second place in the NL.\nIn a 1-game playoff:\n");
            }
            if (type == 'i') {
                if (!ALNL)
                    strcat (buf, "There was a 3-team tie for second place in the AL.\nIn a 2-game playoff:\n");
                else
                    strcat (buf, "There was a 3-team tie for second place in the NL.\nIn a 2-game playoff:\n");
            }
            if (type == 'j') {
                if (!ALNL)
                    strcat (buf, "There was a 4-team tie for second place in the AL.\nIn a 3-game playoff:\n");
                else
                    strcat (buf, "There was a 4-team tie for second place in the NL.\nIn a 3-game playoff:\n");
            }
            if (type == 'k') {
                if (!ALNL)
                    strcat (buf, "There were more than 4 teams tied for second place in the AL.\n");
                else
                    strcat (buf, "There were more than 4 teams tied for second place in the NL.\n");
            }
            if (type == '5' || type == '6' || type == '7' || type == '8') {
                if (!ALNL)
                    strcat (buf, "There was a 2-team tie for a Division in the AL.\n");
                else
                    strcat (buf, "There was a 2-team tie for a Division in the NL.\n");
                if (type == '5' || type == '6') {
                    strcat (buf, "The Division was decided by their record against one another\n");
                    strcat (buf, "  since the loser is the Wild Card.\n");
                }
                else {
                    strcat (buf, "The Division was decided by a coin flip since they had the same record\n");
                    strcat (buf, "  against one another, and since the loser is the Wild Card.\n");
                }
            }
            if (type == '9') {
                if (!ALNL)
                    strcat (buf, "There was a 2-team tie for a Division in the AL.\nIn a 1-game playoff:\n");
                else
                    strcat (buf, "There was a 2-team tie for a Division in the NL.\nIn a 1-game playoff:\n");
            }
            for (flip = 0; *cc != ':'; /* cc is incremented within the loop */ ) {
                if (flip && (type == '4' || type == 'c' || type == 'g' || type == 'k')) {
                    cc++;
                    continue;
                }
                else
                    if (*cc != '0') {
                        strncat (buf, cc, 4);
                        strcat (buf, " ");
                        strncpy (&teamid[0], cc + 4, 4);
                        teamid[4] = '\0';
                        x = atoi (&teamid[0]);
                        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                            if (teaminfo[y].id == x) {
                                strcat (buf, &teaminfo[y].teamname[0]);
                                break;
                            }
                        cc += 8;
                    }
                    else {
                        cc += 4;                 /* point to name of user-created team */
                        work = index (cc, ' ');
                        *work = '\0';
                        strcat (buf, cc);
                        *work = ' ';
                        cc = work + 1;
                    }
                if (!flip) {
                    if (type == '0' || type == '1' || type == '2' || type == '3' || type == '9' || type == 'b' || type == 'd' ||
                                          type == 'a' || type == 'e' || type == 'f' || type == 'h' || type == 'i' || type == 'j')
                        strcat (buf, " beat ");
                    if (type == '7' || type == '8')
                        strcat (buf, " in a coin flip won out against ");
                    if (type == '4' || type == 'c' || type == 'g' || type == 'k')
                        strcat (buf, " won out in a coin flip.\n");
                    if (type == '5' || type == '6')
                        strcat (buf, " won out against ");
                    flip = 1;
                }
                else {
                    strcat (buf, "\n");
                    flip = 0;
                }
            }
            cc += 2;
            strcat (buf, "\n");
        }
        strcat (buf, "\n");
        cc += 2;
    }
    fputs (sp, sfp);
    strcpy (buf, "\n");
    if (!ALNL)
        goto Check4Playoffs;

    if (*cc == '1') {
        int xx;
        char wcyr[4][5], wcid[4][5];

        strcpy (buf, "The regular season is completed.\n");
        fputs (sp, sfp);
		strcpy (buf, "The first round of the post-season will be:\n\n");
        fputs (sp, sfp);
        buf[0] = '\0';
        for (xx = 0, cc++; cc < (&buffer[0] + strlen (&buffer[0])); xx += 2) { /* cc is incremented within loop */
            if (*cc == ' ')
                if (xx == 4 || xx == 8) {
                    strncat (buf, &wcyr[(xx - 4) / 2][0], 4);
                    strncpy (&teamid[0], &wcid[(xx - 4) / 2][0], 4);
                    teamid[4] = '\0';
                    x = atoi (&teamid[0]);
                    for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                        if (teaminfo[y].id == x) {
                            strcat (buf, &teaminfo[y].teamabbrev[0]);
                            break;
                        }
                    strcat (buf, " or ");

                    strncat (buf, &wcyr[(xx - 4) / 2 + 1][0], 4);
                    strncpy (&teamid[0], &wcid[(xx - 4) / 2 + 1][0], 4);
                    teamid[4] = '\0';
                    x = atoi (&teamid[0]);
                    for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                        if (teaminfo[y].id == x) {
                            strcat (buf, &teaminfo[y].teamabbrev[0]);
                            break;
                        }

                    cc += 8;
                    if (!x)
                        cc++;
                    goto standings_vs;
                }
            if (*cc != '0') {
                strncat (buf, cc, 4);
                strcat (buf, " ");
                strncpy (&teamid[0], cc + 4, 4);
                teamid[4] = '\0';
                x = atoi (&teamid[0]);
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == x) {
                        strcat (buf, &teaminfo[y].teamname[0]);
                        break;
                    }
                /* save team yr & id for possible processing later */
                if (!xx || xx == 2) {
                    strncpy (&wcyr[xx][0], cc, 4);
                    strncpy (&wcid[xx][0], cc + 4, 4);
                    wcyr[xx][4] = wcid[xx][4] = '\0';
                }
                cc += 8;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                work = index (cc, ' ');
                *work = '\0';
                strcat (buf, cc);
                *work = ' ';
                cc = work + 1;
            }
standings_vs:
            strcat (buf, " vs ");
            if (*cc != '0') {
                strncat (buf, cc, 4);
                strcat (buf, " ");
                strncpy (&teamid[0], cc + 4, 4);
                teamid[4] = '\0';
                x = atoi (&teamid[0]);
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == x) {
                        strcat (buf, &teaminfo[y].teamname[0]);
                        break;
                    }
                /* save team yr & id for possible processing later */
                if (!xx || xx == 2) {
                    strncpy (&wcyr[xx + 1][0], cc, 4);
                    strncpy (&wcid[xx + 1][0], cc + 4, 4);
                    wcyr[xx + 1][4] = wcid[xx + 1][4] = '\0';
                }
                cc += 8;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                work = index (cc, ' ');
                *work = '\0';
                strcat (buf, cc);
                *work = ' ';
                cc = work + 1;
            }
            strcat (buf, "\n\n");
        }
        fputs (sp, sfp);
    }
    overpssw = 0;
    if (*cc == '2') {
        strcpy (buf, "The post-season results so far:\n\n");
        fputs (sp, sfp);
    }
    if (*cc == '3') {
        strcpy (buf, "The post-season is completed.\n");
        fputs (sp, sfp);
        strcpy (buf, "The post-season results:\n\n");
        fputs (sp, sfp);
        overpssw = 1;
    }
    if (*cc == '2' || *cc == '3') {
        char work[5], *w1, uctn[2][50];
        int id[2], yr[2], w[2], i, oversw = 0;

        for (cc++; cc < (&buffer[0] + strlen (&buffer[0])); /* cc is incremented within loop */) {
            if (*cc == ':') {
                strcpy (buf, "\n\n");
                fputs (sp, sfp);
                cc++;
                continue;
            }
            if (*cc == 'X')
                break;
            strncpy (&work[0], cc, 4);
            work[4] = '\0';
            yr[0] = atoi (&work[0]);
            strncpy (&work[0], cc + 4, 4);
            work[4] = '\0';
            id[0] = atoi (&work[0]);
            if (id[0] < 900) {
                strncpy (&work[0], cc + 8, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
 
                w[0] = atoi (&work[0]);
                cc += 9;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                w1 = index (cc, ' ');
                *w1 = '\0';
                strcpy (&uctn[0][0], cc);
                *w1 = ' ';
                cc = w1 + 1;
                strncpy (&work[0], cc, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[0] = atoi (&work[0]);
                cc++;
            }

            strncpy (&work[0], cc, 4);
            work[4] = '\0';
            yr[1] = atoi (&work[0]);
            strncpy (&work[0], cc + 4, 4);
            work[4] = '\0';
            id[1] = atoi (&work[0]);
            if (id[1] < 900) {
                strncpy (&work[0], cc + 8, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[1] = atoi (&work[0]);
                cc += 9;
            }
            else {
                cc += 9;                 /* point to name of user-created team */
                w1 = index (cc, ' ');
                *w1 = '\0';
                strcpy (&uctn[1][0], cc);
                *w1 = ' ';
                cc = w1 + 1;
                strncpy (&work[0], cc, 1);
                work[1] = '\0';
                if (isalpha (work[0])) {
                    oversw = 1;
                    switch (work[0]) {
                        case 'A':
                            work[0] = '1';
                            break;
                        case 'B':
                            work[0] = '2';
                            break;
                        case 'C':
                            work[0] = '3';
                            break;
                        case 'D':
                            work[0] = '4';
                            break;
                        case 'E':
                            work[0] = '5';
                    }
                }
                w[1] = atoi (&work[0]);
                cc++;
            }

            if (w[0] >= w[1])
                i = 0;
            else
                i = 1;

            if (id[i] < 900) {
                strcpy (buf, (char *) cnvt_int2str (yr[i], 'l'));
                strcat (buf, " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcat (buf, &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcpy (buf, &uctn[i][0]);

            if (oversw) {
                strcat (buf, " beat ");
                oversw = 0;
            }
            else
                if (w[0] == w[1])
                    strcat (buf, " and ");
                else
                    strcat (buf, " are leading ");

            if (i)
                i = 0;
            else
                i = 1;

            if (id[i] < 900) {
                strcat (buf, (char *) cnvt_int2str (yr[i], 'l'));
                strcat (buf, " ");
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcat (buf, &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (buf, &uctn[i][0]);
            strcat (buf, " ");

            if (i)
                i = 0;
            else
                i = 1;

            if (w[0] == w[1])
                strcat (buf, "are tied at ");
            strcat (buf, (char *) cnvt_int2str (w[i], 'l'));
            if (w[0] == w[1])
                if (w[0] == 1)
                    strcat (buf, " game apiece");
                else
                    strcat (buf, " games apiece");
            else
                if (w[i] == 1)
                    strcat (buf, " game to ");
                else
                    strcat (buf, " games to ");

            if (i)
                i = 0;
            else
                i = 1;

            if (w[0] != w[1])
                strcat (buf, (char *) cnvt_int2str (w[i], 'l'));

            strcat (buf, "\n\n");
            fputs (sp, sfp);
        }

        if (*cc == 'X' && overpssw) {
            if (w[0] > w[1])
                i = 0;
            else
                i = 1;

            strcpy (buf, "\n\n");
            fputs (sp, sfp);

            if (id[i] < 900) {
                for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                    if (teaminfo[y].id == id[i]) {
                        strcpy (buf, (char *) cnvt_int2str (yr[i], 'l'));
                        strcat (buf, " ");
                        strcat (buf, &teaminfo[y].teamname[0]);
                        break;
                    }
            }
            else
                strcat (buf, &uctn[i][0]);
            strcat (buf, " WORLD CHAMPS!!\n");

            fputs (sp, sfp);
        }
    }
}

void
DoAutoIR () {
    gint x, y, len, teamid, teamyr, numlines;
    gchar *cc, *cc2, *ns, work[6];
    struct {
        gchar name[100], tmabbrev[20], remgames[10];
    } plist[5001];

    for (x = 0; x < 5001; x++)
        plist[x].name[0] = plist[x].tmabbrev[0] = plist[x].remgames[0] = '\0';

    strcpy (buf, "\n\nLeague Injury Report:\n\n");
    fputs (sp, sfp);

    strcpy (buf, "\n\n PLAYER                   TEAM      Remaining Games Out\n\n");
    fputs (sp, sfp);

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
    for (buf[0] = '\0', x = 0; x < numlines; x++) {
        strcat (buf, &plist[x].name[0]);
        strncat (buf, "                         ", (25 - strlen (&plist[x].name[0])));
        strcat (buf, &plist[x].tmabbrev[0]);
        strncat (buf, "                 ", (17 - strlen (&plist[x].tmabbrev[0])));
        strcat (buf, &plist[x].remgames[0]);
        strcat (buf, "\n");
    }
    fputs (sp, sfp);
}

int
DoAutoCatLeaders (int sock) {
    int x, y, z, teamid, manysw, catl[3][32][8], cati, type, limit = 0, pos = 0;
    char buf1[100], hdr[100], year[5], w[100], w2[100], *cc, *cc1, *work, charid[3], letter;
    struct {
        char name[51],
             tyr[5],
             tabbr[15],
             uctname[50],
             info1[6],
             info2[6],
             info3[6],
             info4[6],
             info5[6],
             info6[6],
             stat[6],
             date[11],
             dis[4],
             user[50];
    } list[50];

    catl[0][0][0] = autoplay.catl.hitting.games;
    catl[0][1][0] = autoplay.catl.hitting.atbats;
    catl[0][2][0] = autoplay.catl.hitting.runs;
    catl[0][3][0] = autoplay.catl.hitting.hits;
    catl[0][4][0] = autoplay.catl.hitting.doubles;
    catl[0][5][0] = autoplay.catl.hitting.triples;
    catl[0][6][0] = autoplay.catl.hitting.homers;
    catl[0][7][0] = autoplay.catl.hitting.rbis;
    catl[0][8][0] = autoplay.catl.hitting.bb;
    catl[0][9][0] = autoplay.catl.hitting.so;
    catl[0][10][0] = autoplay.catl.hitting.hbp;
    catl[0][11][0] = autoplay.catl.hitting.dp;
    catl[0][12][0] = autoplay.catl.hitting.sb;
    catl[0][13][0] = autoplay.catl.hitting.cs;
    catl[0][14][0] = autoplay.catl.hitting.ibb;
    catl[0][15][0] = autoplay.catl.hitting.sh;
    catl[0][16][0] = autoplay.catl.hitting.sf;
    catl[0][17][0] = autoplay.catl.hitting.ba;
    catl[0][18][0] = autoplay.catl.hitting.sa;
    catl[0][19][0] = autoplay.catl.hitting.oba;

    catl[1][0][0] = autoplay.catl.pitching.games;
    catl[1][1][0] = autoplay.catl.pitching.gs;
    catl[1][2][0] = autoplay.catl.pitching.ip;
    catl[1][3][0] = autoplay.catl.pitching.wins;
    catl[1][4][0] = autoplay.catl.pitching.losses;
    catl[1][5][0] = autoplay.catl.pitching.sv;
    catl[1][6][0] = autoplay.catl.pitching.bfp;
    catl[1][7][0] = autoplay.catl.pitching.hits;
    catl[1][8][0] = autoplay.catl.pitching.db;
    catl[1][9][0] = autoplay.catl.pitching.tp;
    catl[1][10][0] = autoplay.catl.pitching.hr;
    catl[1][11][0] = autoplay.catl.pitching.runs;
    catl[1][12][0] = autoplay.catl.pitching.er;
    catl[1][13][0] = autoplay.catl.pitching.rbi;
    catl[1][14][0] = autoplay.catl.pitching.cg;
    catl[1][15][0] = autoplay.catl.pitching.gf;
    catl[1][16][0] = autoplay.catl.pitching.sho;
    catl[1][17][0] = autoplay.catl.pitching.svopp;
    catl[1][18][0] = autoplay.catl.pitching.sb;
    catl[1][19][0] = autoplay.catl.pitching.cs;
    catl[1][20][0] = autoplay.catl.pitching.bb;
    catl[1][21][0] = autoplay.catl.pitching.so;
    catl[1][22][0] = autoplay.catl.pitching.ibb;
    catl[1][23][0] = autoplay.catl.pitching.sh;
    catl[1][24][0] = autoplay.catl.pitching.sf;
    catl[1][25][0] = autoplay.catl.pitching.wp;
    catl[1][26][0] = autoplay.catl.pitching.b;
    catl[1][27][0] = autoplay.catl.pitching.hb;
    catl[1][28][0] = autoplay.catl.pitching.ab;
    catl[1][29][0] = autoplay.catl.pitching.era;
    catl[1][30][0] = autoplay.catl.pitching.pct;
    catl[1][31][0] = autoplay.catl.pitching.oppba;

    for (x = 0; x < 8; x++) {
        catl[2][0][x] = autoplay.catl.fielding.pos[x].games;
        if (x != 7) {
            catl[2][1][x] = autoplay.catl.fielding.pos[x].po;
            catl[2][2][x] = autoplay.catl.fielding.pos[x].dp;
            catl[2][3][x] = autoplay.catl.fielding.pos[x].a;
            catl[2][4][x] = autoplay.catl.fielding.pos[x].pb;
            catl[2][5][x] = autoplay.catl.fielding.pos[x].e;
            catl[2][6][x] = autoplay.catl.fielding.pos[x].pct;
        }
        else
            catl[2][1][x] = catl[2][2][x] = catl[2][3][x] = catl[2][4][x] = catl[2][5][x] = catl[2][6][x] = 0;
    }

    for (type = 0; type < 3; type++) {
        if (type == 0)
            limit = 20;
        if (type == 1)
            limit = 32;
        if (type == 2)
            limit = 7;

        for (pos = cati = 0; cati < limit; cati++) {
            if (catl[type][cati][pos]) {
                if (cati < 9)
                    if (type == 2)
                        sprintf (&w[0], "S40000%d%d%d00\n", (type + 1), (cati + 1), (pos + 1));
                    else
                        sprintf (&w[0], "S40000%d%d000\n", (type + 1), (cati + 1));
                else
                    if (cati < 19) {
                        letter = 'a' + (cati - 9);
                        sprintf (&w[0], "S40000%d%c000\n", (type + 1), letter);
                    }
                    else
                        if (cati < 29) {
                            letter = 'k' + (cati - 19);
                            sprintf (&w[0], "S40000%d%c000\n", (type + 1), letter);
                        }
                        else {
                            letter = 'u' + (cati - 29);
                            sprintf (&w[0], "S40000%d%c000\n", (type + 1), letter);
                        }


                sock_puts (sock, &w[0]);
                if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0)
                    return 1;

                strcpy (&hdr[0], "         Name, Team                       ");

                buf1[0] = '\0';
                strcat (&buf1[0], "\n\n\n");
                strcat (&buf1[0], "NSB Current Regular Season ");
                if (type == 0)
                    strcat (&buf1[0], "Hitting ");
                if (type == 1)
                    strcat (&buf1[0], "Pitching ");
                if (type == 2)
                    strcat (&buf1[0], "Fielding ");

                strcat (&buf1[0], "Leaders in ");

                if (type == 0) {
                    if (cati == 0) {
                        strcat (&buf1[0], "Games Played");
                        strcat (&hdr[0], " G ");
                    }
                    if (cati == 1) {
                        strcat (&buf1[0], "At Bats");
                        strcat (&hdr[0], " AB");
                    }
                    if (cati == 2) {
                        strcat (&buf1[0], "Runs Scored");
                        strcat (&hdr[0], " R ");
                    }
                    if (cati == 3) {
                        strcat (&buf1[0], "Hits");
                        strcat (&hdr[0], " H ");
                    }
                    if (cati == 4) {
                        strcat (&buf1[0], "Doubles");
                        strcat (&hdr[0], " 2B");
                    }
                    if (cati == 5) {
                        strcat (&buf1[0], "Triples");
                        strcat (&hdr[0], " 3B");
                    }
                    if (cati == 6) {
                        strcat (&buf1[0], "Home Runs");
                        strcat (&hdr[0], " HR");
                    }
                    if (cati == 7) {
                        strcat (&buf1[0], "Runs Batted In");
                        strcat (&hdr[0], " BI");
                    }
                    if (cati == 8) {
                        strcat (&buf1[0], "Bases on balls");
                        strcat (&hdr[0], " BB");
                    }
                    if (cati == 9) {
                        strcat (&buf1[0], "Strike Outs");
                        strcat (&hdr[0], " K ");
                    }
                    if (cati == 10) {
                        strcat (&buf1[0], "Hit by Pitches");
                        strcat (&hdr[0], "HBP");
                    }
                    if (cati == 11) {
                        strcat (&buf1[0], "Grounded into DPs");
                        strcat (&hdr[0], "GDP");
                    }
                    if (cati == 12) {
                        strcat (&buf1[0], "Stolen Bases");
                        strcat (&hdr[0], " SB");
                    }
                    if (cati == 13) {
                        strcat (&buf1[0], "Caught Stealing");
                        strcat (&hdr[0], " CS");
                    }
                    if (cati == 14) {
                        strcat (&buf1[0], "Intentional Walks");
                        strcat (&hdr[0], "IBB");
                    }
                    if (cati == 15) {
                        strcat (&buf1[0], "Sacrifice Hits");
                        strcat (&hdr[0], " SH");
                    }
                    if (cati == 16) {
                        strcat (&buf1[0], "Sacrifice Flies");
                        strcat (&hdr[0], " SF");
                    }
                    if (cati == 17) {
                        strcat (&buf1[0], "Batting Average");
                        strcat (&hdr[0], " AB     H    BA");
                    }
                    if (cati == 18) {
                        strcat (&buf1[0], "Slugging Average");
                        strcat (&hdr[0], " AB     H    2B    3B    HR    SA");
                    }
                    if (cati == 19) {
                        strcat (&buf1[0], "On Base Average");
                        strcat (&hdr[0], " PA     H    BB    HB   OBA");
                    }
                }

                if (type == 1) {
                    if (cati == 0) {
                        strcat (&buf1[0], "Games Pitched");
                        strcat (&hdr[0], " G ");
                    }
                    if (cati == 1) {
                        strcat (&buf1[0], "Games Started");
                        strcat (&hdr[0], " GS");
                    }
                    if (cati == 2) {
                        strcat (&buf1[0], "Innings Pitched");
                        strcat (&hdr[0], " IP");
                    }
                    if (cati == 3) {
                        strcat (&buf1[0], "Wins");
                        strcat (&hdr[0], " W ");
                    }
                    if (cati == 4) {
                        strcat (&buf1[0], "Losses");
                        strcat (&hdr[0], " L ");
                    }
                    if (cati == 5) {
                        strcat (&buf1[0], "Saves");
                        strcat (&hdr[0], " S ");
                    }
                    if (cati == 6) {
                        strcat (&buf1[0], "Batters Facing Pitcher");
                        strcat (&hdr[0], "BFP");
                    }
                    if (cati == 7) {
                        strcat (&buf1[0], "Hits Allowed");
                        strcat (&hdr[0], " H ");
                    }
                    if (cati == 8) {
                        strcat (&buf1[0], "Doubles Allowed");
                        strcat (&hdr[0], " 2B");
                    }
                    if (cati == 9) {
                        strcat (&buf1[0], "Triples Allowed");
                        strcat (&hdr[0], " 3B");
                    }
                    if (cati == 10) {
                        strcat (&buf1[0], "Home Runs Allowed");
                        strcat (&hdr[0], " HR");
                    }
                    if (cati == 11) {
                        strcat (&buf1[0], "Runs Allowed");
                        strcat (&hdr[0], " R ");
                    }
                    if (cati == 12) {
                        strcat (&buf1[0], "Earned Runs Allowed");
                        strcat (&hdr[0], " ER");
                    }
                    if (cati == 13) {
                        strcat (&buf1[0], "RBIs Allowed");
                        strcat (&hdr[0], " BI");
                    }
                    if (cati == 14) {
                        strcat (&buf1[0], "Complete Games");
                        strcat (&hdr[0], " CG");
                    }
                    if (cati == 15) {
                        strcat (&buf1[0], "Games Finished");
                        strcat (&hdr[0], " GF");
                    }
                    if (cati == 16) {
                        strcat (&buf1[0], "Shut Outs");
                        strcat (&hdr[0], "SHO");
                    }
                    if (cati == 17) {
                        strcat (&buf1[0], "Save Opportunities");
                        strcat (&hdr[0], "SOP");
                    }
                    if (cati == 18) {
                        strcat (&buf1[0], "Stolen Bases Against");
                        strcat (&hdr[0], " SB");
                    }
                    if (cati == 19) {
                        strcat (&buf1[0], "Caught Stealing Against");
                        strcat (&hdr[0], " CS");
                    }
                    if (cati == 20) {
                        strcat (&buf1[0], "Bases on Balls");
                        strcat (&hdr[0], " BB");
                    }
                    if (cati == 21) {
                        strcat (&buf1[0], "Strike Outs");
                        strcat (&hdr[0], " K ");
                    }
                    if (cati == 22) {
                        strcat (&buf1[0], "Intentional Walks");
                        strcat (&hdr[0], "IBB");
                    }
                    if (cati == 23) {
                        strcat (&buf1[0], "Sacrifice Hits Allowed");
                        strcat (&hdr[0], " SH");
                    }
                    if (cati == 24) {
                        strcat (&buf1[0], "Sacrifice Flies Allowed");
                        strcat (&hdr[0], " SF");
                    }
                    if (cati == 25) {
                        strcat (&buf1[0], "Wild Pitches");
                        strcat (&hdr[0], " WP");
                    }
                    if (cati == 26) {
                        strcat (&buf1[0], "Balks");
                        strcat (&hdr[0], " B ");
                    }
                    if (cati == 27) {
                        strcat (&buf1[0], "Hit Batters");
                        strcat (&hdr[0], " HB");
                    }
                    if (cati == 28) {
                        strcat (&buf1[0], "Opponents' At Bats");
                        strcat (&hdr[0], "OAB");
                    }
                    if (cati == 29) {
                        strcat (&buf1[0], "Earned Run Average");
                        strcat (&hdr[0], " IP      ER   ERA");
                    }
                    if (cati == 30) {
                        strcat (&buf1[0], "Won/Loss Percentage");
                        strcat (&hdr[0], "  W     L   PCT");
                    }
                    if (cati == 31) {
                        strcat (&buf1[0], "Opponents' Batting Average");
                        strcat (&hdr[0], "OAB     H   OBA");
                    }
                }

                if (type == 2) {
                    if (cati == 0) {
                        strcat (&buf1[0], "Games Played");
                        strcat (&hdr[0], " G ");
                    }
                    if (cati == 1) {
                        strcat (&buf1[0], "Put Outs");
                        strcat (&hdr[0], " PO");
                    }
                    if (cati == 2) {
                        strcat (&buf1[0], "Double Plays");
                        strcat (&hdr[0], " DP");
                    }
                    if (cati == 3) {
                        strcat (&buf1[0], "Assists");
                        strcat (&hdr[0], " A ");
                    }
                    if (cati == 4) {
                        strcat (&buf1[0], "Passed Balls");
                        strcat (&hdr[0], " PB");
                    }
                    if (cati == 5) {
                        strcat (&buf1[0], "Errors");
                        strcat (&hdr[0], " E ");
                    }
                    if (cati == 6) {
                        strcat (&buf1[0], "Fielding Avg");
                        strcat (&hdr[0], " TC     E    FA");
                    }

                    strcat (&buf1[0], " for ");

                    if (pos == 0)
                        strcat (&buf1[0], "OF");
                    if (pos == 1)
                        strcat (&buf1[0], "1B");
                    if (pos == 2)
                        strcat (&buf1[0], "2B");
                    if (pos == 3)
                        strcat (&buf1[0], "3B");
                    if (pos == 4)
                        strcat (&buf1[0], "SS");
                    if (pos == 5)
                        strcat (&buf1[0], "P");
                    if (pos == 6)
                        strcat (&buf1[0], "C");
                    if (pos == 7)
                        strcat (&buf1[0], "DH");
                }

                strcpy (buf, &buf1[0]);
                strcat (buf, "\n");
                fputs (sp, sfp);

                for (x = 0; x < 50; x++) {
                    strcpy (&list[x].stat[0], "    0");
                    strcpy (&list[x].info1[0], "    0");
                    strcpy (&list[x].info2[0], "    0");
                    strcpy (&list[x].info3[0], "    0");
                    strcpy (&list[x].info4[0], "    0");
                    strcpy (&list[x].info5[0], "    0");
                }
                manysw = 0;
                for (x = 0, cc = &buffer[0]; x < 50 && cc < (&buffer[0] + strlen (&buffer[0])); x++) {
                    work = (char *) index (cc, '?');
                    *work = '\0';

                    if (*cc == ' ')
                        /* a blank name signifies no more stats */
                        break;

                    /* reverse name */
                    strcpy (&w2[0], cc);
                    strcpy (&w[0], &w2[index (&w2[0], ',') - &w2[0] + 2]);
                    strcat (&w[0], " ");
                    strncat (&w[0], &w2[0], (index (&w2[0], ',') - &w2[0]));
                    strcpy (&list[x].name[0], &w[0]);

                    *work = '?';
                    cc = work + 2;
                    strncpy (&year[0], cc, 4);
                    year[4] = '\0';
                    cc += 4;

                    charid[0] = *cc;
                    cc++;
                    charid[1] = *cc;
                    charid[2] = '\0';
                    cc++;
                    teamid = atoi (&charid[0]);

                    strcpy (&list[x].tyr[0], &year[0]);

                    /* move Team Abbreviation */
                    if (year[0] != '0') {
                        for (y = 0; y <= NUMBER_OF_TEAMS; y++)
                            if (teaminfo[y].id == teamid) {
                                strcpy (&list[x].tabbr[0], &teaminfo[y].teamabbrev[0]);
                                break;
                            }
                        list[x].uctname[0] = '\0';
                    }
                    else {
                        cc++;
                        work = index (cc, ' ');
                        *work = '\0';
                        strcpy (&list[x].uctname[0], cc);
                        *work = ' ';
                        cc = work + 1;
                    }

                    strncpy (&list[x].info1[0], cc, 5);
                    list[x].info1[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info1[y] == '0')
                            list[x].info1[y] = ' ';
                        else
                            break;
                    cc += 5;
                    strncpy (&list[x].info2[0], cc, 5);
                    list[x].info2[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info2[y] == '0')
                            list[x].info2[y] = ' ';
                        else
                            break;
                    cc += 5;
                    strncpy (&list[x].info3[0], cc, 5);
                    list[x].info3[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info3[y] == '0')
                            list[x].info3[y] = ' ';
                        else
                            break;
                    cc += 5;
                    strncpy (&list[x].info4[0], cc, 5);
                    list[x].info4[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info4[y] == '0')
                            list[x].info4[y] = ' ';
                        else
                            break;
                    cc += 5;
                    strncpy (&list[x].info5[0], cc, 5);
                    list[x].info5[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info5[y] == '0')
                            list[x].info5[y] = ' ';
                        else
                            break;
                    cc += 5;

                    for (cc1 = cc, y = 0; *cc1 == '0' && y < 4; y++, cc1++)
                        *cc1 = ' ';

                    if ((type == 0 && (cati == 17 || cati == 18 || cati == 19)) || (type == 1 && (cati == 30 || cati == 31)) || (type == 2 && cati == 6)) {
                        if (*(cc + 1) != ' ')
                            *cc = *(cc + 1);
                        *(cc + 1) = '.';
                    }
                    if (type == 1 && cati == 29) {
                        if (*(cc + 2) != ' ') {
                            *cc = *(cc + 1);
                            *(cc + 1) = *(cc + 2);
                        }
                        *(cc + 2) = '.';
                    }
                    strncpy (&list[x].stat[0], cc, 5);
                    list[x].stat[5] = '\0';
                    cc += 5;

                    strncpy (&list[x].info6[0], cc, 5);
                    list[x].info6[5] = '\0';
                    for (y = 0; y < 4; y++)
                        if (list[x].info6[y] == '0')
                            list[x].info6[y] = ' ';
                        else
                            break;
                    cc += 5;
                }

                buf[0] = '\0';
                if (type == 0 && (cati == 17 || cati == 18 || cati == 19))
                    strcat (&buf[0], "Minimum plate appearances - 3.1 X number of games player's team has played");

                if (type == 1) {
                    if (cati == 29 || cati == 31)
                        strcat (&buf[0], "Minimum innings pitched - number of games player's team has played");
                    if (cati == 30)
                        strcat (&buf[0], "Minimum number of decisions - number of games player's team has played / 12");
                }

                if (type == 2)
                    if (cati == 6) {
                        strcat (&buf[0], "Minimum number of ");
                        if (pos == 5)
                            strcat (&buf[0], "innings pitched");
                        else
                            strcat (&buf[0], "games played");
                        if (pos == 5)
                            strcat (&buf[0], " - number of games player's team has played");
                        else
                            if (pos == 6)
                                strcat (&buf[0], " - number of games player's team has played / 2");
                            else
                                strcat (&buf[0], " - number of games player's team has played / 3 X 2");
                    }

                strcat (buf, "\n\n");
                fputs (sp, sfp);

                strcpy (buf, &hdr[0]);
                strcat (buf, "\n\n");
                fputs (sp, sfp);

                x--;
                if (x > 24) {
                    if (!strcmp (&list[24].stat[0], &list[25].stat[0]))
                        if (!strcmp (&list[24].stat[0], &list[49].stat[0]))
                            for (x = 23; x >= 0; x--) {
                                if (!strcmp (&list[24].stat[0], &list[x].stat[0]))
                                    continue;
                                if (x == 23) {
                                    x = 24;
                                    break;
                                }
                                x++;
                                manysw = 1;
                                break;
                            }
                        else
                            for (x = 25; x < 50; x++)
                                if (!strcmp (&list[24].stat[0], &list[x].stat[0]))
                                    continue;
                                else {
                                    x--;
                                    break;
                                }
                    else
                        x = 24;
                }

                if (x < 0) {
                    x = 0;
                    manysw = 1;
                }

                for (y = 0; y <= x; y++) {
                    buf[0] = '\0';
                    strcat (&buf[0], (char *) cnvt_int2str ((y + 1), 'l'));
                    strcat (&buf[0], "  ");

                    if (x == y && manysw) {
                        strcat (&buf[0], "Many tied with                      ");
                        if (y < 9)
                            strcat (&buf[0], " ");
                        if ((type == 0 && (cati == 17 || cati == 18 || cati == 19)) || (type == 1 && (cati == 29 || cati == 30 || cati == 31)) ||
                                                                                                                          (type == 2 && cati == 6))
                            strcat (&buf[0], "            ");
                        if (type == 0 && cati == 19)
                            strcat (&buf[0], "           ");
                        strcat (&buf[0], &list[y].stat[0]);
                        strcat (&buf[0], "\n");
                        break;
                    }

                    strcat (&buf[0], &list[y].name[0]);
                    strcat (&buf[0], ", ");
                    if (!strlen (&list[y].uctname[0])) {
                        strcat (&buf[0], &list[y].tyr[0]);
                        strcat (&buf[0], &list[y].tabbr[0]);
                    }
                    else
                        strcat (&buf[0], &list[y].uctname[0]);

                    z = strlen (&buf[0]);
                    strncat (&buf[0], "                                        ", 40 - z);

                    if (type == 0) {
                        if (cati == 17) {
                            strcat (&buf[0], &list[y].info1[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info2[0]);
                            strcat (&buf[0], " ");
                        }
                        if (cati == 18) {
                            strcat (&buf[0], &list[y].info1[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info2[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info3[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info4[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info5[0]);
                            strcat (&buf[0], " ");
                        }
                        if (cati == 19) {
                            strcat (&buf[0], &list[y].info1[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info2[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info3[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info4[0]);
                            strcat (&buf[0], " ");
                        }
                    }

                    if (type == 1) {
                        if (cati == 30 || cati == 31) {
                            strcat (&buf[0], &list[y].info1[0]);
                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info2[0]);
                            strcat (&buf[0], " ");
                        }
                        if (cati == 29) {
                            strcat (&buf[0], &list[y].info1[0]);
                            if (strcmp (&list[y].info2[0], "    0")) {
                                strcat (&buf[0], ".");
                                strcat (&buf[0], &list[y].info2[4]);
                            }
                            else
                                strcat (&buf[0], "  ");

                            strcat (&buf[0], " ");
                            strcat (&buf[0], &list[y].info3[0]);
                            strcat (&buf[0], " ");
                        }
                    }

                    if (type == 2 && cati == 6) {
                        strcat (&buf[0], &list[y].info1[0]);
                        strcat (&buf[0], " ");
                        strcat (&buf[0], &list[y].info2[0]);
                        strcat (&buf[0], " ");
                    }

                    strcat (&buf[0], &list[y].stat[0]);

                    strcat (&buf[0], "\n");
                    fputs (sp, sfp);
                    strcpy (&buf[0], "\n");
                }

                strcat (buf, "\n\n");
                fputs (sp, sfp);
            }
            if (type == 2 && pos == 7)
                pos = 0;
            else
                if (type == 2 && pos < 7) {
                    cati--;
                    pos++;
                }
        }
    }

    return 0;
}

int
CheckLock () {
    int trycnt, fret;
    char path2lock[1024];
    static struct flock lock;

    strcpy (&path2lock[0], getenv ("HOME"));
    strcat (&path2lock[0], "/.NSBleaguelock");

    fdlock = -1;
    trycnt = 0;
    while (fdlock == -1 && trycnt < 3) {
        fdlock = open (path2lock, O_WRONLY);
        trycnt++;
    }
    if (fdlock == -1 && trycnt > 2) {
        trycnt = 0;
        while (fdlock == -1 && trycnt < 3) {
            fdlock = open (path2lock, O_CREAT, S_IRWXU);
            if (fdlock != -1)
                close (fdlock);
            trycnt++;
        }
        if (fdlock == -1 && trycnt > 2) {
            /* give up - cannot open nor create lock file */
            syslog (LOG_INFO, "(NSB League Autoplay) cannot open nor create league lock file for user %s", getenv ("USER"));
            return 2;
        }
        else {
            fdlock = open (path2lock, O_WRONLY);
            if (fdlock == -1) {
                /* give up - cannot open lock file */
                syslog (LOG_INFO, "(NSB League Autoplay) cannot open league lock file for user %s", getenv ("USER"));
                return 3;
            }
        }
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid ();

    fret = fcntl (fdlock, F_SETLK, &lock);
    if (fret == -1)
        /* another process is running league games */
        return -1;

    return 0;
}

