
/* Season Autoplay set-up */

#include <syslog.h>
#include "gtknsbc.h"
#include "prototypes.h"
#include "cglobal.h"

GtkWidget *dlgFile, *buttonls, *buttonbs, *buttonstd, *buttoninj, *buttonbg, *buttonbab, *buttonbr, *buttonbh, *buttonb2b, *buttonb3b, *buttonbhr, *buttonbbi,
          *buttonbbb, *buttonbso, *buttonbhbp, *buttonbdp, *buttonbsb, *buttonbcs, *buttonbibb, *buttonbsh, *buttonbsf, *buttonbba, *buttonbsa,
          *buttonboba, *buttonpg, *buttonpgs, *buttonpip, *buttonpw, *buttonpl, *buttonps, *buttonpbfp, *buttonph, *buttonp2b, *buttonp3b, *buttonphr,
          *buttonpr, *buttonper, *buttonpbi, *buttonpcg, *buttonpgf, *buttonpsho, *buttonpsopp, *buttonpsb, *buttonpcs, *buttonpbb, *buttonpso, *buttonpibb,
          *buttonpsh, *buttonpsf, *buttonpwp, *buttonpb, *buttonphb, *buttonpab, *buttonpera, *buttonppct, *buttonpoba, *buttonfg[8], *buttonfpo[7],
          *buttonfdp[7], *buttonfa[7], *buttonfe[7], *buttonfpct[7], *buttonfpb;

