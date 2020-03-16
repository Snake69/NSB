
/* GTK client to talk to NetStatsBaseball (NSB) */

#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"
#include "net.h"
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>

GCond *data_cond = NULL;
GMutex *data_mutex = NULL;
gint FromThread, ShallWeContinue4Thread;  /* used in thread processing */

int fd;
struct termios oldtermios;         /* original tty settings */

/* the main stuff */
GtkWidget *mainbox, *texttable, *vscrollbar;

void
DestroyDialog (GtkWidget *widget, gpointer data) {
    gtk_widget_destroy (GTK_WIDGET (data));
}

gint
delete_event (GtkWidget *widget, GdkEventConfigure *event) {
    if (connected) {
        sock_puts (sock, "D\n");  /* tell server we're disconnecting */
        if (ThreadRunning)
            sock_gets (sock, &buffer[0], sizeof (buffer));
        close (sock);
    }

    gtk_widget_destroy (GTK_WIDGET (widget));
    gtk_main_quit ();

    /* restore original terminal settings */
    if (fd >= 0) {
        tcsetattr (fd, TCSANOW, &oldtermios);
        close (fd);
    }

    return TRUE ;
}

/* connect to a server */
void
ConnectToServer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar AlreadyConnected[256], *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (connected) {
        strcpy (&AlreadyConnected[0], "You are already connected to the NSB service running on ");
        strcat (&AlreadyConnected[0], &hs[0]);
        strcat (&AlreadyConnected[0], ".  Disconnect before connecting to a new server.");
        msg[0] = &AlreadyConnected[0];
        outMessage (msg);
    }
    else
        if (GetServerName ())
            Connect2NSBServer (0);
}

/* disconnect from the currently connected server */
void
DisconnectFromServer (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    if (connected) {
        sock_puts (sock, "D\n");  /* tell server we're disconnecting */
        if (ThreadRunning)
            sock_gets (sock, &buffer[0], sizeof (buffer));
        close (sock);

        strcpy (&work[0], "Connection to server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], " severed.\n");
    }
    else
        strcpy (&work[0], "Not connected.\n");

    sname[0] = '\0';
    connected = 0;

    SetLeagueUnderWay (0);
    SetSeriesUnderWay (0);

    Add2TextWindow (&work[0], 0);

    DisplayTopHalf (0);
}

void
PlayTakeMeOut2TheBallgame (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/take.wav");
}

void
PlayAbbottAndCostello (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/AbbottAndCostello.wav");
}

void
PlayJackieGleason (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/JackieGleason.wav");
}

void
PlayCenterfield (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    g_thread_new (NULL, play_snd, (gpointer) "/usr/local/share/NSB/Centerfield.wav");
}

void
Quit (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    if (connected) {
        sock_puts (sock, "D\n");  /* tell server we're disconnecting */
        if (ThreadRunning)
            sock_gets (sock, &buffer[0], sizeof (buffer));
        close (sock);
    }

    gtk_widget_destroy (GTK_WIDGET (widget));
    gtk_main_quit ();

    /* restore original terminal settings */
    if (fd >= 0) {
        tcsetattr (fd, TCSANOW, &oldtermios);
        close (fd);
    }
}

void
ChangeNSBID (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *NSBIDInv = "This ID is an invalid NSB ID.  Use a different ID.",
          *NSBIDDup = "This ID is already being used on this server.  Use a different ID.", *msg[5], holdnew[100];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = "You are not connected to an NSB server.  ";
        msg[1] = "Connect to an NSB server before changing an NSB ID.";
        outMessage (msg);
        return;
    }
    if (ThreadRunning) {
        msg[0] = "Before changing your NSBID remove your ID ";
        msg[1] = "from the Waiting Pool via Waiting Pool->Remove Name.";
        outMessage (msg);
        return;
    }

    msg[0] = "Continuing will reset your personal records.  And, your personal lifetime stats ";
    msg[1] = "as well as any currently ongoing season on the NSB server on ";
    msg[2] = &hs[0];
    msg[3] = " will be moved to the new NSB ID you choose.  Any server-wide records attributed to your old NSB ID will be retained.\n\n";
    msg[4] = "Do you want to continue?\n\n";
    if (!ShallWeContinue (msg))
        return;

    /* do it */
get_nid:
    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    CreateNSBID (1);
    if (!strlen (&buffer[0]))
        return;

    if (strlen (&buffer[0]) == 1) {
        msg[0] = NSBIDInv;
        outMessage (msg);
        goto get_nid;
    }

    strcpy (&holdnew[0], &buffer[0]);
    strcpy (&buffer[0], "C");
    strcat (&buffer[0], &holdnew[0]);
    sock_puts (sock, &buffer[0]);

    if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0 || !strcmp (&buffer[0], "-1")) {
        msg[0] = "An error occurred.  ";
        msg[1] = "Your NSB ID could not be changed.";
        outMessage (msg);
        return;
    }
    if (!strcmp (&buffer[0], "dup")) {
        msg[0] = NSBIDDup;
        outMessage (msg);
        goto get_nid;
    }
    if (!strcmp (&buffer[0], "inv")) {
        msg[0] = NSBIDInv;
        outMessage (msg);
        goto get_nid;
    }

    /* success */
    holdnew[strlen (&holdnew[0]) - 1] = '\0';
    strcpy (&nsbid[0], &holdnew[0]);

    strcpy (&work[0], "New NSB ID of ");
    strcat (&work[0], &holdnew[0]);
    strcat (&work[0], " accepted on ");
    strcat (&work[0], &hs[0]);
    strcat (&work[0], "\n");
    Add2TextWindow (&work[0], 0);

    DisplayTopHalf (0);
}

