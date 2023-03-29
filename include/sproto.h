#ifndef _SPROTO_H_
#define _SPROTO_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void populate (), cp_stat (), do_schedule (int, char), get_teams (), version (), usage (), sig_chld (), closeup (), chk_wls (), send_stats (int, char), setup_cgs (), zero_home_season (), zero_visitor_season (),
     clear_stats (), sel_order (), determine_battingorder (char), exit (), combine_stats (), move_runners (), determine_starters (char), send_cum_stats (int, char *), force_runners_ahead (), runner_cutdown (),
     runners_adv (), switch_name (), check_for_injury (), clr_vars (), accumulate_stats (), update_more_stats (), update_stats (), update_status (), send_boxscore (), sortleaders (int, char), upd_fielding_stats (),
     check_for_errors (), find_fielder (), send_bothalf (), action (), send_tophalf (), build_game_status (), cp_rec (), update_records (), put_records (), get_records (), playthegame (), *communicate (void *),
     eb_off_checks (), replace_pitcher (), replace_def (), update_some_stats (), check_dec (), check_save (), eb_def_checks (), boi_def_checks (), compai (), send_lineup_info2 (), rep_plyr3 (), rep_plyr2 (),
     rep_plyr (), humanaction (), kill_league (), get_lifetime_records (), put_lifetime_records (), update_lifetime_records (), get_server_records (), update_server_records (), update_sch_ps (), chk_off (),
     Look4OnePlayerPos (int), sorthitting (int, int), sortpitching (int, int), sortfielding (int, int, int), send_lineup (), send_DOB (), sortlhitting (int, int), sortlpitching (int, int),
     sortlfielding (int, int, int), Send2BotHalf (int), sortshitting (int, int), sortspitching (int, int), sortsfielding (int, int, int);

int chk_eos (), play_league_game (), main (), cmp_stat (), get_userinfo (), get_rl_home (), get_rl_vis (), get_stats (int), select_player (), cmp_rec (), replace_player2 (), replace_player (), send_lineup_info (),
    off_choices (), formula (), def_choices (), mkdir (), sock_gets (), sock_puts (), available_pitcher (), GetTotGames (int, int), CreateTeam (), setup_postseason (), chk_eops (), available_player (), PlayoffGame ();

double fabs (double);
float fabsf (float);
char *cnvt_int2str (), *GetTeamName (int), *ReturnDefName (int), *GetUCTeamname (int), *strcasestr (const char *, const char *);
float do_divide ();

#endif