void
SeasonAutoplayEst (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    char labeltext[500];
    GtkWidget *label, *button, *buttons, *buttonr, *box1, *boxh, *sw, *tableh, *table, *hitlabel, *pitchlabel, *fieldlabel;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Setup Autoplay");
    gtk_window_set_default_size (GTK_WINDOW (dlgFile), 750, 500);

    strcpy (&labeltext[0], "Select reports to be included in emails");
    label = gtk_label_new (labeltext);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    box1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), box1, TRUE, TRUE, 0);

    boxh = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (boxh), 8);
    gtk_box_pack_start (GTK_BOX (box1), boxh, TRUE, TRUE, 0);

    buttonls = gtk_check_button_new_with_label ("Game Line Scores");
    gtk_box_pack_start (GTK_BOX (boxh), buttonls, TRUE, TRUE, 0);
    buttonbs = gtk_check_button_new_with_label ("Game Box Scores");
    gtk_box_pack_start (GTK_BOX (boxh), buttonbs, TRUE, TRUE, 0);
    buttonstd = gtk_check_button_new_with_label ("Standings");
    gtk_box_pack_start (GTK_BOX (boxh), buttonstd, TRUE, TRUE, 0);
    buttoninj = gtk_check_button_new_with_label ("Injury Report");
    gtk_box_pack_start (GTK_BOX (boxh), buttoninj, TRUE, TRUE, 0);

    strcpy (&labeltext[0], "Regular Season Category Leaders");
    label = gtk_label_new (labeltext);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    tableh = gtk_table_new (1, 4, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (tableh), 2);
    gtk_table_set_col_spacings (GTK_TABLE (tableh), 2);

    /* insert the column headers */
    hitlabel = g_object_new (GTK_TYPE_LABEL, "label", "Hitting", NULL);
    pitchlabel = g_object_new (GTK_TYPE_LABEL, "label", "Pitching", NULL);
    fieldlabel = g_object_new (GTK_TYPE_LABEL, "label", "Fielding", NULL);
    label = g_object_new (GTK_TYPE_LABEL, "label", " ", NULL);
    gtk_table_attach_defaults (GTK_TABLE (tableh), GTK_WIDGET (hitlabel), 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tableh), GTK_WIDGET (pitchlabel), 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tableh), GTK_WIDGET (fieldlabel), 2, 3, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (tableh), GTK_WIDGET (label), 3, 4, 0, 1);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlgFile)->vbox), tableh);

    box1 = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), box1, TRUE, TRUE, 0);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (box1), sw);

    table = gtk_table_new (33, 5, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);
    gtk_table_set_col_spacings (GTK_TABLE (table), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), table);

    /* insert the buttons */
    buttonbg = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbg), 0, 1, 1, 2);
    buttonbab = gtk_check_button_new_with_label ("At Bats");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbab), 0, 1, 2, 3);
    buttonbr = gtk_check_button_new_with_label ("Runs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbr), 0, 1, 3, 4);
    buttonbh = gtk_check_button_new_with_label ("Hits");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbh), 0, 1, 4, 5);
    buttonb2b = gtk_check_button_new_with_label ("Doubles");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonb2b), 0, 1, 5, 6);
    buttonb3b = gtk_check_button_new_with_label ("Triples");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonb3b), 0, 1, 6, 7);
    buttonbhr = gtk_check_button_new_with_label ("Homers");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbhr), 0, 1, 7, 8);
    buttonbbi = gtk_check_button_new_with_label ("Runs Batted In");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbbi), 0, 1, 8, 9);
    buttonbbb = gtk_check_button_new_with_label ("Walks");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbbb), 0, 1, 9, 10);
    buttonbso = gtk_check_button_new_with_label ("Strike Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbso), 0, 1, 10, 11);
    buttonbhbp = gtk_check_button_new_with_label ("Hit by Pitcher");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbhbp), 0, 1, 11, 12);
    buttonbdp = gtk_check_button_new_with_label ("Grounded Into DP");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbdp), 0, 1, 12, 13);
    buttonbsb = gtk_check_button_new_with_label ("Stolen Bases");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbsb), 0, 1, 13, 14);
    buttonbcs = gtk_check_button_new_with_label ("Caught Stealing");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbcs), 0, 1, 14, 15);
    buttonbibb = gtk_check_button_new_with_label ("Intentional Bases on Balls");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbibb), 0, 1, 15, 16);
    buttonbsh = gtk_check_button_new_with_label ("Sacrifices");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbsh), 0, 1, 16, 17);
    buttonbsf = gtk_check_button_new_with_label ("Sacrifice Flies");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbsf), 0, 1, 17, 18);
    buttonbba = gtk_check_button_new_with_label ("Batting Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbba), 0, 1, 18, 19);
    buttonbsa = gtk_check_button_new_with_label ("Slugging Percentage");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonbsa), 0, 1, 19, 20);
    buttonboba = gtk_check_button_new_with_label ("On-Base Percentage");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonboba), 0, 1, 20, 21);

    buttonpg = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpg), 1, 2, 1, 2);
    buttonpgs = gtk_check_button_new_with_label ("Games Started");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpgs), 1, 2, 2, 3);
    buttonpip = gtk_check_button_new_with_label ("Innings Pitched");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpip), 1, 2, 3, 4);
    buttonpw = gtk_check_button_new_with_label ("Wins");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpw), 1, 2, 4, 5);
    buttonpl = gtk_check_button_new_with_label ("Losses");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpl), 1, 2, 5, 6);
    buttonps = gtk_check_button_new_with_label ("Saves");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonps), 1, 2, 6, 7);
    buttonpbfp = gtk_check_button_new_with_label ("Batters Facing Pitcher");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpbfp), 1, 2, 7, 8);
    buttonph = gtk_check_button_new_with_label ("Hits");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonph), 1, 2, 8, 9);
    buttonp2b = gtk_check_button_new_with_label ("Doubles");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonp2b), 1, 2, 9, 10);
    buttonp3b = gtk_check_button_new_with_label ("Triples");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonp3b), 1, 2, 10, 11);
    buttonphr = gtk_check_button_new_with_label ("Homers");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonphr), 1, 2, 11, 12);
    buttonpr = gtk_check_button_new_with_label ("Runs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpr), 1, 2, 12, 13);
    buttonper = gtk_check_button_new_with_label ("Earned Runs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonper), 1, 2, 13, 14);
    buttonpbi = gtk_check_button_new_with_label ("Runs Batted In");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpbi), 1, 2, 14, 15);
    buttonpcg = gtk_check_button_new_with_label ("Complete Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpcg), 1, 2, 15, 16);
    buttonpgf = gtk_check_button_new_with_label ("Games Finished");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpgf), 1, 2, 16, 17);
    buttonpsho = gtk_check_button_new_with_label ("Shutouts");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpsho), 1, 2, 17, 18);
    buttonpsopp = gtk_check_button_new_with_label ("Save Opportunities");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpsopp), 1, 2, 18, 19);
    buttonpsb = gtk_check_button_new_with_label ("Stolen Bases");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpsb), 1, 2, 19, 20);
    buttonpcs = gtk_check_button_new_with_label ("Caught Stealing");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpcs), 1, 2, 20, 21);
    buttonpbb = gtk_check_button_new_with_label ("Walks");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpbb), 1, 2, 21, 22);
    buttonpso = gtk_check_button_new_with_label ("Strike Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpso), 1, 2, 22, 23);
    buttonpibb = gtk_check_button_new_with_label ("Intentional Bases on Balls");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpibb), 1, 2, 23, 24);
    buttonpsh = gtk_check_button_new_with_label ("Sacrifices");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpsh), 1, 2, 24, 25);
    buttonpsf = gtk_check_button_new_with_label ("Sacrifice Flies");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpsf), 1, 2, 25, 26);
    buttonpwp = gtk_check_button_new_with_label ("Wild Pitches");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpwp), 1, 2, 26, 27);
    buttonpb = gtk_check_button_new_with_label ("Balks");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpb), 1, 2, 27, 28);
    buttonphb = gtk_check_button_new_with_label ("Hit Batters");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonphb), 1, 2, 28, 29);
    buttonpab = gtk_check_button_new_with_label ("Opponent At Bats");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpab), 1, 2, 29, 30);
    buttonpera = gtk_check_button_new_with_label ("Earned Run Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpera), 1, 2, 30, 31);
    buttonppct = gtk_check_button_new_with_label ("Winning Percentage");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonppct), 1, 2, 31, 32);
    buttonpoba = gtk_check_button_new_with_label ("Opponent Batting Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonpoba), 1, 2, 32, 33);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Outfield -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 2, 3, 1, 2);
    buttonfg[0] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[0]), 2, 3, 2, 3);
    buttonfpo[0] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[0]), 2, 3, 3, 4);
    buttonfdp[0] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[0]), 2, 3, 4, 5);
    buttonfa[0] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[0]), 2, 3, 5, 6);
    buttonfe[0] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[0]), 2, 3, 6, 7);
    buttonfpct[0] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[0]), 2, 3, 7, 8);

    label = g_object_new (GTK_TYPE_LABEL, "label", "First Base -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 2, 3, 9, 10);
    buttonfg[1] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[1]), 2, 3, 10, 11);
    buttonfpo[1] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[1]), 2, 3, 11, 12);
    buttonfdp[1] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[1]), 2, 3, 12, 13);
    buttonfa[1] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[1]), 2, 3, 13, 14);
    buttonfe[1] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[1]), 2, 3, 14, 15);
    buttonfpct[1] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[1]), 2, 3, 15, 16);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Second Base -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 2, 3, 17, 18);
    buttonfg[2] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[2]), 2, 3, 18, 19);
    buttonfpo[2] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[2]), 2, 3, 19, 20);
    buttonfdp[2] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[2]), 2, 3, 20, 21);
    buttonfa[2] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[2]), 2, 3, 21, 22);
    buttonfe[2] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[2]), 2, 3, 22, 23);
    buttonfpct[2] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[2]), 2, 3, 23, 24);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Third Base -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 2, 3, 25, 26);
    buttonfg[3] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[3]), 2, 3, 26, 27);
    buttonfpo[3] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[3]), 2, 3, 27, 28);
    buttonfdp[3] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[3]), 2, 3, 28, 29);
    buttonfa[3] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[3]), 2, 3, 29, 30);
    buttonfe[3] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[3]), 2, 3, 30, 31);
    buttonfpct[3] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[3]), 2, 3, 31, 32);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Shortstop -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 3, 4, 1, 2);
    buttonfg[4] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[4]), 3, 4, 2, 3);
    buttonfpo[4] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[4]), 3, 4, 3, 4);
    buttonfdp[4] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[4]), 3, 4, 4, 5);
    buttonfa[4] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[4]), 3, 4, 5, 6);
    buttonfe[4] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[4]), 3, 4, 6, 7);
    buttonfpct[4] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[4]), 3, 4, 7, 8);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Pitcher -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 3, 4, 9, 10);
    buttonfg[5] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[5]), 3, 4, 10, 11);
    buttonfpo[5] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[5]), 3, 4, 11, 12);
    buttonfdp[5] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[5]), 3, 4, 12, 13);
    buttonfa[5] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[5]), 3, 4, 13, 14);
    buttonfe[5] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[5]), 3, 4, 14, 15);
    buttonfpct[5] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[5]), 3, 4, 15, 16);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Catcher -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 3, 4, 17, 18);
    buttonfg[6] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[6]), 3, 4, 18, 19);
    buttonfpo[6] = gtk_check_button_new_with_label ("Put Outs");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpo[6]), 3, 4, 19, 20);
    buttonfdp[6] = gtk_check_button_new_with_label ("Double Plays");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfdp[6]), 3, 4, 20, 21);
    buttonfa[6] = gtk_check_button_new_with_label ("Assists");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfa[6]), 3, 4, 21, 22);
    buttonfe[6] = gtk_check_button_new_with_label ("Errors");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfe[6]), 3, 4, 22, 23);
    buttonfpb = gtk_check_button_new_with_label ("Passed Balls");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpb), 3, 4, 23, 24);
    buttonfpct[6] = gtk_check_button_new_with_label ("Fielding Average");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfpct[6]), 3, 4, 24, 25);

    label = g_object_new (GTK_TYPE_LABEL, "label", "Designated Hitter -", NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label), 3, 4, 26, 27);
    buttonfg[7] = gtk_check_button_new_with_label ("Games");
    gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (buttonfg[7]), 3, 4, 27, 28);

    SetButtons ();

    button = gtk_button_new_with_label ("CANCEL");
    gtk_signal_connect (GTK_OBJECT (button), "clicked", (GtkSignalFunc) CancelOptions, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), button, TRUE, TRUE, 15);
    buttonr = gtk_button_new_with_label ("TURN ALL OFF");
    gtk_signal_connect (GTK_OBJECT (buttonr), "clicked", (GtkSignalFunc) ResetOptions, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), buttonr, TRUE, TRUE, 15);
    buttons = gtk_button_new_with_label ("SAVE");
    gtk_signal_connect (GTK_OBJECT (buttons), "clicked", (GtkSignalFunc) SaveEOptions, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), buttons, TRUE, TRUE, 15);
    gtk_widget_show_all (dlgFile);
}