void
DeleteNSBID (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = "You are not connected to an NSB server.  ";
        msg[1] = "Connect to an NSB server before deleting an NSB ID.";
        outMessage (msg);
        return;
    }

    msg[0] = "Continuing will remove your personal lifetime stats and your personal records ";
    msg[1] = "as well as any currently ongoing season from the NSB server on ";
    msg[2] = &hs[0];
    msg[3] = ".  However, any server-wide records attributed to this NSB ID will be retained.\n\n";
    msg[4] = "Do you want to continue?\n\n";
    if (!ShallWeContinue (msg))
        return;

    /* do it */
    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    sock_puts (sock, "R\n");  /* tell server */

    /* get results */
    sock_gets (sock, &buffer[0], sizeof (buffer));

    if (!strcmp (&buffer[0], "-1")) {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;
        msg[0] = "An error occurred.  ";
        msg[1] = "Your NSB ID could not be removed.";
        outMessage (msg);
    }
    else {
        strcpy (&work[0], "NSB ID ");
        strcat (&work[0], &nsbid[0]);
        strcat (&work[0], " removed from server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], "\n");
        Add2TextWindow (&work[0], 0);

        strcpy (&work[0], "Connection to server ");
        strcat (&work[0], &hs[0]);
        strcat (&work[0], " severed.\n");
        Add2TextWindow (&work[0], 0);

        sname[0] = '\0';
        connected = 0;
        if (ThreadRunning)
            sock_gets (sock, &buffer[0], sizeof (buffer));
        close (sock);

        SetLeagueUnderWay (0);
        SetSeriesUnderWay (0);

        DisplayTopHalf (0);
    }
}

void
WhoIsWaiting (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = "You are not connected to an NSB server.  ";
        msg[1] = "Connect to an NSB server before doing anything with the waiting pool.\n\n";
        outMessage (msg);
        return;
    }

    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    sock_puts (sock, "ZV\n");  /* tell server */

    /* get results */
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "ERROR")) {
        msg[0] = "There is an error with the NSB server.  ";
        msg[1] = "Possibly the waiting pool is not functioning on the currently connected NSB server.\n\n";
        outMessage (msg);
        return;
    }
    else
        if (!strlen (&buffer[0])) {
            msg[0] = "Currently there is no one waiting to play.\n\n";
            outMessage (msg);
            return;
        }
        else
            DisplayWP ();
}

void
AddName (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = "You are not connected to an NSB server.  ";
        msg[1] = "Connect to an NSB server before doing anything with the waiting pool.\n\n";
        outMessage (msg);
        return;
    }

    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    sock_puts (sock, "ZA\n");  /* tell server */

    /* get results */
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "ERROR")) {
        msg[0] = "There is an error with the NSB server.  ";
        msg[1] = "Possibly the waiting pool is not functioning on the currently connected NSB server.\n\n";
        outMessage (msg);
    }
    else
        if (!strcmp (&buffer[0], "OK")) {
            msg[0] = "OK, your ID has been added to the waiting pool.  ";
            msg[1] = "You will be informed if another user challenges you.\n\n";
            outMessage (msg);

            /* listen on another socket for challenges */
            if (!g_thread_create (Wait4Challenge, NULL, FALSE, NULL))
                syslog (LOG_INFO, "error trying to create new thread");
        }
        else
            if (!strcmp (&buffer[0], "DUP")) {
                msg[0] = "Your ID is already in the waiting pool.\n\n";
                outMessage (msg);
            }
            else {
                msg[0] = "Your ID could not be added to the waiting pool.  ";
                msg[1] = "Try again later.\n\n";
                outMessage (msg);
            }
}