void
ResetOptions (GtkWidget *widget, gpointer *pdata) {
    gint x;

    autoplay.active = autoplay.linescore = autoplay.boxscore = autoplay.standings = autoplay.injury = 0;
    autoplay.catl.hitting.games = autoplay.catl.hitting.atbats = autoplay.catl.hitting.runs = autoplay.catl.hitting.hits = autoplay.catl.hitting.doubles =
      autoplay.catl.hitting.triples = autoplay.catl.hitting.homers = autoplay.catl.hitting.rbis = autoplay.catl.hitting.bb = autoplay.catl.hitting.so =
      autoplay.catl.hitting.hbp = autoplay.catl.hitting.dp = autoplay.catl.hitting.sb = autoplay.catl.hitting.cs = autoplay.catl.hitting.ibb =
      autoplay.catl.hitting.sh = autoplay.catl.hitting.sf = autoplay.catl.hitting.ba = autoplay.catl.hitting.sa = autoplay.catl.hitting.oba = 0;
    autoplay.catl.pitching.games = autoplay.catl.pitching.gs = autoplay.catl.pitching.ip = autoplay.catl.pitching.wins = autoplay.catl.pitching.losses =
      autoplay.catl.pitching.sv = autoplay.catl.pitching.bfp = autoplay.catl.pitching.hits = autoplay.catl.pitching.db = autoplay.catl.pitching.tp =
      autoplay.catl.pitching.hr = autoplay.catl.pitching.runs = autoplay.catl.pitching.er = autoplay.catl.pitching.rbi = autoplay.catl.pitching.cg =
      autoplay.catl.pitching.gf = autoplay.catl.pitching.sho = autoplay.catl.pitching.svopp = autoplay.catl.pitching.sb = autoplay.catl.pitching.cs =
      autoplay.catl.pitching.bb = autoplay.catl.pitching.so = autoplay.catl.pitching.ibb = autoplay.catl.pitching.sh = autoplay.catl.pitching.sf =
      autoplay.catl.pitching.wp = autoplay.catl.pitching.b = autoplay.catl.pitching.hb = autoplay.catl.pitching.ab = autoplay.catl.pitching.era =
      autoplay.catl.pitching.pct = autoplay.catl.pitching.oppba = 0;
    for (x = 0; x < 7; x++)
        autoplay.catl.fielding.pos[x].games = autoplay.catl.fielding.pos[x].po = autoplay.catl.fielding.pos[x].dp = autoplay.catl.fielding.pos[x].a =
          autoplay.catl.fielding.pos[x].e = autoplay.catl.fielding.pos[x].pct = 0;
    autoplay.catl.fielding.pos[7].games = autoplay.catl.fielding.pos[6].pb = 0;

    SetButtons ();
}