void
RemoveName (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5];
    gint x;

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (!connected) {
        msg[0] = "You are not connected to an NSB server.  ";
        msg[1] = "Connect to an NSB server before doing anything with the waiting pool.\n\n";
        outMessage (msg);
        return;
    }

    for (x = 0; x < 5; x ++)
        msg[x] = NULL;

    if (!ThreadRunning) {
        msg[0] = "Your ID is not in the waiting pool on this particular server.\n\n";
        outMessage (msg);
    }
    else {
        sock_puts (sock, "ZR\n");  /* tell server */

        /* get results */
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "ERROR")) {
            msg[0] = "There is an error with the NSB server.  ";
            msg[1] = "Possibly the waiting pool is not functioning on the currently connected NSB server.\n\n";
            outMessage (msg);
        }
        else
            if (!strcmp (&buffer[0], "OK")) {
                msg[0] = "OK, your ID has been removed from the waiting pool.\n\n";
                outMessage (msg);
            }
    }
}

void
Request2Play (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gchar *msg[5], work[100];
    gint x;

    netgame = 0;
    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    /* first make sure there is a waiting pool */
    sock_puts (sock, "ZV\n");

    /* get results */
    sock_gets (sock, &buffer[0], sizeof (buffer));
    if (!strcmp (&buffer[0], "ERROR")) {
        msg[0] = "There is an error with the NSB server.  ";
        msg[1] = "Possibly the waiting pool is not functioning on the currently connected NSB server.\n\n";
        outMessage (msg);
        return;
    }
    else
        if (!strlen (&buffer[0])) {
            msg[0] = "Currently there is no one waiting to play.\n\n";
            outMessage (msg);
            return;
        }

    /* if we got this far then there are names in the pool so get a name from the user */
    if (GetRequestee ()) {
        if (!strlen (&RequesteeID[0])) {
            msg[0] = "Please supply an NSBID.\n\n";
            outMessage (msg);
            return;
        }
        /* upon proceding the following will enable the dialog created in GetRequestee () to be destroyed */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        strcpy (&buffer[0], "ZP");
        strcat (&buffer[0], &RequesteeID[0]);
        strcat (&buffer[0], "\n");
        sock_puts (sock, &buffer[0]);
        sock_gets (sock, &buffer[0], sizeof (buffer));
        if (!strcmp (&buffer[0], "ERROR")) {
            msg[0] = "There is an error with the NSB server.  ";
            msg[1] = "Possibly the waiting pool is not functioning on the currently connected NSB server";
            msg[2] = "or the ID you asked to play has removed himself from the pool.  Try another NSBID.\n\n";
            outMessage (msg);
        }
        if (!strcmp (&buffer[0], "NOTPLAY")) {
            msg[0] = "The NSBID you requested does not want to play a game.  Try another NSBID.\n\n";
            outMessage (msg);
        }
        if (!strcmp (&buffer[0], "NOTHERE")) {
            msg[0] = "The NSBID you requested is not in the waiting pool.  Try another NSBID.\n\n";
            outMessage (msg);
        }
        if (!strcmp (&buffer[0], "PLAYSELF")) {
            msg[0] = "Playing with yourself is not allowed ... over a network.\n\n";
            outMessage (msg);
        }
        if (!strncmp (&buffer[0], "COMM ", 5)) {
            msg[0] = "Request to play accepted.  Wait for a message to proceed.\n\n";
            outMessage (msg);
            /* upon proceding the following will enable the dialog created in outMessage () to be destroyed */
            while (gtk_events_pending ())
                gtk_main_iteration ();

            strcpy (&work[0], &buffer[5]);

            sleep (2);  /* give the server time to open the port before connecting */
            sockhvh = make_connection (&work[0], SOCK_STREAM, &hs[0], 1);
            if (sockhvh == -1) {
                syslog (LOG_INFO, "error opening socket for playing a network game");
                return;
            }

            /* this is the challenger */
            challenger_ind = 1;
            PlayNSB1GameOverNetwork ();
        }
    }
}