void
SaveEOptions (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar path2autoplay[1024], *msg[5], *ERR = "The .NSBautoplayrc file cannot be written.";
    FILE *rc;

    autoplay.active = 1;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonls)))
        autoplay.linescore = 1;
    else
        autoplay.linescore = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbs)))
        autoplay.boxscore = 1;
    else
        autoplay.boxscore = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonstd)))
        autoplay.standings = 1;
    else
        autoplay.standings = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttoninj)))
        autoplay.injury = 1;
    else
        autoplay.injury = 0;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbg)))
        autoplay.catl.hitting.games = 1;
    else
        autoplay.catl.hitting.games = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbab)))
        autoplay.catl.hitting.atbats = 1;
    else
        autoplay.catl.hitting.atbats = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbr)))
        autoplay.catl.hitting.runs = 1;
    else
        autoplay.catl.hitting.runs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbh)))
        autoplay.catl.hitting.hits = 1;
    else
        autoplay.catl.hitting.hits = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonb2b)))
        autoplay.catl.hitting.doubles = 1;
    else
        autoplay.catl.hitting.doubles = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonb3b)))
        autoplay.catl.hitting.triples = 1;
    else
        autoplay.catl.hitting.triples = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbhr)))
        autoplay.catl.hitting.homers = 1;
    else
        autoplay.catl.hitting.homers = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbbi)))
        autoplay.catl.hitting.rbis = 1;
    else
        autoplay.catl.hitting.rbis = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbbb)))
        autoplay.catl.hitting.bb = 1;
    else
        autoplay.catl.hitting.bb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbso)))
        autoplay.catl.hitting.so = 1;
    else
        autoplay.catl.hitting.so = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbhbp)))
        autoplay.catl.hitting.hbp = 1;
    else
        autoplay.catl.hitting.hbp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbdp)))
        autoplay.catl.hitting.dp = 1;
    else
        autoplay.catl.hitting.dp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbsb)))
        autoplay.catl.hitting.sb = 1;
    else
        autoplay.catl.hitting.sb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbcs)))
        autoplay.catl.hitting.cs = 1;
    else
        autoplay.catl.hitting.cs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbibb)))
        autoplay.catl.hitting.ibb = 1;
    else
        autoplay.catl.hitting.ibb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbsh)))
        autoplay.catl.hitting.sh = 1;
    else
        autoplay.catl.hitting.sh = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbsf)))
        autoplay.catl.hitting.sf = 1;
    else
        autoplay.catl.hitting.sf = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbba)))
        autoplay.catl.hitting.ba = 1;
    else
        autoplay.catl.hitting.ba = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonbsa)))
        autoplay.catl.hitting.sa = 1;
    else
        autoplay.catl.hitting.sa = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonboba)))
        autoplay.catl.hitting.oba = 1;
    else
        autoplay.catl.hitting.oba = 0;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpg)))
        autoplay.catl.pitching.games = 1;
    else
        autoplay.catl.pitching.games = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpgs)))
        autoplay.catl.pitching.gs = 1;
    else
        autoplay.catl.pitching.gs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpip)))
        autoplay.catl.pitching.ip = 1;
    else
        autoplay.catl.pitching.ip = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpw)))
        autoplay.catl.pitching.wins = 1;
    else
        autoplay.catl.pitching.wins = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpl)))
        autoplay.catl.pitching.losses = 1;
    else
        autoplay.catl.pitching.losses = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonps)))
        autoplay.catl.pitching.sv = 1;
    else
        autoplay.catl.pitching.sv = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpbfp)))
        autoplay.catl.pitching.bfp = 1;
    else
        autoplay.catl.pitching.bfp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonph)))
        autoplay.catl.pitching.hits = 1;
    else
        autoplay.catl.pitching.hits = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonp2b)))
        autoplay.catl.pitching.db = 1;
    else
        autoplay.catl.pitching.db = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonp3b)))
        autoplay.catl.pitching.tp = 1;
    else
        autoplay.catl.pitching.tp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonphr)))
        autoplay.catl.pitching.hr = 1;
    else
        autoplay.catl.pitching.hr = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpr)))
        autoplay.catl.pitching.runs = 1;
    else
        autoplay.catl.pitching.runs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonper)))
        autoplay.catl.pitching.er = 1;
    else
        autoplay.catl.pitching.er = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpbi)))
        autoplay.catl.pitching.rbi = 1;
    else
        autoplay.catl.pitching.rbi = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpcg)))
        autoplay.catl.pitching.cg = 1;
    else
        autoplay.catl.pitching.cg = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpgf)))
        autoplay.catl.pitching.gf = 1;
    else
        autoplay.catl.pitching.gf = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpsho)))
        autoplay.catl.pitching.sho = 1;
    else
        autoplay.catl.pitching.sho = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpsopp)))
        autoplay.catl.pitching.svopp = 1;
    else
        autoplay.catl.pitching.svopp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpsb)))
        autoplay.catl.pitching.sb = 1;
    else
        autoplay.catl.pitching.sb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpcs)))
        autoplay.catl.pitching.cs = 1;
    else
        autoplay.catl.pitching.cs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpcs)))
        autoplay.catl.pitching.cs = 1;
    else
        autoplay.catl.pitching.cs = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpbb)))
        autoplay.catl.pitching.bb = 1;
    else
        autoplay.catl.pitching.bb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpso)))
        autoplay.catl.pitching.so = 1;
    else
        autoplay.catl.pitching.so = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpibb)))
        autoplay.catl.pitching.ibb = 1;
    else
        autoplay.catl.pitching.ibb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpsh)))
        autoplay.catl.pitching.sh = 1;
    else
        autoplay.catl.pitching.sh = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpsf)))
        autoplay.catl.pitching.sf = 1;
    else
        autoplay.catl.pitching.sf = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpwp)))
        autoplay.catl.pitching.wp = 1;
    else
        autoplay.catl.pitching.wp = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpb)))
        autoplay.catl.pitching.b = 1;
    else
        autoplay.catl.pitching.b = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonphb)))
        autoplay.catl.pitching.hb = 1;
    else
        autoplay.catl.pitching.hb = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpab)))
        autoplay.catl.pitching.ab = 1;
    else
        autoplay.catl.pitching.ab = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpera)))
        autoplay.catl.pitching.era = 1;
    else
        autoplay.catl.pitching.era = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonppct)))
        autoplay.catl.pitching.pct = 1;
    else
        autoplay.catl.pitching.pct = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonpoba)))
        autoplay.catl.pitching.oppba = 1;
    else
        autoplay.catl.pitching.oppba = 0;

    for (x = 0; x < 7; x++) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfg[x])))
            autoplay.catl.fielding.pos[x].games = 1;
        else
            autoplay.catl.fielding.pos[x].games = 0;
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfpo[x])))
            autoplay.catl.fielding.pos[x].po = 1;
        else
            autoplay.catl.fielding.pos[x].po = 0;
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfdp[x])))
            autoplay.catl.fielding.pos[x].dp = 1;
        else
            autoplay.catl.fielding.pos[x].dp = 0;
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfa[x])))
            autoplay.catl.fielding.pos[x].a = 1;
        else
            autoplay.catl.fielding.pos[x].a = 0;
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfe[x])))
            autoplay.catl.fielding.pos[x].e = 1;
        else
            autoplay.catl.fielding.pos[x].e = 0;
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfpct[x])))
            autoplay.catl.fielding.pos[x].pct = 1;
        else
            autoplay.catl.fielding.pos[x].pct = 0;

    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfg[7])))
        autoplay.catl.fielding.pos[7].games = 1;
    else
        autoplay.catl.fielding.pos[7].games = 0;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (buttonfpb)))
        autoplay.catl.fielding.pos[6].pb = 1;
    else
        autoplay.catl.fielding.pos[6].pb = 0;

    /* write the user's autoplay set-up file */
    path2autoplay[0] = '\0';
    strcpy (&path2autoplay[0], getenv ("HOME"));
    strcat (&path2autoplay[0], "/.NSBautoplayrc");

    if ((rc = fopen (path2autoplay, "w")) != NULL) {
        fwrite (&autoplay, sizeof autoplay, 1, rc);
        fclose (rc);
    }
    else {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;
        msg[0] = ERR;
        outMessage (msg);
    }

    DestroyDialog (dlgFile, dlgFile);
}

void
SeasonAutoplayAS (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    char labeltext[500];
    GtkWidget *label, *button, *buttons;

    dlgFile = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dlgFile), "Autoplay Activation");

    if (autoplay.active)
        strcpy (&labeltext[0], "Autoplay is currently active.");
    else
        strcpy (&labeltext[0], "Autoplay is currently suspended.");

    label = gtk_label_new (labeltext);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->vbox), label, TRUE, TRUE, 0);

    button = gtk_button_new_with_label ("CANCEL");
    gtk_signal_connect (GTK_OBJECT (button), "clicked", (GtkSignalFunc) DestroyDialog, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), button, TRUE, TRUE, 15);
    if (!autoplay.active)
        buttons = gtk_button_new_with_label ("ACTIVATE AutoPlay");
    else
        buttons = gtk_button_new_with_label ("SUSPEND AutoPlay");
    gtk_signal_connect (GTK_OBJECT (buttons), "clicked", (GtkSignalFunc) ActSusp, dlgFile);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlgFile)->action_area), buttons, TRUE, TRUE, 15);
    gtk_widget_show_all (dlgFile);
}

void
ActSusp (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar path2autoplay[1024], *msg[5], *ERR = "The .NSBautoplayrc file cannot be written.";
    FILE *rc;

    if (autoplay.active)
        autoplay.active = 0;
    else
        autoplay.active = 1;

    /* write the user's autoplay set-up file */
    path2autoplay[0] = '\0';
    strcpy (&path2autoplay[0], getenv ("HOME"));
    strcat (&path2autoplay[0], "/.NSBautoplayrc");

    if ((rc = fopen (path2autoplay, "w")) != NULL) {
        fwrite (&autoplay, sizeof autoplay, 1, rc);
        fclose (rc);
    }
    else {
        for (x = 0; x < 5; x++)
            msg[x] = NULL;
        msg[0] = ERR;
        outMessage (msg);
    }

    DestroyDialog (dlgFile, dlgFile);
}