void
PlayNSB1GameAgainstHumanOverNetwork (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    Request2Play (NULL, 0, NULL);
}

char challenger[50];

/* wait for a gameplay challenge from another user */
void *
Wait4Challenge () {
    int x, port = 0, listensock, connected;
    char buffer[5000], *msg[5];

    ThreadRunning = 1;
    listensock = -1;
    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    wsock = get_connection_nofork (SOCK_STREAM, port, &listensock);

    connected = 1;
    while (connected) {
        /* get communication */
        if (sock_gets (wsock, &buffer[0], sizeof (buffer)) < 0) {
            msg[0] = "There is a problem with the server.\n\n";
            outMessage (msg);
            connected = 0;
        }

        if (buffer[0] == 'R')
            /* this user removed his ID from the waiting pool so end this thread and close the socket */
            connected = 0;

        if (buffer[0] != 'C')
            continue;

        strcpy (&challenger[0], &buffer[1]);

        /* add idle function to process requests to play from challengers */
        g_idle_add ((GSourceFunc) PlayChallenger, NULL);

        /* wait for PlayChallenger() to finish */
        g_mutex_lock (data_mutex);
        FromThread = 0;
        while (!FromThread)
            g_cond_wait (data_cond, data_mutex);
        g_mutex_unlock (data_mutex);

        if (!ShallWeContinue4Thread) {
            /* this challengee does not want to play this challenger */
            sock_puts (wsock, "NO\n");
            for (x = 0; x < 5; x++)
                msg[x] = NULL;
            continue;
        }
        else {
            /* this challengee wants to play */
            char work[50], cport[50], *blank;

            /* tell server to open a port for receiving communications from the challenger (the challengee server
               process will control gameplay) */
            sock_puts (sock, "O\n");
            /* get back port number to use for communicating */
            if (sock_gets (sock, &buffer[0], sizeof (buffer)) < 0) {
                msg[0] = "There is a problem with the server.\n\n";
                outMessage (msg);
                sock_puts (wsock, "ERROR\n");
                connected = 0;
                continue;
            }
            blank = index (&buffer[0], ' ');
            blank++;
            strcpy (&cport[0], blank);

            strcpy (&work[0], "OK ");
            strcat (&work[0], &cport[0]);
            strcat (&work[0], "\n");

            /* tell the pool manager we want to play and pass along the port number to use for communicating */
            sock_puts (wsock, &work[0]);

            g_idle_add ((GSourceFunc) PlayNetGame, NULL);

            /* we no longer need this thread */
            connected = 0;
        }
    }

    shutdown (wsock, SHUT_RDWR);
    close (wsock);
    ThreadRunning = 0;
    return NULL;
}

GtkWidget *dlgFile;

/* display a dialog which calls for a user response */
gboolean
PlayChallenger () {
    gchar labeltext[500];
    GtkWidget *label = NULL, *hbox, *separator, *okbutton, *disbutton;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Play Challenger?");
    gtk_signal_connect (GTK_OBJECT (dlgFile), "delete_event", GTK_SIGNAL_FUNC (donot_delete_event), 0);
    gtk_window_set_modal (GTK_WINDOW (dlgFile), TRUE);

    strcpy (&labeltext[0], &challenger[0]);
    strcat (&labeltext[0], " challenges you to a game.  Do you accept?\n\n");

    label = gtk_label_new (labeltext);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), separator, FALSE, TRUE, 0);

    hbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), hbox, FALSE, FALSE, 0);

    okbutton = gtk_button_new_with_label ("ACCEPT");
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (PlayOK), dlgFile);
    disbutton = gtk_button_new_with_label ("Decline");
    g_signal_connect (G_OBJECT (disbutton), "clicked", G_CALLBACK (PCscatDestroyDialog), NULL);
    gtk_box_pack_start (GTK_BOX (hbox), okbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), disbutton, TRUE, TRUE, 0);

    gtk_widget_show_all (dlgFile);
    return FALSE;
}

void
PlayOK (GtkWidget *widget, gpointer *pdata) {
    g_mutex_lock (data_mutex);
    ShallWeContinue4Thread = 1;
    FromThread = 1;
    g_cond_signal (data_cond);
    g_mutex_unlock (data_mutex);
    DestroyDialog (dlgFile, dlgFile);
}

void
PCscatDestroyDialog (GtkWidget *widget, gpointer *pdata) {
    g_mutex_lock (data_mutex);
    ShallWeContinue4Thread = 0;
    FromThread = 1;
    g_cond_signal (data_cond);
    g_mutex_unlock (data_mutex);
    DestroyDialog (dlgFile, dlgFile);
}

/* this is the challengee */
gboolean
PlayNetGame () {
    challenger_ind = 0;
    PlayNSB1GameOverNetwork ();

    return FALSE;
}

static GtkItemFactoryEntry menu_items[] = {
   { "/Game", NULL, 0, 0, "<Branch>" },
   { "/Game/Connect to a Server", NULL, ConnectToServer, 0 },
   { "/Game/Disconnect from Current Server", NULL, DisconnectFromServer, 0 },
   { "/Game/separator1", NULL, NULL, 0, "<Separator>" },
   { "/Game/Play/One Game Against Computer", NULL, PlayNSB1GameAgainstComputer, 0 },
   { "/Game/Play/One Game Against Human/Same Computer", NULL, PlayNSB1GameAgainstHumanSameComputer, 0 },
   { "/Game/Play/One Game Against Human/Over Network", NULL, PlayNSB1GameAgainstHumanOverNetwork, 0 },
   { "/Game/Play/Watch One Game Computer vs Computer", NULL, PlayNSBWatch1GameComputerVsComputer, 0 },
   { "/Game/Play/Portion of Season", NULL, PlayNSBPortionOfLeague, 0 },
   { "/Game/Play/Game(s) in a Series", NULL, PlaySeries, 0 },
   { "/Game/separator2", NULL, NULL, 0, "<Separator>" },
   { "/Game/Preferences", NULL, Preferences, 0 },
   { "/Game/separator3", NULL, NULL, 0, "<Separator>" },
   { "/Game/Quit", "<control>Q", Quit, 0 },

   { "/Administration", NULL, 0, 0, "<Branch>" },
   { "/Administration/Establish Season", NULL, SetUpLeagueChoices, 0 },
   { "/Administration/Establish a Series", NULL, SetUpSeries, 0 },
   { "/Administration/Injury Report", NULL, LeagueInjuryReport, 0 },
   { "/Administration/Season Autoplay/Set-up", NULL, SeasonAutoplayEst, 0 },
   { "/Administration/Season Autoplay/Activation", NULL, SeasonAutoplayAS, 0 },
   { "/Administration/Season Autoplay/Execute Once Now", NULL, SeasonAutoplayEx, 0 },
   { "/Administration/Users With Accounts on Connected Server", NULL, Users, 0 },
   { "/Administration/separator4", NULL, NULL, 0, "<Separator>" },
   { "/Administration/Create, Edit, Rename or Delete Team", NULL, SetUpTeamChoices, 0 },
   { "/Administration/Some Good Individual Seasons", NULL, GIndS, 0 },
   { "/Administration/separator5", NULL, NULL, 0, "<Separator>" },
   { "/Administration/Change My NSB ID on Connected Server", NULL, ChangeNSBID, 0 },
   { "/Administration/Delete My NSB ID on Connected Server", NULL, DeleteNSBID, 0 },

   { "/Statistics", NULL, 0, 0, "<Branch>" },
   { "/Statistics/Current NSB Season/Standings", NULL, NSBStandings, 0 },
   { "/Statistics/Current NSB Season/Category Leaders/Regular Season", NULL, NSBCategoryLeadersRS, 0 },
   { "/Statistics/Current NSB Season/Category Leaders/Post-Season", NULL, NSBCategoryLeadersPS, 0 },
   { "/Statistics/Current NSB Season/By Team", NULL, NSBByTeam, 0 },
   { "/Statistics/Current NSB Season/Team Totals/Regular Season", NULL, NSBRegularSeasonTeamTotals, 0 },
   { "/Statistics/Current NSB Season/Team Totals/Post-Season", NULL, NSBPostSeasonTeamTotals, 0 },
   { "/Statistics/Current NSB Season/Awards/Most Valuable Player", NULL, NSBMVP, 0 },
   { "/Statistics/Current NSB Season/Awards/Cy Young", NULL, NSBCyYoung, 0 },
   { "/Statistics/Current NSB Season/Awards/Gold Gloves", NULL, NSBGoldGlove, 0 },
   { "/Statistics/Current NSB Season/Awards/Silver Sluggers", NULL, NSBSilverSlugger, 0 },
   { "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Game Records", NULL, NSBRecordsPersonalGame, 0 },
   { "/Statistics/All NSB Seasons/Regular Season Records/Just Your Seasons/Season Records", NULL, NSBRecordsPersonalSeason, 0 },
   { "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Game Records", NULL, NSBRecordsAllGame, 0 },
   { "/Statistics/All NSB Seasons/Regular Season Records/All Seasons for All Users/Season Records", NULL, NSBRecordsAllSeason, 0 },
   { "/Statistics/All NSB Seasons/Lifetime Leaders/Regular Season", NULL, NSBLifetimeRS, 0 },
   { "/Statistics/All NSB Seasons/Lifetime Leaders/Post-Season", NULL, NSBLifetimePS, 0 },
   { "/Statistics/All NSB Seasons/By Team", NULL, NSBLifetimeByTeam, 0 },
   { "/Statistics/Real Life/Season Results", NULL, RealLifeSeasonResults, 0 },
   { "/Statistics/Real Life/Category Leaders", NULL, RealLifeCategoryLeaders, 0 },
   { "/Statistics/Real Life/By Team", NULL, RealLifeByTeam, 0 },
   { "/Statistics/Real Life/Team Totals", NULL, RealLifeTeamTotals, 0 },
   { "/Statistics/Real Life/By Player", NULL, RealLifeByPlayer, 0 },
   { "/Statistics/Real Life/Notable Accomplishments", NULL, NAcc, 0 },
   { "/Statistics/Real Life/Hall of Fame Members", NULL, HOF, 0 },
   { "/Statistics/Real Life/Awards/Most Valuable Player", NULL, MVP, 0 },
   { "/Statistics/Real Life/Awards/Cy Young", NULL, CyYoung, 0 },
   { "/Statistics/Real Life/Awards/Gold Gloves", NULL, GoldGlove, 0 },
   { "/Statistics/Real Life/Awards/Silver Sluggers", NULL, SilverSlugger, 0 },
   { "/Statistics/Real Life/Awards/Rookie of the Year", NULL, Rookie, 0 },
   { "/Statistics/User-Created Teams/By Team", NULL, UserCreatedByTeam, 0 },
   { "/Statistics/User-Created Teams/Team Totals", NULL, UserCreatedTeamTotals, 0 },
   { "/Statistics/Current Series/Series Status", NULL, SeriesStatus, 0 },
   { "/Statistics/Current Series/Category Leaders", NULL, NSBCategoryLeadersSeries, 0 },
   { "/Statistics/Current Series/By Team", NULL, NSBSeriesByTeam, 0 },
   { "/Statistics/Current Series/Team Totals", NULL, NSBSeriesTeamTotals, 0 },

   { "/Waiting Pool", NULL, 0, 0, "<Branch>" },
   { "/Waiting Pool/Who is Waiting", NULL, WhoIsWaiting, 0 },
   { "/Waiting Pool/Add Name", NULL, AddName, 0 },
   { "/Waiting Pool/Remove Name", NULL, RemoveName, 0 },
   { "/Waiting Pool/Request to Play", NULL, Request2Play, 0 },

   { "/Xtras", NULL, 0, 0, "<Branch>" },
   { "/Xtras/Play \"Take Me Out to the Ballgame\"", NULL, PlayTakeMeOut2TheBallgame, 0 },
   { "/Xtras/Play Abbott & Costello's \"Who's On First?\"", NULL, PlayAbbottAndCostello, 0 },
   { "/Xtras/Play Jackie Gleason's \"Casey at the Bat\"", NULL, PlayJackieGleason, 0 },
   { "/Xtras/Play John Fogerty's \"Centerfield\"", NULL, PlayCenterfield, 0 },
   { "/Xtras/separator6", NULL, NULL, 0, "<Separator>" },
   { "/Xtras/This Day in Baseball", NULL, TDIBFromMenu, 0 },
   { "/Xtras/separator7", NULL, NULL, 0, "<Separator>" },
   { "/Xtras/Evaluate Team Seasons", NULL, BestRLTeams, 0 },
   { "/Xtras/Evaluate Player Seasons", NULL, ScoreSeasons, 0 },

   { "/Help", NULL, 0, 0, "<LastBranch>" },
   { "/Help/Help", NULL, Help, 0 },
   { "/Help/Abbreviations", NULL, Abbrev, 0 },
   { "/Help/About GTK Client for NSB", NULL, AboutGTKNSBClient, 0 }
};