void
SeasonAutoplayEx (gpointer callback_data, guint callback_action, GtkWidget *widget) {
    gint x, ret;
    gchar *msg[5], *ERR1 = "You are not connected to a NSB server.", *Success1 = "Autoplay initiated.", cmd[1024],
          *ERR2 = "Use Game->Connect to a Server to connect to the server where you want to autoplay.",
          *ERR3 = "Could not initiate Autoplay.";

    for (x = 0; x < 5; x++)
        msg[x] = NULL;

    if (connected) {
        strcpy (&cmd[0], "gtknsbclient LEAGUEAUTOPLAY ");
        strcat (&cmd[0], &nsbid[0]);
        strcat (&cmd[0], " ");
        strcat (&cmd[0], &hs[0]);
        ret = system (cmd);
        if (ret) {
            msg[0] = ERR3;
            outMessage (msg);

            strcpy (&work[0], "Could not initiate Autoplay.\n");
            Add2TextWindow (&work[0], 1);

            syslog (LOG_INFO, "could not initiate Autoplay from within NSB client");
        }
        else {
            msg[0] = Success1;
            outMessage (msg);

            strcpy (&work[0], "Initiate Autoplay.\n");
            Add2TextWindow (&work[0], 0);
        }
    }
    else {
        msg[0] = ERR1;
        msg[1] = ERR2;
        outMessage (msg);
    }
}