void
CreateMenus (GtkWidget *window) {
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new ();
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<menu>", accel_group);
    gtk_item_factory_create_items (item_factory, 83, menu_items, NULL);
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
    gtk_window_set_title (GTK_WINDOW (window), "GTK Client for NSB");
    gtk_container_border_width (GTK_CONTAINER (window), 0);
    mainbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), mainbox);
    gtk_box_pack_start (GTK_BOX (mainbox), gtk_item_factory_get_widget (item_factory, "<menu>"), FALSE, FALSE, 0);
}

int
main (int argc, char *argv[]) {
    gint boot = 1;
    struct termios t;

    /* if NSB is executed from the command line then save terminal settings so they can be restored when quitting */
    fd = open (ctermid (NULL), O_RDWR | O_NOCTTY );
    if (fd >= 0) {
        tcgetattr (fd, &t);
        oldtermios = t;
    }

    openlog ("gtknsbclient", LOG_NDELAY, LOG_USER);

    if (argc > 1)
        /* is this a non-interactive League-Autoplay running? */
        if (!strcmp ("LEAGUEAUTOPLAY", argv[1])) {
            NSBAutoPlay (argc, argv);
            exit (0);
        }

    if (!g_thread_supported ())
        g_thread_init (NULL);

    /* initialize GTK interface */
    gtk_init (&argc, &argv);

    /* initialize NSB client */
    nsb_init ();

    /* create a main window */
    mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize (mainwin, WINSIZEX, WINSIZEY);
    gtk_window_set_icon_from_file (GTK_WINDOW (mainwin), "/usr/local/share/NSB/nsb-icon.jpg", NULL);
    gtk_widget_realize (mainwin);

    /* add event handlers */
    gtk_widget_add_events (mainwin, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect (GTK_OBJECT (mainwin), "delete_event", GTK_SIGNAL_FUNC (delete_event), 0);

    /* create menubar for the main window */
    CreateMenus (mainwin);

    /* localhost is the default location of the NSB server to which to connect at startup
       if there is an argument use that as the server name */
    if (argc == 1)
        strcpy (&hs[0], "localhost");
    else
        strcpy (&hs[0], argv[1]);

    /* max number of arguments is 1 */
    if (argc > 2) {
        fprintf (stderr, "\nUsage:  %s [host]\n", argv[0]);
        fprintf (stderr, "where host is the machine which is running the\n");
        fprintf (stderr, "NetStats Baseball server.\n\n");
        exit (-1);
    }

    /* add two tables within the main box/window */
    statustable = gtk_table_new (2, 3, TRUE);
    texttable = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (texttable), 0, 2);
    gtk_table_set_col_spacing (GTK_TABLE (texttable), 0, 2);
    gtk_container_add (GTK_CONTAINER (mainbox), statustable);
    gtk_container_add (GTK_CONTAINER (mainbox), texttable);

    /* create a text widget */
    text = gtk_text_new (NULL, NULL);
    gtk_text_set_editable (GTK_TEXT (text), FALSE);
    gtk_table_attach (GTK_TABLE (texttable), text, 0, 1, 0, 12, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* add a vertical scrollbar to the text widget */
    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
    gtk_table_attach (GTK_TABLE (texttable), vscrollbar, 1, 2, 0, 12, GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* get the system color map and allocate the color red */
    cmap = gdk_colormap_get_system ();
    color.red = 0xffff;
    color.green = 0;
    color.blue = 0;
    if (!gdk_color_alloc (cmap, &color))
        g_error ("couldn't allocate color");

    /* realizing a widget creates a window for it, ready for us to insert some text */
    gtk_widget_realize (text);

    /* try to connect to an NSB server at start-up ... either the server designated in the argument or localhost */
    gtk_init_add ((GtkFunction) Connect2NSBServer, &boot);
    /* show TDIB at start-up ... maybe */
    gtk_init_add ((GtkFunction) ShowTDIB, &boot);

    data_cond = g_cond_new ();
    data_mutex = g_mutex_new ();

    /* show main window */
    gtk_widget_show_all (mainwin);

    /* the last thing to get called */
    gtk_main ();

    return 0;
}