void
SetButtons () {
    if (autoplay.linescore)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonls), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonls), FALSE);
    if (autoplay.boxscore)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbs), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbs), FALSE);
    if (autoplay.standings)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonstd), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonstd), FALSE);
    if (autoplay.injury)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttoninj), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttoninj), FALSE);

    if (autoplay.catl.hitting.games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbg), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbg), FALSE);
    if (autoplay.catl.hitting.atbats)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbab), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbab), FALSE);
    if (autoplay.catl.hitting.runs)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbr), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbr), FALSE);
    if (autoplay.catl.hitting.hits)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbh), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbh), FALSE);
    if (autoplay.catl.hitting.doubles)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonb2b), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonb2b), FALSE);
    if (autoplay.catl.hitting.triples)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonb3b), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonb3b), FALSE);
    if (autoplay.catl.hitting.homers)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbhr), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbhr), FALSE);
    if (autoplay.catl.hitting.rbis)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbbi), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbbi), FALSE);
    if (autoplay.catl.hitting.bb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbbb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbbb), FALSE);
    if (autoplay.catl.hitting.so)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbso), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbso), FALSE);
    if (autoplay.catl.hitting.hbp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbhbp), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbhbp), FALSE);
    if (autoplay.catl.hitting.dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbdp), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbdp), FALSE);
    if (autoplay.catl.hitting.sb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsb), FALSE);
    if (autoplay.catl.hitting.cs)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbcs), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbcs), FALSE);
    if (autoplay.catl.hitting.ibb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbibb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbibb), FALSE);
    if (autoplay.catl.hitting.sh)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsh), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsh), FALSE);
    if (autoplay.catl.hitting.sf)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsf), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsf), FALSE);
    if (autoplay.catl.hitting.ba)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbba), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbba), FALSE);
    if (autoplay.catl.hitting.sa)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsa), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonbsa), FALSE);
    if (autoplay.catl.hitting.oba)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonboba), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonboba), FALSE);

    if (autoplay.catl.pitching.games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpg), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpg), FALSE);
    if (autoplay.catl.pitching.gs)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpgs), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpgs), FALSE);
    if (autoplay.catl.pitching.ip)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpip), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpip), FALSE);
    if (autoplay.catl.pitching.wins)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpw), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpw), FALSE);
    if (autoplay.catl.pitching.losses)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpl), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpl), FALSE);
    if (autoplay.catl.pitching.sv)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonps), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonps), FALSE);
    if (autoplay.catl.pitching.bfp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbfp), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbfp), FALSE);
    if (autoplay.catl.pitching.hits)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonph), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonph), FALSE);
    if (autoplay.catl.pitching.db)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonp2b), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonp2b), FALSE);
    if (autoplay.catl.pitching.tp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonp3b), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonp3b), FALSE);
    if (autoplay.catl.pitching.hr)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonphr), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonphr), FALSE);
    if (autoplay.catl.pitching.runs)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpr), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpr), FALSE);
    if (autoplay.catl.pitching.er)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonper), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonper), FALSE);
    if (autoplay.catl.pitching.rbi)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbi), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbi), FALSE);
    if (autoplay.catl.pitching.cg)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpcg), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpcg), FALSE);
    if (autoplay.catl.pitching.gf)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpgf), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpgf), FALSE);
    if (autoplay.catl.pitching.sho)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsho), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsho), FALSE);
    if (autoplay.catl.pitching.svopp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsopp), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsopp), FALSE);
    if (autoplay.catl.pitching.sb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsb), FALSE);
    if (autoplay.catl.pitching.cs)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpcs), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpcs), FALSE);
    if (autoplay.catl.pitching.bb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpbb), FALSE);
    if (autoplay.catl.pitching.so)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpso), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpso), FALSE);
    if (autoplay.catl.pitching.ibb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpibb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpibb), FALSE);
    if (autoplay.catl.pitching.sh)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsh), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsh), FALSE);
    if (autoplay.catl.pitching.sf)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsf), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpsf), FALSE);
    if (autoplay.catl.pitching.wp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpwp), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpwp), FALSE);
    if (autoplay.catl.pitching.b)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpb), FALSE);
    if (autoplay.catl.pitching.hb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonphb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonphb), FALSE);
    if (autoplay.catl.pitching.ab)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpab), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpab), FALSE);
    if (autoplay.catl.pitching.era)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpera), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpera), FALSE);
    if (autoplay.catl.pitching.pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonppct), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonppct), FALSE);
    if (autoplay.catl.pitching.oppba)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpoba), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonpoba), FALSE);

    if (autoplay.catl.fielding.pos[0].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[0]), FALSE);
    if (autoplay.catl.fielding.pos[0].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[0]), FALSE);
    if (autoplay.catl.fielding.pos[0].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[0]), FALSE);
    if (autoplay.catl.fielding.pos[0].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[0]), FALSE);
    if (autoplay.catl.fielding.pos[0].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[0]), FALSE);
    if (autoplay.catl.fielding.pos[0].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[0]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[0]), FALSE);

    if (autoplay.catl.fielding.pos[1].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[1]), FALSE);
    if (autoplay.catl.fielding.pos[1].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[1]), FALSE);
    if (autoplay.catl.fielding.pos[1].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[1]), FALSE);
    if (autoplay.catl.fielding.pos[1].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[1]), FALSE);
    if (autoplay.catl.fielding.pos[1].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[1]), FALSE);
    if (autoplay.catl.fielding.pos[1].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[1]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[1]), FALSE);

    if (autoplay.catl.fielding.pos[2].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[2]), FALSE);
    if (autoplay.catl.fielding.pos[2].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[2]), FALSE);
    if (autoplay.catl.fielding.pos[2].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[2]), FALSE);
    if (autoplay.catl.fielding.pos[2].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[2]), FALSE);
    if (autoplay.catl.fielding.pos[2].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[2]), FALSE);
    if (autoplay.catl.fielding.pos[2].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[2]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[2]), FALSE);

    if (autoplay.catl.fielding.pos[3].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[3]), FALSE);
    if (autoplay.catl.fielding.pos[3].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[3]), FALSE);
    if (autoplay.catl.fielding.pos[3].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[3]), FALSE);
    if (autoplay.catl.fielding.pos[3].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[3]), FALSE);
    if (autoplay.catl.fielding.pos[3].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[3]), FALSE);
    if (autoplay.catl.fielding.pos[3].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[3]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[3]), FALSE);

    if (autoplay.catl.fielding.pos[4].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[4]), FALSE);
    if (autoplay.catl.fielding.pos[4].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[4]), FALSE);
    if (autoplay.catl.fielding.pos[4].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[4]), FALSE);
    if (autoplay.catl.fielding.pos[4].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[4]), FALSE);
    if (autoplay.catl.fielding.pos[4].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[4]), FALSE);
    if (autoplay.catl.fielding.pos[4].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[4]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[4]), FALSE);

    if (autoplay.catl.fielding.pos[5].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[5]), FALSE);
    if (autoplay.catl.fielding.pos[5].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[5]), FALSE);
    if (autoplay.catl.fielding.pos[5].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[5]), FALSE);
    if (autoplay.catl.fielding.pos[5].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[5]), FALSE);
    if (autoplay.catl.fielding.pos[5].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[5]), FALSE);
    if (autoplay.catl.fielding.pos[5].pct)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[5]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpct[5]), FALSE);

    if (autoplay.catl.fielding.pos[6].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[6]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[6]), FALSE);
    if (autoplay.catl.fielding.pos[6].po)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[6]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpo[6]), FALSE);
    if (autoplay.catl.fielding.pos[6].dp)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[6]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfdp[6]), FALSE);
    if (autoplay.catl.fielding.pos[6].a)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[6]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfa[6]), FALSE);
    if (autoplay.catl.fielding.pos[6].e)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[6]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfe[6]), FALSE);
    if (autoplay.catl.fielding.pos[6].pb)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpb), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfpb), FALSE);

    if (autoplay.catl.fielding.pos[7].games)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[7]), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonfg[7]), FALSE);
}

void
CancelOptions (GtkWidget *widget, gpointer *pdata) {
    gint x;
    gchar path2ap[1024];
    FILE *rc;

    /* reload the NSB AUTOPLAY rc file if it exists */
    path2ap[0] = '\0';
    strcpy (&path2ap[0], getenv ("HOME"));
    strcat (&path2ap[0], "/.NSBautoplayrc");

    if ((rc = fopen (path2ap, "r")) != NULL) {
        x = fread (&autoplay, sizeof autoplay, 1, rc);
        fclose (rc);
    }
    else {
        autoplay.active = autoplay.linescore = autoplay.boxscore = autoplay.standings = autoplay.injury = 0;
        autoplay.catl.hitting.games = autoplay.catl.hitting.atbats = autoplay.catl.hitting.runs = autoplay.catl.hitting.hits = autoplay.catl.hitting.doubles =
          autoplay.catl.hitting.triples = autoplay.catl.hitting.homers = autoplay.catl.hitting.rbis = autoplay.catl.hitting.bb = autoplay.catl.hitting.so =
          autoplay.catl.hitting.hbp = autoplay.catl.hitting.dp = autoplay.catl.hitting.sb = autoplay.catl.hitting.cs = autoplay.catl.hitting.ibb =
          autoplay.catl.hitting.sh = autoplay.catl.hitting.sf = autoplay.catl.hitting.ba = autoplay.catl.hitting.sa = autoplay.catl.hitting.oba = 0;
        autoplay.catl.pitching.games = autoplay.catl.pitching.gs = autoplay.catl.pitching.ip = autoplay.catl.pitching.wins = autoplay.catl.pitching.losses =
          autoplay.catl.pitching.sv = autoplay.catl.pitching.bfp = autoplay.catl.pitching.hits = autoplay.catl.pitching.db = autoplay.catl.pitching.tp =
          autoplay.catl.pitching.hr = autoplay.catl.pitching.runs = autoplay.catl.pitching.er = autoplay.catl.pitching.rbi = autoplay.catl.pitching.cg =
          autoplay.catl.pitching.gf = autoplay.catl.pitching.sho = autoplay.catl.pitching.svopp = autoplay.catl.pitching.sb = autoplay.catl.pitching.cs =
          autoplay.catl.pitching.bb = autoplay.catl.pitching.so = autoplay.catl.pitching.ibb = autoplay.catl.pitching.sh = autoplay.catl.pitching.sf =
          autoplay.catl.pitching.wp = autoplay.catl.pitching.b = autoplay.catl.pitching.hb = autoplay.catl.pitching.ab = autoplay.catl.pitching.era =
          autoplay.catl.pitching.pct = autoplay.catl.pitching.oppba = 0;
        for (x = 0; x < 7; x++)
            autoplay.catl.fielding.pos[x].games = autoplay.catl.fielding.pos[x].po = autoplay.catl.fielding.pos[x].dp = autoplay.catl.fielding.pos[x].a =
              autoplay.catl.fielding.pos[x].e = autoplay.catl.fielding.pos[x].pct = 0;
        autoplay.catl.fielding.pos[7].games = autoplay.catl.fielding.pos[6].pb = 0;
    }

    DestroyDialog (dlgFile, dlgFile);
}

